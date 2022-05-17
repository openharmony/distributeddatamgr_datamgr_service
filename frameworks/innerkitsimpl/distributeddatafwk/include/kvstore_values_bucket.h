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

#ifndef KVSTORE_VALUES_BUCKET_H
#define KVSTORE_VALUES_BUCKET_H

#include "types.h"
#include "datashare_values_bucket.h"

namespace OHOS {
namespace DistributedKv {
class KvStoreValuesBucket {
public:
    KvStoreValuesBucket() = default;
    ~KvStoreValuesBucket() = default;

    std::vector<Entry> ToEntry(const std::vector<DataShare::DataShareValuesBucket> &valueBuckets);
    Entry ToEntry(const DataShare::DataShareValuesBucket &valueBucket);

private:
    Status ToEntryData(const std::map<std::string, DataShare::DataShareValueObject> &valuesMap, const std::string field, Blob &kv);
    const std::string KEY = "key";
    const std::string VALUE = "value";
};
} // namespace DistributedKv
} // namespace OHOS
#endif // KVSTORE_VALUES_BUCKET_H