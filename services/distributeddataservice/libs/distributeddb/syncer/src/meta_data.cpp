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

#include "meta_data.h"

#include <openssl/rand.h>
#include "db_common.h"
#include "db_constant.h"
#include "db_errno.h"
#include "hash.h"
#include "log_print.h"
#include "securec.h"
#include "sync_types.h"
#include "time_helper.h"

namespace DistributedDB {
namespace {
    const int STR_TO_LL_BY_DEVALUE = 10;
    // store local timeoffset;this is a special key;
    const std::string LOCALTIME_OFFSET_KEY = "localTimeOffset";
    const std::string DEVICEID_PREFIX_KEY = "deviceId";
}

Metadata::Metadata()
    : localTimeOffset_(0),
      naturalStoragePtr_(nullptr),
      lastLocalTime_(0)
{}

Metadata::~Metadata()
{
    naturalStoragePtr_ = nullptr;
    metadataMap_.clear();
}

int Metadata::Initialize(ISyncInterface* storage)
{
    naturalStoragePtr_ = storage;
    std::vector<uint8_t> key;
    std::vector<uint8_t> timeOffset;
    DBCommon::StringToVector(LOCALTIME_OFFSET_KEY, key);

    int errCode = GetMetadataFromDb(key, timeOffset);
    if (errCode == -E_NOT_FOUND) {
        uint64_t randTimeOffset = GetRandTimeOffset();
        int err = SaveLocalTimeOffset(TimeHelper::BASE_OFFSET + static_cast<int64_t>(randTimeOffset));
        if (err != E_OK) {
            LOGD("[Metadata][Initialize]SaveLocalTimeOffset failed errCode:%d", err);
            return err;
        }
    } else if (errCode == E_OK) {
        localTimeOffset_ = StringToLong(timeOffset);
    } else {
        LOGE("Metadata::Initialize get meatadata from db failed,err=%d", errCode);
        return errCode;
    }
    {
        std::lock_guard<std::mutex> lockGuard(metadataLock_);
        metadataMap_.clear();
    }
    (void)querySyncWaterMarkHelper_.Initialize(storage);
    return LoadAllMetadata();
}

int Metadata::SaveTimeOffset(const DeviceID &deviceId, TimeOffset inValue)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, true);
    metadata.timeOffset = inValue;
    metadata.lastUpdateTime = TimeHelper::GetSysCurrentTime();
    LOGD("Metadata::SaveTimeOffset = %lld dev %s", inValue, STR_MASK(deviceId));
    return SaveMetaDataValue(deviceId, metadata);
}

void Metadata::GetTimeOffset(const DeviceID &deviceId, TimeOffset &outValue)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, true);
    outValue = metadata.timeOffset;
}

void Metadata::GetLocalWaterMark(const DeviceID &deviceId, uint64_t &outValue)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, true);
    outValue = metadata.localWaterMark;
}

int Metadata::SaveLocalWaterMark(const DeviceID &deviceId, uint64_t inValue)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, true);
    metadata.localWaterMark = inValue;
    LOGD("Metadata::SaveLocalWaterMark = %llu", inValue);
    return SaveMetaDataValue(deviceId, metadata);
}

void Metadata::GetPeerWaterMark(const DeviceID &deviceId, uint64_t &outValue)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, true);
    outValue = metadata.peerWaterMark;
}

int Metadata::SavePeerWaterMark(const DeviceID &deviceId, uint64_t inValue, bool isNeedHash)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    GetMetaDataValue(deviceId, metadata, isNeedHash);
    metadata.peerWaterMark = inValue;
    LOGD("Metadata::SavePeerWaterMark = %llu", inValue);
    return SaveMetaDataValue(deviceId, metadata);
}

int Metadata::SaveLocalTimeOffset(TimeOffset timeOffset)
{
    std::string timeOffsetString = std::to_string(timeOffset);
    std::vector<uint8_t> timeOffsetValue(timeOffsetString.begin(), timeOffsetString.end());
    std::vector<uint8_t> localTimeOffsetValue(
        LOCALTIME_OFFSET_KEY.begin(), LOCALTIME_OFFSET_KEY.end());

    std::lock_guard<std::mutex> lockGuard(localTimeOffsetLock_);
    localTimeOffset_ = timeOffset;
    LOGD("Metadata::SaveLocalTimeOffset offset = %lld", timeOffset);
    int errCode = SetMetadataToDb(localTimeOffsetValue, timeOffsetValue);
    if (errCode != E_OK) {
        LOGE("Metadata::SaveLocalTimeOffset SetMetadataToDb failed errCode:%d", errCode);
    }
    return errCode;
}

TimeOffset Metadata::GetLocalTimeOffset() const
{
    TimeOffset localTimeOffset = localTimeOffset_.load(std::memory_order_seq_cst);
    return localTimeOffset;
}

int Metadata::EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash)
{
    return EraseDeviceWaterMark(deviceId, isNeedHash, "");
}

int Metadata::EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash, const std::string &tableName)
{
    // try to erase all the waterMark
    // erase deleteSync recv waterMark
    WaterMark waterMark = 0;
    int errCodeDeleteSync = SetRecvDeleteSyncWaterMark(deviceId, waterMark);
    // erase querySync recv waterMark
    int errCodeQuerySync = ResetRecvQueryWaterMark(deviceId, tableName);
    // peerWaterMark must be erased at last
    int errCode = SavePeerWaterMark(deviceId, 0, isNeedHash);
    if (errCode != E_OK) {
        LOGE("[Metadata] erase peerWaterMark failed errCode:%d", errCode);
        return errCode;
    }
    if (errCodeQuerySync != E_OK) {
        LOGE("[Metadata] erase queryWaterMark failed errCode:%d", errCodeQuerySync);
        return errCodeQuerySync;
    }
    if (errCodeDeleteSync != E_OK) {
        LOGE("[Metadata] erase deleteWaterMark failed errCode:%d", errCodeDeleteSync);
        return errCodeDeleteSync;
    }
    return E_OK;
}

void Metadata::SetLastLocalTime(TimeStamp lastLocalTime)
{
    std::lock_guard<std::mutex> lock(lastLocalTimeLock_);
    if (lastLocalTime > lastLocalTime_) {
        lastLocalTime_ = lastLocalTime;
    }
}

TimeStamp Metadata::GetLastLocalTime() const
{
    std::lock_guard<std::mutex> lock(lastLocalTimeLock_);
    return lastLocalTime_;
}

int Metadata::SaveMetaDataValue(const DeviceID &deviceId, const MetaDataValue &inValue)
{
    std::vector<uint8_t> value;
    int errCode = SerializeMetaData(inValue, value);
    if (errCode != E_OK) {
        return errCode;
    }

    DeviceID hashDeviceId;
    GetHashDeviceId(deviceId, hashDeviceId, true);
    std::vector<uint8_t> key;
    DBCommon::StringToVector(hashDeviceId, key);
    errCode = SetMetadataToDb(key, value);
    if (errCode != E_OK) {
        LOGE("Metadata::SetMetadataToDb failed errCode:%d", errCode);
        return errCode;
    }
    PutMetadataToMap(hashDeviceId, inValue);
    return E_OK;
}

void Metadata::GetMetaDataValue(const DeviceID &deviceId, MetaDataValue &outValue, bool isNeedHash)
{
    DeviceID hashDeviceId;
    GetHashDeviceId(deviceId, hashDeviceId, isNeedHash);
    GetMetadataFromMap(hashDeviceId, outValue);
}

int Metadata::SerializeMetaData(const MetaDataValue &inValue, std::vector<uint8_t> &outValue)
{
    outValue.resize(sizeof(MetaDataValue));
    errno_t err = memcpy_s(&outValue[0], outValue.size(), &inValue, sizeof(MetaDataValue));
    if (err != EOK) {
        return -E_SECUREC_ERROR;
    }
    return E_OK;
}

int Metadata::DeSerializeMetaData(const std::vector<uint8_t> &inValue, MetaDataValue &outValue) const
{
    if (inValue.empty()) {
        return -E_INVALID_ARGS;
    }

    errno_t err = memcpy_s(&outValue, sizeof(MetaDataValue), &inValue[0], inValue.size());
    if (err != EOK) {
        return -E_SECUREC_ERROR;
    }
    return E_OK;
}

int Metadata::GetMetadataFromDb(const std::vector<uint8_t> &key, std::vector<uint8_t> &outValue) const
{
    if (naturalStoragePtr_ == nullptr) {
        return -E_INVALID_DB;
    }
    return naturalStoragePtr_->GetMetaData(key, outValue);
}

int Metadata::SetMetadataToDb(const std::vector<uint8_t> &key, const std::vector<uint8_t> &inValue)
{
    if (naturalStoragePtr_ == nullptr) {
        return -E_INVALID_DB;
    }
    return naturalStoragePtr_->PutMetaData(key, inValue);
}

int Metadata::DeleteMetaDataFromDB(const std::vector<Key> &keys) const
{
    if (naturalStoragePtr_ == nullptr) {
        return -E_INVALID_DB;
    }
    return naturalStoragePtr_->DeleteMetaData(keys);
}

void Metadata::PutMetadataToMap(const DeviceID &deviceId, const MetaDataValue &value)
{
    metadataMap_[deviceId] = value;
}

void Metadata::GetMetadataFromMap(const DeviceID &deviceId, MetaDataValue &outValue)
{
    outValue = metadataMap_[deviceId];
}

int64_t Metadata::StringToLong(const std::vector<uint8_t> &value) const
{
    std::string valueString(value.begin(), value.end());
    int64_t longData = std::strtoll(valueString.c_str(), nullptr, STR_TO_LL_BY_DEVALUE);
    LOGD("Metadata::StringToLong longData = %lld", longData);
    return longData;
}

int Metadata::GetAllMetadataKey(std::vector<std::vector<uint8_t>> &keys)
{
    if (naturalStoragePtr_ == nullptr) {
        return -E_INVALID_DB;
    }
    return naturalStoragePtr_->GetAllMetaKeys(keys);
}

namespace {
bool IsMetaDataKey(const Key &inKey, const std::string &expectPrefix)
{
    if (inKey.size() < expectPrefix.size()) {
        return false;
    }
    std::string prefixInKey(inKey.begin(), inKey.begin() + expectPrefix.size());
    if (prefixInKey != expectPrefix) {
        return false;
    }
    return true;
}
}

int Metadata::LoadAllMetadata()
{
    std::vector<std::vector<uint8_t>> metaDataKeys;
    int errCode = GetAllMetadataKey(metaDataKeys);
    if (errCode != E_OK) {
        LOGE("[Metadata] get all metadata key failed err=%d", errCode);
        return errCode;
    }

    std::vector<std::vector<uint8_t>> querySyncIds;
    for (const auto &deviceId : metaDataKeys) {
        if (IsMetaDataKey(deviceId, DEVICEID_PREFIX_KEY)) {
            int errCode = LoadDeviceIdDataToMap(deviceId);
            if (errCode != E_OK) {
                return errCode;
            }
        } else if (IsMetaDataKey(deviceId, QuerySyncWaterMarkHelper::GetQuerySyncPrefixKey())) {
            querySyncIds.push_back(deviceId);
        } else if (IsMetaDataKey(deviceId, QuerySyncWaterMarkHelper::GetDeleteSyncPrefixKey())) {
            int errCode = querySyncWaterMarkHelper_.LoadDeleteSyncDataToCache(deviceId);
            if (errCode != E_OK) {
                return errCode;
            }
        }
    }
    return querySyncWaterMarkHelper_.RemoveLeastUsedQuerySyncItems(querySyncIds);
}

int Metadata::LoadDeviceIdDataToMap(const Key &key)
{
    std::vector<uint8_t> value;
    int errCode = GetMetadataFromDb(key, value);
    if (errCode != E_OK) {
        return errCode;
    }
    MetaDataValue metaValue;
    std::string metaKey(key.begin(), key.end());
    errCode = DeSerializeMetaData(value, metaValue);
    if (errCode != E_OK) {
        return errCode;
    }
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    PutMetadataToMap(metaKey, metaValue);
    return errCode;
}

uint64_t Metadata::GetRandTimeOffset() const
{
    const int randOffsetLength = 2; // 2 byte
    uint8_t randBytes[randOffsetLength] = { 0 };
    RAND_bytes(randBytes, randOffsetLength);

    // use a 16 bit rand data to make a rand timeoffset
    uint64_t randTimeOffset = (static_cast<uint16_t>(randBytes[1]) << 8) | randBytes[0]; // 16 bit data, 8 is offset
    randTimeOffset = randTimeOffset * 1000 * 1000 * 10; // second, 1000 is scale
    LOGD("[Metadata] GetRandTimeOffset %llu", randTimeOffset);
    return randTimeOffset;
}

void Metadata::GetHashDeviceId(const DeviceID &deviceId, DeviceID &hashDeviceId, bool isNeedHash)
{
    if (!isNeedHash) {
        hashDeviceId = deviceId;
        return;
    }
    if (deviceIdToHashDeviceIdMap_.count(deviceId) == 0) {
        hashDeviceId = DEVICEID_PREFIX_KEY + DBCommon::TransferHashString(deviceId);
        deviceIdToHashDeviceIdMap_.insert(std::pair<DeviceID, DeviceID>(deviceId, hashDeviceId));
    } else {
        hashDeviceId = deviceIdToHashDeviceIdMap_[deviceId];
    }
}

int Metadata::GetRecvQueryWaterMark(const std::string &queryIdentify,
    const std::string &deviceId, WaterMark &waterMark)
{
    QueryWaterMark queryWaterMark;
    int errCode = querySyncWaterMarkHelper_.GetQueryWaterMark(queryIdentify, deviceId, queryWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    WaterMark peerWaterMark;
    GetPeerWaterMark(deviceId, peerWaterMark);
    waterMark = std::max(queryWaterMark.recvWaterMark, peerWaterMark);
    return E_OK;
}

int Metadata::SetRecvQueryWaterMark(const std::string &queryIdentify,
    const std::string &deviceId, const WaterMark &waterMark)
{
    return querySyncWaterMarkHelper_.SetRecvQueryWaterMark(queryIdentify, deviceId, waterMark);
}

int Metadata::GetSendQueryWaterMark(const std::string &queryIdentify,
    const std::string &deviceId, WaterMark &waterMark, bool isAutoLift)
{
    QueryWaterMark queryWaterMark;
    int errCode = querySyncWaterMarkHelper_.GetQueryWaterMark(queryIdentify, deviceId, queryWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    if (isAutoLift) {
        WaterMark localWaterMark;
        GetLocalWaterMark(deviceId, localWaterMark);
        waterMark = std::max(queryWaterMark.sendWaterMark, localWaterMark);
    } else {
        waterMark = queryWaterMark.sendWaterMark;
    }
    return E_OK;
}

int Metadata::SetSendQueryWaterMark(const std::string &queryIdentify,
    const std::string &deviceId, const WaterMark &waterMark)
{
    return querySyncWaterMarkHelper_.SetSendQueryWaterMark(queryIdentify, deviceId, waterMark);
}

int Metadata::GetLastQueryTime(const std::string &queryIdentify, const std::string &deviceId, TimeStamp &timeStamp)
{
    QueryWaterMark queryWaterMark;
    int errCode = querySyncWaterMarkHelper_.GetQueryWaterMark(queryIdentify, deviceId, queryWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    timeStamp = queryWaterMark.lastQueryTime;
    return E_OK;
}

int Metadata::SetLastQueryTime(const std::string &queryIdentify, const std::string &deviceId,
    const TimeStamp &timeStamp)
{
    return querySyncWaterMarkHelper_.SetLastQueryTime(queryIdentify, deviceId, timeStamp);
}

int Metadata::GetSendDeleteSyncWaterMark(const DeviceID &deviceId, WaterMark &waterMark, bool isAutoLift)
{
    DeleteWaterMark deleteWaterMark;
    int errCode = querySyncWaterMarkHelper_.GetDeleteSyncWaterMark(deviceId, deleteWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    if (isAutoLift) {
        WaterMark localWaterMark;
        GetLocalWaterMark(deviceId, localWaterMark);
        waterMark = std::max(deleteWaterMark.sendWaterMark, localWaterMark);
    } else {
        waterMark = deleteWaterMark.sendWaterMark;
    }
    return E_OK;
}

int Metadata::SetSendDeleteSyncWaterMark(const DeviceID &deviceId, const WaterMark &waterMark)
{
    return querySyncWaterMarkHelper_.SetSendDeleteSyncWaterMark(deviceId, waterMark);
}

int Metadata::GetRecvDeleteSyncWaterMark(const DeviceID &deviceId, WaterMark &waterMark)
{
    DeleteWaterMark deleteWaterMark;
    int errCode = querySyncWaterMarkHelper_.GetDeleteSyncWaterMark(deviceId, deleteWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    WaterMark peerWaterMark;
    GetPeerWaterMark(deviceId, peerWaterMark);
    waterMark = std::max(deleteWaterMark.recvWaterMark, peerWaterMark);
    return E_OK;
}

int Metadata::SetRecvDeleteSyncWaterMark(const DeviceID &deviceId, const WaterMark &waterMark)
{
    return querySyncWaterMarkHelper_.SetRecvDeleteSyncWaterMark(deviceId, waterMark);
}

int Metadata::ResetRecvQueryWaterMark(const DeviceID &deviceId, const std::string &tableName)
{
    return querySyncWaterMarkHelper_.ResetRecvQueryWaterMark(deviceId, tableName);
}

void Metadata::GetDbCreateTime(const DeviceID &deviceId, uint64_t &outValue)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    DeviceID hashDeviceId;
    GetHashDeviceId(deviceId, hashDeviceId, true);
    if (metadataMap_.find(hashDeviceId) != metadataMap_.end()) {
        metadata = metadataMap_[hashDeviceId];
        outValue = metadata.dbCreateTime;
        return;
    }
    outValue = 0;
    LOGI("Metadata::GetDbCreateTime, not found dev = %s dbCreateTime", STR_MASK(deviceId));
}

int Metadata::SetDbCreateTime(const DeviceID &deviceId, uint64_t inValue, bool isNeedHash)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    DeviceID hashDeviceId;
    GetHashDeviceId(deviceId, hashDeviceId, true);
    if (metadataMap_.find(hashDeviceId) != metadataMap_.end()) {
        metadata = metadataMap_[hashDeviceId];
        if (metadata.dbCreateTime != 0 && metadata.dbCreateTime != inValue) {
            metadata.clearDeviceDataMark = REMOVE_DEVICE_DATA_MARK;
            LOGI("Metadata::SetDbCreateTime,set cleardata mark,dev=%s,dbCreateTime=%llu", STR_MASK(deviceId), inValue);
        }
        if (metadata.dbCreateTime == 0) {
            LOGI("Metadata::SetDbCreateTime,update dev=%s,dbCreateTime=%llu", STR_MASK(deviceId), inValue);
        }
    }
    metadata.dbCreateTime = inValue;
    return SaveMetaDataValue(deviceId, metadata);
}

int Metadata::ResetMetaDataAfterRemoveData(const DeviceID &deviceId)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    DeviceID hashDeviceId;
    GetHashDeviceId(deviceId, hashDeviceId, true);
    if (metadataMap_.find(hashDeviceId) != metadataMap_.end()) {
        metadata = metadataMap_[hashDeviceId];
        metadata.clearDeviceDataMark = 0;
        // when finished remove data, should reset send watermark between high version sync
        // recv watermark has already reset in removedata func.
        metadata.localWaterMark = 0;
        return SaveMetaDataValue(deviceId, metadata);
    }
    return -E_NOT_FOUND;
}

void Metadata::GetRemoveDataMark(const DeviceID &deviceId, uint64_t &outValue)
{
    MetaDataValue metadata;
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    DeviceID hashDeviceId;
    GetHashDeviceId(deviceId, hashDeviceId, true);
    if (metadataMap_.find(hashDeviceId) != metadataMap_.end()) {
        metadata = metadataMap_[hashDeviceId];
        outValue = metadata.clearDeviceDataMark;
        return;
    }
    outValue = 0;
}

int Metadata::DeleteMetaDataByPrefixKey(const Key &keyPrefix) const
{
    if (naturalStoragePtr_ == nullptr) {
        return -E_INVALID_DB;
    }
    return naturalStoragePtr_->DeleteMetaDataByPrefixKey(keyPrefix);
}

uint64_t Metadata::GetQueryLastTimestamp(const DeviceID &deviceId, const std::string &queryId) const
{
    std::vector<uint8_t> key;
    std::vector<uint8_t> value;
    std::string hashqueryId = DBConstant::SUBSCRIBE_QUERY_PREFIX + DBCommon::TransferHashString(queryId);
    DBCommon::StringToVector(hashqueryId, key);
    int errCode = GetMetadataFromDb(key, value);
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    if (errCode == -E_NOT_FOUND) {
        auto iter = queryIdMap_.find(deviceId);
        if (iter != queryIdMap_.end()) {
            if (iter->second.find(hashqueryId) == iter->second.end()) {
                iter->second.insert(hashqueryId);
                return INT64_MAX;
            }
            return 0;
        } else {
            queryIdMap_[deviceId] = { hashqueryId };
            return INT64_MAX;
        }
    }
    auto iter = queryIdMap_.find(deviceId);
    // while value is found in db, it can be found in db later when db is not closed
    // so no need to record the hashqueryId in map
    if (errCode == E_OK && iter != queryIdMap_.end()) {
        iter->second.erase(hashqueryId);
    }
    return StringToLong(value);
}

void Metadata::RemoveQueryFromRecordSet(const DeviceID &deviceId, const std::string &queryId)
{
    std::lock_guard<std::mutex> lockGuard(metadataLock_);
    std::string hashqueryId = DBConstant::SUBSCRIBE_QUERY_PREFIX + DBCommon::TransferHashString(queryId);
    auto iter = queryIdMap_.find(deviceId);
    if (iter != queryIdMap_.end() && iter->second.find(hashqueryId) != iter->second.end()) {
        iter->second.erase(hashqueryId);
    }
}
}  // namespace DistributedDB