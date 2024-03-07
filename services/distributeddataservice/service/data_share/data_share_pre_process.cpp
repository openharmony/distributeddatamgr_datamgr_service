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
#define LOG_TAG "DataSharePreProcess"

#include "data_share_pre_process.h"

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "datashare_errno.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "uri_utils.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
constexpr const char PERMISSION_REJECT[] = "reject";
constexpr const char DATA_SHARE_EXTENSION_URI_PREFIX[] = "datashare://";
void DataSharePreProcess::GetCallerInfoFromUri(uint32_t &callerTokenId, std::shared_ptr<CalledInfo> calledInfo)
{
    calledInfo->currentUserId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(callerTokenId);
    if (calledInfo->currentUserId == 0) {
        URIUtils::GetInfoFromProxyURI(
            calledInfo->uri, calledInfo->currentUserId, callerTokenId, calledInfo->bundleName);
        URIUtils::FormatUri(calledInfo->uri);
    }
}

int DataSharePreProcess::GetProviderInfoFromProxyData(std::shared_ptr<CalledInfo> calledInfo,
    DataShareProvider &providerInfo)
{
    if (!URIUtils::GetBundleNameFromProxyURI(calledInfo->uri, calledInfo->bundleName)) {
        ZLOGE("Get bundleName from proxyURI failed! uri: %{public}s",
            DistributedData::Anonymous::Change(calledInfo->uri).c_str());
        return E_BUNDLE_NAME_NOT_EXIST;
    }
    BundleInfo bundleInfo;
    if (!BundleMgrProxy::GetInstance()->GetBundleInfoFromBMS(
        calledInfo->bundleName, calledInfo->currentUserId, bundleInfo)) {
        ZLOGE("Get bundleInfo from BMS failed! bundleName: %{public}s", calledInfo->bundleName.c_str());
        return E_BUNDLE_NAME_NOT_EXIST;
    }
    for (auto &hapModuleInfo : bundleInfo.hapModuleInfos) {
        for (auto &data : hapModuleInfo.proxyDatas) {
            if (data.uri != calledInfo->uri) {
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

int DataSharePreProcess::GetProviderInfoFromExtension(std::shared_ptr<CalledInfo> calledInfo,
    DataShareProvider &providerInfo)
{
    if (!GetCalledInfoFromUri(calledInfo)) {
        ZLOGE("Get calledInfo from uri failed! bundleName: %{public}s", calledInfo->bundleName.c_str());
        return E_URI_NOT_EXIST;
    }
    BundleInfo bundleInfo;
    if (!BundleMgrProxy::GetInstance()->GetBundleInfoFromBMS(
        calledInfo->bundleName, calledInfo->currentUserId, bundleInfo)) {
        ZLOGE("Get bundleInfo from BMS failed! bundleName: %{public}s", calledInfo->bundleName.c_str());
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
            auto extensionUri = item.uri;
            URIUtils::RemoveUriPrefix(DATA_SHARE_EXTENSION_URI_PREFIX, extensionUri);
            if (calledInfo->uri.find(extensionUri) == std::string::npos) {
                continue;
            }
            firstExtension = item;
            break;
        }
    }
    providerInfo.uri = std::move(firstExtension.uri);
    providerInfo.readPermission = std::move(firstExtension.readPermission);
    providerInfo.writePermission = std::move(firstExtension.writePermission);
    providerInfo.hapPath = std::move(firstExtension.hapPath);
    providerInfo.resourcePath = std::move(firstExtension.resourcePath);
    return E_OK;
}

bool DataSharePreProcess::GetCalledInfoFromUri(std::shared_ptr<CalledInfo> calledInfo)
{
    UriInfo uriInfo;
    if (!URIUtils::GetInfoFromURI(calledInfo->uri, uriInfo)) {
        return false;
    }
    calledInfo->bundleName = std::move(uriInfo.bundleName);
    calledInfo->moduleName = std::move(uriInfo.moduleName);
    calledInfo->storeName = std::move(uriInfo.storeName);
    calledInfo->tableName = std::move(uriInfo.tableName);
    return true;
}

int DataSharePreProcess::GetProviderInfoFromUri(std::shared_ptr<CalledInfo> calledInfo,
    DataShareProvider &providerInfo)
{
    if (URIUtils::IsDataProxyURI(calledInfo->uri)) {
        auto ret = GetProviderInfoFromProxyData(calledInfo, providerInfo);
        if (ret != E_OK) {
            ZLOGE("Load config from proxyData failed! ret: %{public}d, uri: %{public}s",
                ret, DistributedData::Anonymous::Change(calledInfo->uri).c_str());
            return ret;
        }
    } else {
        auto ret = GetProviderInfoFromExtension(calledInfo, providerInfo);
        if (ret != E_OK) {
            ZLOGE("Load config from extension failed! ret: %{public}d, uri: %{public}s",
                ret, DistributedData::Anonymous::Change(calledInfo->uri).c_str());
            return ret;
        }
    }
    return E_OK;
}

bool DataSharePreProcess::VerifyPermission(const std::string &permission, const uint32_t callerTokerId,
    std::shared_ptr<CalledInfo> calledInfo)
{
    if (permission == PERMISSION_REJECT) {
        ZLOGE("reject permission, uri: %{public}s", DistributedData::Anonymous::Change(calledInfo->uri).c_str());
        return false;
    }
    if (!permission.empty()) {
        int status =
            Security::AccessToken::AccessTokenKit::VerifyAccessToken(callerTokerId, permission);
        if (status != Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
            ZLOGE("Verify permission denied! callerTokenId:%{public}u, permission:%{public}s, uri: %{public}s",
                callerTokerId, permission.c_str(),
                DistributedData::Anonymous::Change(calledInfo->uri).c_str());
            return false;
        }
    }
    return true;
}

int DataSharePreProcess::RegisterObserverProcess(std::shared_ptr<CalledInfo> calledInfo)
{
    if (calledInfo->uri.empty()) {
        return E_URI_NOT_EXIST;
    }
    auto callerTokenId = IPCSkeleton::GetCallingTokenID();
    GetCallerInfoFromUri(callerTokenId, calledInfo);
    DataShareProvider providerInfo;
    auto ret = GetProviderInfoFromUri(calledInfo, providerInfo);
    if (ret != E_OK) {
        return ret;
    }
    std::string permission;
    if (URIUtils::IsDataProxyURI(calledInfo->uri)) {
        permission = providerInfo.readPermission.empty() ? PERMISSION_REJECT : providerInfo.readPermission;
    } else {
        permission = providerInfo.readPermission;
    }
    if (!VerifyPermission(permission, callerTokenId, calledInfo)) {
        return ERR_PERMISSION_DENIED;
    }
    return E_OK;
}
} // namespace OHOS::DataShare
