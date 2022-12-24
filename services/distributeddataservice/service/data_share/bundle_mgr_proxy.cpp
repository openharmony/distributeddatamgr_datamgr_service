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
#define LOG_TAG "BundleMgrProxy"
#include "bundle_mgr_proxy.h"

#include "account/account_delegate.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "system_ability_definition.h"

namespace OHOS::DataShare {
sptr<AppExecFwk::BundleMgrProxy> BundleMgrProxy::GetBundleMgrProxy()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (proxy_ != nullptr) {
        return proxy_;
    }
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        ZLOGE("Failed to get system ability mgr.");
        return nullptr;
    }

    sptr<IRemoteObject> remoteObject = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (remoteObject == nullptr) {
        ZLOGE("Failed to get bundle manager proxy.");
        return nullptr;
    }
    sptr<BundleMgrProxy::ServiceDeathRecipient> deathRecipient = new (std::nothrow)
        BundleMgrProxy::ServiceDeathRecipient(this);
    remoteObject->AddDeathRecipient(deathRecipient);
    proxy_ = iface_cast<AppExecFwk::BundleMgrProxy>(remoteObject);
    ZLOGD("Get bundle manager proxy success.");
    return proxy_;
}

bool BundleMgrProxy::GetBundleInfoFromBMS(
    const std::string &bundleName, uint32_t tokenId, AppExecFwk::BundleInfo &bundleInfo)
{
    auto userId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(tokenId);
    auto bmsClient = GetBundleMgrProxy();
    if (!bmsClient) {
        ZLOGE("GetBundleMgrProxy is nullptr!");
        return false;
    }
    bool ret = bmsClient->GetBundleInfo(
        bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_WITH_EXTENSION_INFO, bundleInfo, userId);
    if (!ret) {
        ZLOGE("GetBundleInfo failed!");
        return false;
    }
    return true;
}

void BundleMgrProxy::OnProxyDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    proxy_ = nullptr;
    ZLOGE("DIED.");
}
} // namespace OHOS::DataShare