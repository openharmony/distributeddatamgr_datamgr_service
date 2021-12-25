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
#include "data_value.h"

#include "db_errno.h"
#include "relational_schema_object.h"
#include "securec.h"

namespace DistributedDB {
Blob::Blob() : ptr_(nullptr), size_(0)
{
}

Blob::~Blob()
{
    if (ptr_ != nullptr) {
        delete[] ptr_;
        ptr_ = nullptr;
    }
    size_ = 0;
}

const uint8_t *Blob::GetData() const
{
    return ptr_;
}

uint32_t Blob::GetSize() const
{
    return size_;
}

int Blob::WriteBlob(const uint8_t *ptrArray, const uint32_t &size)
{
    if (size == 0) return E_OK;
    ptr_ = new(std::nothrow) uint8_t[size];
    if (ptr_ == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    size_ = size;
    errno_t errCode = memcpy_s(ptr_, size, ptrArray, size);
    if (errCode != EOK) {
        return -E_SECUREC_ERROR;
    }
    return E_OK;
}

DataValue::DataValue() : type_(StorageType::STORAGE_TYPE_NULL)
{
    value_.zeroMem = nullptr;
}

DataValue::~DataValue()
{
    ResetValue();
}

DataValue::DataValue(const DataValue &dataValue)
{
    *this = dataValue;
}

DataValue::DataValue(DataValue &&dataValue) noexcept
    :type_(dataValue.type_), value_(dataValue.value_)
{
    switch (type_) {
        case StorageType::STORAGE_TYPE_BLOB:
        case StorageType::STORAGE_TYPE_TEXT:
            dataValue.value_.blobPtr = nullptr;
            break;
        default:
            break;
    }
    dataValue.ResetValue();
}

DataValue &DataValue::operator=(const DataValue &dataValue)
{
    if (this == &dataValue) {
        return *this;
    }
    ResetValue();
    switch (dataValue.type_) {
        case StorageType::STORAGE_TYPE_BOOL:
            dataValue.GetBool(this->value_.bValue);
            break;
        case StorageType::STORAGE_TYPE_INTEGER:
            dataValue.GetInt64(this->value_.iValue);
            break;
        case StorageType::STORAGE_TYPE_REAL:
            dataValue.GetDouble(this->value_.dValue);
            break;
        case StorageType::STORAGE_TYPE_BLOB:
        case StorageType::STORAGE_TYPE_TEXT:
            dataValue.GetBlob(this->value_.blobPtr);
            break;
        default:
            break;
    }
    type_ = dataValue.type_;
    return *this;
}

DataValue &DataValue::operator=(DataValue &&dataValue) noexcept
{
    this->type_ = dataValue.type_;
    this->value_ = dataValue.value_;
    switch (type_) {
        case StorageType::STORAGE_TYPE_BLOB:
        case StorageType::STORAGE_TYPE_TEXT:
            dataValue.value_.blobPtr = nullptr;
            break;
        default:
            break;
    }
    return *this;
}

DataValue &DataValue::operator=(const bool &boolVal)
{
    ResetValue();
    type_ = StorageType::STORAGE_TYPE_BOOL;
    value_.bValue = boolVal;
    return *this;
}

DataValue &DataValue::operator=(const int64_t &intVal)
{
    ResetValue();
    type_ = StorageType::STORAGE_TYPE_INTEGER;
    value_.iValue = intVal;
    return *this;
}

DataValue &DataValue::operator=(const double &doubleVal)
{
    ResetValue();
    type_ = StorageType::STORAGE_TYPE_REAL;
    value_.dValue = doubleVal;
    return *this;
}

DataValue &DataValue::operator=(const Blob &blob)
{
    ResetValue();
    if (blob.GetSize() <= 0) {
        return *this;
    }
    value_.blobPtr = new(std::nothrow) Blob();
    if (value_.blobPtr == nullptr) {
        return *this;
    }
    type_ = StorageType::STORAGE_TYPE_BLOB;
    if (blob.GetSize() > 0) {
        value_.blobPtr->WriteBlob(blob.GetData(), blob.GetSize());
    }
    return *this;
}

DataValue &DataValue::operator=(const std::string &string)
{
    ResetValue();
    value_.blobPtr = new(std::nothrow) Blob();
    if (value_.blobPtr == nullptr) {
        return *this;
    }
    type_ = StorageType::STORAGE_TYPE_TEXT;
    value_.blobPtr->WriteBlob(reinterpret_cast<const uint8_t*>(string.c_str()), string.size());
    return *this;
}

bool DataValue::operator==(const DataValue &dataValue) const
{
    if (dataValue.type_ != type_) {
        return false;
    }
    switch (type_) {
        case StorageType::STORAGE_TYPE_BOOL:
            return dataValue.value_.bValue == value_.bValue;
        case StorageType::STORAGE_TYPE_INTEGER:
            return dataValue.value_.iValue == value_.iValue;
        case StorageType::STORAGE_TYPE_REAL:
            return dataValue.value_.dValue == value_.dValue;
        case StorageType::STORAGE_TYPE_BLOB:
        case StorageType::STORAGE_TYPE_TEXT:
            if (dataValue.value_.blobPtr->GetSize() != value_.blobPtr->GetSize()) {
                return false;
            }
            for (uint32_t i = 0; i < dataValue.value_.blobPtr->GetSize(); ++i) {
                if (dataValue.value_.blobPtr->GetData()[i] != value_.blobPtr->GetData()[i]) {
                    return false;
                }
            }
            return true;
        default:
            return true;
    }
}

bool DataValue::operator!=(const DataValue &dataValue) const
{
    return !(*this==dataValue);
}

int DataValue::GetBool(bool &outVal) const
{
    if (type_ != StorageType::STORAGE_TYPE_BOOL) {
        return -E_NOT_SUPPORT;
    }
    outVal = value_.bValue;
    return E_OK;
}

int DataValue::GetDouble(double &outVal) const
{
    if (type_ != StorageType::STORAGE_TYPE_REAL) {
        return -E_NOT_SUPPORT;
    }
    outVal = value_.dValue;
    return E_OK;
}

int DataValue::GetInt64(int64_t &outVal) const
{
    if (type_ != StorageType::STORAGE_TYPE_INTEGER) {
        return -E_NOT_SUPPORT;
    }
    outVal = value_.iValue;
    return E_OK;
}

int DataValue::GetBlob(Blob *&outVal) const
{
    if (type_ != StorageType::STORAGE_TYPE_BLOB && type_ != StorageType::STORAGE_TYPE_TEXT) {
        return -E_NOT_SUPPORT;
    }
    outVal = new(std::nothrow) Blob();
    if (outVal == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    return outVal->WriteBlob(value_.blobPtr->GetData(), value_.blobPtr->GetSize());
}

int DataValue::GetBlob(Blob &outVal) const
{
    if (type_ != StorageType::STORAGE_TYPE_BLOB && type_ != StorageType::STORAGE_TYPE_TEXT) {
        return -E_NOT_SUPPORT;
    }
    return outVal.WriteBlob(value_.blobPtr->GetData(), value_.blobPtr->GetSize());
}

int DataValue::GetText(std::string &outValue) const
{
    if (type_ != StorageType::STORAGE_TYPE_TEXT) {
        return -E_NOT_SUPPORT;
    }
    const uint8_t *data = value_.blobPtr->GetData();
    uint32_t len = value_.blobPtr->GetSize();
    outValue.resize(len);
    outValue.assign(data, data + len);
    return E_OK;
}

StorageType DataValue::GetType() const
{
    return type_;
}

int DataValue::GetBlobLength(uint32_t &length) const
{
    if (type_ != StorageType::STORAGE_TYPE_BLOB && type_ != StorageType::STORAGE_TYPE_TEXT) {
        return -E_NOT_SUPPORT;
    }
    length = value_.blobPtr->GetSize();
    return E_OK;
}

void DataValue::ResetValue()
{
    switch (type_) {
        case StorageType::STORAGE_TYPE_TEXT:
        case StorageType::STORAGE_TYPE_BLOB:
            delete value_.blobPtr;
            value_.blobPtr = nullptr;
            break;
        case StorageType::STORAGE_TYPE_NULL:
        case StorageType::STORAGE_TYPE_BOOL:
        case StorageType::STORAGE_TYPE_INTEGER:
        case StorageType::STORAGE_TYPE_REAL:
        default:
            break;
    }
    type_ = StorageType::STORAGE_TYPE_NULL;
    value_.zeroMem = nullptr;
}

std::string DataValue::ToString() const
{
    std::string res;
    switch (type_) {
        case StorageType::STORAGE_TYPE_TEXT:
            (void)GetText(res);
            break;
        case StorageType::STORAGE_TYPE_BLOB:
            res = "NOT SUPPORT";
            break;
        case StorageType::STORAGE_TYPE_NULL:
            res = "null";
            break;
        case StorageType::STORAGE_TYPE_BOOL:
            res = std::to_string(value_.bValue);
            break;
        case StorageType::STORAGE_TYPE_INTEGER:
            res = std::to_string(value_.iValue);
            break;
        case StorageType::STORAGE_TYPE_REAL:
            res = std::to_string(value_.dValue);
            break;
        default:
            res = "default";
            break;
    }
    return "[" + res + "]";
}

int ObjectData::GetDouble(const std::string &fieldName, double &outValue) const
{
    if (fieldData.find(fieldName) == fieldData.end()) {
        return -E_NOT_FOUND;
    }
    return fieldData[fieldName].GetDouble(outValue);
}

int ObjectData::GetInt64(const std::string &fieldName, int64_t &outValue) const
{
    if (fieldData.find(fieldName) == fieldData.end()) {
        return -E_NOT_FOUND;
    }
    return fieldData[fieldName].GetInt64(outValue);
}

int ObjectData::GetBlob(const std::string &fieldName, Blob &blob) const
{
    if (fieldData.find(fieldName) == fieldData.end()) {
        return -E_NOT_FOUND;
    }
    int errCode = fieldData[fieldName].GetBlob(blob);
    return errCode;
}

int ObjectData::GetString(const std::string &fieldName, std::string &outValue) const
{
    if (fieldData.find(fieldName) == fieldData.end()) {
        return -E_NOT_FOUND;
    }
    Blob blob;
    int errCode = fieldData[fieldName].GetBlob(blob);
    if (errCode != E_OK) {
        return errCode;
    }
    outValue.resize(blob.GetSize());
    outValue.assign(blob.GetData(), blob.GetData() + blob.GetSize());
    return errCode;
}

int ObjectData::GetBool(const std::string &fieldName, bool &outValue) const
{
    if (fieldData.find(fieldName) == fieldData.end()) {
        return -E_NOT_FOUND;
    }
    return fieldData[fieldName].GetBool(outValue);
}

void ObjectData::PutDataValue(const std::string &fieldName, const DataValue &value)
{
    fieldData[fieldName] = value;
}
} // namespace DistributedDB
#endif