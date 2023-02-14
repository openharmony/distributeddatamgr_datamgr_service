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
#define LOG_TAG "PermissionProxy"
#include "permission_proxy.h"

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "bundle_info.h"
#include "bundle_mgr_proxy.h"
#include "device_manager_adapter.h"
#include "log_print.h"
#include "metadata/appid_meta_data.h"
#include "metadata/meta_data_manager.h"

namespace OHOS::DataShare {
BundleMgrProxy PermissionProxy::bmsProxy_;
bool PermissionProxy::GetBundleInfo(const std::string &bundleName, uint32_t tokenId,
    AppExecFwk::BundleInfo &bundleInfo)
{
    if (!bmsProxy_.GetBundleInfoFromBMS(bundleName, tokenId, bundleInfo)) {
        ZLOGE("GetBundleInfoFromBMS failed!");
        return false;
    }
    return true;
}

bool PermissionProxy::QueryWritePermission(uint32_t tokenId, std::string &permission,
    const AppExecFwk::BundleInfo &bundleInfo)
{
    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type == AppExecFwk::ExtensionAbilityType::DATASHARE) {
            permission = item.writePermission;
            if (permission.empty()) {
                ZLOGW("WritePermission is empty!BundleName is %{public}s,tokenId is %{public}u",
                    bundleInfo.name.c_str(), tokenId);
                return true;
            }
            int status = Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenId, permission);
            if (status != Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
                ZLOGE("Verify write permission denied!");
                return false;
            }
            return true;
        }
    }
    return false;
}

bool PermissionProxy::QueryReadPermission(uint32_t tokenId, std::string &permission,
    const AppExecFwk::BundleInfo &bundleInfo)
{
    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type == AppExecFwk::ExtensionAbilityType::DATASHARE) {
            if (item.readPermission.empty()) {
                ZLOGW("ReadPermission is empty!BundleName is %{public}s,tokenId is %{public}u",
                    bundleInfo.name.c_str(), tokenId);
                return true;
            }
            int status = Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenId, permission);
            if (status != Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
                ZLOGE("Verify Read permission denied!");
                return false;
            }
            return true;
        }
    }
    return false;
}

std::string PermissionProxy::GetTableNameByCrossUserMode(const ProfileInfo &profileInfo,
    int32_t userId, bool isSingleApp, const UriInfo &uriInfo)
{
    std::string tableName = uriInfo.tableName;
    if (!isSingleApp) {
        return tableName;
    }

    AccessSystemMode crossUserMode = GetCrossUserMode(profileInfo, uriInfo);
    if (crossUserMode == AccessSystemMode::UNIQUE) {
        return tableName.append("_").append(std::to_string(userId));
    }
    if (crossUserMode != AccessSystemMode::SHARED) {
        ZLOGE("The crossUserMode is not right. crossUserMode is %{public}d", crossUserMode);
    }
    return tableName;
}

PermissionProxy::AccessSystemMode PermissionProxy::GetCrossUserMode(const ProfileInfo &profileInfo,
    const UriInfo &uriInfo)
{
    AccessSystemMode crossUserMode = AccessSystemMode::UNDEFINED;
    bool isStoreConfig  = false;
    for (auto &item : profileInfo.tableConfig) {
        UriInfo temp;
        AccessSystemMode curUserMode = static_cast<AccessSystemMode>(item.crossUserMode);
        if (curUserMode != AccessSystemMode::UNDEFINED && URIUtils::GetInfoFromURI(item.uri, temp)
            && temp.storeName == uriInfo.storeName && temp.tableName == uriInfo.tableName) {
            crossUserMode = curUserMode;
            return crossUserMode;
        }
        if (curUserMode != AccessSystemMode::UNDEFINED && URIUtils::GetInfoFromURI(item.uri, temp, true)
            && temp.tableName.empty() && temp.storeName == uriInfo.storeName) {
            crossUserMode = curUserMode;
            isStoreConfig = true;
            continue;
        }
        if (curUserMode != AccessSystemMode::UNDEFINED && item.uri == "*" && !isStoreConfig) {
            crossUserMode = curUserMode;
        }
    }
    return crossUserMode;
}

void PermissionProxy::FillData(DistributedData::StoreMetaData &meta, int32_t userId)
{
    meta.deviceId = DistributedData::DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
    meta.user = std::to_string(userId);
}

bool PermissionProxy::QueryMetaData(const std::string &bundleName, const std::string &storeName,
    DistributedData::StoreMetaData &metaData, int32_t userId)
{
    DistributedData::StoreMetaData meta;
    FillData(meta, userId);
    meta.bundleName = bundleName;
    meta.storeId = storeName;
    bool isCreated = DistributedData::MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), metaData);
    if (!isCreated) {
        ZLOGE("Interface token is not equal");
        return false;
    }
    return true;
}
} // namespace OHOS::DataShare