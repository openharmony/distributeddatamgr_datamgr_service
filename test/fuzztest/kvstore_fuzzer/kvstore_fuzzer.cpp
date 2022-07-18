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

#include <sys/stat.h>
#include <string>
#include <vector>
#include "single_kvstore_client.h"
#include "distributed_kv_data_manager.h"
#include "kvstore_fuzzer.h"

using namespace OHOS;
using namespace OHOS::DistributedKv;

namespace OHOS {
static std::shared_ptr<SingleKvStore> singleKvStore_ = nullptr;
static Status status_;

void SetUpTestCase(void)
{
    DistributedKvDataManager manager;
    Options options = { .createIfMissing = true, .encrypt = false, .autoSync = true,
                        .kvStoreType = KvStoreType::SINGLE_VERSION };
    options.area = EL1;
    options.baseDir = std::string("/data/service/el1/public/database/odmf");
    AppId appId = { "odmf" };
    /* define kvstore(database) name. */
    StoreId storeId = { "student_single" };
    mkdir(options.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    /* [create and] open and initialize kvstore instance. */
    status_ = manager.GetSingleKvStore(options, appId, storeId, singleKvStore_);
}

void TearDown(void)
{
    remove("/data/service/el1/public/database/odmf/key");
    remove("/data/service/el1/public/database/odmf/kvdb");
    remove("/data/service/el1/public/database/odmf");
}

void PutFuzz(const uint8_t *data, size_t size)
{
    std::string rawString(reinterpret_cast<const char *>(data), size);
    Key skey = {rawString};
    Value sval = {rawString};
    singleKvStore_->Put(skey, sval);
    singleKvStore_->Delete(skey);
}

void PutBatchFuzz(const uint8_t *data, size_t size)
{
    std::string rawString(reinterpret_cast<const char *>(data), size);
    std::string rawString1 = rawString + "test_1";
    std::string rawString2 = rawString + "test_2";
    std::string rawString3 = rawString + "test_3";
    std::vector<Entry> entries;
    std::vector<Key> keys;
    Entry entry1, entry2, entry3;
    entry1.key = {rawString1};
    entry1.value = {rawString1};
    entry2.key = {rawString2};
    entry2.value = {rawString2};
    entry3.key = {rawString3};
    entry3.value = {rawString3};
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);
    keys.push_back(entry1.key);
    keys.push_back(entry2.key);
    keys.push_back(entry3.key);
    singleKvStore_->PutBatch(entries);
    singleKvStore_->DeleteBatch(keys);
}

void GetFuzz(const uint8_t *data, size_t size)
{
    std::string rawString(reinterpret_cast<const char *>(data), size);
    Key skey = {rawString};
    Value sval = {rawString};
    Value sval1;
    singleKvStore_->Put(skey, sval);
    singleKvStore_->Get(skey, sval1);
    singleKvStore_->Delete(skey);
}

void GetEntriesFuzz1(const uint8_t *data, size_t size)
{
    std::string rawString(reinterpret_cast<const char *>(data), size);
    std::string keys = "test_";
    size_t sum = 10;
    std::vector<Entry> results;
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Put(rawString + keys + std::to_string(i), { keys + std::to_string(i) });
    }
    singleKvStore_->GetEntries(rawString, results);
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Delete(rawString + keys + std::to_string(i));
    }
}

void GetEntriesFuzz2(const uint8_t *data, size_t size)
{
    std::string rawString(reinterpret_cast<const char *>(data), size);
    DataQuery dataQuery;
    dataQuery.KeyPrefix(rawString);
    std::string keys = "test_";
    std::vector<Entry> entries;
    size_t sum = 10;
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Put(rawString + keys + std::to_string(i), keys + std::to_string(i));
    }
    singleKvStore_->GetEntries(dataQuery, entries);
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Delete(rawString + keys + std::to_string(i));
    }
}

void GetResultSetFuzz1(const uint8_t *data, size_t size)
{
    std::string rawString(reinterpret_cast<const char *>(data), size);
    std::string keys = "test_";
    auto fuzzedInt = static_cast<int>(size);
    std::shared_ptr<KvStoreResultSet> resultSet;
    size_t sum = 10;
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Put(rawString + keys + std::to_string(i), keys + std::to_string(i));
    }
    singleKvStore_->GetResultSet(rawString, resultSet);
    resultSet->Move(fuzzedInt);
    resultSet->MoveToPosition(fuzzedInt);
    Entry entry;
    resultSet->GetEntry(entry);
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Delete(rawString + keys + std::to_string(i));
    }
}

void GetResultSetFuzz2(const uint8_t *data, size_t size)
{
    std::string rawString(reinterpret_cast<const char *>(data), size);
    DataQuery dataQuery;
    dataQuery.KeyPrefix(rawString);
    std::string keys = "test_";
    std::shared_ptr<KvStoreResultSet> resultSet;
    size_t sum = 10;
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Put(rawString + keys + std::to_string(i), keys + std::to_string(i));
    }
    singleKvStore_->GetResultSet(dataQuery, resultSet);
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Delete(rawString + keys + std::to_string(i));
    }
}

void GetCountFuzz(const uint8_t *data, size_t size)
{
    int count;
    std::string rawString(reinterpret_cast<const char *>(data), size);
    DataQuery query;
    query.KeyPrefix(rawString);
    std::string keys = "test_";
    size_t sum = 10;
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Put(rawString + keys + std::to_string(i), keys + std::to_string(i));
    }
    singleKvStore_->GetCount(query, count);
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Delete(rawString + keys + std::to_string(i));
    }
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::SetUpTestCase();
    OHOS::PutFuzz(data, size);
    OHOS::PutBatchFuzz(data, size);
    OHOS::GetFuzz(data, size);
    OHOS::GetEntriesFuzz1(data, size);
    OHOS::GetEntriesFuzz2(data, size);
    OHOS::GetResultSetFuzz1(data, size);
    OHOS::GetResultSetFuzz2(data, size);
    OHOS::GetCountFuzz(data, size);
    OHOS::TearDown();
    /* Run your code on data */
    return 0;
}

