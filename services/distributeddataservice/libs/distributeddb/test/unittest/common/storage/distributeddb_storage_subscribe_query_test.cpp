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
#include "process_communicator_test_stub.h"
#include "process_system_api_adapter_impl.h"
#include "query_sync_object.h"
#include "sqlite_single_ver_natural_store.h"
#include "sqlite_single_ver_natural_store_connection.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
DistributedDB::KvStoreConfig g_config;
std::string g_testDir;
string g_resourceDir;

KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
// define the g_kvDelegateCallback, used to get some information when open a kv store.
DBStatus g_kvDelegateStatus = INVALID_ARGS;
KvStoreNbDelegate *g_kvNbDelegatePtr = nullptr;
auto g_kvNbDelegateCallback = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
    placeholders::_1, placeholders::_2, std::ref(g_kvDelegateStatus), std::ref(g_kvNbDelegatePtr));

const uint8_t PRESET_DATA_SIZE = 2;
const std::string SUBSCRIBE_ID = "680A20600517073AE306B11FEA8306C57DC5102CD33E322F7C513176AA707F0C";

const std::string REMOTE_DEVICE_ID = "remote_device_id";
const std::string REMOTE_DEVICE_A = "remote_device_A";
const std::string REMOTE_DEVICE_B = "remote_device_B";
const Key PREFIX_KEY = { 'k' };
const Key KEY1 = { 'k', '1' };
const Key KEY2 = { 'k', '2' };
const Key KEY3 = { 'k', '3' };
const Value VALUE1 = { 'v', '1' };
const Value VALUE2 = { 'v', '2' };
const Value VALUE3 = { 'v', '3' };

const std::string NORMAL_FBS_FILE_NAME = "normal_fbs.bfbs";
const string SCHEMA_STRING =
    "{\"SCHEMA_VERSION\":\"1.0\","
    "\"SCHEMA_MODE\":\"STRICT\","
    "\"SCHEMA_DEFINE\":{"
    "\"field_name1\":\"BOOL\","
    "\"field_name2\":\"BOOL\","
    "\"field_name3\":\"INTEGER, NOT NULL\","
    "\"field_name4\":\"LONG, DEFAULT 100\","
    "\"field_name5\":\"DOUBLE, DEFAULT 3.14\","
    "\"field_name6\":\"STRING, DEFAULT '3.1415'\","
    "\"field_name7\":\"LONG, DEFAULT 100\","
    "\"field_name8\":\"LONG, DEFAULT 100\","
    "\"field_name9\":\"LONG, DEFAULT 100\","
    "\"field_name10\":\"LONG, DEFAULT 100\""
    "},"
    "\"SCHEMA_INDEXES\":[\"$.field_name1\", \"$.field_name2\"]}";

void PreSetData(uint8_t dataNum)
{
    EXPECT_GE(dataNum, 0); // 0 No preset data
    EXPECT_LT(dataNum, 128); // 128 Max preset data size
    for (uint8_t i = 0; i < dataNum; i++) {
        Key keyA = {'K', i};
        Value value;
        std::string validJsonData;
        if (i % 2 == 0) { // 2 : for data construct
            validJsonData = R"({"field_name1":false,"field_name2":true,"field_name3":100})";
        } else {
            validJsonData = R"({"field_name1":false,"field_name2":false,"field_name3":100})";
        }
        value.assign(validJsonData.begin(), validJsonData.end());
        EXPECT_EQ(g_kvNbDelegatePtr->Put(keyA, value), E_OK);
    }
}

void CreateAndGetStore(const std::string &storeId, const std::string &schemaString,
    SQLiteSingleVerNaturalStoreConnection *&conn, SQLiteSingleVerNaturalStore *&store, uint8_t preSetDataNum = 0)
{
    KvStoreNbDelegate::Option option = {true, false, false};
    option.schema = schemaString;
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    EXPECT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    PreSetData(preSetDataNum);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);

    std::string oriIdentifier = USER_ID + "-" + APP_ID + "-" + storeId;
    std::string identifier = DBCommon::TransferHashString(oriIdentifier);
    KvDBProperties property;
    property.SetStringProp(KvDBProperties::IDENTIFIER_DATA, identifier);
    std::string identifierHex = DBCommon::TransferStringToHex(identifier);
    property.SetStringProp(KvDBProperties::DATA_DIR, g_testDir);
    property.SetStringProp(KvDBProperties::STORE_ID, storeId);
    property.SetBoolProp(KvDBProperties::MEMORY_MODE, false);
    property.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE);
    property.SetStringProp(KvDBProperties::IDENTIFIER_DIR, identifierHex);
    property.SetIntProp(KvDBProperties::CONFLICT_RESOLVE_POLICY, ConflictResolvePolicy::LAST_WIN);

    if (!schemaString.empty()) {
        SchemaObject schemaObj;
        schemaObj.ParseFromSchemaString(schemaString);
        EXPECT_EQ(schemaObj.IsSchemaValid(), true);
        property.SetSchema(schemaObj);
    }

    int errCode = E_OK;
    conn = static_cast<SQLiteSingleVerNaturalStoreConnection *>(KvDBManager::GetDatabaseConnection(property, errCode));
    EXPECT_EQ(errCode, E_OK);
    ASSERT_NE(conn, nullptr);
    store = static_cast<SQLiteSingleVerNaturalStore *>(KvDBManager::OpenDatabase(property, errCode));
    EXPECT_EQ(errCode, E_OK);
    ASSERT_NE(store, nullptr);
}

#ifndef OMIT_FLATBUFFER
std::string FbfFileToSchemaString(const std::string &fileName)
{
    std::string filePath = g_resourceDir + "fbs_files_for_ut/" + fileName;
    std::ifstream is(filePath, std::ios::binary | std::ios::ate);
    if (!is.is_open()) {
        LOGE("[FbfFileToSchemaString] open file failed name : %s", filePath.c_str());
        return "";
    }

    auto size = is.tellg();
    LOGE("file size %u", static_cast<unsigned>(size));
    std::string schema(size, '\0');
    is.seekg(0);
    if (is.read(&schema[0], size)) {
        return schema;
    }
    LOGE("[FbfFileToSchemaString] read file failed path : %s", filePath.c_str());
    return "";
}
#endif

void CheckDataNumByKey(const std::string &storeId, const Key& key, size_t expSize)
{
    KvStoreNbDelegate::Option option = {true, false, false};
    option.schema = SCHEMA_STRING;
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    EXPECT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    std::vector<Entry> entries;
    EXPECT_EQ(g_kvNbDelegatePtr->GetEntries(key, entries), E_OK);
    EXPECT_TRUE(entries.size() == expSize);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
}
}

class DistributedDBStorageSubscribeQueryTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
};

static std::shared_ptr<ProcessSystemApiAdapterImpl> g_adapter;
void DistributedDBStorageSubscribeQueryTest::SetUpTestCase(void)
{
    g_mgr.SetProcessLabel("DistributedDBStorageSubscribeQueryTest", "test");
    g_mgr.SetProcessCommunicator(std::make_shared<ProcessCommunicatorTestStub>()); // export and import get devID

    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    ASSERT_EQ(DistributedDBToolsUnitTest::GetResourceDir(g_resourceDir), E_OK);
    LOGD("Test dir is %s", g_testDir.c_str());
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir + "/TestQuerySync/" + DBConstant::SINGLE_SUB_DIR);

    g_config.dataDir = g_testDir;
    g_mgr.SetKvStoreConfig(g_config);

    g_adapter = std::make_shared<ProcessSystemApiAdapterImpl>();
    EXPECT_TRUE(g_adapter != nullptr);
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter);
}

void DistributedDBStorageSubscribeQueryTest::TearDownTestCase(void)
{
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
}

void DistributedDBStorageSubscribeQueryTest::SetUp()
{
    Test::SetUp();
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

void DistributedDBStorageSubscribeQueryTest::TearDown()
{
    Test::TearDown();
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
}

/**
  * @tc.name: CheckAndInitQueryCondition001
  * @tc.desc: Check the condition is legal or not with json schema
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBStorageSubscribeQueryTest, CheckAndInitQueryCondition001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Create a json schema db, get the natural store instance.
     * @tc.expected: step1. Get results OK and non-null store.
     */
    SQLiteSingleVerNaturalStoreConnection *conn = nullptr;
    SQLiteSingleVerNaturalStore *store = nullptr;
    CreateAndGetStore("SchemaCondition01", SCHEMA_STRING, conn, store);

    /**
     * @tc.steps:step2. Create a query with prefixKey only, check it as condition.
     * @tc.expected: step2. Check condition return E_OK.
     */
    Query query1 = Query::Select().PrefixKey({});
    QueryObject queryObject1(query1);
    int errCode = store->CheckAndInitQueryCondition(queryObject1);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps:step3. Create a query with predicate, check it as condition.
     * @tc.expected: step3. Check condition return E_OK.
     */
    Query query2 = Query::Select().GreaterThan("field_name3", 10);
    QueryObject queryObject2(query2);
    errCode = store->CheckAndInitQueryCondition(queryObject2);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps:step4. Create a query with invalid field, check it as condition.
     * @tc.expected: step4. Check condition return E_INVALID_QUERY_FIELD.
     */
    Query query3 = Query::Select().GreaterThan("field_name11", 10);
    QueryObject queryObject3(query3);
    errCode = store->CheckAndInitQueryCondition(queryObject3);
    EXPECT_EQ(errCode, -E_INVALID_QUERY_FIELD);

    /**
     * @tc.steps:step5. Create a query with invalid format, check it as condition.
     * @tc.expected: step5. Check condition return E_INVALID_QUERY_FORMAT.
     */
    Query query4 = Query::Select().GreaterThan("field_name3", 10).And().BeginGroup().
        LessThan("field_name3", 100).OrderBy("field_name3").EndGroup();
    QueryObject queryObject4(query4);
    errCode = store->CheckAndInitQueryCondition(queryObject4);
    EXPECT_EQ(errCode, -E_INVALID_QUERY_FORMAT);

    /**
     * @tc.steps:step6. Close natural store
     * @tc.expected: step6. Close ok
     */
    RefObject::KillAndDecObjRef(store);
    KvDBManager::ReleaseDatabaseConnection(conn);
}

#ifndef OMIT_FLATBUFFER
/**
  * @tc.name: CheckAndInitQueryCondition002
  * @tc.desc: Check the condition always illegal with flatbuffer schema
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBStorageSubscribeQueryTest, CheckAndInitQueryCondition002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Create a flatbuffer schema db, get the natural store instance.
     * @tc.expected: step1. Get results OK and non-null store.
     */
    std::string fbSchema = FbfFileToSchemaString(NORMAL_FBS_FILE_NAME);
    EXPECT_FALSE(fbSchema.empty());
    SQLiteSingleVerNaturalStoreConnection *conn = nullptr;
    SQLiteSingleVerNaturalStore *store = nullptr;
    CreateAndGetStore("SchemaCondition02", fbSchema, conn, store);

    /**
     * @tc.steps:step2. Create a query, check it as condition.
     *                  flatbuffer schema is not support with querySync and subscribe.
     * @tc.expected: step2. Check condition return E_NOT_SUPPORT.
     */
    Query query1 = Query::Select().PrefixKey({});
    QueryObject queryObject1(query1);
    int errCode = store->CheckAndInitQueryCondition(queryObject1);
    EXPECT_EQ(errCode, -E_NOT_SUPPORT);

    /**
     * @tc.steps:step3. Close natural store
     * @tc.expected: step3. Close ok
     */
    RefObject::KillAndDecObjRef(store);
    KvDBManager::ReleaseDatabaseConnection(conn);
}
#endif

/**
  * @tc.name: CheckAndInitQueryCondition003
  * @tc.desc: Check the condition always illegal with flatbuffer schema
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBStorageSubscribeQueryTest, CheckAndInitQueryCondition003, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Create a kv db, get the natural store instance.
     * @tc.expected: step1. Get results OK and non-null store.
     */
    SQLiteSingleVerNaturalStoreConnection *conn = nullptr;
    SQLiteSingleVerNaturalStore *store = nullptr;
    CreateAndGetStore("SchemaCondition03", "", conn, store);

    /**
     * @tc.steps:step2. Create a prefixKey query, check it as condition.
     * @tc.expected: step2. Check condition return E_OK.
     */
    Query query1 = Query::Select().PrefixKey({});
    QueryObject queryObject1(query1);
    int errCode = store->CheckAndInitQueryCondition(queryObject1);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps:step2. Create a predicate query, check it as condition.
     * @tc.expected: step2. Check condition return E_NOT_SUPPORT.
     */
    Query query2 = Query::Select().GreaterThan("field_name3", 10);
    QueryObject queryObject2(query2);
    errCode = store->CheckAndInitQueryCondition(queryObject2);
    EXPECT_EQ(errCode, -E_NOT_SUPPORT);

    /**
     * @tc.steps:step3. Close natural store
     * @tc.expected: step3. Close ok
     */
    RefObject::KillAndDecObjRef(store);
    KvDBManager::ReleaseDatabaseConnection(conn);
}

/**
  * @tc.name: PutSyncDataTestWithQuery
  * @tc.desc: put remote devices sync data(get by query sync or subscribe) with query.
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBStorageSubscribeQueryTest, PutSyncDataTestWithQuery, TestSize.Level1)
{
    /**
     * @tc.steps:step1. create and open a schema store, preset some data;
     * @tc.expected: step1. open success
     */
    const std::string storeId = "PutSyncData01";
    SQLiteSingleVerNaturalStoreConnection *conn = nullptr;
    SQLiteSingleVerNaturalStore *store = nullptr;
    CreateAndGetStore(storeId, SCHEMA_STRING, conn, store, PRESET_DATA_SIZE);

    /**
     * @tc.steps:step2. Construct sync data
     * @tc.expected: OK
     */
    Key key;
    Value value;
    Timestamp now = store->GetCurrentTimestamp();
    LOGD("now time is : %ld", now);
    std::vector<DataItem> data;
    for (uint8_t i = 0; i < PRESET_DATA_SIZE; i++) {
        DataItem item{key, value, now, DataItem::REMOTE_DEVICE_DATA_MISS_QUERY, REMOTE_DEVICE_ID, now};
        item.key.clear();
        DBCommon::CalcValueHash({'K', i}, item.key);
        EXPECT_EQ(item.key.empty(), false);
        data.push_back(item);
    }

    /**
     * @tc.steps:step3. put sync data with query
     * @tc.expected: step3. data put success
     */
    Query query = Query::Select().EqualTo("field_name2", true);
    QueryObject queryObj(query);
    EXPECT_EQ(DistributedDBToolsUnitTest::PutSyncDataTest(store, data, REMOTE_DEVICE_ID, queryObj), E_OK);

    RefObject::KillAndDecObjRef(store);
    KvDBManager::ReleaseDatabaseConnection(conn);

    /**
     * @tc.steps:step4. Check sync data
     * @tc.expected: step4. check data ok
     */
    CheckDataNumByKey(storeId, {'K'}, PRESET_DATA_SIZE / 2);
}

/**
  * @tc.name: PutSyncDataTestWithQuery002
  * @tc.desc: put remote devices sync data(timestamp is smaller then DB data) with query.
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBStorageSubscribeQueryTest, PutSyncDataTestWithQuery002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. create and open a schema store, preset some data;
     * @tc.expected: step1. open success
     */
    const std::string storeId = "PutSyncData02";

    SQLiteSingleVerNaturalStoreConnection *conn = nullptr;
    SQLiteSingleVerNaturalStore *store = nullptr;
    CreateAndGetStore(storeId, SCHEMA_STRING, conn, store);

    Key key({'K', 'e', 'y'});
    Value value;
    Timestamp now = store->GetCurrentTimestamp();
    /**
     * @tc.steps:step2. put sync data
     * @tc.expected: OK
     */
    std::string validJsonData(R"({"field_name1":false,"field_name2":true,"field_name3":100})");
    value.assign(validJsonData.begin(), validJsonData.end());
    std::vector<DataItem> data;
    DataItem item{key, value, now, DataItem::LOCAL_FLAG, REMOTE_DEVICE_ID, now};
    data.push_back(item);
    EXPECT_EQ(DistributedDBToolsUnitTest::PutSyncDataTest(store, data, REMOTE_DEVICE_ID), E_OK);

    /**
     * @tc.steps:step3. put sync miss query data with smaller timestamp
     * @tc.expected: OK
     */
    data.clear();
    value.clear();
    DataItem itemMiss{key, value, now - 1, DataItem::REMOTE_DEVICE_DATA_MISS_QUERY, REMOTE_DEVICE_ID, now - 1};
    itemMiss.key.clear();
    DBCommon::CalcValueHash({'K', 'e', 'y'}, itemMiss.key);
    EXPECT_EQ(itemMiss.key.empty(), false);
    data.push_back(itemMiss);
    Query query = Query::Select().EqualTo("field_name2", true);
    QueryObject queryObj(query);
    EXPECT_EQ(DistributedDBToolsUnitTest::PutSyncDataTest(store, data, REMOTE_DEVICE_ID, queryObj), E_OK);

    RefObject::KillAndDecObjRef(store);
    KvDBManager::ReleaseDatabaseConnection(conn);

    /**
     * @tc.steps:step4. Check sync data
     * @tc.expected: check data ok, data {key} is not erased.
     */
    CheckDataNumByKey(storeId, {'K', 'e', 'y'}, 1);
}

/**
  * @tc.name: PutSyncDataTestWithQuery003
  * @tc.desc: put remote devices sync data(with same timestamp in DB data but different devices) with query.
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBStorageSubscribeQueryTest, PutSyncDataTestWithQuery003, TestSize.Level1)
{
    /**
     * @tc.steps:step1. create and open a schema store, preset some data;
     * @tc.expected: step1. open success
     */
    const std::string storeId = "PutSyncData03";

    SQLiteSingleVerNaturalStoreConnection *conn = nullptr;
    SQLiteSingleVerNaturalStore *store = nullptr;
    CreateAndGetStore(storeId, SCHEMA_STRING, conn, store);

    Key key({'K', 'e', 'y'});
    Value value;
    Timestamp now = store->GetCurrentTimestamp();
    /**
     * @tc.steps:step2. put sync data
     * @tc.expected: OK
     */
    std::string validJsonData(R"({"field_name1":false,"field_name2":true,"field_name3":100})");
    value.assign(validJsonData.begin(), validJsonData.end());
    std::vector<DataItem> data;
    DataItem item{key, value, now, DataItem::LOCAL_FLAG, REMOTE_DEVICE_ID, now};
    data.push_back(item);
    EXPECT_EQ(DistributedDBToolsUnitTest::PutSyncDataTest(store, data, REMOTE_DEVICE_ID), E_OK);

    /**
     * @tc.steps:step3. put sync miss query data with same timestamp
     * @tc.expected: OK
     */
    data.clear();
    DataItem itemMiss{key, {}, now, DataItem::REMOTE_DEVICE_DATA_MISS_QUERY, REMOTE_DEVICE_ID, now};
    itemMiss.key.clear();
    DBCommon::CalcValueHash({'K', 'e', 'y'}, itemMiss.key);
    EXPECT_EQ(itemMiss.key.empty(), false);
    data.push_back(itemMiss);
    Query query = Query::Select().EqualTo("field_name2", true);
    QueryObject queryObj(query);
    EXPECT_EQ(DistributedDBToolsUnitTest::PutSyncDataTest(store, data, REMOTE_DEVICE_ID, queryObj), E_OK);

    RefObject::KillAndDecObjRef(store);
    KvDBManager::ReleaseDatabaseConnection(conn);

    /**
     * @tc.steps:step4. Check sync data
     * @tc.expected: check data ok, data {key} is not erased.
     */
    CheckDataNumByKey(storeId, {'K', 'e', 'y'}, 1);
}

/**
  * @tc.name: PutSyncDataTestWithQuery004
  * @tc.desc: put remote devices sync data(with same timestamp in DB data but different devices) with query.
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBStorageSubscribeQueryTest, PutSyncDataTestWithQuery004, TestSize.Level1)
{
    /**
     * @tc.steps:step1. create and open a schema store, preset some data;
     * @tc.expected: step1. open success
     */
    const std::string storeId = "PutSyncData04";
    SQLiteSingleVerNaturalStoreConnection *conn = nullptr;
    SQLiteSingleVerNaturalStore *store = nullptr;
    CreateAndGetStore(storeId, SCHEMA_STRING, conn, store);

    Key key({'K', 'e', 'y'});
    Value value;
    Timestamp now = store->GetCurrentTimestamp();
    /**
     * @tc.steps:step2. put sync data
     * @tc.expected: OK
     */
    std::string validJsonData(R"({"field_name1":false,"field_name2":true,"field_name3":100})");
    value.assign(validJsonData.begin(), validJsonData.end());
    std::vector<DataItem> data;
    DataItem item{key, value, now, DataItem::LOCAL_FLAG, REMOTE_DEVICE_A, now};
    data.push_back(item);
    EXPECT_EQ(DistributedDBToolsUnitTest::PutSyncDataTest(store, data, REMOTE_DEVICE_ID), E_OK);

    /**
     * @tc.steps:step3. put sync miss query data with same timestamp
     * @tc.expected: OK
     */
    data.clear();
    DataItem itemMiss{key, {}, now, DataItem::REMOTE_DEVICE_DATA_MISS_QUERY, REMOTE_DEVICE_B, now};
    itemMiss.key.clear();
    DBCommon::CalcValueHash({'K', 'e', 'y'}, itemMiss.key);
    EXPECT_EQ(itemMiss.key.empty(), false);
    data.push_back(itemMiss);
    Query query = Query::Select().EqualTo("field_name2", true);
    QueryObject queryObj(query);
    EXPECT_EQ(DistributedDBToolsUnitTest::PutSyncDataTest(store, data, REMOTE_DEVICE_B, queryObj), E_OK);

    RefObject::KillAndDecObjRef(store);
    KvDBManager::ReleaseDatabaseConnection(conn);

    /**
     * @tc.steps:step4. Check sync data
     * @tc.expected: check data ok, data {key} is not erased.
     */
    CheckDataNumByKey(storeId, {'K', 'e', 'y'}, 1);
}

/**
  * @tc.name: AddSubscribeTest001
  * @tc.desc: Add subscribe with query
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBStorageSubscribeQueryTest, AddSubscribeTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Create a json schema db, get the natural store instance.
     * @tc.expected: Get results OK and non-null store.
     */
    SQLiteSingleVerNaturalStoreConnection *conn = nullptr;
    SQLiteSingleVerNaturalStore *store = nullptr;
    CreateAndGetStore("SubscribeTest01", SCHEMA_STRING, conn, store);

    std::vector<Query> queryList;
    queryList.push_back(Query::Select().PrefixKey({10, 20}));
    queryList.push_back(Query::Select().EqualTo("field_name3", 30));
    queryList.push_back(Query::Select().NotEqualTo("field_name3", 30));
    queryList.push_back(Query::Select().GreaterThan("field_name3", 10));
    queryList.push_back(Query::Select().LessThan("field_name3", 30));
    queryList.push_back(Query::Select().GreaterThanOrEqualTo("field_name3", 30));
    queryList.push_back(Query::Select().LessThanOrEqualTo("field_name3", 30));
    queryList.push_back(Query::Select().Like("field_name6", "Abc%"));
    queryList.push_back(Query::Select().NotLike("field_name6", "Asd%"));
    std::vector<int> set = {1, 2, 3, 4};
    queryList.push_back(Query::Select().In("field_name3", set));
    queryList.push_back(Query::Select().NotIn("field_name3", set));
    queryList.push_back(Query::Select().IsNull("field_name4"));
    queryList.push_back(Query::Select().IsNotNull("field_name5"));
    queryList.push_back(Query::Select().EqualTo("field_name3", 30).And().EqualTo("field_name1", true));
    queryList.push_back(Query::Select().EqualTo("field_name3", 30).Or().EqualTo("field_name1", true));
    queryList.push_back(Query::Select().EqualTo("field_name2", false).Or().
        BeginGroup().EqualTo("field_name3", 30).Or().EqualTo("field_name1", true).EndGroup());

    /**
     * @tc.steps:step2. Add subscribe with query, remove subscribe.
     * @tc.expected: success.
     */
    for (const auto &query : queryList) {
        QueryObject queryObj(query);
        EXPECT_EQ(store->AddSubscribe(SUBSCRIBE_ID, queryObj, false), E_OK);
        EXPECT_EQ(store->RemoveSubscribe(SUBSCRIBE_ID), E_OK);
    }

    /**
     * @tc.steps:step6. Close natural store
     * @tc.expected: step6. Close ok
     */
    RefObject::KillAndDecObjRef(store);
    KvDBManager::ReleaseDatabaseConnection(conn);
}

/**
  * @tc.name: AddSubscribeTest002
  * @tc.desc: Add subscribe with same query not failed
  * @tc.type: FUNC
  * @tc.require: AR000FN6G9
  * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBStorageSubscribeQueryTest, AddSubscribeTest002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Create a json schema db, get the natural store instance.
     * @tc.expected: Get results OK and non-null store.
     */
    SQLiteSingleVerNaturalStoreConnection *conn = nullptr;
    SQLiteSingleVerNaturalStore *store = nullptr;
    CreateAndGetStore("SubscribeTest02", SCHEMA_STRING, conn, store);

    Query query = Query::Select().EqualTo("field_name2", false).Or().
        BeginGroup().EqualTo("field_name3", 30).Or().EqualTo("field_name1", true).EndGroup();
    /**
     * @tc.steps:step2. Add subscribe with same query
     * @tc.expected: step2. add success
     */
    QueryObject queryObj(query);
    int errCode = store->AddSubscribe(SUBSCRIBE_ID, queryObj, false);
    EXPECT_EQ(errCode, E_OK);
    errCode = store->AddSubscribe(SUBSCRIBE_ID, queryObj, false);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(store->RemoveSubscribe(SUBSCRIBE_ID), E_OK);
    /**
     * @tc.steps:step3. Close natural store
     * @tc.expected: step3. Close ok
     */
    RefObject::KillAndDecObjRef(store);
    KvDBManager::ReleaseDatabaseConnection(conn);
}