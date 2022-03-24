/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include <gtest/gtest.h>
#include <openssl/rand.h>

#include "db_common.h"
#include "db_errno.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "generic_single_ver_kv_entry.h"
#include "kvdb_manager.h"
#include "query_sync_object.h"
#include "sqlite_single_ver_continue_token.h"
#include "sqlite_single_ver_natural_store.h"
#include "sqlite_single_ver_natural_store_connection.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    DistributedDB::KvStoreConfig g_config;
    std::string g_testDir;
    DistributedDB::SQLiteSingleVerNaturalStore *g_store = nullptr;
    DistributedDB::SQLiteSingleVerNaturalStore *g_schemaStore = nullptr;
    DistributedDB::SQLiteSingleVerNaturalStoreConnection *g_connection = nullptr;
    DistributedDB::SQLiteSingleVerNaturalStoreConnection *g_schemaConnect = nullptr;

    KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
    // define the g_kvDelegateCallback, used to get some information when open a kv store.
    DBStatus g_kvDelegateStatus = INVALID_ARGS;
    KvStoreNbDelegate *g_kvNbDelegatePtr = nullptr;
    auto g_kvNbDelegateCallback = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(g_kvDelegateStatus), std::ref(g_kvNbDelegatePtr));

    const std::string REMOTE_DEVICE_ID = "remote_device_id";
    const Key PREFIX_KEY = { 'k' };
    const Key KEY1 = { 'k', '1' };
    const Key KEY2 = { 'k', '2' };
    const Key KEY3 = { 'k', '3' };
    const Value VALUE1 = { 'v', '1' };
    const Value VALUE2 = { 'v', '2' };
    const Value VALUE3 = { 'v', '3' };

    void ReleaseKvEntries(std::vector<SingleVerKvEntry *> &entries)
    {
        for (auto &itemEntry : entries) {
            delete itemEntry;
            itemEntry = nullptr;
        }
        entries.clear();
    }

    const string SCHEMA_STRING =
    "{\"SCHEMA_VERSION\":\"1.0\","
    "\"SCHEMA_MODE\":\"STRICT\","
    "\"SCHEMA_DEFINE\":{"
    "\"field_name1\":\"BOOL\","
    "\"field_name2\":\"BOOL\","
    "\"field_name3\":\"INTEGER, NOT NULL\","
    "\"field_name4\":\"LONG, DEFAULT 100\","
    "\"field_name5\":\"DOUBLE, NOT NULL, DEFAULT 3.14\","
    "\"field_name6\":\"STRING, NOT NULL, DEFAULT '3.1415'\","
    "\"field_name7\":\"LONG, DEFAULT 100\","
    "\"field_name8\":\"LONG, DEFAULT 100\","
    "\"field_name9\":\"LONG, DEFAULT 100\","
    "\"field_name10\":\"LONG, DEFAULT 100\""
    "},"
    "\"SCHEMA_INDEXES\":[\"$.field_name1\", \"$.field_name2\"]}";

    const std::string SCHEMA_VALUE1 =
    "{\"field_name1\":true,"
    "\"field_name2\":false,"
    "\"field_name3\":10,"
    "\"field_name4\":20,"
    "\"field_name5\":3.14,"
    "\"field_name6\":\"3.1415\","
    "\"field_name7\":100,"
    "\"field_name8\":100,"
    "\"field_name9\":100,"
    "\"field_name10\":100}";
}

class DistributedDBStorageQuerySyncTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBStorageQuerySyncTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGD("Test dir is %s", g_testDir.c_str());
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir + "/TestQuerySync/" + DBConstant::SINGLE_SUB_DIR);

    g_config.dataDir = g_testDir;
    g_mgr.SetKvStoreConfig(g_config);
    // Create schema database
    KvStoreNbDelegate::Option option = {true, false, false};
    option.schema = SCHEMA_STRING;
    g_mgr.GetKvStore("QuerySyncSchema", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    Value value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end());
    g_kvNbDelegatePtr->Put(KEY_1, value);

    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
}

void DistributedDBStorageQuerySyncTest::TearDownTestCase(void)
{
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
}

void DistributedDBStorageQuerySyncTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    KvDBProperties property;
    property.SetStringProp(KvDBProperties::DATA_DIR, g_testDir);
    property.SetStringProp(KvDBProperties::STORE_ID, "31");
    property.SetStringProp(KvDBProperties::IDENTIFIER_DIR, "TestQuerySync");
    property.SetBoolProp(KvDBProperties::MEMORY_MODE, false);
    property.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE);
    property.SetIntProp(KvDBProperties::CONFLICT_RESOLVE_POLICY, ConflictResolvePolicy::DEVICE_COLLABORATION);
    g_store = new (std::nothrow) SQLiteSingleVerNaturalStore;
    ASSERT_NE(g_store, nullptr);
    ASSERT_EQ(g_store->Open(property), E_OK);

    int erroCode = E_OK;
    g_connection = static_cast<SQLiteSingleVerNaturalStoreConnection *>(g_store->GetDBConnection(erroCode));
    ASSERT_NE(g_connection, nullptr);
    g_store->DecObjRef(g_store);
    EXPECT_EQ(erroCode, E_OK);

    std::string oriIdentifier = USER_ID + "-" + APP_ID + "-" + "QuerySyncSchema";
    std::string identifier = DBCommon::TransferHashString(oriIdentifier);
    property.SetStringProp(KvDBProperties::IDENTIFIER_DATA, identifier);
    std::string identifierHex = DBCommon::TransferStringToHex(identifier);
    property.SetStringProp(KvDBProperties::DATA_DIR, g_testDir);
    property.SetStringProp(KvDBProperties::STORE_ID, "QuerySyncSchema");
    property.SetStringProp(KvDBProperties::IDENTIFIER_DIR, identifierHex);

    g_schemaStore = new (std::nothrow) SQLiteSingleVerNaturalStore;
    ASSERT_NE(g_schemaStore, nullptr);
    ASSERT_EQ(g_schemaStore->Open(property), E_OK);
    g_schemaConnect = static_cast<SQLiteSingleVerNaturalStoreConnection *>(g_schemaStore->GetDBConnection(erroCode));
    ASSERT_NE(g_schemaConnect, nullptr);

    std::vector<Entry> entries;
    IOption option;
    option.dataType = IOption::SYNC_DATA;
    g_schemaConnect->GetEntries(option, Query::Select(), entries);
    ASSERT_FALSE(entries.empty());

    g_schemaStore->DecObjRef(g_schemaStore);
}

void DistributedDBStorageQuerySyncTest::TearDown(void)
{
    if (g_connection != nullptr) {
        g_connection->Close();
    }

    if (g_schemaConnect != nullptr) {
        g_schemaConnect->Close();
    }

    g_store = nullptr;
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir + "/TestQuerySync/" + DBConstant::SINGLE_SUB_DIR);
}

/**
  * @tc.name: GetSyncData001
  * @tc.desc: To test the function of querying the data in the time stamp range in the database.
  * @tc.type: FUNC
  * @tc.require: AR000CRAKO
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBStorageQuerySyncTest, GetQuerySyncData001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Obtain the data within the time stamp range
     *  through the GetSyncData(A, C) interface of the NaturalStore, where A<B<C.
     * @tc.expected: step1. GetSyncData The number of output parameter
     *  in the output parameter OK, dataItems is 1.
     */
    IOption option;
    option.dataType = IOption::SYNC_DATA;
    Key key;
    Value value;
    DistributedDBToolsUnitTest::GetRandomKeyValue(key);
    DistributedDBToolsUnitTest::GetRandomKeyValue(value);
    EXPECT_EQ(g_connection->Put(option, key, value), E_OK);

    Query query = Query::Select().PrefixKey(key);
    QueryObject queryObj(query);
    DataSizeSpecInfo  specInfo = {4 * 1024 * 1024, DBConstant::MAX_HPMODE_PACK_ITEM_SIZE};
    std::vector<SingleVerKvEntry *> entries;
    ContinueToken token = nullptr;
    EXPECT_EQ(g_store->GetSyncData(queryObj, SyncTimeRange{}, specInfo, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 1UL);
    ReleaseKvEntries(entries);

    Key keyOther = key;
    keyOther.push_back('1');
    g_store->ReleaseContinueToken(token);
    EXPECT_EQ(g_connection->Put(option, keyOther, value), E_OK);
    EXPECT_EQ(g_store->GetSyncData(queryObj, SyncTimeRange{}, specInfo, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 2UL);
    ReleaseKvEntries(entries);
    g_store->ReleaseContinueToken(token);
}

/**
 * @tc.name: GetQuerySyncData002
 * @tc.desc: To test GetSyncData function is available and check the boundary value.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBStorageQuerySyncTest, GetQuerySyncData002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Put k1 k2. k1's timestamp is 0 and k2's timestamp is INT64_MAX-1.
     * @tc.expected: step1. Put k1 k2 successfully.
     */
    DataItem data1{KEY1, VALUE1, 0, DataItem::LOCAL_FLAG, REMOTE_DEVICE_ID};
    DataItem data2{KEY2, VALUE2, INT64_MAX - 1, DataItem::LOCAL_FLAG, REMOTE_DEVICE_ID};
    EXPECT_EQ(DistributedDBToolsUnitTest::PutSyncDataTest(g_store, vector{data1, data2}, REMOTE_DEVICE_ID), E_OK);

    /**
     * @tc.steps: step2. Get k1 k2. SyncTimeRange is default(all time range).
     * @tc.expected: step2. Get k1 k2 successfully.
     */
    Query query = Query::Select().PrefixKey(PREFIX_KEY);
    QueryObject queryObj(query);
    DataSizeSpecInfo  specInfo = {4 * 1024 * 1024, DBConstant::MAX_HPMODE_PACK_ITEM_SIZE};
    std::vector<SingleVerKvEntry *> entries;
    ContinueToken token = nullptr;
    EXPECT_EQ(g_store->GetSyncData(queryObj, SyncTimeRange{}, specInfo, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 2UL);
    ReleaseKvEntries(entries);
    g_store->ReleaseContinueToken(token);

    /**
     * @tc.steps: step3. Put k3. k3's timestamp t3 is random.
     * @tc.expected: step3. Put k3.
     */
    auto time3 = static_cast<Timestamp>(DistributedDBToolsUnitTest::GetRandInt64(0, g_store->GetCurrentTimestamp()));
    DataItem data3{KEY3, VALUE3, time3, DataItem::LOCAL_FLAG, REMOTE_DEVICE_ID};
    EXPECT_EQ(DistributedDBToolsUnitTest::PutSyncDataTest(g_store, vector{data3}, REMOTE_DEVICE_ID), E_OK);

    /**
     * @tc.steps: step4. Get k3. SyncTimeRange is between t3 and t3 + 1.
     * @tc.expected: step4. Get k3 successfully.
     */
    EXPECT_EQ(g_store->GetSyncData(queryObj, SyncTimeRange{time3, 0, time3 + 1, 0},
        specInfo, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 1UL);
    ReleaseKvEntries(entries);
    g_store->ReleaseContinueToken(token);

    /**
     * @tc.steps: step5. Delete k1 k3.
     * @tc.expected: step5. Delete k1 k3 successfully.
     */
    IOption option{ IOption::SYNC_DATA };
    Timestamp deleteBeginTime = g_store->GetCurrentTimestamp();
    g_connection->DeleteBatch(option, vector{KEY1, KEY3});

    /**
     * @tc.steps: step6. Get deleted data.
     * @tc.expected: step6. Get k1 k3.
     */
    Timestamp deleteEndTime = g_store->GetCurrentTimestamp();
    EXPECT_EQ(g_store->GetSyncData(queryObj, SyncTimeRange{0, deleteBeginTime, 0, deleteEndTime}, specInfo, token,
        entries), E_OK);
    EXPECT_EQ(entries.size(), 2UL);
    ReleaseKvEntries(entries);
    g_store->ReleaseContinueToken(token);
}

/**
 * @tc.name: GetQuerySyncData004
 * @tc.desc: To test GetSyncDataNext function is available and check the boundary value.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBStorageQuerySyncTest, GetQuerySyncData004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Put k1 k2. k1's timestamp is 0 and k2's timestamp is INT64_MAX-1.
     * @tc.expected: step1. Put k1 k2 successfully.
     */
    DataItem data1{KEY1, VALUE1, 0, DataItem::LOCAL_FLAG, REMOTE_DEVICE_ID};
    DataItem data2{KEY2, VALUE2, INT64_MAX - 1, DataItem::LOCAL_FLAG, REMOTE_DEVICE_ID};
    EXPECT_EQ(DistributedDBToolsUnitTest::PutSyncDataTest(g_store, vector{data1, data2}, REMOTE_DEVICE_ID), E_OK);

    /**
     * @tc.steps: step2. Get k1 k2. SyncTimeRange is default(all time range).
     * @tc.expected: step2. Get k1 k2 successfully.
     */
    Query query = Query::Select().PrefixKey(PREFIX_KEY);
    QueryObject queryObj(query);
    DataSizeSpecInfo  specInfo = {4 * 1024 * 1024, DBConstant::MAX_HPMODE_PACK_ITEM_SIZE};
    std::vector<SingleVerKvEntry *> entries;
    ContinueToken token = new (std::nothrow) SQLiteSingleVerContinueToken{SyncTimeRange{}, queryObj};
    EXPECT_EQ(g_store->GetSyncDataNext(entries, token, specInfo), E_OK);
    EXPECT_EQ(entries.size(), 2UL);
    ReleaseKvEntries(entries);
    g_store->ReleaseContinueToken(token);

    /**
     * @tc.steps: step3. Put k3. k3's timestamp t3 is random.
     * @tc.expected: step3. Put k3.
     */
    auto time3 = static_cast<Timestamp>(DistributedDBToolsUnitTest::GetRandInt64(0, g_store->GetCurrentTimestamp()));
    DataItem data3{KEY3, VALUE3, time3, DataItem::LOCAL_FLAG, REMOTE_DEVICE_ID};
    EXPECT_EQ(DistributedDBToolsUnitTest::PutSyncDataTest(g_store, vector{data3}, REMOTE_DEVICE_ID), E_OK);

    /**
     * @tc.steps: step4. Get k3. SyncTimeRange is between t3 and t3 + 1.
     * @tc.expected: step4. Get k3 successfully.
     */
    token = new (std::nothrow) SQLiteSingleVerContinueToken{SyncTimeRange{time3, 0, time3 + 1}, queryObj};
    EXPECT_EQ(g_store->GetSyncDataNext(entries, token, specInfo), E_OK);
    EXPECT_EQ(entries.size(), 1UL);
    ReleaseKvEntries(entries);
    g_store->ReleaseContinueToken(token);

    /**
     * @tc.steps: step5. Delete k1 k3.
     * @tc.expected: step5. Delete k1 k3 successfully.
     */
    IOption option{ IOption::SYNC_DATA };
    Timestamp deleteBeginTime = g_store->GetCurrentTimestamp();
    g_connection->DeleteBatch(option, vector{KEY1, KEY3});

    /**
     * @tc.steps: step6. Get deleted data.
     * @tc.expected: step6. Get k1 k3.
     */
    Timestamp deleteEndTime = g_store->GetCurrentTimestamp();
    token = new (std::nothrow) SQLiteSingleVerContinueToken{SyncTimeRange{0, deleteBeginTime, 0, deleteEndTime},
        queryObj};
    EXPECT_EQ(g_store->GetSyncDataNext(entries, token, specInfo), E_OK);
    EXPECT_EQ(entries.size(), 2UL);
    ReleaseKvEntries(entries);
    g_store->ReleaseContinueToken(token);
}

/**
 * @tc.name: GetQuerySyncData006
 * @tc.desc: To test if parameter is invalid, GetSyncData function return an E_INVALID_ARGS code. If no data found,
    GetSyncData will return E_OK but entries will be empty.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBStorageQuerySyncTest, GetQuerySyncData006, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Get sync data when no data exists in DB.
     * @tc.expected: step1. GetSyncData return E_OK and entries is empty.
     */
    Query query = Query::Select().PrefixKey(PREFIX_KEY);
    QueryObject queryObj(query);
    DataSizeSpecInfo  specInfo = {4 * 1024 * 1024, DBConstant::MAX_HPMODE_PACK_ITEM_SIZE};
    std::vector<SingleVerKvEntry *> entries;
    ContinueToken token = nullptr;
    EXPECT_EQ(g_store->GetSyncData(queryObj, SyncTimeRange{}, specInfo, token, entries), E_OK);
    EXPECT_TRUE(entries.empty());
    ReleaseKvEntries(entries);
    g_store->ReleaseContinueToken(token);

    /**
     * @tc.steps: step2. Get sync data with invalid SyncTimeRange(beginTime is greater than endTime).
     * @tc.expected: step2. GetSyncData return E_INVALID_ARGS.
     */
    auto time = static_cast<Timestamp>(DistributedDBToolsUnitTest::GetRandInt64(0, INT64_MAX));
    EXPECT_EQ(g_store->GetSyncData(queryObj, SyncTimeRange{time, 0, 0}, specInfo, token, entries),
        -E_INVALID_ARGS);
}

/**
 * @tc.name: GetQuerySyncData006
 * @tc.desc: To test QUERY_SYNC_THRESHOLD works. When all query data is found in one get sync data operation,
    if the size of query data is greater than QUERY_SYNC_THRESHOLD*MAX_ITEM_SIZE , will not get deleted data next.
    Otherwise, will get deleted data next.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBStorageQuerySyncTest, GetQuerySyncData008, TestSize.Level1)
{
    const size_t maxItemSize = 200;
    const float querySyncThreshold = 0.50;

    Key key;
    Value value;
    string str;
    IOption option{ IOption::SYNC_DATA };

    /**
     * @tc.steps: step1. Put MAX_ITEM_SIZE / 2 + 1 entries from k0 to k100.
     * @tc.expected: step1. Put data successfully.
     */
    for (unsigned i = 0; i <= maxItemSize * querySyncThreshold; i++) {
        str = "k" + to_string(i);
        key = Key(str.begin(), str.end());
        str[0] = 'v';
        value = Value(str.begin(), str.end());
        EXPECT_EQ(g_connection->Put(option, key, value), E_OK);
    }

    DataItem item{key, value};
    auto oneBlockSize = SQLiteSingleVerStorageExecutor::GetDataItemSerialSize(item, Parcel::GetAppendedLen());

    /**
     * @tc.steps: step2. Delete k0.
     * @tc.expected: step2. Delete k0 successfully.
     */
    str = "k0";
    g_connection->Delete(option, Key(str.begin(), str.end()));

    /**
     * @tc.steps: step3. Get sync data when 100 query data and 1 deleted data exists in DB.
     * @tc.expected: step3. Get 100 query data and no deleted data.
     */
    Query query = Query::Select().PrefixKey(PREFIX_KEY);
    QueryObject queryObj(query);

    DataSizeSpecInfo  specInfo = {static_cast<uint32_t>((maxItemSize) * oneBlockSize), maxItemSize};
    std::vector<SingleVerKvEntry *> entries;
    ContinueToken token = nullptr;
    EXPECT_EQ(g_store->GetSyncData(queryObj, SyncTimeRange{}, specInfo, token, entries), -E_UNFINISHED);
    EXPECT_EQ(entries.size(), 100UL);
    ReleaseKvEntries(entries);
    g_store->ReleaseContinueToken(token);

    /**
     * @tc.steps: step4. Delete k1.
     * @tc.expected: step4. Delete k1 successfully.
     */
    str = "k1";
    g_connection->Delete(option, Key(str.begin(), str.end()));

    /**
     * @tc.steps: step5. Get sync data when 99 query data and 2 deleted data exists in DB.
     * @tc.expected: step5. Get 99 query data and 2 deleted data.
     */
    EXPECT_EQ(g_store->GetSyncData(queryObj, SyncTimeRange{}, specInfo, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 101UL);
    ReleaseKvEntries(entries);
    g_store->ReleaseContinueToken(token);
}

/**
 * @tc.name: GetQuerySyncData009
 * @tc.desc: To test GetSyncData and GetSyncDataNext function works with large amounts of data.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBStorageQuerySyncTest, GetQuerySyncData009, TestSize.Level2)
{
    /**
     * @tc.steps: step1. Put 500 entries from k0 to k499.
     * @tc.expected: step1. Put data successfully.
     */
    Key key;
    Value value;
    string str;
    IOption option{ IOption::SYNC_DATA };
    const uint64_t totalSize = 500; // 500 data in DB.
    for (unsigned i = 0; i < totalSize; i++) {
        str = "k" + to_string(i);
        key = Key(str.begin(), str.end());
        str[0] = 'v';
        value = Value(str.begin(), str.end());
        EXPECT_EQ(g_connection->Put(option, key, value), E_OK);
    }

    /**
     * @tc.steps: step2. Delete 150 entries from k150 to k299.
     * @tc.expected: step2. Delete data successfully.
     */
    for (unsigned i = 150; i < 300; i++) {
        str = "k" + to_string(i);
        g_connection->Delete(option, Key(str.begin(), str.end()));
    }

    /**
     * @tc.steps: step3. Get all sync data;
     * @tc.expected: step3. Get 500 data.
     */
    Query query = Query::Select().PrefixKey(PREFIX_KEY);
    QueryObject queryObj(query);

    uint64_t getSize = 0;
    std::vector<SingleVerKvEntry *> entries;
    const size_t maxItemSize = 100; // Get 100 data at most in one GetSyncData operation.
    DataSizeSpecInfo  specInfo = {MTU_SIZE, maxItemSize};
    ContinueToken token = nullptr;
    g_store->GetSyncData(queryObj, SyncTimeRange{}, specInfo, token, entries);
    getSize += entries.size();
    ReleaseKvEntries(entries);

    while (token != nullptr) {
        g_store->GetSyncDataNext(entries, token, specInfo);
        getSize += entries.size();
        ReleaseKvEntries(entries);
    }

    EXPECT_EQ(getSize, totalSize);
}

/**
 * @tc.name: GetQuerySyncData010
 * @tc.desc: To test GetSyncData when Query with limit.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBStorageQuerySyncTest, GetQuerySyncData010, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Put 100 entries from k100 to k1.
     * @tc.expected: step1. Put data successfully.
     */
    Key key;
    Value value;
    string str;
    IOption option{ IOption::SYNC_DATA };
    const uint64_t totalSize = 100; // 100 data in DB.
    for (unsigned i = totalSize; i > 0; i--) {
        str = "k" + to_string(i);
        key = Key(str.begin(), str.end());
        str[0] = 'v';
        value = Value(str.begin(), str.end());
        EXPECT_EQ(g_connection->Put(option, key, value), E_OK);
    }

    /**
     * @tc.steps: step3. Get half of sync data;
     * @tc.expected: step3. Get half of sync data successfully.
     */
    Query query = Query::Select().PrefixKey(PREFIX_KEY).Limit(totalSize / 2);
    QueryObject queryObj(query);

    uint64_t getSize = 0;
    std::vector<SingleVerKvEntry *> entries;
    const size_t maxItemSize = 10; // Get 10 data at most in one GetSyncData operation.
    DataSizeSpecInfo  specInfo = {MTU_SIZE, maxItemSize};
    ContinueToken token = nullptr;
    g_store->GetSyncData(queryObj, SyncTimeRange{}, specInfo, token, entries);
    getSize += entries.size();
    ReleaseKvEntries(entries);

    while (token != nullptr) {
        g_store->GetSyncDataNext(entries, token, specInfo);
        getSize += entries.size();
        ReleaseKvEntries(entries);
    }

    EXPECT_EQ(getSize, totalSize / 2);
}

/**
  * @tc.name: GetQueryID001
  * @tc.desc: To test the function of generating query identity.
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBStorageQuerySyncTest, GetQueryID001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Get illegal query object, get this object identify
     * @tc.expected: step1. GetIdentify will get empty string
     */
    Query errQuery = Query::Select().GreaterThan("$.test", true);
    QuerySyncObject querySync(errQuery);
    EXPECT_EQ(querySync.GetIdentify().empty(), true);

    /**
     * @tc.steps:step2. use illegal query object to serialized
     * @tc.expected: step2. SerializeData will not return E_OK
     */
    vector<uint8_t> buffer(querySync.CalculateParcelLen(SOFTWARE_VERSION_CURRENT), 0);
    Parcel writeParcel(buffer.data(), querySync.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    Parcel readParcel(buffer.data(), querySync.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    EXPECT_NE(querySync.SerializeData(writeParcel, SOFTWARE_VERSION_CURRENT), E_OK);
}

/**
  * @tc.name: GetQueryID002
  * @tc.desc: To test the function of generating query identity.
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBStorageQuerySyncTest, GetQueryID002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Get empty condition query object, get this object identify
     * @tc.expected: step1. GetIdentify result not change
     */
    Query query1 = Query::Select();

    QuerySyncObject querySync(query1);
    EXPECT_EQ(querySync.GetIdentify().empty(), false);
    // same object identify is same
    EXPECT_EQ(querySync.GetIdentify(), querySync.GetIdentify());

    IOption option;
    option.dataType = IOption::SYNC_DATA;
    Key key;
    Value value;
    DistributedDBToolsUnitTest::GetRandomKeyValue(key);
    DistributedDBToolsUnitTest::GetRandomKeyValue(value);
    EXPECT_EQ(g_connection->Put(option, key, value), E_OK);
    EXPECT_EQ(g_connection->Put(option, KEY_1, VALUE_1), E_OK);
    EXPECT_EQ(g_connection->Put(option, KEY_2, VALUE_2), E_OK);

    /**
     * @tc.steps:step2. Get prefix key condition query object, get this object identify
     * @tc.expected: step2. GetIdentify result not same as other condition query object
     */
    Query query2 = Query::Select().PrefixKey(key);
    QuerySyncObject querySync1(query2);
    EXPECT_EQ(querySync1.GetIdentify().empty(), false);
    // same object identify is not same
    EXPECT_NE(querySync.GetIdentify(), querySync1.GetIdentify());

    /**
     * @tc.steps:step3. empty condition query object can serialized and deserialized normally
     * @tc.expected: step3. after deserialized, can get all key value in database
     */
    vector<uint8_t> buffer(querySync.CalculateParcelLen(SOFTWARE_VERSION_CURRENT), 0);
    Parcel writeParcel(buffer.data(), querySync.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    Parcel readParcel(buffer.data(), querySync.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    EXPECT_EQ(querySync.SerializeData(writeParcel, SOFTWARE_VERSION_CURRENT), E_OK);

    QuerySyncObject queryObj2;
    EXPECT_EQ(QuerySyncObject::DeSerializeData(readParcel, queryObj2), E_OK);
    LOGD("Query obj after serialize!");

    DataSizeSpecInfo  specInfo = {4 * 1024 * 1024, DBConstant::MAX_HPMODE_PACK_ITEM_SIZE};
    std::vector<SingleVerKvEntry *> entries;
    ContinueToken token = nullptr;
    EXPECT_EQ(g_store->GetSyncData(queryObj2, SyncTimeRange{}, specInfo, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 3UL);
    SingleVerKvEntry::Release(entries);
}

/**
  * @tc.name: GetQueryID003
  * @tc.desc: To test the function of generating query identity ignore limit, orderby, suggestion.
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBStorageQuerySyncTest, GetQueryID003, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Get empty condition query object, get this object identify
     */
    Query query1 = Query::Select().PrefixKey({});
    QuerySyncObject querySync1(query1);
    std::string id1 = querySync1.GetIdentify();
    EXPECT_EQ(id1.empty(), false);

    /**
     * @tc.steps:step2. Get limit condition query object, get this object identify
     * @tc.expected: step2. GetIdentify result same as no contain limit condition
     */
    Query query2 = query1.Limit(1, 1);
    QuerySyncObject querySync2(query2);
    std::string id2 = querySync2.GetIdentify();
    EXPECT_EQ(id2, id1);

    /**
     * @tc.steps:step3. Get orderby condition query object, get this object identify
     * @tc.expected: step3. GetIdentify result same as no contain orderby condition
     */
    Query query3 = query2.OrderBy("$.test");
    QuerySyncObject querySync3(query3);
    std::string id3 = querySync3.GetIdentify();
    EXPECT_EQ(id2, id3);
}

/**
  * @tc.name: Serialize001
  * @tc.desc: To test the function of querying the data after serialized and deserialized.
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBStorageQuerySyncTest, Serialize001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Put K1V1 K2V2 and rand KV for query
     */
    IOption option;
    option.dataType = IOption::SYNC_DATA;
    Key key;
    Value value;
    DistributedDBToolsUnitTest::GetRandomKeyValue(key);
    DistributedDBToolsUnitTest::GetRandomKeyValue(value);
    EXPECT_EQ(g_connection->Put(option, key, value), E_OK);
    EXPECT_EQ(g_connection->Put(option, KEY_1, VALUE_1), E_OK);
    EXPECT_EQ(g_connection->Put(option, KEY_2, VALUE_2), E_OK);

    /**
     * @tc.steps:step2. Put K1V1 K2V2 and rand KV for query
     * @tc.expected: step2. GetIdentify result same as no contain limit condition
     */
    Query query = Query::Select().PrefixKey(key);
    QueryObject queryObj(query);
    DataSizeSpecInfo  specInfo = {4 * 1024 * 1024, DBConstant::MAX_HPMODE_PACK_ITEM_SIZE};
    std::vector<SingleVerKvEntry *> entries;
    ContinueToken token = nullptr;
    LOGD("Ori query obj!");
    EXPECT_EQ(g_store->GetSyncData(queryObj, SyncTimeRange{}, specInfo, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 1UL);
    SingleVerKvEntry::Release(entries);

    /**
     * @tc.steps:step3. query result after serialized and deserialized
     * @tc.expected: step3. Get same result
     */
    QuerySyncObject querySync(query);
    vector<uint8_t> buffer(querySync.CalculateParcelLen(SOFTWARE_VERSION_CURRENT), 0);
    Parcel writeParcel(buffer.data(), querySync.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    Parcel readParcel(buffer.data(), querySync.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    EXPECT_EQ(querySync.SerializeData(writeParcel, SOFTWARE_VERSION_CURRENT), E_OK);

    QuerySyncObject queryObj1;
    EXPECT_EQ(QuerySyncObject::DeSerializeData(readParcel, queryObj1), E_OK);

    LOGD("Query obj after serialize!");
    EXPECT_EQ(g_store->GetSyncData(queryObj1, SyncTimeRange{}, specInfo, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 1UL);
    SingleVerKvEntry::Release(entries);

    std::string id = querySync.GetIdentify().c_str();
    EXPECT_EQ(id.size(), 64u);
    LOGD("query identify [%s] [%zu]", id.c_str(), id.size());
}

/**
  * @tc.name: Serialize002
  * @tc.desc: To test the function of serialized illegal query object.
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBStorageQuerySyncTest, Serialize002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Serialized illegal query object
     * @tc.expected: step1. return not E_OK
     */
    Query query = Query::Select().PrefixKey({}).GreaterThan("$.test", true); // bool can not compare
    QuerySyncObject querySync(query);
    vector<uint8_t> buffer(querySync.CalculateParcelLen(SOFTWARE_VERSION_CURRENT), 0);
    Parcel writeParcel(buffer.data(), querySync.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    EXPECT_NE(querySync.SerializeData(writeParcel, SOFTWARE_VERSION_CURRENT), E_OK);
}

/**
  * @tc.name: DeSerialize001
  * @tc.desc: To test the function of deserialized  illegal query object.
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBStorageQuerySyncTest, DeSerialize001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. deserialized empty content query object
     * @tc.expected: step1. return not E_OK
     */
    QuerySyncObject querySync;
    vector<uint8_t> buffer(querySync.CalculateParcelLen(SOFTWARE_VERSION_CURRENT), 0);
    Parcel readParcel(buffer.data(), querySync.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));

    QuerySyncObject queryObj;
    EXPECT_NE(QuerySyncObject::DeSerializeData(readParcel, queryObj), E_OK);

    /**
     * @tc.steps:step2. deserialized empty parcel
     * @tc.expected: step2. return not E_OK
     */
    buffer.resize(0);
    Parcel readParcel1(buffer.data(), 0);
    EXPECT_NE(QuerySyncObject::DeSerializeData(readParcel1, queryObj), E_OK);

    /**
     * @tc.steps:step3. deserialized error size parcel
     * @tc.expected: step3. return not E_OK
     */
    uint8_t simSize = 0;
    RAND_bytes(&simSize, 1);
    buffer.resize(simSize);
    Parcel readParcel2(buffer.data(), simSize);
    EXPECT_NE(QuerySyncObject::DeSerializeData(readParcel2, queryObj), E_OK);
}

/**
  * @tc.name: SameQueryObjectIdInDiffVer001
  * @tc.desc: Same query object have same id in different version.
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBStorageQuerySyncTest, SameQueryObjectIdInDiffVer001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Record the fixed id of the query object of the current version,
     * and keep it unchanged in subsequent versions
     * @tc.expected: step1. Never change in diff version
     */
    Query query1 = Query::Select().PrefixKey({});
    QuerySyncObject querySync1(query1);
    EXPECT_EQ(querySync1.GetIdentify(), "A9AB721457C4CA98726EECC7CB16F94E31B9752BEE6D08569CFE797B4A64A304");

    Query query2 = Query::Select();
    QuerySyncObject querySync2(query2);
    EXPECT_EQ(querySync2.GetIdentify(), "AF5570F5A1810B7AF78CAF4BC70A660F0DF51E42BAF91D4DE5B2328DE0E83DFC");

    Query query3 = Query::Select().NotLike("$.test", "testValue");
    QuerySyncObject querySync3(query3);
    EXPECT_EQ(querySync3.GetIdentify(), "F2BAC2B53FE81F9928E5F8DCDF502F2419E8CEB5DFC157EEBDDB955A66C0148B");

    vector<int> fieldValues{1, 1, 1};
    Query query4 = Query::Select().In("$.test", fieldValues);
    QuerySyncObject querySync4(query4);
    EXPECT_EQ(querySync4.GetIdentify(), "EEAECCD0E1A7217574ED3092C8DAA39469388FA1B8B7B210185B4257B785FE4D");

    Query query5 = Query::Select().OrderBy("$.test.test_child", false);
    QuerySyncObject querySync5(query5);
    EXPECT_EQ(querySync5.GetIdentify(), "AF5570F5A1810B7AF78CAF4BC70A660F0DF51E42BAF91D4DE5B2328DE0E83DFC");

    Query query6 = Query::Select().Limit(1, 2);
    QuerySyncObject querySync6(query6);
    EXPECT_EQ(querySync6.GetIdentify(), "AF5570F5A1810B7AF78CAF4BC70A660F0DF51E42BAF91D4DE5B2328DE0E83DFC");

    Query query7 = Query::Select().IsNull("$.test.test_child");
    QuerySyncObject querySync7(query7);
    EXPECT_EQ(querySync7.GetIdentify(), "762AB5FDF9B1433D6F398269D4DDD6DE6444953F515E87C6796654180A7FF422");

    Query query8 = Query::Select().EqualTo("$.test.test_child", true).And().GreaterThan("$.test.test_child", 1);
    QuerySyncObject querySync8(query8);
    EXPECT_EQ(querySync8.GetIdentify(), "B97FBFFBC690DAF25031FD4EE8ADC92F4698B9E81FD4877CD54EDEA122F6A6E0");

    Query query9 = Query::Select().GreaterThan("$.test", 1).OrderBy("$.test");
    QuerySyncObject querySync9(query9);
    EXPECT_EQ(querySync9.GetIdentify(), "77480E3EE04EB1500BB2F1A31704EE5676DC81F088A7A300F6D30E3FABA7D0A3");

    Query query = Query::Select().GreaterThan("$.test1", 1).OrderBy("$.test1");
    QuerySyncObject querySync(query);
    EXPECT_EQ(querySync.GetIdentify(), "170F5137C0BB49011D7415F706BD96B86F5FAFADA356374981362B1E177263B9");
}

/**
  * @tc.name: querySyncByField
  * @tc.desc: Test for illegal query conditions, use GetSyncData
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBStorageQuerySyncTest, querySyncByField, TestSize.Level1)
{
    Query queryInvalidField = Query::Select().EqualTo("$.field_name11", 1);
    Query queryInvalidCombine = Query::Select().EqualTo("$.field_name3", 1).BeginGroup();
    Query queryAll = Query::Select();
    Query queryPrefixKeyLimit = Query::Select().PrefixKey({}).Limit(1, 0);

    DataSizeSpecInfo  specInfo = {4 * 1024 * 1024, DBConstant::MAX_HPMODE_PACK_ITEM_SIZE};
    std::vector<SingleVerKvEntry *> entries;
    ContinueToken token = nullptr;

    QueryObject queryObj(queryInvalidCombine);
    EXPECT_EQ(g_schemaStore->GetSyncData(queryObj, SyncTimeRange{}, specInfo, token, entries), -E_INVALID_QUERY_FORMAT);

    QueryObject queryObj2(queryAll);
    EXPECT_EQ(g_schemaStore->GetSyncData(queryObj2, SyncTimeRange{}, specInfo, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 1UL);
    ReleaseKvEntries(entries);

    QueryObject queryObj1(queryInvalidField);
    EXPECT_EQ(g_schemaStore->GetSyncData(queryObj1, SyncTimeRange{}, specInfo, token, entries), -E_INVALID_QUERY_FIELD);

    QueryObject queryObj3(queryPrefixKeyLimit);
    EXPECT_EQ(g_schemaStore->GetSyncData(queryObj3, SyncTimeRange{}, specInfo, token, entries), E_OK);
    ReleaseKvEntries(entries);
}

/**
  * @tc.name: IsQueryOnlyByKey
  * @tc.desc: The test can correctly determine whether the value is used for query
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBStorageQuerySyncTest, IsQueryOnlyByKey, TestSize.Level1)
{
    Query queryAll = Query::Select();
    Query queryPrefixKeyLimit = Query::Select().PrefixKey({}).Limit(1, 0);
    Query queryPrefix = Query::Select().PrefixKey({});
    Query queryPrefixKeyLimitIndex = Query::Select().PrefixKey({}).Limit(1, 0).SuggestIndex("$.field_name3");
    Query queryPrefixKeyLimitEQ = Query::Select().PrefixKey({}).Limit(1, 0).EqualTo("$.field_name3", 1);
    Query queryEQ = Query::Select().EqualTo("$.field_name3", 1);
    Query queryLimitEQ = Query::Select().Limit(1, 0).EqualTo("$.field_name3", 1);

    QueryObject queryObj(queryAll);
    EXPECT_TRUE(queryObj.IsQueryOnlyByKey());

    QueryObject queryObj1(queryPrefixKeyLimit);
    EXPECT_TRUE(queryObj1.IsQueryOnlyByKey());

    QueryObject queryObj2(queryPrefix);
    EXPECT_TRUE(queryObj2.IsQueryOnlyByKey());

    QueryObject queryObj3(queryPrefixKeyLimitIndex);
    EXPECT_FALSE(queryObj3.IsQueryOnlyByKey());

    QueryObject queryObj4(queryPrefixKeyLimitEQ);
    EXPECT_FALSE(queryObj4.IsQueryOnlyByKey());

    QueryObject queryObj5(queryEQ);
    EXPECT_FALSE(queryObj5.IsQueryOnlyByKey());

    QueryObject queryObj6(queryLimitEQ);
    EXPECT_FALSE(queryObj6.IsQueryOnlyByKey());
}

/**
  * @tc.name: MultiQueryParcel
  * @tc.desc: Mix multiple conditions for simultaneous query can be serialize
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBStorageQuerySyncTest, MultiQueryParcel, TestSize.Level1)
{
    Query queryInvalidField = Query::Select().LessThan("$.field_name1", 1);
    Query queryInvalidCombine = Query::Select().EqualTo("$.field_name3", 1).BeginGroup();
    Query queryPrefixKeyLimit = Query::Select().PrefixKey({}).Limit(1, 0);

    DataSizeSpecInfo  specInfo = {4 * 1024 * 1024, DBConstant::MAX_HPMODE_PACK_ITEM_SIZE};
    std::vector<SingleVerKvEntry *> entries;
    ContinueToken token = nullptr;

    QuerySyncObject querySyncObj(queryInvalidField);
    vector<uint8_t> buffer(querySyncObj.CalculateParcelLen(SOFTWARE_VERSION_CURRENT), 0);
    Parcel writeParcel(buffer.data(), querySyncObj.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    Parcel readParcel(buffer.data(), querySyncObj.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    EXPECT_EQ(querySyncObj.SerializeData(writeParcel, SOFTWARE_VERSION_CURRENT), E_OK);
    QuerySyncObject queryObjAfterSer;
    EXPECT_EQ(QuerySyncObject::DeSerializeData(readParcel, queryObjAfterSer), E_OK);
    EXPECT_EQ(g_schemaStore->GetSyncData(queryObjAfterSer, SyncTimeRange{}, specInfo, token, entries),
        -E_INVALID_QUERY_FORMAT);

    QuerySyncObject querySyncObj1(queryInvalidCombine);
    vector<uint8_t> buffer1(querySyncObj1.CalculateParcelLen(SOFTWARE_VERSION_CURRENT), 0);
    Parcel writeParcel1(buffer1.data(), querySyncObj1.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    Parcel readParcel1(buffer1.data(), querySyncObj1.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    EXPECT_EQ(querySyncObj1.SerializeData(writeParcel1, SOFTWARE_VERSION_CURRENT), E_OK);
    QuerySyncObject queryObjAfterSer1;
    EXPECT_EQ(QuerySyncObject::DeSerializeData(readParcel1, queryObjAfterSer1), E_OK);
    EXPECT_EQ(g_schemaStore->GetSyncData(queryObjAfterSer1, SyncTimeRange{}, specInfo, token, entries),
        -E_INVALID_QUERY_FORMAT);

    QuerySyncObject querySyncObj2(queryPrefixKeyLimit);
    vector<uint8_t> buffer2(querySyncObj2.CalculateParcelLen(SOFTWARE_VERSION_CURRENT), 0);
    Parcel writeParcel2(buffer2.data(), querySyncObj2.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    Parcel readParcel2(buffer2.data(), querySyncObj2.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    EXPECT_EQ(querySyncObj2.SerializeData(writeParcel2, SOFTWARE_VERSION_CURRENT), E_OK);
    QuerySyncObject queryObjAfterSer2;
    EXPECT_EQ(QuerySyncObject::DeSerializeData(readParcel2, queryObjAfterSer2), E_OK);
    EXPECT_EQ(g_schemaStore->GetSyncData(queryObjAfterSer2, SyncTimeRange{}, specInfo, token, entries),
        E_OK);
    EXPECT_FALSE(entries.empty());
    ReleaseKvEntries(entries);
}


/**
  * @tc.name: QueryParcel001
  * @tc.desc: Query object should has same attribute(Limit, OrderBy) after deserialized
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBStorageQuerySyncTest, QueryParcel001, TestSize.Level1)
{
    Query query = Query::Select().OrderBy("$.field_name1").Limit(10, 5);

    QuerySyncObject querySyncObj(query);
    vector<uint8_t> buffer(querySyncObj.CalculateParcelLen(SOFTWARE_VERSION_CURRENT), 0);
    Parcel writeParcel(buffer.data(), querySyncObj.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    Parcel readParcel(buffer.data(), querySyncObj.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
    EXPECT_EQ(querySyncObj.SerializeData(writeParcel, SOFTWARE_VERSION_CURRENT), E_OK);
    QuerySyncObject queryObjAfterSer;
    EXPECT_EQ(QuerySyncObject::DeSerializeData(readParcel, queryObjAfterSer), E_OK);
    EXPECT_EQ(queryObjAfterSer.HasLimit(), true);
    int limit = 0;
    int offset = 0;
    queryObjAfterSer.GetLimitVal(limit, offset);
    EXPECT_EQ(limit, 10);
    EXPECT_EQ(offset, 5);
    EXPECT_EQ(queryObjAfterSer.HasOrderBy(), true);
}

/**
  * @tc.name: MultiQueryGetSyncData001
  * @tc.desc: Mix multiple conditions for simultaneous query
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBStorageQuerySyncTest, MultiQueryGetSyncData001, TestSize.Level1)
{
    Query query = Query::Select();
    Query query1 = Query::Select().EqualTo("$.field_name1", true);
    Query query2 = Query::Select().BeginGroup().GreaterThan("$.field_name3", 1).EndGroup();
    Query query3 = Query::Select().Like("field_name7", "");
    Query query4 = Query::Select().PrefixKey({}).OrderBy("$.field_name6");
    Query query5 = Query::Select().PrefixKey({}).IsNull("field_name10");

    DataSizeSpecInfo  specInfo = {4 * 1024 * 1024, DBConstant::MAX_HPMODE_PACK_ITEM_SIZE};
    std::vector<SingleVerKvEntry *> entries;
    ContinueToken token = nullptr;

    QueryObject queryObj(query);
    EXPECT_EQ(g_schemaStore->GetSyncData(queryObj, SyncTimeRange{}, specInfo, token, entries), E_OK);
    ReleaseKvEntries(entries);

    QueryObject queryObj1(query1);
    EXPECT_EQ(g_schemaStore->GetSyncData(queryObj1, SyncTimeRange{}, specInfo, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 1UL);
    ReleaseKvEntries(entries);

    QueryObject queryObj2(query2);
    EXPECT_EQ(g_schemaStore->GetSyncData(queryObj2, SyncTimeRange{}, specInfo, token, entries), E_OK);
    ReleaseKvEntries(entries);

    QueryObject queryObj3(query3);
    EXPECT_EQ(g_schemaStore->GetSyncData(queryObj3, SyncTimeRange{}, specInfo, token, entries), E_OK);
    ReleaseKvEntries(entries);

    QueryObject queryObj4(query4);
    EXPECT_EQ(g_schemaStore->GetSyncData(queryObj4, SyncTimeRange{}, specInfo, token, entries), E_OK);
    ReleaseKvEntries(entries);

    QueryObject queryObj5(query5);
    EXPECT_EQ(g_schemaStore->GetSyncData(queryObj5, SyncTimeRange{}, specInfo, token, entries), E_OK);
    ReleaseKvEntries(entries);
}

/**
  * @tc.name: QueryPredicateValidation001
  * @tc.desc: check query object is query only by key and has orderBy or not
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBStorageQuerySyncTest, QueryPredicateValidation001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Create a query object with prefixKey only, check it's predicate
     * @tc.expected: step1. check IsQueryOnlyByKey true; check HasOrderBy false
     */
    Query query1 = Query::Select().PrefixKey({});
    QuerySyncObject querySync1(query1);
    EXPECT_EQ(querySync1.IsQueryOnlyByKey(), true);
    EXPECT_EQ(querySync1.HasOrderBy(), false);

    /**
     * @tc.steps:step2. Create a query object with prefixKey and equalTo, check it's predicate
     * @tc.expected: step2. check IsQueryOnlyByKey false; check HasOrderBy false
     */
    Query query2 = Query::Select().PrefixKey({}).EqualTo("$.testField", 0);
    QuerySyncObject querySync2(query2);
    EXPECT_EQ(querySync2.IsQueryOnlyByKey(), false);
    EXPECT_EQ(querySync2.HasOrderBy(), false);

    /**
     * @tc.steps:step3. Create a query object with orderBy only, check it's predicate
     * @tc.expected: step3. check IsQueryOnlyByKey false; check HasOrderBy true
     */
    Query query3 = Query::Select().OrderBy("$.testField");
    QuerySyncObject querySync3(query3);
    EXPECT_EQ(querySync3.IsQueryOnlyByKey(), false);
    EXPECT_EQ(querySync3.HasOrderBy(), true);
}

/**
  * @tc.name: RelationalQuerySyncTest001
  * @tc.desc: Test querySyncObject serialize with table name is specified
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBStorageQuerySyncTest, RelationalQuerySyncTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Create a query object with table name is specified
     * @tc.expected: ok
     */
    Query query1 = Query::Select("Relational_table").EqualTo("field1", "abc");
    QuerySyncObject obj1(query1);

    /**
     * @tc.steps:step2. Serialize the object
     * @tc.expected: ok
     */
    uint32_t buffLen = obj1.CalculateParcelLen(SOFTWARE_VERSION_CURRENT);
    vector<uint8_t> buffer(buffLen, 0);
    Parcel writeParcel(buffer.data(), buffLen);
    EXPECT_EQ(obj1.SerializeData(writeParcel, SOFTWARE_VERSION_CURRENT), E_OK);

    /**
     * @tc.steps:step3. DeSerialize the data
     * @tc.expected: ok, And the queryId is same
     */
    Parcel readParcel(buffer.data(), buffLen);
    QuerySyncObject queryObj2;
    EXPECT_EQ(QuerySyncObject::DeSerializeData(readParcel, queryObj2), E_OK);
    EXPECT_EQ(obj1.GetIdentify(), queryObj2.GetIdentify());
}

/**
  * @tc.name: RelationalQuerySyncTest002
  * @tc.desc: Test querySyncObject with different table name has different identity
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBStorageQuerySyncTest, RelationalQuerySyncTest002, TestSize.Level1)
{
    Query query1 = Query::Select("Relational_table1").EqualTo("field1", "abc");
    QuerySyncObject obj1(query1);

    Query query2 = Query::Select("Relational_table2").EqualTo("field1", "abc");
    QuerySyncObject obj2(query2);

    /**
     * @tc.steps:step1. check object identity
     * @tc.expected: identity should be different.
     */
    EXPECT_NE(obj1.GetIdentify(), obj2.GetIdentify());
}

/**
 * @tc.name: SerializeAndDeserializeForVer1
 * @tc.desc: Test querySyncObject serialization and deserialization.
 * @tc.type: FUNC
 * @tc.require: AR000GOHO7
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBStorageQuerySyncTest, SerializeAndDeserializeForVer1, TestSize.Level1)
{
    Query qeury1 = Query::Select("table1").EqualTo("field1", "abc").InKeys({KEY_1, KEY_2, KEY_3});
    QuerySyncObject obj1(qeury1);

    /**
     * @tc.steps:step1. Serialize obj1.
     * @tc.expected: Serialize successfully.
     */
    auto len = obj1.CalculateParcelLen(SOFTWARE_VERSION_CURRENT);
    std::vector<uint8_t> buffer(len);
    Parcel parcel1(buffer.data(), buffer.size());
    obj1.SerializeData(parcel1, SOFTWARE_VERSION_CURRENT);
    ASSERT_EQ(parcel1.IsError(), false);

    /**
     * @tc.steps:step2. Deserialize obj1.
     * @tc.expected: Deserialize successfully.
     */
    QuerySyncObject obj2;
    Parcel parcel2(buffer.data(), buffer.size());
    ASSERT_EQ(parcel2.IsError(), false);

    /**
     * @tc.steps:step3. check object identity
     * @tc.expected: identity should be the same.
     */
    EXPECT_NE(obj1.GetIdentify(), obj2.GetIdentify());
}

/**
 * @tc.name: MultiInkeys1
 * @tc.desc: Test the rc when multiple inkeys exists.
 * @tc.type: FUNC
 * @tc.require: AR000GOHO7
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBStorageQuerySyncTest, MultiInkeys1, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Create an invalid query, with multiple inkeys.
     */
    Query query = Query::Select().InKeys({KEY_1, KEY_2}).InKeys({KEY_3});

    /**
     * @tc.steps:step2. Get data.
     * @tc.expected: Return INVALID_QUERY_FORMAT.
     */
    std::vector<Entry> entries;
    IOption option;
    option.dataType = IOption::SYNC_DATA;
    EXPECT_EQ(g_schemaConnect->GetEntries(option, query, entries), -E_INVALID_QUERY_FORMAT);
}