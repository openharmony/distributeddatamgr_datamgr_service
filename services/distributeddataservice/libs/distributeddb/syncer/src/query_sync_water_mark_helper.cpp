/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "query_sync_water_mark_helper.h"

#include <algorithm>
#include <version.h>
#include "platform_specific.h"
#include "parcel.h"
#include "db_errno.h"
#include "db_common.h"
#include "log_print.h"

namespace DistributedDB {
namespace {
    const int MAX_CACHE_ITEMS = 200;
    const uint32_t MAX_STORE_ITEMS = 100000;
    // WaterMark Version
    constexpr uint32_t QUERY_WATERMARK_VERSION_CURRENT = SOFTWARE_VERSION_RELEASE_6_0;
    constexpr uint32_t DELETE_WATERMARK_VERSION_CURRENT = SOFTWARE_VERSION_RELEASE_3_0;
    // Prefix Key in db
    const std::string QUERY_SYNC_PREFIX_KEY = "querySync";
    const std::string DELETE_SYNC_PREFIX_KEY = "deleteSync";
}

QuerySyncWaterMarkHelper::QuerySyncWaterMarkHelper()
    : storage_(nullptr)
{}

QuerySyncWaterMarkHelper::~QuerySyncWaterMarkHelper()
{
    storage_ = nullptr;
    deviceIdToHashQuerySyncIdMap_.clear();
    deleteSyncCache_.clear();
    deviceIdToHashDeleteSyncIdMap_.clear();
}

int LruMap::Put(const std::string &key, const QueryWaterMark &inValue)
{
    std::lock_guard<std::mutex> autoLock(lruLock_);
    cache_[key] = inValue;
    return Elimination(key, inValue);
}

int LruMap::Get(const std::string &key, QueryWaterMark &outValue)
{
    std::lock_guard<std::mutex> autoLock(lruLock_);
    if (cache_.count(key) == 0) {
        return -E_NOT_FOUND;
    }
    outValue = cache_[key];
    return Elimination(key, outValue);
}

void LruMap::RemoveWithPrefixKey(const std::string &prefixKey)
{
    std::lock_guard<std::mutex> autoLock(lruLock_);
    auto iterator = eliminationChain_.begin();
    while (iterator != eliminationChain_.end()) {
        const std::string &key = (*iterator).first;
        if (key.find(prefixKey) == 0) {
            (void)cache_.erase(key);
            iterator = eliminationChain_.erase(iterator);
        } else {
            iterator++;
        }
    }
}

// move the node to last and remove the first node until the size less than limit
int LruMap::Elimination(const std::string &key, const QueryWaterMark &inQueryWaterMark)
{
    auto iterator = eliminationChain_.begin();
    while (iterator != eliminationChain_.end()) {
        if ((*iterator).first == key) {
            eliminationChain_.erase(iterator);
            break;
        }
        iterator++;
    }
    std::pair<std::string, QueryWaterMark> entry = {key, inQueryWaterMark};
    eliminationChain_.push_back(entry);
    while (eliminationChain_.size() > MAX_CACHE_ITEMS) {
        std::pair<std::string, QueryWaterMark> &pair = eliminationChain_.front();
        cache_.erase(pair.first);
        eliminationChain_.pop_front();
    }
    return E_OK;
}

int QuerySyncWaterMarkHelper::GetMetadataFromDb(const std::vector<uint8_t> &key, std::vector<uint8_t> &outValue)
{
    if (storage_ == nullptr) {
        return -E_INVALID_DB;
    }
    return storage_->GetMetaData(key, outValue);
}

int QuerySyncWaterMarkHelper::SetMetadataToDb(const std::vector<uint8_t> &key, const std::vector<uint8_t> &inValue)
{
    if (storage_ == nullptr) {
        return -E_INVALID_DB;
    }
    return storage_->PutMetaData(key, inValue);
}

int QuerySyncWaterMarkHelper::DeleteMetaDataFromDB(const std::vector<Key> &keys) const
{
    if (storage_ == nullptr) {
        return -E_INVALID_DB;
    }
    return storage_->DeleteMetaData(keys);
}

int QuerySyncWaterMarkHelper::Initialize(ISyncInterface *storage)
{
    storage_ = storage;
    return E_OK;
}

int QuerySyncWaterMarkHelper::LoadDeleteSyncDataToCache(const Key &deleteWaterMarkKey)
{
    std::vector<uint8_t> value;
    int errCode = GetMetadataFromDb(deleteWaterMarkKey, value);
    if (errCode != E_OK) {
        return errCode;
    }
    DeleteWaterMark deleteWaterMark;
    std::string dbKey(deleteWaterMarkKey.begin(), deleteWaterMarkKey.end());
    errCode = DeSerializeDeleteWaterMark(value, deleteWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    std::lock_guard<std::mutex> autoLock(deleteSyncLock_);
    deleteSyncCache_[dbKey] = deleteWaterMark;
    return errCode;
}

int QuerySyncWaterMarkHelper::GetQueryWaterMarkInCacheAndDb(const std::string &cacheKey,
    QueryWaterMark &queryWaterMark)
{
    // first get from cache_
    int errCode = querySyncCache_.Get(cacheKey, queryWaterMark);
    bool addToCache = false;
    if (errCode == -E_NOT_FOUND) {
        // second get from db
        errCode = GetQueryWaterMarkFromDB(cacheKey, queryWaterMark);
        addToCache = true;
    }
    if (errCode == -E_NOT_FOUND) {
        // third generate one and save to db
        errCode = PutQueryWaterMarkToDB(cacheKey, queryWaterMark);
    }
    // something error return
    if (errCode != E_OK) {
        LOGE("[Meta]GetQueryWaterMark Fail code = %d", errCode);
        return errCode;
    }
    // remember add to cache_
    if (addToCache) {
        querySyncCache_.Put(cacheKey, queryWaterMark);
    }
    return errCode;
}

int QuerySyncWaterMarkHelper::GetQueryWaterMark(const std::string &queryIdentify, const std::string &deviceId,
    QueryWaterMark &queryWaterMark)
{
    std::string cacheKey;
    GetHashQuerySyncDeviceId(deviceId, queryIdentify, cacheKey);
    std::lock_guard<std::mutex> autoLock(queryWaterMarkLock_);
    return GetQueryWaterMarkInCacheAndDb(cacheKey, queryWaterMark);
}

int QuerySyncWaterMarkHelper::SetRecvQueryWaterMark(const std::string &queryIdentify,
    const std::string &deviceId, const WaterMark &waterMark)
{
    std::string cacheKey;
    GetHashQuerySyncDeviceId(deviceId, queryIdentify, cacheKey);
    std::lock_guard<std::mutex> autoLock(queryWaterMarkLock_);
    return SetRecvQueryWaterMarkWithoutLock(deviceId, cacheKey, waterMark);
}

int QuerySyncWaterMarkHelper::SetLastQueryTime(const std::string &queryIdentify,
    const std::string &deviceId, const TimeStamp &timeStamp)
{
    std::string cacheKey;
    GetHashQuerySyncDeviceId(deviceId, queryIdentify, cacheKey);
    std::lock_guard<std::mutex> autoLock(queryWaterMarkLock_);
    QueryWaterMark queryWaterMark;
    int errCode = GetQueryWaterMarkInCacheAndDb(cacheKey, queryWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    queryWaterMark.lastQueryTime = timeStamp;
    return UpdateCacheAndSave(cacheKey, deviceId, queryWaterMark);
}

int QuerySyncWaterMarkHelper::SetRecvQueryWaterMarkWithoutLock(const std::string &deviceId,
    const std::string &cacheKey, const WaterMark &waterMark)
{
    QueryWaterMark queryWaterMark;
    int errCode = GetQueryWaterMarkInCacheAndDb(cacheKey, queryWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    queryWaterMark.recvWaterMark = waterMark;
    return UpdateCacheAndSave(cacheKey, deviceId, queryWaterMark);
}

int QuerySyncWaterMarkHelper::SetSendQueryWaterMark(const std::string &queryIdentify,
    const std::string &deviceId, const WaterMark &waterMark)
{
    std::string cacheKey;
    GetHashQuerySyncDeviceId(deviceId, queryIdentify, cacheKey);
    QueryWaterMark queryWaterMark;
    std::lock_guard<std::mutex> autoLock(queryWaterMarkLock_);
    int errCode = GetQueryWaterMarkInCacheAndDb(cacheKey, queryWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    queryWaterMark.sendWaterMark = waterMark;
    return UpdateCacheAndSave(cacheKey, deviceId, queryWaterMark);
}

int QuerySyncWaterMarkHelper::UpdateCacheAndSave(const std::string &cacheKey,
    const std::string &deviceId, QueryWaterMark &queryWaterMark)
{
    // update lastUsedTime
    int errCode = OS::GetCurrentSysTimeInMicrosecond(queryWaterMark.lastUsedTime);
    if (errCode != E_OK) {
        return errCode;
    }
    // save db first
    errCode = SaveQueryWaterMarkToDB(cacheKey, queryWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    querySyncCache_.Put(cacheKey, queryWaterMark);
    return errCode;
}

int QuerySyncWaterMarkHelper::PutQueryWaterMarkToDB(const DeviceID &dbKeyString, QueryWaterMark &queryWaterMark)
{
    int errCode = OS::GetCurrentSysTimeInMicrosecond(queryWaterMark.lastUsedTime);
    if (errCode != E_OK) {
        return errCode;
    }
    queryWaterMark.version = QUERY_WATERMARK_VERSION_CURRENT;
    return SaveQueryWaterMarkToDB(dbKeyString, queryWaterMark);
}

int QuerySyncWaterMarkHelper::SaveQueryWaterMarkToDB(const DeviceID &dbKeyString, const QueryWaterMark &queryWaterMark)
{
    // serialize value
    Value dbValue;
    int errCode = SerializeQueryWaterMark(queryWaterMark, dbValue);
    if (errCode != E_OK) {
        return errCode;
    }
    // serialize key
    Key dbKey;
    DBCommon::StringToVector(dbKeyString, dbKey);
    // save
    errCode = SetMetadataToDb(dbKey, dbValue);
    if (errCode != E_OK) {
        LOGE("QuerySyncWaterMarkHelper::SaveQueryWaterMarkToDB failed errCode:%d", errCode);
    }
    return errCode;
}

int QuerySyncWaterMarkHelper::GetQueryWaterMarkFromDB(const DeviceID &dbKeyString, QueryWaterMark &queryWaterMark)
{
    // serialize key
    Key dbKey;
    DBCommon::StringToVector(dbKeyString, dbKey);
    // search in db
    Value dbValue;
    int errCode = GetMetadataFromDb(dbKey, dbValue);
    if (errCode != E_OK) {
        return errCode;
    }
    return DeSerializeQueryWaterMark(dbValue, queryWaterMark);
}

int QuerySyncWaterMarkHelper::SerializeQueryWaterMark(const QueryWaterMark &queryWaterMark, Value &outValue)
{
    uint64_t length = CalculateQueryWaterMarkSize(queryWaterMark);
    outValue.resize(length);
    Parcel parcel(outValue.data(), outValue.size());
    parcel.WriteUInt32(queryWaterMark.version);
    parcel.EightByteAlign();
    parcel.WriteUInt64(queryWaterMark.sendWaterMark);
    parcel.WriteUInt64(queryWaterMark.recvWaterMark);
    parcel.WriteUInt64(queryWaterMark.lastUsedTime);
    parcel.WriteString(queryWaterMark.sql);
    parcel.WriteUInt64(queryWaterMark.lastQueryTime);
    if (parcel.IsError()) {
        LOGE("[Meta] Parcel error when serialize queryWaterMark");
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

int QuerySyncWaterMarkHelper::DeSerializeQueryWaterMark(const Value &dbQueryWaterMark, QueryWaterMark &queryWaterMark)
{
    Parcel parcel(const_cast<uint8_t *>(dbQueryWaterMark.data()), dbQueryWaterMark.size());
    parcel.ReadUInt32(queryWaterMark.version);
    parcel.EightByteAlign();
    parcel.ReadUInt64(queryWaterMark.sendWaterMark);
    parcel.ReadUInt64(queryWaterMark.recvWaterMark);
    parcel.ReadUInt64(queryWaterMark.lastUsedTime);
    parcel.ReadString(queryWaterMark.sql);
    if (queryWaterMark.version >= SOFTWARE_VERSION_RELEASE_6_0) {
        parcel.ReadUInt64(queryWaterMark.lastQueryTime);
    }
    if (parcel.IsError()) {
        LOGE("[Meta] Parcel error when deserialize queryWaterMark");
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

uint64_t QuerySyncWaterMarkHelper::CalculateQueryWaterMarkSize(const QueryWaterMark &queryWaterMark)
{
    uint64_t length = Parcel::GetUInt32Len(); // version
    length = Parcel::GetEightByteAlign(length);
    length += Parcel::GetUInt64Len(); // sendWaterMark
    length += Parcel::GetUInt64Len(); // recvWaterMark
    length += Parcel::GetUInt64Len(); // lastUsedTime
    length += Parcel::GetStringLen(queryWaterMark.sql);
    length += Parcel::GetUInt64Len(); // lastQueryTime
    return length;
}

void QuerySyncWaterMarkHelper::GetHashQuerySyncDeviceId(const DeviceID &deviceId,
    const DeviceID &queryId, DeviceID &hashQuerySyncId)
{
    std::lock_guard<std::mutex> autoLock(queryWaterMarkLock_);
    if (deviceIdToHashQuerySyncIdMap_[deviceId].count(queryId) == 0) {
        // do not modify this
        hashQuerySyncId = QUERY_SYNC_PREFIX_KEY + DBCommon::TransferHashString(deviceId) + queryId;
        deviceIdToHashQuerySyncIdMap_[deviceId][queryId] = hashQuerySyncId;
    } else {
        hashQuerySyncId = deviceIdToHashQuerySyncIdMap_[deviceId][queryId];
    }
}

int QuerySyncWaterMarkHelper::GetDeleteSyncWaterMark(const std::string &deviceId, DeleteWaterMark &deleteWaterMark)
{
    std::string hashId;
    GetHashDeleteSyncDeviceId(deviceId, hashId);
    return GetDeleteWaterMarkFromCache(hashId, deleteWaterMark);
}

int QuerySyncWaterMarkHelper::SetSendDeleteSyncWaterMark(const DeviceID &deviceId, const WaterMark &waterMark)
{
    std::string hashId;
    GetHashDeleteSyncDeviceId(deviceId, hashId);
    DeleteWaterMark deleteWaterMark;
    GetDeleteWaterMarkFromCache(hashId, deleteWaterMark);
    deleteWaterMark.sendWaterMark = waterMark;
    std::lock_guard<std::mutex> autoLock(deleteSyncLock_);
    return UpdateDeleteSyncCacheAndSave(hashId, deleteWaterMark);
}

int QuerySyncWaterMarkHelper::SetRecvDeleteSyncWaterMark(const DeviceID &deviceId, const WaterMark &waterMark)
{
    std::string hashId;
    GetHashDeleteSyncDeviceId(deviceId, hashId);
    DeleteWaterMark deleteWaterMark;
    GetDeleteWaterMarkFromCache(hashId, deleteWaterMark);
    deleteWaterMark.recvWaterMark = waterMark;
    std::lock_guard<std::mutex> autoLock(deleteSyncLock_);
    return UpdateDeleteSyncCacheAndSave(hashId, deleteWaterMark);
}

int QuerySyncWaterMarkHelper::UpdateDeleteSyncCacheAndSave(const std::string &dbKey,
    const DeleteWaterMark &deleteWaterMark)
{
    // save db first
    int errCode = SaveDeleteWaterMarkToDB(dbKey, deleteWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    // modify cache
    deleteSyncCache_[dbKey] = deleteWaterMark;
    return errCode;
}

int QuerySyncWaterMarkHelper::GetDeleteWaterMarkFromCache(const DeviceID &hashDeviceId,
    DeleteWaterMark &deleteWaterMark)
{
    // lock prevent different thread visit deleteSyncCache_
    std::lock_guard<std::mutex> autoLock(deleteSyncLock_);
    // if not found
    if (deleteSyncCache_.find(hashDeviceId) == deleteSyncCache_.end()) {
        DeleteWaterMark waterMark;
        waterMark.version = DELETE_WATERMARK_VERSION_CURRENT;
        int errCode = GetDeleteWaterMarkFromDB(hashDeviceId, waterMark);
        if (errCode == -E_NOT_FOUND) {
            deleteWaterMark.sendWaterMark = 0;
            deleteWaterMark.recvWaterMark = 0;
            errCode = E_OK;
        }
        if (errCode != E_OK) {
            LOGE("[Meta]GetDeleteWaterMark Fail code = %d", errCode);
            return errCode;
        }
        deleteSyncCache_.insert(std::pair<DeviceID, DeleteWaterMark>(hashDeviceId, waterMark));
    }
    deleteWaterMark = deleteSyncCache_[hashDeviceId];
    return E_OK;
}

int QuerySyncWaterMarkHelper::GetDeleteWaterMarkFromDB(const DeviceID &hashDeviceId,
    DeleteWaterMark &deleteWaterMark)
{
    Key dbKey;
    DBCommon::StringToVector(hashDeviceId, dbKey);
    // search in db
    Value dbValue;
    int errCode = GetMetadataFromDb(dbKey, dbValue);
    if (errCode != E_OK) {
        return errCode;
    }
    // serialize value
    return DeSerializeDeleteWaterMark(dbValue, deleteWaterMark);
}

int QuerySyncWaterMarkHelper::SaveDeleteWaterMarkToDB(const DeviceID &hashDeviceId,
    const DeleteWaterMark &deleteWaterMark)
{
    // serialize value
    Value dbValue;
    int errCode = SerializeDeleteWaterMark(deleteWaterMark, dbValue);
    if (errCode != E_OK) {
        return errCode;
    }
    Key dbKey;
    DBCommon::StringToVector(hashDeviceId, dbKey);
    // save
    errCode = SetMetadataToDb(dbKey, dbValue);
    if (errCode != E_OK) {
        LOGE("QuerySyncWaterMarkHelper::SaveDeleteWaterMarkToDB failed errCode:%d", errCode);
    }
    return errCode;
}

void QuerySyncWaterMarkHelper::GetHashDeleteSyncDeviceId(const DeviceID &deviceId, DeviceID &hashDeleteSyncId)
{
    std::lock_guard<std::mutex> autoLock(deleteSyncLock_);
    if (deviceIdToHashDeleteSyncIdMap_.count(deviceId) == 0) {
        hashDeleteSyncId = DELETE_SYNC_PREFIX_KEY + DBCommon::TransferHashString(deviceId);
        deviceIdToHashDeleteSyncIdMap_.insert(std::pair<DeviceID, DeviceID>(deviceId, hashDeleteSyncId));
    } else {
        hashDeleteSyncId = deviceIdToHashDeleteSyncIdMap_[deviceId];
    }
}

int QuerySyncWaterMarkHelper::SerializeDeleteWaterMark(const DeleteWaterMark &deleteWaterMark,
    std::vector<uint8_t> &outValue)
{
    uint64_t length = CalculateDeleteWaterMarkSize(deleteWaterMark);
    outValue.resize(length);
    Parcel parcel(outValue.data(), outValue.size());
    parcel.WriteUInt32(deleteWaterMark.version);
    parcel.EightByteAlign();
    parcel.WriteUInt64(deleteWaterMark.sendWaterMark);
    parcel.WriteUInt64(deleteWaterMark.recvWaterMark);
    if (parcel.IsError()) {
        LOGE("[Meta] Parcel error when serialize deleteWaterMark.");
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

int QuerySyncWaterMarkHelper::DeSerializeDeleteWaterMark(const std::vector<uint8_t> &inValue,
    DeleteWaterMark &deleteWaterMark)
{
    Parcel parcel(const_cast<uint8_t *>(inValue.data()), inValue.size());
    parcel.ReadUInt32(deleteWaterMark.version);
    parcel.EightByteAlign();
    parcel.ReadUInt64(deleteWaterMark.sendWaterMark);
    parcel.ReadUInt64(deleteWaterMark.recvWaterMark);
    if (parcel.IsError()) {
        LOGE("[Meta] Parcel error when deserialize deleteWaterMark.");
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

uint64_t QuerySyncWaterMarkHelper::CalculateDeleteWaterMarkSize(const DeleteWaterMark &deleteWaterMark)
{
    uint64_t length = Parcel::GetUInt32Len(); // version
    length = Parcel::GetEightByteAlign(length);
    length += Parcel::GetUInt64Len(); // sendWaterMark
    length += Parcel::GetUInt64Len(); // recvWaterMark
    return length;
}

std::string QuerySyncWaterMarkHelper::GetQuerySyncPrefixKey()
{
    return QUERY_SYNC_PREFIX_KEY;
}

std::string QuerySyncWaterMarkHelper::GetDeleteSyncPrefixKey()
{
    return DELETE_SYNC_PREFIX_KEY;
}

int QuerySyncWaterMarkHelper::RemoveLeastUsedQuerySyncItems(const std::vector<Key> &querySyncIds)
{
    if (querySyncIds.size() < MAX_STORE_ITEMS) {
        return E_OK;
    }
    std::vector<std::pair<std::string, TimeStamp>> allItems;
    std::map<std::string, std::vector<uint8_t>> idMap;
    std::vector<std::vector<uint8_t>> waitToRemove;
    for (const auto &id : querySyncIds) {
        Value value;
        int errCode = GetMetadataFromDb(id, value);
        if (errCode != E_OK) {
            waitToRemove.push_back(id);
            continue; // may be this failure cause by wrong data
        }
        QueryWaterMark queryWaterMark;
        std::string queryKey(id.begin(), id.end());
        errCode = DeSerializeQueryWaterMark(value, queryWaterMark);
        if (errCode != E_OK) {
            waitToRemove.push_back(id);
            continue; // may be this failure cause by wrong data
        }
        idMap.insert({queryKey, id});
        allItems.emplace_back(queryKey, queryWaterMark.lastUsedTime);
    }
    // we only remove broken data below
    // 1. common data size less then 10w
    // 2. allItems.size() - MAX_STORE_ITEMS - waitToRemove.size() < 0
    // so we only let allItems.size() < MAX_STORE_ITEMS + waitToRemove.size()
    if (allItems.size() < MAX_STORE_ITEMS + waitToRemove.size()) {
        // remove in db
        return DeleteMetaDataFromDB(waitToRemove);
    }
    uint32_t removeCount = allItems.size() - MAX_STORE_ITEMS - waitToRemove.size();
    // quick select the k_th least used
    std::nth_element(allItems.begin(), allItems.begin() + removeCount, allItems.end(),
        [](std::pair<std::string, TimeStamp> &w1, std::pair<std::string, TimeStamp> &w2) {
            return w1.second < w2.second;
        });
    for (uint32_t i = 0; i < removeCount; ++i) {
        waitToRemove.push_back(idMap[allItems[i].first]);
    }
    // remove in db
    return DeleteMetaDataFromDB(waitToRemove);
}

int QuerySyncWaterMarkHelper::ResetRecvQueryWaterMark(const DeviceID &deviceId, const std::string &tableName)
{
    // lock prevent other thread modify queryWaterMark at this moment
    {
        std::lock_guard<std::mutex> autoLock(queryWaterMarkLock_);
        std::string prefixKeyStr = QUERY_SYNC_PREFIX_KEY + DBCommon::TransferHashString(deviceId);
        if (!tableName.empty()) {
            std::string hashTableName = DBCommon::TransferHashString(tableName);
            std::string hexTableName = DBCommon::TransferStringToHex(hashTableName);
            prefixKeyStr += hexTableName;
        }
        
        // remove in db
        Key prefixKey;
        DBCommon::StringToVector(prefixKeyStr, prefixKey);
        int errCode = storage_->DeleteMetaDataByPrefixKey(prefixKey);
        if (errCode != E_OK) {
            LOGE("[META]ResetRecvQueryWaterMark fail errCode:%d", errCode);
            return errCode;
        }
        // clean cache
        querySyncCache_.RemoveWithPrefixKey(prefixKeyStr);
    }
    return E_OK;
}
}  // namespace DistributedDB