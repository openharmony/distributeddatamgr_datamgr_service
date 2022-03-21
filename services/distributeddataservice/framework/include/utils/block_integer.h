/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_UTILS_BLOCK_INTEGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_UTILS_BLOCK_INTEGER_H
#include "visibility.h"
namespace OHOS::DistributedData {
class BlockInteger {
public:
    API_EXPORT explicit BlockInteger(int interval) : interval_(interval) {};
    API_EXPORT BlockInteger(const BlockInteger &integer) : interval_(integer.interval_), value_(integer.value_) {};
    API_EXPORT BlockInteger &operator=(const BlockInteger &integer) = default;

    API_EXPORT ~BlockInteger() = default;

    API_EXPORT operator int() const
    {
        return value_;
    }
    API_EXPORT bool operator<(int other) const
    {
        return value_ < other;
    }

    API_EXPORT BlockInteger &operator=(int value);

    API_EXPORT BlockInteger &operator++();

    API_EXPORT BlockInteger operator++(int);

private:
    int interval_ = 0;
    int value_ = 0;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_UTILS_BLOCK_INTEGER_H
