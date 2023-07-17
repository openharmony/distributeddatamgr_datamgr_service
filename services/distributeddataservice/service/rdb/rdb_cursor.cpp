/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "rdb_cursor.h"

#include "result_set.h"
#include "value_proxy.h"
#include "store_types.h"
namespace OHOS::DistributedRdb {
using namespace OHOS::DistributedData;
using namespace DistributedDB;
RdbCursor::RdbCursor(std::shared_ptr<DistributedDB::ResultSet> resultSet)
    : resultSet_(std::move(resultSet))
{
}

RdbCursor::~RdbCursor()
{
    if (resultSet_) {
        resultSet_->Close();
    }
    resultSet_ = nullptr;
}

int32_t RdbCursor::GetColumnNames(std::vector<std::string> &names) const
{
    resultSet_->GetColumnNames(names);
    return GeneralError::E_OK;
}

int32_t RdbCursor::GetColumnName(int32_t col, std::string &name) const
{
    return resultSet_->GetColumnName(col, name) == DBStatus::OK ? GeneralError::E_OK : GeneralError::E_ERROR;
}

int32_t RdbCursor::GetColumnType(int32_t col) const
{
    ResultSet::ColumnType dbColumnType = ResultSet::ColumnType::INVALID_TYPE;
    auto status = resultSet_->GetColumnType(col, dbColumnType);
    if (status != DBStatus::OK) {
        dbColumnType = ResultSet::ColumnType::INVALID_TYPE;
    }
    return Convert(dbColumnType);
}

int32_t RdbCursor::GetCount() const
{
    return resultSet_->GetCount();
}

int32_t RdbCursor::MoveToFirst()
{
    return resultSet_->MoveToFirst() ? GeneralError::E_OK : GeneralError::E_ERROR;
}

int32_t RdbCursor::MoveToNext()
{
    return resultSet_->MoveToNext() ? GeneralError::E_OK : GeneralError::E_ERROR;
}

int32_t RdbCursor::MoveToPrev()
{
    return resultSet_->MoveToPrevious() ? GeneralError::E_OK : GeneralError::E_ERROR;
}

int32_t RdbCursor::GetEntry(VBucket &entry)
{
    return GetRow(entry);
}

int32_t RdbCursor::GetRow(VBucket &data)
{
    std::map<std::string, VariantData> bucket;
    auto ret = resultSet_->GetRow(bucket);
    data = ValueProxy::Convert(std::move(bucket));
    return ret == DBStatus::OK ? GeneralError::E_OK : GeneralError::E_ERROR;
}

int32_t RdbCursor::Get(int32_t col, DistributedData::Value &value)
{
    std::string name;
    auto status = resultSet_->GetColumnName(col, name);
    if (status != DBStatus::OK) {
        return GeneralError::E_ERROR;
    }
    VBucket bucket;
    if (GetRow(bucket) != GeneralError::E_OK) {
        return GeneralError::E_ERROR;
    }
    value = bucket[name];
    return GeneralError::E_OK;
}

int32_t RdbCursor::Get(const std::string &col, DistributedData::Value &value)
{
    int32_t index = -1;
    auto ret = resultSet_->GetColumnIndex(col, index);
    if (ret != DBStatus::OK) {
        return GeneralError::E_ERROR;
    }
    return Get(index, value);
}

int32_t RdbCursor::Close()
{
    resultSet_->Close();
    return GeneralError::E_OK;
}

bool RdbCursor::IsEnd()
{
    return resultSet_->IsAfterLast();
}

int32_t RdbCursor::Convert(ResultSet::ColumnType columnType)
{
    switch (columnType) {
        case ResultSet::ColumnType::INT64:
            return TYPE_INDEX<int64_t>;
        case ResultSet::ColumnType::STRING:
            return TYPE_INDEX<std::string>;
        case ResultSet::ColumnType::BLOB:
            return TYPE_INDEX<std::vector<uint8_t>>;
        case ResultSet::ColumnType::DOUBLE:
            return TYPE_INDEX<double>;
        case ResultSet::ColumnType::NULL_VALUE:
            return TYPE_INDEX<std::monostate>;
        case ResultSet::ColumnType::INVALID_TYPE:
            return TYPE_INDEX<std::monostate>;
        default:
            return TYPE_INDEX<std::monostate>;
    }
}
} // namespace OHOS::DistributedRdb
