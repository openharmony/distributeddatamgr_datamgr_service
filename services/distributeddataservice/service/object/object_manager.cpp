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

#include "ipc_skeleton.h"
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "datetime_ex.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "object_data_listener.h"
#include "object_asset_loader.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "account/account_delegate.h"
#include "common/bytes.h"
#include "common/string_utils.h"

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
ObjectStoreManager::ObjectStoreManager() {}

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
    }
    Close();
    return result;
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
        proxy->Completed(std::map<std::string, std::vector<uint8_t>>());
        return STORE_NOT_OPEN;
    }

    std::map<std::string, std::vector<uint8_t>> results;
    int32_t status = RetrieveFromStore(bundleName, sessionId, results);
    if (status != OBJECT_SUCCESS) {
        ZLOGE("Retrieve failed, status = %{public}d", status);
        Close();
        proxy->Completed(std::map<std::string, std::vector<uint8_t>>());
        return status;
    }
    TransferAssets(results, tokenId, bundleName);
    // delete local data
    status = RevokeSaveToStore(GetPrefixWithoutDeviceId(bundleName, sessionId));
    if (status != OBJECT_SUCCESS) {
        ZLOGE("revoke save failed, status = %{public}d", status);
        Close();
        proxy->Completed(std::map<std::string, std::vector<uint8_t>>());
        return status;
    }
    Close();
    proxy->Completed(results);
    return status;
}

void ObjectStoreManager::TransferAssets(
    std::map<std::string, std::vector<uint8_t>>& results, int32_t tokenId, const std::string& bundleName)
{
    std::map<std::string, Asset> assets;
    std::string deviceId;

    for (auto const&[key, value] : results) {
        if (key.find(ObjectStore::ASSET_DOT) == std::string::npos) {
            if (key == (ObjectStore::FIELDS_PREFIX + ObjectStore::DEVICEID_KEY)) {
                ObjectStore::StringUtils::BytesToStrWithType(value, deviceId);
            }
        } else {
            std::string assetKey = key.substr(0, key.find(ObjectStore::ASSET_DOT));
            auto it = assets.find(assetKey);
            if (it == assets.end()) {
                Asset asset;
                ObjectStore::StringUtils::BytesToStrWithType(results[assetKey+ObjectStore::NAME_SUFFIX], asset.name);
                asset.name = asset.name.substr(ObjectStore::STRING_PREFIX_LEN);
                ObjectStore::StringUtils::BytesToStrWithType(results[assetKey+ObjectStore::URI_SUFFIX], asset.uri);
                asset.uri = asset.uri.substr(ObjectStore::STRING_PREFIX_LEN);
                assets[assetKey] = asset;
            }
        }
    }
    if (!assets.empty()) {
        const int32_t userId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(tokenId);
        for (auto&[key, asset] : assets) {
            if (!ObjectAssetLoader::GetInstance()->Transfer(userId, bundleName, deviceId, asset)) {
                ZLOGE("Transfer fail, userId: %{public}d, bundleName: %{public}s, networkId: %{public}s, asset name : "
                    "%{public}s", userId, bundleName.c_str(), deviceId.c_str(), asset.name.c_str());
            }
        }
    }
}

void ObjectStoreManager::TransferAssets2(
    std::map<std::string, std::vector<uint8_t>>& results, int32_t userId, const std::string& bundleName)
{
    std::map<std::string, Asset> assets;
    std::string deviceId;

    for (auto const&[key, value] : results) {
        if (key.find(ObjectStore::ASSET_DOT) == std::string::npos) {
            if (key == (ObjectStore::FIELDS_PREFIX + ObjectStore::DEVICEID_KEY)) {
                ObjectStore::StringUtils::BytesToStrWithType(value, deviceId);
            }
        } else {
            std::string assetKey = key.substr(0, key.find(ObjectStore::ASSET_DOT));
            auto it = assets.find(assetKey);
            if (it == assets.end()) {
                Asset asset;
                ObjectStore::StringUtils::BytesToStrWithType(results[assetKey+ObjectStore::NAME_SUFFIX], asset.name);
                asset.name = asset.name.substr(ObjectStore::STRING_PREFIX_LEN);
                ObjectStore::StringUtils::BytesToStrWithType(results[assetKey+ObjectStore::URI_SUFFIX], asset.uri);
                asset.uri = asset.uri.substr(ObjectStore::STRING_PREFIX_LEN);
                assets[assetKey] = asset;
            }
        }
    }
    if (!assets.empty()) {
        for (auto&[key, asset] : assets) {
            if (!ObjectAssetLoader::GetInstance()->Transfer(userId, bundleName, deviceId, asset)) {
                ZLOGE("Transfer fail, userId: %{public}d, bundleName: %{public}s, networkId: %{public}s, asset name : "
                    "%{public}s", userId, bundleName.c_str(), deviceId.c_str(), asset.name.c_str());
            }
        }
    }
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
    ZLOGD("ObjectStoreManager::NotifyChange start");
    SaveUserToMeta();
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> data;
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> transferData;
    for (const auto &item : changedData) {
        std::string prefix = GetBundleName(item.first) + GetSessionId(item.first);
        std::string propertyName = GetPropertyName(item.first);
        data[prefix].insert_or_assign(std::move(propertyName), std::move(item.second));

        std::string bundleName = GetBundleName(item.first);
        transferData[bundleName].insert_or_assign(std::move(propertyName), std::move(item.second));
    }

    const int32_t userId = std::stoi(GetCurrentUser());
    for (auto item : transferData) {
        std::string bundleName = item.first;
        auto data2 = item.second;
        TransferAssets2(data2, userId, bundleName);
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
        std::string id = GetSessionId(key);
        if (sessionIds.count(id) == 0) {
            sessionIds[id] = GetTime(key);
        }
        if (oldestTime == 0 || oldestTime > sessionIds[id]) {
            oldestTime = sessionIds[id];
            deleteKey = GetPrefixWithoutDeviceId(appId, id);
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
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("GetEntries failed, status = %{public}d", status);
        return DB_ERROR;
    }
    ZLOGI("GetEntries successfully");
    std::for_each(entries.begin(), entries.end(), [&results, this](const DistributedDB::Entry &entry) {
        std::string key(entry.key.begin(), entry.key.end());
        results[GetPropertyName(key)] = entry.value;
    });
    return OBJECT_SUCCESS;
}

void ObjectStoreManager::ProcessKeyByIndex(std::string &key, uint8_t index)
{
    std::size_t pos;
    uint8_t i = 0;
    do {
        pos = key.find(SEPERATOR);
        if (pos == std::string::npos) {
            return;
        }
        key.erase(0, pos + 1);
        i++;
    } while (i < index);
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

std::string ObjectStoreManager::GetPropertyName(const std::string &key)
{
    std::string result = key;
    ProcessKeyByIndex(result, 5); // property name is after 5 '_'
    return result;
}

std::string ObjectStoreManager::GetSessionId(const std::string &key)
{
    std::string result = key;
    ProcessKeyByIndex(result, 1); // sessionId is after 1 '_'
    auto pos = result.find(SEPERATOR);
    if (pos == std::string::npos) {
        return result;
    }
    result.erase(pos);
    return result;
}

int64_t ObjectStoreManager::GetTime(const std::string &key)
{
    std::string result = key;
    std::size_t pos;
    int8_t i = 0;
    do {
        pos = result.find(SEPERATOR);
        result.erase(0, pos + 1);
        i++;
    } while (pos != std::string::npos && i < 4); // time is after 4 '_'
    pos = result.find(SEPERATOR);
    result.erase(pos);
    char *end = nullptr;
    return std::strtol(result.c_str(), &end, DECIMAL_BASE);
}

void ObjectStoreManager::CloseAfterMinute()
{
    executors_->Schedule(std::chrono::minutes(INTERVAL), std::bind(&ObjectStoreManager::Close, this));
}

std::string ObjectStoreManager::GetBundleName(const std::string &key)
{
    std::size_t pos = key.find(SEPERATOR);
    if (pos == std::string::npos) {
        return std::string();
    }
    std::string result = key;
    result.erase(pos);
    return result;
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
    bindSnapshots_[storeKey]->emplace(std::pair{asset.uri, snapshots_[snapshotKey]});

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

    snapshots_[snapshotKey]->BindAsset(ValueProxy::Convert(std::move(asset)), ConvertBindInfo(bindInfo), storeInfo);
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
    if (snapshots_.Contains(snapshotKey) && snapshots_[snapshotKey]->IsBoundAsset(dataAsset)) {
        return snapshots_[snapshotKey]->OnDataChanged(dataAsset, deviceId); // needChange
    }

    bool isSuccess = ObjectAssetLoader::GetInstance()->Transfer(userId, appId, deviceId, dataAsset);
    if (isSuccess) {
        return OBJECT_SUCCESS;
    } else {
        return OBJECT_INNER_ERROR;
    }
}

ObjectStoreManager::UriToSnapshot ObjectStoreManager::GetSnapShots(const std::string& bundleName,
    const std::string& storeName)
{
    auto storeKey = bundleName + SEPERATOR + storeName;
    bindSnapshots_.ComputeIfAbsent(
        storeKey, [](const std::string& key) -> auto {
            return std::make_shared<std::map<std::string, std::shared_ptr<Snapshot>>>();
        });
    return bindSnapshots_[storeKey];
}

void ObjectStoreManager::DeleteSnapshot(const std::string& bundleName, const std::string& sessionId)
{
    auto snapshotKey = bundleName + SEPERATOR + sessionId;
    if (!snapshots_.Contains(snapshotKey)) {
        ZLOGD("Not find snapshot, don't need delete");
        return;
    }
    auto snapshot = snapshots_[snapshotKey];
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
