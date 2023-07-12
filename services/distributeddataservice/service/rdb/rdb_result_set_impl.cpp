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
#include "rdb_errno.h"
#include "store_types.h"
#include "rdb_result_set_impl.h"
#include "store/cursor.h"

using DistributedDB::DBStatus;
using OHOS::NativeRdb::ColumnType;

namespace OHOS::DistributedRdb {
using OHOS::DistributedData::GeneralError;
using Cursor = OHOS::DistributedData::Cursor;
RdbResultSetImpl::RdbResultSetImpl(std::shared_ptr<Cursor> resultSet)
{
    resultSet_ = std::move(resultSet);
}

int RdbResultSetImpl::GetAllColumnNames(std::vector<std::string> &columnNames)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
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
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    columnType = ConvertColumnType(resultSet_->GetColumnType(columnIndex));
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GetColumnIndex(const std::string &columnName, int &columnIndex)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    columnIndex = resultSet_->GetColumnIndex(columnName);
    if (columnIndex < 0) {
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GetColumnName(int columnIndex, std::string &columnName)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    return resultSet_->GetColumnName(columnIndex, columnName) == GeneralError::E_OK ? NativeRdb::E_ERROR
                                                                                    : NativeRdb::E_OK;
}

int RdbResultSetImpl::GetRowCount(int &count)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    count = resultSet_->GetCount();
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GetRowIndex(int &position) const
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    position = resultSet_->GetIndex();
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GoTo(int offset)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    return resultSet_->MoveTo(offset) == GeneralError::E_OK ? NativeRdb::E_OK : NativeRdb::E_ERROR;
}

int RdbResultSetImpl::GoToRow(int position)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    return resultSet_->MoveToRow(position) == GeneralError::E_OK ? NativeRdb::E_OK : NativeRdb::E_ERROR;
}

int RdbResultSetImpl::GoToFirstRow()
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    return resultSet_->MoveToFirst() == GeneralError::E_OK ? NativeRdb::E_OK : NativeRdb::E_ERROR;
}

int RdbResultSetImpl::GoToLastRow()
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    return resultSet_->MoveToLast() == GeneralError::E_OK ? NativeRdb::E_OK : NativeRdb::E_ERROR;
}

int RdbResultSetImpl::GoToNextRow()
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    return resultSet_->MoveToNext() == GeneralError::E_OK ? NativeRdb::E_OK : NativeRdb::E_ERROR;
}

int RdbResultSetImpl::GoToPreviousRow()
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    return resultSet_->MoveToPre() == GeneralError::E_OK ? NativeRdb::E_OK : NativeRdb::E_ERROR;
}

int RdbResultSetImpl::IsEnded(bool &result)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    result = resultSet_->IsEnd();
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::IsStarted(bool &result) const
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    result = resultSet_->IsStart();
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::IsAtFirstRow(bool &result) const
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    result = resultSet_->IsFirst();
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::IsAtLastRow(bool &result)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    result = resultSet_->IsLast();
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GetBlob(int columnIndex, std::vector<uint8_t> &value)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    DistributedData::Value var;
    auto status = resultSet_->Get(columnIndex, var);
    if (status != DistributedData::GeneralError::E_OK) {
        return NativeRdb::E_ERROR;
    }
    return DistributedData::Convert(std::move(var), value) ? NativeRdb::E_OK : NativeRdb::E_ERROR;
}

int RdbResultSetImpl::GetString(int columnIndex, std::string &value)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    DistributedData::Value var;
    auto status = resultSet_->Get(columnIndex, var);
    if (status != DistributedData::GeneralError::E_OK) {
        return NativeRdb::E_ERROR;
    }
    return DistributedData::Convert(std::move(var), value) ? NativeRdb::E_OK : NativeRdb::E_ERROR;
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
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    DistributedData::Value var;
    auto status = resultSet_->Get(columnIndex, var);
    if (status != DistributedData::GeneralError::E_OK) {
        return NativeRdb::E_ERROR;
    }
    return DistributedData::Convert(std::move(var), value) ? NativeRdb::E_OK : NativeRdb::E_ERROR;
}

int RdbResultSetImpl::GetDouble(int columnIndex, double &value)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    DistributedData::Value var;
    auto status = resultSet_->Get(columnIndex, var);
    if (status != DistributedData::GeneralError::E_OK) {
        return NativeRdb::E_ERROR;
    }
    return DistributedData::Convert(std::move(var), value) ? NativeRdb::E_OK : NativeRdb::E_ERROR;
}

int RdbResultSetImpl::IsColumnNull(int columnIndex, bool &isNull)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        return NativeRdb::E_STEP_RESULT_CLOSED;
    }
    auto status = resultSet_->IsColumnNull(columnIndex, isNull);
    return status == DistributedData::GeneralError::E_OK ? NativeRdb::E_OK : NativeRdb::E_ERROR;
}

bool RdbResultSetImpl::IsClosed() const
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGW("resultSet already closed.");
        return true;
    }
    return resultSet_->IsClosed();
}

int RdbResultSetImpl::Close()
{
    std::unique_lock<std::shared_mutex> lock(this->mutex_);
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
} // namespace OHOS::DistributedRdb
