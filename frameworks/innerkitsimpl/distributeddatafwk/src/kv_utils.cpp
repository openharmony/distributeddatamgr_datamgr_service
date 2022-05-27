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

#define LOG_TAG "KvUtils"

#include <endian.h>
#include "kv_utils.h"
#include "cov_util.h"
#include "log_print.h"
#include "data_query.h"
#include "kvstore_datashare_bridge.h"

namespace OHOS {
namespace DistributedKv {
using namespace DataShare;
using namespace DistributedData;
const std::string KvUtils::KEY = "key";
const std::string KvUtils::VALUE = "value";
constexpr KvUtils::QueryHandler KvUtils::HANDLERS[LAST_TYPE];

std::shared_ptr<ResultSetBridge> KvUtils::ToResultSetBridge(std::shared_ptr<KvStoreResultSet> resultSet)
{
    if (resultSet == nullptr) {
        ZLOGE("param error, kvResultSet nullptr");
        return nullptr;
    }
    return std::make_shared<KvStoreDataShareBridge>(resultSet);
}

Status KvUtils::ToQuery(const DataSharePredicates &predicates, DataQuery &query)
{
    std::list<OperationItem> operations = predicates.GetOperationList();
    for (const auto &oper : operations) {
        if (oper.operation < 0 || oper.operation >= LAST_TYPE) {
            ZLOGE("operation param error");
            return Status::NOT_SUPPORT;
        }
        (*HANDLERS[oper.operation])(oper, query);
    }
    return Status::SUCCESS;
}

std::vector<Entry> KvUtils::ToEntries(const std::vector<DataShareValuesBucket> &valueBuckets)
{
    std::vector<Entry> entries;
    for (const auto &val : valueBuckets) {
        Entry entry = ToEntry(val);
        entries.push_back(entry);
    }
    return entries;
}

Entry KvUtils::ToEntry(const DataShareValuesBucket &valueBucket)
{
    std::map<std::string, DataShareValueObject> valuesMap;
    valueBucket.GetAll(valuesMap);
    if (valuesMap.empty()) {
        ZLOGE("valuesMap is null");
        return {};
    }
    Entry entry;
    Status status = ToEntryData(valuesMap, KEY, entry.key);
    if (status != Status::SUCCESS) {
        ZLOGE("GetEntry key failed: %{public}d", status);
        return {};
    }
    status = ToEntryData(valuesMap, VALUE, entry.value);
    if (status != Status::SUCCESS) {
        ZLOGE("GetEntry value failed: %{public}d", status);
        return {};
    }
    return entry;
}

Status KvUtils::GetKeys(const DataSharePredicates &predicates, std::vector<Key> &keys)
{
    std::list<OperationItem> operations = predicates.GetOperationList();
    if (operations.empty()) {
        ZLOGE("operations is null");
        return Status::ERROR;
    }

    std::vector<std::string> myKeys;
    for (const auto &oper : operations) {
        if (oper.operation != IN_KEY) {
            ZLOGE("find operation failed");
            return Status::NOT_SUPPORT;
        }
        std::vector<std::string> val = oper.para1;
        myKeys.insert(myKeys.end(), val.begin(), val.end());
    }
    for (const auto &it : myKeys) {
        keys.push_back(it.c_str());
    }
    return Status::SUCCESS;
}

Status KvUtils::ToEntryData(const std::map<std::string, DataShareValueObject> &valuesMap,
    const std::string field, Blob &blob)
{
    auto it = valuesMap.find(field);
    if (it == valuesMap.end()) {
        ZLOGE("field is not find!");
        return Status::ERROR;
    }
    DataShareValueObjectType type = it->second.GetType();

    std::vector<uint8_t> uData;
    if (type == DataShareValueObjectType::TYPE_BLOB) {
        ZLOGE("Value bucket type blob");
        std::vector<uint8_t> data = it->second;
        uData.push_back(KvUtils::BYTE_ARRAY);
        uData.insert(uData.end(), data.begin(), data.end());
    } else if (type == DataShareValueObjectType::TYPE_INT) {
        ZLOGE("Value bucket type int");
        int64_t data = it->second;
        uint64_t data64 = htobe64(*reinterpret_cast<uint64_t*>(&data));
        uint8_t *dataU8 = reinterpret_cast<uint8_t*>(&data64);
        uData.push_back(KvUtils::INTEGER);
        uData.insert(uData.end(), dataU8, dataU8 + sizeof(int64_t) / sizeof(uint8_t));
    } else if (type == DataShareValueObjectType::TYPE_DOUBLE) {
        ZLOGE("Value bucket type double");
        double data = it->second;
        uint64_t data64 = htobe64(*reinterpret_cast<uint64_t*>(&data));
        uint8_t *dataU8 = reinterpret_cast<uint8_t*>(&data64);
        uData.push_back(KvUtils::DOUBLE);
        uData.insert(uData.end(), dataU8, dataU8 + sizeof(double) / sizeof(uint8_t));
    } else if (type == DataShareValueObjectType::TYPE_BOOL) {
        ZLOGE("Value bucket type bool");
        bool data = it->second;
        uData.push_back(KvUtils::BOOLEAN);
        uData.push_back(static_cast<uint8_t>(data));
    } else if (type == DataShareValueObjectType::TYPE_STRING) {
        ZLOGE("Value bucket type string");
        std::string data = it->second;
        uData.push_back(KvUtils::STRING);
        uData.assign(data.begin(), data.end());
    }
    blob = Blob(uData);
    return Status::SUCCESS;
}

void KvUtils::InKeys(const OperationItem &oper, DataQuery &query)
{
    query.InKeys(oper.para1);
}

void KvUtils::KeyPrefix(const OperationItem &oper, DataQuery &query)
{
    query.KeyPrefix(oper.para1);
}

void KvUtils::EqualTo(const OperationItem &oper, DataQuery &query)
{
    Querys equal(&query, QueryType::EQUAL);
    CovUtil::FillField(oper.para1, oper.para2.value, equal);
}

void KvUtils::NotEqualTo(const OperationItem &oper, DataQuery &query)
{
    Querys notEqual(&query, QueryType::NOT_EQUAL);
    CovUtil::FillField(oper.para1, oper.para2.value, notEqual);
}

void KvUtils::GreaterThan(const OperationItem &oper, DataQuery &query)
{
    Querys greater(&query, QueryType::GREATER);
    CovUtil::FillField(oper.para1, oper.para2.value, greater);
}

void KvUtils::LessThan(const OperationItem &oper, DataQuery &query)
{
    Querys less(&query, QueryType::LESS);
    CovUtil::FillField(oper.para1, oper.para2.value, less);
}

void KvUtils::GreaterThanOrEqualTo(const OperationItem &oper, DataQuery &query)
{
    Querys greaterOrEqual(&query, QueryType::GREATER_OR_EQUAL);
    CovUtil::FillField(oper.para1, oper.para2.value, greaterOrEqual);
}

void KvUtils::LessThanOrEqualTo(const OperationItem &oper, DataQuery &query)
{
    Querys lessOrEqual(&query, QueryType::LESS_OR_EQUAL);
    CovUtil::FillField(oper.para1, oper.para2.value, lessOrEqual);
}

void KvUtils::And(const OperationItem &oper, DataQuery &query)
{
    query.And();
}

void KvUtils::Or(const OperationItem &oper, DataQuery &query)
{
    query.Or();
}

void KvUtils::IsNull(const OperationItem &oper, DataQuery &query)
{
    query.IsNull(oper.para1);
}

void KvUtils::IsNotNull(const OperationItem &oper, DataQuery &query)
{
    query.IsNotNull(oper.para1);
}

void KvUtils::In(const OperationItem &oper, DataQuery &query)
{
    InOrNotIn in(&query, QueryType::IN);
    CovUtil::FillField(oper.para1, oper.para2.value, in);
}

void KvUtils::NotIn(const OperationItem &oper, DataQuery &query)
{
    InOrNotIn notIn(&query, QueryType::NOT_IN);
    CovUtil::FillField(oper.para1, oper.para2.value, notIn);
}

void KvUtils::Like(const OperationItem &oper, DataQuery &query)
{
    query.Like(oper.para1, oper.para2);
}

void KvUtils::Unlike(const OperationItem &oper, DataQuery &query)
{
    query.Unlike(oper.para1, oper.para2);
}

void KvUtils::OrderByAsc(const OperationItem &oper, DataQuery &query)
{
    query.OrderByAsc(oper.para1);
}

void KvUtils::OrderByDesc(const OperationItem &oper, DataQuery &query)
{
    query.OrderByDesc(oper.para1);
}

void KvUtils::Limit(const OperationItem &oper, DataQuery &query)
{
    query.Limit(oper.para1, oper.para2);
}
} // namespace DistributedKv
} // namespace OHOS