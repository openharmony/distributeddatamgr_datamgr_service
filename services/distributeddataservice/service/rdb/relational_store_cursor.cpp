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

#include "relational_cursor.h"

namespace OHOS::DistributedRdb {
using namespace NativeRdb;
RelationalStoreCursor::RelationalStoreCursor(std::shared_ptr<OHOS::NativeRdb::ResultSet> resultSet,
    bool isNeedTerminator) : resultSet_(std::move(resultSet)), isNeedTerminator_(isNeedTerminator)
{
}

int32_t RelationalStoreCursor::GetColumnCount(int &count)  
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ERROR;
    }
    return resultSet_->GetColumnCount(count);
}

int32_t RelationalStoreCursor::GetColumnType(int32_t columnIndex, ColumnType &columnType)
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ERROR;
    }
    return resultSet_->GetColumnType(columnIndex, columnType);
}

int32_t RelationalStoreCursor::GetColumnIndex(const std::string &name, int &columnIndex)
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ERROR;
    }
    return resultSet_->GetColumnIndex(name, columnIndex);
}

int32_t RelationalStoreCursor::GetColumnName(int32_t columnIndex, std::string &columnName)
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ERROR;
    }
    return resultSet_->GetColumnName(columnIndex, columnName);
}

int32_t RelationalStoreCursor::GetRowCount(int &count)
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ERROR;
    }
    return resultSet_->GetRowCount(count);
}

int32_t RelationalStoreCursor::GoToNextRow()
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ERROR;
    }
    return resultSet_->GoToNextRow();
}

int32_t RelationalStoreCursor::GetSize(int columnIndex, size_t &size)
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ERROR;
    }
    return resultSet_->GetSize(columnIndex, size);
}

int32_t RelationalStoreCursor::GetText(int columnIndex, std::string &value)
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ERROR;
    }
    return resultSet_->GetString(columnIndex, value);
}

int32_t RelationalStoreCursor::GetInt64(int columnIndex, int64_t &value)
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ERROR;
    }
    return resultSet_->GetLong(columnIndex, value);
}

int32_t RelationalStoreCursor::GetReal(int columnIndex, double &value)
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ERROR;
    }
    return resultSet_->GetDouble(columnIndex, value);
}

int32_t RelationalStoreCursor::GetBlob(int32_t columnIndex, std::vector<uint8_t> &vec)
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ERROR;
    }
    return resultSet_->GetBlob(columnIndex, vec);
}

int32_t RelationalStoreCursor::GetAsset(int32_t columnIndex, NativeRdb::AssetValue &value)
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ERROR;
    }
    return resultSet_->GetAsset(columnIndex, value);
}

int32_t RelationalStoreCursor::GetAssets(int32_t columnIndex, std::vector<NativeRdb::AssetValue> &assets)
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ERROR;
    }
    return resultSet_->GetAssets(columnIndex, assets);
}

int32_t RelationalStoreCursor::Destroy()
{
    if (resultSet_ == nullptr) {
        return GeneralError::E_ERROR;
    }
    return resultSet_->Close();
}