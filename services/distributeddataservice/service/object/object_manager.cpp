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
#include <set>
#include <utils/anonymous.h>

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "block_data.h"
#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "common/bytes.h"
#include "common/string_utils.h"
#include "datetime_ex.h"
#include "distributed_file_daemon_manager.h"
#include "ipc_skeleton.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "object_radar_reporter.h"

namespace OHOS {
namespace DistributedObject {
using namespace OHOS::DistributedKv;
using namespace Security::AccessToken;
using StoreMetaData = OHOS::DistributedData::StoreMetaData;
using AccountDelegate = DistributedKv::AccountDelegate;
using OHOS::DistributedKv::AccountDelegate;
using Account = OHOS::DistributedKv::AccountDelegate;
using AccessTokenKit = Security::AccessToken::AccessTokenKit;
using ValueProxy = OHOS::DistributedData::ValueProxy;
using DistributedFileDaemonManager = Storage::DistributedFile::DistributedFileDaemonManager;
constexpr const char *SAVE_INFO = "p_###SAVEINFO###";
ObjectStoreManager::ObjectStoreManager()
{
    ZLOGI("ObjectStoreManager construct");
    RegisterAssetsLister();
}

ObjectStoreManager::~ObjectStoreManager()
{
    ZLOGI("ObjectStoreManager destroy");
    if (objectAssetsSendListener_ != nullptr) {
        delete objectAssetsSendListener_;
        objectAssetsSendListener_ = nullptr;
    }
    if (objectAssetsRecvListener_ != nullptr) {
        auto status = DistributedFileDaemonManager::GetInstance().UnRegisterAssetCallback(objectAssetsRecvListener_);
        if (status != DistributedDB::DBStatus::OK) {
            ZLOGE("UnRegister assetsRecvListener err %{public}d", status);
        }
        delete objectAssetsRecvListener_;
        objectAssetsRecvListener_ = nullptr;
    }
}

DistributedDB::KvStoreNbDelegate *ObjectStoreManager::OpenObjectKvStore()
{
    DistributedDB::KvStoreNbDelegate *store = nullptr;
    DistributedDB::KvStoreNbDelegate::Option option;
    option.createDirByStoreIdOnly = true;
    option.syncDualTupleMode = true;
    option.secOption = { DistributedDB::S1, DistributedDB::ECE };
    if (objectDataListener_ == nullptr) {
        objectDataListener_ = new ObjectDataListener();
    }
    ZLOGD("start GetKvStore");
    kvStoreDelegateManager_->GetKvStore(ObjectCommon::OBJECTSTORE_DB_STOREID, option,
        [&store, this](DistributedDB::DBStatus dbStatus, DistributedDB::KvStoreNbDelegate *kvStoreNbDelegate) {
            if (dbStatus != DistributedDB::DBStatus::OK) {
                ZLOGE("GetKvStore fail %{public}d", dbStatus);
                return;
            }
            ZLOGI("GetKvStore successsfully");
            store = kvStoreNbDelegate;
            std::vector<uint8_t> tmpKey;
            DistributedDB::DBStatus status = store->RegisterObserver(tmpKey,
                DistributedDB::ObserverMode::OBSERVER_CHANGES_FOREIGN,
                objectDataListener_);
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
    const std::map<std::string, std::vector<uint8_t>> &data, const std::string &deviceId,
    sptr<IRemoteObject> callback)
{
    auto proxy = iface_cast<ObjectSaveCallbackProxy>(callback);
    if (deviceId.size() == 0) {
        ZLOGE("deviceId empty");
        proxy->Completed(std::map<std::string, int32_t>());
        return INVALID_ARGUMENT;
    }
    int32_t result = Open();
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Open failed, errCode = %{public}d", result);
        proxy->Completed(std::map<std::string, int32_t>());
        RADAR_REPORT(ObjectStore::SAVE, ObjectStore::SAVE_TO_STORE, ObjectStore::RADAR_FAILED,
            ObjectStore::ERROR_CODE, ObjectStore::GETKV_FAILED, ObjectStore::BIZ_STATE, ObjectStore::FINISHED);
        return STORE_NOT_OPEN;
    }

    ZLOGD("start SaveToStore");
    SaveUserToMeta();
    result = SaveToStore(appId, sessionId, deviceId, data);
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Save failed, errCode = %{public}d", result);
        Close();
        proxy->Completed(std::map<std::string, int32_t>());
        return result;
    }
    SyncCallBack tmp = [proxy, appId, sessionId, deviceId, this](const std::map<std::string, int32_t> &results) {
        proxy->Completed(results);
        ProcessSyncCallback(results, appId, sessionId, deviceId);
    };
    ZLOGD("start SyncOnStore");
    std::vector<std::string> deviceList = {deviceId};
    result = SyncOnStore(GetPropertyPrefix(appId, sessionId, deviceId), deviceList, tmp);
    if (result != OBJECT_SUCCESS) {
        ZLOGE("sync failed, errCode = %{public}d", result);
        proxy->Completed(std::map<std::string, int32_t>());
        RADAR_REPORT(ObjectStore::SAVE, ObjectStore::SAVE_TO_STORE, ObjectStore::RADAR_FAILED,
            ObjectStore::ERROR_CODE, result, ObjectStore::BIZ_STATE, ObjectStore::FINISHED);
    }
    result = PushAssets(std::stoi(GetCurrentUser()), appId, sessionId, data, deviceId);
    Close();
    return result;
}

int32_t ObjectStoreManager::PushAssets(int32_t userId, const std::string &appId, const std::string &sessionId,
    const std::map<std::string, std::vector<uint8_t>> &data, const std::string &deviceId)
{
    Assets assets = GetAssetsFromDBRecords(data);
    if (assets.empty() || data.find(ObjectStore::FIELDS_PREFIX + ObjectStore::DEVICEID_KEY) == data.end()) {
        return OBJECT_SUCCESS;
    }
    sptr<AssetObj> assetObj = new AssetObj();
    assetObj->dstBundleName_ = appId;
    assetObj->srcBundleName_ = appId;
    assetObj->dstNetworkId_ = deviceId;
    assetObj->sessionId_ = sessionId;
    for (const auto& asset : assets) {
        assetObj->uris_.push_back(asset.uri);
    }
    if (objectAssetsSendListener_ == nullptr) {
        objectAssetsSendListener_ = new ObjectAssetsSendListener();
    }
    auto status =  ObjectAssetLoader::GetInstance()->PushAsset(userId, assetObj, objectAssetsSendListener_);
    return status;
}

int32_t ObjectStoreManager::RevokeSave(
    const std::string &appId, const std::string &sessionId, sptr<IRemoteObject> callback)
{
    auto proxy = iface_cast<ObjectRevokeSaveCallbackProxy>(callback);
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
    auto proxy = iface_cast<ObjectRetrieveCallbackProxy>(callback);
    ZLOGI("enter");
    int32_t result = Open();
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Open failed, errCode = %{public}d", result);
        proxy->Completed(std::map<std::string, std::vector<uint8_t>>(), false);
        return ObjectStore::GETKV_FAILED;
    }
    std::map<std::string, std::vector<uint8_t>> results{};
    int32_t status = RetrieveFromStore(bundleName, sessionId, results);
    if (status != OBJECT_SUCCESS) {
        ZLOGI("Retrieve failed, status = %{public}d", status);
        Close();
        proxy->Completed(std::map<std::string, std::vector<uint8_t>>(), false);
        return status;
    }
    bool allReady = false;
    Assets assets = GetAssetsFromDBRecords(results);
    if (assets.empty() || results.find(ObjectStore::FIELDS_PREFIX + ObjectStore::DEVICEID_KEY) == results.end()) {
        allReady = true;
    } else {
        auto objectKey = bundleName + sessionId;
        restoreStatus_.ComputeIfAbsent(
            objectKey, [](const std::string& key) -> auto {
            return RestoreStatus::NONE;
        });
        if (restoreStatus_.Find(objectKey).second == RestoreStatus::ALL_READY) {
            allReady = true;
        }
    }
    // delete local data
    status = RevokeSaveToStore(GetPrefixWithoutDeviceId(bundleName, sessionId));
    if (status != OBJECT_SUCCESS) {
        ZLOGE("revoke save failed, status = %{public}d", status);
        Close();
        proxy->Completed(std::map<std::string, std::vector<uint8_t>>(), false);
        return status;
    }
    Close();
    proxy->Completed(results, allReady);
    RADAR_REPORT(ObjectStore::CREATE, ObjectStore::RESTORE, ObjectStore::RADAR_SUCCESS, ObjectStore::BIZ_STATE,
        ObjectStore::FINISHED);
    return status;
}

int32_t ObjectStoreManager::Clear()
{
    ZLOGI("enter");
    std::string userId = GetCurrentUser();
    if (userId.empty()) {
        return OBJECT_INNER_ERROR;
    }
    std::vector<StoreMetaData> metaData;
    std::string appId = DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    std::string metaKey = GetMetaUserIdKey(userId, appId);
    if (!DistributedData::MetaDataManager::GetInstance().LoadMeta(metaKey, metaData, true)) {
        ZLOGE("no store of %{public}s", appId.c_str());
        return OBJECT_STORE_NOT_FOUND;
    }
    for (const auto &storeMeta : metaData) {
        if (storeMeta.storeType < StoreMetaData::StoreType::STORE_OBJECT_BEGIN
            || storeMeta.storeType > StoreMetaData::StoreType::STORE_OBJECT_END) {
            continue;
        }
        if (storeMeta.user == userId) {
            ZLOGI("user is same, not need to change, mate user:%{public}s::user:%{public}s.",
                storeMeta.user.c_str(), userId.c_str());
            return OBJECT_SUCCESS;
        }
    }
    ZLOGD("user is change, need to change");
    int32_t result = Open();
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Open failed, errCode = %{public}d", result);
        return STORE_NOT_OPEN;
    }
    result = RevokeSaveToStore("");
    Close();
    return result;
}

int32_t ObjectStoreManager::DeleteByAppId(const std::string &appId)
{
    ZLOGI("enter, %{public}s", appId.c_str());
    int32_t result = Open();
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Open failed, errCode = %{public}d", result);
        return STORE_NOT_OPEN;
    }
    result = RevokeSaveToStore(appId);
    if (result != OBJECT_SUCCESS) {
        ZLOGE("RevokeSaveToStore failed");
    }
    Close();

    std::string userId = GetCurrentUser();
    if (userId.empty()) {
        return OBJECT_INNER_ERROR;
    }
    std::string metaKey = GetMetaUserIdKey(userId, appId);
    DistributedData::MetaDataManager::GetInstance().DelMeta(metaKey, true);
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
    auto proxy = iface_cast<ObjectChangeCallbackProxy>(callback);
    std::string prefix = bundleName + sessionId;
    callbacks_.Compute(tokenId, ([pid, &proxy, &prefix](const uint32_t key, CallbackInfo &value) {
        if (value.pid != pid) {
            value = CallbackInfo { pid };
        }
        value.observers_.insert_or_assign(prefix, proxy);
        return !value.observers_.empty();
    }));
}

void ObjectStoreManager::UnregisterRemoteCallback(const std::string &bundleName, pid_t pid, uint32_t tokenId,
                                                  const std::string &sessionId)
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

void ObjectStoreManager::NotifyChange(std::map<std::string, std::vector<uint8_t>> &changedData)
{
    ZLOGI("OnChange start, size:%{public}zu", changedData.size());
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> data;
    bool hasAsset = false;
    SaveInfo saveInfo;
    for (const auto &[key, value] : changedData) {
        if (key.find(SAVE_INFO) != std::string::npos) {
            DistributedData::Serializable::Unmarshall(std::string(value.begin(), value.end()), saveInfo);
            break;
        }
    }
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
            if (splitKeys.empty()) {
                continue;
            }
            std::string prefix = splitKeys[BUNDLE_NAME_INDEX] + splitKeys[SESSION_ID_INDEX];
            std::string propertyName = splitKeys[PROPERTY_NAME_INDEX];
            data[prefix].insert_or_assign(propertyName, item.second);
            if (IsAssetKey(propertyName)) {
                hasAsset = true;
            }
        }
    }
    if (!hasAsset) {
        callbacks_.ForEach([this, &data](uint32_t tokenId, const CallbackInfo& value) {
            DoNotify(tokenId, value, data, true); // no asset, data ready means all ready
            return false;
        });
        return;
    }
    NotifyDataChanged(data);
    SaveUserToMeta();
}

void ObjectStoreManager::ComputeStatus(const std::string& objectKey,
    const std::map<std::string, std::map<std::string, std::vector<uint8_t>>>& data)
{
    restoreStatus_.Compute(objectKey, [this, &data] (const auto &key, auto &value) {
            if (value == RestoreStatus::ASSETS_READY) {
                value = RestoreStatus::ALL_READY;
                callbacks_.ForEach([this, &data](uint32_t tokenId, const CallbackInfo& value) {
                    DoNotify(tokenId, value, data, true);
                    return false;
                });
            } else {
                value = RestoreStatus::DATA_READY;
                    callbacks_.ForEach([this, &data](uint32_t tokenId, const CallbackInfo& value) {
                    DoNotify(tokenId, value, data, false);
                    return false;
                });
                WaitAssets(key);
            }
            return true;
    });
}

void ObjectStoreManager::NotifyDataChanged(std::map<std::string, std::map<std::string, std::vector<uint8_t>>>& data)
{
    for (auto const& [objectKey, results] : data) {
        restoreStatus_.ComputeIfAbsent(
            objectKey, [](const std::string& key) -> auto {
            return RestoreStatus::NONE;
        });
        ComputeStatus(objectKey, data);
    }
}

int32_t ObjectStoreManager::WaitAssets(const std::string& objectKey)
{
    auto taskId = executors_->Schedule(std::chrono::seconds(WAIT_TIME), [this, objectKey]() {
        ZLOGE("wait assets finisehd timeout, objectKey:%{public}s", objectKey.c_str());
        RADAR_REPORT(ObjectStore::DATA_RESTORE, ObjectStore::ASSETS_RECV, ObjectStore::RADAR_FAILED,
            ObjectStore::ERROR_CODE, ObjectStore::TIMEOUT);
        NotifyAssetsReady(objectKey);
    });

    objectTimer_.ComputeIfAbsent(
        objectKey, [taskId](const std::string& key) -> auto {
            return taskId;
    });
    return  OBJECT_SUCCESS;
}

void ObjectStoreManager::NotifyAssetsReady(const std::string& objectKey, const std::string& srcNetworkId)
{
    restoreStatus_.ComputeIfAbsent(
        objectKey, [](const std::string& key) -> auto {
        return RestoreStatus::NONE;
    });
    restoreStatus_.Compute(objectKey, [this] (const auto &key, auto &value) {
        if (value == RestoreStatus::DATA_READY) {
            value = RestoreStatus::ALL_READY;
            callbacks_.ForEach([this, key](uint32_t tokenId, const CallbackInfo& value) {
                DoNotifyAssetsReady(tokenId, value,  key, true);
                return false;
            });
        } else {
            value = RestoreStatus::ASSETS_READY;
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

bool ObjectStoreManager::IsAssetComplete(const std::map<std::string, std::vector<uint8_t>>& result,
    const std::string& assetPrefix)
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

Assets ObjectStoreManager::GetAssetsFromDBRecords(const std::map<std::string, std::vector<uint8_t>>& result)
{
    Assets assets{};
    std::set<std::string> assetKey;
    for (const auto& [key, value] : result) {
        std::string assetPrefix = key.substr(0, key.find(ObjectStore::ASSET_DOT));
        if (!IsAssetKey(key) || assetKey.find(assetPrefix) != assetKey.end() ||
            result.find(assetPrefix + ObjectStore::NAME_SUFFIX) == result.end() ||
            result.find(assetPrefix + ObjectStore::URI_SUFFIX) == result.end()) {
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
    const std::map<std::string, std::map<std::string, std::vector<uint8_t>>>& data, bool allReady)
{
    for (const auto& observer : value.observers_) {
        auto it = data.find(observer.first);
        if (it == data.end()) {
            continue;
        }
        observer.second->Completed((*it).second, allReady);
        if (allReady) {
            restoreStatus_.Erase(observer.first);
        }
    }
}

void ObjectStoreManager::DoNotifyAssetsReady(uint32_t tokenId, const CallbackInfo& value,
    const std::string& objectKey, bool allReady)
{
    for (const auto& observer : value.observers_) {
        if (objectKey != observer.first) {
            continue;
        }
        observer.second->Completed(std::map<std::string, std::vector<uint8_t>>(), allReady);
        if (allReady) {
            restoreStatus_.Erase(objectKey);
        }
        auto [has, taskId] = objectTimer_.Find(objectKey);
        if (has) {
            executors_->Remove(taskId);
            objectTimer_.Erase(objectKey);
        }
    }
}

void ObjectStoreManager::SetData(const std::string &dataDir, const std::string &userId)
{
    ZLOGI("enter %{public}s", dataDir.c_str());
    kvStoreDelegateManager_ =
        new DistributedDB::KvStoreDelegateManager(DistributedData::Bootstrap::GetInstance().GetProcessLabel(), userId);
    DistributedDB::KvStoreConfig kvStoreConfig { dataDir };
    kvStoreDelegateManager_->SetKvStoreConfig(kvStoreConfig);
    userId_ = userId;
}

int32_t ObjectStoreManager::Open()
{
    if (kvStoreDelegateManager_ == nullptr) {
        ZLOGE("not init");
        return OBJECT_INNER_ERROR;
    }
    std::lock_guard<std::recursive_mutex> lock(kvStoreMutex_);
    if (delegate_ == nullptr) {
        ZLOGI("open store");
        delegate_ = OpenObjectKvStore();
        if (delegate_ == nullptr) {
            ZLOGE("open failed,please check DB status");
            return OBJECT_DBSTATUS_ERROR;
        }
        syncCount_ = 1;
    } else {
        syncCount_++;
        ZLOGI("syncCount = %{public}d", syncCount_);
    }
    return OBJECT_SUCCESS;
}

void ObjectStoreManager::Close()
{
    std::lock_guard<std::recursive_mutex> lock(kvStoreMutex_);
    if (delegate_ == nullptr) {
        return;
    }
    syncCount_--;
    ZLOGI("closed a store, syncCount = %{public}d", syncCount_);
    FlushClosedStore();
}

void ObjectStoreManager::SyncCompleted(
    const std::map<std::string, DistributedDB::DBStatus> &results, uint64_t sequenceId)
{
    std::string userId;
    SequenceSyncManager::Result result = SequenceSyncManager::GetInstance()->Process(sequenceId, results, userId);
    if (result == SequenceSyncManager::SUCCESS_USER_HAS_FINISHED && userId == userId_) {
        std::lock_guard<std::recursive_mutex> lock(kvStoreMutex_);
        SetSyncStatus(false);
        FlushClosedStore();
    }
}

void ObjectStoreManager::FlushClosedStore()
{
    std::lock_guard<std::recursive_mutex> lock(kvStoreMutex_);
    if (!isSyncing_ && syncCount_ == 0 && delegate_ != nullptr) {
        ZLOGD("close store");
        auto status = kvStoreDelegateManager_->CloseKvStore(delegate_);
        if (status != DistributedDB::DBStatus::OK) {
            int timeOut = 1000;
            executors_->Schedule(std::chrono::milliseconds(timeOut), [this]() {
                FlushClosedStore();
            });
            ZLOGE("GetEntries fail %{public}d", status);
            return;
        }
        delegate_ = nullptr;
        if (objectDataListener_ != nullptr) {
            delete objectDataListener_;
            objectDataListener_ = nullptr;
        }
    }
}

void ObjectStoreManager::ProcessOldEntry(const std::string &appId)
{
    std::vector<DistributedDB::Entry> entries;
    auto status = delegate_->GetEntries(std::vector<uint8_t>(appId.begin(), appId.end()), entries);
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("GetEntries fail %{public}d", status);
        return;
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
    ZLOGI("app object is full, delete oldest one %{public}s", deleteKey.c_str());
    int32_t result = RevokeSaveToStore(deleteKey);
    if (result != OBJECT_SUCCESS) {
        ZLOGE("RevokeSaveToStore fail %{public}d", result);
        return;
    }
}

int32_t ObjectStoreManager::SaveToStore(const std::string &appId, const std::string &sessionId,
    const std::string &toDeviceId, const std::map<std::string, std::vector<uint8_t>> &data)
{
    ProcessOldEntry(appId);
    RevokeSaveToStore(GetPropertyPrefix(appId, sessionId, toDeviceId));
    std::string timestamp = std::to_string(GetSecondsSince1970ToNow());
    std::vector<DistributedDB::Entry> entries;
    DistributedDB::Entry saveInfoEntry;
    std::string saveInfoKey = GetPropertyPrefix(appId, sessionId, toDeviceId) + timestamp + SEPERATOR + SAVE_INFO;
    saveInfoEntry.key = std::vector<uint8_t>(saveInfoKey.begin(), saveInfoKey.end());
    SaveInfo saveInfo(appId, sessionId, DmAdaper::GetInstance().GetLocalDevice().udid, toDeviceId, timestamp);
    std::string saveInfoValue = DistributedData::Serializable::Marshall(saveInfo);
    saveInfoEntry.value = std::vector<uint8_t>(saveInfoValue.begin(), saveInfoValue.end());
    entries.emplace_back(saveInfoEntry);
    for (auto &item : data) {
        DistributedDB::Entry entry;
        std::string tmp = GetPropertyPrefix(appId, sessionId, toDeviceId) + timestamp + SEPERATOR + item.first;
        entry.key = std::vector<uint8_t>(tmp.begin(), tmp.end());
        entry.value = item.second;
        entries.emplace_back(entry);
    }
    auto status = delegate_->PutBatch(entries);
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("putBatch fail %{public}d", status);
        RADAR_REPORT(ObjectStore::SAVE, ObjectStore::SAVE_TO_STORE, ObjectStore::RADAR_FAILED,
            ObjectStore::ERROR_CODE, status, ObjectStore::BIZ_STATE, ObjectStore::FINISHED);
    }
    return status;
}

int32_t ObjectStoreManager::SyncOnStore(
    const std::string &prefix, const std::vector<std::string> &deviceList, SyncCallBack &callback)
{
    std::vector<std::string> syncDevices;
    for (auto &device : deviceList) {
        // save to local, do not need sync
        if (device == LOCAL_DEVICE) {
            ZLOGI("save to local successful");
            std::map<std::string, int32_t> result;
            result[LOCAL_DEVICE] = OBJECT_SUCCESS;
            callback(result);
            return OBJECT_SUCCESS;
        }
        syncDevices.emplace_back(DmAdaper::GetInstance().GetUuidByNetworkId(device));
    }
    if (!syncDevices.empty()) {
        uint64_t sequenceId = SequenceSyncManager::GetInstance()->AddNotifier(userId_, callback);
        DistributedDB::Query dbQuery = DistributedDB::Query::Select();
        dbQuery.PrefixKey(std::vector<uint8_t>(prefix.begin(), prefix.end()));
        ZLOGD("start sync");
        auto status = delegate_->Sync(
            syncDevices, DistributedDB::SyncMode::SYNC_MODE_PUSH_ONLY,
            [this, sequenceId](const std::map<std::string, DistributedDB::DBStatus> &devicesMap) {
                ZLOGI("objectstore sync finished");
                std::map<std::string, DistributedDB::DBStatus> result;
                for (auto &item : devicesMap) {
                    result[DmAdaper::GetInstance().ToNetworkID(item.first)] = item.second;
                }
                SyncCompleted(result, sequenceId);
            },
            dbQuery, false);
        if (status != DistributedDB::DBStatus::OK) {
            ZLOGE("sync error %{public}d", status);
            std::string tmp;
            SequenceSyncManager::GetInstance()->DeleteNotifier(sequenceId, tmp);
            return status;
        }
        SetSyncStatus(true);
    } else {
        ZLOGI("single device");
        callback(std::map<std::string, int32_t>());
    }
    return OBJECT_SUCCESS;
}

int32_t ObjectStoreManager::SetSyncStatus(bool status)
{
    std::lock_guard<std::recursive_mutex> lock(kvStoreMutex_);
    isSyncing_ = status;
    return OBJECT_SUCCESS;
}

int32_t ObjectStoreManager::RevokeSaveToStore(const std::string &prefix)
{
    std::vector<DistributedDB::Entry> entries;
    auto status = delegate_->GetEntries(std::vector<uint8_t>(prefix.begin(), prefix.end()), entries);
    if (status == DistributedDB::DBStatus::NOT_FOUND) {
        ZLOGI("not found entry");
        return OBJECT_SUCCESS;
    }
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("GetEntries failed, status = %{public}d", status);
        return DB_ERROR;
    }
    std::vector<std::vector<uint8_t>> keys;
    std::for_each(
        entries.begin(), entries.end(), [&keys](const DistributedDB::Entry &entry) { keys.emplace_back(entry.key); });
    if (!keys.empty()) {
        status = delegate_->DeleteBatch(keys);
        if (status != DistributedDB::DBStatus::OK) {
            ZLOGE("DeleteBatch failed, status = %{public}d", status);
            return DB_ERROR;
        }
    }
    return OBJECT_SUCCESS;
}

int32_t ObjectStoreManager::RetrieveFromStore(
    const std::string &appId, const std::string &sessionId, std::map<std::string, std::vector<uint8_t>> &results)
{
    std::vector<DistributedDB::Entry> entries;
    std::string prefix = GetPrefixWithoutDeviceId(appId, sessionId);
    auto status = delegate_->GetEntries(std::vector<uint8_t>(prefix.begin(), prefix.end()), entries);
    if (status == DistributedDB::DBStatus::NOT_FOUND) {
        ZLOGW("key not found, status = %{public}d", status);
        return KEY_NOT_FOUND;
    }
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("GetEntries failed, status = %{public}d", status);
        return DB_ERROR;
    }
    ZLOGI("GetEntries successfully");
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
    AccountDelegate::GetInstance()->QueryUsers(users);
    if (users.empty()) {
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
    StoreMetaData userMeta;
    userMeta.storeId = DistributedObject::ObjectCommon::OBJECTSTORE_DB_STOREID;
    userMeta.user = userId;
    userMeta.storeType = ObjectDistributedType::OBJECT_SINGLE_VERSION;
    std::string userMetaKey = GetMetaUserIdKey(userId, appId);
    auto saved = DistributedData::MetaDataManager::GetInstance().SaveMeta(userMetaKey, userMeta, true);
    if (!saved) {
        ZLOGE("userMeta save failed");
    }
}

void ObjectStoreManager::CloseAfterMinute()
{
    executors_->Schedule(std::chrono::minutes(INTERVAL), std::bind(&ObjectStoreManager::Close, this));
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
    for (auto &item : results) {
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
    const int32_t userId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(tokenId);
    auto objectAsset = asset;
    Asset dataAsset =  ValueProxy::Convert(std::move(objectAsset));
    auto snapshotKey = appId + SEPERATOR + sessionId;
    int32_t res = OBJECT_SUCCESS;
    bool exist = snapshots_.ComputeIfPresent(snapshotKey,
        [&res, &dataAsset, &deviceId](std::string key, std::shared_ptr<Snapshot> snapshot) {
            if (snapshot != nullptr) {
                res = snapshot->OnDataChanged(dataAsset, deviceId); // needChange
            }
            return snapshot != nullptr;
        });
    if (exist) {
        return res;
    }

    auto block = std::make_shared<BlockData<std::tuple<bool, bool>>>(WAIT_TIME, std::tuple{ true, true });
    ObjectAssetLoader::GetInstance()->TransferAssetsAsync(userId, appId, deviceId, { dataAsset }, [block](bool ret) {
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
} // namespace DistributedObject
} // namespace OHOS
