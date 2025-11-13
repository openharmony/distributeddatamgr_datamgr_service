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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RELATIONALSTORE_CURSOR_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RELATIONALSTORE_CURSOR_H
#include "result_set.h"
namespace OHOS::DistributedRdb {
using namespace NativeRdb;
class RelationalStoreCursor {
public:
    explicit RelationalStoreCursor(std::shared_ptr<OHOS::NativeRdb::ResultSet> resultSet,
        bool isNeedTerminator = true);
    virtual ~RelationalStoreCursor() = default;
    int32_t GetColumnCount(int &count);
    int32_t GetColumnType(int32_t columnIndex, ColumnType &columnType)
    int32_t GetColumnIndex(const std::string &name, int &columnIndex);
    int32_t GetColumnName(int32_t columnIndex, std::string &columnName);
    int32_t GetRowCount(int &count);
    int32_t GoToNextRow();
    int32_t GetSize(int columnIndex, size_t &size);
    int32_t GetText(int columnIndex, std::string &value);
    int32_t GetInt64(int columnIndex, int64_t &value);
    int32_t GetReal(int columnIndex, double &value);
    int32_t GetBlob(int32_t columnIndex, std::vector<uint8_t> &vec);
    int32_t GetAsset(int32_t columnIndex, NativeRdb::AssetValue &value);
    int32_t GetAssets(int32_t columnIndex, std::vector<NativeRdb::AssetValue> &assets);
    int32_t Destroy();

private:
    std::shared_ptr<OHOS::NativeRdb::ResultSet> resultSet_;
    bool isNeedTerminator_;
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_CURSOR_H
