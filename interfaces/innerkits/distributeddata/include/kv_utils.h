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
#include "datashare_predicates.h"
#include "data_query.h"
#include "datashare_values_bucket.h"
#include "kvstore_result_set.h"
#include "result_set_bridge.h"
#include "single_kvstore.h"

namespace OHOS {
namespace DistributedKv {
class KvUtils {
public:
    static std::shared_ptr<DataShare::ResultSetBridge> ToResultSetBridge(std::shared_ptr<KvStoreResultSet> resultSet);
    static Status ToQuery(const DataShare::DataSharePredicates &predicates, DataQuery &query);
    static Entry ToEntry(const DataShare::DataShareValuesBucket &valueBucket);
    static std::vector<Entry> ToEntries(const std::vector<DataShare::DataShareValuesBucket> &Buckets);
private:
    KvUtils(KvUtils &&) = delete;
    KvUtils(const KvUtils &) = delete;
    KvUtils &operator=(KvUtils &&) = delete;
    KvUtils &operator=(const KvUtils &) = delete;
    ~KvUtils() = delete;
    static Status ToEntryData(const std::map<std::string, DataShare::DataShareValueObject> &valuesMap, const std::string field, Blob &kv);
    static const std::string KEY;
    static const std::string VALUE;
};
} // namespace DistributedKv
} // namespace OHOS
#endif // KV_UTILS_H