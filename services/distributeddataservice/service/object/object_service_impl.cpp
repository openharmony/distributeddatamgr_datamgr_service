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

#define LOG_TAG "ObjectServiceImpl"

#include "object_service_impl.h"

#include <ipc_skeleton.h>

#include "account/account_delegate.h"
#include "checker/checker_manager.h"
#include "log_print.h"
#include "permission/permission_validator.h"
#include "communication_provider.h"
#include "bootstrap.h"
#include "directory_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/meta_data_manager.h"

namespace OHOS::DistributedObject {
int32_t ObjectServiceImpl::ObjectStoreSave(const std::string &bundleName, const std::string &sessionId,
    const std::vector<std::string> &deviceList, const std::map<std::string, std::vector<uint8_t>> &data,
    sptr<IObjectSaveCallback> callback)
{
    ZLOGI("begin.");
    auto uid = IPCSkeleton::GetCallingUid();

    DistributedData::CheckerManager::StoreInfo storeInfo;
    storeInfo.uid = uid;
    storeInfo.tokenId = IPCSkeleton::GetCallingTokenID();
    storeInfo.bundleName = bundleName;
    storeInfo.storeId = sessionId;
    std::string appId = DistributedData::CheckerManager::GetInstance().GetAppId(storeInfo);
    if (appId.empty()) {
        ZLOGE("object bundleName wrong");
        return PERMISSION_DENIED;
    }
    if (!PermissionValidator::GetInstance().CheckSyncPermission(storeInfo.tokenId)) {
        ZLOGE("object save permission denied");
        return PERMISSION_DENIED;
    }
    int32_t status = ObjectStoreManager::GetInstance()->Save(appId, sessionId, data, deviceList, callback);
    if (status != SUCCESS) {
        ZLOGE("save fail %{public}d", status);
    }
    return status;
}

void ObjectServiceImpl::Initialize()
{
    ZLOGI("Initialize");
    auto localDeviceId = AppDistributedKv::CommunicationProvider::GetInstance().GetLocalDevice().uuid;
    if (localDeviceId.empty()) {
        ZLOGE("failed to get local device id");
        return;
    }
    auto uid = IPCSkeleton::GetCallingUid();
    const std::string accountId = AccountDelegate::GetInstance()->GetCurrentAccountId();
    const std::string userId = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(uid);
    DistributedData::StoreMetaData saveMeta;
    saveMeta.appType = "default";
    saveMeta.deviceId = localDeviceId;
    saveMeta.storeId = DistributedObject::ObjectCommon::OBJECTSTORE_DB_STOREID;
    saveMeta.isAutoSync = false;
    saveMeta.isBackup = false;
    saveMeta.isEncrypt = false;
    saveMeta.bundleName =  DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    saveMeta.appId =  DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    saveMeta.user = userId;
    saveMeta.account = accountId;
    saveMeta.tokenId = IPCSkeleton::GetCallingTokenID();
    saveMeta.securityLevel = SecurityLevel::S1;
    saveMeta.area = 1;
    saveMeta.dataDir = DistributedData::DirectoryManager::GetInstance().GetStorePath(saveMeta);
    saveMeta.storeType = KvStoreType::SINGLE_VERSION;
    ObjectStoreManager::GetInstance()->SetData(saveMeta.dataDir, userId);
    DistributedData::StoreMetaData oldMeta;
    auto saved = DistributedData::MetaDataManager::GetInstance().SaveMeta(saveMeta.GetKey(), saveMeta);
    if (!saved) {
        ZLOGE("SaveMeta failed");
    }
    ZLOGI("SaveMeta success appId %{public}s, storeId %{public}s", saveMeta.appId.c_str(), saveMeta.storeId.c_str());
    return;
}

int32_t ObjectServiceImpl::ObjectStoreRevokeSave(
    const std::string &bundleName, const std::string &sessionId, sptr<IObjectRevokeSaveCallback> callback)
{
    ZLOGI("begin.");
    auto uid = IPCSkeleton::GetCallingUid();

    DistributedData::CheckerManager::StoreInfo storeInfo;
    storeInfo.uid = uid;
    storeInfo.tokenId = GetCallingTokenID();
    storeInfo.bundleName = bundleName;
    storeInfo.storeId = sessionId;
    std::string appId = DistributedData::CheckerManager::GetInstance().GetAppId(storeInfo);
    if (appId.empty()) {
        ZLOGE("object bundleName wrong");
        return PERMISSION_DENIED;
    }
    if (!PermissionValidator::GetInstance().CheckSyncPermission(storeInfo.tokenId)) {
        ZLOGE("object revoke save permission denied");
        return PERMISSION_DENIED;
    }
    int32_t status = ObjectStoreManager::GetInstance()->RevokeSave(appId, sessionId, callback);
    if (status != SUCCESS) {
        ZLOGE("revoke save fail %{public}d", status);
    }
    return SUCCESS;
}

int32_t ObjectServiceImpl::ObjectStoreRetrieve(
    const std::string &bundleName, const std::string &sessionId, sptr<IObjectRetrieveCallback> callback)
{
    ZLOGI("begin.");
    auto uid = IPCSkeleton::GetCallingUid();
    DistributedData::CheckerManager::StoreInfo storeInfo;
    storeInfo.uid = uid;
    storeInfo.tokenId = GetCallingTokenID();
    storeInfo.bundleName = bundleName;
    storeInfo.storeId = sessionId;
    std::string appId = DistributedData::CheckerManager::GetInstance().GetAppId(storeInfo);
    if (appId.empty()) {
        ZLOGE("object bundleName wrong");
        return PERMISSION_DENIED;
    }
    if (!PermissionValidator::GetInstance().CheckSyncPermission(storeInfo.tokenId)) {
        ZLOGE("object retrieve permission denied");
        return PERMISSION_DENIED;
    }
    int32_t status = ObjectStoreManager::GetInstance()->Retrieve(appId, sessionId, callback);
    if (status != SUCCESS) {
        ZLOGE("retrieve fail %{public}d", status);
    }
    return SUCCESS;
}

void ObjectServiceImpl::Clear()
{
    ZLOGI("begin.");
    int32_t status = ObjectStoreManager::GetInstance()->Clear();
    if (status != SUCCESS) {
        ZLOGE("save fail %{public}d", status);
    }
    return;
}

ObjectServiceImpl::ObjectServiceImpl()
{
}
} // namespace OHOS::DistributedObject
