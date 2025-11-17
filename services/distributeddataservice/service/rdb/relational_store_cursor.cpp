/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "relational_store_cursor.h"
#include "log_print.h"
#include "rdb_errno.h"
#include "rdb_types.h"
#include "result_set.h"
#include <cstdint>

namespace OHOS::DistributedRdb {
using namespace OHOS::DistributedData;
RelationalStoreCursor::RelationalStoreCursor(std::shared_ptr<NativeRdb::ResultSet> resultSet)
    : resultSet_(resultSet)
{
}

RelationalStoreCursor::~RelationalStoreCursor()
{
    if (resultSet_ == nullptr) {
        return;
    }
    resultSet_ = nullptr;
}

int32_t RelationalStoreCursor::ConvertNativeRdbStatus(int32_t status) const
{
    switch (status) {
        case NativeRdb::E_OK:
            return GeneralError::E_OK;
        case NativeRdb::E_SQLITE_BUSY:
        case NativeRdb::E_DATABASE_BUSY:
        case NativeRdb::E_SQLITE_LOCKED:
            return GeneralError::E_BUSY;
        case NativeRdb::E_INVALID_ARGS:
        case NativeRdb::E_INVALID_ARGS_NEW:
            return GeneralError::E_INVALID_ARGS;
        case NativeRdb::E_ALREADY_CLOSED:
            return GeneralError::E_ALREADY_CLOSED;
        case NativeRdb::E_SQLITE_CORRUPT:
            return GeneralError::E_DB_CORRUPT;
        default:
            break;
    }
    return GeneralError::E_ERROR;
}

int32_t RelationalStoreCursor::GetColumnNames(std::vector<std::string> &names) const
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ALREADY_CLOSED;
    }
    auto ret = resultSet_->GetAllColumnNames(names);
    return ConvertNativeRdbStatus(ret);
}

int32_t RelationalStoreCursor::GetColumnName(int32_t col, std::string &name) const
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ALREADY_CLOSED;
    }
    auto ret = resultSet_->GetColumnName(col, name);
    return ConvertNativeRdbStatus(ret);
}

int32_t RelationalStoreCursor::GetColumnType(int32_t col) const
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ALREADY_CLOSED;
    }
    ColumnType columnType = ColumnType::TYPE_NULL;
    auto ret = resultSet_->GetColumnType(col, columnType);
    if (ret == NativeRdb::E_OK) {
        return static_cast<int32_t>(columnType);
    }
    ZLOGE("get column type failed:%{public}d", ret);
    return ConvertNativeRdbStatus(ret);
}

int32_t RelationalStoreCursor::GetCount() const
{
    if (resultSet_ == nullptr) {
        ZLOGE("resultSet is nullptr");
        return GeneralError::E_ALREADY_CLOSED;
    }
    int32_t maxCount = 0;
    auto ret = resultSet_->GetRowCount(maxCount);
    if (ret != NativeRdb::E_OK) {
        ZLOGE("get row count failed:%{public}d", ret);
        return ConvertNativeRdbStatus(ret);
    }
    return maxCount;
}

int32_t RelationalStoreCursor::MoveToFirst()
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ALREADY_CLOSED;
    }
    auto ret = resultSet_->GoToFirstRow();
    return ConvertNativeRdbStatus(ret);
}

int32_t RelationalStoreCursor::MoveToNext()
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ALREADY_CLOSED;
    }
    auto ret = resultSet_->GoToNextRow();
    return ConvertNativeRdbStatus(ret);
}

int32_t RelationalStoreCursor::MoveToPrev()
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ALREADY_CLOSED;
    }
    auto ret = resultSet_->GoToPreviousRow();
    return ConvertNativeRdbStatus(ret);
}

int32_t RelationalStoreCursor::GetEntry(DistributedData::VBucket &entry)
{
    return GetRow(entry);
}

int32_t RelationalStoreCursor::GetRow(DistributedData::VBucket &data)
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ALREADY_CLOSED;
    }
    NativeRdb::RowEntity rowEntity;
    auto ret = resultSet_->GetRow(rowEntity);
    std::map<std::string, NativeRdb::ValueObject> values = rowEntity.Get();
    data = ValueProxy::Convert(std::move(values));
    return ConvertNativeRdbStatus(ret);
}

int32_t RelationalStoreCursor::Get(int32_t col, DistributedData::Value &value)
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ALREADY_CLOSED;
    }
    NativeRdb::ValueObject valueObj;
    auto ret = resultSet_->Get(col, valueObj);
    value = ValueProxy::Convert(std::move(valueObj));
    return ConvertNativeRdbStatus(ret);
}

int32_t RelationalStoreCursor::Get(const std::string &col, DistributedData::Value &value)
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ALREADY_CLOSED;
    }
    int32_t index = -1;
    auto ret = resultSet_->GetColumnIndex(col, index);
    if (ret != GeneralError::E_OK) {
        return ConvertNativeRdbStatus(ret);
    }
    return Get(index, value);
}

int32_t RelationalStoreCursor::Close()
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ALREADY_CLOSED;
    }
    auto ret = resultSet_->Close();
    return ConvertNativeRdbStatus(ret);
}

bool RelationalStoreCursor::IsEnd()
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ALREADY_CLOSED;
    }
    bool isEnd;
    resultSet_->IsEnded(isEnd);
    return isEnd;
}
}