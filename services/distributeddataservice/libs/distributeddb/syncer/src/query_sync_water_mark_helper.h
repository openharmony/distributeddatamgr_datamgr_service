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

#ifndef QUERY_SYNC_WATER_MARK_HELPER_H
#define QUERY_SYNC_WATER_MARK_HELPER_H

#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include "db_types.h"
#include "ikvdb_sync_interface.h"

namespace DistributedDB {
struct QueryWaterMark {
    uint32_t version = 0; // start with 103
    WaterMark sendWaterMark = 0;
    WaterMark recvWaterMark = 0;
    TimeStamp lastUsedTime = 0; // use for delete data
    std::string sql; // for analyze sql from logs
    TimeStamp lastQueryTime = 0; // use for miss query scene add in 106
};

struct DeleteWaterMark {
    uint32_t version = 0;
    WaterMark sendWaterMark = 0;
    WaterMark recvWaterMark = 0;
};

// LRU map
class LruMap final {
public:
    LruMap() = default;
    ~LruMap() = default;

    DISABLE_COPY_ASSIGN_MOVE(LruMap);

    int Get(const std::string &key, QueryWaterMark &outValue);
    int Put(const std::string &key, const QueryWaterMark &inValue);
    void RemoveWithPrefixKey(const std::string &prefixKey);
private:
    int Elimination(const std::string &key, const QueryWaterMark &inQueryWaterMark);

    std::mutex lruLock_;
    std::map<std::string, QueryWaterMark> cache_;
    std::deque<std::pair<std::string, QueryWaterMark>> eliminationChain_;
};

class QuerySyncWaterMarkHelper {
public:
    QuerySyncWaterMarkHelper();
    ~QuerySyncWaterMarkHelper();

    DISABLE_COPY_ASSIGN_MOVE(QuerySyncWaterMarkHelper);

    int Initialize(ISyncInterface *storage);

    int GetQueryWaterMark(const std::string &queryIdentify, const std::string &deviceId,
        QueryWaterMark &queryWaterMark);

    int SetSendQueryWaterMark(const std::string &queryIdentify,
        const std::string &deviceId, const WaterMark &waterMark);

    int SetRecvQueryWaterMark(const std::string &queryIdentify,
        const std::string &deviceId, const WaterMark &waterMark);

    int SetLastQueryTime(const std::string &queryIdentify,
        const std::string &deviceId, const TimeStamp &timeStamp);

    int GetDeleteSyncWaterMark(const std::string &deviceId, DeleteWaterMark &deleteWaterMark);

    int SetSendDeleteSyncWaterMark(const std::string &deviceId, const WaterMark &waterMark);

    int SetRecvDeleteSyncWaterMark(const std::string &deviceId, const WaterMark &waterMark);

    // this function will read deleteWaterMark from db by it's deleteWaterMarkKey
    // and then serialize it and put to cache
    int LoadDeleteSyncDataToCache(const Key &deleteWaterMarkKey);

    // this function will remove data in db
    int RemoveLeastUsedQuerySyncItems(const std::vector<Key> &querySyncIds);

    // reset the waterMark to zero
    int ResetRecvQueryWaterMark(const DeviceID &deviceId, const std::string &tableName);

    static std::string GetQuerySyncPrefixKey();

    static std::string GetDeleteSyncPrefixKey();

private:

    int GetMetadataFromDb(const std::vector<uint8_t> &key, std::vector<uint8_t> &outValue);

    int SetMetadataToDb(const std::vector<uint8_t> &key, const std::vector<uint8_t> &inValue);

    int DeleteMetaDataFromDB(const std::vector<Key> &keys) const;

    int SaveQueryWaterMarkToDB(const DeviceID &dbKeyString, const QueryWaterMark &queryWaterMark);

    int GetQueryWaterMarkFromDB(const DeviceID &dbKeyString, QueryWaterMark &queryWaterMark);

    int SetRecvQueryWaterMarkWithoutLock(const std::string &deviceId, const std::string &cacheKey,
        const WaterMark &waterMark);

    // search the queryWaterMark from db or cache_
    // and ensure it exit in cache_
    int GetQueryWaterMarkInCacheAndDb(const std::string &cacheKey, QueryWaterMark &queryWaterMark);

    // only first create queryWaterMark will call this function
    // it will create a queryWaterMark and save to db
    int PutQueryWaterMarkToDB(const DeviceID &dbKeyString, QueryWaterMark &queryWaterMark);

    // get the querySync hashId in cache_ or generate one and then put it in to cache_
    // the hashId is made up of "QUERY_SYNC_PREFIX_KEY" + hash(deviceId) + queryId
    void GetHashQuerySyncDeviceId(const DeviceID &deviceId,
        const DeviceID &queryId, DeviceID &hashQuerySyncId);

    // put queryWaterMark to lru cache_ and then save to db
    int UpdateCacheAndSave(const std::string &cacheKey, const std::string &deviceId, QueryWaterMark &queryWaterMark);

    // search the deleteWaterMark from db or cache_
    // and ensure it exit in cache_
    int GetDeleteWaterMarkFromCache(const DeviceID &hashDeviceId, DeleteWaterMark &deleteWaterMark);

    // get the deleteSync hashId in cache_ or generate one and then put it in to cache_
    // the hashId is made up of "DELETE_SYNC_PREFIX_KEY" + hash(deviceId)
    void GetHashDeleteSyncDeviceId(const DeviceID &deviceId, DeviceID &hashDeleteSyncId);

    int SaveDeleteWaterMarkToDB(const DeviceID &hashDeviceId, const DeleteWaterMark &deleteWaterMark);

    int GetDeleteWaterMarkFromDB(const DeviceID &hashDeviceId, DeleteWaterMark &deleteWaterMark);

    // put queryWaterMark to lru cache_ and then save to db
    int UpdateDeleteSyncCacheAndSave(const std::string &dbKey, const DeleteWaterMark &deleteWaterMark);

    static int SerializeQueryWaterMark(const QueryWaterMark &queryWaterMark, std::vector<uint8_t> &outValue);

    static int DeSerializeQueryWaterMark(const std::vector<uint8_t> &dbQueryWaterMark, QueryWaterMark &queryWaterMark);

    static uint64_t CalculateQueryWaterMarkSize(const QueryWaterMark &queryWaterMark);

    static int SerializeDeleteWaterMark(const DeleteWaterMark &deleteWaterMark, std::vector<uint8_t> &outValue);

    static int DeSerializeDeleteWaterMark(const std::vector<uint8_t> &inValue, DeleteWaterMark &deleteWaterMark);

    static uint64_t CalculateDeleteWaterMarkSize(const DeleteWaterMark &deleteWaterMark);

    // store or visit queryWaterMark should add a lock
    // because it will change the eliminationChain
    // and the queryWaterMark use a LRU Map to store in ram
    std::mutex queryWaterMarkLock_;
    LruMap querySyncCache_;
    std::map<DeviceID, std::map<std::string, std::string>> deviceIdToHashQuerySyncIdMap_;

    // also store deleteKeyWaterMark should add a lock
    std::mutex deleteSyncLock_;
    std::map<std::string, DeleteWaterMark> deleteSyncCache_;
    std::map<DeviceID, std::string> deviceIdToHashDeleteSyncIdMap_;

    ISyncInterface *storage_;
};
} // namespace DistributedDB
#endif
