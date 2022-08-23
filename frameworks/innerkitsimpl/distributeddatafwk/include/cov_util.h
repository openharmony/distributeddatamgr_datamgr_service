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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_INNERKITSIMPL_NATIVE_PREFERENCES_INCLUDE_COV_UTIL_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_INNERKITSIMPL_NATIVE_PREFERENCES_INCLUDE_COV_UTIL_H

#include <string>
#include <variant>
#include <set>
#include "data_query.h"

namespace OHOS {
namespace DistributedKv {
class CovUtil final {
public:
    template<typename _VTp, typename _TTp, typename _First>
    static auto FillField(const std::string &field, const _VTp &data, _TTp &target)
    {
        return target();
    }

    template<typename _VTp, typename _TTp, typename _First, typename _Second, typename ..._Rest>
    static auto FillField(const std::string &field, const _VTp &data, _TTp &target)
    {
        if ((sizeof ...(_Rest) + 1) == data.index()) {
            return target(field, std::get<(sizeof ...(_Rest) + 1)>(data));
        }
        return FillField<_VTp, _TTp, _Second, _Rest...>(field, data, target);
    }

    template<typename _TTp, typename ..._Types>
    static auto FillField(const std::string &field, const std::variant<_Types...> &data, _TTp &target)
    {
        return FillField<decltype(data), _TTp, _Types...>(field, data, target);
    }
};

enum class QueryType {
    EQUAL = 0,
    NOT_EQUAL = 1,
    GREATER = 2,
    LESS = 3,
    GREATER_OR_EQUAL = 4,
    LESS_OR_EQUAL = 5,
    IN = 6,
    NOT_IN = 7
};

class Querys {
public:
    Querys(OHOS::DistributedKv::DataQuery *dataQuery, QueryType type) : dataQuery_(dataQuery), type_(type) {};
    template<typename T>
    int operator()(const std::string &field, const T &value)
    {
        if (type_ == QueryType::EQUAL) {
            dataQuery_->EqualTo(field, value);
        } else if (type_ == QueryType::NOT_EQUAL) {
            dataQuery_->NotEqualTo(field, value);
        } else if (type_ == QueryType::GREATER) {
            dataQuery_->GreaterThan(field, value);
        } else if (type_ == QueryType::LESS) {
            dataQuery_->LessThan(field, value);
        } else if (type_ == QueryType::GREATER_OR_EQUAL) {
            dataQuery_->GreaterThanOrEqualTo(field, value);
        } else if (type_ == QueryType::LESS_OR_EQUAL) {
            dataQuery_->LessThanOrEqualTo(field, value);
        }
        return 0;
    }

    int operator()()
    {
        return 0;
    }

private:
    OHOS::DistributedKv::DataQuery *dataQuery_;
    QueryType type_;
};

class InOrNotIn {
public:
    InOrNotIn(OHOS::DistributedKv::DataQuery *dataQuery, QueryType type) : dataQuery_(dataQuery), type_(type)  {};
    template<typename T>
    int operator()(const std::string &field, const std::vector<T> &value)
    {
        if (type_ == QueryType::IN) {
            dataQuery_->In(field, value);
        } else if (type_ == QueryType::NOT_IN) {
            dataQuery_->NotIn(field, value);
        }
        return 0;
    }

    int operator()()
    {
        return 0;
    }

private:
    OHOS::DistributedKv::DataQuery *dataQuery_;
    QueryType type_;
};
}
}
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_INNERKITSIMPL_NATIVE_PREFERENCES_INCLUDE_COV_UTIL_H
