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

#ifndef DISTRIBUTED_DB_DATA_VALUE_H
#define DISTRIBUTED_DB_DATA_VALUE_H

#include <cstdint>
#include <macro_utils.h>
#include <map>
#include <string>
namespace DistributedDB {
// field types stored in sqlite
enum class StorageType {
    STORAGE_TYPE_NONE = 0,
    STORAGE_TYPE_NULL,
    STORAGE_TYPE_BOOL,
    STORAGE_TYPE_INTEGER,
    STORAGE_TYPE_REAL,
    STORAGE_TYPE_TEXT,
    STORAGE_TYPE_BLOB
};

class Blob {
public:
    Blob();
    ~Blob();

    DISABLE_COPY_ASSIGN_MOVE(Blob);

    const uint8_t* GetData() const;
    uint32_t GetSize() const;

    int WriteBlob(const uint8_t *ptrArray, const uint32_t &size);

private:
    uint8_t* ptr_;
    uint32_t size_;
};

class DataValue {
public:
    DataValue();
    ~DataValue();

    // copy constructor
    DataValue(const DataValue &dataValue);
    DataValue &operator=(const DataValue &dataValue);
    // move constructor
    DataValue(DataValue &&dataValue) noexcept;
    DataValue &operator=(DataValue &&dataValue) noexcept;
    DataValue &operator=(const bool &boolVal);
    DataValue &operator=(const int64_t &intVal);
    DataValue &operator=(const double &doubleVal);
    DataValue &operator=(const Blob &blob);
    DataValue &operator=(const std::string &string);
    // equals
    bool operator==(const DataValue &dataValue) const;
    bool operator!=(const DataValue &dataValue) const;

    StorageType GetType() const;
    int GetBool(bool &outVal) const;
    int GetInt64(int64_t &outVal) const;
    int GetDouble(double &outVal) const;
    int GetBlob(Blob *&outVal) const;
    int GetBlob(Blob &outVal) const;
    int GetText(std::string &outVal) const;
    void ResetValue();
    int GetBlobLength(uint32_t &length) const;
    std::string ToString() const;

private:
    StorageType type_ = StorageType::STORAGE_TYPE_NULL;
    union {
        void* zeroMem;
        bool bValue;
        Blob* blobPtr;
        double dValue;
        int64_t iValue;
    } value_{};
};

class ObjectData {
public:
    int GetBool(const std::string &fieldName, bool &outValue) const;
    int GetInt64(const std::string &fieldName, int64_t &outValue) const;
    int GetDouble(const std::string &fieldName, double &outValue) const;
    int GetString(const std::string &fieldName, std::string &outValue) const;
    int GetBlob(const std::string &fieldName, Blob &blob) const;
    void PutDataValue(const std::string &fieldName, const DataValue &value);
private:
    mutable std::map<std::string, DataValue> fieldData;
};
}
#endif // DISTRIBUTED_DB_DATA_VALUE_H
