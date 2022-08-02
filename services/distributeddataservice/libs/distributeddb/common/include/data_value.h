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
#include <vector>
#include "db_types.h"
namespace DistributedDB {
class Blob {
public:
    Blob();
    ~Blob();

    Blob(Blob &&);
    Blob(const Blob &) = delete;
    Blob &operator=(Blob &&) noexcept;
    Blob &operator=(const Blob &) = delete;

    const uint8_t* GetData() const;
    uint32_t GetSize() const;

    int WriteBlob(const uint8_t *ptrArray, const uint32_t &size);
    std::vector<uint8_t> ToVector() const;

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
    DataValue &operator=(int64_t intVal);
    DataValue &operator=(double doubleVal);
    DataValue &operator=(const Blob &blob);
    DataValue &operator=(const std::string &string);
    int Set(Blob *&blob);

    // equals
    bool operator==(const DataValue &dataValue) const;
    bool operator!=(const DataValue &dataValue) const;

    StorageType GetType() const;
    int GetInt64(int64_t &outVal) const;
    int GetDouble(double &outVal) const;
    int GetBlob(Blob *&outVal) const;
    int SetBlob(const Blob &val);
    int GetBlob(Blob &outVal) const;
    int SetText(const std::string &val);
    int SetText(const uint8_t *val, uint32_t length);
    int GetText(std::string &outVal) const;
    void ResetValue();
    int GetBlobLength(uint32_t &length) const;
    std::string ToString() const;

private:
    StorageType type_ = StorageType::STORAGE_TYPE_NULL;
    union {
        void* zeroMem;
        Blob* blobPtr;
        double dValue;
        int64_t iValue;
    } value_{};
};
}
#endif // DISTRIBUTED_DB_DATA_VALUE_H
