/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
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
#define LOG_TAG "BundleMgrAdapter"

#include "bundlemgr_adapter.h"
#include <memory>
#include "accesstoken_kit.h"
#include "bundlemgr/bundle_mgr_proxy.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace DistributedData {
BundleMgrAdapter::BundleMgrAdapter()
{
}
BundleMgrAdapter::~BundleMgrAdapter()
{
}
BundleMgrAdapter& BundleMgrAdapter::GetInstance()
{
    static BundleMgrAdapter instance;
    return instance;
}

std::string BundleMgrAdapter::GetKey(const std::string &bundleName, int32_t userId)
{
    return bundleName + "###" + std::to_string(userId);
}

std::string BundleMgrAdapter::GetAppidFromCache(const std::string &bundleName, int32_t userId)
{
    std::string appId;
    std::string key = GetKey(bundleName, userId);
    appIds_.Get(key, appId);
    return appId;
}

std::string BundleMgrAdapter::GetBundleAppId(const std::string &bundleName, int32_t userId)
{
    std::string appId = GetAppidFromCache(bundleName, userId);
    if (!appId.empty()) {
        return appId;
    }
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgrProxy == nullptr) {
        ZLOGE("Failed to get system ability mgr.");
        return "";
    }
    auto bundleMgrProxy = samgrProxy->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (bundleMgrProxy == nullptr) {
        ZLOGE("Failed to Get BMS SA.");
        return "";
    }
    auto bundleManager = iface_cast<AppExecFwk::IBundleMgr>(bundleMgrProxy);
    if (bundleManager == nullptr) {
        ZLOGE("Failed to get bundle manager");
        return "";
    }
    appId = bundleManager->GetAppIdByBundleName(bundleName, userId);
    if (appId.empty()) {
        ZLOGE("GetAppIdByBundleName failed appId:%{public}s, bundleName:%{public}s, uid:%{public}d",
            appId.c_str(), bundleName.c_str(), userId);
    } else {
        appIds_.Set(GetKey(bundleName, userId), appId);
    }
    return appId;
}

void BundleMgrAdapter::DeleteCache(const std::string &bundleName, int32_t user)
{
    std::string key = GetKey(bundleName, user);
    appIds_.Delete(key);
}

void BundleMgrAdapter::ClearCache()
{
    appIds_.ResetCapacity(0);
    appIds_.ResetCapacity(CACHE_SIZE);
}
} // namespace DistributedData
} // namespace OHOS