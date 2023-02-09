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
bool PermissionProxy::QueryWritePermission(const std::string &bundleName, uint32_t tokenId,
    std::string &permission, AppExecFwk::BundleInfo &bundleInfo)
{
    if (!bmsProxy_.GetBundleInfoFromBMS(bundleName, tokenId, bundleInfo)) {
        ZLOGE("GetBundleInfoFromBMS failed!");
        return false;
    }
    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type == AppExecFwk::ExtensionAbilityType::DATASHARE) {
            permission = item.writePermission;
            if (permission.empty()) {
                ZLOGW("WritePermission is empty!BundleName is %{public}s,tokenId is %{public}u", bundleName.c_str(),
                    tokenId);
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

bool PermissionProxy::QueryReadPermission(const std::string &bundleName, uint32_t tokenId,
    std::string &permission, AppExecFwk::BundleInfo &bundleInfo)
{
    if (!bmsProxy_.GetBundleInfoFromBMS(bundleName, tokenId, bundleInfo)) {
        ZLOGE("GetBundleInfoFromBMS failed!");
        return false;
    }
    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type == AppExecFwk::ExtensionAbilityType::DATASHARE) {
            if (item.readPermission.empty()) {
                ZLOGW("ReadPermission is empty!BundleName is %{public}s,tokenId is %{public}u", bundleName.c_str(),
                    tokenId);
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

bool PermissionProxy::IsCrossUserMode(const ProfileInfo &profileInfo, const AppExecFwk::BundleInfo &bundleInfo,
    int32_t userId, bool isSingleApp, UriInfo &uriInfo)
{
    if (!isSingleApp) {
        return true;
    }
    int crossUserMode = GetCrossUserMode(profileInfo, uriInfo);
    if (crossUserMode != USERMODE_SHARED && crossUserMode != USERMODE_UNIQUE) {
        ZLOGE("The crossUserMode is not right.");
        return false;
    }
    if (crossUserMode == USERMODE_UNIQUE) {
        uriInfo.tableName.append("_").append(std::to_string(userId));
    }
    return true;
}

int PermissionProxy::GetCrossUserMode(const ProfileInfo &profileInfo, const UriInfo &uriInfo)
{
    int crossUserMode = USERMODE_UNDEFINED;
    bool getStoreConfig = false;
    for (auto &item : profileInfo.tableConfig) {
        UriInfo temp;
        if (item.crossUserMode != USERMODE_UNDEFINED && URIUtils::GetInfoFromURI(item.uri, temp)
            && temp.storeName == uriInfo.storeName && temp.tableName == uriInfo.tableName) {
            crossUserMode = item.crossUserMode;
            return crossUserMode;
        }
        if (item.crossUserMode != USERMODE_UNDEFINED && URIUtils::GetInfoFromURI(item.uri, temp, true)
            && temp.tableName.empty() && temp.storeName == uriInfo.storeName) {
            crossUserMode = item.crossUserMode;
            getStoreConfig = true;
            continue;
        }
        if (item.crossUserMode != USERMODE_UNDEFINED && item.uri == "*" && !getStoreConfig) {
            crossUserMode = item.crossUserMode;
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
    DistributedData::StoreMetaData &metaData, int32_t userId, bool isSingleApp)
{
    DistributedData::StoreMetaData meta;
    FillData(meta, userId);
    meta.bundleName = bundleName;
    meta.storeId = storeName;
    if (isSingleApp) {
        ZLOGD("This hap is allowed to access across user sessions");
        meta.user = "0";
    }
    bool isCreated = DistributedData::MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), metaData);
    if (!isCreated) {
        ZLOGE("Interface token is not equal");
        return false;
    }
    return true;
}
} // namespace OHOS::DataShare