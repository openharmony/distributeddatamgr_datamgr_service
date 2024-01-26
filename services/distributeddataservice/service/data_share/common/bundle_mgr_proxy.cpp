/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
        return iface_cast<AppExecFwk::BundleMgrProxy>(proxy_);
    }
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        ZLOGE("Failed to get system ability mgr.");
        return nullptr;
    }

    proxy_ = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (proxy_ == nullptr) {
        ZLOGE("Failed to get bundle manager proxy.");
        return nullptr;
    }
    deathRecipient_ = new (std::nothrow)BundleMgrProxy::ServiceDeathRecipient(weak_from_this());
    if (deathRecipient_ == nullptr) {
        ZLOGE("deathRecipient alloc failed.");
        return nullptr;
    }
    if (!proxy_->AddDeathRecipient(deathRecipient_)) {
        ZLOGE("add death recipient failed.");
        proxy_ = nullptr;
        deathRecipient_ = nullptr;
        return nullptr;
    }
    return iface_cast<AppExecFwk::BundleMgrProxy>(proxy_);
}

bool BundleMgrProxy::GetBundleInfoFromBMS(
    const std::string &bundleName, int32_t userId, AppExecFwk::BundleInfo &bundleInfo)
{
    auto bundleKey = bundleName + std::to_string(userId);
    auto it = bundleCache_.Find(bundleKey);
    if (it.first) {
        bundleInfo = it.second;
        return true;
    }
    auto bmsClient = GetBundleMgrProxy();
    if (bmsClient == nullptr) {
        ZLOGE("GetBundleMgrProxy is nullptr!");
        return false;
    }
    bool ret = bmsClient->GetBundleInfo(
        bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_WITH_EXTENSION_INFO, bundleInfo, userId);
    if (!ret) {
        ZLOGE("GetBundleInfo failed!bundleName is %{public}s, userId is %{public}d", bundleName.c_str(), userId);
        return false;
    }
    bundleCache_.Insert(bundleKey, bundleInfo);
    return true;
}

void BundleMgrProxy::OnProxyDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    ZLOGE("bundleMgr died, proxy=null ? %{public}s.", proxy_ == nullptr ? "true" : "false");
    if (proxy_ != nullptr) {
        proxy_->RemoveDeathRecipient(deathRecipient_);
    }
    proxy_ = nullptr;
    deathRecipient_ = nullptr;
}

BundleMgrProxy::~BundleMgrProxy()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (proxy_ != nullptr) {
        proxy_->RemoveDeathRecipient(deathRecipient_);
    }
}

void BundleMgrProxy::Delete(const std::string &bundleName, int32_t userId)
{
    bundleCache_.Erase(bundleName + std::to_string(userId));
    return;
}

std::shared_ptr<BundleMgrProxy> BundleMgrProxy::GetInstance()
{
    static std::shared_ptr<BundleMgrProxy> proxy(new BundleMgrProxy());
    return proxy;
}
} // namespace OHOS::DataShare