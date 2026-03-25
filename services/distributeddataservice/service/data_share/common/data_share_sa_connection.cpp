/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define LOG_TAG "DSSaConnection"

#include "data_share_sa_connection.h"
#include "datashare_errno.h"
#include "log_print.h"
#include "string_ex.h"

namespace OHOS::DataShare {
static constexpr const char *DATA_SHARE_CONNECTION_EXTENSION = "datashare_connection";
std::pair<int32_t, ConnectionInterfaceInfo> SAConnection::GetConnectionInterfaceInfo()
{
    if (!CheckAndLoadSystemAbility()) {
        return std::make_pair(E_SYSTEM_ABILITY_OPERATE_FAILED, ConnectionInterfaceInfo());
    }
    auto localAbilityManager = GetLocalAbilityManager();
    if (localAbilityManager == nullptr) {
        return std::make_pair(E_SYSTEM_ABILITY_OPERATE_FAILED, ConnectionInterfaceInfo());
    }
    // No need to add a lock, as each call creates a new object and there is no concurrency
    int32_t ret = localAbilityManager->SystemAbilityExtProc(std::string(DATA_SHARE_CONNECTION_EXTENSION),
        systemAbilityId_, this);
    if (ret != ERR_OK) {
        ZLOGE("SystemAbilityExtProc failed, err: %{public}d", ret);
        return std::make_pair(E_SYSTEM_ABILITY_OPERATE_FAILED, ConnectionInterfaceInfo());
    }
    ZLOGI("success, code: %{public}d, descriptor: %{public}s", code_,
        Str16ToStr8(descriptor_).c_str());
    return std::make_pair(ERR_OK, ConnectionInterfaceInfo(code_, descriptor_));
}

bool SAConnection::CheckAndLoadSystemAbility()
{
    auto manager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (manager == nullptr) {
        ZLOGE("get system ability manager failed");
        return false;
    }
    auto remoteObject = manager->CheckSystemAbility(systemAbilityId_);
    if (remoteObject == nullptr) {
        // check SA failed, try load SA
        ZLOGW("get system ability failed, saId: %{public}d", systemAbilityId_);
        sptr<SALoadCallback> loadCallback(new SALoadCallback());
        if (loadCallback == nullptr) {
            ZLOGE("loadCallback is nullptr");
            return false;
        }
        int32_t ret = manager->LoadSystemAbility(systemAbilityId_, loadCallback);
        if (ret != ERR_OK) {
            ZLOGE("failed to Load sa, saId:%{public}d. err:%{public}d", systemAbilityId_, ret);
            return false;
        }
        std::unique_lock<std::mutex> lock(mutex_);
        if (loadCallback->proxyConVar_.wait_for(lock, std::chrono::seconds(waitTime_),
            [loadCallback]() { return loadCallback->isLoadSuccess_.load(); })) {
            return true;
        } else {
            ZLOGE("Load sa timeout, saId:%{public}d", systemAbilityId_);
            return false;
        }
    }
    return true;
}

sptr<ILocalAbilityManager> SAConnection::GetLocalAbilityManager()
{
    std::vector<ISystemAbilityManager::SaExtensionInfo> saExtentionInfos;
    sptr<ISystemAbilityManager> manager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (manager == nullptr) {
        ZLOGE("get system ability manager failed");
        return nullptr;
    }
    int32_t ret = manager->GetRunningSaExtensionInfoList(std::string(DATA_SHARE_CONNECTION_EXTENSION),
        saExtentionInfos);
    if (ret != ERR_OK) {
        ZLOGE("GetRunningSaExtensionInfoList err: %{public}d.", ret);
        return nullptr;
    }
    for (const auto& saExtentionInfo : saExtentionInfos) {
        if (saExtentionInfo.saId == systemAbilityId_) {
            return iface_cast<ILocalAbilityManager>(saExtentionInfo.processObj);
        }
    }
    ZLOGE("ExtensionInfoList not found saId: %{public}d.", systemAbilityId_);
    return nullptr;
}

void SAConnection::SALoadCallback::OnLoadSystemAbilitySuccess(int32_t systemAbilityId,
    const OHOS::sptr<IRemoteObject> &remoteObject)
{
    ZLOGI("Load sa success, saId: %{public}d, remoteObject:%{public}s", systemAbilityId,
        (remoteObject != nullptr) ? "true" : "false");
    isLoadSuccess_.store(remoteObject != nullptr);
    proxyConVar_.notify_one();
}

void SAConnection::SALoadCallback::OnLoadSystemAbilityFail(int32_t systemAbilityId)
{
    ZLOGE("Load sa failed, saId:%{public}d", systemAbilityId);
    isLoadSuccess_.store(false);
    proxyConVar_.notify_one();
}

bool SAConnection::InputParaSet(MessageParcel &data)
{
    return true;
}

bool SAConnection::OutputParaGet(MessageParcel &reply)
{
    descriptor_ = reply.ReadInterfaceToken();
    code_ = reply.ReadInt32();
    return true;
}
} // namespace OHOS::DataShare