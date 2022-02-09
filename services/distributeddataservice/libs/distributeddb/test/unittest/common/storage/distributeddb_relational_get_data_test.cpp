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
#ifdef RELATIONAL_STORE
#include <gtest/gtest.h>

#include "data_transformer.h"
#include "db_common.h"
#include "db_constant.h"
#include "db_errno.h"
#include "db_types.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "generic_single_ver_kv_entry.h"
#include "kvdb_properties.h"
#include "log_print.h"
#include "relational_schema_object.h"
#include "relational_store_delegate.h"
#include "relational_store_instance.h"
#include "relational_store_manager.h"
#include "relational_store_sqlite_ext.h"
#include "relational_sync_able_storage.h"
#include "sqlite_relational_store.h"
#include "sqlite_utils.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
string g_testDir;
string g_storePath;
string g_storeID = "dftStoreID";
const string g_tableName { "data" };
DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
RelationalStoreDelegate *g_delegate = nullptr;
IRelationalStore *g_store = nullptr;

void CreateDBAndTable()
{
    sqlite3 *db = nullptr;
    int errCode = sqlite3_open(g_storePath.c_str(), &db);
    if (errCode != SQLITE_OK) {
        LOGE("open db failed:%d", errCode);
        sqlite3_close(db);
        return;
    }

    const string sql =
        "PRAGMA journal_mode=WAL;"
        "CREATE TABLE " + g_tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    char *zErrMsg = nullptr;
    errCode = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &zErrMsg);
    if (errCode != SQLITE_OK) {
        LOGE("sql error:%s", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    sqlite3_close(db);
}

int AddOrUpdateRecord(int64_t key, int64_t value)
{
    sqlite3 *db = nullptr;
    int errCode = sqlite3_open(g_storePath.c_str(), &db);
    if (errCode == SQLITE_OK) {
        const string sql =
            "INSERT OR REPLACE INTO " + g_tableName + " VALUES(" + to_string(key) + "," + to_string(value) + ");";
        errCode = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
    }
    errCode = SQLiteUtils::MapSQLiteErrno(errCode);
    sqlite3_close(db);
    return errCode;
}

int GetLogData(int key, uint64_t &flag, TimeStamp &timestamp, const DeviceID &device = "")
{
    string tableName = g_tableName;
    if (!device.empty()) {
    }
    const string sql = "SELECT timestamp, flag \
        FROM " + g_tableName + " as a, " + DBConstant::RELATIONAL_PREFIX + g_tableName + "_log as b \
        WHERE a.key=? AND a.rowid=b.data_key;";

    sqlite3 *db = nullptr;
    sqlite3_stmt *statement = nullptr;
    int errCode = sqlite3_open(g_storePath.c_str(), &db);
    if (errCode != SQLITE_OK) {
        LOGE("open db failed:%d", errCode);
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
        goto END;
    }
    errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = SQLiteUtils::BindInt64ToStatement(statement, 1, key); // 1 means key's index
    if (errCode != E_OK) {
        goto END;
    }
    errCode = SQLiteUtils::StepWithRetry(statement, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        timestamp = static_cast<TimeStamp>(sqlite3_column_int64(statement, 0));
        flag = static_cast<TimeStamp>(sqlite3_column_int64(statement, 1));
        errCode = E_OK;
    } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = -E_NOT_FOUND;
    }

END:
    SQLiteUtils::ResetStatement(statement, true, errCode);
    sqlite3_close(db);
    return errCode;
}

void InitStoreProp(const std::string &storePath, const std::string &appId, const std::string &userId,
    RelationalDBProperties &properties)
{
    properties.SetStringProp(RelationalDBProperties::DATA_DIR, storePath);
    properties.SetStringProp(RelationalDBProperties::APP_ID, appId);
    properties.SetStringProp(RelationalDBProperties::USER_ID, userId);
    properties.SetStringProp(RelationalDBProperties::STORE_ID, g_storeID);
    std::string identifier = userId + "-" + appId + "-" + g_storeID;
    std::string hashIdentifier = DBCommon::TransferHashString(identifier);
    properties.SetStringProp(RelationalDBProperties::IDENTIFIER_DATA, hashIdentifier);
}

const RelationalSyncAbleStorage *GetRelationalStore()
{
    RelationalDBProperties properties;
    InitStoreProp(g_storePath, APP_ID, USER_ID, properties);
    int errCode = E_OK;
    g_store = RelationalStoreInstance::GetDataBase(properties, errCode);
    if (g_store == nullptr) {
        LOGE("Get db failed:%d", errCode);
        return nullptr;
    }
    return static_cast<SQLiteRelationalStore *>(g_store)->GetStorageEngine();
}

int GetCount(sqlite3 *db, const string &sql, size_t &count)
{
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        count = static_cast<size_t>(sqlite3_column_int64(stmt, 0));
        errCode = E_OK;
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}

int PutBatchData(uint32_t totalCount, uint32_t valueSize)
{
    sqlite3 *db = nullptr;
    sqlite3_stmt *stmt = nullptr;
    const string sql = "INSERT INTO " + g_tableName + " VALUES(?,?);";
    int errCode = sqlite3_open(g_storePath.c_str(), &db);
    if (errCode != SQLITE_OK) {
        goto ERROR;
    }
    errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        goto ERROR;
    }
    for (uint32_t i = 0; i < totalCount; i++) {
        errCode = SQLiteUtils::BindBlobToStatement(stmt, 2, Value(valueSize, 'a'), false);
        if (errCode != E_OK) {
            break;
        }
        errCode = SQLiteUtils::StepWithRetry(stmt);
        if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            break;
        }
        errCode = E_OK;
        SQLiteUtils::ResetStatement(stmt, false, errCode);
    }

ERROR:
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    errCode = SQLiteUtils::MapSQLiteErrno(errCode);
    sqlite3_close(db);
    return errCode;
}
}

class DistributedDBRelationalGetDataTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBRelationalGetDataTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_storePath = g_testDir + "/getDataTest.db";
    LOGI("The test db is:%s", g_testDir.c_str());
}

void DistributedDBRelationalGetDataTest::TearDownTestCase(void)
{}

void DistributedDBRelationalGetDataTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    CreateDBAndTable();
}

void DistributedDBRelationalGetDataTest::TearDown(void)
{
    if (g_delegate != nullptr) {
        EXPECT_EQ(g_mgr.CloseStore(g_delegate), DBStatus::OK);
        g_delegate = nullptr;
    }
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
    return;
}

/**
 * @tc.name: LogTbl1
 * @tc.desc: When put sync data to relational store, trigger generate log.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalGetDataTest, LogTbl1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step1. Put data.
     * @tc.expected: Succeed, return OK.
     */
    int insertKey = 1;
    int insertValue = 1;
    EXPECT_EQ(AddOrUpdateRecord(insertKey, insertValue), E_OK);

    /**
     * @tc.steps: step2. Check log record.
     * @tc.expected: Record exists.
     */
    uint64_t flag = 0;
    TimeStamp timestamp1 = 0;
    EXPECT_EQ(GetLogData(insertKey, flag, timestamp1), E_OK);
    EXPECT_EQ(flag, DataItem::LOCAL_FLAG);
    EXPECT_NE(timestamp1, 0ULL);
}

/**
 * @tc.name: GetSyncData1
 * @tc.desc: GetSyncData interface
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
  */
HWTEST_F(DistributedDBRelationalGetDataTest, GetSyncData1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step1. Put 500 records.
     * @tc.expected: Succeed, return OK.
     */
    const int RECORD_COUNT = 500;
    for (int i = 0; i < RECORD_COUNT; ++i) {
        EXPECT_EQ(AddOrUpdateRecord(i, i), E_OK);
    }

    /**
     * @tc.steps: step2. Get all data.
     * @tc.expected: Succeed and the count is right.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    ContinueToken token = nullptr;
    QueryObject query(Query::Select(g_tableName));
    std::vector<SingleVerKvEntry *> entries;
    DataSizeSpecInfo sizeInfo {MTU_SIZE, 50};

    int errCode = store->GetSyncData(query, SyncTimeRange {}, sizeInfo, token, entries);
    int count = entries.size();
    SingleVerKvEntry::Release(entries);
    EXPECT_EQ(errCode, -E_UNFINISHED);
    while (token != nullptr) {
        errCode = store->GetSyncDataNext(entries, token, sizeInfo);
        count += entries.size();
        SingleVerKvEntry::Release(entries);
        EXPECT_TRUE(errCode == E_OK || errCode == -E_UNFINISHED);
    }
    EXPECT_EQ(count, RECORD_COUNT);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: GetSyncData2
 * @tc.desc: GetSyncData interface. For overlarge data(over 4M), ignore it.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
  */
HWTEST_F(DistributedDBRelationalGetDataTest, GetSyncData2, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step1. Put 10 records.(1M + 2M + 3M + 4M + 5M) * 2.
     * @tc.expected: Succeed, return OK.
     */
    for (int i = 1; i <= 5; ++i) {
        EXPECT_EQ(PutBatchData(1, i * 1024 * 1024), E_OK);  // 1024*1024 equals 1M.
    }
    for (int i = 1; i <= 5; ++i) {
        EXPECT_EQ(PutBatchData(1, i * 1024 * 1024), E_OK);  // 1024*1024 equals 1M.
    }

    /**
     * @tc.steps: step2. Get all data.
     * @tc.expected: Succeed and the count is 6.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    ContinueToken token = nullptr;
    QueryObject query(Query::Select(g_tableName));
    std::vector<SingleVerKvEntry *> entries;

    const size_t EXPECT_COUNT = 6;  // expect 6 records.
    DataSizeSpecInfo sizeInfo;
    sizeInfo.blockSize = 100 * 1024 * 1024;  // permit 100M.
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, sizeInfo, token, entries), E_OK);
    EXPECT_EQ(entries.size(), EXPECT_COUNT);
    SingleVerKvEntry::Release(entries);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: GetSyncData3
 * @tc.desc: GetSyncData interface. For deleted data.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalGetDataTest, GetSyncData3, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step1. Create distributed table "dataPlus".
     * @tc.expected: Succeed, return OK.
     */
    const string tableName = g_tableName + "Plus";
    std::string sql = "CREATE TABLE " + tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegate->CreateDistributedTable(tableName), DBStatus::OK);

    /**
     * @tc.steps: step2. Put 5 records with different type into "dataPlus" table.
     * @tc.expected: Succeed, return OK.
     */
    vector<string> sqls = {
        "INSERT INTO " + tableName + " VALUES(NULL, 1);",
        "INSERT INTO " + tableName + " VALUES(NULL, 0.01);",
        "INSERT INTO " + tableName + " VALUES(NULL, NULL);",
        "INSERT INTO " + tableName + " VALUES(NULL, 'This is a text.');",
        "INSERT INTO " + tableName + " VALUES(NULL, x'0123456789');",
    };
    const size_t RECORD_COUNT = sqls.size();
    for (const auto &sql : sqls) {
        ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    }

    /**
     * @tc.steps: step3. Get all data from "dataPlus" table.
     * @tc.expected: Succeed and the count is right.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    ContinueToken token = nullptr;
    QueryObject query(Query::Select(tableName));
    std::vector<SingleVerKvEntry *> entries;
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    EXPECT_EQ(entries.size(), RECORD_COUNT);

    /**
     * @tc.steps: step4. Put data into "data" table from deviceA.
     * @tc.expected: Succeed, return OK.
     */
    query = QueryObject(Query::Select(g_tableName));
    const DeviceID deviceID = "deviceA";
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(db, store->GetSchemaInfo().GetTable(g_tableName),
        DBCommon::GetDistributedTableName(deviceID, g_tableName)));
    EXPECT_EQ(const_cast<RelationalSyncAbleStorage *>(store)->PutSyncDataWithQuery(query, entries, deviceID), E_OK);
    SingleVerKvEntry::Release(entries);

    /**
     * @tc.steps: step5. Delete all "dataPlus" data.
     * @tc.expected: Succeed.
     */
    sql = "DELETE FROM " + tableName + ";";
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);

    /**
     * @tc.steps: step6. Get all data from "dataPlus" table.
     * @tc.expected: Succeed and the count is right.
     */
    query = QueryObject(Query::Select(tableName));
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    EXPECT_EQ(entries.size(), RECORD_COUNT);

    /**
     * @tc.steps: step7. Put data into "data" table from deviceA.
     * @tc.expected: Succeed, return OK.
     */
    query = QueryObject(Query::Select(g_tableName));
    EXPECT_EQ(const_cast<RelationalSyncAbleStorage *>(store)->PutSyncDataWithQuery(query, entries, deviceID), E_OK);
    SingleVerKvEntry::Release(entries);

    /**
     * @tc.steps: step8. Check data.
     * @tc.expected: All data in the two tables are deleted.
     */
    sql = "SELECT count(*) FROM " + DBConstant::RELATIONAL_PREFIX + tableName + "_log WHERE flag=3;";
    size_t count = 0;
    EXPECT_EQ(GetCount(db, sql, count), E_OK);
    EXPECT_EQ(count, RECORD_COUNT);

    sql = "SELECT count(*) FROM " + DBConstant::RELATIONAL_PREFIX + g_tableName + "_log WHERE flag=1;";
    count = 0;
    EXPECT_EQ(GetCount(db, sql, count), E_OK);
    EXPECT_EQ(count, RECORD_COUNT);

    sql = "SELECT count(*) FROM " + DBConstant::RELATIONAL_PREFIX + g_tableName + "_log as a," +
                                    DBConstant::RELATIONAL_PREFIX + tableName + "_log as b " +
          "WHERE hex(a.hash_key)=hex(b.hash_key);";
    count = 0;
    EXPECT_EQ(GetCount(db, sql, count), E_OK);
    EXPECT_EQ(count, RECORD_COUNT);

    sqlite3_close(db);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: GetQuerySyncData1
 * @tc.desc: GetSyncData interface.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalGetDataTest, GetQuerySyncData1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step1. Put 100 records.
     * @tc.expected: Succeed, return OK.
     */
    const int RECORD_COUNT = 100; // 100 records.
    for (int i = 0; i < RECORD_COUNT; ++i) {
        EXPECT_EQ(AddOrUpdateRecord(i, i), E_OK);
    }

    /**
     * @tc.steps: step2. Get data limit 80, offset 30.
     * @tc.expected: Get 70 records.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    ContinueToken token = nullptr;
    const unsigned int LIMIT = 80; // limit as 80.
    const unsigned int OFFSET = 30; // offset as 30.
    const unsigned int EXPECT_COUNT = RECORD_COUNT - OFFSET; // expect 70 records.
    QueryObject query(Query::Select(g_tableName).Limit(LIMIT, OFFSET));
    std::vector<SingleVerKvEntry *> entries;

    int errCode = store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries);
    EXPECT_EQ(entries.size(), EXPECT_COUNT);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(token, nullptr);
    SingleVerKvEntry::Release(entries);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: GetQuerySyncData2
 * @tc.desc: GetSyncData interface.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalGetDataTest, GetQuerySyncData2, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step1. Put 100 records.
     * @tc.expected: Succeed, return OK.
     */
    const int RECORD_COUNT = 100; // 100 records.
    for (int i = 0; i < RECORD_COUNT; ++i) {
        EXPECT_EQ(AddOrUpdateRecord(i, i), E_OK);
    }

    /**
     * @tc.steps: step2. Get record whose key is not equal to 10 and value is not equal to 20, order by key desc.
     * @tc.expected: Succeed, Get 98 records.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    ContinueToken token = nullptr;

    Query query = Query::Select(g_tableName).NotEqualTo("key", 10).And().NotEqualTo("value", 20).OrderBy("key", false);
    size_t expectCount = 98; // expect 98 records.
    QueryObject queryObj(query);
    queryObj.SetSchema(store->GetSchemaInfo());

    std::vector<SingleVerKvEntry *> entries;
    EXPECT_EQ(store->GetSyncData(queryObj, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    EXPECT_EQ(entries.size(), expectCount);
    EXPECT_EQ(token, nullptr);
    for (auto iter = entries.begin(); iter != entries.end(); ++iter) {
        auto nextOne = std::next(iter, 1);
        if (nextOne != entries.end()) {
            EXPECT_GT((*iter)->GetTimestamp(), (*nextOne)->GetTimestamp());
        }
    }
    SingleVerKvEntry::Release(entries);

    /**
     * @tc.steps: step3. Get record whose key is equal to 10 or value is equal to 20, order by key asc.
     * @tc.expected: Succeed, Get 98 records.
     */
    query = Query::Select(g_tableName).EqualTo("key", 10).Or().EqualTo("value", 20).OrderBy("key", true);
    expectCount = 2; // expect 2 records.
    queryObj = QueryObject(query);
    queryObj.SetSchema(store->GetSchemaInfo());

    EXPECT_EQ(store->GetSyncData(queryObj, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    EXPECT_EQ(entries.size(), expectCount);
    EXPECT_EQ(token, nullptr);
    for (auto iter = entries.begin(); iter != entries.end(); ++iter) {
        auto nextOne = std::next(iter, 1);
        if (nextOne != entries.end()) {
            EXPECT_LT((*iter)->GetTimestamp(), (*nextOne)->GetTimestamp());
        }
    }
    SingleVerKvEntry::Release(entries);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: GetIncorrectTypeData1
 * @tc.desc: GetSyncData and PutSyncDataWithQuery interface.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalGetDataTest, GetIncorrectTypeData1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step1. Create 2 index for table "data".
     * @tc.expected: Succeed, return OK.
     */
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    string sql = "CREATE INDEX index1 ON " + g_tableName + "(value);"
                 "CREATE UNIQUE INDEX index2 ON " + g_tableName + "(value,key);";
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);

    /**
     * @tc.steps: step2. Create distributed table "dataPlus".
     * @tc.expected: Succeed, return OK.
     */
    const string tableName = g_tableName + "Plus";
    sql = "CREATE TABLE " + tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegate->CreateDistributedTable(tableName), DBStatus::OK);

    /**
     * @tc.steps: step3. Put 5 records with different type into "dataPlus" table.
     * @tc.expected: Succeed, return OK.
     */
    vector<string> sqls = {
        "INSERT INTO " + tableName + " VALUES(NULL, 1);",
        "INSERT INTO " + tableName + " VALUES(NULL, 0.01);",
        "INSERT INTO " + tableName + " VALUES(NULL, NULL);",
        "INSERT INTO " + tableName + " VALUES(NULL, 'This is a text.');",
        "INSERT INTO " + tableName + " VALUES(NULL, x'0123456789');",
    };
    const size_t RECORD_COUNT = sqls.size();
    for (const auto &sql : sqls) {
        ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    }

    /**
     * @tc.steps: step4. Get all data from "dataPlus" table.
     * @tc.expected: Succeed and the count is right.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    ContinueToken token = nullptr;
    QueryObject query(Query::Select(tableName));
    std::vector<SingleVerKvEntry *> entries;
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    EXPECT_EQ(entries.size(), RECORD_COUNT);

    /**
     * @tc.steps: step5. Put data into "data" table from deviceA.
     * @tc.expected: Succeed, return OK.
     */
    QueryObject queryPlus(Query::Select(g_tableName));
    const DeviceID deviceID = "deviceA";
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(db, store->GetSchemaInfo().GetTable(g_tableName),
        DBCommon::GetDistributedTableName(deviceID, g_tableName)));
    ASSERT_EQ(E_OK, SQLiteUtils::CloneIndexes(db, g_tableName,
        DBCommon::GetDistributedTableName(deviceID, g_tableName)));
    EXPECT_EQ(const_cast<RelationalSyncAbleStorage *>(store)->PutSyncDataWithQuery(queryPlus, entries, deviceID), E_OK);
    SingleVerKvEntry::Release(entries);

    /**
     * @tc.steps: step6. Check data.
     * @tc.expected: All data in the two tables are same.
     */
    sql = "SELECT count(*) "
        "FROM " + tableName + " as a, " + DBConstant::RELATIONAL_PREFIX + g_tableName + "_" +
            DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceID)) + " as b "
        "WHERE a.key=b.key AND (a.value=b.value OR (a.value is NULL AND b.value is NULL));";
    size_t count = 0;
    EXPECT_EQ(GetCount(db, sql, count), E_OK);
    EXPECT_EQ(count, RECORD_COUNT);

    /**
     * @tc.steps: step7. Check index.
     * @tc.expected: 2 index for deviceA's data table exists.
     */
    sql = "SELECT count(*) FROM sqlite_master WHERE type='index' AND tbl_name='" + DBConstant::RELATIONAL_PREFIX +
        g_tableName + "_" + DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceID)) + "'";
    EXPECT_EQ(GetCount(db, sql, count), E_OK);
    EXPECT_EQ(count, 2UL); // The index count is 2.
    sqlite3_close(db);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: UpdateData1
 * @tc.desc: UpdateData succeed when the table has primary key.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalGetDataTest, UpdateData1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step1. Create distributed table "dataPlus".
     * @tc.expected: Succeed, return OK.
     */
    const string tableName = g_tableName + "Plus";
    std::string sql = "CREATE TABLE " + tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegate->CreateDistributedTable(tableName), DBStatus::OK);

    /**
     * @tc.steps: step2. Put 5 records with different type into "dataPlus" table.
     * @tc.expected: Succeed, return OK.
     */
    vector<string> sqls = {
        "INSERT INTO " + tableName + " VALUES(NULL, 1);",
        "INSERT INTO " + tableName + " VALUES(NULL, 0.01);",
        "INSERT INTO " + tableName + " VALUES(NULL, NULL);",
        "INSERT INTO " + tableName + " VALUES(NULL, 'This is a text.');",
        "INSERT INTO " + tableName + " VALUES(NULL, x'0123456789');",
    };
    const size_t RECORD_COUNT = sqls.size();
    for (const auto &sql : sqls) {
        ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    }

    /**
     * @tc.steps: step3. Get all data from "dataPlus" table.
     * @tc.expected: Succeed and the count is right.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    ContinueToken token = nullptr;
    QueryObject query(Query::Select(tableName));
    std::vector<SingleVerKvEntry *> entries;
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    EXPECT_EQ(entries.size(), RECORD_COUNT);

    /**
     * @tc.steps: step4. Put data into "data" table from deviceA for 10 times.
     * @tc.expected: Succeed, return OK.
     */
    query = QueryObject(Query::Select(g_tableName));
    const DeviceID deviceID = "deviceA";
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(db, store->GetSchemaInfo().GetTable(g_tableName),
        DBCommon::GetDistributedTableName(deviceID, g_tableName)));
    for (uint32_t i = 0; i < 10; ++i) {  // 10 for test.
        EXPECT_EQ(const_cast<RelationalSyncAbleStorage *>(store)->PutSyncDataWithQuery(query, entries, deviceID), E_OK);
    }
    SingleVerKvEntry::Release(entries);

    /**
     * @tc.steps: step5. Check data.
     * @tc.expected: There is 5 data in table.
     */
    sql = "SELECT count(*) FROM " + DBConstant::RELATIONAL_PREFIX + g_tableName + "_" +
        DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceID)) + ";";
    size_t count = 0;
    EXPECT_EQ(GetCount(db, sql, count), E_OK);
    EXPECT_EQ(count, RECORD_COUNT);

    sql = "SELECT count(*) FROM " + DBConstant::RELATIONAL_PREFIX + g_tableName + "_log;";
    count = 0;
    EXPECT_EQ(GetCount(db, sql, count), E_OK);
    EXPECT_EQ(count, RECORD_COUNT);

    sqlite3_close(db);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: UpdateDataWithMulDevData1
 * @tc.desc: UpdateData succeed when there is multiple devices data exists.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalGetDataTest, UpdateDataWithMulDevData1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);
    /**
     * @tc.steps: step1. Create distributed table "dataPlus".
     * @tc.expected: Succeed, return OK.
     */
    const string tableName = g_tableName + "Plus";
    std::string sql = "CREATE TABLE " + tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegate->CreateDistributedTable(tableName), DBStatus::OK);
    /**
     * @tc.steps: step2. Put k1v1 into "dataPlus" table.
     * @tc.expected: Succeed, return OK.
     */
    sql = "INSERT INTO " + tableName + " VALUES(1, 1);"; // k1v1
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    /**
     * @tc.steps: step3. Get k1v1 from "dataPlus" table.
     * @tc.expected: Succeed and the count is right.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    ContinueToken token = nullptr;
    QueryObject query(Query::Select(tableName));
    std::vector<SingleVerKvEntry *> entries;
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    /**
     * @tc.steps: step4. Put k1v1 into "data" table from deviceA.
     * @tc.expected: Succeed, return OK.
     */
    query = QueryObject(Query::Select(g_tableName));
    const DeviceID deviceID = "deviceA";
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(db, store->GetSchemaInfo().GetTable(g_tableName),
        DBCommon::GetDistributedTableName(deviceID, g_tableName)));
    EXPECT_EQ(const_cast<RelationalSyncAbleStorage *>(store)->PutSyncDataWithQuery(query, entries, deviceID), E_OK);
    SingleVerKvEntry::Release(entries);
    /**
     * @tc.steps: step4. Put k1v1 into "data" table.
     * @tc.expected: Succeed, return OK.
     */
    EXPECT_EQ(AddOrUpdateRecord(1, 1), E_OK); // k1v1
    /**
     * @tc.steps: step5. Change k1v1 to k1v2
     * @tc.expected: Succeed, return OK.
     */
    sql = "UPDATE " + g_tableName + " SET value=2 WHERE key=1;"; // k1v1
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK); // change k1v1 to k1v2
}
#endif