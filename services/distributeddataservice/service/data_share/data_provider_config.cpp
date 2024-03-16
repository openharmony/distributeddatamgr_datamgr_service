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

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "datashare_errno.h"
#include "hap_token_info.h"
#include "log_print.h"
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
    Uri uriTemp(providerInfo_.uri);
    if (uriTemp.GetAuthority().empty()) {
        ZLOGE("proxy authority empty! uri: %{public}s", Anonymous::Change(providerInfo_.uri).c_str());
        return E_URI_NOT_EXIST;
    }
    providerInfo_.bundleName = uriTemp.GetAuthority();
    BundleInfo bundleInfo;
    if (!BundleMgrProxy::GetInstance()->GetBundleInfoFromBMS(
        providerInfo_.bundleName, providerInfo_.currentUserId, bundleInfo)) {
        ZLOGE("BundleInfo failed! bundleName: %{public}s", providerInfo_.bundleName.c_str());
        return E_BUNDLE_NAME_NOT_EXIST;
    }
    for (auto &hapModuleInfo : bundleInfo.hapModuleInfos) {
        for (auto &data : hapModuleInfo.proxyDatas) {
            if (data.uri != providerInfo_.uri) {
                continue;
            }
            providerInfo_.uri = std::move(data.uri);
            providerInfo_.readPermission = std::move(data.requiredReadPermission);
            providerInfo_.writePermission = std::move(data.requiredWritePermission);
            providerInfo_.moduleName = std::move(hapModuleInfo.moduleName);
            return E_OK;
        }
    }
    return E_URI_NOT_EXIST;
}

int DataProviderConfig::GetFromExtension()
{
    if (!GetFromUriPath()) {
        ZLOGE("Uri Path failed! uri:%{public}s", Anonymous::Change(providerInfo_.uri).c_str());
        return E_URI_NOT_EXIST;
    }
    BundleInfo bundleInfo;
    if (!BundleMgrProxy::GetInstance()->GetBundleInfoFromBMS(
        providerInfo_.bundleName, providerInfo_.currentUserId, bundleInfo)) {
        ZLOGE("BundleInfo failed! bundleName: %{public}s", providerInfo_.bundleName.c_str());
        return E_BUNDLE_NAME_NOT_EXIST;
    }
    if (bundleInfo.extensionInfos.size() == 0) {
        ZLOGE("extension empty! uri:%{public}s", Anonymous::Change(providerInfo_.uri).c_str());
        return E_URI_NOT_EXIST;
    }
    ExtensionAbility firstExtension = bundleInfo.extensionInfos.at(0);
    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type != AppExecFwk::ExtensionAbilityType::DATASHARE) {
            continue;
        }
        Uri uriTemp(item.uri);
        if (providerInfo_.uri.find(uriTemp.GetAuthority()) == std::string::npos) {
            continue;
        }
        firstExtension = item;
        break;
    }
    providerInfo_.readPermission = std::move(firstExtension.readPermission);
    providerInfo_.writePermission = std::move(firstExtension.writePermission);
    return E_OK;
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
        ZLOGE("Invalid uri ! uri: %{public}s", Anonymous::Change(providerInfo_.uri).c_str());
        return false;
    }
    providerInfo_.bundleName = pathSegments[static_cast<int32_t>(PATH_PARAM::BUNDLE_NAME)];
    providerInfo_.moduleName = pathSegments[static_cast<int32_t>(PATH_PARAM::MODULE_NAME)];
    providerInfo_.storeName = pathSegments[static_cast<int32_t>(PATH_PARAM::STORE_NAME)];
    providerInfo_.tableName = pathSegments[static_cast<int32_t>(PATH_PARAM::TABLE_NAME)];
    return true;
}

std::pair<int, DataProviderConfig::ProviderInfo> DataProviderConfig::GetProviderInfo(bool isProxyData)
{
    auto func = isProxyData ?
        &DataProviderConfig::GetFromProxyData : &DataProviderConfig::GetFromExtension;
    auto ret = (this->*func)();
    if (ret != E_OK) {
        ZLOGE("failed! isProxyData:%{public}d, ret: %{public}d, uri: %{public}s",
            isProxyData, ret, Anonymous::Change(providerInfo_.uri).c_str());
    }
    return std::make_pair(ret, providerInfo_);
}
} // namespace OHOS::DataShare
