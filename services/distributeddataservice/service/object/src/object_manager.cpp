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
#define LOG_TAG "ObjectStoreManager"

#include "object_manager.h"

#include <regex>

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "block_data.h"
#include "bootstrap.h"
#include "common/bytes.h"
#include "common/string_utils.h"
#include "datetime_ex.h"
#include "distributed_file_daemon_manager.h"
#include "device_matrix.h"
#include "ipc_skeleton.h"
#include "metadata/capability_meta_data.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/object_user_meta_data.h"
#include "metadata/store_meta_data.h"
#include "object_dms_handler.h"
#include "object_radar_reporter.h"
#include "utils/anonymous.h"
#include "utils/constant.h"

namespace OHOS {
namespace DistributedObject {
using namespace OHOS::DistributedKv;
using namespace Security::AccessToken;
using StoreMetaData = OHOS::DistributedData::StoreMetaData;
using Account = OHOS::DistributedData::AccountDelegate;
using AccessTokenKit = Security::AccessToken::AccessTokenKit;
using ValueProxy = OHOS::DistributedData::ValueProxy;
using DistributedFileDaemonManager = Storage::DistributedFile::DistributedFileDaemonManager;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using ObjectUserMetaData = OHOS::DistributedData::ObjectUserMetaData;

constexpr const char *SAVE_INFO = "p_###SAVEINFO###";
constexpr int32_t PROGRESS_MAX = 100;
constexpr int32_t PROGRESS_INVALID = -1;
constexpr uint32_t LOCK_TIMEOUT = 3600; // second
ObjectStoreManager::ObjectStoreManager()
{
    ZLOGI("ObjectStoreManager construct");
    RegisterAssetsLister();
}

ObjectStoreManager::~ObjectStoreManager()
{
}

DistributedDB::KvStoreNbDelegate *ObjectStoreManager::OpenObjectKvStore()
{
    DistributedDB::KvStoreNbDelegate *store = nullptr;
    DistributedDB::KvStoreNbDelegate::Option option;
    option.createDirByStoreIdOnly = true;
    option.syncDualTupleMode = true;
    option.secOption = { DistributedDB::S1, DistributedDB::ECE };
    ZLOGD("start GetKvStore");
    auto kvStoreDelegateManager = GetKvStoreDelegateManager();
    if (kvStoreDelegateManager == nullptr) {
        ZLOGE("Kvstore delegate manager is nullptr.");
        return store;
    }
    if (objectDataListener_ == nullptr) {
        objectDataListener_ = std::make_shared<ObjectDataListener>();
    }
    kvStoreDelegateManager->GetKvStore(ObjectCommon::OBJECTSTORE_DB_STOREID, option,
        [&store, this](DistributedDB::DBStatus dbStatus, DistributedDB::KvStoreNbDelegate *kvStoreNbDelegate) {
            if (dbStatus != DistributedDB::DBStatus::OK) {
                ZLOGE("GetKvStore fail %{public}d", dbStatus);
                return;
            }
            if (kvStoreNbDelegate == nullptr) {
                ZLOGE("GetKvStore kvStoreNbDelegate is nullptr");
                return;
            }
            ZLOGI("GetKvStore successsfully");
            store = kvStoreNbDelegate;
            std::vector<uint8_t> tmpKey;
            DistributedDB::DBStatus status = store->RegisterObserver(tmpKey,
                DistributedDB::ObserverMode::OBSERVER_CHANGES_FOREIGN, objectDataListener_);
            if (status != DistributedDB::DBStatus::OK) {
                ZLOGE("RegisterObserver err %{public}d", status);
            }
        });
    return store;
}

bool ObjectStoreManager::RegisterAssetsLister()
{
    if (objectAssetsSendListener_ == nullptr) {
        objectAssetsSendListener_ = new ObjectAssetsSendListener();
    }
    if (objectAssetsRecvListener_ == nullptr) {
        objectAssetsRecvListener_ = new ObjectAssetsRecvListener();
    }
    auto status = DistributedFileDaemonManager::GetInstance().RegisterAssetCallback(objectAssetsRecvListener_);
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("Register assetsRecvListener err %{public}d", status);
        return false;
    }
    return true;
}

bool ObjectStoreManager::UnRegisterAssetsLister()
{
    ZLOGI("ObjectStoreManager UnRegisterAssetsLister");
    if (objectAssetsRecvListener_ != nullptr) {
        auto status = DistributedFileDaemonManager::GetInstance().UnRegisterAssetCallback(objectAssetsRecvListener_);
        if (status != DistributedDB::DBStatus::OK) {
            ZLOGE("UnRegister assetsRecvListener err %{public}d", status);
            return false;
        }
    }
    return true;
}

void ObjectStoreManager::ProcessSyncCallback(const std::map<std::string, int32_t> &results, const std::string &appId,
    const std::string &sessionId, const std::string &deviceId)
{
    if (results.empty() || results.find(LOCAL_DEVICE) != results.end()) {
        return;
    }
    int32_t result = Open();
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Open failed, errCode = %{public}d", result);
        return;
    }
    // delete local data
    result = RevokeSaveToStore(GetPropertyPrefix(appId, sessionId, deviceId));
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Save failed, status = %{public}d", result);
    }
    Close();
    return;
}

int32_t ObjectStoreManager::Save(const std::string &appId, const std::string &sessionId,
    const ObjectRecord &data, const std::string &deviceId, sptr<IRemoteObject> callback)
{
    sptr<ObjectSaveCallbackProxy> proxy = new (std::nothrow)ObjectSaveCallbackProxy(callback);
    if (proxy == nullptr) {
        ZLOGE("proxy is nullptr, callback is %{public}s.", (callback == nullptr) ? "nullptr" : "not null");
        return INVALID_ARGUMENT;
    }
    if (deviceId.size() == 0) {
        ZLOGE("DeviceId empty, appId: %{public}s, sessionId: %{public}s", appId.c_str(),
            Anonymous::Change(sessionId).c_str());
        proxy->Completed(std::map<std::string, int32_t>());
        return INVALID_ARGUMENT;
    }
    int32_t result = Open();
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Open object kvstore failed, result: %{public}d", result);
        ObjectStore::RadarReporter::ReportStateError(std::string(__FUNCTION__), ObjectStore::SAVE,
            ObjectStore::SAVE_TO_STORE, ObjectStore::RADAR_FAILED, ObjectStore::GETKV_FAILED, ObjectStore::FINISHED);
        proxy->Completed(std::map<std::string, int32_t>());
        return STORE_NOT_OPEN;
    }
    SaveUserToMeta();
    std::string dstBundleName = ObjectDmsHandler::GetInstance().GetDstBundleName(appId, deviceId);
    result = SaveToStore(dstBundleName, sessionId, deviceId, data);
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Save to store failed, result: %{public}d", result);
        ObjectStore::RadarReporter::ReportStateError(std::string(__FUNCTION__), ObjectStore::SAVE,
            ObjectStore::SAVE_TO_STORE, ObjectStore::RADAR_FAILED, result, ObjectStore::FINISHED);
        Close();
        proxy->Completed(std::map<std::string, int32_t>());
        return result;
    }
    ZLOGI("Sync data, bundleName: %{public}s, sessionId: %{public}s, deviceId: %{public}s", dstBundleName.c_str(),
        Anonymous::Change(sessionId).c_str(), Anonymous::Change(deviceId).c_str());
    SyncCallBack syncCallback =
        [proxy, dstBundleName, sessionId, deviceId, this](const std::map<std::string, int32_t> &results) {
            ProcessSyncCallback(results, dstBundleName, sessionId, deviceId);
            proxy->Completed(results);
        };
    result = SyncOnStore(GetPropertyPrefix(dstBundleName, sessionId, deviceId), { deviceId }, syncCallback);
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Sync data failed, result: %{public}d", result);
        ObjectStore::RadarReporter::ReportStateError(std::string(__FUNCTION__), ObjectStore::SAVE,
            ObjectStore::SYNC_DATA, ObjectStore::RADAR_FAILED, result, ObjectStore::FINISHED);
        Close();
        proxy->Completed(std::map<std::string, int32_t>());
        return result;
    }
    Close();
    return PushAssets(appId, dstBundleName, sessionId, data, deviceId);
}

int32_t ObjectStoreManager::PushAssets(const std::string &srcBundleName, const std::string &dstBundleName,
    const std::string &sessionId, const ObjectRecord &data, const std::string &deviceId)
{
    Assets assets = GetAssetsFromDBRecords(data);
    if (assets.empty() || data.find(ObjectStore::FIELDS_PREFIX + ObjectStore::DEVICEID_KEY) == data.end()) {
        return OBJECT_SUCCESS;
    }
    sptr<AssetObj> assetObj = new (std::nothrow) AssetObj();
    if (assetObj == nullptr) {
        ZLOGE("New AssetObj failed");
        return OBJECT_INNER_ERROR;
    }
    assetObj->dstBundleName_ = dstBundleName;
    assetObj->srcBundleName_ = srcBundleName;
    assetObj->dstNetworkId_ = deviceId;
    assetObj->sessionId_ = sessionId;
    for (const auto& asset : assets) {
        assetObj->uris_.push_back(asset.uri);
    }
    if (objectAssetsSendListener_ == nullptr) {
        objectAssetsSendListener_ = new ObjectAssetsSendListener();
    }
    int userId = std::atoi(GetCurrentUser().c_str());
    auto status = ObjectAssetLoader::GetInstance().PushAsset(userId, assetObj, objectAssetsSendListener_);
    return status;
}

int32_t ObjectStoreManager::RevokeSave(
    const std::string &appId, const std::string &sessionId, sptr<IRemoteObject> callback)
{
    sptr<ObjectRevokeSaveCallbackProxy> proxy = new (std::nothrow)ObjectRevokeSaveCallbackProxy(callback);
    if (proxy == nullptr) {
        ZLOGE("proxy is nullptr, callback is %{public}s, appId: %{public}s, sessionId: %{public}s.",
            (callback == nullptr) ? "nullptr" : "not null", appId.c_str(), Anonymous::Change(sessionId).c_str());
        return INVALID_ARGUMENT;
    }
    int32_t result = Open();
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Open failed, errCode = %{public}d", result);
        proxy->Completed(STORE_NOT_OPEN);
        return STORE_NOT_OPEN;
    }

    result = RevokeSaveToStore(GetPrefixWithoutDeviceId(appId, sessionId));
    if (result != OBJECT_SUCCESS) {
        ZLOGE("RevokeSave failed, errCode = %{public}d", result);
        Close();
        proxy->Completed(result);
        return result;
    }
    std::vector<std::string> deviceList;
    auto deviceInfos = DmAdaper::GetInstance().GetRemoteDevices();
    std::for_each(deviceInfos.begin(), deviceInfos.end(),
        [&deviceList](AppDistributedKv::DeviceInfo info) { deviceList.emplace_back(info.networkId); });
    if (!deviceList.empty()) {
        SyncCallBack tmp = [proxy](const std::map<std::string, int32_t> &results) {
            ZLOGI("revoke save finished");
            proxy->Completed(OBJECT_SUCCESS);
        };
        result = SyncOnStore(GetPropertyPrefix(appId, sessionId), deviceList, tmp);
        if (result != OBJECT_SUCCESS) {
            ZLOGE("sync failed, errCode = %{public}d", result);
            proxy->Completed(result);
        }
    } else {
        proxy->Completed(OBJECT_SUCCESS);
    };
    Close();
    return result;
}

int32_t ObjectStoreManager::Retrieve(
    const std::string &bundleName, const std::string &sessionId, sptr<IRemoteObject> callback, uint32_t tokenId)
{
    sptr<ObjectRetrieveCallbackProxy> proxy = new (std::nothrow) ObjectRetrieveCallbackProxy(callback);
    if (proxy == nullptr) {
        ZLOGE("proxy is nullptr, callback is %{public}s.", (callback == nullptr) ? "nullptr" : "not null");
        return INVALID_ARGUMENT;
    }
    int32_t result = Open();
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Open object kvstore failed, result: %{public}d", result);
        proxy->Completed(ObjectRecord(), false);
        return ObjectStore::GETKV_FAILED;
    }
    ObjectRecord results{};
    int32_t status = RetrieveFromStore(bundleName, sessionId, results);
    if (status != OBJECT_SUCCESS) {
        ZLOGE("Retrieve from store failed, status: %{public}d, close after one minute.", status);
        CloseAfterMinute();
        proxy->Completed(ObjectRecord(), false);
        return status;
    }
    bool allReady = false;
    Assets assets = GetAssetsFromDBRecords(results);
    if (assets.empty() || results.find(ObjectStore::FIELDS_PREFIX + ObjectStore::DEVICEID_KEY) == results.end()) {
        allReady = true;
    } else {
        restoreStatus_.ComputeIfPresent(bundleName + sessionId, [&allReady](const auto &key, auto &value) {
            if (value == RestoreStatus::ALL_READY) {
                allReady = true;
                return false;
            }
            if (value == RestoreStatus::DATA_READY) {
                value = RestoreStatus::DATA_NOTIFIED;
            }
            return true;
        });
    }
    status = RevokeSaveToStore(GetPrefixWithoutDeviceId(bundleName, sessionId));
    Close();
    if (status != OBJECT_SUCCESS) {
        ZLOGE("Revoke save failed, status: %{public}d", status);
        proxy->Completed(ObjectRecord(), false);
        return status;
    }
    proxy->Completed(results, allReady);
    if (allReady) {
        ObjectStore::RadarReporter::ReportStateFinished(std::string(__FUNCTION__), ObjectStore::DATA_RESTORE,
            ObjectStore::NOTIFY, ObjectStore::RADAR_SUCCESS, ObjectStore::FINISHED);
    }
    return status;
}

int32_t ObjectStoreManager::Clear()
{
    ZLOGI("enter");
    DistributedData::ObjectUserMetaData userMeta;
    if (!DistributedData::MetaDataManager::GetInstance().LoadMeta(userMeta.GetKey(), userMeta, true)) {
        ZLOGE("load meta error");
        return OBJECT_INNER_ERROR;
    }
    if (userMeta.userId.empty()) {
        ZLOGI("no object user meta, don't need clean");
        return OBJECT_SUCCESS;
    }
    std::string userId = GetCurrentUser();
    if (userId.empty()) {
        ZLOGE("no user error");
        return OBJECT_INNER_ERROR;
    }
    if (userMeta.userId == userId) {
        ZLOGI("user is same, don't need clear, user:%{public}s.", userId.c_str());
        return OBJECT_SUCCESS;
    }
    ZLOGI("user changed, need clear, userId:%{public}s", userId.c_str());
    int32_t result = Open();
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Open failed, errCode = %{public}d", result);
        return STORE_NOT_OPEN;
    }
    result = RevokeSaveToStore("");
    callbacks_.Clear();
    processCallbacks_.Clear();
    ForceClose();
    return result;
}

int32_t ObjectStoreManager::InitUserMeta()
{
    ObjectUserMetaData userMeta;
    if (DistributedData::MetaDataManager::GetInstance().LoadMeta(userMeta.GetKey(), userMeta, true)) {
        ZLOGI("userId has been set, don't need clean");
        return OBJECT_SUCCESS;
    }
    std::string userId = GetCurrentUser();
    if (userId.empty()) {
        ZLOGI("get userId error");
        return OBJECT_INNER_ERROR;
    }
    userMeta.userId = userId;
    std::string appId = DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    std::string metaKey = GetMetaUserIdKey(userId, appId);
    if (!DistributedData::MetaDataManager::GetInstance().DelMeta(metaKey, true)) {
        ZLOGE("delete old meta error, userId:%{public}s", userId.c_str());
        return OBJECT_INNER_ERROR;
    }
    if (!DistributedData::MetaDataManager::GetInstance().SaveMeta(DistributedData::ObjectUserMetaData::GetKey(),
        userMeta, true)) {
        ZLOGE("save meta error, userId:%{public}s", userId.c_str());
        return OBJECT_INNER_ERROR;
    }
    return OBJECT_SUCCESS;
}

int32_t ObjectStoreManager::DeleteByAppId(const std::string &appId, int32_t user)
{
    int32_t result = Open();
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Open store failed, result: %{public}d, appId: %{public}s, user: %{public}d", result,
            appId.c_str(), user);
        return STORE_NOT_OPEN;
    }
    result = RevokeSaveToStore(appId);
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Revoke save failed, result: %{public}d, appId: %{public}s, user: %{public}d", result,
            appId.c_str(), user);
    }
    Close();
    return result;
}

void ObjectStoreManager::RegisterRemoteCallback(const std::string &bundleName, const std::string &sessionId,
                                                pid_t pid, uint32_t tokenId,
                                                sptr<IRemoteObject> callback)
{
    if (bundleName.empty() || sessionId.empty()) {
        ZLOGD("ObjectStoreManager::RegisterRemoteCallback empty");
        return;
    }
    ZLOGD("ObjectStoreManager::RegisterRemoteCallback start");
    sptr<ObjectChangeCallbackProxy> proxy = new (std::nothrow) ObjectChangeCallbackProxy(callback);
    if (proxy == nullptr) {
        ZLOGE("proxy is nullptr, callback is %{public}s, bundleName: %{public}s, sessionId: %{public}s, pid: "
              "%{public}d, tokenId: %{public}u.",
            (callback == nullptr) ? "nullptr" : "not null", bundleName.c_str(), Anonymous::Change(sessionId).c_str(),
            pid, tokenId);
        return;
    }
    std::string prefix = bundleName + sessionId;
    callbacks_.Compute(tokenId, ([pid, &proxy, &prefix](const uint32_t key, CallbackInfo &value) {
        if (value.pid != pid) {
            value = CallbackInfo { pid };
        }
        value.observers_.insert_or_assign(prefix, proxy);
        return !value.observers_.empty();
    }));
}

void ObjectStoreManager::UnregisterRemoteCallback(
    const std::string &bundleName, pid_t pid, uint32_t tokenId, const std::string &sessionId)
{
    if (bundleName.empty()) {
        ZLOGD("bundleName is empty");
        return;
    }
    callbacks_.Compute(tokenId, ([pid, &sessionId, &bundleName](const uint32_t key, CallbackInfo &value) {
        if (value.pid != pid) {
            return true;
        }
        if (sessionId.empty()) {
            return false;
        }
        std::string prefix = bundleName + sessionId;
        for (auto it = value.observers_.begin(); it != value.observers_.end();) {
            if ((*it).first == prefix) {
                it = value.observers_.erase(it);
            } else {
                ++it;
            }
        }
        return true;
    }));
}

void ObjectStoreManager::RegisterProgressObserverCallback(const std::string &bundleName, const std::string &sessionId,
    pid_t pid, uint32_t tokenId, sptr<IRemoteObject> callback)
{
    if (bundleName.empty() || sessionId.empty()) {
        ZLOGD("ObjectStoreManager::RegisterProgressObserverCallback empty bundleName = %{public}s, sessionId = "
              "%{public}s",
            bundleName.c_str(), DistributedData::Anonymous::Change(sessionId).c_str());
        return;
    }
    sptr<ObjectProgressCallbackProxy> proxy = new (std::nothrow) ObjectProgressCallbackProxy(callback);
    if (proxy == nullptr) {
        ZLOGE("proxy is nullptr, callback is %{public}s, bundleName: %{public}s, sessionId: %{public}s, pid: "
              "%{public}d, tokenId: %{public}u.",
            (callback == nullptr) ? "nullptr" : "not null", bundleName.c_str(), Anonymous::Change(sessionId).c_str(),
            pid, tokenId);
        return;
    }
    std::string objectKey = bundleName + sessionId;
    sptr<ObjectProgressCallbackProxyBroker> observer;
    processCallbacks_.Compute(
        tokenId, ([pid, &proxy, &objectKey, &observer](const uint32_t key, ProgressCallbackInfo &value) {
            if (value.pid != pid) {
                value = ProgressCallbackInfo{ pid };
            }
            value.observers_.insert_or_assign(objectKey, proxy);
            observer = value.observers_[objectKey];
            return !value.observers_.empty();
        }));
    int32_t progress = PROGRESS_INVALID;
    {
        std::lock_guard<std::mutex> lock(progressMutex_);
        auto it = progressInfo_.find(objectKey);
        if (it == progressInfo_.end()) {
            return;
        }
        progress = it->second;
    }
    if (observer != nullptr && progress != PROGRESS_INVALID) {
        observer->Completed(progress);
    }
    if (progress == PROGRESS_MAX) {
        assetsRecvProgress_.Erase(objectKey);
        std::lock_guard<std::mutex> lock(progressMutex_);
        progressInfo_.erase(objectKey);
    }
}

void ObjectStoreManager::UnregisterProgressObserverCallback(
    const std::string &bundleName, pid_t pid, uint32_t tokenId, const std::string &sessionId)
{
    if (bundleName.empty()) {
        ZLOGD("bundleName is empty");
        return;
    }
    processCallbacks_.Compute(
        tokenId, ([pid, &sessionId, &bundleName](const uint32_t key, ProgressCallbackInfo &value) {
        if (value.pid != pid) {
            return true;
        }
        if (sessionId.empty()) {
            return false;
        }
        std::string prefix = bundleName + sessionId;
        for (auto it = value.observers_.begin(); it != value.observers_.end();) {
            if ((*it).first == prefix) {
                it = value.observers_.erase(it);
            } else {
                ++it;
            }
        }
        return true;
    }));
}

void ObjectStoreManager::NotifyChange(const ObjectRecord &changedData)
{
    ZLOGI("OnChange start, size:%{public}zu", changedData.size());
    bool hasAsset = false;
    SaveInfo saveInfo;
    for (const auto &[key, value] : changedData) {
        if (key.find(SAVE_INFO) != std::string::npos) {
            DistributedData::Serializable::Unmarshall(std::string(value.begin(), value.end()), saveInfo);
            break;
        }
    }
    auto data = GetObjectData(changedData, saveInfo, hasAsset);
    if (!hasAsset) {
        ObjectStore::RadarReporter::ReportStateStart(std::string(__FUNCTION__), ObjectStore::DATA_RESTORE,
            ObjectStore::DATA_RECV, ObjectStore::RADAR_SUCCESS, ObjectStore::START, saveInfo.bundleName);
        callbacks_.ForEach([this, &data](uint32_t tokenId, const CallbackInfo& value) {
            DoNotify(tokenId, value, data, true); // no asset, data ready means all ready
            return false;
        });
        return;
    }
    NotifyDataChanged(data, saveInfo);
    SaveUserToMeta();
}

std::map<std::string, ObjectRecord> ObjectStoreManager::GetObjectData(const ObjectRecord& changedData,
    SaveInfo& saveInfo, bool& hasAsset)
{
    std::map<std::string, ObjectRecord> data;
    std::string keyPrefix = saveInfo.ToPropertyPrefix();
    if (!keyPrefix.empty()) {
        std::string observerKey = saveInfo.bundleName + saveInfo.sessionId;
        for (const auto &[key, value] : changedData) {
            if (key.size() < keyPrefix.size() || key.find(SAVE_INFO) != std::string::npos) {
                continue;
            }
            std::string propertyName = key.substr(keyPrefix.size());
            data[observerKey].insert_or_assign(propertyName, value);
            if (!hasAsset && IsAssetKey(propertyName)) {
                hasAsset = true;
            }
        }
    } else {
        for (const auto &item : changedData) {
            std::vector<std::string> splitKeys = SplitEntryKey(item.first);
            if (splitKeys.size() <= PROPERTY_NAME_INDEX) {
                continue;
            }
            if (saveInfo.sourceDeviceId.empty() || saveInfo.bundleName.empty()) {
                saveInfo.sourceDeviceId = splitKeys[SOURCE_DEVICE_UDID_INDEX];
                saveInfo.bundleName = splitKeys[BUNDLE_NAME_INDEX];
                saveInfo.sessionId = splitKeys[SESSION_ID_INDEX];
                saveInfo.timestamp = splitKeys[TIME_INDEX];
            }
            std::string prefix = splitKeys[BUNDLE_NAME_INDEX] + splitKeys[SESSION_ID_INDEX];
            std::string propertyName = splitKeys[PROPERTY_NAME_INDEX];
            data[prefix].insert_or_assign(propertyName, item.second);
            if (IsAssetKey(propertyName)) {
                hasAsset = true;
            }
        }
    }
    return data;
}

void ObjectStoreManager::ComputeStatus(const std::string& objectKey, const SaveInfo& saveInfo,
    const std::map<std::string, ObjectRecord>& data)
{
    restoreStatus_.Compute(objectKey, [this, &data, saveInfo] (const auto &key, auto &value) {
        if (value == RestoreStatus::ASSETS_READY) {
            value = RestoreStatus::ALL_READY;
            ObjectStore::RadarReporter::ReportStage(std::string(__FUNCTION__), ObjectStore::DATA_RESTORE,
                ObjectStore::DATA_RECV, ObjectStore::RADAR_SUCCESS);
            callbacks_.ForEach([this, &data](uint32_t tokenId, const CallbackInfo& value) {
                DoNotify(tokenId, value, data, true);
                return false;
            });
        } else {
            value = RestoreStatus::DATA_READY;
            ObjectStore::RadarReporter::ReportStateStart(std::string(__FUNCTION__), ObjectStore::DATA_RESTORE,
                ObjectStore::DATA_RECV, ObjectStore::RADAR_SUCCESS, ObjectStore::START, saveInfo.bundleName);
            callbacks_.ForEach([this, &data](uint32_t tokenId, const CallbackInfo& value) {
                DoNotify(tokenId, value, data, false);
                return false;
            });
            WaitAssets(key, saveInfo, data);
        }
        return true;
    });
}

bool ObjectStoreManager::IsNeedMetaSync(const StoreMetaData &meta, const std::vector<std::string> &uuids)
{
    bool isAfterMeta = false;
    for (const auto &uuid : uuids) {
        auto metaData = meta;
        metaData.deviceId = uuid;
        CapMetaData capMeta;
        auto capKey = CapMetaRow::GetKeyFor(uuid);
        bool flag = !MetaDataManager::GetInstance().LoadMeta(std::string(capKey.begin(), capKey.end()), capMeta) ||
            !MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData);
        if (flag) {
            isAfterMeta = true;
            break;
        }
        auto [exist, mask] = DeviceMatrix::GetInstance().GetRemoteMask(uuid);
        if ((mask & DeviceMatrix::META_STORE_MASK) == DeviceMatrix::META_STORE_MASK) {
            isAfterMeta = true;
            break;
        }
        auto [existLocal, localMask] = DeviceMatrix::GetInstance().GetMask(uuid);
        if ((localMask & DeviceMatrix::META_STORE_MASK) == DeviceMatrix::META_STORE_MASK) {
            isAfterMeta = true;
            break;
        }
    }
    return isAfterMeta;
}

void ObjectStoreManager::NotifyDataChanged(const std::map<std::string, ObjectRecord>& data, const SaveInfo& saveInfo)
{
    for (auto const& [objectKey, results] : data) {
        restoreStatus_.ComputeIfAbsent(
            objectKey, [](const std::string& key) -> auto {
            return RestoreStatus::NONE;
        });
        ComputeStatus(objectKey, saveInfo, data);
    }
}

int32_t ObjectStoreManager::WaitAssets(const std::string& objectKey, const SaveInfo& saveInfo,
    const std::map<std::string, ObjectRecord>& data)
{
    if (executors_ == nullptr) {
        ZLOGE("executors_ is null");
        return OBJECT_INNER_ERROR;
    }
    auto taskId = executors_->Schedule(std::chrono::seconds(WAIT_TIME), [this, objectKey, data, saveInfo]() {
        ZLOGE("wait assets finisehd timeout, try pull assets, objectKey:%{public}s", objectKey.c_str());
        PullAssets(data, saveInfo);
        DoNotifyWaitAssetTimeout(objectKey);
    });

    objectTimer_.ComputeIfAbsent(
        objectKey, [taskId](const std::string& key) -> auto {
            return taskId;
    });
    return  OBJECT_SUCCESS;
}

void ObjectStoreManager::PullAssets(const std::map<std::string, ObjectRecord>& data, const SaveInfo& saveInfo)
{
    std::map<std::string, Assets> changedAssets;
    for (auto const& [objectId, result] : data) {
        changedAssets[objectId] = GetAssetsFromDBRecords(result);
    }
    for (const auto& [objectId, assets] : changedAssets) {
        std::string networkId = DmAdaper::GetInstance().ToNetworkID(saveInfo.sourceDeviceId);
        auto block = std::make_shared<BlockData<std::tuple<bool, bool>>>(WAIT_TIME, std::tuple{ true, true });
        ObjectAssetLoader::GetInstance().TransferAssetsAsync(std::atoi(GetCurrentUser().c_str()),
            saveInfo.bundleName, networkId, assets, [this, block](bool success) {
                block->SetValue({ false, success });
        });
        auto [timeout, success] = block->GetValue();
        ZLOGI("Pull assets end, timeout: %{public}d, success: %{public}d, size:%{public}zu, deviceId: %{public}s",
            timeout, success, assets.size(), DistributedData::Anonymous::Change(networkId).c_str());
    }
}

void ObjectStoreManager::NotifyAssetsRecvProgress(const std::string &objectKey, int32_t progress)
{
    assetsRecvProgress_.InsertOrAssign(objectKey, progress);
    std::list<sptr<ObjectProgressCallbackProxyBroker>> observers;
    bool flag = false;
    processCallbacks_.ForEach(
        [&objectKey, &observers, &flag](uint32_t tokenId, const ProgressCallbackInfo &value) {
            if (value.observers_.empty()) {
                flag = true;
                return false;
            }
            auto it = value.observers_.find(objectKey);
            if (it != value.observers_.end()) {
                observers.push_back(it->second);
            }
            return false;
        });
    if (flag) {
        std::lock_guard<std::mutex> lock(progressMutex_);
        progressInfo_.insert_or_assign(objectKey, progress);
    }
    for (auto &observer : observers) {
        if (observer == nullptr) {
            continue;
        }
        observer->Completed(progress);
    }
    if (!observers.empty() && progress == PROGRESS_MAX) {
        assetsRecvProgress_.Erase(objectKey);
        std::lock_guard<std::mutex> lock(progressMutex_);
        progressInfo_.erase(objectKey);
    }
}

void ObjectStoreManager::NotifyAssetsReady(
    const std::string &objectKey, const std::string &bundleName, const std::string &srcNetworkId)
{
    if (executors_ == nullptr) {
        ZLOGE("executors_ is nullptr");
        return;
    }
    restoreStatus_.ComputeIfAbsent(
        objectKey, [](const std::string& key) -> auto {
        return RestoreStatus::NONE;
    });
    restoreStatus_.Compute(objectKey, [this, &bundleName] (const auto &key, auto &value) {
        if (value == RestoreStatus::DATA_NOTIFIED) {
            value = RestoreStatus::ALL_READY;
            ObjectStore::RadarReporter::ReportStage(std::string(__FUNCTION__), ObjectStore::DATA_RESTORE,
                ObjectStore::ASSETS_RECV, ObjectStore::RADAR_SUCCESS);
            callbacks_.ForEach([this, key](uint32_t tokenId, const CallbackInfo& value) {
                DoNotifyAssetsReady(tokenId, value,  key, true);
                return false;
            });
        } else if (value == RestoreStatus::DATA_READY) {
            value = RestoreStatus::ALL_READY;
            ObjectStore::RadarReporter::ReportStage(std::string(__FUNCTION__), ObjectStore::DATA_RESTORE,
                ObjectStore::ASSETS_RECV, ObjectStore::RADAR_SUCCESS);
            auto [has, taskId] = objectTimer_.Find(key);
            if (has) {
                executors_->Remove(taskId);
                objectTimer_.Erase(key);
            }
        } else {
            value = RestoreStatus::ASSETS_READY;
            ObjectStore::RadarReporter::ReportStateStart(std::string(__FUNCTION__), ObjectStore::DATA_RESTORE,
                ObjectStore::ASSETS_RECV, ObjectStore::RADAR_SUCCESS, ObjectStore::START, bundleName);
        }
        return true;
    });
}

void ObjectStoreManager::NotifyAssetsStart(const std::string& objectKey, const std::string& srcNetworkId)
{
    restoreStatus_.ComputeIfAbsent(
        objectKey, [](const std::string& key) -> auto {
        return RestoreStatus::NONE;
    });
}

bool ObjectStoreManager::IsAssetKey(const std::string& key)
{
    return key.find(ObjectStore::ASSET_DOT) != std::string::npos;
}

bool ObjectStoreManager::IsAssetComplete(const ObjectRecord& result, const std::string& assetPrefix)
{
    if (result.find(assetPrefix + ObjectStore::NAME_SUFFIX) == result.end() ||
        result.find(assetPrefix + ObjectStore::URI_SUFFIX) == result.end() ||
        result.find(assetPrefix + ObjectStore::PATH_SUFFIX) == result.end() ||
        result.find(assetPrefix + ObjectStore::CREATE_TIME_SUFFIX) == result.end() ||
        result.find(assetPrefix + ObjectStore::MODIFY_TIME_SUFFIX) == result.end() ||
        result.find(assetPrefix + ObjectStore::SIZE_SUFFIX) == result.end()) {
        return false;
    }
    return true;
}

Assets ObjectStoreManager::GetAssetsFromDBRecords(const ObjectRecord& result)
{
    Assets assets{};
    std::set<std::string> assetKey;
    for (const auto& [key, value] : result) {
        std::string assetPrefix = key.substr(0, key.find(ObjectStore::ASSET_DOT));
        if (!IsAssetKey(key) || assetKey.find(assetPrefix) != assetKey.end() ||
            result.find(assetPrefix + ObjectStore::NAME_SUFFIX) == result.end() ||
            result.find(assetPrefix + ObjectStore::URI_SUFFIX) == result.end() ||
            result.find(assetPrefix + ObjectStore::MODIFY_TIME_SUFFIX) == result.end() ||
            result.find(assetPrefix + ObjectStore::SIZE_SUFFIX) == result.end()) {
            continue;
        }
        Asset asset;
        ObjectStore::StringUtils::BytesToStrWithType(
            result.find(assetPrefix + ObjectStore::NAME_SUFFIX)->second, asset.name);
        if (asset.name.find(ObjectStore::STRING_PREFIX) != std::string::npos) {
            asset.name = asset.name.substr(ObjectStore::STRING_PREFIX_LEN);
        }
        ObjectStore::StringUtils::BytesToStrWithType(
            result.find(assetPrefix + ObjectStore::URI_SUFFIX)->second, asset.uri);
        if (asset.uri.find(ObjectStore::STRING_PREFIX) != std::string::npos) {
            asset.uri = asset.uri.substr(ObjectStore::STRING_PREFIX_LEN);
        }
        ObjectStore::StringUtils::BytesToStrWithType(
            result.find(assetPrefix + ObjectStore::MODIFY_TIME_SUFFIX)->second, asset.modifyTime);
        if (asset.modifyTime.find(ObjectStore::STRING_PREFIX) != std::string::npos) {
            asset.modifyTime = asset.modifyTime.substr(ObjectStore::STRING_PREFIX_LEN);
        }
        ObjectStore::StringUtils::BytesToStrWithType(
            result.find(assetPrefix + ObjectStore::SIZE_SUFFIX)->second, asset.size);
        if (asset.size.find(ObjectStore::STRING_PREFIX) != std::string::npos) {
            asset.size = asset.size.substr(ObjectStore::STRING_PREFIX_LEN);
        }
        asset.hash = asset.modifyTime + "_" + asset.size;
        assets.push_back(asset);
        assetKey.insert(assetPrefix);
    }
    return assets;
}

void ObjectStoreManager::DoNotify(uint32_t tokenId, const CallbackInfo& value,
    const std::map<std::string, ObjectRecord>& data, bool allReady)
{
    for (const auto& observer : value.observers_) {
        auto it = data.find(observer.first);
        if (it == data.end()) {
            continue;
        }
        observer.second->Completed((*it).second, allReady);
        if (allReady) {
            restoreStatus_.Erase(observer.first);
            ObjectStore::RadarReporter::ReportStateFinished(std::string(__FUNCTION__), ObjectStore::DATA_RESTORE,
                ObjectStore::NOTIFY, ObjectStore::RADAR_SUCCESS, ObjectStore::FINISHED);
        } else {
            restoreStatus_.ComputeIfPresent(observer.first, [](const auto &key, auto &value) {
                value = RestoreStatus::DATA_NOTIFIED;
                return true;
            });
        }
    }
}

void ObjectStoreManager::DoNotifyAssetsReady(uint32_t tokenId, const CallbackInfo& value,
    const std::string& objectKey, bool allReady)
{
    if (executors_ == nullptr) {
        ZLOGE("executors_ is nullptr");
        return;
    }
    for (const auto& observer : value.observers_) {
        if (objectKey != observer.first) {
            continue;
        }
        observer.second->Completed(ObjectRecord(), allReady);
        if (allReady) {
            restoreStatus_.Erase(objectKey);
            ObjectStore::RadarReporter::ReportStateFinished(std::string(__FUNCTION__), ObjectStore::DATA_RESTORE,
                ObjectStore::NOTIFY, ObjectStore::RADAR_SUCCESS, ObjectStore::FINISHED);
        }
        auto [has, taskId] = objectTimer_.Find(objectKey);
        if (has) {
            executors_->Remove(taskId);
            objectTimer_.Erase(objectKey);
        }
    }
}

void ObjectStoreManager::DoNotifyWaitAssetTimeout(const std::string &objectKey)
{
    if (executors_ == nullptr) {
        ZLOGE("executors_ is nullptr");
        return;
    }
    ObjectStore::RadarReporter::ReportStageError(std::string(__FUNCTION__), ObjectStore::DATA_RESTORE,
        ObjectStore::ASSETS_RECV, ObjectStore::RADAR_FAILED, ObjectStore::TIMEOUT);
    callbacks_.ForEach([this, &objectKey](uint32_t tokenId, const CallbackInfo &value) {
        for (const auto& observer : value.observers_) {
            if (objectKey != observer.first) {
                continue;
            }
            observer.second->Completed(ObjectRecord(), true);
            restoreStatus_.Erase(objectKey);
            auto [has, taskId] = objectTimer_.Find(objectKey);
            if (has) {
                executors_->Remove(taskId);
                objectTimer_.Erase(objectKey);
            }
            ObjectStore::RadarReporter::ReportStateError(std::string(__FUNCTION__), ObjectStore::DATA_RESTORE,
                ObjectStore::NOTIFY, ObjectStore::RADAR_FAILED, ObjectStore::TIMEOUT, ObjectStore::FINISHED);
        }
        return false;
    });
}

void ObjectStoreManager::SetData(const std::string &dataDir, const std::string &userId)
{
    ZLOGI("enter, user: %{public}s", userId.c_str());
    InitKvStoreDelegateManager(userId);
    DistributedDB::KvStoreConfig kvStoreConfig { dataDir };
    auto kvStoreDelegateManager = GetKvStoreDelegateManager();
    if (kvStoreDelegateManager == nullptr) {
        ZLOGE("Kvstore delegate manager is nullptr.");
        return;
    }
    auto status = kvStoreDelegateManager->SetKvStoreConfig(kvStoreConfig);
    if (status != DistributedDB::OK) {
        ZLOGE("Set kvstore config failed, status: %{public}d", status);
        return;
    }
    userId_ = userId;
}

int32_t ObjectStoreManager::Open()
{
    auto kvStoreDelegateManager = GetKvStoreDelegateManager();
    if (kvStoreDelegateManager == nullptr) {
        ZLOGE("Kvstore delegate manager is not init");
        return OBJECT_INNER_ERROR;
    }
    std::unique_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        delegate_ = OpenObjectKvStore();
        if (delegate_ == nullptr) {
            ZLOGE("Open object kvstore failed");
            return OBJECT_DBSTATUS_ERROR;
        }
        syncCount_ = 1;
        ZLOGI("Open object kvstore success");
    } else {
        syncCount_++;
        ZLOGI("Object kvstore syncCount: %{public}d", syncCount_.load());
    }
    auto res = delegate_->SetProperty({ { Constant::TOKEN_ID, IPCSkeleton::GetCallingTokenID() } });
    if (res != DistributedDB::DBStatus::OK) {
        ZLOGW("set DB property fail, res:%{public}d", res);
    }
    return OBJECT_SUCCESS;
}

void ObjectStoreManager::ForceClose()
{
    auto kvStoreDelegateManager = GetKvStoreDelegateManager();
    if (kvStoreDelegateManager == nullptr) {
        ZLOGE("Kvstore delegate manager is nullptr.");
        return;
    }
    std::unique_lock<decltype(rwMutex_)> lock(rwMutex_, std::chrono::seconds(LOCK_TIMEOUT));
    if (delegate_ == nullptr) {
        return;
    }
    delegate_->UnRegisterObserver(objectDataListener_);
    auto status = kvStoreDelegateManager->CloseKvStore(delegate_);
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("CloseKvStore fail %{public}d", status);
        return;
    }
    delegate_ = nullptr;
    isSyncing_ = false;
    syncCount_ = 0;
}

void ObjectStoreManager::Close()
{
    int32_t taskCount = 0;
    {
        std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
        if (delegate_ == nullptr) {
            return;
        }
        taskCount = delegate_->GetTaskCount();
    }
    if (taskCount > 0 && syncCount_ == 1) {
        CloseAfterMinute();
        ZLOGW("Store is busy, close after a minute, task count: %{public}d", taskCount);
        return;
    }
    syncCount_--;
    ZLOGI("closed a store, syncCount = %{public}d", syncCount_.load());
    FlushClosedStore();
}

void ObjectStoreManager::SyncCompleted(
    const std::map<std::string, DistributedDB::DBStatus> &results, uint64_t sequenceId)
{
    std::string userId;
    SequenceSyncManager::Result result = SequenceSyncManager::GetInstance()->Process(sequenceId, results, userId);
    if (result == SequenceSyncManager::SUCCESS_USER_HAS_FINISHED && userId == userId_) {
        SetSyncStatus(false);
        FlushClosedStore();
    }
    for (const auto &item : results) {
        if (item.second == DistributedDB::DBStatus::OK) {
            ZLOGI("Sync data success, sequenceId: 0x%{public}" PRIx64 "", sequenceId);
            ObjectStore::RadarReporter::ReportStateFinished(std::string(__FUNCTION__), ObjectStore::SAVE,
                ObjectStore::SYNC_DATA, ObjectStore::RADAR_SUCCESS, ObjectStore::FINISHED);
        } else {
            ZLOGE("Sync data failed, sequenceId: 0x%{public}" PRIx64 ", status: %{public}d", sequenceId, item.second);
            ObjectStore::RadarReporter::ReportStateError(std::string(__FUNCTION__), ObjectStore::SAVE,
                ObjectStore::SYNC_DATA, ObjectStore::RADAR_FAILED, item.second, ObjectStore::FINISHED);
        }
    }
}

void ObjectStoreManager::FlushClosedStore()
{
    if (executors_ == nullptr) {
        ZLOGE("executors_ is nullptr");
        return;
    }
    auto kvStoreDelegateManager = GetKvStoreDelegateManager();
    if (kvStoreDelegateManager == nullptr) {
        ZLOGE("Kvstore delegate manager nullptr");
        return;
    }
    std::unique_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (!isSyncing_ && syncCount_ == 0 && delegate_ != nullptr) {
        ZLOGD("close store");
        delegate_->UnRegisterObserver(objectDataListener_);
        auto status = kvStoreDelegateManager->CloseKvStore(delegate_);
        if (status != DistributedDB::DBStatus::OK) {
            int timeOut = 1000;
            executors_->Schedule(std::chrono::milliseconds(timeOut), [this]() {
                FlushClosedStore();
            });
            ZLOGE("GetEntries fail %{public}d", status);
            return;
        }
        delegate_ = nullptr;
        objectDataListener_ = nullptr;
    }
}

void ObjectStoreManager::ProcessOldEntry(const std::string &appId)
{
    std::vector<DistributedDB::Entry> entries;
    {
        std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
        if (delegate_ == nullptr) {
            ZLOGE("delegate is nullptr.");
            return;
        }
        auto status = delegate_->GetEntries(std::vector<uint8_t>(appId.begin(), appId.end()), entries);
        if (status != DistributedDB::DBStatus::OK) {
            ZLOGE("Get old entries failed, bundleName: %{public}s, status %{public}d", appId.c_str(), status);
            return;
        }
    }
    std::map<std::string, int64_t> sessionIds;
    int64_t oldestTime = 0;
    std::string deleteKey;
    for (auto &item : entries) {
        std::string key(item.key.begin(), item.key.end());
        std::vector<std::string> splitKeys = SplitEntryKey(key);
        if (splitKeys.empty()) {
            continue;
        }
        std::string bundleName = splitKeys[BUNDLE_NAME_INDEX];
        std::string sessionId = splitKeys[SESSION_ID_INDEX];
        if (sessionIds.count(sessionId) == 0) {
            char *end = nullptr;
            sessionIds[sessionId] = strtol(splitKeys[TIME_INDEX].c_str(), &end, DECIMAL_BASE);
        }
        if (oldestTime == 0 || oldestTime > sessionIds[sessionId]) {
            oldestTime = sessionIds[sessionId];
            deleteKey = GetPrefixWithoutDeviceId(bundleName, sessionId);
        }
    }
    if (sessionIds.size() < MAX_OBJECT_SIZE_PER_APP) {
        return;
    }
    int32_t result = RevokeSaveToStore(deleteKey);
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Delete old entries failed, deleteKey: %{public}s, status: %{public}d", deleteKey.c_str(), result);
        return;
    }
    ZLOGI("Delete old entries success, deleteKey: %{public}s", deleteKey.c_str());
}

int32_t ObjectStoreManager::SaveToStore(const std::string &appId, const std::string &sessionId,
    const std::string &toDeviceId, const ObjectRecord &data)
{
    ProcessOldEntry(appId);
    RevokeSaveToStore(GetPropertyPrefix(appId, sessionId, toDeviceId));
    std::string timestamp = std::to_string(GetSecondsSince1970ToNow());
    std::string prefix = GetPropertyPrefix(appId, sessionId, toDeviceId) + timestamp + SEPERATOR;
    DistributedDB::Entry saveInfoEntry;
    std::string saveInfoKey = prefix + SAVE_INFO;
    saveInfoEntry.key = std::vector<uint8_t>(saveInfoKey.begin(), saveInfoKey.end());
    SaveInfo saveInfo(appId, sessionId, DmAdaper::GetInstance().GetLocalDevice().udid, toDeviceId, timestamp);
    std::string saveInfoValue = DistributedData::Serializable::Marshall(saveInfo);
    saveInfoEntry.value = std::vector<uint8_t>(saveInfoValue.begin(), saveInfoValue.end());
    std::vector<DistributedDB::Entry> entries;
    entries.emplace_back(saveInfoEntry);
    for (const auto &item : data) {
        DistributedDB::Entry entry;
        std::string key = GetPropertyPrefix(appId, sessionId, toDeviceId) + timestamp + SEPERATOR + item.first;
        entry.key = std::vector<uint8_t>(key.begin(), key.end());
        entry.value = item.second;
        entries.emplace_back(entry);
    }
    {
        std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
        if (delegate_ == nullptr) {
            ZLOGE("delegate is nullptr.");
            return E_DB_ERROR;
        }
        auto status = delegate_->PutBatch(entries);
        if (status != DistributedDB::DBStatus::OK) {
            ZLOGE("PutBatch failed, bundleName: %{public}s, sessionId: %{public}s, dstNetworkId: %{public}s, "
                  "status: %{public}d",
                appId.c_str(), Anonymous::Change(sessionId).c_str(), Anonymous::Change(toDeviceId).c_str(), status);
            return status;
        }
    }
    ZLOGI("PutBatch success, bundleName: %{public}s, sessionId: %{public}s, dstNetworkId: %{public}s, "
          "count: %{public}zu",
        appId.c_str(), Anonymous::Change(sessionId).c_str(), Anonymous::Change(toDeviceId).c_str(), entries.size());
    return OBJECT_SUCCESS;
}

int32_t ObjectStoreManager::SyncOnStore(
    const std::string &prefix, const std::vector<std::string> &deviceList, SyncCallBack &callback)
{
    std::vector<std::string> syncDevices;
    for (auto const &device : deviceList) {
        if (device == LOCAL_DEVICE) {
            ZLOGI("Save to local, do not need sync, prefix: %{public}s", Anonymous::Change(prefix).c_str());
            callback({{LOCAL_DEVICE, OBJECT_SUCCESS}});
            return OBJECT_SUCCESS;
        }
        syncDevices.emplace_back(DmAdaper::GetInstance().GetUuidByNetworkId(device));
    }
    if (syncDevices.empty()) {
        ZLOGI("Device list is empty, prefix: %{public}s", Anonymous::Change(prefix).c_str());
        callback(std::map<std::string, int32_t>());
        return OBJECT_SUCCESS;
    }
    uint64_t sequenceId = SequenceSyncManager::GetInstance()->AddNotifier(userId_, callback);

    int32_t id = AccountDelegate::GetInstance()->GetUserByToken(IPCSkeleton::GetCallingFullTokenID());
    StoreMetaData meta = StoreMetaData(std::to_string(id), Bootstrap::GetInstance().GetProcessLabel(),
        DistributedObject::ObjectCommon::OBJECTSTORE_DB_STOREID);
    auto uuids = DmAdapter::GetInstance().ToUUID(syncDevices);
    bool isNeedMetaSync = IsNeedMetaSync(meta, uuids);
    if (!isNeedMetaSync) {
        return DoSync(prefix, syncDevices, sequenceId);
    }
    bool result = MetaDataManager::GetInstance().Sync(uuids, [this, prefix, syncDevices, sequenceId](auto &results) {
        auto status = DoSync(prefix, syncDevices, sequenceId);
        ZLOGI("Store sync after meta sync end, status:%{public}d", status);
    });
    ZLOGI("prefix:%{public}s, meta sync end, result:%{public}d", Anonymous::Change(prefix).c_str(), result);
    return result ? OBJECT_SUCCESS : DoSync(prefix, syncDevices, sequenceId);
}

int32_t ObjectStoreManager::DoSync(const std::string &prefix, const std::vector<std::string> &deviceList,
    uint64_t sequenceId)
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("db store was closed.");
        return E_DB_ERROR;
    }
    DistributedDB::Query dbQuery = DistributedDB::Query::Select();
    dbQuery.PrefixKey(std::vector<uint8_t>(prefix.begin(), prefix.end()));
    ZLOGI("Start sync data, sequenceId: 0x%{public}" PRIx64 "", sequenceId);
    auto status = delegate_->Sync(deviceList, DistributedDB::SyncMode::SYNC_MODE_PUSH_ONLY,
        [this, sequenceId](const std::map<std::string, DistributedDB::DBStatus> &devicesMap) {
            ZLOGI("Sync data finished, sequenceId: 0x%{public}" PRIx64 "", sequenceId);
            std::map<std::string, DistributedDB::DBStatus> result;
            for (auto &item : devicesMap) {
                result[DmAdaper::GetInstance().ToNetworkID(item.first)] = item.second;
            }
            SyncCompleted(result, sequenceId);
        }, dbQuery, false);
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("Sync data failed, prefix: %{public}s, sequenceId: 0x%{public}" PRIx64 ", status: %{public}d",
            Anonymous::Change(prefix).c_str(), sequenceId, status);
        std::string tmp;
        SequenceSyncManager::GetInstance()->DeleteNotifier(sequenceId, tmp);
        return status;
    }
    SetSyncStatus(true);
    return OBJECT_SUCCESS;
}

int32_t ObjectStoreManager::SetSyncStatus(bool status)
{
    isSyncing_ = status;
    return OBJECT_SUCCESS;
}

int32_t ObjectStoreManager::RevokeSaveToStore(const std::string &prefix)
{
    std::vector<DistributedDB::Entry> entries;
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("db store was closed.");
        return E_DB_ERROR;
    }
    auto status = delegate_->GetEntries(std::vector<uint8_t>(prefix.begin(), prefix.end()), entries);
    if (status == DistributedDB::DBStatus::NOT_FOUND) {
        ZLOGI("Get entries empty, prefix: %{public}s", Anonymous::Change(prefix).c_str());
        return OBJECT_SUCCESS;
    }
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("Get entries failed, prefix: %{public}s, status: %{public}d", Anonymous::Change(prefix).c_str(), status);
        return DB_ERROR;
    }
    std::vector<std::vector<uint8_t>> keys;
    std::for_each(entries.begin(), entries.end(), [&keys](const DistributedDB::Entry &entry) {
        keys.emplace_back(entry.key);
    });
    if (keys.empty()) {
        return OBJECT_SUCCESS;
    }
    status = delegate_->DeleteBatch(keys);
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("Delete entries failed, prefix: %{public}s, status: %{public}d", Anonymous::Change(prefix).c_str(),
            status);
        return DB_ERROR;
    }
    ZLOGI("Delete entries success, prefix: %{public}s, count: %{public}zu", Anonymous::Change(prefix).c_str(),
        keys.size());
    return OBJECT_SUCCESS;
}

int32_t ObjectStoreManager::RetrieveFromStore(const std::string &appId, const std::string &sessionId,
    ObjectRecord &results)
{
    std::vector<DistributedDB::Entry> entries;
    std::string prefix = GetPrefixWithoutDeviceId(appId, sessionId);
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("db store was closed.");
        return E_DB_ERROR;
    }
    auto status = delegate_->GetEntries(std::vector<uint8_t>(prefix.begin(), prefix.end()), entries);
    if (status == DistributedDB::DBStatus::NOT_FOUND) {
        ZLOGI("Get entries empty, prefix: %{public}s, status: %{public}d", Anonymous::Change(prefix).c_str(), status);
        return KEY_NOT_FOUND;
    }
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("Get entries failed, prefix: %{public}s, status: %{public}d", Anonymous::Change(prefix).c_str(), status);
        return DB_ERROR;
    }
    ZLOGI("GetEntries success,prefix:%{public}s,count:%{public}zu", Anonymous::Change(prefix).c_str(), entries.size());
    for (const auto &entry : entries) {
        std::string key(entry.key.begin(), entry.key.end());
        if (key.find(SAVE_INFO) != std::string::npos) {
            continue;
        }
        auto splitKeys = SplitEntryKey(key);
        if (!splitKeys.empty()) {
            results[splitKeys[PROPERTY_NAME_INDEX]] = entry.value;
        }
    }
    return OBJECT_SUCCESS;
}

ObjectStoreManager::SaveInfo::SaveInfo(const std::string &bundleName, const std::string &sessionId,
    const std::string &sourceDeviceId, const std::string &targetDeviceId, const std::string &timestamp)
    : bundleName(bundleName), sessionId(sessionId), sourceDeviceId(sourceDeviceId), targetDeviceId(targetDeviceId),
    timestamp(timestamp) {}

bool ObjectStoreManager::SaveInfo::Marshal(json &node) const
{
    SetValue(node[GET_NAME(bundleName)], bundleName);
    SetValue(node[GET_NAME(sessionId)], sessionId);
    SetValue(node[GET_NAME(sourceDeviceId)], sourceDeviceId);
    SetValue(node[GET_NAME(targetDeviceId)], targetDeviceId);
    SetValue(node[GET_NAME(timestamp)], timestamp);
    return true;
}

bool ObjectStoreManager::SaveInfo::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(bundleName), bundleName);
    GetValue(node, GET_NAME(sessionId), sessionId);
    GetValue(node, GET_NAME(sourceDeviceId), sourceDeviceId);
    GetValue(node, GET_NAME(targetDeviceId), targetDeviceId);
    GetValue(node, GET_NAME(timestamp), timestamp);
    return true;
}

std::string ObjectStoreManager::SaveInfo::ToPropertyPrefix()
{
    if (bundleName.empty() || sessionId.empty() || sourceDeviceId.empty() || targetDeviceId.empty() ||
        timestamp.empty()) {
        return "";
    }
    return bundleName + SEPERATOR + sessionId + SEPERATOR + sourceDeviceId + SEPERATOR + targetDeviceId + SEPERATOR +
        timestamp + SEPERATOR;
}

std::vector<std::string> ObjectStoreManager::SplitEntryKey(const std::string &key)
{
    std::smatch match;
    std::regex timeRegex(TIME_REGEX);
    if (!std::regex_search(key, match, timeRegex)) {
        ZLOGW("Format error, key.size = %{public}zu", key.size());
        return {};
    }
    auto timePos = match.position();
    std::string fromTime = key.substr(timePos + 1);
    std::string beforeTime = key.substr(0, timePos);

    size_t targetDevicePos = beforeTime.find_last_of(SEPERATOR);
    if (targetDevicePos == std::string::npos) {
        ZLOGW("Format error, key.size = %{public}zu", key.size());
        return {};
    }
    std::string targetDevice = beforeTime.substr(targetDevicePos + 1);
    std::string beforeTargetDevice = beforeTime.substr(0, targetDevicePos);

    size_t sourceDeviceUdidPos = beforeTargetDevice.find_last_of(SEPERATOR);
    if (sourceDeviceUdidPos == std::string::npos) {
        ZLOGW("Format error, key.size = %{public}zu", key.size());
        return {};
    }
    std::string sourceDeviceUdid = beforeTargetDevice.substr(sourceDeviceUdidPos + 1);
    std::string beforeSourceDeviceUdid = beforeTargetDevice.substr(0, sourceDeviceUdidPos);

    size_t sessionIdPos = beforeSourceDeviceUdid.find_last_of(SEPERATOR);
    if (sessionIdPos == std::string::npos) {
        ZLOGW("Format error, key.size = %{public}zu", key.size());
        return {};
    }
    std::string sessionId = beforeSourceDeviceUdid.substr(sessionIdPos + 1);
    std::string bundleName = beforeSourceDeviceUdid.substr(0, sessionIdPos);

    size_t propertyNamePos = fromTime.find_first_of(SEPERATOR);
    if (propertyNamePos == std::string::npos) {
        ZLOGW("Format error, key.size = %{public}zu", key.size());
        return {};
    }
    std::string propertyName = fromTime.substr(propertyNamePos + 1);
    std::string time = fromTime.substr(0, propertyNamePos);

    return { bundleName, sessionId, sourceDeviceUdid, targetDevice, time, propertyName };
}

std::string ObjectStoreManager::GetCurrentUser()
{
    std::vector<int> users;
    auto ret = AccountDelegate::GetInstance()->QueryUsers(users);
    if (!ret || users.empty()) {
        ZLOGE("failed to query os accounts, ret:%{public}d", ret);
        return "";
    }
    return std::to_string(users[0]);
}

void ObjectStoreManager::SaveUserToMeta()
{
    ZLOGD("start.");
    std::string userId = GetCurrentUser();
    if (userId.empty()) {
        return;
    }
    std::string appId = DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    ObjectUserMetaData userMeta;
    if (!MetaDataManager::GetInstance().LoadMeta(userMeta.GetKey(), userMeta, true) || userId != userMeta.userId) {
        userMeta.userId = userId;
        if (!MetaDataManager::GetInstance().SaveMeta(ObjectUserMetaData::GetKey(), userMeta, true)) {
            ZLOGE("userMeta save failed, userId:%{public}s", userId.c_str());
        }
    }
}

void ObjectStoreManager::CloseAfterMinute()
{
    if (executors_ == nullptr) {
        ZLOGE("executors_ is nullptr.");
        return;
    }
    executors_->Schedule(std::chrono::minutes(INTERVAL), [this]() {
        Close();
    });
}

void ObjectStoreManager::SetThreadPool(std::shared_ptr<ExecutorPool> executors)
{
    executors_ = executors;
}

uint64_t SequenceSyncManager::AddNotifier(const std::string &userId, SyncCallBack &callback)
{
    std::lock_guard<std::mutex> lock(notifierLock_);
    uint64_t sequenceId = KvStoreUtils::GenerateSequenceId();
    userIdSeqIdRelations_[userId].emplace_back(sequenceId);
    seqIdCallbackRelations_[sequenceId] = callback;
    return sequenceId;
}

SequenceSyncManager::Result SequenceSyncManager::Process(
    uint64_t sequenceId, const std::map<std::string, DistributedDB::DBStatus> &results, std::string &userId)
{
    std::lock_guard<std::mutex> lock(notifierLock_);
    if (seqIdCallbackRelations_.count(sequenceId) == 0) {
        ZLOGE("not exist");
        return ERR_SID_NOT_EXIST;
    }
    std::map<std::string, int32_t> syncResults;
    for (const auto &item : results) {
        syncResults[item.first] = item.second == DistributedDB::DBStatus::OK ? 0 : -1;
    }
    seqIdCallbackRelations_[sequenceId](syncResults);
    ZLOGD("end complete");
    return DeleteNotifierNoLock(sequenceId, userId);
}

SequenceSyncManager::Result SequenceSyncManager::DeleteNotifier(uint64_t sequenceId, std::string &userId)
{
    std::lock_guard<std::mutex> lock(notifierLock_);
    if (seqIdCallbackRelations_.count(sequenceId) == 0) {
        ZLOGE("not exist");
        return ERR_SID_NOT_EXIST;
    }
    return DeleteNotifierNoLock(sequenceId, userId);
}

SequenceSyncManager::Result SequenceSyncManager::DeleteNotifierNoLock(uint64_t sequenceId, std::string &userId)
{
    seqIdCallbackRelations_.erase(sequenceId);
    auto userIdIter = userIdSeqIdRelations_.begin();
    while (userIdIter != userIdSeqIdRelations_.end()) {
        auto sIdIter = std::find(userIdIter->second.begin(), userIdIter->second.end(), sequenceId);
        if (sIdIter != userIdIter->second.end()) {
            userIdIter->second.erase(sIdIter);
            if (userIdIter->second.empty()) {
                ZLOGD("finished user callback, userId = %{public}s", userIdIter->first.c_str());
                userId = userIdIter->first;
                userIdSeqIdRelations_.erase(userIdIter);
                return SUCCESS_USER_HAS_FINISHED;
            } else {
                ZLOGD(" finished a callback but user not finished, userId = %{public}s", userIdIter->first.c_str());
                return SUCCESS_USER_IN_USE;
            }
        }
        userIdIter++;
    }
    return SUCCESS_USER_HAS_FINISHED;
}

int32_t ObjectStoreManager::BindAsset(const uint32_t tokenId, const std::string& appId, const std::string& sessionId,
    ObjectStore::Asset& asset, ObjectStore::AssetBindInfo& bindInfo)
{
    auto snapshotKey = appId + SEPERATOR + sessionId;
    snapshots_.ComputeIfAbsent(
        snapshotKey, [](const std::string& key) -> auto {
            return std::make_shared<ObjectSnapshot>();
        });
    auto storeKey = appId + SEPERATOR + bindInfo.storeName;
    bindSnapshots_.ComputeIfAbsent(
        storeKey, [](const std::string& key) -> auto {
            return std::make_shared<std::map<std::string, std::shared_ptr<Snapshot>>>();
        });
    auto snapshots = snapshots_.Find(snapshotKey).second;
    bindSnapshots_.Compute(storeKey, [this, &asset, snapshots] (const auto &key, auto &value) {
        value->emplace(std::pair{asset.uri, snapshots});
        return true;
    });

    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != TOKEN_HAP) {
        ZLOGE("TokenType is not TOKEN_HAP, token:0x%{public}x, bundleName:%{public}s", tokenId, appId.c_str());
        return GeneralError::E_ERROR;
    }
    HapTokenInfo tokenInfo;
    auto status = AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (status != RET_SUCCESS) {
        ZLOGE("token:0x%{public}x, result:%{public}d, bundleName:%{public}s", tokenId, status, appId.c_str());
        return GeneralError::E_ERROR;
    }
    StoreInfo storeInfo;
    storeInfo.bundleName = appId;
    storeInfo.tokenId = tokenId;
    storeInfo.instanceId = tokenInfo.instIndex;
    storeInfo.user = tokenInfo.userID;
    storeInfo.storeName = bindInfo.storeName;

    snapshots_.Compute(snapshotKey, [this, &asset, &bindInfo, &storeInfo] (const auto &key, auto &value) {
        value->BindAsset(ValueProxy::Convert(std::move(asset)), ConvertBindInfo(bindInfo), storeInfo);
        return true;
    });
    return OBJECT_SUCCESS;
}

DistributedData::AssetBindInfo ObjectStoreManager::ConvertBindInfo(ObjectStore::AssetBindInfo& bindInfo)
{
    return DistributedData::AssetBindInfo{
        .storeName = std::move(bindInfo.storeName),
        .tableName = std::move(bindInfo.tableName),
        .primaryKey = ValueProxy::Convert(std::move(bindInfo.primaryKey)),
        .field = std::move(bindInfo.field),
        .assetName = std::move(bindInfo.assetName),
    };
}

int32_t ObjectStoreManager::OnAssetChanged(const uint32_t tokenId, const std::string& appId,
    const std::string& sessionId, const std::string& deviceId, const ObjectStore::Asset& asset)
{
    const int32_t userId = AccountDelegate::GetInstance()->GetUserByToken(tokenId);
    auto objectAsset = asset;
    Asset dataAsset =  ValueProxy::Convert(std::move(objectAsset));
    auto snapshotKey = appId + SEPERATOR + sessionId;
    int32_t res = OBJECT_SUCCESS;
    bool exist = snapshots_.ComputeIfPresent(snapshotKey,
        [&res, &dataAsset, &deviceId](const std::string &key, std::shared_ptr<Snapshot> snapshot) {
            if (snapshot != nullptr) {
                res = snapshot->OnDataChanged(dataAsset, deviceId); // needChange
            }
            return snapshot != nullptr;
        });
    if (exist) {
        return res;
    }

    auto block = std::make_shared<BlockData<std::tuple<bool, bool>>>(WAIT_TIME, std::tuple{ true, true });
    ObjectAssetLoader::GetInstance().TransferAssetsAsync(userId, appId, deviceId, { dataAsset }, [block](bool ret) {
        block->SetValue({ false, ret });
    });
    auto [timeout, success] = block->GetValue();
    if (timeout || !success) {
        ZLOGE("transfer failed, timeout: %{public}d, success: %{public}d, name: %{public}s, deviceId: %{public}s ",
            timeout, success, asset.name.c_str(), DistributedData::Anonymous::Change(deviceId).c_str());
        return OBJECT_INNER_ERROR;
    }
    return OBJECT_SUCCESS;
}

ObjectStoreManager::UriToSnapshot ObjectStoreManager::GetSnapShots(const std::string& bundleName,
    const std::string& storeName)
{
    auto storeKey = bundleName + SEPERATOR + storeName;
    bindSnapshots_.ComputeIfAbsent(
        storeKey, [](const std::string& key) -> auto {
            return std::make_shared<std::map<std::string, std::shared_ptr<Snapshot>>>();
        });
    return bindSnapshots_.Find(storeKey).second;
}

void ObjectStoreManager::DeleteSnapshot(const std::string& bundleName, const std::string& sessionId)
{
    auto snapshotKey = bundleName + SEPERATOR + sessionId;
    auto snapshot = snapshots_.Find(snapshotKey).second;
    if (snapshot == nullptr) {
        ZLOGD("Not find snapshot, don't need delete");
        return;
    }
    bindSnapshots_.ForEach([snapshot](auto& key, auto& value) {
        for (auto pos = value->begin(); pos != value->end();) {
            if (pos->second == snapshot) {
                pos = value->erase(pos);
            } else {
                ++pos;
            }
        }
        return true;
    });
    snapshots_.Erase(snapshotKey);
}

int32_t ObjectStoreManager::AutoLaunchStore()
{
    int32_t status = Open();
    if (status != OBJECT_SUCCESS) {
        ZLOGE("Open fail %{public}d", status);
        return status;
    }
    CloseAfterMinute();
    ZLOGI("Auto launch, close after a minute");
    return OBJECT_SUCCESS;
}

std::shared_ptr<DistributedDB::KvStoreDelegateManager> ObjectStoreManager::GetKvStoreDelegateManager()
{
    std::shared_lock<decltype(rwKvStoreMutex_)> lock(rwKvStoreMutex_);
    return kvStoreDelegateManager_;
}

void ObjectStoreManager::InitKvStoreDelegateManager(const std::string &userId)
{
    std::unique_lock<decltype(rwKvStoreMutex_)> lock(rwKvStoreMutex_);
    kvStoreDelegateManager_ = std::make_shared<DistributedDB::KvStoreDelegateManager>(
        DistributedData::Bootstrap::GetInstance().GetProcessLabel(), userId);
}
} // namespace DistributedObject
} // namespace OHOS
