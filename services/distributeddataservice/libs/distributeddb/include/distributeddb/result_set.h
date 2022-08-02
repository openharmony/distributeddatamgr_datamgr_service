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

#ifndef RESULT_SET_H
#define RESULT_SET_H

#include <variant>
#include "store_types.h"

namespace DistributedDB {
using VariantData = std::variant<std::monostate, std::vector<uint8_t>, std::string, int64_t, double>;

class ResultSet {
public:
    DB_API virtual ~ResultSet() {};

    // Returns the count of rows in the result set.
    DB_API virtual int GetCount() const = 0;

    // Returns the current read position of the result set.
    DB_API virtual int GetPosition() const = 0;

    // Move the read position to the first row, return false if the result set is empty.
    DB_API virtual bool MoveToFirst() = 0;

    // Move the read position to the last row, return false if the result set is empty.
    DB_API virtual bool MoveToLast() = 0;

    // Move the read position to the next row, return false if the result set is empty
    // or the read position is already past the last entry in the result set.
    DB_API virtual bool MoveToNext() = 0;

    // Move the read position to the previous row, return false if the result set is empty
    // or the read position is already before the first entry in the result set.
    DB_API virtual bool MoveToPrevious() = 0;

    // Move the read position by a relative amount from the current position.
    DB_API virtual bool Move(int offset) = 0;

    // Move the read position to an absolute position value.
    DB_API virtual bool MoveToPosition(int position) = 0;

    // Returns whether the read position is pointing to the first row.
    DB_API virtual bool IsFirst() const = 0;

    // Returns whether the read position is pointing to the last row.
    DB_API virtual bool IsLast() const = 0;

    // Returns whether the read position is before the first row.
    DB_API virtual bool IsBeforeFirst() const = 0;

    // Returns whether the read position is after the last row
    DB_API virtual bool IsAfterLast() const = 0;

    // Returns whether the result set is empty.
    DB_API virtual bool IsClosed() const = 0;

    // Clear the result set. Set the position -1.
    DB_API virtual void Close() = 0;

    // Get a key-value entry. Just for kv delegate. Returns OK or NOT_SUPPORT.
    DB_API virtual DBStatus GetEntry(Entry &entry) const = 0;

    // Get column names.
    DB_API virtual void GetColumnNames(std::vector<std::string> &columnNames) const = 0;

    enum ColumnType { INVALID_TYPE = -1, INT64, STRING, BLOB, DOUBLE, NULL_VALUE };
    // Get the column name by column index. Returns OK, NOT_FOUND or NONEXISTENT.
    DB_API virtual DBStatus GetColumnType(int columnIndex, ColumnType &columnType) const = 0;

    // Get the column index by column name. Returns OK, NOT_FOUND or NONEXISTENT.
    DB_API virtual DBStatus GetColumnIndex(const std::string &columnName, int &columnIndex) const = 0;

    // Get the column name by column index. Returns OK, NOT_FOUND or NONEXISTENT.
    DB_API virtual DBStatus GetColumnName(int columnIndex, std::string &columnName) const = 0;

    // Get blob. Returns OK,, NOT_FOUND NONEXISTENT or TYPE_MISMATCH.
    DB_API virtual DBStatus Get(int columnIndex, std::vector<uint8_t> &value) const = 0;

    // Get string. Returns OK, NOT_FOUND, NONEXISTENT or TYPE_MISMATCH.
    DB_API virtual DBStatus Get(int columnIndex, std::string &value) const = 0;

    // Get int64. Returns OK, NOT_FOUND, NONEXISTENT or TYPE_MISMATCH.
    DB_API virtual DBStatus Get(int columnIndex, int64_t &value) const = 0;

    // Get double. Returns OK, NOT_FOUND, NONEXISTENT or TYPE_MISMATCH.
    DB_API virtual DBStatus Get(int columnIndex, double &value) const = 0;

    // Get whether the column value is null. Returns OK, NOT_FOUND or NONEXISTENT.
    DB_API virtual DBStatus IsColumnNull(int columnIndex, bool &isNull) const = 0;

    // Get the row record. Returns OK, NOT_FOUND or NOT_SUPPORT.
    DB_API virtual DBStatus GetRow(std::map<std::string, VariantData> &data) const = 0;
};
} // namespace DistributedDB
#endif  // RESULT_SET_H