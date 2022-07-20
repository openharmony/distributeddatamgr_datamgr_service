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
    std::string skey(reinterpret_cast<const char *>(data), size);
    std::string svalue(reinterpret_cast<const char *>(data), size + 1);
    Key key = {skey};
    Value val = {svalue};
    singleKvStore_->Put(key, val);
    singleKvStore_->Delete(key);
}

void PutBatchFuzz(const uint8_t *data, size_t size)
{
    std::string skey(reinterpret_cast<const char *>(data), size);
    std::string svalue(reinterpret_cast<const char *>(data), size);
    std::vector<Entry> entries;
    std::vector<Key> keys;
    Entry entry1, entry2, entry3;
    entry1.key = {skey + "test_key1"};
    entry1.value = {svalue + "test_val1"};
    entry2.key = {skey + "test_key2"};
    entry2.value = {svalue + "test_val2"};
    entry3.key = {skey + "test_key3"};
    entry3.value = {svalue + "test_val3"};
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
    std::string skey(reinterpret_cast<const char *>(data), size);
    std::string svalue(reinterpret_cast<const char *>(data), size + 1);
    Key key = {skey};
    Value val = {svalue};
    Value val1;
    singleKvStore_->Put(key, val);
    singleKvStore_->Get(key, val1);
    singleKvStore_->Delete(key);
}

void GetEntriesFuzz1(const uint8_t *data, size_t size)
{
    std::string prefix(reinterpret_cast<const char *>(data), size);
    std::string keys = "test_";
    size_t sum = 10;
    std::vector<Entry> results;
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Put(prefix + keys + std::to_string(i), { keys + std::to_string(i) });
    }
    singleKvStore_->GetEntries(prefix, results);
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Delete(prefix + keys + std::to_string(i));
    }
}

void GetEntriesFuzz2(const uint8_t *data, size_t size)
{
    std::string prefix(reinterpret_cast<const char *>(data), size);
    DataQuery dataQuery;
    dataQuery.KeyPrefix(prefix);
    std::string keys = "test_";
    std::vector<Entry> entries;
    size_t sum = 10;
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Put(prefix + keys + std::to_string(i), keys + std::to_string(i));
    }
    singleKvStore_->GetEntries(dataQuery, entries);
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Delete(prefix + keys + std::to_string(i));
    }
}

void GetResultSetFuzz1(const uint8_t *data, size_t size)
{
    std::string prefix(reinterpret_cast<const char *>(data), size);
    std::string keys = "test_";
    int position = static_cast<int>(size);
    std::shared_ptr<KvStoreResultSet> resultSet;
    size_t sum = 10;
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Put(prefix + keys + std::to_string(i), keys + std::to_string(i));
    }
    singleKvStore_->GetResultSet(prefix, resultSet);
    resultSet->Move(position);
    resultSet->MoveToPosition(position);
    Entry entry;
    resultSet->GetEntry(entry);
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Delete(prefix + keys + std::to_string(i));
    }
}

void GetResultSetFuzz2(const uint8_t *data, size_t size)
{
    std::string prefix(reinterpret_cast<const char *>(data), size);
    DataQuery dataQuery;
    dataQuery.KeyPrefix(prefix);
    std::string keys = "test_";
    std::shared_ptr<KvStoreResultSet> resultSet;
    size_t sum = 10;
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Put(prefix + keys + std::to_string(i), keys + std::to_string(i));
    }
    singleKvStore_->GetResultSet(dataQuery, resultSet);
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Delete(prefix + keys + std::to_string(i));
    }
}

void GetCountFuzz1(const uint8_t *data, size_t size)
{
    int count;
    std::string prefix(reinterpret_cast<const char *>(data), size);
    DataQuery query;
    query.KeyPrefix(prefix);
    std::string keys = "test_";
    size_t sum = 10;
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Put(prefix + keys + std::to_string(i), keys + std::to_string(i));
    }
    singleKvStore_->GetCount(query, count);
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Delete(prefix + keys + std::to_string(i));
    }
}

void GetCountFuzz2(const uint8_t *data, size_t size)
{
    int count;
    size_t sum = 10;
    std::vector<std::string> keys;
    std::string prefix(reinterpret_cast<const char *>(data), size);
    for (size_t i = 0; i < sum; i++) {
        keys.push_back(prefix);
    }
    DataQuery query;
    query.InKeys(keys);
    std::string skey = "test_";
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Put(prefix + skey + std::to_string(i), skey + std::to_string(i));
    }
    singleKvStore_->GetCount(query, count);
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Delete(prefix + skey + std::to_string(i));
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
    OHOS::GetCountFuzz1(data, size);
    OHOS::GetCountFuzz2(data, size);
    OHOS::TearDown();
    /* Run your code on data */
    return 0;
}