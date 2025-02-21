/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "RdbServiceImpl"
#include "rdb_service_impl.h"

#include "abs_rdb_predicates.h"
#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "changeevent/remote_change_event.h"
#include "checker/checker_manager.h"
#include "cloud/change_event.h"
#include "cloud/cloud_lock_event.h"
#include "cloud/cloud_share_event.h"
#include "cloud/make_query_event.h"
#include "cloud/schema_meta.h"
#include "communicator/device_manager_adapter.h"
#include "crypto_manager.h"
#include "device_matrix.h"
#include "directory/directory_manager.h"
#include "dump/dump_manager.h"
#include "eventcenter/event_center.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "metadata/appid_meta_data.h"
#include "metadata/auto_launch_meta_data.h"
#include "metadata/capability_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_debug_info.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "rdb_general_store.h"
#include "rdb_notifier_proxy.h"
#include "rdb_query.h"
#include "rdb_schema_config.h"
#include "rdb_result_set_impl.h"
#include "rdb_watcher.h"
#include "store/general_store.h"
#include "tokenid_kit.h"
#include "types_export.h"
#include "utils/anonymous.h"
#include "utils/constant.h"
#include "utils/converter.h"
#include "xcollie.h"
#include "permit_delegate.h"
#include "bootstrap.h"
#include "rdb_hiview_adapter.h"
using OHOS::DistributedData::Anonymous;
using OHOS::DistributedData::CheckerManager;
using OHOS::DistributedData::MetaDataManager;
using OHOS::DistributedData::StoreMetaData;
using OHOS::DistributedData::AccountDelegate;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
using DistributedDB::RelationalStoreManager;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using RdbSchemaConfig = OHOS::DistributedRdb::RdbSchemaConfig;
using DumpManager = OHOS::DistributedData::DumpManager;
using system_clock = std::chrono::system_clock;

constexpr uint32_t ITERATE_TIMES = 10000;
constexpr uint32_t ALLOW_ONLINE_AUTO_SYNC = 8;
const size_t KEY_COUNT = 2;
namespace OHOS::DistributedRdb {
__attribute__((used)) RdbServiceImpl::Factory RdbServiceImpl::factory_;
RdbServiceImpl::Factory::Factory()
{
    FeatureSystem::GetInstance().RegisterCreator(RdbServiceImpl::SERVICE_NAME, [this]() {
        if (product_ == nullptr) {
            product_ = std::make_shared<RdbServiceImpl>();
        }
        return product_;
    });
    AutoCache::GetInstance().RegCreator(RDB_DEVICE_COLLABORATION, [](const StoreMetaData& metaData) -> GeneralStore* {
        auto store = new (std::nothrow) RdbGeneralStore(metaData);
        if (store != nullptr && !store->IsValid()) {
            delete store;
            store = nullptr;
        }
        return store;
    });
    staticActs_ = std::make_shared<RdbStatic>();
    FeatureSystem::GetInstance().RegisterStaticActs(RdbServiceImpl::SERVICE_NAME, staticActs_);
}

RdbServiceImpl::Factory::~Factory()
{
}

RdbServiceImpl::RdbServiceImpl()
{
    ZLOGI("construct");
    DistributedDB::RelationalStoreManager::SetAutoLaunchRequestCallback(
        [this](const std::string& identifier, DistributedDB::AutoLaunchParam &param) {
            return ResolveAutoLaunch(identifier, param);
        });
    auto process = [this](const Event &event) {
        auto &evt = static_cast<const CloudEvent &>(event);
        auto &storeInfo = evt.GetStoreInfo();
        StoreMetaData meta(storeInfo);
        meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
        if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true)) {
            ZLOGE("meta empty, bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
                meta.GetStoreAlias().c_str());
            return;
        }
        if (meta.storeType < StoreMetaData::STORE_RELATIONAL_BEGIN ||
            meta.storeType > StoreMetaData::STORE_RELATIONAL_END) {
            return;
        }
        auto watchers = GetWatchers(meta.tokenId, meta.storeId);
        auto store = AutoCache::GetInstance().GetStore(meta, watchers);
        if (store == nullptr) {
            ZLOGE("store null, storeId:%{public}s", meta.GetStoreAlias().c_str());
            return;
        }
        store->RegisterDetailProgressObserver(GetCallbacks(meta.tokenId, storeInfo.storeName));
    };
    EventCenter::GetInstance().Subscribe(CloudEvent::CLOUD_SYNC, process);
    EventCenter::GetInstance().Subscribe(CloudEvent::CLEAN_DATA, process);

    EventCenter::GetInstance().Subscribe(CloudEvent::MAKE_QUERY, [](const Event& event) {
        auto& evt = static_cast<const MakeQueryEvent&>(event);
        auto callback = evt.GetCallback();
        if (!callback) {
            return;
        }
        auto predicate = evt.GetPredicates();
        auto rdbQuery = std::make_shared<RdbQuery>();
        rdbQuery->MakeQuery(*predicate);
        rdbQuery->SetColumns(evt.GetColumns());
        callback(rdbQuery);
    });
    auto compensateSyncProcess = [this] (const Event &event) {
        auto &evt = static_cast<const BindEvent &>(event);
        DoCompensateSync(evt);
    };
    EventCenter::GetInstance().Subscribe(BindEvent::COMPENSATE_SYNC, compensateSyncProcess);
    EventCenter::GetInstance().Subscribe(BindEvent::RECOVER_SYNC, compensateSyncProcess);
}

int32_t RdbServiceImpl::ResolveAutoLaunch(const std::string &identifier, DistributedDB::AutoLaunchParam &param)
{
    std::string identifierHex = TransferStringToHex(identifier);
    ZLOGI("%{public}.6s", identifierHex.c_str());
    std::vector<StoreMetaData> entries;
    auto localId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (!MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ localId }), entries)) {
        ZLOGE("get meta failed");
        return false;
    }
    ZLOGI("size=%{public}d", static_cast<int32_t>(entries.size()));
    for (const auto& entry : entries) {
        if (entry.storeType != RDB_DEVICE_COLLABORATION) {
            continue;
        }

        auto aIdentifier = DistributedDB::RelationalStoreManager::GetRelationalStoreIdentifier(
            "", entry.appId, entry.storeId, true);
        ZLOGD("%{public}s %{public}s %{public}s",
            entry.user.c_str(), entry.appId.c_str(), Anonymous::Change(entry.storeId).c_str());
        if (aIdentifier != identifier) {
            continue;
        }
        ZLOGI("find identifier %{public}s", Anonymous::Change(entry.storeId).c_str());
        param.userId = entry.user;
        param.appId = entry.appId;
        param.storeId = entry.storeId;
        param.path = entry.dataDir;
        param.option.storeObserver = nullptr;
        param.option.isEncryptedDb = entry.isEncrypt;
        if (entry.isEncrypt) {
            param.option.iterateTimes = ITERATE_TIMES;
            param.option.cipher = DistributedDB::CipherType::AES_256_GCM;
            GetDBPassword(entry, param.option.passwd);
        }
        AutoCache::GetInstance().GetStore(entry, GetWatchers(entry.tokenId, entry.storeId));
        return true;
    }
    ZLOGE("not find identifier");
    return false;
}

int32_t RdbServiceImpl::OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &bundleName)
{
    ZLOGI("client dead, tokenId:%{public}d, pid:%{public}d ", tokenId, pid);
    bool destroyed = false;
    syncAgents_.ComputeIfPresent(tokenId, [pid, &destroyed](auto &key, SyncAgents &agents) {
        auto it = agents.find(pid);
        if (it != agents.end()) {
            it->second.SetNotifier(nullptr);
            agents.erase(it);
        }
        if (!agents.empty()) {
            return true;
        }
        destroyed = true;
        return false;
    });
    if (destroyed) {
        auto stores = AutoCache::GetInstance().GetStoresIfPresent(tokenId);
        for (auto store : stores) {
            if (store != nullptr) {
                store->UnregisterDetailProgressObserver();
            }
        }
        AutoCache::GetInstance().Enable(tokenId);
    }
    heartbeatTaskIds_.Erase(pid);
    return E_OK;
}

bool RdbServiceImpl::CheckAccess(const std::string& bundleName, const std::string& storeName)
{
    CheckerManager::StoreInfo storeInfo;
    storeInfo.uid = IPCSkeleton::GetCallingUid();
    storeInfo.tokenId = IPCSkeleton::GetCallingTokenID();
    storeInfo.bundleName = bundleName;
    storeInfo.storeId = RemoveSuffix(storeName);
    return !CheckerManager::GetInstance().GetAppId(storeInfo).empty();
}

std::string RdbServiceImpl::ObtainDistributedTableName(const std::string &device, const std::string &table)
{
    ZLOGI("device=%{public}s table=%{public}s", Anonymous::Change(device).c_str(), table.c_str());
    auto uuid = DmAdapter::GetInstance().GetUuidByNetworkId(device);
    if (uuid.empty()) {
        ZLOGE("get uuid failed");
        return "";
    }
    return DistributedDB::RelationalStoreManager::GetDistributedTableName(uuid, table);
}

int32_t RdbServiceImpl::InitNotifier(const RdbSyncerParam &param, const sptr<IRemoteObject> notifier)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    if (!CheckAccess(param.bundleName_, "")) {
        ZLOGE("bundleName:%{public}s. Permission error", param.bundleName_.c_str());
        return RDB_ERROR;
    }
    if (notifier == nullptr) {
        ZLOGE("notifier is null");
        return RDB_ERROR;
    }

    auto notifierProxy = iface_cast<RdbNotifierProxy>(notifier);
    pid_t pid = IPCSkeleton::GetCallingPid();
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    syncAgents_.Compute(tokenId, [bundleName = param.bundleName_, notifierProxy, pid](auto, SyncAgents &agents) {
        auto [it, success] = agents.try_emplace(pid, SyncAgent(bundleName));
        if (it == agents.end()) {
            return true;
        }
        it->second.SetNotifier(notifierProxy);
        return true;
    });
    ZLOGI("success tokenId:%{public}x, pid=%{public}d", tokenId, pid);
    return RDB_OK;
}

std::shared_ptr<DistributedData::GeneralStore> RdbServiceImpl::GetStore(const RdbSyncerParam &param)
{
    StoreMetaData storeMetaData = GetStoreMetaData(param);
    MetaDataManager::GetInstance().LoadMeta(storeMetaData.GetKey(), storeMetaData, true);
    auto watchers = GetWatchers(storeMetaData.tokenId, storeMetaData.storeId);
    auto store = AutoCache::GetInstance().GetStore(storeMetaData, watchers);
    if (store == nullptr) {
        ZLOGE("store null, storeId:%{public}s", storeMetaData.GetStoreAlias().c_str());
    }
    return store;
}

void RdbServiceImpl::UpdateMeta(const StoreMetaData &meta, const StoreMetaData &localMeta, AutoCache::Store store)
{
    StoreMetaData syncMeta;
    bool isCreatedSync = MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), syncMeta);
    if (!isCreatedSync || localMeta != syncMeta) {
        ZLOGI("save sync meta. bundle:%{public}s store:%{public}s type:%{public}d->%{public}d "
              "encrypt:%{public}d->%{public}d , area:%{public}d->%{public}d",
            meta.bundleName.c_str(), meta.GetStoreAlias().c_str(), syncMeta.storeType, meta.storeType,
            syncMeta.isEncrypt, meta.isEncrypt, syncMeta.area, meta.area);
        MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), localMeta);
    }
    Database dataBase;
    if (RdbSchemaConfig::GetDistributedSchema(localMeta, dataBase) && !dataBase.name.empty() &&
        !dataBase.bundleName.empty()) {
        MetaDataManager::GetInstance().SaveMeta(dataBase.GetKey(), dataBase, true);
        store->SetConfig({false, GeneralStore::DistributedTableMode::COLLABORATION});
    }
}

int32_t RdbServiceImpl::SetDistributedTables(const RdbSyncerParam &param, const std::vector<std::string> &tables,
    const std::vector<Reference> &references, bool isRebuild, int32_t type)
{
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. Permission error", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return RDB_ERROR;
    }
    if (type == DistributedTableType::DISTRIBUTED_SEARCH) {
        DistributedData::SetSearchableEvent::EventInfo eventInfo;
        eventInfo.isRebuild = isRebuild;
        return PostSearchEvent(CloudEvent::SET_SEARCH_TRIGGER, param, eventInfo);
    }
    auto meta = GetStoreMetaData(param);
    StoreMetaData localMeta;
    bool isCreatedLocal = MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), localMeta, true);
    if (!isCreatedLocal) {
        ZLOGE("no meta. bundleName:%{public}s, storeName:%{public}s. GetStore failed", param.bundleName_.c_str(),
              Anonymous::Change(param.storeName_).c_str());
        return RDB_ERROR;
    }
    auto store = GetStore(param);
    if (store == nullptr) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. GetStore failed", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return RDB_ERROR;
    }
    if (type == DistributedTableType::DISTRIBUTED_DEVICE) {
        UpdateMeta(meta, localMeta, store);
    } else if (type == DistributedTableType::DISTRIBUTED_CLOUD) {
        if (localMeta.asyncDownloadAsset != param.asyncDownloadAsset_ || localMeta.enableCloud != param.enableCloud_) {
            ZLOGI("update meta, bundleName:%{public}s, storeName:%{public}s, asyncDownloadAsset? [%{public}d -> "
                "%{public}d],enableCloud? [%{public}d -> %{public}d]", param.bundleName_.c_str(),
                Anonymous::Change(param.storeName_).c_str(), localMeta.asyncDownloadAsset, param.asyncDownloadAsset_,
                localMeta.enableCloud, param.enableCloud_);
            localMeta.asyncDownloadAsset = param.asyncDownloadAsset_;
            localMeta.enableCloud = param.enableCloud_;
            MetaDataManager::GetInstance().SaveMeta(localMeta.GetKey(), localMeta, true);
        }
    }
    std::vector<DistributedData::Reference> relationships;
    for (const auto &reference : references) {
        DistributedData::Reference relationship = { reference.sourceTable, reference.targetTable, reference.refFields };
        relationships.emplace_back(relationship);
    }
    return store->SetDistributedTables(tables, type, relationships);
}

void RdbServiceImpl::OnAsyncComplete(uint32_t tokenId, pid_t pid, uint32_t seqNum, Details &&result)
{
    ZLOGI("tokenId=%{public}x, pid=%{public}d, seqnum=%{public}u", tokenId, pid, seqNum);
    sptr<RdbNotifierProxy> notifier = nullptr;
    syncAgents_.ComputeIfPresent(tokenId, [&notifier, pid](auto, SyncAgents &syncAgents) {
        auto it = syncAgents.find(pid);
        if (it != syncAgents.end()) {
            notifier = it->second.notifier_;
        }
        return true;
    });
    if (notifier != nullptr) {
        notifier->OnComplete(seqNum, std::move(result));
    }
}

std::string RdbServiceImpl::TransferStringToHex(const std::string &origStr)
{
    if (origStr.empty()) {
        return "";
    }
    const char *hex = "0123456789abcdef";
    std::string tmp;
    for (auto item : origStr) {
        auto currentByte = static_cast<uint8_t>(item);
        tmp.push_back(hex[currentByte >> 4]); // high 4 bit to one hex.
        tmp.push_back(hex[currentByte & 0x0F]); // low 4 bit to one hex.
    }
    return tmp;
}

AutoCache::Watchers RdbServiceImpl::GetWatchers(uint32_t tokenId, const std::string &storeName)
{
    AutoCache::Watchers watchers;
    syncAgents_.ComputeIfPresent(tokenId, [&watchers](auto, SyncAgents &syncAgents) {
        std::for_each(syncAgents.begin(), syncAgents.end(), [&watchers](const auto &item) {
            if (item.second.watcher_ != nullptr) {
                watchers.insert(item.second.watcher_);
            }
        });
        return true;
    });
    return watchers;
}

RdbServiceImpl::DetailAsync RdbServiceImpl::GetCallbacks(uint32_t tokenId, const std::string &storeName)
{
    std::list<sptr<RdbNotifierProxy>> notifiers;
    syncAgents_.ComputeIfPresent(tokenId, [&storeName, &notifiers](auto, SyncAgents &syncAgents) {
        std::for_each(syncAgents.begin(), syncAgents.end(), [&storeName, &notifiers](const auto &item) {
            if (item.second.callBackStores_.count(storeName) != 0) {
                notifiers.push_back(item.second.notifier_);
            }
        });
        return true;
    });
    if (notifiers.empty()) {
        return nullptr;
    }
    return [notifiers, storeName](const GenDetails &details) {
        for (const auto &notifier : notifiers) {
            if (notifier != nullptr) {
                notifier->OnComplete(storeName, HandleGenDetails(details));
            }
        }
    };
}

std::pair<int32_t, std::shared_ptr<RdbServiceImpl::ResultSet>> RdbServiceImpl::RemoteQuery(const RdbSyncerParam& param,
    const std::string& device, const std::string& sql, const std::vector<std::string>& selectionArgs)
{
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. Permission error", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return { RDB_ERROR, nullptr };
    }
    auto store = GetStore(param);
    if (store == nullptr) {
        ZLOGE("store is null");
        return { RDB_ERROR, nullptr };
    }
    RdbQuery rdbQuery;
    rdbQuery.MakeRemoteQuery(DmAdapter::GetInstance().ToUUID(device), sql, ValueProxy::Convert(selectionArgs));
    auto [errCode, cursor] = store->Query("", rdbQuery);
    if (errCode != GeneralError::E_OK) {
        return { RDB_ERROR, nullptr };
    }
    return { RDB_OK, std::make_shared<RdbResultSetImpl>(cursor) };
}

int32_t RdbServiceImpl::Sync(const RdbSyncerParam &param, const Option &option, const PredicatesMemo &predicates,
                             const AsyncDetail &async)
{
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. Permission error", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return RDB_ERROR;
    }
    if (option.mode < DistributedData::GeneralStore::CLOUD_END &&
        option.mode >= DistributedData::GeneralStore::CLOUD_BEGIN) {
        DoCloudSync(param, option, predicates, async);
        return RDB_OK;
    }
    return DoSync(param, option, predicates, async);
}

int RdbServiceImpl::DoSync(const RdbSyncerParam &param, const RdbService::Option &option,
    const PredicatesMemo &predicates, const AsyncDetail &async)
{
    auto store = GetStore(param);
    if (store == nullptr) {
        return RDB_ERROR;
    }
    RdbQuery rdbQuery;
    rdbQuery.MakeQuery(predicates);
    auto devices = rdbQuery.GetDevices().empty() ? DmAdapter::ToUUID(DmAdapter::GetInstance().GetRemoteDevices())
                                                 : DmAdapter::ToUUID(rdbQuery.GetDevices());
    auto pid = IPCSkeleton::GetCallingPid();
    SyncParam syncParam = { option.mode, 0, option.isCompensation };
    StoreMetaData meta = GetStoreMetaData(param);
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    ZLOGD("seqNum=%{public}u", option.seqNum);
    auto complete = [this, rdbQuery, store, pid, syncParam, tokenId, async, seq = option.seqNum](
                        const auto &results) mutable {
        auto ret = ProcessResult(results);
        store->Sync(
            ret.first, rdbQuery,
            [this, tokenId, seq, pid](const GenDetails &result) mutable {
                OnAsyncComplete(tokenId, pid, seq, HandleGenDetails(result));
            },
            syncParam);
    };
    if (IsNeedMetaSync(meta, devices)) {
        auto result = MetaDataManager::GetInstance().Sync(devices, complete);
        return result ? GeneralError::E_OK : GeneralError::E_ERROR;
    }
    return store->Sync(
        devices, rdbQuery,
        [this, tokenId, pid, seqNum = option.seqNum](const GenDetails &result) mutable {
            OnAsyncComplete(tokenId, pid, seqNum, HandleGenDetails(result));
        },
        syncParam).first;
}

bool RdbServiceImpl::IsNeedMetaSync(const StoreMetaData &meta, const std::vector<std::string> &uuids)
{
    bool isAfterMeta = false;
    for (const auto &uuid : uuids) {
        auto metaData = meta;
        metaData.deviceId = uuid;
        CapMetaData capMeta;
        auto capKey = CapMetaRow::GetKeyFor(uuid);
        if (!MetaDataManager::GetInstance().LoadMeta(std::string(capKey.begin(), capKey.end()), capMeta) ||
            !MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData)) {
            isAfterMeta = true;
            break;
        }
        auto [exist, mask] = DeviceMatrix::GetInstance().GetRemoteMask(uuid);
        if ((mask & DeviceMatrix::META_STORE_MASK) == DeviceMatrix::META_STORE_MASK) {
            isAfterMeta = true;
            break;
        }
        auto [existLocal, localMask] = DeviceMatrix::GetInstance().GetMask(uuid);
        if ((localMask & DeviceMatrix::META_STORE_MASK) == DeviceMatrix::META_STORE_MASK) {
            isAfterMeta = true;
            break;
        }
    }
    return isAfterMeta;
}

RdbServiceImpl::SyncResult RdbServiceImpl::ProcessResult(const std::map<std::string, int32_t> &results)
{
    std::map<std::string, DBStatus> dbResults;
    std::vector<std::string> devices;
    for (const auto &[uuid, status] : results) {
        dbResults.insert_or_assign(uuid, static_cast<DBStatus>(status));
        if (static_cast<DBStatus>(status) != DBStatus::OK) {
            continue;
        }
        DeviceMatrix::GetInstance().OnExchanged(uuid, DeviceMatrix::META_STORE_MASK);
        devices.emplace_back(uuid);
    }
    ZLOGD("meta sync finish, total size:%{public}zu, success size:%{public}zu", dbResults.size(), devices.size());
    return { devices, dbResults };
}

void RdbServiceImpl::DoCompensateSync(const BindEvent& event)
{
    auto bindInfo = event.GetBindInfo();
    StoreInfo storeInfo;
    storeInfo.bundleName = bindInfo.bundleName;
    storeInfo.tokenId = bindInfo.tokenId;
    storeInfo.user = bindInfo.user;
    storeInfo.storeName = bindInfo.storeName;
    OHOS::NativeRdb::AbsRdbPredicates predicates(bindInfo.tableName);
    for (auto& [key, value] : bindInfo.primaryKey) {
        predicates.In(key, std::vector<NativeRdb::ValueObject>({ ValueProxy::Convert(std::move(value)) }));
    }
    auto memo = predicates.GetDistributedPredicates();
    std::shared_ptr<RdbQuery> query = nullptr;
    if (!memo.tables_.empty()) {
        query = std::make_shared<RdbQuery>();
        query->MakeCloudQuery(memo);
    }
    auto mixMode = event.GetEventId() == BindEvent::COMPENSATE_SYNC
                       ? GeneralStore::MixMode(TIME_FIRST, GeneralStore::AUTO_SYNC_MODE)
                       : GeneralStore::MixMode(CLOUD_FIRST, GeneralStore::AUTO_SYNC_MODE);
    auto info = ChangeEvent::EventInfo(mixMode, 0, false, query, nullptr);
    auto evt = std::make_unique<ChangeEvent>(std::move(storeInfo), std::move(info));
    EventCenter::GetInstance().PostEvent(std::move(evt));
}

void RdbServiceImpl::DoCloudSync(const RdbSyncerParam &param, const RdbService::Option &option,
    const PredicatesMemo &predicates, const AsyncDetail &async)
{
    StoreInfo storeInfo;
    storeInfo.bundleName = param.bundleName_;
    storeInfo.tokenId = IPCSkeleton::GetCallingTokenID();
    storeInfo.user = AccountDelegate::GetInstance()->GetUserByToken(storeInfo.tokenId);
    storeInfo.storeName = RemoveSuffix(param.storeName_);
    std::shared_ptr<RdbQuery> query = nullptr;
    if (!predicates.tables_.empty()) {
        query = std::make_shared<RdbQuery>();
        query->MakeCloudQuery(predicates);
    }
    auto pid = IPCSkeleton::GetCallingPid();
    GenAsync asyncCallback = [this, tokenId = storeInfo.tokenId, seqNum = option.seqNum, pid](
                                 const GenDetails &result) mutable {
        OnAsyncComplete(tokenId, pid, seqNum, HandleGenDetails(result));
    };
    GenAsync syncCallback = [async, &param](const GenDetails &details) {
        ZLOGD("Cloud Sync complete, bundleName:%{public}s, storeName:%{public}s", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        if (async != nullptr) {
            async(HandleGenDetails(details));
        }
    };
    auto highMode = (!predicates.tables_.empty() && option.mode == DistributedData::GeneralStore::CLOUD_ClOUD_FIRST)
                    ? GeneralStore::ASSETS_SYNC_MODE
                    : (option.isAutoSync ? GeneralStore::AUTO_SYNC_MODE : GeneralStore::MANUAL_SYNC_MODE);
    auto mixMode = static_cast<int32_t>(GeneralStore::MixMode(option.mode, highMode));
    SyncParam syncParam = { mixMode, (option.isAsync ? 0 : WAIT_TIME), option.isCompensation };
    syncParam.asyncDownloadAsset = param.asyncDownloadAsset_;
    auto info = ChangeEvent::EventInfo(syncParam, option.isAutoSync, query,
        option.isAutoSync ? nullptr
        : option.isAsync  ? asyncCallback
                          : syncCallback);
    auto evt = std::make_unique<ChangeEvent>(std::move(storeInfo), std::move(info));
    EventCenter::GetInstance().PostEvent(std::move(evt));
}

int32_t RdbServiceImpl::Subscribe(const RdbSyncerParam &param, const SubscribeOption &option,
    RdbStoreObserver *observer)
{
    if (option.mode < 0 || option.mode >= SUBSCRIBE_MODE_MAX) {
        ZLOGE("mode:%{public}d error", option.mode);
        return RDB_ERROR;
    }
    pid_t pid = IPCSkeleton::GetCallingPid();
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    bool isCreate = false;
    syncAgents_.Compute(tokenId, [pid, &param, &isCreate](auto &key, SyncAgents &agents) {
        auto [it, _] = agents.try_emplace(pid, param.bundleName_);
        if (it == agents.end()) {
            return !agents.empty();
        }
        if (it->second.watcher_ == nullptr) {
            isCreate = true;
            it->second.SetWatcher(std::make_shared<RdbWatcher>());
        }
        it->second.count_++;
        return true;
    });
    if (isCreate) {
        AutoCache::GetInstance().SetObserver(tokenId, RemoveSuffix(param.storeName_),
            GetWatchers(tokenId, param.storeName_));
    }
    return RDB_OK;
}

int32_t RdbServiceImpl::UnSubscribe(const RdbSyncerParam &param, const SubscribeOption &option,
    RdbStoreObserver *observer)
{
    if (option.mode < 0 || option.mode >= SUBSCRIBE_MODE_MAX) {
        ZLOGE("mode:%{public}d error", option.mode);
        return RDB_ERROR;
    }
    bool destroyed = false;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto pid = IPCSkeleton::GetCallingPid();
    syncAgents_.ComputeIfPresent(tokenId, [pid, &destroyed](auto, SyncAgents &agents) {
        auto it = agents.find(pid);
        if (it == agents.end()) {
            return !agents.empty();
        }
        it->second.count_--;
        if (it->second.count_ <= 0) {
            destroyed = true;
            it->second.SetWatcher(nullptr);
        }
        return true;
    });
    if (destroyed) {
        AutoCache::GetInstance().SetObserver(tokenId, RemoveSuffix(param.storeName_),
            GetWatchers(tokenId, param.storeName_));
    }
    return RDB_OK;
}

int32_t RdbServiceImpl::RegisterAutoSyncCallback(const RdbSyncerParam& param,
    std::shared_ptr<DetailProgressObserver> observer)
{
    pid_t pid = IPCSkeleton::GetCallingPid();
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto storeName = RemoveSuffix(param.storeName_);
    syncAgents_.Compute(tokenId, [pid, &param, &storeName](auto, SyncAgents &agents) {
        auto [it, success] = agents.try_emplace(pid, param.bundleName_);
        if (it == agents.end()) {
            return !agents.empty();
        }
        if (success) {
            it->second.callBackStores_.insert(std::make_pair(storeName, 0));
        }
        it->second.callBackStores_[storeName]++;
        return true;
    });
    return RDB_OK;
}

int32_t RdbServiceImpl::UnregisterAutoSyncCallback(const RdbSyncerParam& param,
    std::shared_ptr<DetailProgressObserver> observer)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto pid = IPCSkeleton::GetCallingPid();
    auto storeName = RemoveSuffix(param.storeName_);
    syncAgents_.ComputeIfPresent(tokenId, [pid, &storeName](auto, SyncAgents &agents) {
        auto agent = agents.find(pid);
        if (agent == agents.end()) {
            return !agents.empty();
        }
        auto it = agent->second.callBackStores_.find(storeName);
        if (it == agent->second.callBackStores_.end()) {
            return !agents.empty();
        }
        it->second--;
        if (it->second <= 0) {
            agent->second.callBackStores_.erase(it);
        }
        return !agents.empty();
    });
    return RDB_OK;
}

int32_t RdbServiceImpl::Delete(const RdbSyncerParam &param)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    AutoCache::GetInstance().CloseStore(tokenId, RemoveSuffix(param.storeName_));
    RdbSyncerParam tmpParam = param;
    HapTokenInfo hapTokenInfo;
    AccessTokenKit::GetHapTokenInfo(tokenId, hapTokenInfo);
    tmpParam.bundleName_ = hapTokenInfo.bundleName;
    auto storeMeta = GetStoreMetaData(tmpParam);
    MetaDataManager::GetInstance().DelMeta(storeMeta.GetKey());
    MetaDataManager::GetInstance().DelMeta(storeMeta.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(storeMeta.GetKeyLocal(), true);
    MetaDataManager::GetInstance().DelMeta(storeMeta.GetSecretKey(), true);
    MetaDataManager::GetInstance().DelMeta(storeMeta.GetStrategyKey());
    MetaDataManager::GetInstance().DelMeta(storeMeta.GetBackupSecretKey(), true);
    MetaDataManager::GetInstance().DelMeta(storeMeta.GetAutoLaunchKey(), true);
    MetaDataManager::GetInstance().DelMeta(storeMeta.GetDebugInfoKey(), true);
    MetaDataManager::GetInstance().DelMeta(storeMeta.GetCloneSecretKey(), true);
    return RDB_OK;
}

std::pair<int32_t, std::shared_ptr<RdbService::ResultSet>> RdbServiceImpl::QuerySharingResource(
    const RdbSyncerParam& param, const PredicatesMemo& predicates, const std::vector<std::string>& columns)
{
    if (!CheckAccess(param.bundleName_, param.storeName_) ||
        !TokenIdKit::IsSystemAppByFullTokenID(IPCSkeleton::GetCallingFullTokenID())) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. Permission error", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return { RDB_ERROR, {} };
    }
    if (predicates.tables_.empty()) {
        ZLOGE("tables is empty, bundleName:%{public}s, storeName:%{public}s", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return { RDB_ERROR, {} };
    }
    auto rdbQuery = std::make_shared<RdbQuery>();
    rdbQuery->MakeQuery(predicates);
    rdbQuery->SetColumns(columns);
    StoreInfo storeInfo;
    storeInfo.bundleName = param.bundleName_;
    storeInfo.tokenId = IPCSkeleton::GetCallingTokenID();
    storeInfo.user = AccountDelegate::GetInstance()->GetUserByToken(storeInfo.tokenId);
    storeInfo.storeName = RemoveSuffix(param.storeName_);
    auto [status, cursor] = AllocResource(storeInfo, rdbQuery);
    if (cursor == nullptr) {
        ZLOGE("cursor is null, bundleName:%{public}s, storeName:%{public}s", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return { RDB_ERROR, {} };
    }
    return { RDB_OK, std::make_shared<RdbResultSetImpl>(cursor) };
}

std::pair<int32_t, std::shared_ptr<Cursor>> RdbServiceImpl::AllocResource(StoreInfo& storeInfo,
    std::shared_ptr<RdbQuery> rdbQuery)
{
    std::pair<int32_t, std::shared_ptr<Cursor>> result;
    CloudShareEvent::Callback asyncCallback = [&result](int32_t status, std::shared_ptr<Cursor> cursor) {
        result.first = status;
        result.second = cursor;
    };
    auto evt = std::make_unique<CloudShareEvent>(std::move(storeInfo), rdbQuery, asyncCallback);
    EventCenter::GetInstance().PostEvent(std::move(evt));
    return result;
}

int32_t RdbServiceImpl::BeforeOpen(RdbSyncerParam &param)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. Permission error", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return RDB_ERROR;
    }
    auto meta = GetStoreMetaData(param);
    auto isCreated = MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true);
    if (!isCreated) {
        ZLOGW("bundleName:%{public}s, storeName:%{public}s. no meta", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return RDB_NO_META;
    }
    SetReturnParam(meta, param);
    return RDB_OK;
}

void RdbServiceImpl::SetReturnParam(StoreMetaData &metadata, RdbSyncerParam &param)
{
    param.bundleName_ = metadata.bundleName;
    param.type_ = metadata.storeType;
    param.level_ = metadata.securityLevel;
    param.area_ = metadata.area;
    param.hapName_ = metadata.hapName;
    param.customDir_ = metadata.customDir;
    param.isEncrypt_ = metadata.isEncrypt;
    param.isAutoClean_ = !metadata.isManualClean;
    param.isSearchable_ = metadata.isSearchable;
    param.haMode_ = metadata.haMode;
}

int32_t RdbServiceImpl::AfterOpen(const RdbSyncerParam &param)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. Permission error", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return RDB_ERROR;
    }
    auto meta = GetStoreMetaData(param);
    StoreMetaData old;
    auto isCreated = MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), old, true);
    if (!isCreated || meta != old) {
        Upgrade(param, old);
        ZLOGI("meta bundle:%{public}s store:%{public}s type:%{public}d->%{public}d encrypt:%{public}d->%{public}d "
              "area:%{public}d->%{public}d",
            meta.bundleName.c_str(), meta.GetStoreAlias().c_str(), old.storeType, meta.storeType,
            old.isEncrypt, meta.isEncrypt, old.area, meta.area);
        MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true);
        AutoLaunchMetaData launchData;
        if (!MetaDataManager::GetInstance().LoadMeta(meta.GetAutoLaunchKey(), launchData, true)) {
            RemoteChangeEvent::DataInfo info;
            info.bundleName = meta.bundleName;
            info.deviceId = meta.deviceId;
            info.userId = meta.user;
            auto evt = std::make_unique<RemoteChangeEvent>(RemoteChangeEvent::RDB_META_SAVE, std::move(info));
            EventCenter::GetInstance().PostEvent(std::move(evt));
        }
    }

    SaveDebugInfo(meta, param);
    SavePromiseInfo(meta, param);

    AppIDMetaData appIdMeta;
    appIdMeta.bundleName = meta.bundleName;
    appIdMeta.appId = meta.appId;
    if (!MetaDataManager::GetInstance().SaveMeta(appIdMeta.GetKey(), appIdMeta, true)) {
        ZLOGE("meta bundle:%{public}s store:%{public}s type:%{public}d->%{public}d encrypt:%{public}d->%{public}d "
              "area:%{public}d->%{public}d",
            meta.bundleName.c_str(), meta.GetStoreAlias().c_str(), old.storeType, meta.storeType,
            old.isEncrypt, meta.isEncrypt, old.area, meta.area);
        return RDB_ERROR;
    }
    if (param.isEncrypt_ && !param.password_.empty()) {
        auto ret = SetSecretKey(param, meta);
        if (ret != RDB_OK) {
            ZLOGE("Set secret key failed, bundle:%{public}s store:%{public}s",
                  meta.bundleName.c_str(), meta.GetStoreAlias().c_str());
            return RDB_ERROR;
        }
    }
    GetCloudSchema(param);
    return RDB_OK;
}

int32_t RdbServiceImpl::ReportStatistic(const RdbSyncerParam& param, const RdbStatEvent &statEvent)
{
    RdbHiViewAdapter::GetInstance().ReportStatistic(statEvent);
    return RDB_OK;
}

void RdbServiceImpl::GetCloudSchema(const RdbSyncerParam &param)
{
    if (executors_ != nullptr) {
        StoreInfo storeInfo;
        storeInfo.tokenId = IPCSkeleton::GetCallingTokenID();
        storeInfo.bundleName = param.bundleName_;
        storeInfo.storeName = RemoveSuffix(param.storeName_);
        auto [instanceId,  user]= GetInstIndexAndUser(storeInfo.tokenId, param.bundleName_);
        storeInfo.instanceId = instanceId;
        storeInfo.user = user;
        storeInfo.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
        executors_->Execute([storeInfo]() {
            auto event = std::make_unique<CloudEvent>(CloudEvent::GET_SCHEMA, std::move(storeInfo));
            EventCenter::GetInstance().PostEvent(move(event));
            return;
        });
    }
}

StoreMetaData RdbServiceImpl::GetStoreMetaData(const RdbSyncerParam &param)
{
    StoreMetaData metaData;
    metaData.uid = IPCSkeleton::GetCallingUid();
    metaData.tokenId = IPCSkeleton::GetCallingTokenID();
    auto [instanceId, user] = GetInstIndexAndUser(metaData.tokenId, param.bundleName_);
    metaData.instanceId = instanceId;
    metaData.bundleName = param.bundleName_;
    metaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData.storeId = RemoveSuffix(param.storeName_);
    metaData.user = std::to_string(user);
    metaData.storeType = param.type_;
    metaData.securityLevel = param.level_;
    metaData.area = param.area_;
    metaData.appId = CheckerManager::GetInstance().GetAppId(Converter::ConvertToStoreInfo(metaData));
    metaData.appType = "harmony";
    metaData.hapName = param.hapName_;
    metaData.customDir = param.customDir_;
    metaData.dataDir = DirectoryManager::GetInstance().GetStorePath(metaData) + "/" + param.storeName_;
    metaData.account = AccountDelegate::GetInstance()->GetCurrentAccountId();
    metaData.isEncrypt = param.isEncrypt_;
    metaData.isManualClean = !param.isAutoClean_;
    metaData.isSearchable = param.isSearchable_;
    metaData.haMode = param.haMode_;
    metaData.asyncDownloadAsset = param.asyncDownloadAsset_;
    return metaData;
}

int32_t RdbServiceImpl::SetSecretKey(const RdbSyncerParam &param, const StoreMetaData &meta)
{
    SecretKeyMetaData newSecretKey;
    newSecretKey.storeType = meta.storeType;
    newSecretKey.sKey = CryptoManager::GetInstance().Encrypt(param.password_);
    if (newSecretKey.sKey.empty()) {
        ZLOGE("encrypt work key error.");
        return RDB_ERROR;
    }
    auto time = system_clock::to_time_t(system_clock::now());
    newSecretKey.time = { reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time) };
    return MetaDataManager::GetInstance().SaveMeta(meta.GetSecretKey(), newSecretKey, true) ? RDB_OK : RDB_ERROR;
}

int32_t RdbServiceImpl::Upgrade(const RdbSyncerParam &param, const StoreMetaData &old)
{
    if (old.storeType == RDB_DEVICE_COLLABORATION && old.version < StoreMetaData::UUID_CHANGED_TAG) {
        auto store = GetStore(param);
        if (store == nullptr) {
            ZLOGE("store is null, bundleName:%{public}s storeName:%{public}s", param.bundleName_.c_str(),
                Anonymous::Change(param.storeName_).c_str());
            return RDB_ERROR;
        }
        return store->Clean({}, GeneralStore::CleanMode::NEARBY_DATA, "") == GeneralError::E_OK ? RDB_OK : RDB_ERROR;
    }
    return RDB_OK;
}

Details RdbServiceImpl::HandleGenDetails(const GenDetails &details)
{
    Details dbDetails;
    for (const auto& [id, detail] : details) {
        auto &dbDetail = dbDetails[id];
        dbDetail.progress = detail.progress;
        dbDetail.code = detail.code;
        for (auto &[name, table] : detail.details) {
            auto &dbTable = dbDetail.details[name];
            Constant::Copy(&dbTable, &table);
        }
    }
    return dbDetails;
}

bool RdbServiceImpl::GetDBPassword(const StoreMetaData &metaData, DistributedDB::CipherPassword &password)
{
    if (!metaData.isEncrypt) {
        return true;
    }

    std::string key = metaData.GetSecretKey();
    DistributedData::SecretKeyMetaData secretKeyMeta;
    MetaDataManager::GetInstance().LoadMeta(key, secretKeyMeta, true);
    std::vector<uint8_t> decryptKey;
    CryptoManager::GetInstance().Decrypt(secretKeyMeta.sKey, decryptKey);
    if (password.SetValue(decryptKey.data(), decryptKey.size()) != DistributedDB::CipherPassword::OK) {
        std::fill(decryptKey.begin(), decryptKey.end(), 0);
        ZLOGE("Set secret key value failed. len is (%{public}d)", int32_t(decryptKey.size()));
        return false;
    }
    std::fill(decryptKey.begin(), decryptKey.end(), 0);
    return true;
}

std::string RdbServiceImpl::RemoveSuffix(const std::string& name)
{
    std::string suffix(".db");
    auto pos = name.rfind(suffix);
    if (pos == std::string::npos || pos < name.length() - suffix.length()) {
        return name;
    }
    return std::string(name, 0, pos);
}

std::pair<int32_t, int32_t> RdbServiceImpl::GetInstIndexAndUser(uint32_t tokenId, const std::string &bundleName)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != TOKEN_HAP) {
        return { 0, 0 };
    }

    HapTokenInfo tokenInfo;
    tokenInfo.instIndex = -1;
    int errCode = AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (errCode != RET_SUCCESS) {
        ZLOGE("GetHapTokenInfo error:%{public}d, tokenId:0x%{public}x appId:%{public}s", errCode, tokenId,
            bundleName.c_str());
        return { -1, -1 };
    }
    return { tokenInfo.instIndex, tokenInfo.userID };
}

int32_t RdbServiceImpl::OnBind(const BindInfo &bindInfo)
{
    executors_ = bindInfo.executors;
    RdbHiViewAdapter::GetInstance().SetThreadPool(executors_);
    return 0;
}

StoreMetaData RdbServiceImpl::GetStoreMetaData(const Database &dataBase)
{
    StoreMetaData storeMetaData;
    storeMetaData.storeId = dataBase.name;
    storeMetaData.bundleName = dataBase.bundleName;
    storeMetaData.user = dataBase.user;
    storeMetaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    storeMetaData.tokenId = tokenId;
    auto [instanceId, user] = GetInstIndexAndUser(storeMetaData.tokenId, storeMetaData.bundleName);
    storeMetaData.instanceId = instanceId;
    MetaDataManager::GetInstance().LoadMeta(storeMetaData.GetKey(), storeMetaData, true);
    return storeMetaData;
}


std::shared_ptr<DistributedData::GeneralStore> RdbServiceImpl::GetStore(const StoreMetaData &storeMetaData)
{
    auto watchers = GetWatchers(storeMetaData.tokenId, storeMetaData.storeId);
    auto store = AutoCache::GetInstance().GetStore(storeMetaData, watchers);
    return store;
}

std::vector<std::string> RdbServiceImpl::GetReuseDevice(const std::vector<std::string> &devices)
{
    std::vector<std::string> onDevices;
    auto instance = AppDistributedKv::ProcessCommunicatorImpl::GetInstance();
    for (auto &device : devices) {
        if (instance->ReuseConnect({device}) == Status::SUCCESS) {
            onDevices.push_back(device);
        }
    }
    return onDevices;
}

int RdbServiceImpl::DoAutoSync(
    const std::vector<std::string> &devices, const Database &dataBase, std::vector<std::string> tables)
{
    StoreMetaData storeMetaData = GetStoreMetaData(dataBase);
    auto tokenId = storeMetaData.tokenId;
    auto store = GetStore(storeMetaData);
    if (store == nullptr) {
        ZLOGE("autosync store null, storeId:%{public}s", storeMetaData.GetStoreAlias().c_str());
        return RDB_ERROR;
    }
    SyncParam syncParam = {0, 0};
    auto pid = IPCSkeleton::GetCallingPid();
    DetailAsync async;
    if (executors_ == nullptr) {
        ZLOGE("autosync executors_ null, storeId:%{public}s", storeMetaData.GetStoreAlias().c_str());
        return RDB_ERROR;
    }
    for (auto &table : tables) {
        executors_->Execute([this, table, store, pid, syncParam, tokenId, async,
                                devices, storeMetaData]() {
            RdbQuery rdbQuery;
            rdbQuery.MakeQuery(table);
            std::vector<std::string> onDevices = GetReuseDevice(devices);
            if (onDevices.empty()) {
                ZLOGE("autosync ondevices null, storeId:%{public}s", storeMetaData.GetStoreAlias().c_str());
                return;
            }
            auto complete = [this, rdbQuery, store, pid, syncParam, tokenId, async, seq = 0](
                                const auto &results) mutable {
                auto ret = ProcessResult(results);
                store->Sync(ret.first, rdbQuery, async, syncParam);
            };
            if (IsNeedMetaSync(storeMetaData, onDevices)) {
                MetaDataManager::GetInstance().Sync(onDevices, complete);
                return;
            }
            (void)store->Sync(onDevices, rdbQuery, async, syncParam).first;
            return;
        });
    }
    return RDB_OK;
}

int RdbServiceImpl::DoOnlineSync(const std::vector<std::string> &devices, const Database &dataBase)
{
    std::vector<std::string> tableNames;
    for (auto &table : dataBase.tables) {
        if (!table.deviceSyncFields.empty()) {
            tableNames.push_back(table.name);
        }
    }
    return DoAutoSync(devices, dataBase, tableNames);
}

int32_t RdbServiceImpl::OnReady(const std::string &device)
{
    int index = ALLOW_ONLINE_AUTO_SYNC;
    Database dataBase;
    std::string prefix = dataBase.GetPrefix({});
    std::vector<Database> dataBases;
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, dataBases, true)) {
        return 0;
    }
    for (auto dataBase : dataBases) {
        if ((dataBase.autoSyncType == AutoSyncType::SYNC_ON_READY ||
                dataBase.autoSyncType == AutoSyncType::SYNC_ON_CHANGE_READY) &&
            index > 0) {
            std::vector<std::string> devices = {device};
            if (!DoOnlineSync(devices, dataBase)) {
                ZLOGE("store online sync fail, storeId:%{public}s", dataBase.name.c_str());
            }
            index--;
        }
    }
    return 0;
}

void RdbServiceImpl::SyncAgent::SetNotifier(sptr<RdbNotifierProxy> notifier)
{
    notifier_ = notifier;
    if (watcher_ != nullptr) {
        watcher_->SetNotifier(notifier);
    }
}

void RdbServiceImpl::SyncAgent::SetWatcher(std::shared_ptr<RdbWatcher> watcher)
{
    if (watcher_ != watcher) {
        watcher_ = watcher;
        if (watcher_ != nullptr) {
            watcher_->SetNotifier(notifier_);
        }
    }
}

RdbServiceImpl::SyncAgent::SyncAgent(const std::string &bundleName) : bundleName_(bundleName)
{
    notifier_ = nullptr;
    watcher_ = nullptr;
    count_ = 0;
    callBackStores_.clear();
}

int32_t RdbServiceImpl::RdbStatic::CloseStore(const std::string &bundleName, int32_t user, int32_t index,
    int32_t tokenId) const
{
    if (tokenId != RdbServiceImpl::RdbStatic::INVALID_TOKENID) {
        AutoCache::GetInstance().CloseStore(tokenId);
        return E_OK;
    }
    std::string prefix = StoreMetaData::GetPrefix(
        { DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid, std::to_string(user), "default", bundleName });
    std::vector<StoreMetaData> storeMetaData;
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, storeMetaData, true)) {
        ZLOGE("load meta failed! bundleName:%{public}s, user:%{public}d, index:%{public}d",
            bundleName.c_str(), user, index);
        return E_ERROR;
    }
    for (const auto &meta : storeMetaData) {
        if (meta.storeType < StoreMetaData::STORE_RELATIONAL_BEGIN ||
            meta.storeType > StoreMetaData::STORE_RELATIONAL_END) {
            continue;
        }
        if (meta.instanceId == index && !meta.appId.empty() && !meta.storeId.empty()) {
            AutoCache::GetInstance().CloseStore(meta.tokenId);
            break;
        }
    }
    return E_OK;
}

int32_t RdbServiceImpl::RdbStatic::OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index)
{
    std::string prefix = Database::GetPrefix({std::to_string(user), "default", bundleName});
    std::vector<Database> dataBase;
    if (MetaDataManager::GetInstance().LoadMeta(prefix, dataBase, true)) {
        for (const auto &dataBase : dataBase) {
            MetaDataManager::GetInstance().DelMeta(dataBase.GetKey(), true);
        }
    }
    return CloseStore(bundleName, user, index);
}

int32_t RdbServiceImpl::RdbStatic::OnAppUpdate(const std::string &bundleName, int32_t user, int32_t index)
{
    std::string prefix = Database::GetPrefix({std::to_string(user), "default", bundleName});
    std::vector<Database> dataBase;
    if (MetaDataManager::GetInstance().LoadMeta(prefix, dataBase, true)) {
        for (const auto &dataBase : dataBase) {
            MetaDataManager::GetInstance().DelMeta(dataBase.GetKey(), true);
        }
    }
    return CloseStore(bundleName, user, index);
}

int32_t RdbServiceImpl::RdbStatic::OnClearAppStorage(const std::string &bundleName, int32_t user, int32_t index,
    int32_t tokenId)
{
    return CloseStore(bundleName, user, index, tokenId);
}

void RdbServiceImpl::RegisterRdbServiceInfo()
{
    DumpManager::Config serviceInfoConfig;
    serviceInfoConfig.fullCmd = "--feature-info";
    serviceInfoConfig.abbrCmd = "-f";
    serviceInfoConfig.dumpName = "FEATURE_INFO";
    serviceInfoConfig.dumpCaption = { "| Display all the service statistics" };
    DumpManager::GetInstance().AddConfig("FEATURE_INFO", serviceInfoConfig);
}

void RdbServiceImpl::RegisterHandler()
{
    Handler handler =
        std::bind(&RdbServiceImpl::DumpRdbServiceInfo, this, std::placeholders::_1, std::placeholders::_2);
    DumpManager::GetInstance().AddHandler("FEATURE_INFO", uintptr_t(this), handler);
}
void RdbServiceImpl::DumpRdbServiceInfo(int fd, std::map<std::string, std::vector<std::string>> &params)
{
    (void)params;
    std::string info;
    dprintf(fd, "-------------------------------------RdbServiceInfo------------------------------\n%s\n",
        info.c_str());
}
int32_t RdbServiceImpl::OnInitialize()
{
    RegisterRdbServiceInfo();
    RegisterHandler();
    return RDB_OK;
}

RdbServiceImpl::~RdbServiceImpl()
{
    DumpManager::GetInstance().RemoveHandler("FEATURE_INFO", uintptr_t(this));
}

int RdbServiceImpl::DoDataChangeSync(const StoreInfo &storeInfo, const RdbChangedData &rdbChangedData)
{
    std::vector<std::string> tableNames;
    Database dataBase;
    dataBase.bundleName = storeInfo.bundleName;
    dataBase.name = storeInfo.storeName;
    dataBase.user = std::to_string(storeInfo.user);
    dataBase.deviceId = storeInfo.deviceId;
    for (const auto &[key, value] : rdbChangedData.tableData) {
        if (value.isP2pSyncDataChange) {
            tableNames.push_back(key);
        }
    }
    if (MetaDataManager::GetInstance().LoadMeta(dataBase.GetKey(), dataBase, true)) {
        std::vector<std::string> devices = DmAdapter::ToUUID(DmAdapter::GetInstance().GetRemoteDevices());
        if ((dataBase.autoSyncType == AutoSyncType::SYNC_ON_CHANGE ||
                dataBase.autoSyncType == AutoSyncType::SYNC_ON_CHANGE_READY) &&
            !devices.empty()) {
            return DoAutoSync(devices, dataBase, tableNames);
        }
    }
    return RDB_OK;
}

int32_t RdbServiceImpl::NotifyDataChange(
    const RdbSyncerParam &param, const RdbChangedData &rdbChangedData, const RdbNotifyConfig &rdbNotifyConfig)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. Permission error", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return RDB_ERROR;
    }
    StoreInfo storeInfo;
    storeInfo.tokenId = IPCSkeleton::GetCallingTokenID();
    storeInfo.bundleName = param.bundleName_;
    storeInfo.storeName = RemoveSuffix(param.storeName_);
    auto [instanceId, user] = GetInstIndexAndUser(storeInfo.tokenId, param.bundleName_);
    storeInfo.instanceId = instanceId;
    storeInfo.user = user;
    storeInfo.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    DataChangeEvent::EventInfo eventInfo;
    eventInfo.isFull = rdbNotifyConfig.isFull_;
    if (!DoDataChangeSync(storeInfo, rdbChangedData)) {
        ZLOGE("store datachange sync fail, storeId:%{public}s", storeInfo.storeName.c_str());
    }
    for (const auto &[key, value] : rdbChangedData.tableData) {
        if (value.isTrackedDataChange) {
            DataChangeEvent::TableChangeProperties tableChangeProperties = {value.isTrackedDataChange};
            eventInfo.tableProperties.insert_or_assign(key, std::move(tableChangeProperties));
        }
    }
    if (IsPostImmediately(IPCSkeleton::GetCallingPid(), rdbNotifyConfig, storeInfo, eventInfo, param.storeName_)) {
        auto evt = std::make_unique<DataChangeEvent>(std::move(storeInfo), std::move(eventInfo));
        EventCenter::GetInstance().PostEvent(std::move(evt));
    }
    return RDB_OK;
}

bool RdbServiceImpl::IsPostImmediately(const int32_t callingPid, const RdbNotifyConfig &rdbNotifyConfig,
    StoreInfo &storeInfo, DataChangeEvent::EventInfo &eventInfo, const std::string &storeName)
{
    bool postImmediately = false;
    heartbeatTaskIds_.Compute(callingPid, [this, &postImmediately, &rdbNotifyConfig, &storeInfo, &eventInfo,
        &storeName](const int32_t &key, std::map<std::string, ExecutorPool::TaskId> &tasks) {
        auto iter = tasks.find(storeName);
        ExecutorPool::TaskId taskId = ExecutorPool::INVALID_TASK_ID;
        if (iter != tasks.end()) {
            taskId = iter->second;
        }
        if (rdbNotifyConfig.delay_ == 0) {
            if (taskId != ExecutorPool::INVALID_TASK_ID && executors_ != nullptr) {
                executors_->Remove(taskId);
            }
            postImmediately = true;
            tasks.erase(storeName);
            return !tasks.empty();
        }

        if (executors_ != nullptr) {
            auto task = [storeInfoInner = storeInfo, eventInfoInner = eventInfo]() {
                auto evt = std::make_unique<DataChangeEvent>(std::move(storeInfoInner), std::move(eventInfoInner));
                EventCenter::GetInstance().PostEvent(std::move(evt));
            };
            if (taskId == ExecutorPool::INVALID_TASK_ID) {
                taskId = executors_->Schedule(std::chrono::milliseconds(rdbNotifyConfig.delay_), task);
            } else {
                taskId = executors_->Reset(taskId, std::chrono::milliseconds(rdbNotifyConfig.delay_));
            }
        }
        tasks.insert_or_assign(storeName, taskId);
        return true;
    });
    return postImmediately;
}

int32_t RdbServiceImpl::SetSearchable(const RdbSyncerParam& param, bool isSearchable)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. Permission error", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return RDB_ERROR;
    }
    SetSearchableEvent::EventInfo eventInfo;
    eventInfo.isSearchable = isSearchable;
    return PostSearchEvent(CloudEvent::SET_SEARCHABLE, param, eventInfo);
}

int32_t RdbServiceImpl::PostSearchEvent(int32_t evtId, const RdbSyncerParam& param,
    SetSearchableEvent::EventInfo &eventInfo)
{
    StoreInfo storeInfo;
    storeInfo.tokenId = IPCSkeleton::GetCallingTokenID();
    storeInfo.bundleName = param.bundleName_;
    storeInfo.storeName = RemoveSuffix(param.storeName_);
    auto [instanceId,  user]= GetInstIndexAndUser(storeInfo.tokenId, param.bundleName_);
    storeInfo.instanceId = instanceId;
    storeInfo.user = user;
    storeInfo.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;

    auto evt = std::make_unique<SetSearchableEvent>(std::move(storeInfo), std::move(eventInfo), evtId);
    EventCenter::GetInstance().PostEvent(std::move(evt));
    return RDB_OK;
}

int32_t RdbServiceImpl::Disable(const RdbSyncerParam &param)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto storeId = RemoveSuffix(param.storeName_);
    AutoCache::GetInstance().Disable(tokenId, storeId);
    return RDB_OK;
}

int32_t RdbServiceImpl::Enable(const RdbSyncerParam &param)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto storeId = RemoveSuffix(param.storeName_);
    AutoCache::GetInstance().Enable(tokenId, storeId);
    return RDB_OK;
}

int32_t RdbServiceImpl::GetPassword(const RdbSyncerParam &param, std::vector<std::vector<uint8_t>> &password)
{
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. Permission error", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return RDB_ERROR;
    }
    auto meta = GetStoreMetaData(param);
    password.resize(KEY_COUNT);
    SecretKeyMetaData secretKey;
    SecretKeyMetaData cloneSecretKey;
    bool keyMeta = MetaDataManager::GetInstance().LoadMeta(meta.GetSecretKey(), secretKey, true);
    bool cloneKeyMeta = MetaDataManager::GetInstance().LoadMeta(meta.GetCloneSecretKey(), cloneSecretKey, true);
    if (!keyMeta && !cloneKeyMeta) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. no meta", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return RDB_NO_META;
    }
    bool key = CryptoManager::GetInstance().Decrypt(secretKey.sKey, password.at(0));
    bool cloneKey = CryptoManager::GetInstance().Decrypt(cloneSecretKey.sKey, password.at(1));
    if (!key && !cloneKey) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. decrypt err", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        for (auto &item : password) {
            item.assign(item.size(), 0);
        }
        return RDB_ERROR;
    }
    return RDB_OK;
}

StoreInfo RdbServiceImpl::GetStoreInfo(const RdbSyncerParam &param)
{
    StoreInfo storeInfo;
    storeInfo.bundleName = param.bundleName_;
    storeInfo.tokenId = IPCSkeleton::GetCallingTokenID();
    storeInfo.user = AccountDelegate::GetInstance()->GetUserByToken(storeInfo.tokenId);
    storeInfo.storeName = RemoveSuffix(param.storeName_);
    return storeInfo;
}

std::pair<int32_t, uint32_t> RdbServiceImpl::LockCloudContainer(const RdbSyncerParam &param)
{
    std::pair<int32_t, uint32_t> result { RDB_ERROR, 0 };
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. Permission error", param.bundleName_.c_str(),
              Anonymous::Change(param.storeName_).c_str());
        return result;
    }
    ZLOGI("start to lock cloud db: bundleName:%{public}s, storeName:%{public}s", param.bundleName_.c_str(),
        Anonymous::Change(param.storeName_).c_str());

    auto storeInfo = GetStoreInfo(param);

    CloudLockEvent::Callback callback = [&result](int32_t status, uint32_t expiredTime) {
        result.first = status;
        result.second = expiredTime;
    };
    auto evt = std::make_unique<CloudLockEvent>(CloudEvent::LOCK_CLOUD_CONTAINER, std::move(storeInfo), callback);
    EventCenter::GetInstance().PostEvent(std::move(evt));
    return result;
}

int32_t RdbServiceImpl::UnlockCloudContainer(const RdbSyncerParam &param)
{
    int32_t result = RDB_ERROR;
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. Permission error", param.bundleName_.c_str(),
              Anonymous::Change(param.storeName_).c_str());
        return result;
    }
    ZLOGI("start to unlock cloud db: bundleName:%{public}s, storeName:%{public}s", param.bundleName_.c_str(),
        Anonymous::Change(param.storeName_).c_str());

    auto storeInfo = GetStoreInfo(param);

    CloudLockEvent::Callback callback = [&result](int32_t status, uint32_t expiredTime) {
        (void)expiredTime;
        result = status;
    };
    auto evt = std::make_unique<CloudLockEvent>(CloudEvent::UNLOCK_CLOUD_CONTAINER, std::move(storeInfo), callback);
    EventCenter::GetInstance().PostEvent(std::move(evt));
    return result;
}

int32_t RdbServiceImpl::GetDebugInfo(const RdbSyncerParam &param, std::map<std::string, RdbDebugInfo> &debugInfo)
{
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. Permission error", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return RDB_ERROR;
    }
    auto metaData = GetStoreMetaData(param);
    auto isCreated = MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData, true);
    if (!isCreated) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. no meta data", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return RDB_OK;
    }
    DistributedData::StoreDebugInfo debugMeta;
    isCreated = MetaDataManager::GetInstance().LoadMeta(metaData.GetDebugInfoKey(), debugMeta, true);
    if (!isCreated) {
        return RDB_OK;
    }

    for (auto &[name, fileDebug] : debugMeta.fileInfos) {
        RdbDebugInfo rdbInfo;
        rdbInfo.inode_ = fileDebug.inode;
        rdbInfo.size_ = fileDebug.size;
        rdbInfo.dev_ = fileDebug.dev;
        rdbInfo.mode_ = fileDebug.mode;
        rdbInfo.uid_ = fileDebug.uid;
        rdbInfo.gid_ = fileDebug.gid;
        debugInfo.insert(std::pair{ name, rdbInfo });
    }
    return RDB_OK;
}

int32_t RdbServiceImpl::SaveDebugInfo(const StoreMetaData &metaData, const RdbSyncerParam &param)
{
    if (param.infos_.empty()) {
        return RDB_OK;
    }
    DistributedData::StoreDebugInfo debugMeta;
    for (auto &[name, info] : param.infos_) {
        DistributedData::StoreDebugInfo::FileInfo fileInfo;
        fileInfo.inode = info.inode_;
        fileInfo.size = info.size_;
        fileInfo.dev = info.dev_;
        fileInfo.mode = info.mode_;
        fileInfo.uid = info.uid_;
        fileInfo.gid = info.gid_;
        debugMeta.fileInfos.insert(std::pair{name, fileInfo});
    }
    MetaDataManager::GetInstance().SaveMeta(metaData.GetDebugInfoKey(), debugMeta, true);
    return RDB_OK;
}

int32_t RdbServiceImpl::SavePromiseInfo(const StoreMetaData &metaData, const RdbSyncerParam &param)
{
    if (param.tokenIds_.empty() && param.uids_.empty()) {
        return RDB_OK;
    }
    StoreMetaDataLocal localMeta;
    localMeta.promiseInfo.tokenIds = param.tokenIds_;
    localMeta.promiseInfo.uids = param.uids_;
    localMeta.promiseInfo.permissionNames = param.permissionNames_;
    MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyLocal(), localMeta, true);
    return RDB_OK;
}

int32_t RdbServiceImpl::VerifyPromiseInfo(const RdbSyncerParam &param)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    auto meta = GetStoreMetaData(param);
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto uid = IPCSkeleton::GetCallingUid();
    meta.user = param.user_;
    StoreMetaDataLocal localMeta;
    auto isCreated = MetaDataManager::GetInstance().LoadMeta(meta.GetKeyLocal(), localMeta, true);
    if (!isCreated) {
        ZLOGE("Store not exist. bundleName:%{public}s, storeName:%{public}s", meta.bundleName.c_str(),
            meta.storeId.c_str());
        return RDB_ERROR;
    }
    ATokenTypeEnum type = AccessTokenKit::GetTokenType(tokenId);
    if (type == ATokenTypeEnum::TOKEN_NATIVE) {
        auto tokenIdRet =
            std::find(localMeta.promiseInfo.tokenIds.begin(), localMeta.promiseInfo.tokenIds.end(), tokenId);
        auto uidRet = std::find(localMeta.promiseInfo.uids.begin(), localMeta.promiseInfo.uids.end(), uid);
        bool isPromise = std::any_of(localMeta.promiseInfo.permissionNames.begin(),
            localMeta.promiseInfo.permissionNames.end(), [tokenId](const std::string &permissionName) {
                return PermitDelegate::VerifyPermission(permissionName, tokenId);
        });
        if (tokenIdRet == localMeta.promiseInfo.tokenIds.end() && uidRet == localMeta.promiseInfo.uids.end() &&
            !isPromise) {
            return RDB_ERROR;
        }
    }
    if (type == ATokenTypeEnum::TOKEN_HAP) {
        for (const auto& permissionName : localMeta.promiseInfo.permissionNames) {
            if (PermitDelegate::VerifyPermission(permissionName, tokenId)) {
                return RDB_OK;
            }
        }
        ZLOGE("Permission denied! tokenId:0x%{public}x", tokenId);
        return RDB_ERROR;
    }
    return RDB_OK;
}
} // namespace OHOS::DistributedRdb