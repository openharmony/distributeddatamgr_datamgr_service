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
#define LOG_TAG "ResultSetJsonFormatter"
#include "resultset_json_formatter.h"

#include "log_print.h"
#include "rdb_errno.h"

namespace OHOS::DataShare {
bool ResultSetJsonFormatter::Marshal(json &node) const
{
    node = json::array();
    int columnCount = 0;
    auto result = resultSet->GetColumnCount(columnCount);
    if (result != NativeRdb::E_OK) {
        ZLOGE("GetColumnCount err, %{public}d", result);
        return false;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        if (!MarshalRow(node, columnCount)) {
            ZLOGE("MarshalRow err");
            return false;
        }
    }
    return true;
}

bool ResultSetJsonFormatter::Unmarshal(const DistributedData::Serializable::json &node)
{
    return false;
}

bool ResultSetJsonFormatter::MarshalRow(DistributedData::Serializable::json &node, int columnCount) const
{
    using namespace NativeRdb;
    json result;
    std::string columnName;
    NativeRdb::ColumnType type;
    for (int i = 0; i < columnCount; i++) {
        if (resultSet->GetColumnType(i, type) != E_OK) {
            ZLOGE("GetColumnType err %{public}d", i);
            return false;
        }
        if (resultSet->GetColumnName(i, columnName) != E_OK) {
            ZLOGE("GetColumnName err %{public}d", i);
            return false;
        }
        switch (type) {
            case ColumnType::TYPE_INTEGER:
                int64_t value;
                resultSet->GetLong(i, value);
                SetValue(result[columnName], value);
                break;
            case ColumnType::TYPE_FLOAT:
                double dValue;
                resultSet->GetDouble(i, dValue);
                SetValue(result[columnName], dValue);
                break;
            case ColumnType::TYPE_NULL:
                result[columnName] = nullptr;
                break;
            case ColumnType::TYPE_STRING: {
                std::string stringValue;
                resultSet->GetString(i, stringValue);
                SetValue(result[columnName], stringValue);
                break;
            }
            case ColumnType::TYPE_BLOB: {
                std::vector<uint8_t> blobValue;
                resultSet->GetBlob(i, blobValue);
                SetValue(result[columnName], blobValue);
                break;
            }
            default:
                ZLOGE("unknow type %{public}d", type);
                return false;
        }
    }
    node.push_back(result);
    return true;
}
} // namespace OHOS::DataShare