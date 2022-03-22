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

void ExpectCount(sqlite3 *db, const string &sql, size_t expectCount)
{
    size_t count = 0;
    ASSERT_EQ(GetCount(db, sql, count), E_OK);
    EXPECT_EQ(count, expectCount);
}

std::string GetOneText(sqlite3 *db, const string &sql)
{
    std::string result;
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        return result;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        SQLiteUtils::GetColumnTextValue(stmt, 0, result);
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return result;
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
    EXPECT_EQ(sqlite3_exec(db, "BEGIN IMMEDIATE TRANSACTION", nullptr, nullptr, nullptr), SQLITE_OK);
    errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        goto ERROR;
    }
    for (uint32_t i = 0; i < totalCount; i++) {
        errCode = SQLiteUtils::BindBlobToStatement(stmt, 2, Value(valueSize, 'a'), false);  // 2 means value index
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
    if (errCode == E_OK) {
        EXPECT_EQ(sqlite3_exec(db, "COMMIT TRANSACTION", nullptr, nullptr, nullptr), SQLITE_OK);
    } else {
        EXPECT_EQ(sqlite3_exec(db, "ROLLBACK TRANSACTION", nullptr, nullptr, nullptr), SQLITE_OK);
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    errCode = SQLiteUtils::MapSQLiteErrno(errCode);
    sqlite3_close(db);
    return errCode;
}

void ExecSqlAndAssertOK(sqlite3 *db, const std::string &sql)
{
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
}

void ExecSqlAndAssertOK(sqlite3 *db, const initializer_list<std::string> &sqlList)
{
    for (const auto &sql : sqlList) {
        ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    }
}

void ExpectMissQueryCnt(const std::vector<SingleVerKvEntry *> &entries, size_t expectCount)
{
    size_t count = 0;
    for (auto iter = entries.begin(); iter != entries.end(); ++iter) {
        if (((*iter)->GetFlag() & DataItem::REMOTE_DEVICE_DATA_MISS_QUERY) == 0) {
            count++;
        }
        auto nextOne = std::next(iter, 1);
        if (nextOne != entries.end()) {
            EXPECT_LT((*iter)->GetTimestamp(), (*nextOne)->GetTimestamp());
        }
    }
    EXPECT_EQ(count, expectCount);
};
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
    const size_t RECORD_COUNT = 500;
    for (size_t i = 0; i < RECORD_COUNT; ++i) {
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
    auto count = entries.size();
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
     * @tc.steps: step2. Put 5 records with different type into "dataPlus" table. Put 5 records into "data" table.
     * @tc.expected: Succeed, return OK.
     */
    const size_t RECORD_COUNT = 5;  // 5 records
    ExecSqlAndAssertOK(db, {"INSERT INTO " + tableName + " VALUES(NULL, 1);",
                            "INSERT INTO " + tableName + " VALUES(NULL, 0.01);",
                            "INSERT INTO " + tableName + " VALUES(NULL, NULL);",
                            "INSERT INTO " + tableName + " VALUES(NULL, 'This is a text.');",
                            "INSERT INTO " + tableName + " VALUES(NULL, x'0123456789');"});

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
     * @tc.steps: step4. Put data into "data" table from deviceA and deviceB
     * @tc.expected: Succeed, return OK.
     */
    QueryObject gQuery(Query::Select(g_tableName));
    DeviceID deviceA = "deviceA";
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(db, store->GetSchemaInfo().GetTable(g_tableName),
        DBCommon::GetDistributedTableName(deviceA, g_tableName)));
    EXPECT_EQ(const_cast<RelationalSyncAbleStorage *>(store)->PutSyncDataWithQuery(gQuery, entries, deviceA), E_OK);

    DeviceID deviceB = "deviceB";
    auto rEntries = std::vector<SingleVerKvEntry *>(entries.rbegin(), entries.rend());
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(db, store->GetSchemaInfo().GetTable(g_tableName),
        DBCommon::GetDistributedTableName(deviceB, g_tableName)));
    EXPECT_EQ(const_cast<RelationalSyncAbleStorage *>(store)->PutSyncDataWithQuery(gQuery, rEntries, deviceB), E_OK);
    rEntries.clear();
    SingleVerKvEntry::Release(entries);

    /**
     * @tc.steps: step5. Delete 2 "dataPlus" data from deviceA.
     * @tc.expected: Succeed.
     */
    ExecSqlAndAssertOK(db, "DELETE FROM " + tableName + " WHERE rowid<=2;");
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    EXPECT_EQ(entries.size(), RECORD_COUNT);
    EXPECT_EQ(const_cast<RelationalSyncAbleStorage *>(store)->PutSyncDataWithQuery(gQuery, entries, deviceA), E_OK);
    SingleVerKvEntry::Release(entries);

    /**
     * @tc.steps: step6. Check data.
     * @tc.expected: 2 data in the from deviceA are deleted and all data from deviceB are not deleted.
     */
    ExpectCount(db, "SELECT count(*) FROM " + DBConstant::RELATIONAL_PREFIX + g_tableName +
        "_log WHERE flag&0x01=0x01;", 2U);  // 2 deleted log
    ExpectCount(db, "SELECT count(*) FROM " + DBConstant::RELATIONAL_PREFIX + g_tableName + "_" +
        DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceA)) + ";", 3U);  // 3 records in A
    ExpectCount(db, "SELECT count(*) FROM " + DBConstant::RELATIONAL_PREFIX + g_tableName + "_" +
        DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceB)) + ";", RECORD_COUNT);  // 5 records in B

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
    const size_t RECORD_COUNT = 100; // 100 records.
    for (size_t i = 0; i < RECORD_COUNT; ++i) {
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
    const size_t RECORD_COUNT = 100; // 100 records.
    for (size_t i = 0; i < RECORD_COUNT; ++i) {
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
    QueryObject queryObj(query);
    queryObj.SetSchema(store->GetSchemaInfo());

    std::vector<SingleVerKvEntry *> entries;
    EXPECT_EQ(store->GetSyncData(queryObj, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    EXPECT_EQ(token, nullptr);
    size_t expectCount = 98;  // expect 98 records.
    EXPECT_EQ(entries.size(), expectCount);
    for (auto iter = entries.begin(); iter != entries.end(); ++iter) {
        auto nextOne = std::next(iter, 1);
        if (nextOne != entries.end()) {
            EXPECT_LT((*iter)->GetTimestamp(), (*nextOne)->GetTimestamp());
        }
    }
    SingleVerKvEntry::Release(entries);

    /**
     * @tc.steps: step3. Get record whose key is equal to 10 or value is equal to 20, order by key asc.
     * @tc.expected: Succeed, Get 98 records.
     */
    query = Query::Select(g_tableName).EqualTo("key", 10).Or().EqualTo("value", 20).OrderBy("key", true);
    queryObj = QueryObject(query);
    queryObj.SetSchema(store->GetSchemaInfo());

    EXPECT_EQ(store->GetSyncData(queryObj, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    EXPECT_EQ(token, nullptr);
    expectCount = 2;  // expect 2 records.
    EXPECT_EQ(entries.size(), expectCount);
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

    ExecSqlAndAssertOK(db, {"CREATE INDEX index1 ON " + g_tableName + "(value);",
                            "CREATE UNIQUE INDEX index2 ON " + g_tableName + "(value,key);"});

    /**
     * @tc.steps: step2. Create distributed table "dataPlus".
     * @tc.expected: Succeed, return OK.
     */
    const string tableName = g_tableName + "Plus";
    string sql = "CREATE TABLE " + tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegate->CreateDistributedTable(tableName), DBStatus::OK);

    /**
     * @tc.steps: step3. Put 5 records with different type into "dataPlus" table.
     * @tc.expected: Succeed, return OK.
     */
    const size_t RECORD_COUNT = 5;  // 5 sqls
    ExecSqlAndAssertOK(db, {"INSERT INTO " + tableName + " VALUES(NULL, 1);",
                            "INSERT INTO " + tableName + " VALUES(NULL, 0.01);",
                            "INSERT INTO " + tableName + " VALUES(NULL, NULL);",
                            "INSERT INTO " + tableName + " VALUES(NULL, 'This is a text.');",
                            "INSERT INTO " + tableName + " VALUES(NULL, x'0123456789');"});

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
    ExpectCount(db, "SELECT count(*) FROM " + tableName + " as a, " + DBConstant::RELATIONAL_PREFIX + g_tableName +
        "_" + DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceID)) + " as b "
        "WHERE a.key=b.key AND (a.value=b.value OR (a.value is NULL AND b.value is NULL));", RECORD_COUNT);

    /**
     * @tc.steps: step7. Check index.
     * @tc.expected: 2 index for deviceA's data table exists.
     */
    ExpectCount(db,
        "SELECT count(*) FROM sqlite_master WHERE type='index' AND tbl_name='" + DBConstant::RELATIONAL_PREFIX +
        g_tableName + "_" + DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceID)) + "'", 2U); // 2 index
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

    sqlite3_close(db);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: MissQuery1
 * @tc.desc: Check REMOTE_DEVICE_DATA_MISS_QUERY flag succeed.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalGetDataTest, MissQuery1, TestSize.Level1)
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
    ExecSqlAndAssertOK(db, {"INSERT INTO " + tableName + " VALUES(NULL, 1);",
        "INSERT INTO " + tableName + " VALUES(NULL, 2);", "INSERT INTO " + tableName + " VALUES(NULL, 3);",
        "INSERT INTO " + tableName + " VALUES(NULL, 4);", "INSERT INTO " + tableName + " VALUES(NULL, 5);"});

    /**
     * @tc.steps: step3. Get all data from "dataPlus" table.
     * @tc.expected: Succeed and the count is right.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    ContinueToken token = nullptr;
    SyncTimeRange timeRange;
    QueryObject query(Query::Select(tableName).EqualTo("value", 2).Or().EqualTo("value", 3).Or().EqualTo("value", 4));
    std::vector<SingleVerKvEntry *> entries;
    EXPECT_EQ(store->GetSyncData(query, timeRange, DataSizeSpecInfo {}, token, entries), E_OK);
    timeRange.lastQueryTime = (*(entries.rbegin()))->GetTimestamp();
    EXPECT_EQ(entries.size(), 3U);  // 3 for test

    /**
     * @tc.steps: step4. Put data into "data" table from deviceA for 10 times.
     * @tc.expected: Succeed, return OK.
     */
    query = QueryObject(Query::Select(g_tableName));
    const DeviceID deviceID = "deviceA";
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(db, store->GetSchemaInfo().GetTable(g_tableName),
        DBCommon::GetDistributedTableName(deviceID, g_tableName)));
    EXPECT_EQ(const_cast<RelationalSyncAbleStorage *>(store)->PutSyncDataWithQuery(query, entries, deviceID), E_OK);
    SingleVerKvEntry::Release(entries);

    /**
     * @tc.steps: step5. Check data.
     * @tc.expected: There is 3 data in table.
     */
    std::string getDataSql = "SELECT count(*) FROM " + DBConstant::RELATIONAL_PREFIX + g_tableName + "_" +
        DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceID)) + ";";
    ExpectCount(db, getDataSql, 3);  // 2,3,4

    std::string getLogSql = "SELECT count(*) FROM " + DBConstant::RELATIONAL_PREFIX + g_tableName + "_log;";
    ExpectCount(db, getLogSql, 3);  // 2,3,4

    /**
     * @tc.steps: step6. Update data. k2v2 to k2v102, k3v3 to k3v103, k4v4 to k4v104.
     * @tc.expected: Update succeed.
     */
    ExecSqlAndAssertOK(db, {"INSERT OR REPLACE INTO " + tableName + " VALUES(2, 102);",
                            "UPDATE " + tableName + " SET value=103 WHERE value=3;",
                            "DELETE FROM " + tableName + " WHERE key=4;",
                            "INSERT INTO " + tableName + " VALUES(4, 104);"});

    /**
     * @tc.steps: step7. Get all data from "dataPlus" table.
     * @tc.expected: Succeed and the count is right.
     */
    query = QueryObject(Query::Select(tableName).EqualTo("value", 2).Or().EqualTo("value", 3).Or().EqualTo("value", 4));
    EXPECT_EQ(store->GetSyncData(query, timeRange, DataSizeSpecInfo {}, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 3U);  // 3 miss query data.

    /**
     * @tc.steps: step8. Put data into "data" table from deviceA for 10 times.
     * @tc.expected: Succeed, return OK.
     */
    query = QueryObject(Query::Select(g_tableName));
    EXPECT_EQ(const_cast<RelationalSyncAbleStorage *>(store)->PutSyncDataWithQuery(query, entries, deviceID), E_OK);
    SingleVerKvEntry::Release(entries);

    /**
     * @tc.steps: step9. Check data.
     * @tc.expected: There is 0 data in table.
     */
    ExpectCount(db, getDataSql, 0U);  // 0 data exists
    ExpectCount(db, getLogSql, 0U);  // 0 data exists

    sqlite3_close(db);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: CompatibleData1
 * @tc.desc: Check compatibility.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalGetDataTest, CompatibleData1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);
    /**
     * @tc.steps: step1. Create distributed table "dataPlus".
     * @tc.expected: Succeed, return OK.
     */
    const string tableName = g_tableName + "Plus";
    std::string sql = "CREATE TABLE " + tableName + "(key INTEGER, value INTEGER NOT NULL, \
        extra_field TEXT NOT NULL DEFAULT 'default_value');";
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegate->CreateDistributedTable(tableName), DBStatus::OK);
    /**
     * @tc.steps: step2. Put 1 record into data and dataPlus table.
     * @tc.expected: Succeed, return OK.
     */
    ASSERT_EQ(AddOrUpdateRecord(1, 101), E_OK);
    sql = "INSERT INTO " + tableName + " VALUES(2, 102, 'f3');"; // k2v102
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    /**
     * @tc.steps: step3. Get all data from "data" table.
     * @tc.expected: Succeed and the count is right.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    ContinueToken token = nullptr;
    QueryObject query(Query::Select(g_tableName));
    std::vector<SingleVerKvEntry *> entries;
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 1UL);
    /**
     * @tc.steps: step4. Put data into "data_plus" table from deviceA.
     * @tc.expected: Succeed, return OK.
     */
    query = QueryObject(Query::Select(tableName));
    const DeviceID deviceID = "deviceA";
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(db, store->GetSchemaInfo().GetTable(tableName),
        DBCommon::GetDistributedTableName(deviceID, tableName)));
    EXPECT_EQ(const_cast<RelationalSyncAbleStorage *>(store)->PutSyncDataWithQuery(query, entries, deviceID), E_OK);
    SingleVerKvEntry::Release(entries);
    /**
     * @tc.steps: step4. Get all data from "dataPlus" table.
     * @tc.expected: Succeed and the count is right.
     */
    query = QueryObject(Query::Select(tableName));
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 1UL);
    /**
     * @tc.steps: step5. Put data into "data" table from deviceA.
     * @tc.expected: Succeed, return OK.
     */
    query = QueryObject(Query::Select(g_tableName));
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(db, store->GetSchemaInfo().GetTable(g_tableName),
        DBCommon::GetDistributedTableName(deviceID, g_tableName)));
    EXPECT_EQ(const_cast<RelationalSyncAbleStorage *>(store)->PutSyncDataWithQuery(query, entries, deviceID), E_OK);
    SingleVerKvEntry::Release(entries);
    /**
     * @tc.steps: step6. Check data.
     * @tc.expected: All data in the two tables are same.
     */
    sql = "SELECT count(*) FROM " + g_tableName + " as a," + DBConstant::RELATIONAL_PREFIX + tableName + "_" +
        DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceID)) + " as b " +
        "WHERE a.key=b.key AND a.value=b.value;";
    size_t count = 0;
    EXPECT_EQ(GetCount(db, sql, count), E_OK);
    EXPECT_EQ(count, 1UL);
    sql = "SELECT count(*) FROM " + tableName + " as a," + DBConstant::RELATIONAL_PREFIX + g_tableName + "_" +
        DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceID)) + " as b " +
        "WHERE a.key=b.key AND a.value=b.value;";
    count = 0;
    EXPECT_EQ(GetCount(db, sql, count), E_OK);
    EXPECT_EQ(count, 1UL);
    sqlite3_close(db);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: GetDataSortByTime1
 * @tc.desc: All query get data sort by time asc.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalGetDataTest, GetDataSortByTime1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);
    /**
     * @tc.steps: step2. Add 3 record into data. k1v105, k2v104, k3v103, timestamp desc.
     * @tc.expected: Succeed, return OK.
     */
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    std::string sql = "INSERT INTO " + g_tableName + " VALUES(1, 101);"; // k1v101
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    sql = "INSERT INTO " + g_tableName + " VALUES(2, 102);"; // k2v102
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    sql = "INSERT INTO " + g_tableName + " VALUES(3, 103);"; // k3v103
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    sql = "UPDATE " + g_tableName + " SET value=104 WHERE key=2;"; // k2v104
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    sql = "UPDATE " + g_tableName + " SET value=105 WHERE key=1;"; // k1v105
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    /**
     * @tc.steps: step3. Get all data from "data" table by all query.
     * @tc.expected: Succeed and the count is right.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    ContinueToken token = nullptr;
    QueryObject query(Query::Select(g_tableName));
    std::vector<SingleVerKvEntry *> entries;
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    ExpectMissQueryCnt(entries, 3UL);  // 3 data
    SingleVerKvEntry::Release(entries);

    query = QueryObject(Query::Select(g_tableName).EqualTo("key", 1).Or().EqualTo("key", 3));
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    ExpectMissQueryCnt(entries, 2UL);  // 2 data
    SingleVerKvEntry::Release(entries);

    query = QueryObject(Query::Select(g_tableName).OrderBy("key", false));
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    ExpectMissQueryCnt(entries, 3UL);  // 3 data
    SingleVerKvEntry::Release(entries);

    query = QueryObject(Query::Select(g_tableName).OrderBy("value", false));
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    ExpectMissQueryCnt(entries, 3UL);  // 3 data
    SingleVerKvEntry::Release(entries);

    query = QueryObject(Query::Select(g_tableName).Limit(2));
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    ExpectMissQueryCnt(entries, 2UL);  // 2 data
    SingleVerKvEntry::Release(entries);

    sqlite3_close(db);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: SameFieldWithLogTable1
 * @tc.desc: Get query data OK when the table has same field with log table.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalGetDataTest, SameFieldWithLogTable1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    /**
     * @tc.steps: step1. Create distributed table "dataPlus".
     * @tc.expected: Succeed, return OK.
     */
    const string tableName = g_tableName + "Plus";
    std::string sql = "CREATE TABLE " + tableName + "(key INTEGER, flag INTEGER NOT NULL, \
        device TEXT NOT NULL DEFAULT 'default_value');";
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegate->CreateDistributedTable(tableName), DBStatus::OK);
    /**
     * @tc.steps: step2. Put 1 record into dataPlus table.
     * @tc.expected: Succeed, return OK.
     */
    sql = "INSERT INTO " + tableName + " VALUES(1, 101, 'f3');"; // k1v101
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    /**
     * @tc.steps: step3. Get all data from dataPlus table.
     * @tc.expected: Succeed and the count is right.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    ContinueToken token = nullptr;
    QueryObject query(Query::Select(tableName).EqualTo("flag", 101).OrderBy("device", false));
    std::vector<SingleVerKvEntry *> entries;
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 1UL);
    SingleVerKvEntry::Release(entries);
    sqlite3_close(db);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: CompatibleData2
 * @tc.desc: Check compatibility.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalGetDataTest, CompatibleData2, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);

    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);

    /**
     * @tc.steps: step1. Create distributed table from deviceA.
     * @tc.expected: Succeed, return OK.
     */
    const DeviceID deviceID = "deviceA";
    ASSERT_EQ(E_OK, SQLiteUtils::CreateSameStuTable(db, store->GetSchemaInfo().GetTable(g_tableName),
        DBCommon::GetDistributedTableName(deviceID, g_tableName)));

    /**
     * @tc.steps: step2. Alter "data" table and create distributed table again.
     * @tc.expected: Succeed.
     */
    std::string sql = "ALTER TABLE " + g_tableName + " ADD COLUMN integer_type INTEGER DEFAULT 123 not null;"
        "ALTER TABLE " + g_tableName + " ADD COLUMN text_type TEXT DEFAULT 'high_version' not null;"
        "ALTER TABLE " + g_tableName + " ADD COLUMN real_type REAL DEFAULT 123.123456 not null;"
        "ALTER TABLE " + g_tableName + " ADD COLUMN blob_type BLOB DEFAULT 123 not null;";
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step3. Check deviceA's distributed table.
     * @tc.expected: The create sql is correct.
     */
    std::string expectSql = "CREATE TABLE naturalbase_rdb_aux_data_"
        "265a9c8c3c690cdfdac72acfe7a50f748811802635d987bb7d69dc602ed3794f(key integer NOT NULL PRIMARY KEY,"
        "value integer, integer_type integer NOT NULL DEFAULT 123, text_type text NOT NULL DEFAULT 'high_version', "
        "real_type real NOT NULL DEFAULT 123.123456, blob_type blob NOT NULL DEFAULT 123)";
    sql = "SELECT sql FROM sqlite_master WHERE tbl_name='" + DBConstant::RELATIONAL_PREFIX + g_tableName + "_" +
        DBCommon::TransferStringToHex(DBCommon::TransferHashString(deviceID)) + "';";
    EXPECT_EQ(GetOneText(db, sql), expectSql);

    sqlite3_close(db);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: PutSyncDataConflictDataTest001
 * @tc.desc: Check put with conflict sync data.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalGetDataTest, PutSyncDataConflictDataTest001, TestSize.Level1)
{
    const DeviceID deviceID_A = "deviceA";
    const DeviceID deviceID_B = "deviceB";
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_storePath);
    RelationalTestUtils::CreateDeviceTable(db, g_tableName, deviceID_B);

    DBStatus status = g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate);
    EXPECT_EQ(status, DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    EXPECT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    auto store = const_cast<RelationalSyncAbleStorage *>(GetRelationalStore());
    ASSERT_NE(store, nullptr);

    RelationalTestUtils::ExecSql(db, "INSERT OR REPLACE INTO " + g_tableName + " (key,value) VALUES (1001,'VAL_1');");
    RelationalTestUtils::ExecSql(db, "INSERT OR REPLACE INTO " + g_tableName + " (key,value) VALUES (1002,'VAL_2');");
    RelationalTestUtils::ExecSql(db, "INSERT OR REPLACE INTO " + g_tableName + " (key,value) VALUES (1003,'VAL_3');");

    DataSizeSpecInfo sizeInfo {MTU_SIZE, 50};
    ContinueToken token = nullptr;
    QueryObject query(Query::Select(g_tableName));
    std::vector<SingleVerKvEntry *> entries;
    int errCode = store->GetSyncData(query, {}, sizeInfo, token, entries);
    EXPECT_EQ(errCode, E_OK);

    errCode = store->PutSyncDataWithQuery(query, entries, deviceID_B);
    EXPECT_EQ(errCode, E_OK);
    GenericSingleVerKvEntry::Release(entries);

    QueryObject query2(Query::Select(g_tableName).EqualTo("key", 1001));
    std::vector<SingleVerKvEntry *> entries2;
    store->GetSyncData(query2, {}, sizeInfo, token, entries2);

    errCode = store->PutSyncDataWithQuery(query, entries2, deviceID_B);
    EXPECT_EQ(errCode, E_OK);
    GenericSingleVerKvEntry::Release(entries2);

    RefObject::DecObjRef(g_store);

    std::string deviceTable = DBCommon::GetDistributedTableName(deviceID_B, g_tableName);
    EXPECT_EQ(RelationalTestUtils::CheckTableRecords(db, deviceTable), 3);
    sqlite3_close_v2(db);
}

/**
 * @tc.name: SaveNonexistDevdata1
 * @tc.desc: Save non-exist device data and check errCode.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalGetDataTest, SaveNonexistDevdata1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);
    /**
     * @tc.steps: step1. Create distributed table "dataPlus".
     * @tc.expected: Succeed, return OK.
     */
    const string tableName = g_tableName + "Plus";
    std::string sql = "CREATE TABLE " + tableName + "(key INTEGER, value INTEGER NOT NULL, \
        extra_field TEXT NOT NULL DEFAULT 'default_value');";
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(g_delegate->CreateDistributedTable(tableName), DBStatus::OK);
    /**
     * @tc.steps: step2. Put 1 record into data table.
     * @tc.expected: Succeed, return OK.
     */
    ASSERT_EQ(AddOrUpdateRecord(1, 101), E_OK);

    /**
     * @tc.steps: step3. Get all data from "data" table.
     * @tc.expected: Succeed and the count is right.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    ContinueToken token = nullptr;
    QueryObject query(Query::Select(g_tableName));
    std::vector<SingleVerKvEntry *> entries;
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 1UL);

    /**
     * @tc.steps: step4. Put data into "data_plus" table from deviceA and deviceA does not exist.
     * @tc.expected: Succeed, return OK.
     */
    query = QueryObject(Query::Select(tableName));
    const DeviceID deviceID = "deviceA";
    EXPECT_EQ(const_cast<RelationalSyncAbleStorage *>(store)->PutSyncDataWithQuery(query, entries, deviceID),
        -1);  // -1 means error
    SingleVerKvEntry::Release(entries);

    sqlite3_close(db);
    RefObject::DecObjRef(g_store);
}
#endif
