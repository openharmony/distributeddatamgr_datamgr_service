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
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "error/general_error.h"
#include "traits.h"
namespace OHOS::DistributedData {
enum GenProgress {
    SYNC_BEGIN,
    SYNC_IN_PROGRESS,
    SYNC_FINISH,
};

struct GenStatistic {
    uint32_t total;
    uint32_t success;
    uint32_t failed;
    uint32_t untreated;
};

struct GenTableDetail {
    GenStatistic upload;
    GenStatistic download;
};

struct GenProgressDetail {
    int32_t progress;
    int32_t code;
    std::map<std::string, GenTableDetail> details;
};

struct Asset {
    enum Status : int32_t {
        STATUS_UNKNOWN,
        STATUS_NORMAL,
        STATUS_INSERT,
        STATUS_UPDATE,
        STATUS_DELETE,
        STATUS_ABNORMAL,
        STATUS_DOWNLOADING,
        STATUS_BUTT
    };

    static constexpr uint64_t NO_EXPIRES_TIME = 0;
    uint32_t version = 0;
    uint32_t status = STATUS_UNKNOWN;
    uint64_t expiresTime = NO_EXPIRES_TIME;
    std::string id;
    std::string name;
    std::string uri;
    std::string createTime;
    std::string modifyTime;
    std::string size;
    std::string hash;
    std::string path;
};

struct Reference {
    std::string sourceTable;
    std::string targetTable;
    std::map<std::string, std::string> refFields;
};

struct SyncParam {
    int32_t mode;
    int32_t wait;
    bool isCompensation = false;
};

using Assets = std::vector<Asset>;
using Bytes = std::vector<uint8_t>;
using Relations = std::map<std::string, std::string>;
using Value = std::variant<std::monostate, int64_t, double, std::string, bool, Bytes, Asset, Assets, Relations>;
using Values = std::vector<Value>;
using VBucket = std::map<std::string, Value>;
using VBuckets = std::vector<VBucket>;
using GenDetails = std::map<std::string, GenProgressDetail>;
using GenAsync = std::function<void(const GenDetails &details)>;
template<typename T>
inline constexpr size_t TYPE_INDEX = Traits::variant_index_of_v<T, Value>;

inline constexpr size_t TYPE_MAX = Traits::variant_size_of_v<Value>;

enum class QueryOperation : uint32_t {
    ILLEGAL = 0,
    IN = 1,
    OR = 0x101,
    AND,
    EQUAL_TO = 0x201,
    BEGIN_GROUP = 0x301,
    END_GROUP
};

struct QueryNode {
    QueryOperation op = QueryOperation::ILLEGAL;
    std::string fieldName;
    std::vector<Value> fieldValue;
};
using QueryNodes = std::vector<QueryNode>;

struct GenQuery {
    virtual ~GenQuery() = default;
    virtual bool IsEqual(uint64_t tid) = 0;
    virtual std::vector<std::string> GetTables() = 0;
    virtual void SetQueryNodes(const std::string& tableName, QueryNodes&& nodes) {};
    virtual QueryNodes GetQueryNodes(const std::string& tableName)
    {
        return {};
    };

    template<typename T>
    int32_t QueryInterface(T *&query)
    {
        if (!IsEqual(T::TYPE_ID)) {
            return E_INVALID_ARGS;
        }
        query = static_cast<T *>(this);
        return E_OK;
    };
};

template<typename T, typename O>
bool GetItem(T &&input, O &output)
{
    return false;
}

template<typename T, typename O, typename First, typename... Rest>
bool GetItem(T &&input, O &output)
{
    auto val = Traits::get_if<First>(&input);
    if (val != nullptr) {
        output = std::move(*val);
        return true;
    }
    return GetItem<T, O, Rest...>(std::move(input), output);
}

template<typename T, typename... Types>
bool Convert(T &&input, std::variant<Types...> &output)
{
    return GetItem<T, decltype(output), Types...>(std::move(input), output);
}
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_VALUE_H
