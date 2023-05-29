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
#define LOG_TAG "KVDBServiceImpl"
#include "kvdb_service_impl.h"

#include <chrono>
#include <cinttypes>

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "backup_manager.h"
#include "checker/checker_manager.h"
#include "communication_provider.h"
#include "crypto_manager.h"
#include "device_manager_adapter.h"
#include "directory_manager.h"
#include "eventcenter/event_center.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "matrix_event.h"
#include "metadata/appid_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "permit_delegate.h"
#include "query_helper.h"
#include "upgrade.h"
#include "utils/anonymous.h"
#include "utils/constant.h"
#include "utils/converter.h"
namespace OHOS::DistributedKv {
using namespace OHOS::DistributedData;
using namespace OHOS::AppDistributedKv;
using namespace OHOS::Security::AccessToken;
using DMAdapter = DistributedData::DeviceManagerAdapter;
__attribute__((used)) KVDBServiceImpl::Factory KVDBServiceImpl::factory_;
KVDBServiceImpl::Factory::Factory()
{
    FeatureSystem::GetInstance().RegisterCreator("kv_store", []() { return std::make_shared<KVDBServiceImpl>(); });
}

KVDBServiceImpl::Factory::~Factory()
{
}

KVDBServiceImpl::KVDBServiceImpl()
{
    EventCenter::GetInstance().Subscribe(DeviceMatrix::MATRIX_META_FINISHED, [this](const Event &event) {
        auto &matrixEvent = static_cast<const MatrixEvent &>(event);
        auto deviceId = matrixEvent.GetDeviceId();
        RefCount refCount([deviceId] { DMAdapter::GetInstance().NotifyReadyEvent(deviceId); });
        std::vector<StoreMetaData> metaData;
        auto prefix = StoreMetaData::GetPrefix({ DMAdapter::GetInstance().GetLocalDevice().uuid });
        if (!MetaDataManager::GetInstance().LoadMeta(prefix, metaData)) {
            ZLOGE("load meta failed!");
            return;
        }
        auto mask = matrixEvent.GetMask();
        for (const auto &data : metaData) {
            StoreMetaDataLocal localMetaData;
            MetaDataManager::GetInstance().LoadMeta(data.GetKeyLocal(), localMetaData, true);
            if (!localMetaData.HasPolicy(PolicyType::IMMEDIATE_SYNC_ON_ONLINE)) {
                continue;
            }

            auto code = DeviceMatrix::GetInstance().GetCode(data);
            if ((mask & code) != code) {
                continue;
            }

            auto policy = localMetaData.GetPolicy(PolicyType::IMMEDIATE_SYNC_ON_ONLINE);
            SyncInfo syncInfo;
            syncInfo.mode = PUSH_PULL;
            syncInfo.delay = 0;
            syncInfo.devices = { deviceId };
            if (policy.IsValueEffect()) {
                syncInfo.delay = policy.valueUint;
            }
            ZLOGI("[online] appId:%{public}s, storeId:%{public}s", data.bundleName.c_str(),
                Anonymous::Change(data.storeId).c_str());
            auto delay = GetSyncDelayTime(syncInfo.delay, { data.storeId });
            KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(data.tokenId), delay,
                std::bind(&KVDBServiceImpl::DoSync, this, data, syncInfo, std::placeholders::_1, ACTION_SYNC),
                std::bind(&KVDBServiceImpl::DoComplete, this, data, syncInfo, refCount, std::placeholders::_1));
        }
    });
}

KVDBServiceImpl::~KVDBServiceImpl()
{
}

Status KVDBServiceImpl::GetStoreIds(const AppId &appId, std::vector<StoreId> &storeIds)
{
    std::vector<StoreMetaData> metaData;
    auto user = AccountDelegate::GetInstance()->GetUserByToken(IPCSkeleton::GetCallingTokenID());
    auto deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    auto prefix = StoreMetaData::GetPrefix({ deviceId, std::to_string(user), "default", appId.appId });
    auto instanceId = GetInstIndex(IPCSkeleton::GetCallingTokenID(), appId);
    MetaDataManager::GetInstance().LoadMeta(prefix, metaData);
    for (auto &item : metaData) {
        if (item.storeType > KvStoreType::MULTI_VERSION || item.instanceId != instanceId) {
            continue;
        }
        storeIds.push_back({ item.storeId });
    }
    ZLOGD("appId:%{public}s store size:%{public}zu", appId.appId.c_str(), storeIds.size());
    return SUCCESS;
}

Status KVDBServiceImpl::Delete(const AppId &appId, const StoreId &storeId)
{
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    if (metaData.instanceId < 0) {
        return ILLEGAL_STATE;
    }

    auto tokenId = IPCSkeleton::GetCallingTokenID();
    syncAgents_.ComputeIfPresent(tokenId, [&appId, &storeId](auto &key, SyncAgent &syncAgent) {
        if (syncAgent.pid_ != IPCSkeleton::GetCallingPid()) {
            ZLOGW("agent already changed! old pid:%{public}d new pid:%{public}d appId:%{public}s",
                IPCSkeleton::GetCallingPid(), syncAgent.pid_, appId.appId.c_str());
            return true;
        }
        syncAgent.delayTimes_.erase(storeId);
        syncAgent.observers_.erase(storeId);
        return true;
    });
    MetaDataManager::GetInstance().DelMeta(metaData.GetKey());
    MetaDataManager::GetInstance().DelMeta(metaData.GetSecretKey(), true);
    MetaDataManager::GetInstance().DelMeta(metaData.GetStrategyKey());
    MetaDataManager::GetInstance().DelMeta(metaData.GetKeyLocal(), true);
    PermitDelegate::GetInstance().DelCache(metaData.GetKey());
    storeCache_.CloseStore(tokenId, storeId);
    ZLOGD("appId:%{public}s storeId:%{public}s instanceId:%{public}d", appId.appId.c_str(), storeId.storeId.c_str(),
        metaData.instanceId);
    return SUCCESS;
}

Status KVDBServiceImpl::Sync(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData);
    auto delay = GetSyncDelayTime(syncInfo.delay, storeId);
    if (metaData.isAutoSync && syncInfo.seqId == std::numeric_limits<uint64_t>::max()) {
        DeviceMatrix::GetInstance().OnChanged(DeviceMatrix::GetInstance().GetCode(metaData));
        StoreMetaDataLocal localMeta;
        MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyLocal(), localMeta, true);
        if (!localMeta.HasPolicy(IMMEDIATE_SYNC_ON_CHANGE)) {
            ZLOGW("appId:%{public}s storeId:%{public}s no IMMEDIATE_SYNC_ON_CHANGE ", appId.appId.c_str(),
                Anonymous::Change(storeId.storeId).c_str());
            return Status::SUCCESS;
        }
    }
    return KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(metaData.tokenId), delay,
        std::bind(&KVDBServiceImpl::DoSync, this, metaData, syncInfo, std::placeholders::_1, ACTION_SYNC),
        std::bind(&KVDBServiceImpl::DoComplete, this, metaData, syncInfo, RefCount(), std::placeholders::_1));
}

Status KVDBServiceImpl::RegisterSyncCallback(const AppId &appId, sptr<IKvStoreSyncCallback> callback)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    syncAgents_.Compute(tokenId, [&appId, callback](const auto &, SyncAgent &value) {
        if (value.pid_ != IPCSkeleton::GetCallingPid()) {
            value.ReInit(IPCSkeleton::GetCallingPid(), appId);
        }
        value.callback_ = callback;
        return true;
    });
    return SUCCESS;
}

Status KVDBServiceImpl::UnregisterSyncCallback(const AppId &appId)
{
    syncAgents_.ComputeIfPresent(IPCSkeleton::GetCallingTokenID(), [&appId](const auto &key, SyncAgent &value) {
        if (value.pid_ != IPCSkeleton::GetCallingPid()) {
            ZLOGW("agent already changed! old pid:%{public}d, new pid:%{public}d, appId:%{public}s",
                IPCSkeleton::GetCallingPid(), value.pid_, appId.appId.c_str());
            return true;
        }
        value.callback_ = nullptr;
        return true;
    });
    return SUCCESS;
}

Status KVDBServiceImpl::SetSyncParam(const AppId &appId, const StoreId &storeId, const KvSyncParam &syncParam)
{
    if (syncParam.allowedDelayMs > 0 && syncParam.allowedDelayMs < KvStoreSyncManager::SYNC_MIN_DELAY_MS) {
        return Status::INVALID_ARGUMENT;
    }
    if (syncParam.allowedDelayMs > KvStoreSyncManager::SYNC_MAX_DELAY_MS) {
        return Status::INVALID_ARGUMENT;
    }
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    syncAgents_.Compute(tokenId, [&appId, &storeId, &syncParam](auto &key, SyncAgent &value) {
        if (value.pid_ != IPCSkeleton::GetCallingPid()) {
            value.ReInit(IPCSkeleton::GetCallingPid(), appId);
        }
        value.delayTimes_[storeId] = syncParam.allowedDelayMs;
        return true;
    });
    return SUCCESS;
}

Status KVDBServiceImpl::GetSyncParam(const AppId &appId, const StoreId &storeId, KvSyncParam &syncParam)
{
    syncParam.allowedDelayMs = 0;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    syncAgents_.ComputeIfPresent(tokenId, [&appId, &storeId, &syncParam](auto &key, SyncAgent &value) {
        if (value.pid_ != IPCSkeleton::GetCallingPid()) {
            ZLOGW("agent already changed! old pid:%{public}d, new pid:%{public}d, appId:%{public}s",
                IPCSkeleton::GetCallingPid(), value.pid_, appId.appId.c_str());
            return true;
        }

        auto it = value.delayTimes_.find(storeId);
        if (it != value.delayTimes_.end()) {
            syncParam.allowedDelayMs = it->second;
        }
        return true;
    });
    return SUCCESS;
}

Status KVDBServiceImpl::EnableCapability(const AppId &appId, const StoreId &storeId)
{
    StrategyMeta strategyMeta = GetStrategyMeta(appId, storeId);
    if (strategyMeta.instanceId < 0) {
        return ILLEGAL_STATE;
    }
    MetaDataManager::GetInstance().LoadMeta(strategyMeta.GetKey(), strategyMeta);
    strategyMeta.capabilityEnabled = true;
    MetaDataManager::GetInstance().SaveMeta(strategyMeta.GetKey(), strategyMeta);
    return SUCCESS;
}

Status KVDBServiceImpl::DisableCapability(const AppId &appId, const StoreId &storeId)
{
    StrategyMeta strategyMeta = GetStrategyMeta(appId, storeId);
    if (strategyMeta.instanceId < 0) {
        return ILLEGAL_STATE;
    }
    MetaDataManager::GetInstance().LoadMeta(strategyMeta.GetKey(), strategyMeta);
    strategyMeta.capabilityEnabled = false;
    MetaDataManager::GetInstance().SaveMeta(strategyMeta.GetKey(), strategyMeta);
    return SUCCESS;
}

Status KVDBServiceImpl::SetCapability(const AppId &appId, const StoreId &storeId,
    const std::vector<std::string> &local, const std::vector<std::string> &remote)
{
    StrategyMeta strategy = GetStrategyMeta(appId, storeId);
    if (strategy.instanceId < 0) {
        return ILLEGAL_STATE;
    }
    MetaDataManager::GetInstance().LoadMeta(strategy.GetKey(), strategy);
    strategy.capabilityRange.localLabel = local;
    strategy.capabilityRange.remoteLabel = remote;
    MetaDataManager::GetInstance().SaveMeta(strategy.GetKey(), strategy);
    return SUCCESS;
}

Status KVDBServiceImpl::AddSubscribeInfo(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData);
    auto delay = GetSyncDelayTime(syncInfo.delay, storeId);
    return KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(metaData.tokenId), delay,
        std::bind(&KVDBServiceImpl::DoSync, this, metaData, syncInfo, std::placeholders::_1, ACTION_SUBSCRIBE),
        std::bind(&KVDBServiceImpl::DoComplete, this, metaData, syncInfo, RefCount(), std::placeholders::_1));
}

Status KVDBServiceImpl::RmvSubscribeInfo(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData);
    auto delay = GetSyncDelayTime(syncInfo.delay, storeId);
    return KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(metaData.tokenId), delay,
        std::bind(&KVDBServiceImpl::DoSync, this, metaData, syncInfo, std::placeholders::_1, ACTION_UNSUBSCRIBE),
        std::bind(&KVDBServiceImpl::DoComplete, this, metaData, syncInfo, RefCount(), std::placeholders::_1));
}

Status KVDBServiceImpl::Subscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    ZLOGI("appId:%{public}s storeId:%{public}s tokenId:0x%{public}x", appId.appId.c_str(),
        Anonymous::Change(storeId.storeId).c_str(), tokenId);
    syncAgents_.Compute(tokenId, [&appId, &storeId, &observer](auto &key, SyncAgent &value) {
        if (value.pid_ != IPCSkeleton::GetCallingPid()) {
            value.ReInit(IPCSkeleton::GetCallingPid(), appId);
        }
        auto it = value.observers_.find(storeId);
        if (it == value.observers_.end()) {
            value.observers_[storeId] = std::make_shared<StoreCache::Observers>();
        }
        value.observers_[storeId]->insert(observer);
        return true;
    });
    auto observers = GetObservers(tokenId, storeId);
    storeCache_.SetObserver(tokenId, storeId, observers);
    return SUCCESS;
}

Status KVDBServiceImpl::Unsubscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    ZLOGI("appId:%{public}s storeId:%{public}s tokenId:0x%{public}x", appId.appId.c_str(),
        Anonymous::Change(storeId.storeId).c_str(), tokenId);
    syncAgents_.ComputeIfPresent(tokenId, [&appId, &storeId, &observer](auto &key, SyncAgent &value) {
        if (value.pid_ != IPCSkeleton::GetCallingPid()) {
            ZLOGW("agent already changed! old pid:%{public}d new pid:%{public}d appId:%{public}s",
                IPCSkeleton::GetCallingPid(), value.pid_, appId.appId.c_str());
            return true;
        }
        auto it = value.observers_.find(storeId);
        if (it != value.observers_.end()) {
            it->second->erase(observer);
        }
        return true;
    });
    return SUCCESS;
}

Status KVDBServiceImpl::GetBackupPassword(const AppId &appId, const StoreId &storeId, std::vector<uint8_t> &password)
{
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    return (BackupManager::GetInstance().GetPassWord(metaData, password)) ? SUCCESS : ERROR;
}

Status KVDBServiceImpl::BeforeCreate(const AppId &appId, const StoreId &storeId, const Options &options)
{
    ZLOGD("appId:%{public}s storeId:%{public}s to export data", appId.appId.c_str(), storeId.storeId.c_str());
    StoreMetaData meta = GetStoreMetaData(appId, storeId);
    AddOptions(options, meta);

    StoreMetaData old;
    auto isCreated = MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), old);
    if (!isCreated) {
        return SUCCESS;
    }
    if (old.storeType != meta.storeType || Constant::NotEqual(old.isEncrypt, meta.isEncrypt) ||
        old.area != meta.area || !options.persistent) {
        ZLOGE("meta appId:%{public}s storeId:%{public}s type:%{public}d->%{public}d encrypt:%{public}d->%{public}d "
              "area:%{public}d->%{public}d persistent:%{public}d",
            appId.appId.c_str(), Anonymous::Change(storeId.storeId).c_str(), old.storeType, meta.storeType,
            old.isEncrypt, meta.isEncrypt, old.area, meta.area, options.persistent);
        return Status::STORE_META_CHANGED;
    }

    auto dbStatus = DBStatus::OK;
    if (old != meta) {
        dbStatus = Upgrade::GetInstance().ExportStore(old, meta);
    }
    return dbStatus == DBStatus::OK ? SUCCESS : DB_ERROR;
}

Status KVDBServiceImpl::AfterCreate(const AppId &appId, const StoreId &storeId, const Options &options,
    const std::vector<uint8_t> &password)
{
    if (!appId.IsValid() || !storeId.IsValid() || !options.IsValidType()) {
        ZLOGE("failed please check type:%{public}d appId:%{public}s storeId:%{public}s", options.kvStoreType,
            appId.appId.c_str(), Anonymous::Change(storeId.storeId).c_str());
        return INVALID_ARGUMENT;
    }

    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    AddOptions(options, metaData);

    StoreMetaData oldMeta;
    auto isCreated = MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), oldMeta);
    Status status = SUCCESS;
    if (isCreated && oldMeta != metaData) {
        auto dbStatus = Upgrade::GetInstance().UpdateStore(oldMeta, metaData, password);
        ZLOGI("update status:%{public}d appId:%{public}s storeId:%{public}s inst:%{public}d "
              "type:%{public}d->%{public}d dir:%{public}s",
            dbStatus, appId.appId.c_str(), Anonymous::Change(storeId.storeId).c_str(), metaData.instanceId,
            oldMeta.storeType, metaData.storeType, metaData.dataDir.c_str());
        if (dbStatus != DBStatus::OK) {
            status = STORE_UPGRADE_FAILED;
        }
    }

    if (!isCreated || oldMeta != metaData) {
        MetaDataManager::GetInstance().SaveMeta(metaData.GetKey(), metaData);
    }
    AppIDMetaData appIdMeta;
    appIdMeta.bundleName = metaData.bundleName;
    appIdMeta.appId = metaData.appId;
    MetaDataManager::GetInstance().SaveMeta(appIdMeta.GetKey(), appIdMeta, true);
    SaveLocalMetaData(options, metaData);
    Upgrade::GetInstance().UpdatePassword(metaData, password);
    ZLOGI("appId:%{public}s storeId:%{public}s instanceId:%{public}d type:%{public}d dir:%{public}s",
        appId.appId.c_str(), Anonymous::Change(storeId.storeId).c_str(), metaData.instanceId, metaData.storeType,
        metaData.dataDir.c_str());
    return status;
}

int32_t KVDBServiceImpl::OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &appId)
{
    ZLOGI("pid:%{public}d uid:%{public}d appId:%{public}s", pid, uid, appId.c_str());
    std::vector<std::string> storeIds;
    syncAgents_.ComputeIfPresent(tokenId, [pid, &storeIds](auto &, SyncAgent &value) {
        if (value.pid_ != pid) {
            return true;
        }

        for (auto &[key, value] : value.observers_) {
            storeIds.push_back(key);
        }
        return false;
    });

    for (auto &storeId : storeIds) {
        storeCache_.CloseStore(tokenId, storeId);
    }
    return SUCCESS;
}

int32_t KVDBServiceImpl::ResolveAutoLaunch(const std::string &identifier, DBLaunchParam &param)
{
    ZLOGI("user:%{public}s appId:%{public}s storeId:%{public}s identifier:%{public}s", param.userId.c_str(),
        param.appId.c_str(), Anonymous::Change(param.storeId).c_str(), Anonymous::Change(identifier).c_str());
    std::vector<StoreMetaData> metaData;
    auto prefix = StoreMetaData::GetPrefix({ DMAdapter::GetInstance().GetLocalDevice().uuid, param.userId });
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, metaData)) {
        ZLOGE("no store in user:%{public}s", param.userId.c_str());
        return STORE_NOT_FOUND;
    }

    for (const auto &storeMeta : metaData) {
        auto identifierTag = DBManager::GetKvStoreIdentifier("", storeMeta.appId, storeMeta.storeId, true);
        if (identifier != identifierTag) {
            continue;
        }

        auto observers = GetObservers(storeMeta.tokenId, storeMeta.storeId);
        ZLOGD("user:%{public}s appId:%{public}s storeId:%{public}s observers:%{public}zu", storeMeta.user.c_str(),
            storeMeta.bundleName.c_str(), Anonymous::Change(storeMeta.storeId).c_str(),
            (observers) ? observers->size() : size_t(0));
        DBStatus status;
        storeCache_.GetStore(storeMeta, observers, status);
    }
    return SUCCESS;
}
int32_t KVDBServiceImpl::OnUserChange(uint32_t code, const std::string &user, const std::string &account)
{
    (void)code;
    (void)user;
    (void)account;
    std::vector<int32_t> users;
    AccountDelegate::GetInstance()->QueryUsers(users);
    std::set<int32_t> userIds(users.begin(), users.end());
    storeCache_.CloseExcept(userIds);
    return SUCCESS;
}

int32_t KVDBServiceImpl::OnReady(const std::string &device)
{
    return SUCCESS;
}

void KVDBServiceImpl::AddOptions(const Options &options, StoreMetaData &metaData)
{
    metaData.isAutoSync = options.autoSync;
    metaData.isBackup = options.backup;
    metaData.isEncrypt = options.encrypt;
    metaData.storeType = options.kvStoreType;
    metaData.securityLevel = options.securityLevel;
    metaData.area = options.area;
    metaData.appId = CheckerManager::GetInstance().GetAppId(Converter::ConvertToStoreInfo(metaData));
    metaData.appType = "harmony";
    metaData.hapName = options.hapName;
    metaData.dataDir = DirectoryManager::GetInstance().GetStorePath(metaData);
    metaData.schema = options.schema;
    metaData.account = AccountDelegate::GetInstance()->GetCurrentAccountId();
}

void KVDBServiceImpl::SaveLocalMetaData(const Options &options, const StoreMetaData &metaData)
{
    StoreMetaDataLocal localMetaData;
    localMetaData.isAutoSync = options.autoSync;
    localMetaData.isBackup = options.backup;
    localMetaData.isEncrypt = options.encrypt;
    localMetaData.dataDir = DirectoryManager::GetInstance().GetStorePath(metaData);
    localMetaData.schema = options.schema;
    for (auto &policy : options.policies) {
        OHOS::DistributedData::PolicyValue value;
        value.type = policy.type;
        value.index = policy.value.index();
        if (const uint32_t *pval = std::get_if<uint32_t>(&policy.value)) {
            value.valueUint = *pval;
        }
        localMetaData.policies.emplace_back(value);
    }
    MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyLocal(), localMetaData, true);
}

StoreMetaData KVDBServiceImpl::GetStoreMetaData(const AppId &appId, const StoreId &storeId)
{
    StoreMetaData metaData;
    metaData.uid = IPCSkeleton::GetCallingUid();
    metaData.tokenId = IPCSkeleton::GetCallingTokenID();
    metaData.instanceId = GetInstIndex(metaData.tokenId, appId);
    metaData.bundleName = appId.appId;
    metaData.deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    metaData.storeId = storeId.storeId;
    metaData.user = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(metaData.uid);
    return metaData;
}

StrategyMeta KVDBServiceImpl::GetStrategyMeta(const AppId &appId, const StoreId &storeId)
{
    auto deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    auto userId = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(IPCSkeleton::GetCallingUid());
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    StrategyMeta strategyMeta(deviceId, userId, appId.appId, storeId.storeId);
    strategyMeta.instanceId = GetInstIndex(tokenId, appId);
    return strategyMeta;
}

int32_t KVDBServiceImpl::GetInstIndex(uint32_t tokenId, const AppId &appId)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != TOKEN_HAP) {
        return 0;
    }

    HapTokenInfo tokenInfo;
    tokenInfo.instIndex = -1;
    int errCode = AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (errCode != RET_SUCCESS) {
        ZLOGE("GetHapTokenInfo error:%{public}d, tokenId:0x%{public}x appId:%{public}s", errCode, tokenId,
            appId.appId.c_str());
        return -1;
    }
    return tokenInfo.instIndex;
}

Status KVDBServiceImpl::DoSync(const StoreMetaData &meta, const SyncInfo &info, const SyncEnd &complete, int32_t type)
{
    ZLOGD("seqId:0x%{public}" PRIx64 " type:%{public}d remote:%{public}zu appId:%{public}s storeId:%{public}s",
        info.seqId, type, info.devices.size(), meta.bundleName.c_str(), Anonymous::Change(meta.storeId).c_str());
    auto uuids = ConvertDevices(info.devices);
    if (uuids.empty()) {
        ZLOGW("no device online seqId:0x%{public}" PRIx64 " remote:%{public}zu appId:%{public}s storeId:%{public}s",
            info.seqId, info.devices.size(), meta.bundleName.c_str(), Anonymous::Change(meta.storeId).c_str());
        return Status::ERROR;
    }

    DistributedDB::DBStatus status;
    auto observers = GetObservers(meta.tokenId, meta.storeId);
    auto store = storeCache_.GetStore(meta, observers, status);
    if (store == nullptr) {
        ZLOGE("failed! status:%{public}d appId:%{public}s storeId:%{public}s dir:%{public}s", status,
            meta.bundleName.c_str(), Anonymous::Change(meta.storeId).c_str(), meta.dataDir.c_str());
        return ConvertDbStatus(status);
    }
    bool isSuccess = false;
    auto dbQuery = QueryHelper::StringToDbQuery(info.query, isSuccess);
    if (!isSuccess && !info.query.empty()) {
        ZLOGE("failed DBQuery:%{public}s", Anonymous::Change(info.query).c_str());
        return Status::INVALID_ARGUMENT;
    }

    switch (type) {
        case ACTION_SYNC:
            status = store->Sync(uuids, ConvertDBMode(SyncMode(info.mode)), complete, dbQuery, false);
            break;
        case ACTION_SUBSCRIBE:
            status = store->SubscribeRemoteQuery(uuids, complete, dbQuery, false);
            break;
        case ACTION_UNSUBSCRIBE:
            status = store->UnSubscribeRemoteQuery(uuids, complete, dbQuery, false);
            break;
        default:
            status = DBStatus::INVALID_ARGS;
            break;
    }
    return ConvertDbStatus(status);
}

Status KVDBServiceImpl::DoComplete(const StoreMetaData &meta, const SyncInfo &info, RefCount refCount,
    const DBResult &dbResult)
{
    ZLOGD("seqId:0x%{public}" PRIx64 " tokenId:0x%{public}x remote:%{public}zu", info.seqId, meta.tokenId,
        dbResult.size());
    if (refCount) {
        DeviceMatrix::GetInstance().OnExchanged(info.devices[0], DeviceMatrix::GetInstance().GetCode(meta));
    }
    if (info.seqId == std::numeric_limits<uint64_t>::max()) {
        return SUCCESS;
    }
    sptr<IKvStoreSyncCallback> callback;
    syncAgents_.ComputeIfPresent(meta.tokenId, [&callback](auto &key, SyncAgent &agent) {
        callback = agent.callback_;
        return true;
    });
    if (callback == nullptr) {
        return SUCCESS;
    }

    std::map<std::string, Status> result;
    for (auto &[key, status] : dbResult) {
        result[key] = ConvertDbStatus(status);
    }
    callback->SyncCompleted(result, info.seqId);
    return SUCCESS;
}

uint32_t KVDBServiceImpl::GetSyncDelayTime(uint32_t delay, const StoreId &storeId)
{
    if (delay != 0) {
        return std::min(std::max(delay, KvStoreSyncManager::SYNC_MIN_DELAY_MS), KvStoreSyncManager::SYNC_MAX_DELAY_MS);
    }

    bool isBackground = Constant::IsBackground(IPCSkeleton::GetCallingPid());
    if (!isBackground) {
        return delay;
    }
    delay = KvStoreSyncManager::SYNC_DEFAULT_DELAY_MS;
    syncAgents_.ComputeIfPresent(IPCSkeleton::GetCallingTokenID(), [&delay, &storeId](auto &, SyncAgent &agent) {
        auto it = agent.delayTimes_.find(storeId);
        if (it != agent.delayTimes_.end() && it->second != 0) {
            delay = it->second;
        }
        return true;
    });
    return delay;
}

Status KVDBServiceImpl::ConvertDbStatus(DBStatus status) const
{
    switch (status) {
        case DBStatus::BUSY: // fallthrough
        case DBStatus::DB_ERROR:
            return Status::DB_ERROR;
        case DBStatus::OK:
            return Status::SUCCESS;
        case DBStatus::INVALID_ARGS:
            return Status::INVALID_ARGUMENT;
        case DBStatus::NOT_FOUND:
            return Status::KEY_NOT_FOUND;
        case DBStatus::INVALID_VALUE_FIELDS:
            return Status::INVALID_VALUE_FIELDS;
        case DBStatus::INVALID_FIELD_TYPE:
            return Status::INVALID_FIELD_TYPE;
        case DBStatus::CONSTRAIN_VIOLATION:
            return Status::CONSTRAIN_VIOLATION;
        case DBStatus::INVALID_FORMAT:
            return Status::INVALID_FORMAT;
        case DBStatus::INVALID_QUERY_FORMAT:
            return Status::INVALID_QUERY_FORMAT;
        case DBStatus::INVALID_QUERY_FIELD:
            return Status::INVALID_QUERY_FIELD;
        case DBStatus::NOT_SUPPORT:
            return Status::NOT_SUPPORT;
        case DBStatus::TIME_OUT:
            return Status::TIME_OUT;
        case DBStatus::OVER_MAX_LIMITS:
            return Status::OVER_MAX_SUBSCRIBE_LIMITS;
        case DBStatus::EKEYREVOKED_ERROR: // fallthrough
        case DBStatus::SECURITY_OPTION_CHECK_ERROR:
            return Status::SECURITY_LEVEL_ERROR;
        default:
            break;
    }
    return Status::ERROR;
}

KVDBServiceImpl::DBMode KVDBServiceImpl::ConvertDBMode(SyncMode syncMode) const
{
    DBMode dbMode;
    if (syncMode == SyncMode::PUSH) {
        dbMode = DBMode::SYNC_MODE_PUSH_ONLY;
    } else if (syncMode == SyncMode::PULL) {
        dbMode = DBMode::SYNC_MODE_PULL_ONLY;
    } else {
        dbMode = DBMode::SYNC_MODE_PUSH_PULL;
    }
    return dbMode;
}

std::vector<std::string> KVDBServiceImpl::ConvertDevices(const std::vector<std::string> &deviceIds) const
{
    if (deviceIds.empty()) {
        return DMAdapter::ToUUID(DMAdapter::GetInstance().GetRemoteDevices());
    }
    return DMAdapter::ToUUID(deviceIds);
}

std::shared_ptr<StoreCache::Observers> KVDBServiceImpl::GetObservers(uint32_t tokenId, const std::string &storeId)
{
    std::shared_ptr<StoreCache::Observers> observers;
    syncAgents_.ComputeIfPresent(tokenId, [&storeId, &observers](auto, SyncAgent &agent) {
        auto it = agent.observers_.find(storeId);
        if (it != agent.observers_.end()) {
            observers = it->second;
        }
        return true;
    });
    return observers;
}

void KVDBServiceImpl::SyncAgent::ReInit(pid_t pid, const AppId &appId)
{
    ZLOGW("pid:%{public}d->%{public}d appId:%{public}s callback:%{public}d observer:%{public}zu", pid, pid_,
        appId_.appId.c_str(), callback_ == nullptr, observers_.size());
    pid_ = pid;
    appId_ = appId;
    callback_ = nullptr;
    delayTimes_.clear();
    observers_.clear();
}
} // namespace OHOS::DistributedKv