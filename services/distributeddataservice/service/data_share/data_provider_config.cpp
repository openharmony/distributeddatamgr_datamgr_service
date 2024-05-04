/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "DataProviderConfig"

#include "data_provider_config.h"

#include <vector>

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "datashare_errno.h"
#include "hap_token_info.h"
#include "log_print.h"
#include "strategies/general/load_config_common_strategy.h"
#include "uri_utils.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
using namespace OHOS::DistributedData;
DataProviderConfig::DataProviderConfig(const std::string &uri, uint32_t callerTokenId)
{
    providerInfo_.uri = uri;
    providerInfo_.currentUserId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(callerTokenId);
    if (providerInfo_.currentUserId == 0) {
        LoadConfigCommonStrategy::GetInfoFromProxyURI(providerInfo_.uri, providerInfo_.currentUserId,
            callerTokenId, providerInfo_.bundleName);
        URIUtils::FormatUri(providerInfo_.uri);
    }
    uriConfig_ = URIUtils::GetUriConfig(providerInfo_.uri);
}

std::pair<int, BundleInfo> DataProviderConfig::GetBundleInfo()
{
    BundleInfo bundleInfo;
    providerInfo_.bundleName = uriConfig_.authority;
    if (providerInfo_.bundleName.empty()) {
        if (uriConfig_.pathSegments.empty()) {
            return std::make_pair(E_URI_NOT_EXIST, bundleInfo);
        }
        providerInfo_.bundleName = uriConfig_.pathSegments[0];
    }
    if (!BundleMgrProxy::GetInstance()->GetBundleInfoFromBMS(
        providerInfo_.bundleName, providerInfo_.currentUserId, bundleInfo)) {
        return std::make_pair(E_BUNDLE_NAME_NOT_EXIST, bundleInfo);
    }
    return std::make_pair(E_OK, bundleInfo);
}

int DataProviderConfig::GetFromProxyData()
{
    auto [errCode, bundleInfo] = GetBundleInfo();
    if (errCode != E_OK) {
        ZLOGE("Get bundleInfo failed! bundleName:%{public}s, userId:%{public}d, uri:%{public}s",
            providerInfo_.bundleName.c_str(), providerInfo_.currentUserId,
            URIUtils::Anonymous(providerInfo_.uri).c_str());
        return errCode;
    }
    providerInfo_.singleton = bundleInfo.singleton;
    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type != AppExecFwk::ExtensionAbilityType::DATASHARE) {
            continue;
        }
        providerInfo_.hasExtension = true;
        break;
    }
    for (auto &hapModuleInfo : bundleInfo.hapModuleInfos) {
        auto &proxyDatas = hapModuleInfo.proxyDatas;
        std::sort(proxyDatas.begin(), proxyDatas.end(), [](const AppExecFwk::ProxyData &curr,
            const AppExecFwk::ProxyData &prev) {
            return curr.uri.length() > prev.uri.length();
        });
        for (auto &data : proxyDatas) {
            if (data.uri.length() > uriConfig_.formatUri.length() ||
                uriConfig_.formatUri.compare(0, data.uri.length(), data.uri) != 0) {
                continue;
            }
            providerInfo_.readPermission = std::move(data.requiredReadPermission);
            providerInfo_.writePermission = std::move(data.requiredWritePermission);
            auto [ret, profileInfo] = DataShareProfileConfig::GetDataProperties(
                std::vector<AppExecFwk::Metadata>{data.metadata}, hapModuleInfo.resourcePath,
                hapModuleInfo.hapPath, DATA_SHARE_PROPERTIES_META);
            if (ret == NOT_FOUND) {
                return E_OK;
            }
            if (ret == ERROR) {
                ZLOGE("Profile unmarshall error.uri: %{public}s", URIUtils::Anonymous(providerInfo_.uri).c_str());
                return E_ERROR;
            }
            return GetFromDataProperties(profileInfo, hapModuleInfo.moduleName);
        }
    }
    return E_URI_NOT_EXIST;
}

int DataProviderConfig::GetFromDataProperties(const ProfileInfo &profileInfo,
    const std::string &moduleName)
{
    if (profileInfo.scope == MODULE_SCOPE) {
        providerInfo_.moduleName = moduleName;
    }
    providerInfo_.storeName = profileInfo.storeName;
    providerInfo_.tableName = profileInfo.tableName;
    providerInfo_.type = profileInfo.type;
    if (profileInfo.tableConfig.empty()) {
        return E_OK;
    }
    return GetFromExtensionProperties(profileInfo, moduleName);
}

int DataProviderConfig::GetFromExtensionProperties(const ProfileInfo &profileInfo,
    const std::string &moduleName)
{
    std::string storeUri = URIUtils::DATA_SHARE_SCHEMA + providerInfo_.bundleName + URIUtils::URI_SEPARATOR +
            moduleName + URIUtils::URI_SEPARATOR + providerInfo_.storeName;
    std::string tableUri = storeUri + URIUtils::URI_SEPARATOR + providerInfo_.tableName;
    providerInfo_.accessCrossMode = DataShareProfileConfig::GetAccessCrossMode(profileInfo, tableUri, storeUri);
    if (providerInfo_.singleton && providerInfo_.accessCrossMode == AccessCrossMode::USER_UNDEFINED) {
        ZLOGE("Single app must config user cross mode,bundleName:%{public}s, uri:%{public}s",
            providerInfo_.bundleName.c_str(), URIUtils::Anonymous(providerInfo_.uri).c_str());
        return E_ERROR;
    }
    if (providerInfo_.singleton && providerInfo_.accessCrossMode == AccessCrossMode::USER_SINGLE) {
        providerInfo_.tableName.append("_").append(std::to_string(providerInfo_.currentUserId));
    }
    return E_OK;
}

int DataProviderConfig::GetFromExtension()
{
    if (!GetFromUriPath()) {
        ZLOGE("Uri Path failed! uri:%{public}s", URIUtils::Anonymous(providerInfo_.uri).c_str());
        return E_URI_NOT_EXIST;
    }
    BundleInfo bundleInfo;
    if (!BundleMgrProxy::GetInstance()->GetBundleInfoFromBMS(
        providerInfo_.bundleName, providerInfo_.currentUserId, bundleInfo)) {
        ZLOGE("BundleInfo failed! bundleName: %{public}s", providerInfo_.bundleName.c_str());
        return E_BUNDLE_NAME_NOT_EXIST;
    }
    providerInfo_.singleton = bundleInfo.singleton;
    providerInfo_.allowEmptyPermission = true;
    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type != AppExecFwk::ExtensionAbilityType::DATASHARE) {
            continue;
        }
        providerInfo_.hasExtension = true;
        providerInfo_.readPermission = std::move(item.readPermission);
        providerInfo_.writePermission = std::move(item.writePermission);
        auto [ret, profileInfo] = DataShareProfileConfig::GetDataProperties(
            item.metadata, item.resourcePath, item.hapPath, DATA_SHARE_EXTENSION_META);
        if (ret == NOT_FOUND) {
            return E_OK;
        }
        if (ret == ERROR) {
            ZLOGE("Profile Unmarshall failed! uri:%{public}s", URIUtils::Anonymous(providerInfo_.uri).c_str());
            return E_ERROR;
        }
        return GetFromExtensionProperties(profileInfo, providerInfo_.moduleName);
    }
    return E_URI_NOT_EXIST;
}

bool DataProviderConfig::GetFromUriPath()
{
    auto& pathSegments = uriConfig_.pathSegments;
    if (pathSegments.size() < static_cast<std::size_t>(PATH_PARAM::PARAM_SIZE) ||
        pathSegments[static_cast<std::size_t>(PATH_PARAM::BUNDLE_NAME)].empty() ||
        pathSegments[static_cast<std::size_t>(PATH_PARAM::MODULE_NAME)].empty() ||
        pathSegments[static_cast<std::size_t>(PATH_PARAM::STORE_NAME)].empty() ||
        pathSegments[static_cast<std::size_t>(PATH_PARAM::TABLE_NAME)].empty()) {
        ZLOGE("Invalid uri ! uri: %{public}s", URIUtils::Anonymous(providerInfo_.uri).c_str());
        return false;
    }
    providerInfo_.bundleName = pathSegments[static_cast<std::size_t>(PATH_PARAM::BUNDLE_NAME)];
    providerInfo_.moduleName = pathSegments[static_cast<std::size_t>(PATH_PARAM::MODULE_NAME)];
    providerInfo_.storeName = pathSegments[static_cast<std::size_t>(PATH_PARAM::STORE_NAME)];
    providerInfo_.tableName = pathSegments[static_cast<std::size_t>(PATH_PARAM::TABLE_NAME)];
    return true;
}

std::pair<int, DataProviderConfig::ProviderInfo> DataProviderConfig::GetProviderInfo()
{
    auto ret = GetFromProxyData();
    if (ret == E_OK) {
        return std::make_pair(ret, providerInfo_);
    }
    ret = GetFromExtension();
    if (ret != E_OK) {
        ZLOGE("Get providerInfo failed! ret: %{public}d, uri: %{public}s",
            ret, URIUtils::Anonymous(providerInfo_.uri).c_str());
    }
    return std::make_pair(ret, providerInfo_);
}
} // namespace OHOS::DataShare
