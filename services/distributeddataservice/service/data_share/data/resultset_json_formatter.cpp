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

#include "rdb_errno.h"
#include "log_print.h"

namespace OHOS::DataShare {
bool ResultSetJsonFormatter::Marshal(json &node) const
{
    int columnCount = 0;
    auto result = resultSet->GetColumnCount(columnCount);
    if (result != NativeRdb::E_OK) {
        ZLOGE("GetColumnCount err, %{public}d", result);
        return false;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        json result;
        for (int i = 0; i < columnCount; i++) {
            std::string columnName;
            std::string value;
            resultSet->GetColumnName(i, columnName);
            resultSet->GetString(i, value);
            SetValue(result[columnName], value);
        }
        node.push_back(result);
    }
    return true;
}

bool ResultSetJsonFormatter::Unmarshal(const DistributedData::Serializable::json &node)
{
    return false;
}
} // namespace OHOS::DataShare