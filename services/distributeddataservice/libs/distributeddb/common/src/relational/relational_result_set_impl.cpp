/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "relational_result_set_impl.h"
#include "kv_store_errno.h"

namespace DistributedDB {
inline bool RelationalResultSetImpl::IsValid() const
{
    return !isClosed_ && cacheDataSet_.empty();
}

inline bool RelationalResultSetImpl::IsValid(int64_t index) const
{
    return !isClosed_ && cacheDataSet_.empty() && index >= 0 && index < dataSet_.GetSize();
}

int RelationalResultSetImpl::GetCount() const
{
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    if (IsValid()) {
        return dataSet_.GetSize();
    }
    return 0;
}

int RelationalResultSetImpl::GetPosition() const
{
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    return index_;
}

bool RelationalResultSetImpl::MoveToFirst()
{
    return MoveToPosition(0);
}

bool RelationalResultSetImpl::MoveToLast()
{
    return MoveToPosition(dataSet_.GetSize() - 1);
}

bool RelationalResultSetImpl::MoveToNext()
{
    return Move(1);
}

bool RelationalResultSetImpl::MoveToPrevious()
{
    return Move(-1);
}

bool RelationalResultSetImpl::Move(int offset)
{
    return MoveToPosition(index_ + offset);
}

bool RelationalResultSetImpl::MoveToPosition(int position)
{
    std::unique_lock<std::shared_mutex> writeLock(mutex_);
    int64_t index = position;
    if (IsValid(index)) {
        index_ = index;
        return true;
    }
    if (index < 0) {
        index_ = -1;
    } else if (index >= dataSet_.GetSize()) {
        index_ = dataSet_.GetSize();
    }
    return false;
}

bool RelationalResultSetImpl::IsFirst() const
{
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    return IsValid(index_) && index_ == 0;
}

bool RelationalResultSetImpl::IsLast() const
{
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    return IsValid(index_) && index_ == dataSet_.GetSize() - 1;
}

bool RelationalResultSetImpl::IsBeforeFirst() const
{
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    return dataSet_.GetSize() == 0 || index_ <= -1;
}

bool RelationalResultSetImpl::IsAfterLast() const
{
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    return dataSet_.GetSize() == 0 || index_ >= dataSet_.GetSize();
}

bool RelationalResultSetImpl::IsClosed() const
{
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    return isClosed_;
}

void RelationalResultSetImpl::Close()
{
    if (IsClosed()) {
        return;
    }
    std::unique_lock<std::shared_mutex> writeLock(mutex_);
    isClosed_ = true;
    index_ = -1;
    cacheDataSet_.clear();
    colNames_.clear();
    dataSet_.Clear();
}

DBStatus RelationalResultSetImpl::GetEntry(Entry &entry) const
{
    return NOT_SUPPORT;
}

void RelationalResultSetImpl::GetColumnNames(std::vector<std::string> &columnNames) const
{
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    columnNames = dataSet_.GetColNames();
}

namespace {
struct ColumnTypePair {
    int index_;
    ResultSet::ColumnType type_;
};

ResultSet::ColumnType GetColType(int index, const ColumnTypePair *colMap, int32_t len)
{
    int32_t head = 0;
    int32_t end = len - 1;
    while (head <= end) {
        int32_t mid = (head + end) / 2;
        if (colMap[mid].index_ < index) {
            head = mid + 1;
            continue;
        }
        if (colMap[mid].index_ > index) {
            end = mid - 1;
            continue;
        }
        return colMap[mid].type_;
    }
    return ResultSet::ColumnType::INVALID_TYPE;
}
}

DBStatus RelationalResultSetImpl::GetColumnType(int columnIndex, ColumnType &columnType) const
{
    static constexpr ColumnTypePair mappingTbl[] = {
        { static_cast<int>(StorageType::STORAGE_TYPE_NONE),    ColumnType::INVALID_TYPE },
        { static_cast<int>(StorageType::STORAGE_TYPE_NULL),    ColumnType::NULL_VALUE },
        { static_cast<int>(StorageType::STORAGE_TYPE_INTEGER), ColumnType::INT64 },
        { static_cast<int>(StorageType::STORAGE_TYPE_REAL),    ColumnType::DOUBLE },
        { static_cast<int>(StorageType::STORAGE_TYPE_TEXT),    ColumnType::STRING },
        { static_cast<int>(StorageType::STORAGE_TYPE_BLOB),    ColumnType::BLOB },
    };

    std::shared_lock<std::shared_mutex> readLock(mutex_);
    const RelationalRowData *rowData = dataSet_.Get(index_);
    if (rowData == nullptr) {
        return NOT_FOUND;
    }
    auto type = StorageType::STORAGE_TYPE_NONE;
    int errCode = rowData->GetType(columnIndex, type);
    if (errCode == E_OK) {
        columnType = GetColType(static_cast<int>(type), mappingTbl, sizeof(mappingTbl) / sizeof(ColumnTypePair));
    }
    return TransferDBErrno(errCode);
}

DBStatus RelationalResultSetImpl::GetColumnIndex(const std::string &columnName, int &columnIndex) const
{
    if (colNames_.empty()) {
        std::unique_lock<std::shared_mutex> writeLock(mutex_);
        if (colNames_.empty()) {
            for (size_t i = 0; i < dataSet_.GetColNames().size(); ++i) {
                colNames_[dataSet_.GetColNames().at(i)] = i;
            }
        }
    }
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    auto iter = colNames_.find(columnName);
    if (iter == colNames_.end()) {
        return NONEXISTENT;
    }
    columnIndex = iter->second;
    return OK;
}

DBStatus RelationalResultSetImpl::GetColumnName(int columnIndex, std::string &columnName) const
{
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    const auto &colNames = dataSet_.GetColNames();
    if (columnIndex < 0 || columnIndex >= static_cast<int>(colNames.size())) {
        return NONEXISTENT;
    }
    columnName = colNames.at(columnIndex);
    return OK;
}

DBStatus RelationalResultSetImpl::Get(int columnIndex, std::vector<uint8_t> &value) const
{
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    const RelationalRowData *rowData = dataSet_.Get(index_);
    if (rowData == nullptr) {
        return NOT_FOUND;
    }
    return TransferDBErrno(rowData->Get(columnIndex, value));
}

DBStatus RelationalResultSetImpl::Get(int columnIndex, std::string &value) const
{
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    const RelationalRowData *rowData = dataSet_.Get(index_);
    if (rowData == nullptr) {
        return NOT_FOUND;
    }
    return TransferDBErrno(rowData->Get(columnIndex, value));
}

DBStatus RelationalResultSetImpl::Get(int columnIndex, int64_t &value) const
{
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    const RelationalRowData *rowData = dataSet_.Get(index_);
    if (rowData == nullptr) {
        return NOT_FOUND;
    }
    return TransferDBErrno(rowData->Get(columnIndex, value));
}

DBStatus RelationalResultSetImpl::Get(int columnIndex, double &value) const
{
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    const RelationalRowData *rowData = dataSet_.Get(index_);
    if (rowData == nullptr) {
        return NOT_FOUND;
    }
    return TransferDBErrno(rowData->Get(columnIndex, value));
}

DBStatus RelationalResultSetImpl::IsColumnNull(int columnIndex, bool &isNull) const
{
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    const RelationalRowData *rowData = dataSet_.Get(index_);
    if (rowData == nullptr) {
        return NOT_FOUND;
    }
    auto type = StorageType::STORAGE_TYPE_NONE;
    int errCode = rowData->GetType(columnIndex, type);
    if (errCode == E_OK) {
        isNull = type == StorageType::STORAGE_TYPE_NULL;
    }
    return TransferDBErrno(errCode);
}

// This func is not API. Impossible concurrency. There is no need to hold mutex. columnIndex must be valid
VariantData RelationalResultSetImpl::GetData(int64_t columnIndex) const
{
    auto columnType = ColumnType::INVALID_TYPE;
    (void)GetColumnType(columnIndex, columnType);
    switch (columnType) {
        case INT64: {
            int64_t value = 0;
            (void)Get(columnIndex, value);
            return value;
        }
        case DOUBLE: {
            double value = 0;
            (void)Get(columnIndex, value);
            return value;
        }
        case STRING: {
            std::string value;
            (void)Get(columnIndex, value);
            return value;
        }
        case BLOB: {
            std::vector<uint8_t> value;
            (void)Get(columnIndex, value);
            return value;
        }
        case NULL_VALUE:
        default:
            return VariantData();
    }
}

DBStatus RelationalResultSetImpl::GetRow(std::map<std::string, VariantData> &data) const
{
    data.clear();
    std::shared_lock<std::shared_mutex> readLock(mutex_);
    if (!IsValid(index_)) {
        return NOT_FOUND;
    }

    for (int columnIndex = 0; columnIndex < static_cast<int>(dataSet_.GetColNames().size()); ++columnIndex) {
        data[dataSet_.GetColNames().at(columnIndex)] = GetData(columnIndex);
    }
    return OK;
}

// This func is not API. Impossible concurrency. There is no need to hold mutex.
int RelationalResultSetImpl::Put(const DeviceID &deviceName, uint32_t sequenceId, RelationalRowDataSet &&data)
{
    cacheDataSet_[sequenceId - 1] = std::move(data);
    for (auto iter = cacheDataSet_.begin(); iter != cacheDataSet_.end();) {
        if (iter->first != static_cast<uint32_t>(dataSetSize_)) {
            break;
        }
        int errCode = dataSet_.Merge(std::move(iter->second));  // pay attention, this is rvalue.
        if (errCode != E_OK) {
            return errCode;
        }
        iter = cacheDataSet_.erase(iter);
        dataSetSize_++;
    }
    return E_OK;
}
}
#endif