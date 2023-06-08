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
#include "cloud/cloud_event.h"
#include "communicator/device_manager_adapter.h"
#include "crypto_manager.h"
#include "directory/directory_manager.h"
#include "eventcenter/event_center.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "metadata/appid_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "rdb_watcher.h"
#include "rdb_notifier_proxy.h"
#include "rdb_query.h"
#include "types_export.h"
#include "utils/anonymous.h"
#include "utils/constant.h"
#include "utils/converter.h"
#include "cloud/schema_meta.h"
#include "rdb_general_store.h"
using OHOS::DistributedKv::AccountDelegate;
using OHOS::DistributedData::CheckerManager;
using OHOS::DistributedData::MetaDataManager;
using OHOS::DistributedData::StoreMetaData;
using OHOS::DistributedData::Anonymous;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
using DistributedDB::RelationalStoreManager;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
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
    AutoCache::GetInstance().RegCreator(RDB_DEVICE_COLLABORATION, [](const StoreMetaData &metaData) -> GeneralStore* {
        return new (std::nothrow) RdbGeneralStore(metaData);
    });
}

RdbServiceImpl::Factory::~Factory()
{
}

RdbServiceImpl::RdbServiceImpl() : autoLaunchObserver_(this)
{
    ZLOGI("construct");
    DistributedDB::RelationalStoreManager::SetAutoLaunchRequestCallback(
        [this](const std::string& identifier, DistributedDB::AutoLaunchParam &param) {
            return ResolveAutoLaunch(identifier, param);
        });
    auto process = [this](const Event &event) {
        auto &evt = static_cast<const CloudEvent &>(event);
        auto storeInfo = evt.GetStoreInfo();
        StoreMetaData meta;
        meta.storeId = storeInfo.storeName;
        meta.bundleName = storeInfo.bundleName;
        meta.user = std::to_string(storeInfo.user);
        meta.instanceId = storeInfo.instanceId;
        meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
        if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta)) {
            ZLOGE("meta empty, bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
                Anonymous::Change(meta.storeId).c_str());
            return;
        }
        auto watchers = GetWatchers(meta.tokenId, meta.storeId);
        auto store = AutoCache::GetInstance().GetStore(meta, watchers);
        if (store == nullptr) {
            ZLOGE("store null, storeId:%{public}s", Anonymous::Change(meta.storeId).c_str());
            return;
        }
    };
    EventCenter::GetInstance().Subscribe(CloudEvent::CLOUD_SYNC, process);
}

int32_t RdbServiceImpl::ResolveAutoLaunch(const std::string &identifier, DistributedDB::AutoLaunchParam &param)
{
    std::string identifierHex = TransferStringToHex(identifier);
    ZLOGI("%{public}.6s", identifierHex.c_str());
    std::vector<StoreMetaData> entries;
    auto localId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (!MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix( { localId } ), entries)) {
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
        ZLOGI("%{public}s %{public}s %{public}s", entry.user.c_str(), entry.appId.c_str(), entry.storeId.c_str());
        if (aIdentifier != identifier) {
            continue;
        }
        ZLOGI("find identifier %{public}s", entry.storeId.c_str());
        param.userId = entry.user;
        param.appId = entry.appId;
        param.storeId = entry.storeId;
        param.path = entry.dataDir;
        param.option.storeObserver = &autoLaunchObserver_;
        param.option.isEncryptedDb = entry.isEncrypt;
        if (entry.isEncrypt) {
            param.option.iterateTimes = ITERATE_TIMES;
            param.option.cipher = DistributedDB::CipherType::AES_256_GCM;
            RdbSyncer::GetPassword(entry, param.option.passwd);
        }
        return true;
    }

    ZLOGE("not find identifier");
    return false;
}

int32_t RdbServiceImpl::OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &bundleName)
{
    OnClientDied(pid);
    return E_OK;
}

void RdbServiceImpl::OnClientDied(pid_t pid)
{
    ZLOGI("client dead pid=%{public}d", pid);
    syncers_.ComputeIfPresent(pid, [this](const auto& key, StoreSyncersType& syncers) {
        syncerNum_ -= static_cast<int32_t>(syncers.size());
        for (const auto& [name, syncer] : syncers) {
            executors_->Remove(syncer->GetTimerId());
        }
        return false;
    });
    identifiers_.EraseIf([pid](const auto &key, std::pair<pid_t, uint32_t> &value) {
        return value.first == pid;
    });
    syncAgents_.EraseIf([pid](auto &key, SyncAgent &agent) { return agent.pid_ == pid; });
}

bool RdbServiceImpl::CheckAccess(const std::string& bundleName, const std::string& storeName)
{
    CheckerManager::StoreInfo storeInfo;
    storeInfo.uid = IPCSkeleton::GetCallingUid();
    storeInfo.tokenId = IPCSkeleton::GetCallingTokenID();
    storeInfo.bundleName = bundleName;
    storeInfo.storeId = RdbSyncer::RemoveSuffix(storeName);
    auto [instanceId, user] = RdbSyncer::GetInstIndexAndUser(storeInfo.tokenId, storeInfo.bundleName);
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
        ZLOGE("permission error");
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

void RdbServiceImpl::OnDataChange(pid_t pid, uint32_t tokenId, const DistributedDB::StoreChangedData &data)
{
    DistributedDB::StoreProperty property;
    data.GetStoreProperty(property);
    ZLOGI("%{public}d %{public}s", pid, property.storeId.c_str());
    if (pid == 0) {
        auto identifier = RelationalStoreManager::GetRelationalStoreIdentifier(property.userId, property.appId,
                                                                               property.storeId);
        auto [success, info] = identifiers_.Find(TransferStringToHex(identifier));
        if (!success) {
            ZLOGI("client doesn't subscribe");
            return;
        }
        pid = info.first;
        tokenId = info.second;
        ZLOGI("fixed pid=%{public}d and tokenId=0x%{public}d", pid, tokenId);
    }
    auto [success, agent] = syncAgents_.Find(tokenId);
    if (success && agent.notifier_ != nullptr && pid == agent.pid_) {
        std::string device = data.GetDataChangeDevice();
        auto networkId = DmAdapter::GetInstance().ToNetworkID(device);
        Origin origin;
        origin.origin = Origin::ORIGIN_NEARBY;
        origin.store = property.storeId;
        origin.id.push_back(networkId);
        agent.notifier_->OnChange(origin, {}, {});
    }
}

void RdbServiceImpl::SyncerTimeout(std::shared_ptr<RdbSyncer> syncer)
{
    if (syncer == nullptr) {
        return;
    }
    auto storeId = syncer->GetStoreId();
    ZLOGI("%{public}s", Anonymous::Change(storeId).c_str());
    syncers_.ComputeIfPresent(syncer->GetPid(), [this, storeId](const auto& key, StoreSyncersType& syncers) {
        syncers.erase(storeId);
        syncerNum_--;
        return true;
    });
}

std::shared_ptr<RdbSyncer> RdbServiceImpl::GetRdbSyncer(const RdbSyncerParam &param)
{
    pid_t pid = IPCSkeleton::GetCallingPid();
    pid_t uid = IPCSkeleton::GetCallingUid();
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    std::shared_ptr<RdbSyncer> syncer;
    syncers_.Compute(pid, [this, &param, pid, uid, tokenId, &syncer] (const auto& key, StoreSyncersType& syncers) {
        auto storeId = RdbSyncer::RemoveSuffix(param.storeName_);
        auto it = syncers.find(storeId);
        if (it != syncers.end()) {
            syncer = it->second;
            if (!param.isEncrypt_ || param.password_.empty()) {
                executors_->Remove(syncer->GetTimerId(), true);
                auto timerId = executors_->Schedule(std::chrono::milliseconds(SYNCER_TIMEOUT), [this, syncer] {
                    SyncerTimeout(syncer);
                });
                syncer->SetTimerId(timerId);
                return true;
            }
            syncers.erase(storeId);
        }
        if (syncers.size() >= MAX_SYNCER_PER_PROCESS || syncerNum_ >= MAX_SYNCER_NUM) {
            ZLOGE("pid: %{public}d, syncers size: %{public}zu. syncerNum: %{public}d", pid, syncers.size(), syncerNum_);
            return !syncers.empty();
        }
        auto rdbObserver = new (std::nothrow) RdbStoreObserverImpl(this, pid, tokenId);
        if (rdbObserver == nullptr) {
            return !syncers.empty();
        }
        auto syncer_ = std::make_shared<RdbSyncer>(param, rdbObserver);
        StoreMetaData storeMetaData = GetStoreMetaData(param);
        MetaDataManager::GetInstance().LoadMeta(storeMetaData.GetKey(), storeMetaData);
        if (syncer_->Init(pid, uid, tokenId, storeMetaData) != RDB_OK) {
            return !syncers.empty();
        }
        syncers[storeId] = syncer_;
        syncer = syncer_;
        syncerNum_++;
        auto timerId = executors_->Schedule(std::chrono::milliseconds(SYNCER_TIMEOUT), [this, syncer] {
            SyncerTimeout(syncer);
        });
        syncer->SetTimerId(timerId);
        return !syncers.empty();
    });

    if (syncer != nullptr) {
        identifiers_.Insert(syncer->GetIdentifier(), { pid, tokenId });
    } else {
        ZLOGE("syncer is nullptr");
    }
    return syncer;
}

int32_t RdbServiceImpl::SetDistributedTables(const RdbSyncerParam &param, const std::vector<std::string> &tables,
    int32_t type)
{
    ZLOGI("enter");
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("permission error");
        return RDB_ERROR;
    }
    auto syncer = GetRdbSyncer(param);
    if (syncer == nullptr) {
        return RDB_ERROR;
    }
    return syncer->SetDistributedTables(tables, type);
}

std::pair<int32_t, Details> RdbServiceImpl::DoSync(const RdbSyncerParam &param, const Option &option,
    const PredicatesMemo &pred)
{
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("permission error");
        return {RDB_ERROR, {}};
    }
    auto syncer = GetRdbSyncer(param);
    if (syncer == nullptr) {
        return {RDB_ERROR, {}};
    }
    Details details = {};
    auto status = syncer->DoSync(option, pred, [&details, &param](auto &&result) mutable {
        ZLOGD("Sync complete, bundleName:%{public}s, storeName:%{public}s", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        details = std::move(result);
    });
    return { status, std::move(details) };
}

void RdbServiceImpl::OnAsyncComplete(uint32_t tokenId, uint32_t seqNum, Details &&result)
{
    ZLOGI("pid=%{public}x seqnum=%{public}u", tokenId, seqNum);
    auto [success, agent] = syncAgents_.Find(tokenId);
    if (success && agent.notifier_ != nullptr) {
        agent.notifier_->OnComplete(seqNum, std::move(result));
    }
}

int32_t RdbServiceImpl::DoAsync(const RdbSyncerParam &param, const Option &option, const PredicatesMemo &pred)
{
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("permission error");
        return RDB_ERROR;
    }
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    ZLOGI("seq num=%{public}u", option.seqNum);
    auto syncer = GetRdbSyncer(param);
    if (syncer == nullptr) {
        return RDB_ERROR;
    }
    return syncer->DoSync(option, pred, [this, tokenId, seqNum = option.seqNum](Details &&result) {
        OnAsyncComplete(tokenId, seqNum, std::move(result));
    });
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

std::string RdbServiceImpl::GenIdentifier(const RdbSyncerParam &param)
{
    pid_t uid = IPCSkeleton::GetCallingUid();
    uint32_t token = IPCSkeleton::GetCallingTokenID();
    auto storeId = RdbSyncer::RemoveSuffix(param.storeName_);
    CheckerManager::StoreInfo storeInfo { uid, token, param.bundleName_, storeId };
    auto userId = AccountDelegate::GetInstance()->GetUserByToken(token);
    std::string appId = CheckerManager::GetInstance().GetAppId(storeInfo);
    std::string identifier = RelationalStoreManager::GetRelationalStoreIdentifier(
        std::to_string(userId), appId, storeId);
    return TransferStringToHex(identifier);
}

AutoCache::Watchers RdbServiceImpl::GetWatchers(uint32_t tokenId, const std::string &storeName)
{
    auto [success, agent] = syncAgents_.Find(tokenId);
    if (agent.watcher_ == nullptr) {
        return {};
    }
    return { agent.watcher_ };
}

void RdbServiceImpl::SyncAgent::ReInit(pid_t pid, const std::string &bundleName)
{
    pid_ = pid;
    count_ = 0;
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
        watcher_->SetNotifier(notifier_);
    }
}

int32_t RdbServiceImpl::RemoteQuery(const RdbSyncerParam& param, const std::string& device, const std::string& sql,
                                    const std::vector<std::string>& selectionArgs, sptr<IRemoteObject>& resultSet)
{
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("permission error");
        return RDB_ERROR;
    }
    auto syncer = GetRdbSyncer(param);
    if (syncer == nullptr) {
        ZLOGE("syncer is null");
        return RDB_ERROR;
    }
    return syncer->RemoteQuery(device, sql, selectionArgs, resultSet);
}

int32_t RdbServiceImpl::Sync(const RdbSyncerParam &param, const Option &option, const PredicatesMemo &predicates,
                             const AsyncDetail &async)
{
    if (!option.isAsync) {
        auto [status, details] = DoSync(param, option, predicates);
        if (async != nullptr) {
            async(std::move(details));
        }
        return status;
    }
    return DoAsync(param, option, predicates);
}

int32_t RdbServiceImpl::Subscribe(const RdbSyncerParam &param, const SubscribeOption &option,
    RdbStoreObserver *observer)
{
    pid_t pid = IPCSkeleton::GetCallingPid();
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    switch (option.mode) {
        case SubscribeMode::REMOTE: {
            auto identifier = GenIdentifier(param);
            identifiers_.Insert(identifier, std::pair { pid, tokenId });
            ZLOGI("%{public}s %{public}.6s %{public}d", Anonymous::Change(param.storeName_).c_str(),
                identifier.c_str(), pid);
            break;
        }
        case SubscribeMode::CLOUD: // fallthrough
        case SubscribeMode::CLOUD_DETAIL: {
            syncAgents_.Compute(tokenId, [pid, &param](auto &key, SyncAgent &agent) {
                if (pid != agent.pid_) {
                    agent.ReInit(pid, param.bundleName_);
                }
                if (agent.watcher_ == nullptr) {
                    agent.SetWatcher(std::make_shared<RdbWatcher>());
                }
                agent.count_++;
                return true;
            });
            break;
        }
        default:
            return RDB_ERROR;
    }
    return RDB_OK;
}

int32_t RdbServiceImpl::UnSubscribe(const RdbSyncerParam &param, const SubscribeOption &option,
    RdbStoreObserver *observer)
{
    switch (option.mode) {
        case SubscribeMode::REMOTE: {
            auto identifier = GenIdentifier(param);
            ZLOGI("%{public}s %{public}.6s", param.storeName_.c_str(), identifier.c_str());
            identifiers_.Erase(identifier);
            break;
        }
        case SubscribeMode::CLOUD: // fallthrough
        case SubscribeMode::CLOUD_DETAIL: {
            syncAgents_.ComputeIfPresent(IPCSkeleton::GetCallingTokenID(), [](auto &key, SyncAgent &agent) {
                if (agent.count_ > 0) {
                    agent.count_--;
                }
                if (agent.count_ == 0) {
                    agent.SetWatcher(nullptr);
                }
                return true;
            });
            break;
        }
        default:
            ZLOGI("mode:%{public}d", option.mode);
            return RDB_ERROR;
    }
    return RDB_OK;
}

int32_t RdbServiceImpl::OnInitialize()
{
    return RDB_OK;
}

int32_t RdbServiceImpl::GetSchema(const RdbSyncerParam &param)
{
    if (!CheckAccess(param.bundleName_, param.storeName_)) {
        ZLOGE("permission error");
        return RDB_ERROR;
    }
    StoreMetaData storeMeta;
    if (CreateMetaData(param, storeMeta) != RDB_OK) {
        return RDB_ERROR;
    }

    if (executors_ != nullptr) {
        CloudEvent::StoreInfo storeInfo;
        storeInfo.tokenId = IPCSkeleton::GetCallingTokenID();
        storeInfo.bundleName = param.bundleName_;
        storeInfo.storeName = RdbSyncer::RemoveSuffix(param.storeName_);
        auto [instanceId,  user]= RdbSyncer::GetInstIndexAndUser(storeInfo.tokenId, param.bundleName_);
        storeInfo.instanceId = instanceId;
        storeInfo.user = user;
        executors_->Execute([storeInfo]() {
            auto event = std::make_unique<CloudEvent>(CloudEvent::GET_SCHEMA, std::move(storeInfo));
            EventCenter::GetInstance().PostEvent(move(event));
            return;
        });
    }
    return RDB_OK;
}

StoreMetaData RdbServiceImpl::GetStoreMetaData(const RdbSyncerParam &param)
{
    StoreMetaData metaData;
    metaData.uid = IPCSkeleton::GetCallingUid();
    metaData.tokenId = IPCSkeleton::GetCallingTokenID();
    auto [instanceId, user] = RdbSyncer::GetInstIndexAndUser(metaData.tokenId, param.bundleName_);
    metaData.instanceId = instanceId;
    metaData.bundleName = param.bundleName_;
    metaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData.storeId = RdbSyncer::RemoveSuffix(param.storeName_);
    metaData.user = std::to_string(user);
    metaData.storeType = param.type_;
    metaData.securityLevel = param.level_;
    metaData.area = param.area_;
    metaData.appId = CheckerManager::GetInstance().GetAppId(Converter::ConvertToStoreInfo(metaData));
    metaData.appType = "harmony";
    metaData.hapName = param.hapName_;
    metaData.dataDir = DirectoryManager::GetInstance().GetStorePath(metaData) + "/" + param.storeName_;
    metaData.account = AccountDelegate::GetInstance()->GetCurrentAccountId();
    metaData.isEncrypt = param.isEncrypt_;
    return metaData;
}

int32_t RdbServiceImpl::CreateMetaData(const RdbSyncerParam &param, StoreMetaData &old)
{
    auto meta = GetStoreMetaData(param);
    bool isCreated = MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), old);
    if (isCreated && (old.storeType != meta.storeType || Constant::NotEqual(old.isEncrypt, meta.isEncrypt) ||
                         old.area != meta.area)) {
        ZLOGE("meta bundle:%{public}s store:%{public}s type:%{public}d->%{public}d encrypt:%{public}d->%{public}d "
              "area:%{public}d->%{public}d",
            meta.bundleName.c_str(), Anonymous::Change(meta.storeId).c_str(), old.storeType, meta.storeType,
            old.isEncrypt, meta.isEncrypt, old.area, meta.area);
        return RDB_ERROR;
    }
    if (!isCreated || meta != old) {
        Upgrade(param, old);
        ZLOGD("meta bundle:%{public}s store:%{public}s type:%{public}d->%{public}d encrypt:%{public}d->%{public}d "
              "area:%{public}d->%{public}d",
            meta.bundleName.c_str(), Anonymous::Change(meta.storeId).c_str(), old.storeType, meta.storeType,
            old.isEncrypt, meta.isEncrypt, old.area, meta.area);
        MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta);
    }
    AppIDMetaData appIdMeta;
    appIdMeta.bundleName = meta.bundleName;
    appIdMeta.appId = meta.appId;
    if (!MetaDataManager::GetInstance().SaveMeta(appIdMeta.GetKey(), appIdMeta, true)) {
        ZLOGE("meta bundle:%{public}s store:%{public}s type:%{public}d->%{public}d encrypt:%{public}d->%{public}d "
              "area:%{public}d->%{public}d",
            meta.bundleName.c_str(), Anonymous::Change(meta.storeId).c_str(), old.storeType, meta.storeType,
            old.isEncrypt, meta.isEncrypt, old.area, meta.area);
        return RDB_ERROR;
    }
    if (!param.isEncrypt_ || param.password_.empty()) {
        return RDB_OK;
    }
    return SetSecretKey(param, meta);
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
        auto syncer = GetRdbSyncer(param);
        if (syncer == nullptr) {
            ZLOGE("syncer is null, bundleName:%{public}s storeName:%{public}s", param.bundleName_.c_str(),
                param.storeName_.c_str());
            return RDB_ERROR;
        }
        return syncer->RemoveDeviceData();
    }
    return RDB_OK;
}

int32_t RdbServiceImpl::OnBind(const BindInfo &bindInfo)
{
    executors_ = bindInfo.executors;
    return 0;
}
} // namespace OHOS::DistributedRdb
