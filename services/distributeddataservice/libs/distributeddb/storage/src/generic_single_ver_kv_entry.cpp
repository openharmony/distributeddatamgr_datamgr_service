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

#include "generic_single_ver_kv_entry.h"

#include <algorithm>
#include "data_compression.h"
#include "db_errno.h"
#include "parcel.h"
#include "version.h"

namespace DistributedDB {
GenericSingleVerKvEntry::GenericSingleVerKvEntry()
{
}

GenericSingleVerKvEntry::~GenericSingleVerKvEntry()
{
}

std::string GenericSingleVerKvEntry::GetOrigDevice() const
{
    return dataItem_.origDev;
}

void GenericSingleVerKvEntry::SetOrigDevice(const std::string &dev)
{
    dataItem_.origDev = dev;
}

Timestamp GenericSingleVerKvEntry::GetTimestamp() const
{
    return dataItem_.timestamp;
}

void GenericSingleVerKvEntry::SetTimestamp(Timestamp time)
{
    dataItem_.timestamp = time;
}

Timestamp GenericSingleVerKvEntry::GetWriteTimestamp() const
{
    return dataItem_.writeTimestamp;
}

void GenericSingleVerKvEntry::SetWriteTimestamp(Timestamp time)
{
    dataItem_.writeTimestamp = time;
}

void GenericSingleVerKvEntry::SetEntryData(DataItem &&dataItem)
{
    dataItem_ = dataItem;
}

void GenericSingleVerKvEntry::GetKey(Key &key) const
{
    key = dataItem_.key;
}

void GenericSingleVerKvEntry::GetHashKey(Key &key) const
{
    key = dataItem_.hashKey;
}

const Key &GenericSingleVerKvEntry::GetKey() const
{
    return dataItem_.key;
}

void GenericSingleVerKvEntry::GetValue(Value &value) const
{
    value = dataItem_.value;
}

const Value &GenericSingleVerKvEntry::GetValue() const
{
    return dataItem_.value;
}

uint64_t GenericSingleVerKvEntry::GetFlag() const
{
    return dataItem_.flag;
}

void GenericSingleVerKvEntry::SetKey(const Key &key)
{
    dataItem_.key = key;
}

void GenericSingleVerKvEntry::SetValue(const Value &value)
{
    dataItem_.value = value;
}

void GenericSingleVerKvEntry::SetHashKey(const Key &hashKey)
{
    dataItem_.hashKey = hashKey;
}

// this func should do compatible
int GenericSingleVerKvEntry::SerializeData(Parcel &parcel, uint32_t targetVersion)
{
    uint64_t len = 0;
    int errCode = parcel.WriteUInt32(targetVersion);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = AdaptToVersion(OperType::SERIALIZE, targetVersion, parcel, len);
    if (errCode != E_OK) {
        return errCode;
    }
    return errCode;
}

int GenericSingleVerKvEntry::SerializeDatas(const std::vector<SingleVerKvEntry *> &kvEntries, Parcel &parcel,
    uint32_t targetVersion)
{
    uint32_t size = kvEntries.size();
    int errCode = parcel.WriteUInt32(size);
    if (errCode != E_OK) {
        LOGE("[SerializeDatas] write entries size failed, errCode=%d.", errCode);
        return errCode;
    }
    parcel.EightByteAlign();
    for (const auto &kvEntry : kvEntries) {
        if (kvEntry == nullptr) {
            continue;
        }
        errCode = kvEntry->SerializeData(parcel, targetVersion);
        if (errCode != E_OK) {
            LOGE("[SerializeDatas] write kvEntry failed, errCode=%d.", errCode);
            return errCode;
        }
    }
    return errCode;
}

// this func should do compatible
uint32_t GenericSingleVerKvEntry::CalculateLen(uint32_t targetVersion)
{
    uint64_t len = 0;
    int errCode = AdaptToVersion(OperType::CAL_LEN, targetVersion, len);
    if ((len > INT32_MAX) || (errCode != E_OK)) {
        return 0;
    }
    return len;
}

uint32_t GenericSingleVerKvEntry::CalculateLens(const std::vector<SingleVerKvEntry *> &kvEntries,
    uint32_t targetVersion)
{
    uint64_t len = 0;
    len += Parcel::GetUInt32Len();
    len = BYTE_8_ALIGN(len);
    for (const auto &kvEntry : kvEntries) {
        if (kvEntry == nullptr) {
            continue;
        }
        len += kvEntry->CalculateLen(targetVersion);
        if (len > INT32_MAX) {
            return 0;
        }
    }
    return len;
}

// this func should do compatible
int GenericSingleVerKvEntry::DeSerializeData(Parcel &parcel)
{
    uint32_t version = VERSION_INVALID;
    uint64_t len = parcel.ReadUInt32(version);
    if (parcel.IsError()) {
        return 0;
    }
    int errCode = AdaptToVersion(OperType::DESERIALIZE, version, parcel, len);
    if (errCode != E_OK) {
        len = 0;
    }
    return len;
}

int GenericSingleVerKvEntry::DeSerializeDatas(std::vector<SingleVerKvEntry *> &kvEntries, Parcel &parcel)
{
    uint64_t len = 0;
    uint32_t size = 0;
    len += parcel.ReadUInt32(size);
    parcel.EightByteAlign();
    len = BYTE_8_ALIGN(len);
    for (uint32_t i = 0; i < size; i++) {
        auto kvEntry = new (std::nothrow) GenericSingleVerKvEntry();
        if (kvEntry == nullptr) {
            LOGE("Create kvEntry failed.");
            len = 0;
            goto END;
        }
        len += kvEntry->DeSerializeData(parcel);
        kvEntries.push_back(kvEntry);
        if (len > INT32_MAX) {
            len = 0;
            goto END;
        }
    }
END:
    if (len == 0) {
        for (auto &kvEntry : kvEntries) {
            delete kvEntry;
            kvEntry = nullptr;
        }
    }
    return len;
}

int GenericSingleVerKvEntry::AdaptToVersion(OperType operType, uint32_t targetVersion, Parcel &parcel,
    uint64_t &datalen)
{
    if (targetVersion < SOFTWARE_VERSION_EARLIEST || targetVersion > SOFTWARE_VERSION_CURRENT) {
        return -E_VERSION_NOT_SUPPORT;
    }
    int errCode = E_OK;
    switch (operType) {
        case OperType::SERIALIZE:
            errCode = SerializeDataByVersion(targetVersion, parcel);
            break;
        case OperType::DESERIALIZE:
            errCode = DeSerializeByVersion(targetVersion, parcel, datalen);
            break;
        default:
            LOGE("Unknown upgrade serialize oper!");
            return -E_UPGRADE_FAILED;
    }
    return errCode;
}

int GenericSingleVerKvEntry::AdaptToVersion(OperType operType, uint32_t targetVersion, uint64_t &datalen)
{
    if (targetVersion < SOFTWARE_VERSION_EARLIEST || targetVersion > SOFTWARE_VERSION_CURRENT) {
        return -E_VERSION_NOT_SUPPORT;
    }

    if (operType == OperType::CAL_LEN) {
        return CalLenByVersion(targetVersion, datalen);
    } else {
        LOGE("Unknown upgrade serialize oper!");
        return -E_UPGRADE_FAILED;
    }
}

int GenericSingleVerKvEntry::SerializeDataByFirstVersion(Parcel &parcel) const
{
    int errCode = parcel.WriteVectorChar(dataItem_.key);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = parcel.WriteVectorChar(dataItem_.value);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = parcel.WriteUInt64(dataItem_.timestamp);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = parcel.WriteUInt64(dataItem_.flag);
    if (errCode != E_OK) {
        return errCode;
    }

    return parcel.WriteString(dataItem_.origDev);
}

int GenericSingleVerKvEntry::SerializeDataByLaterVersion(Parcel &parcel, uint32_t targetVersion) const
{
    Timestamp writeTimestamp = dataItem_.writeTimestamp;
    if (writeTimestamp == 0) {
        writeTimestamp = dataItem_.timestamp;
    }
    int errCode = parcel.WriteUInt64(writeTimestamp);
    if (errCode != E_OK) {
        return errCode;
    }
    if (targetVersion >= SOFTWARE_VERSION_RELEASE_6_0) {
        errCode = parcel.WriteVector(dataItem_.hashKey);
    }
    return errCode;
}

int GenericSingleVerKvEntry::SerializeDataByVersion(uint32_t targetVersion, Parcel &parcel) const
{
    int errCode = SerializeDataByFirstVersion(parcel);
    if (targetVersion == SOFTWARE_VERSION_EARLIEST || errCode != E_OK) {
        return errCode;
    }
    return SerializeDataByLaterVersion(parcel, targetVersion);
}

void GenericSingleVerKvEntry::CalLenByFirstVersion(uint64_t &len) const
{
    len += Parcel::GetUInt32Len();
    len += Parcel::GetVectorCharLen(dataItem_.key);
    len += Parcel::GetVectorCharLen(dataItem_.value);
    len += Parcel::GetUInt64Len();
    len += Parcel::GetUInt64Len();
    len += Parcel::GetStringLen(dataItem_.origDev);
}

void GenericSingleVerKvEntry::CalLenByLaterVersion(uint64_t &len, uint32_t targetVersion) const
{
    len += Parcel::GetUInt64Len();
    if (targetVersion >= SOFTWARE_VERSION_RELEASE_6_0) {
        len += Parcel::GetVectorLen(dataItem_.hashKey);
    }
}

int GenericSingleVerKvEntry::CalLenByVersion(uint32_t targetVersion, uint64_t &len) const
{
    CalLenByFirstVersion(len);
    if (targetVersion == SOFTWARE_VERSION_EARLIEST) {
        return E_OK;
    }
    CalLenByLaterVersion(len, targetVersion);
    return E_OK;
}

void GenericSingleVerKvEntry::DeSerializeByFirstVersion(uint64_t &len, Parcel &parcel)
{
    len += parcel.ReadVectorChar(dataItem_.key);
    len += parcel.ReadVectorChar(dataItem_.value);
    len += parcel.ReadUInt64(dataItem_.timestamp);
    len += parcel.ReadUInt64(dataItem_.flag);
    len += parcel.ReadString(dataItem_.origDev);
    dataItem_.writeTimestamp = dataItem_.timestamp;
}

void GenericSingleVerKvEntry::DeSerializeByLaterVersion(uint64_t &len, Parcel &parcel, uint32_t targetVersion)
{
    len += parcel.ReadUInt64(dataItem_.writeTimestamp);
    if (targetVersion >= SOFTWARE_VERSION_RELEASE_6_0) {
        len += parcel.ReadVector(dataItem_.hashKey);
    }
}

int GenericSingleVerKvEntry::DeSerializeByVersion(uint32_t targetVersion, Parcel &parcel, uint64_t &len)
{
    DeSerializeByFirstVersion(len, parcel);
    if (targetVersion == SOFTWARE_VERSION_EARLIEST) {
        return E_OK;
    }
    DeSerializeByLaterVersion(len, parcel, targetVersion);
    return E_OK;
}

uint32_t GenericSingleVerKvEntry::CalculateCompressedLens(const std::vector<uint8_t> &compressedData)
{
    // No compressed data in sync.
    if (compressedData.empty()) {
        return 0;
    }

    // Calculate compressed data length.
    uint64_t len = 0;
    len += Parcel::GetUInt32Len(); // srcLen.
    len += Parcel::GetUInt32Len(); // compression algorithm type.
    len += Parcel::GetVectorLen(compressedData); // compressed data.
    return (len > INT32_MAX) ? 0 : len;
}

int GenericSingleVerKvEntry::Compress(const std::vector<SingleVerKvEntry *> &kvEntries, std::vector<uint8_t> &destData,
    const CompressInfo &compressInfo)
{
    // Calculate length.
    auto srcLen = CalculateLens(kvEntries, compressInfo.targetVersion);
    if (srcLen == 0) {
        LOGE("Over limit size, cannot compress.");
        return -E_INVALID_ARGS;
    }

    // Serialize data.
    std::vector<uint8_t> srcData(srcLen, 0);
    Parcel parcel(srcData.data(), srcData.size());
    int errCode = SerializeDatas(kvEntries, parcel, compressInfo.targetVersion);
    if (errCode != E_OK) {
        return errCode;
    }

    // Compress data.
    auto inst = DataCompression::GetInstance(compressInfo.compressAlgo);
    if (inst == nullptr) {
        return -E_INVALID_COMPRESS_ALGO;
    }
    return inst->Compress(srcData, destData);
}

int GenericSingleVerKvEntry::Uncompress(const std::vector<uint8_t> &srcData, std::vector<SingleVerKvEntry *> &kvEntries,
    uint32_t destLen, CompressAlgorithm algo)
{
    // Uncompress data.
    std::vector<uint8_t> destData(destLen, 0);
    auto inst = DataCompression::GetInstance(algo);
    if (inst == nullptr) {
        return -E_INVALID_COMPRESS_ALGO;
    }
    int errCode = inst->Uncompress(srcData, destData, destLen);
    if (errCode != E_OK) {
        return errCode;
    }

    // Deserialize data.
    Parcel parcel(destData.data(), destData.size());
    if (DeSerializeDatas(kvEntries, parcel) == 0) {
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

int GenericSingleVerKvEntry::SerializeCompressedDatas(const std::vector<SingleVerKvEntry *> &kvEntries,
    const std::vector<uint8_t> &compressedEntries, Parcel &parcel, uint32_t targetVersion, CompressAlgorithm algo)
{
    uint32_t srcLen = CalculateLens(kvEntries, targetVersion);
    (void)parcel.WriteUInt32(static_cast<uint32_t>(algo));
    (void)parcel.WriteUInt32(srcLen);
    (void)parcel.WriteVector(compressedEntries);
    return parcel.IsError() ? -E_PARSE_FAIL : E_OK;
}

int GenericSingleVerKvEntry::DeSerializeCompressedDatas(std::vector<SingleVerKvEntry *> &kvEntries, Parcel &parcel)
{
    // Get compression algo type.
    uint32_t algoType = 0;
    (void)parcel.ReadUInt32(algoType);
    CompressAlgorithm compressAlgo = CompressAlgorithm::NONE;
    int errCode = DataCompression::TransferCompressionAlgo(algoType, compressAlgo);
    if (errCode != E_OK) {
        return errCode;
    }

    // Get buffer length.
    uint32_t destLen = 0;
    (void)parcel.ReadUInt32(destLen);

    // Get compressed data.
    std::vector<uint8_t> srcData;
    (void)parcel.ReadVector(srcData);
    if (parcel.IsError()) {
        return -E_PARSE_FAIL;
    }

    // Uncompress data.
    return GenericSingleVerKvEntry::Uncompress(srcData, kvEntries, destLen, compressAlgo);
}
} // namespace DistributedDB
