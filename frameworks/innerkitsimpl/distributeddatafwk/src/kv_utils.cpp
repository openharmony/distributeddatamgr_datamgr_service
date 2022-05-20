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

#include "kv_utils.h"
#include "kvstore_datashare_result_set.h"
#include "kvstore_predicates.h"
#include "log_print.h"
#include "data_query.h"
// #include "kvstore_values_bucket.h"

namespace OHOS {
namespace DistributedKv {
using namespace DataShare;
const std::string KvUtils::KEY = "key";
const std::string KvUtils::VALUE = "value";
std::shared_ptr<ResultSetBridge> KvUtils::ToResultSetBridge(
    std::shared_ptr<KvStoreResultSet> resultSet)
{
    if (resultSet == nullptr) {
        ZLOGE("param error, kvResultSet nullptr");
        return nullptr;
    }
    return std::make_shared<KvStoreDataShareResultSet>(resultSet);
}

Status KvUtils::ToQuery(const DataSharePredicates &predicates, DataQuery &query)
{
  
    auto kvPredicates = std::make_shared<KvStorePredicates>();
    Status status = kvPredicates->ToQuery(predicates, query);
    if (status != Status::SUCCESS) {
        ZLOGE("ToQuery failed: %{public}d", status);
        return status;
    }
    return status;
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

Status KvUtils::ToEntryData(const std::map<std::string, DataShareValueObject> &valuesMap, const std::string field, Blob &kv)
{
    auto it = valuesMap.find(field);
    if (it == valuesMap.end()) {
        ZLOGE("field is not find!");
        return Status::ERROR;
    }
    DataShareValueObjectType type = it->second.GetType();
    if (type != DataShareValueObjectType::TYPE_BLOB) {
        ZLOGE("key type is not TYPE_BLOB");
        return Status::ERROR;
    }
    std::vector<uint8_t> data;
    int status = it->second.GetBlob(data); 
    if (status != Status::SUCCESS) {
        ZLOGE("GetBlob failed: %{public}d", status);
        return Status::ERROR;
    }
    kv = data;
    return Status::SUCCESS;
}
} // namespace DistributedKv
} // namespace OHOS