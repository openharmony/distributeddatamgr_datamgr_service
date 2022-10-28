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

#include "schemaquery_fuzzer.h"
#include "distributed_kv_data_manager.h"
#include "data_query.h"
#include "sys/stat.h"

using namespace std;
using namespace OHOS;
using namespace DistributedKv;

namespace OHOS {
constexpr const char *VALID_SCHEMA_STRICT_DEFINE = "{\"SCHEMA_VERSION\":\"1.0\","
    "\"SCHEMA_MODE\":\"STRICT\",\"SCHEMA_DEFINE\":{"
    "\"name\":\"INTEGER, NOT NULL\"},\"SCHEMA_INDEXES\":[\"$.name\"}}";
static DistributedKvDataManager g_kvManager;

// Test 1: Open KvStore with fuzzed schema string.
void TestOpenSchemaStore001(const std::string &fuzzedString)
{
    std::shared_ptr<SingleKvStore> singleKvStorePtr;
    Options options = {.createIfMissing = true, .encrypt = true, .autoSync = true};
    options.schema = fuzzedString;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/schemaqueryfuzztest";
    AppId appId = {"schemaqueryfuzztest"};
    StoreId storeId = {"TestOpenSchemaStore001_storeId_fuzz"};
    g_kvManager.GetSingleKvStore(options, appId, storeId, singleKvStorePtr);
    g_kvManager.CloseAllKvStore(appId);
    g_kvManager.DeleteAllKvStore(appId);
}

void ExecuteQuery(SingleKvStore* singleKvStorePtr, const DataQuery &query)
{
    if (singleKvStorePtr == nullptr) {
        return;
    }
    std::vector<Entry> results1;
    singleKvStorePtr->GetEntries(query, results1);
    Key prefix1;
    std::vector<Entry> results2;
    singleKvStorePtr->GetEntries(prefix1, results2);
    std::shared_ptr<KvStoreResultSet> callback1;
    singleKvStorePtr->GetResultSet(query, callback1);
    singleKvStorePtr->CloseResultSet(callback1);
    Key prefix2;
    std::shared_ptr<KvStoreResultSet> callback2;
    singleKvStorePtr->GetResultSet(prefix2, callback2);
    singleKvStorePtr->CloseResultSet(callback2);
    int resultSize = 0;
    singleKvStorePtr->GetCount(query, resultSize);
}

// Test 2: Query EqualTo.
void TestQuerySchemaStore001(int fuzzedInt, int64_t fuzzedLong, double fuzzedDouble,
    bool fuzzedBoolean, std::string &fuzzedString)
{
    std::shared_ptr<SingleKvStore> singleKvStorePtr;
    Options options = {.createIfMissing = true, .encrypt = true, .autoSync = true};
    options.schema = VALID_SCHEMA_STRICT_DEFINE;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/schemaqueryfuzztest";
    AppId appId = {"schemaqueryfuzztest"};
    StoreId storeId = {"TestQuerySchemaStore001_storeId_fuzz"};
    g_kvManager.GetSingleKvStore(options, appId, storeId, singleKvStorePtr);
    DataQuery query;
    query.Reset();
    query.EqualTo(fuzzedString, fuzzedInt);
    query.And();
    query.EqualTo(fuzzedString, fuzzedLong);
    query.Or();
    query.EqualTo(fuzzedString, fuzzedDouble);
    query.And();
    query.EqualTo(fuzzedString, fuzzedBoolean);
    query.Or();
    query.EqualTo(fuzzedString, fuzzedString);
    ExecuteQuery(singleKvStorePtr.get(), query);
    g_kvManager.CloseAllKvStore(appId);
    g_kvManager.DeleteAllKvStore(appId);
}

// Test 3: Query NotEqualTo.
void TestQuerySchemaStore002(int fuzzedInt, int64_t fuzzedLong, double fuzzedDouble,
    bool fuzzedBoolean, std::string &fuzzedString)
{
    std::shared_ptr<SingleKvStore> singleKvStorePtr;
    Options options = {.createIfMissing = true, .encrypt = true, .autoSync = true};
    options.schema = VALID_SCHEMA_STRICT_DEFINE;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/schemaqueryfuzztest";
    AppId appId = {"schemaqueryfuzztest"};
    StoreId storeId = {"TestQuerySchemaStore002_storeId_fuzz"};
    g_kvManager.GetSingleKvStore(options, appId, storeId, singleKvStorePtr);
    DataQuery query;
    query.Reset();
    query.NotEqualTo(fuzzedString, fuzzedInt);
    query.And();
    query.NotEqualTo(fuzzedString, fuzzedLong);
    query.Or();
    query.NotEqualTo(fuzzedString, fuzzedDouble);
    query.And();
    query.NotEqualTo(fuzzedString, fuzzedBoolean);
    query.Or();
    query.NotEqualTo(fuzzedString, fuzzedString);
    ExecuteQuery(singleKvStorePtr.get(), query);
    g_kvManager.CloseAllKvStore(appId);
    g_kvManager.DeleteAllKvStore(appId);
}

// Test 4: Query GreaterThan.
void TestQuerySchemaStore003(int fuzzedInt, int64_t fuzzedLong, double fuzzedDouble, std::string &fuzzedString)
{
    std::shared_ptr<SingleKvStore> singleKvStorePtr;
    Options options = {.createIfMissing = true, .encrypt = true, .autoSync = true};
    options.schema = VALID_SCHEMA_STRICT_DEFINE;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/schemaqueryfuzztest";
    AppId appId = {"schemaqueryfuzztest"};
    StoreId storeId = {"TestQuerySchemaStore003_storeId_fuzz"};
    g_kvManager.GetSingleKvStore(options, appId, storeId, singleKvStorePtr);
    DataQuery query;
    query.Reset();
    query.GreaterThan(fuzzedString, fuzzedInt);
    query.And();
    query.GreaterThan(fuzzedString, fuzzedLong);
    query.Or();
    query.GreaterThan(fuzzedString, fuzzedDouble);
    query.And();
    query.GreaterThan(fuzzedString, fuzzedString);
    ExecuteQuery(singleKvStorePtr.get(), query);
    g_kvManager.CloseAllKvStore(appId);
    g_kvManager.DeleteAllKvStore(appId);
}

// Test 5: Query LessThan.
void TestQuerySchemaStore004(int fuzzedInt, int64_t fuzzedLong, double fuzzedDouble, std::string &fuzzedString)
{
    std::shared_ptr<SingleKvStore> singleKvStorePtr;
    Options options = {.createIfMissing = true, .encrypt = true, .autoSync = true};
    options.schema = VALID_SCHEMA_STRICT_DEFINE;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/schemaqueryfuzztest";
    AppId appId = {"schemaqueryfuzztest"};
    StoreId storeId = {"TestQuerySchemaStore004_storeId_fuzz"};
    g_kvManager.GetSingleKvStore(options, appId, storeId, singleKvStorePtr);
    DataQuery query;
    query.Reset();
    query.LessThan(fuzzedString, fuzzedInt);
    query.And();
    query.LessThan(fuzzedString, fuzzedLong);
    query.Or();
    query.LessThan(fuzzedString, fuzzedDouble);
    query.And();
    query.LessThan(fuzzedString, fuzzedString);
    ExecuteQuery(singleKvStorePtr.get(), query);
    g_kvManager.CloseAllKvStore(appId);
    g_kvManager.DeleteAllKvStore(appId);
}

// Test 6: Query GreaterThanOrEqualTo
void TestQuerySchemaStore005(int fuzzedInt, int64_t fuzzedLong, double fuzzedDouble, std::string &fuzzedString)
{
    std::shared_ptr<SingleKvStore> singleKvStorePtr;
    Options options = {.createIfMissing = true, .encrypt = true, .autoSync = true};
    options.schema = VALID_SCHEMA_STRICT_DEFINE;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/schemaqueryfuzztest";
    AppId appId = {"schemaqueryfuzztest"};
    StoreId storeId = {"TestQuerySchemaStore005_storeId_fuzz"};
    g_kvManager.GetSingleKvStore(options, appId, storeId, singleKvStorePtr);
    DataQuery query;
    query.Reset();
    query.GreaterThanOrEqualTo(fuzzedString, fuzzedInt);
    query.And();
    query.GreaterThanOrEqualTo(fuzzedString, fuzzedLong);
    query.Or();
    query.GreaterThanOrEqualTo(fuzzedString, fuzzedDouble);
    query.And();
    query.GreaterThanOrEqualTo(fuzzedString, fuzzedString);
    ExecuteQuery(singleKvStorePtr.get(), query);
    g_kvManager.CloseAllKvStore(appId);
    g_kvManager.DeleteAllKvStore(appId);
}

// Test 7: Query LessThanOrEqualTo.
void TestQuerySchemaStore006(int fuzzedInt, int64_t fuzzedLong, double fuzzedDouble, std::string &fuzzedString)
{
    std::shared_ptr<SingleKvStore> singleKvStorePtr;
    Options options = {.createIfMissing = true, .encrypt = true, .autoSync = true};
    options.schema = VALID_SCHEMA_STRICT_DEFINE;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/schemaqueryfuzztest";
    AppId appId = {"schemaqueryfuzztest"};
    StoreId storeId = {"TestQuerySchemaStore006_storeId_fuzz"};
    g_kvManager.GetSingleKvStore(options, appId, storeId, singleKvStorePtr);
    DataQuery query;
    query.Reset();
    query.LessThanOrEqualTo(fuzzedString, fuzzedInt);
    query.And();
    query.LessThanOrEqualTo(fuzzedString, fuzzedLong);
    query.Or();
    query.LessThanOrEqualTo(fuzzedString, fuzzedDouble);
    query.And();
    query.LessThanOrEqualTo(fuzzedString, fuzzedString);
    ExecuteQuery(singleKvStorePtr.get(), query);
    g_kvManager.CloseAllKvStore(appId);
    g_kvManager.DeleteAllKvStore(appId);
}

// Test 8: Query IsNull.
void TestQuerySchemaStore007(std::string &fuzzedString)
{
    std::shared_ptr<SingleKvStore> singleKvStorePtr;
    Options options = {.createIfMissing = true, .encrypt = true, .autoSync = true};
    options.schema = VALID_SCHEMA_STRICT_DEFINE;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/schemaqueryfuzztest";
    AppId appId = {"schemaqueryfuzztest"};
    StoreId storeId = {"TestQuerySchemaStore007_storeId_fuzz"};
    g_kvManager.GetSingleKvStore(options, appId, storeId, singleKvStorePtr);
    DataQuery query;
    query.Reset();
    query.IsNull(fuzzedString);
    ExecuteQuery(singleKvStorePtr.get(), query);
    g_kvManager.CloseAllKvStore(appId);
    g_kvManager.DeleteAllKvStore(appId);
}

// Test 9: Query Like Unlike OrderByAsc OrderByDesc Limit.
void TestQuerySchemaStore008(int fuzzedInt, std::string &fuzzedString)
{
    std::shared_ptr<SingleKvStore> singleKvStorePtr;
    Options options = {.createIfMissing = true, .encrypt = true, .autoSync = true};
    options.schema = VALID_SCHEMA_STRICT_DEFINE;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/schemaqueryfuzztest";
    AppId appId = {"schemaqueryfuzztest"};
    StoreId storeId = {"TestQuerySchemaStore008_storeId_fuzz"};
    g_kvManager.GetSingleKvStore(options, appId, storeId, singleKvStorePtr);
    DataQuery query;
    query.Reset();
    query.Like(fuzzedString, fuzzedString);
    query.And();
    query.Unlike(fuzzedString, fuzzedString);
    query.OrderByAsc(fuzzedString);
    query.OrderByDesc(fuzzedString);
    query.Limit(fuzzedInt, fuzzedInt);
    ExecuteQuery(singleKvStorePtr.get(), query);
    g_kvManager.CloseAllKvStore(appId);
    g_kvManager.DeleteAllKvStore(appId);
}

// Test 10: Batch.
void TestBatch001(std::string &fuzzedString)
{
    std::shared_ptr<SingleKvStore> singleKvStorePtr;
    Options options = {.createIfMissing = true, .encrypt = true, .autoSync = true};
    options.schema = VALID_SCHEMA_STRICT_DEFINE;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/schemaqueryfuzztest";
    AppId appId = {"schemaqueryfuzztest"};
    StoreId storeId = {"TestBatch001_storeId_fuzz"};
    g_kvManager.GetSingleKvStore(options, appId, storeId, singleKvStorePtr);
    if (singleKvStorePtr == nullptr) {
        return;
    }
    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = fuzzedString;
    entry1.value = fuzzedString;
    entry2.key = fuzzedString;
    entry2.value = fuzzedString;
    entry3.key = fuzzedString;
    entry3.value = fuzzedString;
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);
    std::vector<Key> keys;
    keys.push_back(fuzzedString);
    singleKvStorePtr->PutBatch(entries);
    singleKvStorePtr->DeleteBatch(keys);
    g_kvManager.CloseAllKvStore(appId);
    g_kvManager.DeleteAllKvStore(appId);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // odd or even 2
    bool fuzzedBoolean = ((size % 2) == 0);
    auto fuzzedInt = static_cast<int>(size);
    auto fuzzedLong = static_cast<int64_t>(size);
    auto fuzzedDouble = static_cast<double>(size);
    std::string fuzzedString(reinterpret_cast<const char *>(data), size);

    std::string storeDir = "/data/service/el1/public/database/schemaqueryfuzztest";
    mkdir(storeDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

    OHOS::TestOpenSchemaStore001(fuzzedString);
    OHOS::TestQuerySchemaStore001(fuzzedInt, fuzzedLong, fuzzedDouble, fuzzedBoolean, fuzzedString);
    OHOS::TestQuerySchemaStore002(fuzzedInt, fuzzedLong, fuzzedDouble, fuzzedBoolean, fuzzedString);
    OHOS::TestQuerySchemaStore003(fuzzedInt, fuzzedLong, fuzzedDouble, fuzzedString);
    OHOS::TestQuerySchemaStore004(fuzzedInt, fuzzedLong, fuzzedDouble, fuzzedString);
    OHOS::TestQuerySchemaStore005(fuzzedInt, fuzzedLong, fuzzedDouble, fuzzedString);
    OHOS::TestQuerySchemaStore006(fuzzedInt, fuzzedLong, fuzzedDouble, fuzzedString);
    OHOS::TestQuerySchemaStore007(fuzzedString);
    OHOS::TestQuerySchemaStore008(fuzzedInt, fuzzedString);
    OHOS::TestBatch001(fuzzedString);

    (void)remove("/data/service/el1/public/database/schemaqueryfuzztest/key");
    (void)remove("/data/service/el1/public/database/schemaqueryfuzztest/kvdb");
    (void)remove("/data/service/el1/public/database/schemaqueryfuzztest");
    return 0;
}