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

#define LOG_TAG "BmsDelegateImpl"
#include "bms_delegate_impl.h"

#include <memory>

#include "bundlemgr/bundle_mgr_interface.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "system_ability_definition.h"

namespace OHOS::DistributedData {

__attribute__((used)) static bool g_init =
    BmsDelegate::RegisterInstance(std::static_pointer_cast<BmsDelegate>(std::make_shared<BmsDelegateImpl>()));

sptr<AppExecFwk::IBundleMgr> BmsDelegateImpl::GetBundleMgr()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (object_ != nullptr) {
        return iface_cast<AppExecFwk::IBundleMgr>(object_);
    }
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        ZLOGE("failed to get system ability mgr");
        return nullptr;
    }
    object_ = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (object_ == nullptr) {
        ZLOGE("BMS service not ready");
        return nullptr;
    }
    deathRecipient_ = new (std::nothrow) BmsDelegateImpl::ServiceDeathRecipient(this);
    if (deathRecipient_ == nullptr) {
        ZLOGE("deathRecipient alloc failed");
        object_ = nullptr;
        return nullptr;
    }
    if (!object_->AddDeathRecipient(deathRecipient_)) {
        ZLOGE("add death recipient failed");
        object_ = nullptr;
        deathRecipient_ = nullptr;
        return nullptr;
    }
    return iface_cast<AppExecFwk::IBundleMgr>(object_);
}

std::string BmsDelegateImpl::GetCallerAppIdentifier(const std::string &bundleName, int32_t userId)
{
    auto bmsClient = GetBundleMgr();
    if (bmsClient == nullptr) {
        ZLOGE("GetBundleMgr is nullptr");
        return "";
    }
    AppExecFwk::BundleInfo bundleInfo;
    int32_t flag = static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_SIGNATURE_INFO);
    int32_t ret = bmsClient->GetBundleInfoV9(bundleName, flag, bundleInfo, userId);
    if (ret != ERR_OK) {
        ZLOGE("GetBundleInfoV9 failed, bundleName:%{public}s, userId:%{public}d, ret:%{public}d",
            bundleName.c_str(), userId, ret);
        return "";
    }
    return bundleInfo.signatureInfo.appIdentifier;
}

void BmsDelegateImpl::OnRemoteDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    ZLOGE("remote object died, object=null ? %{public}s.", object_ == nullptr ? "true" : "false");
    if (object_ != nullptr) {
        object_->RemoveDeathRecipient(deathRecipient_);
    }
    object_ = nullptr;
    deathRecipient_ = nullptr;
}

BmsDelegateImpl::~BmsDelegateImpl()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (object_ != nullptr) {
        object_->RemoveDeathRecipient(deathRecipient_);
    }
}
} // namespace OHOS::DistributedData
