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

#ifndef META_DATA_H
#define META_DATA_H

#include <atomic>
#include <map>
#include <mutex>
#include <vector>

#include "db_types.h"
#include "ikvdb_sync_interface.h"
#include "query_sync_water_mark_helper.h"

namespace DistributedDB {
struct MetaDataValue {
    TimeOffset timeOffset = 0;
    uint64_t lastUpdateTime = 0;
    uint64_t localWaterMark = 0;
    uint64_t peerWaterMark = 0;
    TimeStamp dbCreateTime = 0;
    uint64_t clearDeviceDataMark = 0; // Default 0 for not remove device data.
};

class Metadata {
public:
    Metadata();
    virtual ~Metadata();

    int Initialize(ISyncInterface *storage);

    int SaveTimeOffset(const DeviceID &deviceId, TimeOffset inValue);

    void GetTimeOffset(const DeviceID &deviceId, TimeOffset &outValue);

    void GetLocalWaterMark(const DeviceID &deviceId, uint64_t &outValue);

    int SaveLocalWaterMark(const DeviceID &deviceId, uint64_t inValue);

    void GetPeerWaterMark(const DeviceID &deviceId, uint64_t &outValue);

    int SavePeerWaterMark(const DeviceID &deviceId, uint64_t inValue, bool isNeedHash);

    int SaveLocalTimeOffset(TimeOffset timeOffset);

    TimeOffset GetLocalTimeOffset() const;

    int EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash);

    int EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash, const std::string &tableName);

    void SetLastLocalTime(TimeStamp lastLocalTime);

    TimeStamp GetLastLocalTime() const;

    int SetSendQueryWaterMark(const std::string &queryIdentify,
        const std::string &deviceId, const WaterMark &waterMark);

    // the querySync's sendWatermark will increase by the device watermark
    // if the sendWatermark less than device watermark
    int GetSendQueryWaterMark(const std::string &queryIdentify,
        const std::string &deviceId, WaterMark &waterMark, bool isAutoLift = true);

    int SetRecvQueryWaterMark(const std::string &queryIdentify,
        const std::string &deviceId, const WaterMark &waterMark);

    // the querySync's recvWatermark will increase by the device watermark
    // if the watermark less than device watermark
    int GetRecvQueryWaterMark(const std::string &queryIdentify,
        const std::string &deviceId, WaterMark &waterMark);

    virtual int SetLastQueryTime(const std::string &queryIdentify, const std::string &deviceId,
        const TimeStamp &timeStamp);

    virtual int GetLastQueryTime(const std::string &queryIdentify, const std::string &deviceId, TimeStamp &timeStamp);

    int SetSendDeleteSyncWaterMark(const std::string &deviceId, const WaterMark &waterMark);

    // the deleteSync's sendWatermark will increase by the device watermark
    // if the sendWatermark less than device watermark
    int GetSendDeleteSyncWaterMark(const std::string &deviceId, WaterMark &waterMark, bool isAutoLift = true);

    int SetRecvDeleteSyncWaterMark(const std::string &deviceId, const WaterMark &waterMark);

    // the deleteSync's recvWatermark will increase by the device watermark
    // if the recvWatermark less than device watermark
    int GetRecvDeleteSyncWaterMark(const std::string &deviceId, WaterMark &waterMark);

    void GetDbCreateTime(const DeviceID &deviceId, uint64_t &outValue);

    int SetDbCreateTime(const DeviceID &deviceId, uint64_t inValue, bool isNeedHash);

    int ResetMetaDataAfterRemoveData(const DeviceID &deviceId);

    void GetRemoveDataMark(const DeviceID &deviceId, uint64_t &outValue);

    // always get value from db, value updated from storage trigger
    uint64_t GetQueryLastTimestamp(const DeviceID &deviceId, const std::string &queryId) const;

    void RemoveQueryFromRecordSet(const DeviceID &deviceId, const std::string &queryId);
private:

    int SaveMetaDataValue(const DeviceID &deviceId, const MetaDataValue &inValue);

    // sync module need hash devices id
    void GetMetaDataValue(const DeviceID &deviceId, MetaDataValue &outValue, bool isNeedHash);

    int SerializeMetaData(const MetaDataValue &inValue, std::vector<uint8_t> &outValue);

    int DeSerializeMetaData(const std::vector<uint8_t> &inValue, MetaDataValue &outValue) const;

    int GetMetadataFromDb(const std::vector<uint8_t> &key, std::vector<uint8_t> &outValue) const;

    int SetMetadataToDb(const std::vector<uint8_t> &key, const std::vector<uint8_t> &inValue);

    int DeleteMetaDataFromDB(const std::vector<Key> &keys) const;
    int DeleteMetaDataByPrefixKey(const Key &keyPrefix) const;

    void PutMetadataToMap(const DeviceID &deviceId, const MetaDataValue &value);

    void GetMetadataFromMap(const DeviceID &deviceId, MetaDataValue &outValue);

    int64_t StringToLong(const std::vector<uint8_t> &value) const;

    int GetAllMetadataKey(std::vector<std::vector<uint8_t>> &keys);

    int LoadAllMetadata();

    uint64_t GetRandTimeOffset() const;

    void GetHashDeviceId(const DeviceID &deviceId, DeviceID &hashDeviceId, bool isNeedHash);

    // this function will read data from db by metaData's key
    // and then serialize it and put to map
    int LoadDeviceIdDataToMap(const Key &key);

    // reset the waterMark to zero
    int ResetRecvQueryWaterMark(const DeviceID &deviceId, const std::string &tableName = "");

    // store localTimeOffset in ram; if change, should add a lock first, change here and metadata,
    // then release lock
    std::atomic<TimeOffset> localTimeOffset_;
    std::mutex localTimeOffsetLock_;
    ISyncInterface *naturalStoragePtr_;

    // if changed, it should be locked from save-to-db to change-in-memory.save to db must be first,
    // if save to db fail, it will not be changed in memory.
    std::map<std::string, MetaDataValue> metadataMap_;
    mutable std::mutex metadataLock_;
    std::map<DeviceID, DeviceID> deviceIdToHashDeviceIdMap_;

    // store localTimeOffset in ram, used to make timestamp increase
    mutable std::mutex lastLocalTimeLock_;
    TimeStamp lastLocalTime_;

    QuerySyncWaterMarkHelper querySyncWaterMarkHelper_;

    // set value: SUBSCRIBE_QUERY_PREFIX + DBCommon::TransferHashString(queryId)
    // queryId is not in set while key is not found from db first time, and return lastTimestamp = INT64_MAX
    // if query is in set return 0 while not found from db, means already sync before, don't trigger again
    mutable std::map<DeviceID, std::set<std::string>> queryIdMap_;
};
}  // namespace DistributedDB
#endif
