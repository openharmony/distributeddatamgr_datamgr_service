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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_CURSOR_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_CURSOR_H
#include "store/cursor.h"
#include "distributeddb/result_set.h"
namespace OHOS::DistributedRdb {
class RdbCursor : public DistributedData::Cursor {
public:
    explicit RdbCursor(std::shared_ptr<DistributedDB::ResultSet> resultSet);
    ~RdbCursor();
    int32_t GetColumnNames(std::vector<std::string> &names) const override;
    int32_t GetColumnName(int32_t col, std::string &name) const override;
    int32_t GetColumnIndex(const std::string &columnName) const override;
    int32_t GetIndex() const override;
    int32_t MoveToPre() override;
    int32_t MoveTo(int offset) override;
    int32_t MoveToRow(int position) override;
    int32_t MoveToLast() override;
    bool IsStart() override;
    bool IsFirst() override;
    bool IsLast() override;
    bool IsClosed() override;
    int32_t IsColumnNull(int32_t col, bool &isNull) const override;
    int32_t GetColumnType(int32_t col) const override;
    int32_t GetCount() const override;
    int32_t MoveToFirst() override;
    int32_t MoveToNext() override;
    int32_t GetEntry(DistributedData::VBucket &entry) override;
    int32_t GetRow(DistributedData::VBucket &data) override;
    int32_t Get(int32_t col, DistributedData::Value &value) override;
    int32_t Get(const std::string &col, DistributedData::Value &value) override;
    int32_t Close() override;
    bool IsEnd() override;

private:
    std::shared_ptr<DistributedDB::ResultSet> resultSet_;
    static ColumnType Convert(DistributedDB::ResultSet::ColumnType columnType);
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_CURSOR_H
