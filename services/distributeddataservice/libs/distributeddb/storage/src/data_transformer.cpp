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
#ifdef RELATIONAL_STORE
#include "data_transformer.h"

#include "db_common.h"
#include "db_errno.h"
#include "log_print.h"
#include "parcel.h"
#include "relational_schema_object.h"

namespace DistributedDB {
int DataTransformer::TransformTableData(const TableDataWithLog &tableDataWithLog,
    const std::vector<FieldInfo> &fieldInfoList, std::vector<DataItem> &dataItems)
{
    if (tableDataWithLog.dataList.empty()) {
        return E_OK;
    }
    for (const RowDataWithLog& data : tableDataWithLog.dataList) {
        DataItem dataItem;
        int errCode = SerializeDataItem(data, fieldInfoList, dataItem);
        if (errCode != E_OK) {
            return errCode;
        }
        dataItems.push_back(std::move(dataItem));
    }
    return E_OK;
}

int DataTransformer::TransformDataItem(const std::vector<DataItem> &dataItems,
    const std::vector<FieldInfo> &remoteFieldInfo, const std::vector<FieldInfo> &localFieldInfo,
    OptTableDataWithLog &tableDataWithLog)
{
    if (dataItems.empty()) {
        return E_OK;
    }
    std::vector<int> indexMapping;
    ReduceMapping(remoteFieldInfo, localFieldInfo);
    for (const DataItem &dataItem : dataItems) {
        OptRowDataWithLog dataWithLog;
        int errCode = DeSerializeDataItem(dataItem, dataWithLog, remoteFieldInfo);
        if (errCode != E_OK) {
            return errCode;
        }
        tableDataWithLog.dataList.push_back(std::move(dataWithLog));
    }
    return E_OK;
}

int DataTransformer::SerializeDataItem(const RowDataWithLog &data,
    const std::vector<FieldInfo> &fieldInfo, DataItem &dataItem)
{
    int errCode = SerializeValue(dataItem.value, data.rowData, fieldInfo);
    if (errCode != E_OK) {
        return errCode;
    }
    const LogInfo &logInfo = data.logInfo;
    dataItem.timestamp = logInfo.timestamp;
    dataItem.dev = logInfo.device;
    dataItem.origDev = logInfo.originDev;
    dataItem.writeTimestamp = logInfo.wTimestamp;
    dataItem.flag = logInfo.flag;
    dataItem.hashKey = logInfo.hashKey;
    return E_OK;
}

int DataTransformer::DeSerializeDataItem(const DataItem &dataItem, OptRowDataWithLog &data,
    const std::vector<FieldInfo> &remoteFieldInfo)
{
    if ((dataItem.flag & DataItem::DELETE_FLAG) == 0 &&
        (dataItem.flag & DataItem::REMOTE_DEVICE_DATA_MISS_QUERY) == 0) {
        int errCode = DeSerializeValue(dataItem.value, data.optionalData, remoteFieldInfo);
        if (errCode != E_OK) {
            return errCode;
        }
    }

    LogInfo &logInfo = data.logInfo;
    logInfo.timestamp = dataItem.timestamp;
    logInfo.device = dataItem.dev;
    logInfo.originDev = dataItem.origDev;
    logInfo.wTimestamp = dataItem.writeTimestamp;
    logInfo.flag = dataItem.flag;
    logInfo.hashKey = dataItem.hashKey;
    return E_OK;
}

uint32_t DataTransformer::CalDataValueLength(const DataValue &dataValue)
{
    static std::map<StorageType, uint32_t> lengthMap = {
        { StorageType::STORAGE_TYPE_NULL, Parcel::GetUInt32Len()},
        { StorageType::STORAGE_TYPE_INTEGER, Parcel::GetInt64Len()},
        { StorageType::STORAGE_TYPE_REAL, Parcel::GetDoubleLen()}
    };
    if (lengthMap.find(dataValue.GetType()) != lengthMap.end()) {
        return lengthMap[dataValue.GetType()];
    }
    if (dataValue.GetType() != StorageType::STORAGE_TYPE_BLOB &&
        dataValue.GetType() != StorageType::STORAGE_TYPE_TEXT) {
        return 0u;
    }
    uint32_t length = 0;
    switch (dataValue.GetType()) {
        case StorageType::STORAGE_TYPE_BLOB:
        case StorageType::STORAGE_TYPE_TEXT:
            (void)dataValue.GetBlobLength(length);
            length = Parcel::GetEightByteAlign(length);
            length += Parcel::GetUInt32Len(); // record data length
            break;
        default:
            break;
    }
    return length;
}

void DataTransformer::ReduceMapping(const std::vector<FieldInfo> &remoteFieldInfo,
    const std::vector<FieldInfo> &localFieldInfo)
{
    std::map<std::string, int> fieldMap;
    for (int i = 0; i < static_cast<int>(remoteFieldInfo.size()); ++i) {
        const auto &fieldInfo = remoteFieldInfo[i];
        fieldMap[fieldInfo.GetFieldName()] = i;
    }
}

namespace {
int SerializeNullValue(const DataValue &dataValue, Parcel &parcel)
{
    return parcel.WriteUInt32(0u);
}

int DeSerializeNullValue(DataValue &dataValue, Parcel &parcel)
{
    uint32_t dataLength = -1;
    (void)parcel.ReadUInt32(dataLength);
    if (parcel.IsError() || dataLength != 0) {
        return -E_PARSE_FAIL;
    }
    dataValue.ResetValue();
    return E_OK;
}

int SerializeIntValue(const DataValue &dataValue, Parcel &parcel)
{
    int64_t val = 0;
    (void)dataValue.GetInt64(val);
    return parcel.WriteInt64(val);
}

int DeSerializeIntValue(DataValue &dataValue, Parcel &parcel)
{
    int64_t val = 0;
    (void)parcel.ReadInt64(val);
    if (parcel.IsError()) {
        return -E_PARSE_FAIL;
    }
    dataValue = val;
    return E_OK;
}

int SerializeDoubleValue(const DataValue &dataValue, Parcel &parcel)
{
    double val = 0;
    (void)dataValue.GetDouble(val);
    return parcel.WriteDouble(val);
}

int DeSerializeDoubleValue(DataValue &dataValue, Parcel &parcel)
{
    double val = 0;
    (void)parcel.ReadDouble(val);
    if (parcel.IsError()) {
        return -E_PARSE_FAIL;
    }
    dataValue = val;
    return E_OK;
}

int SerializeBlobValue(const DataValue &dataValue, Parcel &parcel)
{
    Blob val;
    (void)dataValue.GetBlob(val);
    uint32_t size = val.GetSize();
    if (size == 0) {
        return SerializeNullValue(dataValue, parcel);
    }
    int errCode = parcel.WriteUInt32(size);
    if (errCode != E_OK) {
        return errCode;
    }
    return parcel.WriteBlob(reinterpret_cast<const char *>(val.GetData()), size);
}

int DeSerializeBlobByType(DataValue &dataValue, Parcel &parcel, StorageType type)
{
    uint32_t blobLength = 0;
    (void)parcel.ReadUInt32(blobLength);
    if (blobLength == 0) {
        dataValue.ResetValue();
        return E_OK;
    }
    if (blobLength >= DBConstant::MAX_VALUE_SIZE || parcel.IsError()) { // One blob cannot be over one value size.
        return -E_PARSE_FAIL;
    }
    auto array = new (std::nothrow) char[blobLength]();
    if (array == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    (void)parcel.ReadBlob(array, blobLength);
    if (parcel.IsError()) {
        delete []array;
        return -E_PARSE_FAIL;
    }
    int errCode = -E_NOT_SUPPORT;
    if (type == StorageType::STORAGE_TYPE_TEXT) {
        errCode = dataValue.SetText(reinterpret_cast<const uint8_t *>(array), blobLength);
    } else if (type == StorageType::STORAGE_TYPE_BLOB) {
        Blob val;
        errCode = val.WriteBlob(reinterpret_cast<const uint8_t *>(array), blobLength);
        if (errCode == E_OK) {
            errCode = dataValue.SetBlob(val);
        }
    }
    delete []array;
    return errCode;
}

int DeSerializeBlobValue(DataValue &dataValue, Parcel &parcel)
{
    return DeSerializeBlobByType(dataValue, parcel, StorageType::STORAGE_TYPE_BLOB);
}

int SerializeTextValue(const DataValue &dataValue, Parcel &parcel)
{
    return SerializeBlobValue(dataValue, parcel);
}

int DeSerializeTextValue(DataValue &dataValue, Parcel &parcel)
{
    return DeSerializeBlobByType(dataValue, parcel, StorageType::STORAGE_TYPE_TEXT);
}

int SerializeDataValue(const DataValue &dataValue, Parcel &parcel)
{
    static const std::function<int(const DataValue&, Parcel&)> funcs[] = {
        SerializeNullValue, SerializeIntValue,
        SerializeDoubleValue, SerializeTextValue, SerializeBlobValue,
    };
    StorageType type = dataValue.GetType();
    parcel.WriteUInt32(static_cast<uint32_t>(type));
    if (type < StorageType::STORAGE_TYPE_NULL || type > StorageType::STORAGE_TYPE_BLOB) {
        LOGE("Cannot serialize %u", static_cast<unsigned>(type));
        return -E_NOT_SUPPORT;
    }
    return funcs[static_cast<uint32_t>(type) - 1](dataValue, parcel);
}

int DeserializeDataValue(DataValue &dataValue, Parcel &parcel)
{
    static const std::function<int(DataValue&, Parcel&)> funcs[] = {
        DeSerializeNullValue, DeSerializeIntValue,
        DeSerializeDoubleValue, DeSerializeTextValue, DeSerializeBlobValue,
    };
    uint32_t type = 0;
    parcel.ReadUInt32(type);
    if (type < static_cast<uint32_t>(StorageType::STORAGE_TYPE_NULL) ||
        type > static_cast<uint32_t>(StorageType::STORAGE_TYPE_BLOB)) {
        LOGE("Cannot deserialize %u", type);
        return -E_PARSE_FAIL;
    }
    return funcs[type - 1](dataValue, parcel);
}
}

int DataTransformer::SerializeValue(Value &value, const RowData &rowData, const std::vector<FieldInfo> &fieldInfoList)
{
    if (rowData.size() != fieldInfoList.size()) {
        LOGE("[DataTransformer][SerializeValue] unequal field counts!");
        return -E_INVALID_ARGS;
    }

    uint32_t totalLength = Parcel::GetUInt64Len(); // first record field count
    for (uint32_t i = 0; i < rowData.size(); ++i) {
        const auto &dataValue = rowData[i];
        totalLength += Parcel::GetUInt32Len(); // For save the dataValue's type.
        uint32_t dataLength = CalDataValueLength(dataValue);
        totalLength += dataLength;
    }
    value.resize(totalLength);
    if (value.size() != totalLength) {
        return -E_OUT_OF_MEMORY;
    }
    Parcel parcel(value.data(), value.size());
    (void)parcel.WriteUInt64(rowData.size());
    for (const auto &dataValue : rowData) {
        int errCode = SerializeDataValue(dataValue, parcel);
        if (errCode != E_OK) {
            value.clear();
            return errCode;
        }
    }
    return E_OK;
}

int DataTransformer::DeSerializeValue(const Value &value, OptRowData &optionalData,
    const std::vector<FieldInfo> &remoteFieldInfo)
{
    Parcel parcel(const_cast<uint8_t *>(value.data()), value.size());
    uint64_t fieldCount = 0;
    (void)parcel.ReadUInt64(fieldCount);
    if (fieldCount > DBConstant::MAX_COLUMN || parcel.IsError()) {
        return -E_PARSE_FAIL;
    }
    for (size_t i = 0; i < fieldCount; ++i) {
        DataValue dataValue;
        int errCode = DeserializeDataValue(dataValue, parcel);
        if (errCode != E_OK) {
            LOGD("[DataTransformer][DeSerializeValue] deSerialize failed");
            return errCode;
        }
        optionalData.push_back(std::move(dataValue));
    }
    return E_OK;
}
} // namespace DistributedDB
#endif