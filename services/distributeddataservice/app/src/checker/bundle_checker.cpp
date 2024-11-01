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
#define LOG_TAG "BundleChecker"

#include "bundle_checker.h"
#include <iservice_registry.h>
#include <memory>
#include <system_ability_definition.h>
#include "accesstoken_kit.h"
#include "bundlemgr/bundle_mgr_proxy.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "utils/crypto.h"

namespace OHOS {
namespace DistributedData {
using namespace Security::AccessToken;
__attribute__((used)) BundleChecker BundleChecker::instance_;
BundleChecker::BundleChecker() noexcept
{
    CheckerManager::GetInstance().RegisterPlugin(
        "BundleChecker", [this]() -> auto { return this; });
}

BundleChecker::~BundleChecker()
{
}

void BundleChecker::Initialize()
{
}

bool BundleChecker::SetTrustInfo(const CheckerManager::Trust &trust)
{
    trusts_[trust.bundleName] = trust.appId;
    return true;
}

bool BundleChecker::SetDistrustInfo(const CheckerManager::Distrust &distrust)
{
    distrusts_[distrust.bundleName] = distrust.appId;
    return true;
}

bool BundleChecker::SetSwitchesInfo(const CheckerManager::Switches &switches)
{
    switches_[switches.bundleName] = switches.appId;
    return true;
}

std::string BundleChecker::GetBundleAppId(const CheckerManager::StoreInfo &info)
{
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgrProxy == nullptr) {
        ZLOGE("Failed to get system ability mgr.");
        return "";
    }
    auto bundleMgrProxy = samgrProxy->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (bundleMgrProxy == nullptr) {
        ZLOGE("Failed to GetSystemAbility.");
        return "";
    }
    auto bundleManager = iface_cast<AppExecFwk::IBundleMgr>(bundleMgrProxy);
    if (bundleManager == nullptr) {
        ZLOGE("Failed to get bundle manager");
        return "";
    }
    int32_t userId = IPCSkeleton::GetCallingUid() / OHOS::AppExecFwk::Constants::BASE_USER_RANGE;
    std::string appId = bundleManager->GetAppIdByBundleName(info.bundleName, userId);
    if (appId.empty()) {
        ZLOGE("GetAppIdByBundleName failed appId:%{public}s, bundleName:%{public}s, uid:%{public}d",
            appId.c_str(), info.bundleName.c_str(), userId);
        return "";
    }
    return appId;
}

std::string BundleChecker::GetAppId(const CheckerManager::StoreInfo &info)
{
    auto appId = GetBundleAppId(info);
    if (appId.empty()) {
        return "";
    }
    auto it = trusts_.find(info.bundleName);
    if (it != trusts_.end() && (it->second == appId)) {
        return info.bundleName;
    }
    ZLOGD("bundleName:%{public}s, appId:%{public}s", info.bundleName.c_str(), appId.c_str());
    return Crypto::Sha256(appId);
}

bool BundleChecker::IsValid(const CheckerManager::StoreInfo &info)
{
    if (AccessTokenKit::GetTokenTypeFlag(info.tokenId) != TOKEN_HAP) {
        return false;
    }

    HapTokenInfo tokenInfo;
    if (AccessTokenKit::GetHapTokenInfo(info.tokenId, tokenInfo) != RET_SUCCESS) {
        return false;
    }

    return tokenInfo.bundleName == info.bundleName;
}

bool BundleChecker::IsDistrust(const CheckerManager::StoreInfo &info)
{
    auto appId = GetBundleAppId(info);
    if (appId.empty()) {
        return false;
    }
    auto it = distrusts_.find(info.bundleName);
    if (it != distrusts_.end() && (it->second == appId)) {
        return true;
    }
    return false;
}

bool BundleChecker::IsSwitches(const CheckerManager::StoreInfo &info)
{
    return false;
}

std::vector<CheckerManager::StoreInfo> BundleChecker::GetDynamicStores()
{
    return dynamicStores_;
}

std::vector<CheckerManager::StoreInfo> BundleChecker::GetStaticStores()
{
    return staticStores_;
}

bool BundleChecker::IsDynamic(const CheckerManager::StoreInfo &info)
{
    for (const auto &store : dynamicStores_) {
        if (info.bundleName == store.bundleName && info.storeId == store.storeId) {
            return true;
        }
    }
    return false;
}

bool BundleChecker::IsStatic(const CheckerManager::StoreInfo &info)
{
    for (const auto &store : staticStores_) {
        if (info.bundleName == store.bundleName && info.storeId == store.storeId) {
            return true;
        }
    }
    return false;
}

bool BundleChecker::AddDynamicStore(const CheckerManager::StoreInfo &storeInfo)
{
    dynamicStores_.push_back(storeInfo);
    return true;
}

bool BundleChecker::AddStaticStore(const CheckerManager::StoreInfo &storeInfo)
{
    staticStores_.push_back(storeInfo);
    return true;
}
} // namespace DistributedData
} // namespace OHOS