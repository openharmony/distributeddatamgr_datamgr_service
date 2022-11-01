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
#include "bundlemgr/bundle_mgr_proxy.h"
#include "communication_provider.h"
#include "if_system_ability_manager.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "metadata/appid_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "system_ability_definition.h"

namespace OHOS::DataShare {
const std::pair<std::string, std::string> ALLOW_CROSS_USER = std::pair<std::string, std::string>("com.ohos.settingsdatas", "settingsdatas");
static sptr<AppExecFwk::BundleMgrProxy> GetBundleMgrProxy()
{
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        ZLOGE("Failed to get system ability mgr.");
        return nullptr;
    }

    sptr<IRemoteObject> remoteObject = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (remoteObject == nullptr) {
        ZLOGE("Failed to get bundle manager proxy.");
        return nullptr;
    }

    ZLOGD("Get bundle manager proxy success.");
    return iface_cast<AppExecFwk::BundleMgrProxy>(remoteObject);
}

bool GetBundleInfoFromBMS(const std::string &bundleName, uint32_t tokenId,
    AppExecFwk::BundleInfo &bundleInfo)
{
    int32_t userId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(tokenId);
    auto bmsClient = GetBundleMgrProxy();
    if (!bmsClient) {
        ZLOGE("GetBundleMgrProxy is nullptr!");
        return false;
    }
    bool ret = bmsClient->GetBundleInfo(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_WITH_EXTENSION_INFO, bundleInfo,
        userId);
    if (!ret) {
        ZLOGE("GetBundleInfo failed!");
        return false;
    }
    return true;
}

bool PermissionProxy::QueryWritePermission(const std::string &bundleName, uint32_t tokenId, std::string &permission)
{
    AppExecFwk::BundleInfo bundleInfo;
    if (!GetBundleInfoFromBMS(bundleName, tokenId, bundleInfo)) {
        ZLOGE("GetBundleInfoFromBMS failed!");
        return false;
    }
    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type == AppExecFwk::ExtensionAbilityType::DATASHARE) {
            permission = item.writePermission;
            if (permission.empty()) {
                ZLOGW("WritePermission is empty!BundleName is %{pravite}s,tokenId is %{pravite}s", bundleName, tokenId);
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

bool PermissionProxy::QueryReadPermission(const std::string &bundleName, uint32_t tokenId, std::string &permission)
{
    AppExecFwk::BundleInfo bundleInfo;
    if (!GetBundleInfoFromBMS(bundleName, tokenId, bundleInfo)) {
        ZLOGE("GetBundleInfoFromBMS failed!");
        return false;
    }
    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type == AppExecFwk::ExtensionAbilityType::DATASHARE) {
            if (item.readPermission.empty()) {
                ZLOGW("ReadPermission is empty!BundleName is %{pravite}s,tokenId is %{pravite}s", bundleName, tokenId);
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

void PermissionProxy::FillData(DistributedData::StoreMetaData &meta)
{
    meta.deviceId = AppDistributedKv::CommunicationProvider::GetInstance().GetLocalDevice().uuid;
    meta.user = DistributedKv::AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(IPCSkeleton::GetCallingUid());
}

bool PermissionProxy::QueryMetaData(const std::string &bundleName, const std::string &moduleName,
    const std::string &storeName, DistributedData::StoreMetaData &metaData)
{
    DistributedData::StoreMetaData meta;
    FillData(meta);
    meta.bundleName = bundleName;
    meta.storeId = storeName;
    if (IsAllowCrossToU0(bundleName, storeName)) {
        ZLOGD("This hap is allowed to access across user sessions");
        meta.user = "0";
    }
    ZLOGD("This hap is not allowed to access across user sessions");
    bool isCreated = DistributedData::MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), metaData);
    if (!isCreated) {
        ZLOGE("Interface token is not equal");
        return false;
    }
    return true;
}
inline bool PermissionProxy::IsAllowCrossToU0(const std::string &bundleName, const std::string &storeName)
{
    return bundleName == ALLOW_CROSS_USER.first && storeName == ALLOW_CROSS_USER.second;
}
} // namespace OHOS::DataShare