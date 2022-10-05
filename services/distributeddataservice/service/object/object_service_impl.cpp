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
#include "metadata/appid_meta_data.h"
#include "metadata/store_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "utils/anonymous.h"

namespace OHOS::DistributedObject {
using Commu = AppDistributedKv::CommunicationProvider;
using StoreMetaData = OHOS::DistributedData::StoreMetaData;
using FeatureSystem = OHOS::DistributedData::FeatureSystem;
__attribute__((used)) ObjectServiceImpl::Factory ObjectServiceImpl::factory_;
ObjectServiceImpl::Factory::Factory()
{
    FeatureSystem::GetInstance().RegisterCreator("data_object", []() { return std::make_shared<ObjectServiceImpl>(); });
}

ObjectServiceImpl::Factory::~Factory()
{
}

int32_t ObjectServiceImpl::ObjectStoreSave(const std::string &bundleName, const std::string &sessionId,
    const std::string &deviceId, const std::map<std::string, std::vector<uint8_t>> &data,
    sptr<IObjectSaveCallback> callback)
{
    ZLOGI("begin.");
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t status = IsBundleNameEqualTokenId(bundleName, sessionId, tokenId);
    if (status != OBJECT_SUCCESS) {
        return status;
    }
    if (!PermissionValidator::GetInstance().CheckSyncPermission(tokenId)) {
        ZLOGE("object save permission denied");
        return OBJECT_PERMISSION_DENIED;
    }
    status = ObjectStoreManager::GetInstance()->Save(bundleName, sessionId, data, deviceId, callback);
    if (status != OBJECT_SUCCESS) {
        ZLOGE("save fail %{public}d", status);
    }
    return status;
}

int32_t ObjectServiceImpl::OnInitialize()
{
    ZLOGI("Initialize");
    auto localDeviceId = AppDistributedKv::CommunicationProvider::GetInstance().GetLocalDevice().uuid;
    if (localDeviceId.empty()) {
        ZLOGE("failed to get local device id");
        return OBJECT_INNER_ERROR;
    }
    auto uid = IPCSkeleton::GetCallingUid();
    const std::string accountId = AccountDelegate::GetInstance()->GetCurrentAccountId();
    const std::string userId = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(uid);
    StoreMetaData saveMeta;
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
    saveMeta.uid = uid;
    saveMeta.storeType = KvStoreType::SINGLE_VERSION;
    saveMeta.dataDir = DistributedData::DirectoryManager::GetInstance().GetStorePath(saveMeta);
    ObjectStoreManager::GetInstance()->SetData(saveMeta.dataDir, userId);
    auto saved = DistributedData::MetaDataManager::GetInstance().SaveMeta(saveMeta.GetKey(), saveMeta);
    if (!saved) {
        ZLOGE("SaveMeta failed");
        return OBJECT_INNER_ERROR;
    }
    DistributedData::AppIDMetaData appIdMeta;
    appIdMeta.bundleName = saveMeta.bundleName;
    appIdMeta.appId = saveMeta.appId;
    saved = DistributedData::MetaDataManager::GetInstance().SaveMeta(appIdMeta.GetKey(), appIdMeta, true);
    if (!saved) {
        ZLOGE("Save appIdMeta failed");
    }
    ZLOGI("SaveMeta success appId %{public}s, storeId %{public}s", saveMeta.appId.c_str(), saveMeta.storeId.c_str());
    return OBJECT_SUCCESS;
}

int32_t ObjectServiceImpl::OnUserChange(uint32_t code, const std::string &user, const std::string &account)
{
    if (code == uint32_t(AccountStatus::DEVICE_ACCOUNT_SWITCHED)) {
        Clear();
    }
    return Feature::OnUserChange(code, user, account);
}

int32_t ObjectServiceImpl::ObjectStoreRevokeSave(
    const std::string &bundleName, const std::string &sessionId, sptr<IObjectRevokeSaveCallback> callback)
{
    ZLOGI("begin.");
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t status = IsBundleNameEqualTokenId(bundleName, sessionId, tokenId);
    if (status != OBJECT_SUCCESS) {
        return status;
    }
    if (!PermissionValidator::GetInstance().CheckSyncPermission(tokenId)) {
        ZLOGE("object revoke save permission denied");
        return OBJECT_PERMISSION_DENIED;
    }
    status = ObjectStoreManager::GetInstance()->RevokeSave(bundleName, sessionId, callback);
    if (status != OBJECT_SUCCESS) {
        ZLOGE("revoke save fail %{public}d", status);
    }
    return OBJECT_SUCCESS;
}

int32_t ObjectServiceImpl::ObjectStoreRetrieve(
    const std::string &bundleName, const std::string &sessionId, sptr<IObjectRetrieveCallback> callback)
{
    ZLOGI("begin.");
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t status = IsBundleNameEqualTokenId(bundleName, sessionId, tokenId);
    if (status != OBJECT_SUCCESS) {
        return status;
    }
    if (!PermissionValidator::GetInstance().CheckSyncPermission(tokenId)) {
        ZLOGE("object retrieve permission denied");
        return OBJECT_PERMISSION_DENIED;
    }
    status = ObjectStoreManager::GetInstance()->Retrieve(bundleName, sessionId, callback);
    if (status != OBJECT_SUCCESS) {
        ZLOGE("retrieve fail %{public}d", status);
    }
    return OBJECT_SUCCESS;
}

int32_t ObjectServiceImpl::RegisterDataObserver(
    const std::string &bundleName, const std::string &sessionId, sptr<IObjectChangeCallback> callback)
{
    ZLOGD("begin.");
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t status = IsBundleNameEqualTokenId(bundleName, sessionId, tokenId);
    if (status != OBJECT_SUCCESS) {
        return status;
    }
    auto pid = IPCSkeleton::GetCallingPid();
    ObjectStoreManager::GetInstance()->RegisterRemoteCallback(bundleName, sessionId, pid, tokenId, callback);
    return OBJECT_SUCCESS;
}

int32_t ObjectServiceImpl::UnregisterDataChangeObserver(const std::string &bundleName, const std::string &sessionId)
{
    ZLOGD("begin.");
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t status = IsBundleNameEqualTokenId(bundleName, sessionId, tokenId);
    if (status != OBJECT_SUCCESS) {
        return status;
    }
    auto pid = IPCSkeleton::GetCallingPid();
    ObjectStoreManager::GetInstance()->UnregisterRemoteCallback(bundleName, pid, tokenId, sessionId);
    return OBJECT_SUCCESS;
}

int32_t ObjectServiceImpl::IsBundleNameEqualTokenId(
    const std::string &bundleName, const std::string &sessionId, const uint32_t &tokenId)
{
    DistributedData::CheckerManager::StoreInfo storeInfo;
    storeInfo.uid = IPCSkeleton::GetCallingUid();
    storeInfo.tokenId = tokenId;
    storeInfo.bundleName = bundleName;
    storeInfo.storeId = sessionId;
    std::string appId = DistributedData::CheckerManager::GetInstance().GetAppId(storeInfo);
    if (appId.empty()) {
        ZLOGE("object bundleName wrong, bundleName = %{public}s, uid = %{public}d, tokenId = 0x%{public}x",
              bundleName.c_str(), storeInfo.uid, storeInfo.tokenId);
        return OBJECT_PERMISSION_DENIED;
    }
    return OBJECT_SUCCESS;
}

void ObjectServiceImpl::Clear()
{
    ZLOGI("begin.");
    int32_t status = ObjectStoreManager::GetInstance()->Clear();
    if (status != OBJECT_SUCCESS) {
        ZLOGE("save fail %{public}d", status);
    }
    return;
}

int32_t ObjectServiceImpl::OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId)
{
    ZLOGI("begin. %{public}s", bundleName.c_str());
    int32_t result = ObjectStoreManager::GetInstance()->DeleteByAppId(bundleName);
    if (result != OBJECT_SUCCESS) {
        pid_t uid = IPCSkeleton::GetCallingUid();
        uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
        ZLOGE("Delete fail %{public}d, bundleName = %{public}s, uid = %{public}d, tokenId = 0x%{public}x",
            result, bundleName.c_str(), uid, tokenId);
    }
    return result;
}

int32_t ObjectServiceImpl::ResolveAutoLaunch(const std::string &identifier, DistributedDB::AutoLaunchParam &param)
{
    ZLOGI("ObjectServiceImpl::ResolveAutoLaunch start");
    ZLOGI("user:%{public}s appId:%{public}s storeId:%{public}s identifier:%{public}s", param.userId.c_str(),
          param.appId.c_str(), param.storeId.c_str(), DistributedData::Anonymous::Change(identifier).c_str());
    std::vector<StoreMetaData> metaData;
    auto prefix = StoreMetaData::GetPrefix({ Commu::GetInstance().GetLocalDevice().uuid, param.userId });
    if (!DistributedData::MetaDataManager::GetInstance().LoadMeta(prefix, metaData)) {
        ZLOGE("no store in user:%{public}s", param.userId.c_str());
        return OBJECT_STORE_NOT_FOUND;
    }
    
    for (const auto &storeMeta : metaData) {
        auto identifierTag = DistributedDB::KvStoreDelegateManager::GetKvStoreIdentifier("", storeMeta.appId,
                                                                                         storeMeta.storeId, true);
        if (identifier != identifierTag) {
            continue;
        }
        if (storeMeta.bundleName == DistributedData::Bootstrap::GetInstance().GetProcessLabel()) {
            int32_t status = DistributedObject::ObjectStoreManager::GetInstance()->Open();
            if (status != OBJECT_SUCCESS) {
                ZLOGE("Open fail %{public}d", status);
                continue;
            }
            DistributedObject::ObjectStoreManager::GetInstance()->CloseAfterMinute();
            return OBJECT_SUCCESS;
        }
    }
    return OBJECT_SUCCESS;
}

int32_t ObjectServiceImpl::OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &appId)
{
    ZLOGI("ObjectServiceImpl::OnAppExit uid=%{public}d, pid=%{public}d, tokenId=%{public}d, bundleName=%{public}s",
          uid, pid, tokenId, appId.c_str());
    ObjectStoreManager::GetInstance()->UnregisterRemoteCallback(appId, pid, tokenId);
    return FeatureSystem::STUB_SUCCESS;
}

ObjectServiceImpl::ObjectServiceImpl()
{
}
} // namespace OHOS::DistributedObject
