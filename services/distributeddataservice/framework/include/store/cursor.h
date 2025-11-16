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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_CURSOR_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_CURSOR_H
#include <map>
#include <variant>
#include <vector>

#include "store/general_value.h"
namespace OHOS::DistributedData {
class Cursor {
public:
    virtual ~Cursor() = default;

    virtual int32_t GetColumnNames(std::vector<std::string> &names) const = 0;

    virtual int32_t GetColumnName(int32_t col, std::string &name) const = 0;

    virtual int32_t GetColumnType(int32_t col) const = 0;

    virtual int32_t GetCount() const = 0;

    virtual int32_t MoveToFirst() = 0;

    virtual int32_t MoveToNext() = 0;

    virtual int32_t MoveToPrev() = 0;

    virtual int32_t GetEntry(VBucket &entry) = 0;

    virtual int32_t GetRow(VBucket &data) = 0;

    virtual int32_t Get(int32_t col, Value &value) = 0;

    virtual int32_t Get(const std::string &col, Value &value) = 0;

    virtual int32_t Close() = 0;

    virtual bool IsEnd() = 0;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_CURSOR_H
