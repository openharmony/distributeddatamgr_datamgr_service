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
#include "kvstore_utils.h"
#include "log_print.h"
#include "query_helper.h"

namespace OHOS {
namespace DistributedObject {
ObjectStoreManager::ObjectStoreManager()
{
}

DistributedDB::KvStoreNbDelegate *ObjectStoreManager::OpenObjectKvStore()
{
    DistributedDB::KvStoreNbDelegate *store = nullptr;
    DistributedDB::KvStoreNbDelegate::Option option = {
        .createIfNecessary = true,
        .isEncryptedDb = false,
        .createDirByStoreIdOnly = true,
        .syncDualTupleMode = true,
        .secOption = { DistributedDB::S1, DistributedDB::ECE }
};
ZLOGI("start GetKvStore");
    kvStoreDelegateManager_->GetKvStore(UtilsConstants::OBJECTSTORE_DB_STOREID, option,
        [&store](DistributedDB::DBStatus dbStatus, DistributedDB::KvStoreNbDelegate *kvStoreNbDelegate) {
            if (dbStatus != DistributedDB::DBStatus::OK) {
                ZLOGE("GetKvStore fail %{public}d", dbStatus);
                return;
            }
            ZLOGI("GetKvStore successsfully");
            store = kvStoreNbDelegate;
        });
    return store;
}

int32_t ObjectStoreManager::Save(const std::string &appId, const std::string &sessionId,
    const std::map<std::string, std::vector<uint8_t>> &data, const std::vector<std::string> &deviceList,
    sptr<IObjectSaveCallback> &callback)
{
    int32_t result = Open();
    if (result != SUCCESS) {
        ZLOGE("Open objectStore DB failed,please check DB status");
        callback->Completed(std::map<std::string, int32_t>());
        return STORE_NOT_OPEN;
    }

    ZLOGI("start SaveToStore");
    result = SaveToStore(appId, sessionId, data);
    if (result != SUCCESS) {
        ZLOGE("Save to store failed, please check DB status, status = %{public}d", result);
        Close();
        callback->Completed(std::map<std::string, int32_t>());
        return result;
    }
    SyncCallBack tmp = [callback, appId, sessionId, this](const std::map<std::string, int32_t> &results) {
        callback->Completed(results);
        bool isLocal = false;
        for (auto &item : results) {
            if (item.first == LOCAL_DEVICE) {
                isLocal = true;
                break;
            }
        }
        if (!isLocal) {
            ZLOGE("delete local data sessionId = %{public}s", sessionId.c_str());
            int32_t result = Open();
            if (result != SUCCESS) {
                ZLOGE("Open objectStore DB failed,please check DB status");
                return;
            }
            // delete local data
            result = RevokeSaveToStore(appId, sessionId);
            if (result != SUCCESS) {
                ZLOGE("Save to store failed,please check DB status, status = %{public}d", result);
                Close();
                return;
            }
            Close();
        }
        return;
    };
    ZLOGI("start SyncOnStore");
    result = SyncOnStore(GetPropertyPrefix(appId, sessionId), deviceList, tmp);
    if (result != SUCCESS) {
        ZLOGE("sync on store failed,please check DB status, status = %{public}d", result);
        callback->Completed(std::map<std::string, int32_t>());
    }
    Close();
    return result;
}

int32_t ObjectStoreManager::RevokeSave(
    const std::string &appId, const std::string &sessionId, sptr<IObjectRevokeSaveCallback> &callback)
{
    int32_t result = Open();
    if (result != SUCCESS) {
        ZLOGE("Open objectStore DB failed,please check DB status");
        callback->Completed(STORE_NOT_OPEN);
        return STORE_NOT_OPEN;
    }

    result = RevokeSaveToStore(appId, sessionId);
    if (result != SUCCESS) {
        ZLOGE("Save to store failed,please check DB status, status = %{public}d", result);
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
            callback->Completed(SUCCESS);
        };
        result = SyncOnStore(GetPropertyPrefix(appId, sessionId), deviceList, tmp);
        if (result != SUCCESS) {
            ZLOGE("sync on store failed,please check DB status, status = %{public}d", result);
            callback->Completed(result);
        }
    } else {
        callback->Completed(SUCCESS);
    };
    Close();
    return result;
}

int32_t ObjectStoreManager::Retrieve(
    const std::string &appId, const std::string &sessionId, sptr<IObjectRetrieveCallback> callback)
{
    ZLOGI("enter");
    int32_t result = Open();
    if (result != SUCCESS) {
        ZLOGE("Open objectStore DB failed,please check DB status");
        callback->Completed(std::map<std::string, std::vector<uint8_t>>());
        return STORE_NOT_OPEN;
    }

    std::map<std::string, std::vector<uint8_t>> results;
    int32_t status = RetrieveFromStore(appId, sessionId, results);
    if (status != SUCCESS) {
        ZLOGE("Retrieve from store failed,please check DB status, status = %{public}d", status);
        Close();
        callback->Completed(std::map<std::string, std::vector<uint8_t>>());
        return status;
    }
    // delete local data
    status = RevokeSaveToStore(appId, sessionId);
    if (status != SUCCESS) {
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
    if (result != SUCCESS) {
        ZLOGE("Open objectStore DB failed,please check DB status");
        return STORE_NOT_OPEN;
    }
    std::vector<DistributedDB::Entry> entries;
    auto status = delegate_->GetEntries(std::vector<uint8_t>(), entries);
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("GetEntries failed,please check DB status");
        Close();
        return DB_ERROR;
    }
    std::vector<std::vector<uint8_t>> keys;
    std::for_each(
        entries.begin(), entries.end(), [&keys](const DistributedDB::Entry &entry) { keys.emplace_back(entry.key); });
    if (!keys.empty()) {
        delegate_->DeleteBatch(keys);
    }
    Close();
    return result;
}

void ObjectStoreManager::SetData(const std::string &dataDir, const std::string &userId)
{
    ZLOGI("enter %{public}s", dataDir.c_str());
    kvStoreDelegateManager_ = new DistributedDB::KvStoreDelegateManager(
        DistributedData::Bootstrap::GetInstance().GetProcessLabel(), userId);
    DistributedDB::KvStoreConfig kvStoreConfig { dataDir };
    kvStoreDelegateManager_->SetKvStoreConfig(kvStoreConfig);
    userId_ = userId;
}

int32_t ObjectStoreManager::Open()
{
    if (kvStoreDelegateManager_ == nullptr) {
        ZLOGE("not init");
        return FAILED;
    }
    std::lock_guard<std::mutex> lock(kvStoreMutex_);
    if (delegate_ == nullptr) {
        ZLOGI("open store");
        delegate_ = OpenObjectKvStore();
        if (delegate_ == nullptr) {
            ZLOGE("open failed,please check DB status");
            return FAILED;
        }
        syncCount_ = 1;
    } else {
        syncCount_++;
        ZLOGI("syncCount = %{public}d", syncCount_);
    }
    return SUCCESS;
}

int32_t ObjectStoreManager::Close()
{
    std::lock_guard<std::mutex> lock(kvStoreMutex_);
    if (delegate_ == nullptr) {
        return SUCCESS;
    }
    syncCount_--;
    ZLOGI("closed a store, syncCount = %{public}d", syncCount_);
    FlushClosedStore();
    return SUCCESS;
}

void ObjectStoreManager::SyncCompleted(
    const std::map<std::string, DistributedDB::DBStatus> &results, uint64_t sequenceId)
{
    std::string userId;
    Result result = SequenceSyncManager::GetInstance()->Process(sequenceId, results, userId);
    if (result == SUCCESS_USER_HAS_FINISHED && userId == userId_) {
        std::lock_guard<std::mutex> lock(kvStoreMutex_);
        SetSyncStatus(false);
        FlushClosedStore();
    }
}

void ObjectStoreManager::FlushClosedStore()
{
    if (!isSyncing_ && delegate_ != nullptr) {
        ZLOGD("close store");
        kvStoreDelegateManager_->CloseKvStore(delegate_);
        delegate_ = nullptr;
    }
}

int32_t ObjectStoreManager::SaveToStore(
    const std::string &appId, const std::string &sessionId, const std::map<std::string, std::vector<uint8_t>> &data)
{
    std::vector<DistributedDB::Entry> entries;
    std::string key;
    for (auto &item : data) {
        DistributedDB::Entry entry;
        std::string tmp = GetPropertyPrefix(appId, sessionId) + item.first;
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
            result[LOCAL_DEVICE] = SUCCESS;
            callback(result);
            return SUCCESS;
        }
        syncDevices.emplace_back(AppDistributedKv::CommunicationProvider::GetInstance().GetUuidByNodeId(device));
    }
    if (!syncDevices.empty()) {
        uint64_t sequenceId = SequenceSyncManager::GetInstance()->AddNotifier(userId_, callback);
        DistributedDB::Query dbQuery = DistributedDB::Query::Select();
        dbQuery.PrefixKey(std::vector<uint8_t>(prefix.begin(), prefix.end()));
        ZLOGI("start sync");
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
        return SUCCESS;
    }
    return SUCCESS;
}

int32_t ObjectStoreManager::SetSyncStatus(bool status)
{
    isSyncing_ = status;
    return SUCCESS;
}

int32_t ObjectStoreManager::RevokeSaveToStore(const std::string &appId, const std::string &sessionId)
{
    std::vector<DistributedDB::Entry> entries;
    ZLOGI("revoke save object %{public}s", sessionId.c_str());
    std::string prefix = GetDeletePrefix(appId, sessionId);
    auto status = delegate_->GetEntries(std::vector<uint8_t>(prefix.begin(), prefix.end()), entries);
    if (status == DistributedDB::DBStatus::NOT_FOUND) {
        ZLOGI("not found entry");
        return SUCCESS;
    }
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("GetEntries failed,please check DB status");
        return DB_ERROR;
    }
    std::vector<std::vector<uint8_t>> keys;
    std::for_each(
        entries.begin(), entries.end(), [&keys](const DistributedDB::Entry &entry) { keys.emplace_back(entry.key); });
    if (!keys.empty()) {
        delegate_->DeleteBatch(keys);
    }
    return SUCCESS;
}

int32_t ObjectStoreManager::RetrieveFromStore(
    const std::string &appId, const std::string &sessionId, std::map<std::string, std::vector<uint8_t>> &results)
{
    std::vector<DistributedDB::Entry> entries;
    std::string prefix = GetPropertyPrefix(appId, sessionId);
    auto status = delegate_->GetEntries(std::vector<uint8_t>(prefix.begin(), prefix.end()), entries);
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("GetEntries failed,please check DB status");
        return DB_ERROR;
    }
    ZLOGI("GetEntries successfully");
    std::for_each(
        entries.begin(), entries.end(), [&results, &appId, &sessionId, this](const DistributedDB::Entry &entry) {
            std::string key = BytesToStr(entry.key);
            ZLOGI("transferm propName = %{public}s", key.c_str());
            results[key.erase(0, GetPropertyPrefix(appId, sessionId).size())] = entry.value;
        });
    return SUCCESS;
}

uint64_t SequenceSyncManager::AddNotifier(const std::string &userId, SyncCallBack &callback)
{
    std::lock_guard<std::mutex> lock(notifierLock_);
    uint64_t sequenceId = KvStoreUtils::GenerateSequenceId();
    userIdSeqIdRelations_[userId].emplace_back(sequenceId);
    seqIdCallbackRelations_[sequenceId] = callback;
    return sequenceId;
}

Result SequenceSyncManager::Process(
    uint64_t sequenceId, const std::map<std::string, DistributedDB::DBStatus> &results, std::string &userId)
{
    std::lock_guard<std::mutex> lock(notifierLock_);
    if (seqIdCallbackRelations_.count(sequenceId) == 0) {
        ZLOGE("not exist");
        return ERR_SID_NOT_EXIST;
    }
    ZLOGI("start complete");
    std::map<std::string, int32_t> syncResults;
    for (auto &item : results) {
        ZLOGI("sync result %{public}s, %{public}d", item.first.c_str(), item.second);
        syncResults[item.first] = item.second == DistributedDB::DBStatus::OK ? 0 : -1;
    }
    seqIdCallbackRelations_[sequenceId](syncResults);
    ZLOGI("end complete");
    return DeleteNotifierNoLock(sequenceId, userId);
}

Result SequenceSyncManager::DeleteNotifier(uint64_t sequenceId, std::string &userId)
{
    std::lock_guard<std::mutex> lock(notifierLock_);
    if (seqIdCallbackRelations_.count(sequenceId) == 0) {
        ZLOGE("not exist");
        return ERR_SID_NOT_EXIST;
    }
    return DeleteNotifierNoLock(sequenceId, userId);
}

Result SequenceSyncManager::DeleteNotifierNoLock(uint64_t sequenceId, std::string &userId)
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
