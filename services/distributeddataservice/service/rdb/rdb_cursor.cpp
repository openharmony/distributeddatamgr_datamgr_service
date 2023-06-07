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

#include "rdb_general_store.h"
#include "result_set.h"
#include "value_proxy.h"
namespace OHOS::DistributedRdb {
using namespace OHOS::DistributedData;
RdbCursor::RdbCursor(std::shared_ptr<NativeRdb::ResultSet> resultSet)
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
    return resultSet_->GetAllColumnNames(names);
}

int32_t RdbCursor::GetColumnName(int32_t col, std::string &name) const
{
    return resultSet_->GetColumnName(col, name);
}

int32_t RdbCursor::GetColumnType(int32_t col) const
{
    NativeRdb::ColumnType columnType;
    resultSet_->GetColumnType(col, columnType);
    return int32_t(columnType);
}

int32_t RdbCursor::GetCount() const
{
    int32_t count = -1;
    resultSet_->GetRowCount(count);
    return count;
}

int32_t RdbCursor::MoveToFirst()
{
    return resultSet_->GoToFirstRow();
}

int32_t RdbCursor::MoveToNext()
{
    return resultSet_->GoToNextRow();
}

int32_t RdbCursor::GetEntry(VBucket &entry)
{
    return GetRow(entry);
}

int32_t RdbCursor::GetRow(VBucket &data)
{
    NativeRdb::RowEntity bucket;
    auto ret = resultSet_->GetRow(bucket);
    data = ValueProxy::Convert(NativeRdb::ValuesBucket(bucket.Steal()));
    return ret;
}

int32_t RdbCursor::Get(int32_t col, Value &value)
{
    NativeRdb::ValueObject object;
    auto ret = resultSet_->Get(col, object);
    value = ValueProxy::Convert(std::move(object));
    return ret;
}

int32_t RdbCursor::Get(const std::string &col, Value &value)
{
    int32_t index = -1;
    auto ret = resultSet_->GetColumnIndex(col, index);
    if (ret != NativeRdb::E_OK) {
        return ret;
    }
    return Get(index, value);
}

int32_t RdbCursor::Close()
{
    return resultSet_->Close();
}

bool RdbCursor::IsEnd()
{
    return false;
}
} // namespace OHOS::DistributedRdb
