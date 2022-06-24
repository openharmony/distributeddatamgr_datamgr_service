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

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "checker/checker_manager.h"
#include "communication_provider.h"
#include "crypto_manager.h"
#include "directory_manager.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "query_helper.h"
#include "utils/constant.h"
#include "utils/converter.h"
namespace OHOS::DistributedKv {
using namespace OHOS::DistributedData;
using namespace OHOS::AppDistributedKv;
using namespace OHOS::Security::AccessToken;
using system_clock = std::chrono::system_clock;
using Commu = AppDistributedKv::CommunicationProvider;
KVDBServiceImpl::KVDBServiceImpl()
{
}

KVDBServiceImpl::~KVDBServiceImpl()
{
}

Status KVDBServiceImpl::GetStoreIds(const AppId &appId, std::vector<StoreId> &storeIds)
{
    std::vector<StoreMetaData> metaData;
    auto user = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(IPCSkeleton::GetCallingPid());
    auto deviceId = Commu::GetInstance().GetLocalDevice().uuid;
    auto prefix = StoreMetaData::GetPrefix({ deviceId, user, "default", appId.appId });
    MetaDataManager::GetInstance().LoadMeta(prefix, metaData);
    for (auto &item : metaData) {
        storeIds.push_back({ item.storeId });
    }
    ZLOGD("appId:%{public}s, store size:%{public}zu", appId.appId.c_str(), storeIds.size());
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
            ZLOGW("agent already changed! old pid:%{public}d, new pid:%{public}d, appId:%{public}s",
                IPCSkeleton::GetCallingPid(), syncAgent.pid_, appId.appId.c_str());
            return true;
        }
        syncAgent.delayTimes_.erase(storeId);
        syncAgent.observers_.erase(storeId);
        syncAgent.conditions_.erase(storeId);
        return true;
    });
    storeCache_.CloseStore(tokenId, storeId);

    MetaDataManager::GetInstance().DelMeta(metaData.GetKey());
    auto key = SecretKeyMetaData::GetKey({ metaData.user, "default", metaData.bundleName, metaData.storeId });
    MetaDataManager::GetInstance().DelMeta(key, true);
    ZLOGD("appId:%{public}s, storeId:%{public}s", appId.appId.c_str(), storeId.storeId.c_str());
    return SUCCESS;
}

Status KVDBServiceImpl::Sync(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData);
    auto delay = GetSyncDelayTime(syncInfo.delay, storeId);
    return KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(metaData.tokenId), delay,
        std::bind(&KVDBServiceImpl::DoSync, this, metaData, syncInfo, std::placeholders::_1, ACTION_SYNC),
        std::bind(&KVDBServiceImpl::DoComplete, this, metaData, syncInfo, std::placeholders::_1));
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
    StrategyMeta strategyMeta = GetStrategyMeta(appId, storeId);
    if (strategyMeta.instanceId < 0) {
        return ILLEGAL_STATE;
    }
    MetaDataManager::GetInstance().LoadMeta(strategyMeta.GetKey(), strategyMeta);
    strategyMeta.capabilityRange.localLabel = local;
    strategyMeta.capabilityRange.remoteLabel = remote;
    MetaDataManager::GetInstance().SaveMeta(strategyMeta.GetKey(), strategyMeta);
    return SUCCESS;
}

Status KVDBServiceImpl::AddSubscribeInfo(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData);
    auto delay = GetSyncDelayTime(syncInfo.delay, storeId);
    return KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(metaData.tokenId), delay,
        std::bind(&KVDBServiceImpl::DoSync, this, metaData, syncInfo, std::placeholders::_1, ACTION_SUBSCRIBE),
        std::bind(&KVDBServiceImpl::DoComplete, this, metaData, syncInfo, std::placeholders::_1));
}

Status KVDBServiceImpl::RmvSubscribeInfo(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData);
    auto delay = GetSyncDelayTime(syncInfo.delay, storeId);
    return KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(metaData.tokenId), delay,
        std::bind(&KVDBServiceImpl::DoSync, this, metaData, syncInfo, std::placeholders::_1, ACTION_UNSUBSCRIBE),
        std::bind(&KVDBServiceImpl::DoComplete, this, metaData, syncInfo, std::placeholders::_1));
}

Status KVDBServiceImpl::Subscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
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
    return SUCCESS;
}

Status KVDBServiceImpl::Unsubscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    syncAgents_.ComputeIfPresent(tokenId, [&appId, &storeId, &observer](auto &key, SyncAgent &value) {
        if (value.pid_ != IPCSkeleton::GetCallingPid()) {
            ZLOGW("agent already changed! old pid:%{public}d, new pid:%{public}d, appId:%{public}s",
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

Status KVDBServiceImpl::BeforeCreate(const AppId &appId, const StoreId &storeId, const Options &options)
{
    ZLOGD("appId:%{public}s, storeId:%{public}s, nothing to do", appId.appId.c_str(), storeId.storeId.c_str());
    return SUCCESS;
}

Status KVDBServiceImpl::AfterCreate(const AppId &appId, const StoreId &storeId, const Options &options,
    const std::vector<uint8_t> &password)
{
    if (!appId.IsValid() || !storeId.IsValid() || !options.IsValidType()) {
        ZLOGE("failed, please check type:%{public}d, appId:%{public}s, storeId:%{public}s", options.kvStoreType,
            appId.appId.c_str(), storeId.storeId.c_str());
        return INVALID_ARGUMENT;
    }
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    AddOptions(options, metaData);
    StoreMetaData oldMeta;
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), oldMeta);
    bool isSaved = false;
    if (oldMeta != metaData) {
        // implement update
        ZLOGI("update meta data appId:%{public}s, storeId:%{public}s instanceId:%{public}d dir:%{public}s",
            appId.appId.c_str(), storeId.storeId.c_str(), metaData.instanceId, metaData.dataDir.c_str());
        isSaved = MetaDataManager::GetInstance().SaveMeta(metaData.GetKey(), metaData);
    }

    if (metaData.isEncrypt) {
        SecretKeyMetaData secretKey;
        secretKey.storeType = metaData.storeType;
        secretKey.sKey = CryptoManager::GetInstance().Encrypt(password);
        auto time = system_clock::to_time_t(system_clock::now());
        secretKey.time = { reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time) };
        auto storeKey = SecretKeyMetaData::GetKey({ metaData.user, "default", metaData.bundleName, metaData.storeId });
        MetaDataManager::GetInstance().SaveMeta(storeKey, secretKey, true);
    }
    ZLOGD("saved:%{public}d appId:%{public}s, storeId:%{public}s instanceId:%{public}d dir:%{public}s", isSaved,
        appId.appId.c_str(), storeId.storeId.c_str(), metaData.instanceId, metaData.dataDir.c_str());
    return SUCCESS;
}

Status KVDBServiceImpl::AppExit(pid_t uid, pid_t pid, uint32_t tokenId, const AppId &appId)
{
    ZLOGI("pid:%{public}d, uid:%{public}d, appId:%{public}s", pid, uid, appId.appId.c_str());
    syncAgents_.ComputeIfPresent(tokenId, [pid](auto &, auto &value) { return (value.pid_ != pid); });
    return SUCCESS;
}

Status KVDBServiceImpl::ResolveAutoLaunch(const std::string &identifier, DBLaunchParam &param)
{
    std::vector<StoreMetaData> metaData;
    auto prefix = StoreMetaData::GetPrefix({ Commu::GetInstance().GetLocalDevice().uuid });
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, metaData)) {
        ZLOGE("There is no store in user:%{public}s", param.userId.c_str());
        return STORE_NOT_FOUND;
    }

    for (const auto &storeMeta : metaData) {
        auto storeIdentifier = DBManager::GetKvStoreIdentifier("", storeMeta.appId, storeMeta.storeId, true);
        if (identifier != storeIdentifier) {
            continue;
        }

        std::shared_ptr<StoreCache::Observers> observers;
        syncAgents_.ComputeIfPresent(storeMeta.tokenId, [&storeMeta, &observers](auto, SyncAgent &agent) {
            auto it = agent.observers_.find(storeMeta.storeId);
            if (it != agent.observers_.end()) {
                observers = it->second;
            }
            return true;
        });

        if (observers == nullptr || observers->empty()) {
            continue;
        }

        DBStatus status;
        storeCache_.GetStore(storeMeta, observers, status);
    }
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

StoreMetaData KVDBServiceImpl::GetStoreMetaData(const AppId &appId, const StoreId &storeId)
{
    StoreMetaData metaData;
    metaData.uid = IPCSkeleton::GetCallingUid();
    metaData.tokenId = IPCSkeleton::GetCallingTokenID();
    metaData.instanceId = GetInstIndex(metaData.tokenId, appId);
    metaData.bundleName = appId.appId;
    metaData.deviceId = Commu::GetInstance().GetLocalDevice().uuid;
    metaData.storeId = storeId.storeId;
    metaData.user = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(metaData.uid);
    return metaData;
}

StrategyMeta KVDBServiceImpl::GetStrategyMeta(const AppId &appId, const StoreId &storeId)
{
    auto deviceId = Commu::GetInstance().GetLocalDevice().uuid;
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

Status KVDBServiceImpl::DoSync(const StoreMetaData &metaData, const SyncInfo &syncInfo, const SyncEnd &complete,
    int32_t type)
{
    ZLOGD("start.");
    std::vector<std::string> uuids;
    if (syncInfo.devices.empty()) {
        auto remotes = Commu::GetInstance().GetRemoteDevices();
        for (auto &remote : remotes) {
            uuids.push_back(std::move(remote.uuid));
        }
    }
    for (const auto &networkId : syncInfo.devices) {
        auto uuid = Commu::GetInstance().GetDeviceInfo(networkId).uuid;
        if (!uuid.empty()) {
            uuids.push_back(std::move(uuid));
        }
    }
    if (uuids.empty()) {
        ZLOGE("not found deviceIds.");
        return Status::ERROR;
    }
    DistributedDB::DBStatus status;
    auto store = storeCache_.GetStore(metaData, nullptr, status);
    if (store == nullptr) {
        ZLOGE("failed! status:%{public}d appId:%{public}s storeId:%{public}s dir:%{public}s", status,
            metaData.bundleName.c_str(), metaData.storeId.c_str(), metaData.dataDir.c_str());
        return ConvertDbStatus(status);
    }
    bool isSuccess = false;
    auto dbQuery = QueryHelper::StringToDbQuery(syncInfo.query, isSuccess);
    if (!isSuccess && !syncInfo.query.empty()) {
        ZLOGE("DBQuery:%{public}s failed.", syncInfo.query.c_str());
        return Status::INVALID_ARGUMENT;
    }

    auto mode = static_cast<DistributedDB::SyncMode>(syncInfo.mode);
    switch (type) {
        case ACTION_SYNC:
            status = store->Sync(uuids, mode, complete, dbQuery, false);
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

Status KVDBServiceImpl::DoComplete(const StoreMetaData &metaData, const SyncInfo &syncInfo, const DBResult &dbResult)
{
    if (syncInfo.seqId == std::numeric_limits<uint64_t>::max()) {
        return SUCCESS;
    }
    sptr<IKvStoreSyncCallback> callback;
    syncAgents_.ComputeIfPresent(metaData.tokenId, [&callback](auto &key, SyncAgent &agent) {
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
    callback->SyncCompleted(result, syncInfo.seqId);
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

Status KVDBServiceImpl::ConvertDbStatus(DBStatus status)
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

void KVDBServiceImpl::SyncAgent::ReInit(pid_t pid, const AppId &appId)
{
    ZLOGW("now pid:%{public}d, pid:%{public}d, appId:%{public}s, callback:%{public}d, observer:%{public}zu", pid, pid_,
        appId_.appId.c_str(), callback_ == nullptr, observers_.size());
    pid_ = pid;
    appId_ = appId;
    callback_ = nullptr;
    delayTimes_.clear();
    observers_.clear();
    conditions_.clear();
}
} // namespace OHOS::DistributedKv