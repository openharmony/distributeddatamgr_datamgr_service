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

#ifndef RELATIONAL_RESULT_SET_IMPL_H
#define RELATIONAL_RESULT_SET_IMPL_H

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "db_types.h"
#include "distributeddb/result_set.h"
#include "relational_row_data_set.h"

namespace DistributedDB {
class RelationalResultSetImpl : public ResultSet {
public:
    RelationalResultSetImpl()
        : isClosed_(false), dataSetSize_(0), index_(-1), dataSet_(), colNames_(), cacheDataSet_(), mutex_()
    {}

    ~RelationalResultSetImpl() = default;

    // Returns the count of rows in the result set.
    int GetCount() const override;

    // Returns the current read position of the result set.
    int GetPosition() const override;

    // Move the read position to the first row, return false if the result set is empty.
    bool MoveToFirst() override;

    // Move the read position to the last row, return false if the result set is empty.
    bool MoveToLast() override;

    // Move the read position to the next row, return false if the result set is empty
    // or the read position is already past the last entry in the result set.
    bool MoveToNext() override;

    // Move the read position to the previous row, return false if the result set is empty
    // or the read position is already before the first entry in the result set.
    bool MoveToPrevious() override;

    // Move the read position by a relative amount from the current position.
    bool Move(int offset) override;

    // Move the read position to an absolute position value.
    bool MoveToPosition(int position) override;

    // Returns whether the read position is pointing to the first row.
    bool IsFirst() const override;

    // Returns whether the read position is pointing to the last row.
    bool IsLast() const override;

    // Returns whether the read position is before the first row.
    bool IsBeforeFirst() const override;

    // Returns whether the read position is after the last row
    bool IsAfterLast() const override;

    // Returns whether the result set is empty.
    bool IsClosed() const override;

    // Clear the result set. Set the position -1.
    void Close() override;

    // Get a key-value entry. Just for kv delegate. Returns OK or NOT_SUPPORT.
    DBStatus GetEntry(Entry &entry) const override;

    // Get column names.
    void GetColumnNames(std::vector<std::string> &columnNames) const override;

    // Get the column name by column index. Returns OK, NOT_FOUND or NONEXISTENT.
    DBStatus GetColumnType(int columnIndex, ColumnType &columnType) const override;

    // Get the column index by column name. Returns OK, NOT_FOUND or NONEXISTENT.
    DBStatus GetColumnIndex(const std::string &columnName, int &columnIndex) const override;

    // Get the column name by column index. Returns OK, NOT_FOUND or NONEXISTENT.
    DBStatus GetColumnName(int columnIndex, std::string &columnName) const override;

    // Get blob. Returns OK, NOT_FOUND, NONEXISTENT or TYPE_MISMATCH.
    DBStatus Get(int columnIndex, std::vector<uint8_t> &value) const override;

    // Get string. Returns OK, NOT_FOUND, NONEXISTENT or TYPE_MISMATCH.
    DBStatus Get(int columnIndex, std::string &value) const override;

    // Get int64. Returns OK, NOT_FOUND, NONEXISTENT or TYPE_MISMATCH.
    DBStatus Get(int columnIndex, int64_t &value) const override;

    // Get double. Returns OK, NOT_FOUND, NONEXISTENT or TYPE_MISMATCH.
    DBStatus Get(int columnIndex, double &value) const override;

    // Get whether the column value is null. Returns OK, NOT_FOUND or NONEXISTENT.
    DBStatus IsColumnNull(int columnIndex, bool &isNull) const override;

    // Get the row record. Returns OK, NOT_FOUND or NOT_SUPPORT.
    DBStatus GetRow(std::map<std::string, VariantData> &data) const override;

    // sequenceId start from 1.
    int Put(const DeviceID &deviceName, uint32_t sequenceId, RelationalRowDataSet &&data);

private:
    inline bool IsValid() const;
    inline bool IsValid(int64_t columnIndex) const;
    VariantData GetData(int64_t columnIndex) const;

    bool isClosed_;
    int dataSetSize_;
    int64_t index_;
    RelationalRowDataSet dataSet_;
    mutable std::unordered_map<std::string, int> colNames_;
    std::map<uint32_t, RelationalRowDataSet> cacheDataSet_;
    mutable std::shared_mutex mutex_;
};
} // namespace DistributedDB
#endif  // RELATIONAL_RESULT_SET_IMPL_H