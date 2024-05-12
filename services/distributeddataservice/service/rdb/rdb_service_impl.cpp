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
#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "checker/checker_manager.h"
#include "abs_rdb_predicates.h"
#include "cloud/change_event.h"
#include "cloud/cloud_share_event.h"
#include "cloud/make_query_event.h"
#include "commonevent/data_change_event.h"
#include "communicator/device_manager_adapter.h"
#include "crypto_manager.h"
#include "directory/directory_manager.h"
#include "dump/dump_manager.h"
#include "eventcenter/event_center.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "metadata/appid_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "rdb_watcher.h"
#include "rdb_notifier_proxy.h"
#include "rdb_query.h"
#include "store/general_store.h"
#include "tokenid_kit.h"
#include "types_export.h"
#include "utils/anonymous.h"
#include "utils/constant.h"
#include "utils/converter.h"
#include "cloud/schema_meta.h"
#include "rdb_general_store.h"
#include "rdb_result_set_impl.h"
using OHOS::DistributedKv::AccountDelegate;
using OHOS::DistributedData::CheckerManager;
using OHOS::DistributedData::MetaDataManager;
using OHOS::DistributedData::StoreMetaData;
using OHOS::DistributedData::Anonymous;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
using DistributedDB::RelationalStoreManager;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using DumpManager = OHOS::DistributedData::DumpManager;
using system_clock = std::chrono::system_clock;

constexpr uint32_t ITERATE_TIMES = 10000;
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
            entry.user, entry.appId, entry.storeId);
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
            GetPassword(entry, param.option.passwd);
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
    syncAgents_.EraseIf([pid](auto& key, SyncAgent& agent) {
        if (agent.pid_ != pid) {
            return false;
        }
        if (agent.watcher_ != nullptr) {
            agent.watcher_->SetNotifier(nullptr);
        }
        auto stores = AutoCache::GetInstance().GetStoresIfPresent(key);
        for (auto store : stores) {
            if (store != nullptr) {
                store->UnregisterDetailProgressObserver();
            }
        }
        return true;
    });
    AutoCache::GetInstance().Enable(tokenId);
    return E_OK;
}

bool RdbServiceImpl::CheckAccess(const std::string& bundleName, const std::string& storeName)
{
    CheckerManager::StoreInfo storeInfo;
    storeInfo.uid = IPCSkeleton::GetCallingUid();
    storeInfo.tokenId = IPCSkeleton::GetCallingTokenID();
    storeInfo.bundleName = bundleName;
    storeInfo.storeId = RemoveSuffix(storeName);
    auto [instanceId, user] = GetInstIndexAndUser(storeInfo.tokenId, storeInfo.bundleName);
    if (instanceId != 0) {
        return false;
    }
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
    syncAgents_.Compute(tokenId, [&param, notifierProxy, pid](auto, SyncAgent &agent) {
        if (pid != agent.pid_) {
            agent.ReInit(pid, param.bundleName_);
        }
        agent.SetNotifier(notifierProxy);
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

int32_t RdbServiceImpl::SetDistributedTables(const RdbSyncerParam &param, const std::vector<std::string> &tables,
    const std::vector<Reference> &references, int32_t type)
{
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. Permission error", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return RDB_ERROR;
    }
    auto meta = GetStoreMetaData(param);

    if (type == DistributedRdb::DistributedTableType::DISTRIBUTED_DEVICE) {
        StoreMetaData localMeta;
        bool isCreatedLocal = MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), localMeta, true);
        if (!isCreatedLocal) {
            ZLOGE("no meta. bundleName:%{public}s, storeName:%{public}s. GetStore failed", param.bundleName_.c_str(),
                Anonymous::Change(param.storeName_).c_str());
            return RDB_ERROR;
        }
        StoreMetaData syncMeta;
        bool isCreatedSync = MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), syncMeta);
        if (!isCreatedSync || localMeta != syncMeta) {
            ZLOGI("save sync meta. bundle:%{public}s store:%{public}s type:%{public}d->%{public}d "
                  "encrypt:%{public}d->%{public}d , area:%{public}d->%{public}d",
                meta.bundleName.c_str(), meta.GetStoreAlias().c_str(), syncMeta.storeType, meta.storeType,
                syncMeta.isEncrypt, meta.isEncrypt, syncMeta.area, meta.area);
            MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), localMeta);
        }
    }
    auto store = GetStore(param);
    if (store == nullptr) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. GetStore failed", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return RDB_ERROR;
    }
    std::vector<DistributedData::Reference> relationships;
    for (const auto &reference : references) {
        DistributedData::Reference relationship = { reference.sourceTable, reference.targetTable, reference.refFields };
        relationships.emplace_back(relationship);
    }
    return store->SetDistributedTables(tables, type, relationships);
}

void RdbServiceImpl::OnAsyncComplete(uint32_t tokenId, uint32_t seqNum, Details &&result)
{
    ZLOGI("tokenId=%{public}x seqnum=%{public}u", tokenId, seqNum);
    auto [success, agent] = syncAgents_.Find(tokenId);
    if (success && agent.notifier_ != nullptr) {
        agent.notifier_->OnComplete(seqNum, std::move(result));
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
    auto [success, agent] = syncAgents_.Find(tokenId);
    if (agent.watcher_ == nullptr) {
        return {};
    }
    return { agent.watcher_ };
}

RdbServiceImpl::DetailAsync RdbServiceImpl::GetCallbacks(uint32_t tokenId, const std::string& storeName)
{
    sptr<RdbNotifierProxy> notifier = nullptr;
    syncAgents_.ComputeIfPresent(tokenId, [&storeName, &notifier](auto, SyncAgent& syncAgent) {
        if (syncAgent.callBackStores_.count(storeName) != 0) {
            notifier = syncAgent.notifier_;
        }
        return true;
    });
    if (notifier == nullptr) {
        return nullptr;
    }
    return [notifier, storeName](const GenDetails& details) {
        if (notifier != nullptr) {
            notifier->OnComplete(storeName, HandleGenDetails(details));
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
    auto cursor = store->Query("", rdbQuery);
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
    if (!option.isAsync) {
        SyncParam syncParam = { option.mode, 1, option.isCompensation };
        Details details = {};
        auto status = store->Sync(
            devices, rdbQuery,
            [&details, &param](const GenDetails &result) mutable {
                ZLOGD("Sync complete, bundleName:%{public}s, storeName:%{public}s", param.bundleName_.c_str(),
                    Anonymous::Change(param.storeName_).c_str());
                details = HandleGenDetails(result);
            },
            syncParam);
        if (async != nullptr) {
            async(std::move(details));
        }
        return status;
    }
    ZLOGD("seqNum=%{public}u", option.seqNum);
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    SyncParam syncParam = { option.mode, 0, option.isCompensation };
    return store->Sync(
        devices, rdbQuery,
        [this, tokenId, seqNum = option.seqNum](const GenDetails &result) mutable {
            OnAsyncComplete(tokenId, seqNum, HandleGenDetails(result));
        },
        syncParam);
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
    GenAsync asyncCallback = [this, tokenId = storeInfo.tokenId, seqNum = option.seqNum](
                                 const GenDetails &result) mutable {
        OnAsyncComplete(tokenId, seqNum, HandleGenDetails(result));
    };
    GenAsync syncCallback = [async, &param](const GenDetails &details) {
        ZLOGD("Cloud Sync complete, bundleName:%{public}s, storeName:%{public}s", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        if (async != nullptr) {
            async(HandleGenDetails(details));
        }
    };
    auto mixMode = static_cast<int32_t>(GeneralStore::MixMode(option.mode,
        option.isAutoSync ? GeneralStore::AUTO_SYNC_MODE : GeneralStore::MANUAL_SYNC_MODE));
    SyncParam syncParam = { mixMode, (option.isAsync ? 0 : WAIT_TIME), option.isCompensation };
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
    syncAgents_.Compute(tokenId, [pid, &param, &isCreate](auto &key, SyncAgent &agent) {
        if (pid != agent.pid_) {
            agent.ReInit(pid, param.bundleName_);
        }
        if (agent.watcher_ == nullptr) {
            isCreate = true;
            agent.SetWatcher(std::make_shared<RdbWatcher>());
        }
        agent.count_++;
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
    syncAgents_.ComputeIfPresent(tokenId, [&destroyed](auto &key, SyncAgent &agent) {
        if (agent.count_ > 0) {
            agent.count_--;
        }
        if (agent.count_ == 0) {
            destroyed = true;
            agent.SetWatcher(nullptr);
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
    bool isCreate = false;
    syncAgents_.Compute(tokenId, [pid, &param, &storeName, &isCreate](auto, SyncAgent& agent) {
        if (pid != agent.pid_) {
            agent.ReInit(pid, param.bundleName_);
        }
        auto it = agent.callBackStores_.find(storeName);
        if (it == agent.callBackStores_.end()) {
            agent.callBackStores_.insert(std::make_pair(storeName, 0));
            isCreate = true;
        }
        agent.callBackStores_[storeName]++;
        return true;
    });
    if (isCreate) {
        auto stores = AutoCache::GetInstance().GetStoresIfPresent(tokenId, storeName);
        if (!stores.empty() && *stores.begin() != nullptr) {
            (*stores.begin())->RegisterDetailProgressObserver(GetCallbacks(tokenId, storeName));
        }
    }
    return RDB_OK;
}

int32_t RdbServiceImpl::UnregisterAutoSyncCallback(const RdbSyncerParam& param,
    std::shared_ptr<DetailProgressObserver> observer)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto storeName = RemoveSuffix(param.storeName_);
    bool destroyed = false;
    syncAgents_.ComputeIfPresent(tokenId, [&storeName, &destroyed](auto, SyncAgent& agent) {
        auto it = agent.callBackStores_.find(storeName);
        if (it == agent.callBackStores_.end()) {
            return true;
        }
        it->second--;
        if (it->second <= 0) {
            agent.callBackStores_.erase(it);
            destroyed = true;
        }
        return true;
    });
    if (destroyed) {
        auto stores = AutoCache::GetInstance().GetStoresIfPresent(tokenId, storeName);
        if (!stores.empty() && *stores.begin() != nullptr) {
            (*stores.begin())->UnregisterDetailProgressObserver();
        }
    }
    return RDB_OK;
}

int32_t RdbServiceImpl::Delete(const RdbSyncerParam &param)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    AutoCache::GetInstance().CloseStore(tokenId, RemoveSuffix(param.storeName_));
    RdbSyncerParam tmpParam = param;
    HapTokenInfo hapTokenInfo;
    AccessTokenKit::GetHapTokenInfo(tokenId, hapTokenInfo);
    tmpParam.bundleName_ = hapTokenInfo.bundleName;
    auto storeMeta = GetStoreMetaData(tmpParam);
    MetaDataManager::GetInstance().DelMeta(storeMeta.GetKey());
    MetaDataManager::GetInstance().DelMeta(storeMeta.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(storeMeta.GetSecretKey(), true);
    MetaDataManager::GetInstance().DelMeta(storeMeta.GetStrategyKey());
    MetaDataManager::GetInstance().DelMeta(storeMeta.GetKeyLocal(), true);
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
    auto meta = GetStoreMetaData(param);
    auto isCreated =
        MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true);
    if (!isCreated) {
        return RDB_OK;
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
}

int32_t RdbServiceImpl::AfterOpen(const RdbSyncerParam &param)
{
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
        ZLOGD("meta bundle:%{public}s store:%{public}s type:%{public}d->%{public}d encrypt:%{public}d->%{public}d "
              "area:%{public}d->%{public}d",
            meta.bundleName.c_str(), meta.GetStoreAlias().c_str(), old.storeType, meta.storeType,
            old.isEncrypt, meta.isEncrypt, old.area, meta.area);
        MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true);
    }
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

bool RdbServiceImpl::GetPassword(const StoreMetaData &metaData, DistributedDB::CipherPassword &password)
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
    return 0;
}

void RdbServiceImpl::SyncAgent::ReInit(pid_t pid, const std::string &bundleName)
{
    pid_ = pid;
    count_ = 0;
    callBackStores_.clear();
    bundleName_ = bundleName;
    notifier_ = nullptr;
    if (watcher_ != nullptr) {
        watcher_->SetNotifier(nullptr);
    }
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
    return CloseStore(bundleName, user, index);
}

int32_t RdbServiceImpl::RdbStatic::OnAppUpdate(const std::string &bundleName, int32_t user, int32_t index)
{
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

int32_t RdbServiceImpl::NotifyDataChange(const RdbSyncerParam &param, const RdbChangedData &rdbChangedData)
{
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("bundleName:%{public}s, storeName:%{public}s. Permission error", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return RDB_ERROR;
    }
    StoreInfo storeInfo;
    storeInfo.tokenId = IPCSkeleton::GetCallingTokenID();
    storeInfo.bundleName = param.bundleName_;
    storeInfo.storeName = RemoveSuffix(param.storeName_);
    auto [instanceId,  user]= GetInstIndexAndUser(storeInfo.tokenId, param.bundleName_);
    storeInfo.instanceId = instanceId;
    storeInfo.user = user;
    storeInfo.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;

    DataChangeEvent::EventInfo eventInfo;
    for (const auto& [key, value] : rdbChangedData.tableData) {
        DataChangeEvent::TableChangeProperties tableChangeProperties = {value.isTrackedDataChange};
        eventInfo.tableProperties.insert_or_assign(key, std::move(tableChangeProperties));
    }

    auto evt = std::make_unique<DataChangeEvent>(std::move(storeInfo), std::move(eventInfo));
    EventCenter::GetInstance().PostEvent(std::move(evt));
    return RDB_OK;
}

int32_t RdbServiceImpl::Disable(const RdbSyncerParam& param)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto storeId = RemoveSuffix(param.storeName_);
    AutoCache::GetInstance().Disable(tokenId, storeId);
    return E_OK;
}
int32_t RdbServiceImpl::Enable(const RdbSyncerParam& param)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto storeId = RemoveSuffix(param.storeName_);
    AutoCache::GetInstance().Enable(tokenId, storeId);
    return E_OK;
}
} // namespace OHOS::DistributedRdb
