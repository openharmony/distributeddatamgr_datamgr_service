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

#ifndef KVSTORE_PREDICATE_H
#define KVSTORE_PREDICATE_H

#include <map>
#include "data_query.h"
#include "types.h"
#include "datashare_predicates.h"

namespace OHOS {
namespace DistributedKv {
class KvStorePredicates {
public:
    struct Context {
        int intValue;
        int64_t longValue;
        double doubleValue;
        bool boolValue;
        std::string field;
        std::string stringValue;
        std::vector<int> intList;
        std::vector<int64_t> longList;
        std::vector<double> doubleList;
        std::vector<std::string> stringList;
        DataShare::DataSharePredicatesObjectType innerType;
    };
    KvStorePredicates() = default;
    ~KvStorePredicates() = default;
    Status ToQuery(const DataShare::DataSharePredicates &predicates, DataQuery &query);
    Status GetKeys(const DataShare::DataSharePredicates &predicates, std::vector<Key> &keys);
private:
    Status EqualTo(const DataShare::OperationItem &oper, DataQuery &query);
    Status NotEqualTo(const DataShare::OperationItem &oper, DataQuery &query);
    Status GreaterThan(const DataShare::OperationItem &oper, DataQuery &query);
    Status LessThan(const DataShare::OperationItem &oper, DataQuery &query);
    Status GreaterThanOrEqualTo(const DataShare::OperationItem &oper, DataQuery &query);
    Status LessThanOrEqualTo(const DataShare::OperationItem &oper, DataQuery &query);
    Status And(const DataShare::OperationItem &oper, DataQuery &query);
    Status Or(const DataShare::OperationItem &oper, DataQuery &query);
    Status IsNull(const DataShare::OperationItem &oper, DataQuery &query);
    Status IsNotNull(const DataShare::OperationItem &oper, DataQuery &query);
    Status In(const DataShare::OperationItem &oper, DataQuery &query);
    Status NotIn(const DataShare::OperationItem &oper, DataQuery &query);
    Status Like(const DataShare::OperationItem &oper, DataQuery &query);
    Status Unlike(const DataShare::OperationItem &oper, DataQuery &query);
    Status OrderByAsc(const DataShare::OperationItem &oper, DataQuery &query);
    Status OrderByDesc(const DataShare::OperationItem &oper, DataQuery &query);
    Status Limit(const DataShare::OperationItem &oper, DataQuery &query);
    Status InKeys(const DataShare::OperationItem &oper, DataQuery &query);
    Status KeyPrefix(const DataShare::OperationItem &oper, DataQuery &query);
    Status GetContext(const DataShare::OperationItem &oper, Context &context);
    using QueryHandler = Status (KvStorePredicates::*)(const DataShare::OperationItem &, DataQuery &);
    static constexpr QueryHandler HANDLERS[DataShare::LAST_TYPE] = {
            [DataShare::EQUAL_TO] = &KvStorePredicates::EqualTo,
            [DataShare::NOT_EQUAL_TO] = &KvStorePredicates::NotEqualTo,
            [DataShare::GREATER_THAN] = &KvStorePredicates::GreaterThan,
            [DataShare::LESS_THAN] = &KvStorePredicates::LessThan,
            [DataShare::GREATER_THAN_OR_EQUAL_TO] = &KvStorePredicates::GreaterThanOrEqualTo,
            [DataShare::LESS_THAN_OR_EQUAL_TO] = &KvStorePredicates::LessThanOrEqualTo,
            [DataShare::AND] = &KvStorePredicates::And,
            [DataShare::OR] = &KvStorePredicates::Or,
            [DataShare::IS_NULL] = &KvStorePredicates::IsNull,
            [DataShare::IS_NOT_NULL] = &KvStorePredicates::IsNotNull,
            [DataShare::NOT_IN] = &KvStorePredicates::NotIn,
            [DataShare::LIKE] = &KvStorePredicates::Like,
            [DataShare::UNLIKE] = &KvStorePredicates::Unlike,
            [DataShare::ORDER_BY_ASC] = &KvStorePredicates::OrderByAsc,
            [DataShare::ORDER_BY_DESC] = &KvStorePredicates::OrderByDesc,
            [DataShare::LIMIT] = &KvStorePredicates::Limit,
            [DataShare::IN_KEY] = &KvStorePredicates::InKeys,
            [DataShare::KEY_PREFIX] = &KvStorePredicates::KeyPrefix,
            [DataShare::IN] = &KvStorePredicates::In,
            };
};
}// namespace DistributedKv
}//namespace 

#endif  // KVSTORE_PREDICATE_H


