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

Blob::Blob(Blob &&blob) : ptr_(blob.ptr_), size_(blob.size_)
{
    blob.ptr_ = nullptr;
    blob.size_ = 0;
}

Blob &Blob::operator=(Blob &&blob) noexcept
{
    if (&blob != this) {
        delete[] ptr_;
        ptr_ = blob.ptr_;
        size_ = blob.size_;
        blob.ptr_ = nullptr;
        blob.size_ = 0;
    }
    return *this;
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
    if (ptrArray == nullptr || size == 0) {
        return E_OK;
    }

    delete[] ptr_;
    ptr_ = nullptr;

    ptr_ = new (std::nothrow) uint8_t[size];
    if (ptr_ == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    errno_t errCode = memcpy_s(ptr_, size, ptrArray, size);
    if (errCode != EOK) {
        return -E_SECUREC_ERROR;
    }
    size_ = size;
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
{
    *this = std::move(dataValue);
}

DataValue &DataValue::operator=(const DataValue &dataValue)
{
    if (&dataValue == this) {
        return *this;
    }
    ResetValue();
    switch (dataValue.type_) {
        case StorageType::STORAGE_TYPE_INTEGER:
            (void)dataValue.GetInt64(this->value_.iValue);
            break;
        case StorageType::STORAGE_TYPE_REAL:
            (void)dataValue.GetDouble(this->value_.dValue);
            break;
        case StorageType::STORAGE_TYPE_BLOB:
        case StorageType::STORAGE_TYPE_TEXT:
            (void)dataValue.GetBlob(this->value_.blobPtr);
            break;
        default:
            break;
    }
    type_ = dataValue.type_;
    return *this;
}

DataValue &DataValue::operator=(DataValue &&dataValue) noexcept
{
    if (&dataValue == this) {
        return *this;
    }
    ResetValue();
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

DataValue &DataValue::operator=(int64_t intVal)
{
    ResetValue();
    type_ = StorageType::STORAGE_TYPE_INTEGER;
    value_.iValue = intVal;
    return *this;
}

DataValue &DataValue::operator=(double doubleVal)
{
    ResetValue();
    type_ = StorageType::STORAGE_TYPE_REAL;
    value_.dValue = doubleVal;
    return *this;
}

DataValue &DataValue::operator=(const Blob &blob)
{
    (void)SetBlob(blob);
    return *this;
}

int DataValue::Set(Blob *&blob)
{
    ResetValue();
    if (blob == nullptr || blob->GetSize() <= 0) {
        LOGE("Transfer Blob to DataValue failed.");
        return -E_INVALID_ARGS;
    }
    type_ = StorageType::STORAGE_TYPE_BLOB;
    value_.blobPtr = blob;
    blob = nullptr;
    return E_OK;
}

DataValue &DataValue::operator=(const std::string &string)
{
    (void)SetText(string);
    return *this;
}

bool DataValue::operator==(const DataValue &dataValue) const
{
    if (dataValue.type_ != type_) {
        return false;
    }
    switch (type_) {
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
    return !(*this == dataValue);
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
    delete outVal;
    outVal = nullptr;
    outVal = new (std::nothrow) Blob();
    if (outVal == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    return outVal->WriteBlob(value_.blobPtr->GetData(), value_.blobPtr->GetSize());
}

int DataValue::SetBlob(const Blob &val)
{
    ResetValue();
    if (val.GetSize() <= 0) {
        return E_OK;
    }
    value_.blobPtr = new(std::nothrow) Blob();
    if (value_.blobPtr == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    type_ = StorageType::STORAGE_TYPE_BLOB;
    int errCode = E_OK;
    if (val.GetSize() > 0) {
        errCode = value_.blobPtr->WriteBlob(val.GetData(), val.GetSize());
    }
    return errCode;
}

int DataValue::GetBlob(Blob &outVal) const
{
    if (type_ != StorageType::STORAGE_TYPE_BLOB && type_ != StorageType::STORAGE_TYPE_TEXT) {
        return -E_NOT_SUPPORT;
    }
    return outVal.WriteBlob(value_.blobPtr->GetData(), value_.blobPtr->GetSize());
}

int DataValue::SetText(const std::string &val)
{
    return SetText(reinterpret_cast<const uint8_t *>(val.c_str()), val.length());
}

int DataValue::SetText(const uint8_t *val, uint32_t length)
{
    ResetValue();
    value_.blobPtr = new(std::nothrow) Blob();
    if (value_.blobPtr == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    type_ = StorageType::STORAGE_TYPE_TEXT;
    return value_.blobPtr->WriteBlob(val, length);
}

int DataValue::GetText(std::string &outValue) const
{
    if (type_ != StorageType::STORAGE_TYPE_TEXT) {
        return -E_NOT_SUPPORT;
    }
    const uint8_t *data = value_.blobPtr->GetData();
    uint32_t len = value_.blobPtr->GetSize();
    if (len == 0) {
        outValue = "";
        return E_OK;
    }
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
} // namespace DistributedDB
#endif