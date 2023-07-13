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

#define LOG_TAG "RdbResultSetImpl"

#include "log_print.h"
#include "store_types.h"
#include "rdb_result_set_impl.h"
#include "store/cursor.h"

using DistributedDB::DBStatus;
using OHOS::NativeRdb::ColumnType;

namespace OHOS::DistributedRdb {
using OHOS::DistributedData::GeneralError;
using Cursor = OHOS::DistributedData::Cursor;
RdbResultSetImpl::RdbResultSetImpl(std::shared_ptr<Cursor> resultSet) : resultSet_(std::move(resultSet)), index_(-1) {}

int RdbResultSetImpl::GetAllColumnNames(std::vector<std::string> &columnNames)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    return resultSet_->GetColumnNames(columnNames) == GeneralError::E_OK ? NativeRdb::E_ERROR : NativeRdb::E_OK;
}

int RdbResultSetImpl::GetColumnCount(int &count)
{
    return NativeRdb::E_NOT_SUPPORT;
}

int RdbResultSetImpl::GetColumnType(int columnIndex, ColumnType &columnType)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    columnType = ConvertColumnType(resultSet_->GetColumnType(columnIndex));
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GetColumnIndex(const std::string &columnName, int &columnIndex)
{
    return NativeRdb::E_NOT_SUPPORT;
}

int RdbResultSetImpl::GetColumnName(int columnIndex, std::string &columnName)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    return resultSet_->GetColumnName(columnIndex, columnName) == GeneralError::E_OK ? NativeRdb::E_ERROR
                                                                                    : NativeRdb::E_OK;
}

int RdbResultSetImpl::GetRowCount(int &count)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    count = resultSet_->GetCount();
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GetRowIndex(int &position) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    position = index_;
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GoTo(int offset)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    int64_t position = index_;
    if (!isValid(position + offset)) {
        return NativeRdb::E_ERROR;
    }
    index_ = position + offset;
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GoToRow(int position)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    if (!isValid(position)) {
        return NativeRdb::E_ERROR;
    }
    index_ = position;
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GoToFirstRow()
{
    return GoToRow(0);
}

int RdbResultSetImpl::GoToLastRow()
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    index_ = resultSet_->GetCount() - 1;
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GoToNextRow()
{
    return GoTo(1);
}

int RdbResultSetImpl::GoToPreviousRow()
{
    return GoTo(-1);
}

int RdbResultSetImpl::IsEnded(bool &result)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    result = index_ >= resultSet_->GetCount() || resultSet_->GetCount() <= 0;
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::IsStarted(bool &result) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    result = index_ < 0 || resultSet_->GetCount() <= 0;
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::IsAtFirstRow(bool &result) const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    result = resultSet_->GetCount() > 0 && index_ == 0;
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::IsAtLastRow(bool &result)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    result = resultSet_->GetCount() > 0 && index_ == resultSet_->GetCount() - 1;
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GetBlob(int columnIndex, std::vector<uint8_t> &value)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    return Get(columnIndex, value);
}

int RdbResultSetImpl::GetString(int columnIndex, std::string &value)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    return Get(columnIndex,value);
}

int RdbResultSetImpl::GetInt(int columnIndex, int &value)
{
    int64_t tmpValue;
    int status = GetLong(columnIndex, tmpValue);
    if (status == NativeRdb::E_OK) {
        if (tmpValue < INT32_MIN || tmpValue > INT32_MAX) {
            ZLOGE("Get int value overflow.");
            return NativeRdb::E_ERROR;
        }
        value = static_cast<int32_t>(tmpValue);
    }
    return status;
}

int RdbResultSetImpl::GetLong(int columnIndex, int64_t &value)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    return Get(columnIndex,value);
}

int RdbResultSetImpl::GetDouble(int columnIndex, double &value)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    return Get(columnIndex,value);
}

int RdbResultSetImpl::IsColumnNull(int columnIndex, bool &isNull)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    DistributedData::Value var;
    auto status = resultSet_->Get(columnIndex, var);
    if (status != DistributedData::GeneralError::E_OK) {
        return NativeRdb::E_ERROR;
    }
    isNull = var.index() == DistributedData::TYPE_INDEX<std::monostate>;
    return NativeRdb::E_OK;
}

bool RdbResultSetImpl::IsClosed() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return resultSet_ == nullptr;
}

int RdbResultSetImpl::Close()
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (resultSet_ == nullptr) {
        ZLOGW("Result set has been closed.");
        return NativeRdb::E_OK;
    }
    resultSet_->Close();
    resultSet_ = nullptr;
    return NativeRdb::E_OK;
}

ColumnType RdbResultSetImpl::ConvertColumnType(int32_t columnType) const
{
    if (columnType >= GenColumnType::TYPE_BUTT || columnType < 0) {
        return ColumnType::TYPE_NULL;
    }
    return COLUMNTYPES[columnType];
}

bool RdbResultSetImpl::isValid(int64_t position) const
{
    return resultSet_ != nullptr && position >= 0 && position < resultSet_->GetCount();
}
} // namespace OHOS::DistributedRdb
