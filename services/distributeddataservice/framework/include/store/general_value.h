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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_VALUE_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_VALUE_H
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include "traits.h"
#include "error/general_error.h"
namespace OHOS::DistributedData {
struct Asset {
    uint32_t version;
    std::string name;
    std::string uri;
    std::string createTime;
    std::string modifyTime;
    std::string size;
    std::string hash;
};

struct GenQuery {
    virtual ~GenQuery() = default;
    virtual int32_t GetInterfaceId() = 0;
};

using Assets = std::vector<Asset>;
using Bytes = std::vector<uint8_t>;
using Value = std::variant<std::monostate, int64_t, double, std::string, bool, Bytes, Asset, Assets>;
using Values = std::vector<Value>;
using VBucket = std::map<std::string, Value>;
using VBuckets = std::vector<VBucket>;

template<typename T>
inline constexpr size_t TYPE_INDEX = Traits::variant_index_of_v<T, Value>;

inline constexpr size_t TYPE_MAX = Traits::variant_size_of_v<Value>;

template<typename T, typename O>
bool GetItem(const T &input, O &output)
{
    return false;
}

template<typename T, typename O, typename First, typename... Rest>
bool GetItem(const T &input, O &output)
{
    auto val =  Traits::get_if<First>(&input);
    if (val != nullptr) {
        output = *val;
        return true;
    }
    return GetItem<T, O, Rest...>(input, output);
}

template<typename T, typename... Types>
bool Convert(const T &input, std::variant<Types...> &output)
{
    return GetItem<T, decltype(output), Types...>(input, output);
}
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_VALUE_H
