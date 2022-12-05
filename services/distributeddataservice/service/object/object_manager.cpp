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

#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "datetime_ex.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "object_data_listener.h"

namespace OHOS {
namespace DistributedObject {
ObjectStoreManager::ObjectStoreManager() : timer_("CloseRetryTimer")
{
    timer_.Setup();
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

void ObjectStoreManager::ProcessSyncCallback(const std::map<std::string, int32_t> &results, const std::string &appId,
    const std::string &sessionId, const std::string &deviceId)
{
    if (results.empty() || results.find(LOCAL_DEVICE) != results.end()) {
        return;
    }
    int32_t result = Open();
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Open objectStore DB failed,please check DB errCode, errCode = %{public}d", result);
        return;
    }
    // delete local data
    result = RevokeSaveToStore(GetPropertyPrefix(appId, sessionId, deviceId));
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Save to store failed,please check DB status, status = %{public}d", result);
    }
    Close();
    return;
}

int32_t ObjectStoreManager::Save(const std::string &appId, const std::string &sessionId,
    const std::map<std::string, std::vector<uint8_t>> &data, const std::string &deviceId,
    sptr<IObjectSaveCallback> &callback)
{
    if (deviceId.size() == 0) {
        ZLOGE("deviceId empty");
        callback->Completed(std::map<std::string, int32_t>());
        return INVALID_ARGUMENT;
    }
    int32_t result = Open();
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Open objectStore DB failed,please check errCode, errCode = %{public}d", result);
        callback->Completed(std::map<std::string, int32_t>());
        return STORE_NOT_OPEN;
    }

    ZLOGD("start SaveToStore");
    result = SaveToStore(appId, sessionId, deviceId, data);
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Save to store failed, please check DB errCode, errCode = %{public}d", result);
        Close();
        callback->Completed(std::map<std::string, int32_t>());
        return result;
    }
    SyncCallBack tmp = [callback, appId, sessionId, deviceId, this](const std::map<std::string, int32_t> &results) {
        callback->Completed(results);
        ProcessSyncCallback(results, appId, sessionId, deviceId);
    };
    ZLOGD("start SyncOnStore");
    std::vector<std::string> deviceList = {deviceId};
    result = SyncOnStore(GetPropertyPrefix(appId, sessionId, deviceId), deviceList, tmp);
    if (result != OBJECT_SUCCESS) {
        ZLOGI("sync on store failed,please check DB errCode, errCode = %{public}d", result);
        callback->Completed(std::map<std::string, int32_t>());
    }
    Close();
    return result;
}

int32_t ObjectStoreManager::RevokeSave(
    const std::string &appId, const std::string &sessionId, sptr<IObjectRevokeSaveCallback> &callback)
{
    int32_t result = Open();
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Open objectStore DB failed,please check errCode, errCode = %{public}d", result);
        callback->Completed(STORE_NOT_OPEN);
        return STORE_NOT_OPEN;
    }

    result = RevokeSaveToStore(GetPrefixWithoutDeviceId(appId, sessionId));
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Save to store failed,please check DB errCode, errCode = %{public}d", result);
        Close();
        callback->Completed(result);
        return result;
    }
    std::vector<std::string> deviceList;
    auto deviceInfos = AppDistributedKv::CommunicationProvider::GetInstance().GetRemoteDevices();
    std::for_each(deviceInfos.begin(), deviceInfos.end(),
        [&deviceList](AppDistributedKv::DeviceInfo info) { deviceList.emplace_back(info.networkId); });
    if (!deviceList.empty()) {
        SyncCallBack tmp = [callback](const std::map<std::string, int32_t> &results) {
            ZLOGI("revoke save finished");
            callback->Completed(OBJECT_SUCCESS);
        };
        result = SyncOnStore(GetPropertyPrefix(appId, sessionId), deviceList, tmp);
        if (result != OBJECT_SUCCESS) {
            ZLOGE("sync on store failed,please check DB errCode, errCode = %{public}d", result);
            callback->Completed(result);
        }
    } else {
        callback->Completed(OBJECT_SUCCESS);
    };
    Close();
    return result;
}

int32_t ObjectStoreManager::Retrieve(
    const std::string &appId, const std::string &sessionId, sptr<IObjectRetrieveCallback> callback)
{
    ZLOGI("enter");
    int32_t result = Open();
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Open objectStore DB failed,please check DB errCode, errCode = %{public}d", result);
        callback->Completed(std::map<std::string, std::vector<uint8_t>>());
        return STORE_NOT_OPEN;
    }

    std::map<std::string, std::vector<uint8_t>> results;
    int32_t status = RetrieveFromStore(appId, sessionId, results);
    if (status != OBJECT_SUCCESS) {
        ZLOGE("Retrieve from store failed,please check DB status, status = %{public}d", status);
        Close();
        callback->Completed(std::map<std::string, std::vector<uint8_t>>());
        return status;
    }
    // delete local data
    status = RevokeSaveToStore(GetPrefixWithoutDeviceId(appId, sessionId));
    if (status != OBJECT_SUCCESS) {
        ZLOGE("revoke save to store failed,please check DB status, status = %{public}d", status);
        Close();
        callback->Completed(std::map<std::string, std::vector<uint8_t>>());
        return status;
    }
    Close();
    callback->Completed(results);
    return status;
}

int32_t ObjectStoreManager::Clear()
{
    ZLOGI("enter");
    int32_t result = Open();
    if (result != OBJECT_SUCCESS) {
        ZLOGE("Open objectStore DB failed,please check DB status");
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
        ZLOGE("Open objectStore DB failed,please check DB errCode, errCode = %{public}d", result);
        return STORE_NOT_OPEN;
    }
    result = RevokeSaveToStore(appId);
    if (result != OBJECT_SUCCESS) {
        ZLOGE("RevokeSaveToStore failed");
    }
    Close();
    return result;
}

void ObjectStoreManager::RegisterRemoteCallback(const std::string &bundleName, const std::string &sessionId,
                                                pid_t pid, uint32_t tokenId,
                                                sptr <IObjectChangeCallback> &callback)
{
    if (bundleName.empty() || sessionId.empty()) {
        ZLOGD("ObjectStoreManager::RegisterRemoteCallback empty");
        return;
    }
    ZLOGD("ObjectStoreManager::RegisterRemoteCallback start");
    std::string prefix = bundleName + sessionId;
    callbacks_.Compute(tokenId, ([pid, &callback, &prefix](const uint32_t key, CallbackInfo &value) {
        if (value.pid != pid) {
            value = CallbackInfo { pid };
        }
        value.observers_.insert_or_assign(prefix, callback);
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
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> data;
    for (const auto &item : changedData) {
        std::string prefix = GetBundleName(item.first) + GetSessionId(item.first);
        std::string propertyName = GetPropertyName(item.first);
        data[prefix].insert_or_assign(std::move(propertyName), std::move(item.second));
    }

    callbacks_.ForEach([&data](uint32_t tokenId, CallbackInfo &value) {
        for (const auto &observer : value.observers_) {
            auto it = data.find(observer.first);
            if (it == data.end()) {
                continue;
            }
            observer.second->Completed((*it).second);
        }
        return false;
    });
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
            timer_.Register([this]() { FlushClosedStore(); }, 1000, true); // retry after 1000ms
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
        syncDevices.emplace_back(AppDistributedKv::CommunicationProvider::GetInstance().GetUuidByNodeId(device));
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
                    result[AppDistributedKv::CommunicationProvider::GetInstance().ToNodeId(item.first)] = item.second;
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
        return OBJECT_SUCCESS;
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
        ZLOGE("GetEntries failed,please check DB status");
        return DB_ERROR;
    }
    std::vector<std::vector<uint8_t>> keys;
    std::for_each(
        entries.begin(), entries.end(), [&keys](const DistributedDB::Entry &entry) { keys.emplace_back(entry.key); });
    if (!keys.empty()) {
        status = delegate_->DeleteBatch(keys);
        if (status != DistributedDB::DBStatus::OK) {
            ZLOGE("DeleteBatch failed,please check DB status, status = %{public}d", status);
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
        ZLOGE("GetEntries failed,please check DB status, status = %{public}d", status);
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
    scheduler_.At(std::chrono::steady_clock::now() + std::chrono::minutes(INTERVAL),
                  std::bind(&ObjectStoreManager::Close, this));
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
} // namespace DistributedObject
} // namespace OHOS
