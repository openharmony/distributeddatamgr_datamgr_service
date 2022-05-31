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

#ifndef KV_UTILS_H
#define KV_UTILS_H

#include "types.h"
#include "datashare_abs_predicates.h"
#include "data_query.h"
#include "datashare_values_bucket.h"
#include "kvstore_result_set.h"
#include "result_set_bridge.h"

namespace OHOS {
namespace DistributedKv {
class KvUtils {
public:
    enum {
        STRING = 0,
        INTEGER = 1,
        FLOAT = 2,
        BYTE_ARRAY = 3,
        BOOLEAN = 4,
        DOUBLE = 5,
        INVALID = 255
    };
    static std::shared_ptr<DataShare::ResultSetBridge> ToResultSetBridge(std::shared_ptr<KvStoreResultSet> resultSet);
    static Status ToQuery(const DataShare::DataShareAbsPredicates &predicates, DataQuery &query);
    static Entry ToEntry(const DataShare::DataShareValuesBucket &valueBucket);
    static std::vector<Entry> ToEntries(const std::vector<DataShare::DataShareValuesBucket> &valueBuckets);
    static Status GetKeys(const DataShare::DataShareAbsPredicates &predicates, std::vector<Key> &keys);
private:
    static void NoSupport(const DataShare::OperationItem &oper, DataQuery &query);
    static void EqualTo(const DataShare::OperationItem &oper, DataQuery &query);
    static void NotEqualTo(const DataShare::OperationItem &oper, DataQuery &query);
    static void GreaterThan(const DataShare::OperationItem &oper, DataQuery &query);
    static void LessThan(const DataShare::OperationItem &oper, DataQuery &query);
    static void GreaterThanOrEqualTo(const DataShare::OperationItem &oper, DataQuery &query);
    static void LessThanOrEqualTo(const DataShare::OperationItem &oper, DataQuery &query);
    static void And(const DataShare::OperationItem &oper, DataQuery &query);
    static void Or(const DataShare::OperationItem &oper, DataQuery &query);
    static void IsNull(const DataShare::OperationItem &oper, DataQuery &query);
    static void IsNotNull(const DataShare::OperationItem &oper, DataQuery &query);
    static void In(const DataShare::OperationItem &oper, DataQuery &query);
    static void NotIn(const DataShare::OperationItem &oper, DataQuery &query);
    static void Like(const DataShare::OperationItem &oper, DataQuery &query);
    static void Unlike(const DataShare::OperationItem &oper, DataQuery &query);
    static void OrderByAsc(const DataShare::OperationItem &oper, DataQuery &query);
    static void OrderByDesc(const DataShare::OperationItem &oper, DataQuery &query);
    static void Limit(const DataShare::OperationItem &oper, DataQuery &query);
    static void InKeys(const DataShare::OperationItem &oper, DataQuery &query);
    static void KeyPrefix(const DataShare::OperationItem &oper, DataQuery &query);

    KvUtils(KvUtils &&) = delete;
    KvUtils(const KvUtils &) = delete;
    KvUtils &operator=(KvUtils &&) = delete;
    KvUtils &operator=(const KvUtils &) = delete;
    ~KvUtils() = delete;
    static Status ToEntryData(const std::map<std::string,
        DataShare::DataShareValueObject> &valuesMap, const std::string field, Blob &blob);
    static const std::string KEY;
    static const std::string VALUE;
    using QueryHandler = void (*)(const DataShare::OperationItem &, DataQuery &);
    static constexpr QueryHandler HANDLERS[DataShare::LAST_TYPE] = {
        [DataShare::INVALID_OPERATION] = &KvUtils::NoSupport,
        [DataShare::EQUAL_TO] = &KvUtils::EqualTo,
        [DataShare::NOT_EQUAL_TO] = &KvUtils::NotEqualTo,
        [DataShare::GREATER_THAN] = &KvUtils::GreaterThan,
        [DataShare::LESS_THAN] = &KvUtils::LessThan,
        [DataShare::GREATER_THAN_OR_EQUAL_TO] = &KvUtils::GreaterThanOrEqualTo,
        [DataShare::LESS_THAN_OR_EQUAL_TO] = &KvUtils::LessThanOrEqualTo,
        [DataShare::AND] = &KvUtils::And,
        [DataShare::OR] = &KvUtils::Or,
        [DataShare::IS_NULL] = &KvUtils::IsNull,
        [DataShare::IS_NOT_NULL] = &KvUtils::IsNotNull,
        [DataShare::IN] = &KvUtils::In,
        [DataShare::NOT_IN] = &KvUtils::NotIn,
        [DataShare::LIKE] = &KvUtils::Like,
        [DataShare::UNLIKE] = &KvUtils::Unlike,
        [DataShare::ORDER_BY_ASC] = &KvUtils::OrderByAsc,
        [DataShare::ORDER_BY_DESC] = &KvUtils::OrderByDesc,
        [DataShare::LIMIT] = &KvUtils::Limit,
        [DataShare::OFFSET] = &KvUtils::NoSupport,
        [DataShare::BEGIN_WARP] = &KvUtils::NoSupport,
        [DataShare::END_WARP] = &KvUtils::NoSupport,
        [DataShare::BEGIN_WITH] = &KvUtils::NoSupport,
        [DataShare::END_WITH] = &KvUtils::NoSupport,
        [DataShare::IN_KEY] = &KvUtils::InKeys,
        [DataShare::DISTINCT] = &KvUtils::NoSupport,
        [DataShare::GROUP_BY] = &KvUtils::NoSupport,
        [DataShare::INDEXED_BY] = &KvUtils::NoSupport,
        [DataShare::CONTAINS] = &KvUtils::NoSupport,
        [DataShare::GLOB] = &KvUtils::NoSupport,
        [DataShare::BETWEEN] = &KvUtils::NoSupport,
        [DataShare::NOTBETWEEN] = &KvUtils::NoSupport,
        [DataShare::KEY_PREFIX] = &KvUtils::KeyPrefix, 
        };
};
} // namespace DistributedKv
} // namespace OHOS
#endif // KV_UTILS_H