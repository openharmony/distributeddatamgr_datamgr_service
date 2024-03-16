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
#define LOG_TAG "DataShareProvider"

#include "data_share_provider_data.h"

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "datashare_errno.h"
#include "hap_token_info.h"
#include "log_print.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
using namespace OHOS::DistributedData;
int DataShareProvider::GetProviderInfoFromProxyData(ProviderInfo &providerInfo)
{
    Uri uriTemp(providerInfo.uri);
    if (uriTemp.GetAuthority().empty()) {
        ZLOGE("proxy authority empty! uri: %{public}s", Anonymous::Change(providerInfo.uri).c_str());
        return E_URI_NOT_EXIST;
    }
    providerInfo.bundleName = uriTemp.GetAuthority();
    BundleInfo bundleInfo;
    if (!BundleMgrProxy::GetInstance()->GetBundleInfoFromBMS(
        providerInfo.bundleName, providerInfo.currentUserId, bundleInfo)) {
        ZLOGE("BundleInfo failed! bundleName: %{public}s", providerInfo.bundleName.c_str());
        return E_BUNDLE_NAME_NOT_EXIST;
    }
    for (auto &hapModuleInfo : bundleInfo.hapModuleInfos) {
        for (auto &data : hapModuleInfo.proxyDatas) {
            if (data.uri != providerInfo.uri) {
                continue;
            }
            providerInfo.uri = std::move(data.uri);
            providerInfo.readPermission = std::move(data.requiredReadPermission);
            providerInfo.writePermission = std::move(data.requiredWritePermission);
            providerInfo.moduleName = std::move(hapModuleInfo.moduleName);
            providerInfo.hapPath = std::move(hapModuleInfo.hapPath);
            providerInfo.resourcePath = std::move(hapModuleInfo.resourcePath);
            break;
        }
    }
    return E_OK;
}

int DataShareProvider::GetProviderInfoFromExtension(ProviderInfo &providerInfo)
{
    if (!GetProviderInfoFromUriPath(providerInfo)) {
        ZLOGE("Uri Path failed! uri:%{public}s", Anonymous::Change(providerInfo.uri).c_str());
        return E_URI_NOT_EXIST;
    }
    BundleInfo bundleInfo;
    if (!BundleMgrProxy::GetInstance()->GetBundleInfoFromBMS(
        providerInfo.bundleName, providerInfo.currentUserId, bundleInfo)) {
        ZLOGE("BundleInfo failed! bundleName: %{public}s", providerInfo.bundleName.c_str());
        return E_BUNDLE_NAME_NOT_EXIST;
    }
    ExtensionAbility firstExtension;
    bool isFirstFind = true;
    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type == AppExecFwk::ExtensionAbilityType::DATASHARE) {
            if (isFirstFind) {
                firstExtension = item;
                isFirstFind = false;
            }
            Uri uriTemp(item.uri);
            if (providerInfo.uri.find(uriTemp.GetAuthority()) == std::string::npos) {
                continue;
            }
            firstExtension = item;
            break;
        }
    }
    providerInfo.readPermission = std::move(firstExtension.readPermission);
    providerInfo.writePermission = std::move(firstExtension.writePermission);
    providerInfo.hapPath = std::move(firstExtension.hapPath);
    providerInfo.resourcePath = std::move(firstExtension.resourcePath);
    return E_OK;
}

bool DataShareProvider::GetProviderInfoFromUriPath(ProviderInfo &providerInfo)
{
    Uri uriTemp(providerInfo.uri);
    std::vector<std::string> pathSegments;
    uriTemp.GetPathSegments(pathSegments);
    if (pathSegments.size() < PARAM_SIZE || pathSegments[BUNDLE_NAME].empty() || 
        pathSegments[MODULE_NAME].empty() || pathSegments[STORE_NAME].empty() || 
        pathSegments[TABLE_NAME].empty()) {
        ZLOGE("Invalid uri ! uri: %{public}s", Anonymous::Change(providerInfo.uri).c_str());
        return false;
    }
    providerInfo.bundleName = pathSegments[BUNDLE_NAME];
    providerInfo.moduleName = pathSegments[MODULE_NAME];
    providerInfo.storeName = pathSegments[STORE_NAME];
    providerInfo.tableName = pathSegments[TABLE_NAME];
    return true;
}

int DataShareProvider::GetProviderInfo(ProviderInfo &providerInfo)
{
    if (providerInfo.isProxyUri) {
        auto ret = GetProviderInfoFromProxyData(providerInfo);
        if (ret != E_OK) {
            ZLOGE("ProxyData failed! ret: %{public}d, uri: %{public}s",
                ret, Anonymous::Change(providerInfo.uri).c_str());
            return ret;
        }
    } else {
        auto ret = GetProviderInfoFromExtension(providerInfo);
        if (ret != E_OK) {
            ZLOGE("Extension failed! ret: %{public}d, uri: %{public}s",
                ret, Anonymous::Change(providerInfo.uri).c_str());
            return ret;
        }
    }
    return E_OK;
}
} // namespace OHOS::DataShare
