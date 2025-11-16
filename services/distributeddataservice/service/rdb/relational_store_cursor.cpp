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
#include "rdb_types.h"
#include "result_set.h"
#include <cstdint>

namespace OHOS::DistributedRdb {
RelationalStoreCursor::RelationalStoreCursor(std::shared_ptr<NativeRdb::ResultSet> resultSet)
    : resultSet_(resultSet)
{
}

RelationalStoreCursor::~RelationalStoreCursor()
{
    resultSet_->Close();
}

int32_t RelationalStoreCursor::GetColumnNames(std::vector<std::string> &names) const
{
    auto errCode = resultSet_->GetAllColumnNames(names);
    if (errCode != NativeRdb::E_OK) {
        return errCode;
    }
    return NativeRdb::E_OK;
}

int32_t RelationalStoreCursor::GetColumnName(int32_t col, std::string &name) const
{
    return resultSet_->GetColumnName(col, name);
}

int32_t RelationalStoreCursor::GetColumnType(int32_t col) const
{
    ColumnType columnType = ColumnType::TYPE_NULL;
    int32_t errCode = resultSet_->GetColumnType(col, columnType);
    if (errCode != NativeRdb::E_OK) {
        columnType = ColumnType::TYPE_NULL;
    }
    return static_cast<int32_t>(columnType);
}

int32_t RelationalStoreCursor::GetCount() const
{
    int32_t maxCount = 0;
    resultSet_->GetRowCount(maxCount);
    return maxCount;
}

int32_t RelationalStoreCursor::MoveToFirst()
{
    return resultSet_->GoToFirstRow() ? NativeRdb::E_OK : NativeRdb::E_ERROR;
}

int32_t RelationalStoreCursor::MoveToNext()
{
    return resultSet_->GoToNextRow() ? NativeRdb::E_OK : NativeRdb::E_ERROR;
}

int32_t RelationalStoreCursor::MoveToPrev()
{
    return resultSet_->GoToPreviousRow() ? NativeRdb::E_OK : NativeRdb::E_ERROR;
}

int32_t RelationalStoreCursor::GetEntry(DistributedData::VBucket &entry)
{
    return GetRow(entry);
}

int32_t RelationalStoreCursor::GetRow(DistributedData::VBucket &data)
{
    NativeRdb::RowEntity rowEntity;
    auto ret = resultSet_->GetRow(rowEntity);
    std::map<std::string, NativeRdb::ValueObject> values = rowEntity.Get();
    data = ValueProxy::Convert(std::move(values));
    return ret == NativeRdb::E_OK ? NativeRdb::E_OK : NativeRdb::E_ERROR;
}

int32_t RelationalStoreCursor::Get(int32_t col, DistributedData::Value &value)
{
    NativeRdb::ValueObject valueObj;
    auto ret = resultSet_->Get(col, valueObj);
    value = ValueProxy::Convert(std::move(valueObj));
    return ret == NativeRdb::E_OK ? NativeRdb::E_OK : NativeRdb::E_ERROR;
}

int32_t RelationalStoreCursor::Get(const std::string &col, DistributedData::Value &value)
{
    int32_t index = -1;
    auto ret = resultSet_->GetColumnIndex(col, index);
    if (ret != NativeRdb::E_OK) {
        return NativeRdb::E_ERROR;
    }
    return Get(col, value);
}

int32_t RelationalStoreCursor::Close()
{
    return resultSet_->Close();
}

bool RelationalStoreCursor::IsEnd()
{
    bool isEnd;
    resultSet_->IsEnded(isEnd);
    return isEnd;
}
}