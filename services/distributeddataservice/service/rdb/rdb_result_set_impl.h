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

#ifndef DISTRIBUTED_RDB_RDB_RESULT_SET_IMPL_H
#define DISTRIBUTED_RDB_RDB_RESULT_SET_IMPL_H

#include <shared_mutex>
#include "rdb_result_set_stub.h"
#include "store/cursor.h"

namespace OHOS::DistributedRdb {
class RdbResultSetImpl final : public RdbResultSetStub {
public:
    explicit RdbResultSetImpl(std::shared_ptr<DistributedData::Cursor> resultSet);
    ~RdbResultSetImpl() override {};
    int GetAllColumnNames(std::vector<std::string> &columnNames) override;
    int GetColumnCount(int &count) override;
    int GetColumnType(int columnIndex, ColumnType &columnType) override;
    int GetColumnIndex(const std::string &columnName, int &columnIndex) override;
    int GetColumnName(int columnIndex, std::string &columnName) override;
    int GetRowCount(int &count) override;
    int GetRowIndex(int &position) const override;
    int GoTo(int offset) override;
    int GoToRow(int position) override;
    int GoToFirstRow() override;
    int GoToLastRow() override;
    int GoToNextRow() override;
    int GoToPreviousRow() override;
    int IsEnded(bool &result) override;
    int IsStarted(bool &result) const override;
    int IsAtFirstRow(bool &result) const override;
    int IsAtLastRow(bool &result) override;
    int GetBlob(int columnIndex, std::vector<uint8_t> &value) override;
    int GetString(int columnIndex, std::string &value) override;
    int GetInt(int columnIndex, int &value) override;
    int GetLong(int columnIndex, int64_t &value) override;
    int GetDouble(int columnIndex, double &value) override;
    int IsColumnNull(int columnIndex, bool &isNull) override;
    bool IsClosed() const override;
    int Close() override;

private:
    mutable std::shared_mutex mutex_ {};
    using GenColumnType = DistributedData::Cursor::ColumnType;
    static inline constexpr ColumnType COLUMNTYPES[GenColumnType::TYPE_BUTT] = {
        [GenColumnType::INVALID_TYPE] = ColumnType::TYPE_NULL,
        [GenColumnType::INT64] = ColumnType::TYPE_INTEGER,
        [GenColumnType::STRING] = ColumnType::TYPE_STRING,
        [GenColumnType::BLOB] = ColumnType::TYPE_BLOB,
        [GenColumnType::DOUBLE] = ColumnType::TYPE_FLOAT,
        [GenColumnType::NULL_VALUE] = ColumnType::TYPE_NULL
    };
    std::shared_ptr<DistributedData::Cursor> resultSet_;
    ColumnType ConvertColumnType(int32_t columnType) const;
};
} // namespace OHOS::DistributedRdb
#endif // DISTRIBUTED_RDB_RDB_RESULT_SET_IMPL_H
