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

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "checker/checker_manager.h"
#include "communication_provider.h"
#include "directory_manager.h"
#include "ipc_skeleton.h"
#include "kvstore_sync_manager.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "utils/converter.h"
namespace OHOS::DistributedKv {
using namespace OHOS::DistributedData;
using namespace OHOS::AppDistributedKv;
using namespace OHOS::Security::AccessToken;
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
    auto deviceId = CommunicationProvider::GetInstance().GetLocalDevice().uuid;
    auto prefix = StoreMetaData::GetPrefix({ deviceId, user, "default", appId.appId });
    MetaDataManager::GetInstance().LoadMeta(prefix, metaData);
    for (auto &item : metaData) {
        storeIds.push_back({ item.storeId });
    }
    return SUCCESS;
}

Status KVDBServiceImpl::Delete(const AppId &appId, const StoreId &storeId)
{
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    if (metaData.instanceId < 0) {
        return ILLEGAL_STATE;
    }

    MetaDataManager::GetInstance().DelMeta(metaData.GetKey());
    auto key = SecretKeyMetaData::GetKey({ metaData.user, "default", metaData.bundleName, metaData.storeId });
    MetaDataManager::GetInstance().DelMeta(key, true);
    return SUCCESS;
}

Status KVDBServiceImpl::Sync(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    return NOT_SUPPORT;
}

Status KVDBServiceImpl::RegisterSyncCallback(const AppId &appId, sptr<IKvStoreSyncCallback> callback)
{
    syncAgents_.InsertOrAssign(IPCSkeleton::GetCallingTokenID(), std::pair{ IPCSkeleton::GetCallingPid(), callback });
    return SUCCESS;
}

Status KVDBServiceImpl::UnregisterSyncCallback(const AppId &appId)
{
    syncAgents_.ComputeIfPresent(
        IPCSkeleton::GetCallingTokenID(), [](const auto &key, std::pair<pid_t, sptr<IKvStoreSyncCallback>> &value) {
            return !(value.first == IPCSkeleton::GetCallingPid());
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
    delayTimes_.Compute(
        IPCSkeleton::GetCallingTokenID(), [&storeId, &syncParam](auto &key, std::map<std::string, uint32_t> &values) {
            values[storeId] = syncParam.allowedDelayMs;
            return !values.empty();
        });
    return SUCCESS;
}

Status KVDBServiceImpl::GetSyncParam(const AppId &appId, const StoreId &storeId, KvSyncParam &syncParam)
{
    delayTimes_.ComputeIfPresent(IPCSkeleton::GetCallingTokenID(), [&storeId, &syncParam](auto &key, std::map<std::string, uint32_t> &values) {
        auto it = values.find(storeId);
        if (it != values.end()) {
            syncParam.allowedDelayMs = it->second;
        }
        return !values.empty();
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

Status KVDBServiceImpl::AddSubscribeInfo(
    const AppId &appId, const StoreId &storeId, const std::vector<std::string> &devices, const std::string &query)
{
    return NOT_SUPPORT;
}

Status KVDBServiceImpl::RmvSubscribeInfo(
    const AppId &appId, const StoreId &storeId, const std::vector<std::string> &devices, const std::string &query)
{
    return NOT_SUPPORT;
}

Status KVDBServiceImpl::Subscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer)
{
    return SUCCESS;
}

Status KVDBServiceImpl::Unsubscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer)
{
    return SUCCESS;
}

Status KVDBServiceImpl::BeforeCreate(const AppId &appId, const StoreId &storeId, const Options &options)
{
    return SUCCESS;
}

Status KVDBServiceImpl::AfterCreate(
    const AppId &appId, const StoreId &storeId, const Options &options, const std::vector<uint8_t> &password)
{
    if (!appId.IsValid() || !storeId.IsValid() || !options.IsValidType()) {
        return INVALID_ARGUMENT;
    }
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    StoreMetaData oldMeta;
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), oldMeta);
    if (oldMeta == metaData) {
        return SUCCESS;
    }
    AddOptions(options, metaData);

    return NOT_SUPPORT;
}

Status KVDBServiceImpl::AppExit(pid_t uid, pid_t pid, uint32_t tokenId, const AppId &appId)
{
    dataObservers_.ComputeIfPresent(tokenId, [pid](const auto &, auto &value) { return !(value.first == pid); });
    syncAgents_.ComputeIfPresent(tokenId, [pid](const auto &, auto &value) { return !(value.first == pid); });
    delayTimes_.Erase(tokenId);
    return SUCCESS;
}

void KVDBServiceImpl::AddOptions(const Options &options, StoreMetaData &metaData)
{
    metaData.securityLevel = options.securityLevel;
    metaData.storeType = options.kvStoreType;
    metaData.isBackup = options.backup;
    metaData.isEncrypt = options.encrypt;
    metaData.isAutoSync = options.autoSync;
    metaData.appId = CheckerManager::GetInstance().GetAppId(Converter::ConvertToStoreInfo(metaData));
    metaData.user = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(metaData.uid);
    metaData.account = AccountDelegate::GetInstance()->GetCurrentAccountId();
    metaData.dataDir = DirectoryManager::GetInstance().GetStorePath(metaData);
}

StoreMetaData KVDBServiceImpl::GetStoreMetaData(const AppId &appId, const StoreId &storeId)
{
    StoreMetaData metaData;
    metaData.deviceId = AppDistributedKv::CommunicationProvider::GetInstance().GetLocalDevice().uuid;
    metaData.uid = IPCSkeleton::GetCallingUid();
    metaData.tokenId = IPCSkeleton::GetCallingTokenID();
    metaData.bundleName = appId.appId;
    metaData.storeId = storeId.storeId;
    metaData.user = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(metaData.uid);
    metaData.instanceId = GetInstIndex(metaData.tokenId, appId);
    return metaData;
}

StrategyMeta KVDBServiceImpl::GetStrategyMeta(const AppId &appId, const StoreId &storeId)
{
    auto deviceId = AppDistributedKv::CommunicationProvider::GetInstance().GetLocalDevice().uuid;
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
        ZLOGE("GetHapTokenInfo error:%{public}d, appId:%{public}s", errCode, appId.appId.c_str());
        return -1;
    }
    return tokenInfo.instIndex;
}

} // namespace OHOS::DistributedKv