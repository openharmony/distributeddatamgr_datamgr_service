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
#include <vector>
#define LOG_TAG "DataProviderConfig"

#include "data_provider_config.h"

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "data_share_profile_config.h"
#include "datashare_errno.h"
#include "hap_token_info.h"
#include "log_print.h"
#include "uri_utils.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
using namespace OHOS::DistributedData;
DataProviderConfig::DataProviderConfig(const std::string &uri, uint32_t callerTokenId)
{
    providerInfo_.uri = uri;
    providerInfo_.currentUserId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(callerTokenId);
}

int DataProviderConfig::GetFromProxyData()
{
    auto uriConfig = URIUtils::GetUriConfig(providerInfo_.uri);
    if (uriConfig.authority_.empty()) {
        ZLOGE("uri authority empty! uri: %{public}s", Anonymous::Anonymity(providerInfo_.uri).c_str());
        return E_URI_NOT_EXIST;
    }
    providerInfo_.bundleName = uriConfig.authority_;
    BundleInfo bundleInfo;
    if (!BundleMgrProxy::GetInstance()->GetBundleInfoFromBMS(
        providerInfo_.bundleName, providerInfo_.currentUserId, bundleInfo)) {
        ZLOGE("BundleInfo failed! bundleName:%{public}s, userId:%{public}d, uri:%{public}s",
            providerInfo_.bundleName.c_str(), providerInfo_.currentUserId, Anonymous::Anonymity(providerInfo_.uri).c_str());
        return E_BUNDLE_NAME_NOT_EXIST;
    }
    providerInfo_.singleton = bundleInfo.singleton;
    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type != AppExecFwk::ExtensionAbilityType::DATASHARE) {
            continue;
        }
        providerInfo_.haveDataShareExtension = true;
        break;
    }
    for (auto &hapModuleInfo : bundleInfo.hapModuleInfos) {
        for (auto &data : hapModuleInfo.proxyDatas) {
            if (data.uri != uriConfig.formatUri_) {
                continue;
            }
            providerInfo_.readPermission = std::move(data.requiredReadPermission);
            providerInfo_.writePermission = std::move(data.requiredWritePermission);
            std::string resourcePath = !hapModuleInfo.hapPath.empty() ? hapModuleInfo.hapPath : hapModuleInfo.resourcePath;
            auto [ret, properties] = DataShareProfileConfig::GetDataProperties(resourcePath,
                std::vector<AppExecFwk::Metadata>{data.metadata}, !hapModuleInfo.hapPath.empty(), true);
            if (!ret) {
                ZLOGE("ProfileInfo from proxyData error. uri: %{public}s", Anonymous::Anonymity(providerInfo_.uri).c_str());
                return true;
            }
            GetFromProperties(properties, hapModuleInfo.moduleName, true, false);
            return E_OK;
        }
    }
    return E_URI_NOT_EXIST;
}

int DataProviderConfig::GetFromProperties(const ProfileInfo &profileInfo,
    const std::string &moduleName, bool isProxyData, bool singleton)
{
    if (isProxyData) {
        if (profileInfo.scope == ProfileInfo::MODULE_SCOPE) {
            // module scope
            providerInfo_.moduleName = moduleName;
        }
        providerInfo_.storeName = profileInfo.storeName;
        providerInfo_.tableName = profileInfo.tableName;
        providerInfo_.type = profileInfo.type;
        return E_OK;
    }
    std::string storeUri = URIUtils::DATA_SHARE_SCHEMA + providerInfo_.bundleName + URIUtils::URI_SEPARATOR +
            moduleName + URIUtils::URI_SEPARATOR + providerInfo_.storeName;
    std::string tableUri = storeUri + URIUtils::URI_SEPARATOR + providerInfo_.tableName;
    DataShareProfileConfig profileConfig;
    providerInfo_.accessCrossMode = profileConfig.GetFromTableConfigs(profileInfo, tableUri, storeUri);
    if (singleton && providerInfo_.accessCrossMode == AccessCrossMode::USER_UNDEFINED) {
        ZLOGE("single app must config user cross mode,bundleName:%{public}s, uri:%{public}s",
            providerInfo_.bundleName.c_str(), Anonymous::Anonymity(providerInfo_.uri).c_str());
        return E_ERROR;
    }
    if (singleton && providerInfo_.accessCrossMode == AccessCrossMode::USER_SINGLE) {
        providerInfo_.tableName.append("_").append(std::to_string(providerInfo_.currentUserId));
    }
    return E_OK;
}

int DataProviderConfig::GetFromExtension()
{
    if (!GetFromUriPath()) {
        ZLOGE("Uri Path failed! uri:%{public}s", Anonymous::Anonymity(providerInfo_.uri).c_str());
        return E_URI_NOT_EXIST;
    }
    BundleInfo bundleInfo;
    if (!BundleMgrProxy::GetInstance()->GetBundleInfoFromBMS(
        providerInfo_.bundleName, providerInfo_.currentUserId, bundleInfo)) {
        ZLOGE("BundleInfo failed! bundleName: %{public}s", providerInfo_.bundleName.c_str());
        return E_BUNDLE_NAME_NOT_EXIST;
    }
    providerInfo_.singleton = bundleInfo.singleton;
    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type != AppExecFwk::ExtensionAbilityType::DATASHARE) {
            continue;
        }
        providerInfo_.haveDataShareExtension = true;
        providerInfo_.readPermission = std::move(item.readPermission);
        providerInfo_.writePermission = std::move(item.writePermission);
        std::string resourcePath = !item.hapPath.empty() ? item.hapPath : item.resourcePath;
        auto [ret, properties] = DataShareProfileConfig::GetDataProperties(resourcePath,
            item.metadata, !item.hapPath.empty(), false);
        if (!ret) {
            ZLOGE("ProfileInfo from extension error.uri: %{public}s", Anonymous::Anonymity(providerInfo_.uri).c_str());
            return E_OK;
        }
        return GetFromProperties(properties, providerInfo_.moduleName, false, bundleInfo.singleton);
    }
    return E_URI_NOT_EXIST;
}

bool DataProviderConfig::GetFromUriPath()
{
    Uri uriTemp(providerInfo_.uri);
    std::vector<std::string> pathSegments;
    uriTemp.GetPathSegments(pathSegments);
    if (pathSegments.size() < static_cast<int32_t>(PATH_PARAM::PARAM_SIZE) ||
        pathSegments[static_cast<int32_t>(PATH_PARAM::BUNDLE_NAME)].empty() ||
        pathSegments[static_cast<int32_t>(PATH_PARAM::MODULE_NAME)].empty() ||
        pathSegments[static_cast<int32_t>(PATH_PARAM::STORE_NAME)].empty() ||
        pathSegments[static_cast<int32_t>(PATH_PARAM::TABLE_NAME)].empty()) {
        ZLOGE("Invalid uri ! uri: %{public}s", Anonymous::Anonymity(providerInfo_.uri).c_str());
        return false;
    }
    providerInfo_.bundleName = pathSegments[static_cast<int32_t>(PATH_PARAM::BUNDLE_NAME)];
    providerInfo_.moduleName = pathSegments[static_cast<int32_t>(PATH_PARAM::MODULE_NAME)];
    providerInfo_.storeName = pathSegments[static_cast<int32_t>(PATH_PARAM::STORE_NAME)];
    providerInfo_.tableName = pathSegments[static_cast<int32_t>(PATH_PARAM::TABLE_NAME)];
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
            ret, Anonymous::Anonymity(providerInfo_.uri).c_str());
    }
    return std::make_pair(ret, providerInfo_);
}
} // namespace OHOS::DataShare
