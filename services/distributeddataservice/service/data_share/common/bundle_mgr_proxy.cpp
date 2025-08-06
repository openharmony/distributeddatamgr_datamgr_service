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
#include "common_utils.h"
#include "datashare_errno.h"
#include "datashare_radar_reporter.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "system_ability_definition.h"
#include "uri_utils.h"
#include "ipc_skeleton.h"
#include "hiview_fault_adapter.h"

namespace OHOS::DataShare {
sptr<AppExecFwk::BundleMgrProxy> BundleMgrProxy::GetBundleMgrProxy()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (proxy_ != nullptr) {
        return iface_cast<AppExecFwk::BundleMgrProxy>(proxy_);
    }
    proxy_ = CheckBMS();
    if (proxy_ == nullptr) {
        ZLOGE("BMS service not ready to complete.");
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

sptr<IRemoteObject> BundleMgrProxy::CheckBMS()
{
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        ZLOGE("Failed to get system ability mgr.");
        return nullptr;
    }
    return systemAbilityManager->CheckSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
}

int BundleMgrProxy::GetBundleInfoFromBMS(
    const std::string &bundleName, int32_t userId, BundleConfig &bundleConfig, int32_t appIndex)
{
    std::string bundleKey = bundleName + std::to_string(userId);
    if (appIndex != 0) {
        bundleKey += "appIndex" + std::to_string(appIndex);
    }
    auto it = bundleCache_.Find(bundleKey);
    if (it.first) {
        bundleConfig = it.second;
        return E_OK;
    }
    auto bmsClient = GetBundleMgrProxy();
    if (bmsClient == nullptr) {
        ZLOGE("GetBundleMgrProxy is nullptr!");
        return E_BMS_NOT_READY;
    }
    AppExecFwk::BundleInfo bundleInfo;
    bool ret;
    TimeoutReport timeoutReport({bundleName, "", "", __FUNCTION__, 0});
    if (appIndex == 0) {
        ret = bmsClient->GetBundleInfo(
            bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_WITH_EXTENSION_INFO, bundleInfo, userId);
    } else {
        ret = bmsClient->GetCloneBundleInfo(bundleName,
            static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_EXTENSION_ABILITY) |
            static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_HAP_MODULE),
            appIndex, bundleInfo, userId);
        // when there is no error, the former function returns 1 while the new function returns 0
        ret = !ret;
        for (auto &item : bundleInfo.hapModuleInfos) {
            for (auto &item2 : item.extensionInfos) {
                bundleInfo.extensionInfos.push_back(item2);
            }
        }
    }
    timeoutReport.Report(std::to_string(userId), IPCSkeleton::GetCallingPid(), appIndex);
    if (!ret) {
        ZLOGE("GetBundleInfo failed!bundleName is %{public}s, userId is %{public}d", bundleName.c_str(), userId);
        return E_ERROR;
    }
    auto [errCode, bundle] = ConvertToDataShareBundle(bundleInfo);
    if (errCode != E_OK) {
        ZLOGE("Profile Unmarshall failed! bundleName:%{public}s", URIUtils::Anonymous(bundle.name).c_str());
        return errCode;
    }
    bundleConfig = bundle;
    bundleCache_.Insert(bundleKey, bundle);
    return E_OK;
}

int BundleMgrProxy::GetBundleInfoFromBMSWithCheck(
    const std::string &bundleName, int32_t userId, BundleConfig &bundleConfig, int32_t appIndex)
{
    int res = GetBundleInfoFromBMS(bundleName, userId, bundleConfig, appIndex);
    if (res != E_OK) {
        return res;
    }
    // Not allow normal app visit normal app.
    if (!DataShareThreadLocal::IsFromSystemApp() && !bundleConfig.isSystemApp) {
        ZLOGE("Not allow normal app visit normal app, bundle:%{public}s, callingPid:%{public}d",
            bundleName.c_str(), IPCSkeleton::GetCallingPid());
        return E_NOT_SYSTEM_APP;
    }

    return E_OK;
}

std::pair<int, std::string> BundleMgrProxy::GetCallerAppIdentifier(
    const std::string &bundleName, int32_t userId)
{
    std::string callerAppIdentifier;
    std::string callerKey = bundleName + std::to_string(userId);
    auto it = callerInfoCache_.Find(callerKey);
    if (it.first) {
        callerAppIdentifier = it.second;
        return std::make_pair(E_OK, callerAppIdentifier);
    }
    AppExecFwk::BundleInfo bundleInfo;
    auto bmsClient = GetBundleMgrProxy();
    if (bmsClient == nullptr) {
        RADAR_REPORT(__FUNCTION__, RadarReporter::SILENT_ACCESS, RadarReporter::GET_BMS,
            RadarReporter::FAILED, RadarReporter::ERROR_CODE, RadarReporter::GET_BMS_FAILED);
        ZLOGE("GetBundleMgrProxy is nullptr!");
        return std::make_pair(E_BMS_NOT_READY, bundleInfo.signatureInfo.appIdentifier);
    }

    int ret = bmsClient->GetBundleInfoV9(bundleName,
        static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_SIGNATURE_INFO), bundleInfo, userId);
    if (ret != 0) {
        RADAR_REPORT(__FUNCTION__, RadarReporter::SILENT_ACCESS, RadarReporter::GET_BMS,
            RadarReporter::FAILED, RadarReporter::ERROR_CODE, RadarReporter::GET_BUNDLE_INFP_FAILED);
        ZLOGE("GetBundleInfo failed!bundleName is %{public}s, userId is %{public}d, errcode is %{public}d",
            bundleName.c_str(), userId, ret);
        return std::make_pair(E_BUNDLE_NAME_NOT_EXIST, bundleInfo.signatureInfo.appIdentifier);
    }

    callerInfoCache_.Insert(callerKey, bundleInfo.signatureInfo.appIdentifier);
    return std::make_pair(ret, bundleInfo.signatureInfo.appIdentifier);
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

void BundleMgrProxy::Delete(const std::string &bundleName, int32_t userId, int32_t appIndex)
{
    if (appIndex != 0) {
        bundleCache_.Erase(bundleName + std::to_string(userId) + "appIndex" + std::to_string(appIndex));
    } else {
        bundleCache_.Erase(bundleName + std::to_string(userId));
    }
    callerInfoCache_.Erase(bundleName + std::to_string(userId));
    return;
}

std::shared_ptr<BundleMgrProxy> BundleMgrProxy::GetInstance()
{
    static std::shared_ptr<BundleMgrProxy> proxy = std::make_shared<BundleMgrProxy>();
    return proxy;
}

std::pair<int, BundleConfig> BundleMgrProxy::ConvertToDataShareBundle(AppExecFwk::BundleInfo &bundleInfo)
{
    BundleConfig bundleConfig;
    bundleConfig.name = std::move(bundleInfo.name);
    bundleConfig.singleton = std::move(bundleInfo.singleton);
    auto [errCode, hapModuleInfos] = ConvertHapModuleInfo(bundleInfo);
    if (errCode != E_OK) {
        return std::make_pair(errCode, bundleConfig);
    }
    bundleConfig.hapModuleInfos = hapModuleInfos;

    auto [err, extensionInfos] = ConvertExtensionAbility(bundleInfo);
    if (err != E_OK) {
        return std::make_pair(err, bundleConfig);
    }
    bundleConfig.extensionInfos = extensionInfos;
    bundleConfig.isSystemApp = bundleInfo.applicationInfo.isSystemApp;
    return std::make_pair(E_OK, bundleConfig);
}

std::pair<int, std::vector<ExtensionAbilityInfo>> BundleMgrProxy::ConvertExtensionAbility(
    AppExecFwk::BundleInfo &bundleInfo)
{
    std::vector<ExtensionAbilityInfo> extensionInfos;
    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type != AppExecFwk::ExtensionAbilityType::DATASHARE) {
            continue;
        }
        ExtensionAbilityInfo extensionInfo;
        extensionInfo.type = std::move(item.type);
        extensionInfo.readPermission = std::move(item.readPermission);
        extensionInfo.writePermission = std::move(item.writePermission);
        extensionInfo.uri = std::move(item.uri);
        extensionInfo.resourcePath = std::move(item.resourcePath);
        extensionInfo.hapPath = std::move(item.hapPath);
        ProfileConfig profileConfig;
        auto [ret, profileInfo] = DataShareProfileConfig::GetDataProperties(
            item.metadata, extensionInfo.resourcePath, extensionInfo.hapPath, DATA_SHARE_EXTENSION_META);
        if (ret == NOT_FOUND) {
            profileConfig.resultCode = NOT_FOUND;
        }
        if (ret == ERROR) {
            profileConfig.resultCode = ERROR;
            ZLOGE("Profile unmarshall error.uri: %{public}s", URIUtils::Anonymous(extensionInfo.uri).c_str());
            return std::make_pair(E_ERROR, extensionInfos);
        }
        profileConfig.profile = profileInfo;
        extensionInfo.profileInfo = profileConfig;
        extensionInfos.emplace_back(extensionInfo);
    }
    return std::make_pair(E_OK, extensionInfos);
}

std::pair<int, std::vector<HapModuleInfo>> BundleMgrProxy::ConvertHapModuleInfo(AppExecFwk::BundleInfo &bundleInfo)
{
    std::vector<HapModuleInfo> hapModuleInfos;
    for (auto &item : bundleInfo.hapModuleInfos) {
        HapModuleInfo hapModuleInfo;
        hapModuleInfo.resourcePath = std::move(item.resourcePath);
        hapModuleInfo.hapPath = std::move(item.hapPath);
        hapModuleInfo.moduleName = std::move(item.moduleName);
        hapModuleInfo.crossAppSharedConfig.resourcePath = std::move(item.crossAppSharedConfig);
        std::vector<ProxyData> proxyDatas;
        for (auto &proxyData : item.proxyDatas) {
            ProxyData data;
            data.uri = std::move(proxyData.uri);
            data.requiredReadPermission = std::move(proxyData.requiredReadPermission);
            data.requiredWritePermission = std::move(proxyData.requiredWritePermission);
            ProfileConfig profileConfig;
            auto [ret, profileInfo] = DataShareProfileConfig::GetDataProperties(
                std::vector<AppExecFwk::Metadata>{proxyData.metadata}, hapModuleInfo.resourcePath,
                hapModuleInfo.hapPath, DATA_SHARE_PROPERTIES_META);
            if (ret == NOT_FOUND) {
                profileConfig.resultCode = NOT_FOUND;
            }
            if (ret == ERROR) {
                profileConfig.resultCode = ERROR;
                ZLOGE("Profile unmarshall error.uri: %{public}s", URIUtils::Anonymous(data.uri).c_str());
                return std::make_pair(E_ERROR, hapModuleInfos);
            }
            profileConfig.profile = profileInfo;
            data.profileInfo = profileConfig;
            proxyDatas.emplace_back(data);
        }
        hapModuleInfo.proxyDatas = proxyDatas;
        hapModuleInfos.emplace_back(hapModuleInfo);
    }
    return std::make_pair(E_OK, hapModuleInfos);
}

std::pair<int, bool> BundleMgrProxy::CheckSilentConfig(const std::string &bundleName, int32_t userId)
{
    AppExecFwk::BundleInfo bundleInfo;
    auto bmsClient = GetBundleMgrProxy();
    if (bmsClient == nullptr) {
        ZLOGE(
            "GetBundleMgrProxy is nullptr! bundleName is %{public}s, userId is %{public}d", bundleName.c_str(), userId);
        return std::make_pair(E_BMS_NOT_READY, false);
    }

    auto ret = bmsClient->GetBundleInfo(bundleName,
        static_cast<int32_t>(AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_HAP_MODULE), bundleInfo, userId);
    if (!ret) {
        ZLOGE("Get bundleInfo failed! ret: %{public}d", ret);
        return std::make_pair(E_ERROR, false);
    }

    for (auto &hapModuleInfo : bundleInfo.hapModuleInfos) {
        if (!hapModuleInfo.proxyDatas.empty()) {
            return std::make_pair(E_OK, true);
        }
    }
    return std::make_pair(E_SILENT_PROXY_DISABLE, false);
}
} // namespace OHOS::DataShare