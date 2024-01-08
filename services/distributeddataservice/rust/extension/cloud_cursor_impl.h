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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_EXTENSION_CLOUD_CURSOR_IMPL_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_EXTENSION_CLOUD_CURSOR_IMPL_H

#include <set>
#include "basic_rust_types.h"
#include "cloud_ext_types.h"
#include "cloud/schema_meta.h"
#include "store/cursor.h"

namespace OHOS::CloudData {
using DBVBucket = DistributedData::VBucket;
using DBSchemaMeta = DistributedData::SchemaMeta;
using DBValue = DistributedData::Value;
using DBErr = DistributedData::GeneralError;
class CloudCursorImpl : public DistributedData::Cursor {
public:
    explicit CloudCursorImpl(OhCloudExtCloudDbData *cloudData);
    ~CloudCursorImpl();
    int32_t GetColumnNames(std::vector<std::string> &names) const override;
    int32_t GetColumnName(int32_t col, std::string &name) const override;
    int32_t GetColumnType(int32_t col) const override;
    int32_t GetCount() const override;
    int32_t MoveToFirst() override;
    int32_t MoveToNext() override;
    int32_t MoveToPrev() override;
    int32_t GetEntry(DBVBucket &entry) override;
    int32_t GetRow(DBVBucket &data) override;
    int32_t Get(int32_t col, DBValue &value) override;
    int32_t Get(const std::string &col, DBValue &value) override;
    int32_t Close() override;
    bool IsEnd() override;

private:
    enum Flag : int32_t {
        INSERT = 0,
        UPDATE = 1,
        DELETE = 2
    };
    static const size_t INVALID_INDEX = SIZE_MAX;
    static constexpr const char *OPERATION_KEY = "operation";
    static constexpr const char *GID_KEY = "id";
    static constexpr const char *CREATE_TIME_KEY = "createTime";
    static constexpr const char *MODIFY_TIME_KEY = "modifyTime";
    template<class T>
    inline static constexpr auto TYPE_INDEX = DistributedData::TYPE_INDEX<T>;

    std::vector<std::pair<std::string, DBValue>> GetData(OhCloudExtValueBucket *vb);
    DBValue GetExtend(OhCloudExtValueBucket *vb, const std::string &col);
    OhCloudExtCloudDbData *cloudData_ = nullptr;
    OhCloudExtVector *values_ = nullptr;
    size_t valuesLen_ = 0;
    bool finished_ = true;
    std::string cursor_;
    size_t index_ = INVALID_INDEX;
    bool consumed_ = false;
    std::vector<std::string> names_;
};
} // namespace OHOS::CloudData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_EXTENSION_CLOUD_CURSOR_IMPL_H
