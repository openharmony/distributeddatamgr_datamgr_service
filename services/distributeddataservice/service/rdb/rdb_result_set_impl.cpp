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

using DistributedDB::DBStatus;
using OHOS::NativeRdb::ColumnType;

namespace OHOS::DistributedRdb {
RdbResultSetImpl::RdbResultSetImpl(std::shared_ptr<DistributedDB::ResultSet> &resultSet) : resultSet_(resultSet)
{
}

int RdbResultSetImpl::GetAllColumnNames(std::vector<std::string> &columnNames)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    resultSet_->GetColumnNames(columnNames);
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GetColumnCount(int &count)
{
    return NativeRdb::E_NOT_SUPPORT;
}

int RdbResultSetImpl::GetColumnType(int columnIndex, ColumnType &columnType)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    DbColumnType dbColumnType;
    DBStatus status = resultSet_->GetColumnType(columnIndex, dbColumnType);
    if (status != DBStatus::OK) {
        ZLOGE("DistributedDB resultSet operate failed, status is %{public}d.", status);
        return NativeRdb::E_ERROR;
    }
    columnType = ConvertColumnType(dbColumnType);
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GetColumnIndex(const std::string &columnName, int &columnIndex)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    DBStatus status = resultSet_->GetColumnIndex(columnName, columnIndex);
    if (status != DBStatus::OK) {
        ZLOGE("DistributedDB resultSet operate failed, status is %{public}d.", status);
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GetColumnName(int columnIndex, std::string &columnName)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    DBStatus status = resultSet_->GetColumnName(columnIndex, columnName);
    if (status != DBStatus::OK) {
        ZLOGE("DistributedDB resultSet operate failed, status is %{public}d.", status);
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GetRowCount(int &count)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    count = resultSet_->GetCount();
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GetRowIndex(int &position) const
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    position = resultSet_->GetPosition();
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GoTo(int offset)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    if (!resultSet_->Move(offset)) {
        ZLOGE("DistributedDB resultSet operate failed.");
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GoToRow(int position)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    if (!resultSet_->MoveToPosition(position)) {
        ZLOGE("DistributedDB resultSet operate failed.");
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GoToFirstRow()
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    if (!resultSet_->MoveToFirst()) {
        ZLOGE("DistributedDB resultSet operate failed.");
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GoToLastRow()
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    if (!resultSet_->MoveToLast()) {
        ZLOGE("DistributedDB resultSet operate failed.");
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GoToNextRow()
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    if (!resultSet_->MoveToNext()) {
        ZLOGE("DistributedDB resultSet operate failed.");
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GoToPreviousRow()
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    if (!resultSet_->MoveToPrevious()) {
        ZLOGE("DistributedDB resultSet operate failed.");
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::IsEnded(bool &result)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    result = resultSet_->IsAfterLast();
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::IsStarted(bool &result) const
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    result = resultSet_->IsBeforeFirst();
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::IsAtFirstRow(bool &result) const
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    result = resultSet_->IsFirst();
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::IsAtLastRow(bool &result)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    result = resultSet_->IsLast();
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GetBlob(int columnIndex, std::vector<uint8_t> &blob)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    DBStatus status = resultSet_->Get(columnIndex, blob);
    if (status != DBStatus::OK) {
        ZLOGE("DistributedDB resultSet operate failed, status is %{public}d.", status);
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GetString(int columnIndex, std::string &value)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    DBStatus status = resultSet_->Get(columnIndex, value);
    if (status != DBStatus::OK) {
        ZLOGE("DistributedDB resultSet operate failed, status is %{public}d.", status);
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GetInt(int columnIndex, int &value)
{
    return NativeRdb::E_NOT_SUPPORT;
}

int RdbResultSetImpl::GetLong(int columnIndex, int64_t &value)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    DBStatus status = resultSet_->Get(columnIndex, value);
    if (status != DBStatus::OK) {
        ZLOGE("DistributedDB resultSet operate failed, status is %{public}d.", status);
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::GetDouble(int columnIndex, double &value)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    DBStatus status = resultSet_->Get(columnIndex, value);
    if (status != DBStatus::OK) {
        ZLOGE("DistributedDB resultSet operate failed, status is %{public}d.", status);
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

int RdbResultSetImpl::IsColumnNull(int columnIndex, bool &isNull)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return NativeRdb::E_ERROR;
    }
    DBStatus status = resultSet_->IsColumnNull(columnIndex, isNull);
    if (status != DBStatus::OK) {
        ZLOGE("DistributedDB resultSet operate failed, status is %{public}d.", status);
        return NativeRdb::E_ERROR;
    }
    return NativeRdb::E_OK;
}

bool RdbResultSetImpl::IsClosed() const
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("DistributedDB resultSet is null.");
        return true;
    }
    return resultSet_->IsClosed();
}

int RdbResultSetImpl::Close()
{
    std::unique_lock<std::shared_mutex> lock(this->mutex_);
    if (resultSet_ == nullptr) {
        ZLOGE("Result set has been closed.");
        return NativeRdb::E_OK;
    }
    resultSet_->Close();
    resultSet_ = nullptr;
    return NativeRdb::E_OK;
}

ColumnType RdbResultSetImpl::ConvertColumnType(DbColumnType columnType) const
{
    switch (columnType) {
        case DbColumnType::INT64:
            return ColumnType::TYPE_INTEGER;
        case DbColumnType::STRING:
            return ColumnType::TYPE_STRING;
        case DbColumnType::BLOB:
            return ColumnType::TYPE_BLOB;
        case DbColumnType::DOUBLE:
            return ColumnType::TYPE_FLOAT;
        case DbColumnType::NULL_VALUE:
            return ColumnType::TYPE_NULL;
        default:
            return ColumnType::TYPE_NULL;
    }
}
}
