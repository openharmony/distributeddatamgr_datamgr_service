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

#include <atomic>
#include <shared_mutex>
#include "result_set.h"
#include "rdb_errno.h"
#include "store/cursor.h"
#include "value_proxy.h"
#include "store/general_value.h"

namespace OHOS::DistributedRdb {
class RdbResultSetImpl final : public NativeRdb::ResultSet {
public:
    using ValueProxy = DistributedData::ValueProxy;
    using ColumnType = NativeRdb::ColumnType;
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
    int GetAsset(int32_t col, NativeRdb::ValueObject::Asset& value) override;
    int GetAssets(int32_t col, NativeRdb::ValueObject::Assets& value) override;
    int GetFloat32Array(int32_t index, NativeRdb::ValueObject::FloatVector& vecs) override;
    int Get(int32_t col, NativeRdb::ValueObject& value) override;
    int GetRow(NativeRdb::RowEntity& rowEntity) override;
    int GetSize(int columnIndex, size_t& size) override;

private:
    template<typename T>
    std::enable_if_t < ValueProxy::CVT_INDEX<T, ValueProxy::Proxy><ValueProxy::MAX, int>
    Get(int columnIndex, T &value) const
    {
        auto [ret, val] = GetValue(columnIndex);
        value = val.operator T();
        return ret;
    };

    std::pair<int32_t, ValueProxy::Value> GetValue(int columnIndex) const
    {
        DistributedData::Value var;
        auto status = resultSet_->Get(columnIndex, var);
        if (status != DistributedData::GeneralError::E_OK) {
            return { NativeRdb::E_ERROR, ValueProxy::Value() };
        }
        return {NativeRdb::E_OK, ValueProxy::Convert(std::move(var))};
    };

    mutable std::shared_mutex mutex_ {};
    static constexpr ColumnType COLUMNTYPES[DistributedData::TYPE_MAX] = {
        [DistributedData::TYPE_INDEX<std::monostate>] = ColumnType::TYPE_NULL,
        [DistributedData::TYPE_INDEX<int64_t>] = ColumnType::TYPE_INTEGER,
        [DistributedData::TYPE_INDEX<double>] = ColumnType::TYPE_FLOAT,
        [DistributedData::TYPE_INDEX<std::string>] = ColumnType::TYPE_STRING,
        [DistributedData::TYPE_INDEX<bool>] = ColumnType::TYPE_INTEGER,
        [DistributedData::TYPE_INDEX<DistributedData::Bytes>] = ColumnType::TYPE_BLOB,
        [DistributedData::TYPE_INDEX<DistributedData::Asset>] = ColumnType::TYPE_BLOB,
        [DistributedData::TYPE_INDEX<DistributedData::Assets>] = ColumnType::TYPE_BLOB,
    };
    std::shared_ptr<DistributedData::Cursor> resultSet_;
    std::atomic<int32_t> current_ = -1;
    int32_t count_ = 0;
    std::vector<std::string> colNames_;
    ColumnType ConvertColumnType(int32_t columnType) const;
};
} // namespace OHOS::DistributedRdb
#endif // DISTRIBUTED_RDB_RDB_RESULT_SET_IMPL_H
