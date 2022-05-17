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
#include "kvstore_values_bucket.h"

namespace OHOS {
namespace DistributedKv {
using namespace DataShare;
std::shared_ptr<ResultSetBridge> KvUtils::ToResultSetBridge(
    const std::shared_ptr<KvStoreResultSet> resultSet)
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

std::vector<Entry> KvUtils::ToEntry(const std::vector<DataShareValuesBucket> &valueBuckets)
{
    auto KvValuesBucket = std::make_shared<KvStoreValuesBucket>();
    std::vector<Entry> entries = KvValuesBucket->ToEntry(valueBuckets);
    if (entries.empty()) {
        ZLOGE("ToEntry entries failed");
        return {};
    }
    return entries;
}

Entry KvUtils::ToEntry(const DataShareValuesBucket &valueBucket)
{
    auto KvValuesBucket = std::make_shared<KvStoreValuesBucket>();
    return KvValuesBucket->ToEntry(valueBucket);
}

Status KvUtils::GetKeys(const DataSharePredicates &predicates, std::vector<Key> &keys)
{
    std::list<OperationItem> operationList = predicates.GetOperationList();
    if (operationList.empty()) {
        ZLOGE("operationList is null");
        return Status::ERROR;
    }

    std::vector<std::string> myKeys;
    for(const auto &oper : operationList)
    { 
        if (oper.operation != IN_KEY) {
            ZLOGE("find operation failed");
            return Status::NOT_SUPPORT;
        }
        std::vector<std::string> val;
        int status = oper.para1.GetStringVector(val);
        if (status != E_OK) {
            ZLOGE("GetStringVector failed: %{public}d", status);
            return Status::ERROR;
        }
        myKeys.insert(myKeys.end(), val.begin(), val.end());
    }
    for (const auto &it : myKeys)
    {
        keys.push_back(it.c_str());
    }
    return Status::SUCCESS;  
}
} // namespace DistributedKv
} // namespace OHOS