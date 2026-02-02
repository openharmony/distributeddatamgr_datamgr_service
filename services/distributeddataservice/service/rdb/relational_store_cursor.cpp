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
#include "rdb_utils.h"
#include "rdb_types.h"
#include "result_set.h"
#include "value_proxy.h"

namespace OHOS::DistributedRdb {
using namespace OHOS::DistributedData;
RelationalStoreCursor::RelationalStoreCursor(NativeRdb::ResultSet &resultSet,
    std::shared_ptr<NativeRdb::ResultSet> hold)
    : resultSet_(resultSet), hold_(std::move(hold))
{
}

RelationalStoreCursor::~RelationalStoreCursor()
{
    resultSet_.Close();
}

int32_t RelationalStoreCursor::GetColumnNames(std::vector<std::string> &names) const
{
    auto ret = resultSet_.GetAllColumnNames(names);
    return RdbUtils::ConvertNativeRdbStatus(ret);
}

int32_t RelationalStoreCursor::GetColumnName(int32_t col, std::string &name) const
{
    auto ret = resultSet_.GetColumnName(col, name);
    return RdbUtils::ConvertNativeRdbStatus(ret);
}

int32_t RelationalStoreCursor::GetColumnType(int32_t col) const
{
    ColumnType columnType = ColumnType::TYPE_NULL;
    auto ret = resultSet_.GetColumnType(col, columnType);
    if (ret != NativeRdb::E_OK) {
        ZLOGE("get column type failed:%{public}d", ret);
    }
    return static_cast<int32_t>(columnType);
}

int32_t RelationalStoreCursor::GetCount() const
{
    int32_t maxCount = 0;
    auto ret = resultSet_.GetRowCount(maxCount);
    if (ret != NativeRdb::E_OK) {
        ZLOGE("get row count failed:%{public}d", ret);
    }
    return maxCount;
}

int32_t RelationalStoreCursor::MoveToFirst()
{
    auto ret = resultSet_.GoToFirstRow();
    return RdbUtils::ConvertNativeRdbStatus(ret);
}

int32_t RelationalStoreCursor::MoveToNext()
{
    auto ret = resultSet_.GoToNextRow();
    return RdbUtils::ConvertNativeRdbStatus(ret);
}

int32_t RelationalStoreCursor::MoveToPrev()
{
    auto ret = resultSet_.GoToPreviousRow();
    return RdbUtils::ConvertNativeRdbStatus(ret);
}

int32_t RelationalStoreCursor::GetEntry(DistributedData::VBucket &entry)
{
    return GetRow(entry);
}

int32_t RelationalStoreCursor::GetRow(DistributedData::VBucket &data)
{
    NativeRdb::RowEntity rowEntity;
    auto ret = resultSet_.GetRow(rowEntity);
    std::map<std::string, NativeRdb::ValueObject> values = rowEntity.Steal();
    data = ValueProxy::Convert(std::move(values));
    return RdbUtils::ConvertNativeRdbStatus(ret);
}

int32_t RelationalStoreCursor::Get(int32_t col, DistributedData::Value &value)
{
    NativeRdb::ValueObject valueObj;
    auto ret = resultSet_.Get(col, valueObj);
    value = ValueProxy::Convert(std::move(valueObj));
    return RdbUtils::ConvertNativeRdbStatus(ret);
}

int32_t RelationalStoreCursor::Get(const std::string &col, DistributedData::Value &value)
{
    int32_t index = -1;
    auto ret = resultSet_.GetColumnIndex(col, index);
    if (ret != NativeRdb::E_OK) {
        ZLOGE("get column index failed:%{public}d", ret);
        return RdbUtils::ConvertNativeRdbStatus(ret);
    }
    return Get(index, value);
}

int32_t RelationalStoreCursor::Close()
{
    auto ret = resultSet_.Close();
    return RdbUtils::ConvertNativeRdbStatus(ret);
}

bool RelationalStoreCursor::IsEnd()
{
    bool isEnd = false;
    resultSet_.IsEnded(isEnd);
    return isEnd;
}
} // namespace OHOS::DistributedRdb
