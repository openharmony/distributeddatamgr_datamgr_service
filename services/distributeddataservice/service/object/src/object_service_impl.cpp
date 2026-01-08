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

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "device_manager_adapter.h"
#include "directory/directory_manager.h"
#include "dump/dump_manager.h"
#include "eventcenter/event_center.h"
#include "log_print.h"
#include "metadata/appid_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "object_asset_loader.h"
#include "object_dms_handler.h"
#include "snapshot/bind_event.h"
#include "store/auto_cache.h"
#include "utils/anonymous.h"
#include "object_radar_reporter.h"

namespace OHOS::DistributedObject {
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using StoreMetaData = OHOS::DistributedData::StoreMetaData;
using FeatureSystem = OHOS::DistributedData::FeatureSystem;
using DumpManager = OHOS::DistributedData::DumpManager;
using CheckerManager = OHOS::DistributedData::CheckerManager;
using AppIDMetaData = OHOS::DistributedData::AppIDMetaData;
__attribute__((used)) ObjectServiceImpl::Factory ObjectServiceImpl::factory_;
constexpr const char *METADATA_STORE_PATH = "/data/service/el1/public/database/distributeddata/kvdb";
ObjectServiceImpl::Factory::Factory()
{
    FeatureSystem::GetInstance().RegisterCreator(
        "data_object",
        []() {
            return std::make_shared<ObjectServiceImpl>();
        },
        FeatureSystem::BIND_NOW);
    staticActs_ = std::make_shared<ObjectStatic>();
    FeatureSystem::GetInstance().RegisterStaticActs("data_object", staticActs_);
}

ObjectServiceImpl::Factory::~Factory()
{
}

int32_t ObjectServiceImpl::ObjectStoreSave(const std::string &bundleName, const std::string &sessionId,
    const std::string &deviceId, const std::map<std::string, std::vector<uint8_t>> &data,
    sptr<IRemoteObject> callback)
{
    ZLOGI("begin.");
    ObjectStore::RadarReporter::ReportStage(std::string(__FUNCTION__), ObjectStore::SAVE,
        ObjectStore::SAVE_TO_STORE, ObjectStore::IDLE);
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t status = IsBundleNameEqualTokenId(bundleName, sessionId, tokenId);
    if (status != OBJECT_SUCCESS) {
        return status;
    }
    status = ObjectStoreManager::GetInstance().Save(bundleName, sessionId, data, deviceId, callback);
    if (status != OBJECT_SUCCESS) {
        ZLOGE("save fail %{public}d", status);
    }
    ObjectStore::RadarReporter::ReportStage(std::string(__FUNCTION__), ObjectStore::SAVE,
        ObjectStore::SAVE_TO_STORE, ObjectStore::RADAR_SUCCESS);
    return status;
}

int32_t ObjectServiceImpl::OnAssetChanged(const std::string &bundleName, const std::string &sessionId,
    const std::string &deviceId, const ObjectStore::Asset &assetValue)
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t status = IsBundleNameEqualTokenId(bundleName, sessionId, tokenId);
    if (status != OBJECT_SUCCESS) {
        return status;
    }
    status = ObjectStoreManager::GetInstance().OnAssetChanged(tokenId, bundleName, sessionId, deviceId, assetValue);
    if (status != OBJECT_SUCCESS) {
        ZLOGE("file transfer failed fail %{public}d", status);
    }
    return status;
}

int32_t ObjectServiceImpl::BindAssetStore(const std::string &bundleName, const std::string &sessionId,
    ObjectStore::Asset &asset, ObjectStore::AssetBindInfo &bindInfo)
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t status = IsBundleNameEqualTokenId(bundleName, sessionId, tokenId);
    if (status != OBJECT_SUCCESS) {
        return status;
    }
    status = ObjectStoreManager::GetInstance().BindAsset(tokenId, bundleName, sessionId, asset, bindInfo);
    if (status != OBJECT_SUCCESS) {
        ZLOGE("bind asset fail %{public}d, bundleName:%{public}s, sessionId:%{public}s, assetName:%{public}s", status,
            bundleName.c_str(), Anonymous::Change(sessionId).c_str(), Anonymous::Change(asset.name).c_str());
    }
    return status;
}

int32_t ObjectServiceImpl::IsContinue(bool &result)
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    if (Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId) != Security::AccessToken::TOKEN_HAP) {
        ZLOGE("TokenType is not TOKEN_HAP, tokenId: %{public}u", tokenId);
        return OBJECT_INNER_ERROR;
    }
    Security::AccessToken::HapTokenInfo tokenInfo;
    auto status = Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (status != 0) {
        ZLOGE("Get hap token info failed, tokenId: %{public}u, status: %{public}d", tokenId, status);
        return status;
    }
    result = ObjectDmsHandler::GetInstance().IsContinue(tokenInfo.bundleName);
    return OBJECT_SUCCESS;
}

int32_t ObjectServiceImpl::OnInitialize()
{
    ZLOGI("Initialize");
    auto localDeviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (localDeviceId.empty()) {
        ZLOGE("failed to get local device id");
        return OBJECT_INNER_ERROR;
    }

    if (executors_ == nullptr) {
        ZLOGE("executors_ is nullptr");
        return OBJECT_INNER_ERROR;
    }
    executors_->Schedule(std::chrono::seconds(WAIT_ACCOUNT_SERVICE), [this]() {
        int foregroundUserId = 0;
        DistributedData::AccountDelegate::GetInstance()->QueryForegroundUserId(foregroundUserId);
        std::string userId = std::to_string(foregroundUserId);
        SaveMetaData(userId);
        ObjectStoreManager::GetInstance().SetData(METADATA_STORE_PATH, userId);
        ObjectStoreManager::GetInstance().InitUserMeta();
        RegisterObjectServiceInfo();
        RegisterHandler();
        ObjectDmsHandler::GetInstance().RegisterDmsEvent();
    });

    return OBJECT_SUCCESS;
}

int32_t ObjectServiceImpl::SaveMetaData(const std::string &userId)
{
    auto localDeviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (localDeviceId.empty()) {
        ZLOGE("failed to get local device id");
        return OBJECT_INNER_ERROR;
    }
    auto saveMeta = GetMetaData(localDeviceId, userId);
    if (!DistributedData::DirectoryManager::GetInstance().CreateDirectory(saveMeta.dataDir)) {
        ZLOGE("Create directory error, dataDir: %{public}s.", Anonymous::Change(saveMeta.dataDir).c_str());
        return OBJECT_INNER_ERROR;
    }
    StoreMetaData oldStoreMetaData;
    if (!MetaDataManager::GetInstance().LoadMeta(saveMeta.GetKey(), oldStoreMetaData, true)
        || saveMeta != oldStoreMetaData) {
        if (!MetaDataManager::GetInstance().SaveMeta(saveMeta.GetKey(), saveMeta, true)) {
            ZLOGE("SaveMeta getKey failed");
            return OBJECT_INNER_ERROR;
        }
    }
    if (!MetaDataManager::GetInstance().LoadMeta(saveMeta.GetKeyWithoutPath(), oldStoreMetaData)
        || saveMeta != oldStoreMetaData) {
        if (!MetaDataManager::GetInstance().SaveMeta(saveMeta.GetKeyWithoutPath(), saveMeta)) {
            ZLOGE("SaveMeta getKeyWithoutPath failed");
            return OBJECT_INNER_ERROR;
        }
    }
    AppIDMetaData appIdMeta;
    appIdMeta.bundleName = saveMeta.bundleName;
    appIdMeta.appId = saveMeta.appId;
    AppIDMetaData oldAppIdMeta;
    if (!MetaDataManager::GetInstance().LoadMeta(appIdMeta.GetKey(), oldAppIdMeta, true) || appIdMeta != oldAppIdMeta) {
        if (!MetaDataManager::GetInstance().SaveMeta(appIdMeta.GetKey(), appIdMeta, true)) {
            ZLOGE("Save appIdMeta failed");
            return OBJECT_INNER_ERROR;
        }
    }
    ZLOGI("SaveMeta success appId %{public}s, storeId %{public}s", saveMeta.appId.c_str(),
        saveMeta.GetStoreAlias().c_str());
    return OBJECT_SUCCESS;
}

int32_t ObjectServiceImpl::OnUserChange(uint32_t code, const std::string &user, const std::string &account)
{
    if (code == static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_SWITCHED)) {
        int32_t status = ObjectStoreManager::GetInstance().Clear();
        if (status != OBJECT_SUCCESS) {
            ZLOGE("Clear fail user:%{public}s, status: %{public}d", user.c_str(), status);
        }
        int foregroundUserId = 0;
        DistributedData::AccountDelegate::GetInstance()->QueryForegroundUserId(foregroundUserId);
        std::string userId = std::to_string(foregroundUserId);
        SaveMetaData(userId);
        ObjectStoreManager::GetInstance().SetData(METADATA_STORE_PATH, userId);
    }
    return Feature::OnUserChange(code, user, account);
}

int32_t ObjectServiceImpl::ObjectStoreRevokeSave(
    const std::string &bundleName, const std::string &sessionId, sptr<IRemoteObject> callback)
{
    ZLOGI("begin.");
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t status = IsBundleNameEqualTokenId(bundleName, sessionId, tokenId);
    if (status != OBJECT_SUCCESS) {
        return status;
    }
    status = ObjectStoreManager::GetInstance().RevokeSave(bundleName, sessionId, callback);
    if (status != OBJECT_SUCCESS) {
        ZLOGE("revoke save fail %{public}d", status);
        return status;
    }
    return OBJECT_SUCCESS;
}

int32_t ObjectServiceImpl::ObjectStoreRetrieve(
    const std::string &bundleName, const std::string &sessionId, sptr<IRemoteObject> callback)
{
    ZLOGI("begin.");
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t status = IsBundleNameEqualTokenId(bundleName, sessionId, tokenId);
    if (status != OBJECT_SUCCESS) {
        return status;
    }
    status = ObjectStoreManager::GetInstance().Retrieve(bundleName, sessionId, callback, tokenId);
    if (status != OBJECT_SUCCESS) {
        ZLOGE("retrieve fail %{public}d", status);
        return status;
    }
    return OBJECT_SUCCESS;
}

int32_t ObjectServiceImpl::RegisterDataObserver(
    const std::string &bundleName, const std::string &sessionId, sptr<IRemoteObject> callback)
{
    ZLOGD("begin.");
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t status = IsBundleNameEqualTokenId(bundleName, sessionId, tokenId);
    if (status != OBJECT_SUCCESS) {
        return status;
    }
    auto pid = IPCSkeleton::GetCallingPid();
    ObjectStoreManager::GetInstance().RegisterRemoteCallback(bundleName, sessionId, pid, tokenId, callback);
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
    ObjectStoreManager::GetInstance().UnregisterRemoteCallback(bundleName, pid, tokenId, sessionId);
    return OBJECT_SUCCESS;
}

int32_t ObjectServiceImpl::RegisterProgressObserver(
    const std::string &bundleName, const std::string &sessionId, sptr<IRemoteObject> callback)
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t status = IsBundleNameEqualTokenId(bundleName, sessionId, tokenId);
    if (status != OBJECT_SUCCESS) {
        return status;
    }
    auto pid = IPCSkeleton::GetCallingPid();
    ObjectStoreManager::GetInstance().RegisterProgressObserverCallback(bundleName, sessionId, pid, tokenId, callback);
    return OBJECT_SUCCESS;
}

int32_t ObjectServiceImpl::UnregisterProgressObserver(const std::string &bundleName, const std::string &sessionId)
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t status = IsBundleNameEqualTokenId(bundleName, sessionId, tokenId);
    if (status != OBJECT_SUCCESS) {
        return status;
    }
    auto pid = IPCSkeleton::GetCallingPid();
    ObjectStoreManager::GetInstance().UnregisterProgressObserverCallback(bundleName, pid, tokenId, sessionId);
    return OBJECT_SUCCESS;
}

int32_t ObjectServiceImpl::DeleteSnapshot(const std::string &bundleName, const std::string &sessionId)
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    int32_t status = IsBundleNameEqualTokenId(bundleName, sessionId, tokenId);
    if (status != OBJECT_SUCCESS) {
        return status;
    }
    ObjectStoreManager::GetInstance().DeleteSnapshot(bundleName, sessionId);
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
    if (!CheckerManager::GetInstance().IsValid(storeInfo)) {
        ZLOGE("object bundleName wrong, bundleName = %{public}s, uid = %{public}d, tokenId = %{public}s",
              bundleName.c_str(), storeInfo.uid, Anonymous::Change(std::to_string(storeInfo.tokenId)).c_str());
        return OBJECT_PERMISSION_DENIED;
    }
    return OBJECT_SUCCESS;
}

int32_t ObjectServiceImpl::ObjectStatic::OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index,
    int32_t tokenId)
{
    int32_t result = ObjectStoreManager::GetInstance().DeleteByAppId(bundleName, user);
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Delete object data failed, result:%{public}d, bundleName:%{public}s, user:%{public}d, index:%{public}d",
            result, bundleName.c_str(), user, index);
        return result;
    }
    ZLOGI("Delete object data, bundleName:%{public}s, userId:%{public}d, index:%{public}d", bundleName.c_str(), user,
        index);
    return result;
}

int32_t ObjectServiceImpl::ResolveAutoLaunch(const std::string &identifier, DistributedDB::AutoLaunchParam &param)
{
    ZLOGI("start, user:%{public}s appId:%{public}s storeId:%{public}s identifier:%{public}s", param.userId.c_str(),
        param.appId.c_str(), DistributedData::Anonymous::Change(param.storeId).c_str(),
        DistributedData::Anonymous::Change(identifier).c_str());
    std::vector<StoreMetaData> metaData;
    auto prefix = StoreMetaData::GetPrefix({ DmAdapter::GetInstance().GetLocalDevice().uuid });
    if (!DistributedData::MetaDataManager::GetInstance().LoadMeta(prefix, metaData)) {
        ZLOGE("no store in user:%{public}s", param.userId.c_str());
        return OBJECT_STORE_NOT_FOUND;
    }
    
    for (const auto &storeMeta : metaData) {
        if (storeMeta.storeType < StoreMetaData::StoreType::STORE_OBJECT_BEGIN
            || storeMeta.storeType > StoreMetaData::StoreType::STORE_OBJECT_END) {
            continue;
        }
        auto identifierTag = DistributedDB::KvStoreDelegateManager::GetKvStoreIdentifier("", storeMeta.appId,
                                                                                         storeMeta.storeId, true);
        if (identifier != identifierTag) {
            continue;
        }
        if (storeMeta.bundleName == DistributedData::Bootstrap::GetInstance().GetProcessLabel()) {
            int32_t status = DistributedObject::ObjectStoreManager::GetInstance().AutoLaunchStore();
            if (status != OBJECT_SUCCESS) {
                continue;
            }
            return OBJECT_SUCCESS;
        }
    }
    return OBJECT_SUCCESS;
}

int32_t ObjectServiceImpl::OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &appId)
{
    ZLOGI("ObjectServiceImpl::OnAppExit uid=%{public}d, pid=%{public}d, tokenId=%{public}d, bundleName=%{public}s",
          uid, pid, tokenId, appId.c_str());
    ObjectStoreManager::GetInstance().UnregisterRemoteCallback(appId, pid, tokenId);
    ObjectStoreManager::GetInstance().UnregisterProgressObserverCallback(appId, pid, tokenId);
    return FeatureSystem::STUB_SUCCESS;
}

ObjectServiceImpl::ObjectServiceImpl()
{
    auto process = [](const Event& event) {
        auto& evt = static_cast<const BindEvent&>(event);
        auto eventInfo = evt.GetBindInfo();
        StoreMetaData meta;
        meta.storeId = eventInfo.storeName;
        meta.bundleName = eventInfo.bundleName;
        meta.user = std::to_string(eventInfo.user);
        meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
        if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKeyWithoutPath(), meta)) {
            ZLOGE("meta empty, bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
                meta.GetStoreAlias().c_str());
            return;
        }
        auto store = AutoCache::GetInstance().GetStore(meta, {});
        if (store == nullptr) {
            ZLOGE("store null, storeId:%{public}s", meta.GetStoreAlias().c_str());
            return;
        }
        auto bindAssets = ObjectStoreManager::GetInstance().GetSnapShots(eventInfo.bundleName, eventInfo.storeName);
        store->BindSnapshots(bindAssets);
    };
    EventCenter::GetInstance().Subscribe(BindEvent::BIND_SNAPSHOT, process);
}

void ObjectServiceImpl::RegisterObjectServiceInfo()
{
    // LCOV_EXCL_START
    DumpManager::Config serviceInfoConfig;
    serviceInfoConfig.fullCmd = "--feature-info";
    serviceInfoConfig.abbrCmd = "-f";
    serviceInfoConfig.dumpName = "FEATURE_INFO";
    serviceInfoConfig.dumpCaption = { "| Display all the service statistics" };
    DumpManager::GetInstance().AddConfig("FEATURE_INFO", serviceInfoConfig);
    // LCOV_EXCL_STOP
}

void ObjectServiceImpl::RegisterHandler()
{
    // LCOV_EXCL_START
    Handler handler = [this](int fd, std::map<std::string, std::vector<std::string>> &params) {
        DumpObjectServiceInfo(fd, params);
    };
    DumpManager::GetInstance().AddHandler("FEATURE_INFO", uintptr_t(this), handler);
    // LCOV_EXCL_STOP
}

void ObjectServiceImpl::DumpObjectServiceInfo(int fd, std::map<std::string, std::vector<std::string>> &params)
{
    (void)params;
    std::string info;
    dprintf(fd, "-------------------------------------ObjectServiceInfo------------------------------\n%s\n",
        info.c_str());
}
ObjectServiceImpl::~ObjectServiceImpl()
{
    DumpManager::GetInstance().RemoveHandler("FEATURE_INFO", uintptr_t(this));
}

int32_t ObjectServiceImpl::OnBind(const BindInfo &bindInfo)
{
    executors_ = bindInfo.executors;
    ObjectStoreManager::GetInstance().SetThreadPool(executors_);
    ObjectAssetLoader::GetInstance().SetThreadPool(executors_);
    return 0;
}

StoreMetaData ObjectServiceImpl::GetMetaData(const std::string &deviceId, const std::string &userId)
{
    StoreMetaData storeMetaData;
    storeMetaData.appType = "default";
    storeMetaData.deviceId = deviceId;
    storeMetaData.storeId = DistributedObject::ObjectCommon::OBJECTSTORE_DB_STOREID;
    storeMetaData.isAutoSync = false;
    storeMetaData.isBackup = false;
    storeMetaData.isEncrypt = false;
    storeMetaData.bundleName = DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    storeMetaData.appId = DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    storeMetaData.account = DistributedData::AccountDelegate::GetInstance()->GetCurrentAccountId();
    storeMetaData.tokenId = IPCSkeleton::GetCallingTokenID();
    storeMetaData.securityLevel = DistributedKv::SecurityLevel::S1;
    storeMetaData.area = DistributedKv::Area::EL1;
    storeMetaData.uid = IPCSkeleton::GetCallingUid();
    storeMetaData.storeType = ObjectDistributedType::OBJECT_SINGLE_VERSION;
    storeMetaData.dataType = DistributedKv::DataType::TYPE_DYNAMICAL;
    storeMetaData.authType = DistributedKv::AuthType::IDENTICAL_ACCOUNT;
    storeMetaData.user = userId;
    storeMetaData.dataDir = METADATA_STORE_PATH;
    return storeMetaData;
}
} // namespace OHOS::DistributedObject
