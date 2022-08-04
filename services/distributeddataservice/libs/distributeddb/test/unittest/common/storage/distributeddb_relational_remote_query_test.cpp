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

#include "db_common.h"
#include "db_errno.h"
#include "db_types.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "kvdb_properties.h"
#include "log_print.h"
#include "prepared_stmt.h"
#include "relational_row_data_set.h"
#include "relational_store_delegate.h"
#include "relational_store_instance.h"
#include "relational_store_manager.h"
#include "relational_store_sqlite_ext.h"
#include "relational_sync_able_storage.h"
#include "sqlite_relational_store.h"
#include "sqlite_utils.h"
#include "virtual_communicator_aggregator.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
string g_testDir;
string g_storePath;
string g_storeID = "dftStoreID";
string g_tableName { "data" };
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

int ExecSql(sqlite3 *db, const std::string &sql)
{
    return sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
}

int InsertData(sqlite3 *db, size_t byteSize)
{
    static const std::string sql = "INSERT OR REPLACE INTO " + g_tableName + " VALUES(?,?);";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        return errCode;
    }
    std::vector<uint8_t> value(byteSize);
    errCode = SQLiteUtils::BindBlobToStatement(stmt, 2, value, false);  // 2 means value's index.
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}
}

class DistributedDBRelationalRemoteQueryTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void DistributedDBRelationalRemoteQueryTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_storePath = g_testDir + "/relationalRemoteQueryTest.db";
    LOGI("The test db is:%s", g_testDir.c_str());

    auto communicator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(communicator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(communicator);
}

void DistributedDBRelationalRemoteQueryTest::TearDownTestCase(void)
{
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void DistributedDBRelationalRemoteQueryTest::SetUp(void)
{
    g_tableName = "data";
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    CreateDBAndTable();
}

void DistributedDBRelationalRemoteQueryTest::TearDown(void)
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
 * @tc.name: NormalQuery1
 * @tc.desc: Normal query.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalRemoteQueryTest, NormalQuery1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step1. Put data.
     * @tc.expected: Succeed, return OK.
     */
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    ASSERT_NE(db, nullptr);

    EXPECT_EQ(ExecSql(db, "INSERT INTO " + g_tableName + " values(1, 1);"), 0);
    sqlite3_close(db);

    /**
     * @tc.steps: step2. Query with QUERY opcode and a select sql.
     * @tc.expected: Query successfully.
     */
    auto store = GetRelationalStore();

    ASSERT_NE(store, nullptr);
    PreparedStmt prepStmt(PreparedStmt::QUERY, "SELECT * FROM " + g_tableName, {});
    RelationalRowDataSet rowDataSet {};
    ContinueToken token = nullptr;
    EXPECT_EQ(store->ExecuteQuery(prepStmt, DBConstant::MAX_MTU_SIZE, rowDataSet, token), E_OK);
    EXPECT_EQ(rowDataSet.GetSize(), 1);
    EXPECT_EQ(token, nullptr);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: BoolQuery1
 * @tc.desc: Query bool.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalRemoteQueryTest, BoolQuery1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step1. Put data.
     * @tc.expected: Succeed, return OK.
     */
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    ASSERT_NE(db, nullptr);

    EXPECT_EQ(ExecSql(db, "INSERT INTO " + g_tableName + " values(1, 1);"), 0);
    EXPECT_EQ(ExecSql(db, "INSERT INTO " + g_tableName + " values(2, '1');"), 0);
    EXPECT_EQ(ExecSql(db, "INSERT INTO " + g_tableName + " values(3, true);"), 0);
    sqlite3_close(db);

    /**
     * @tc.steps: step2. Check bool.
     * @tc.expected: '1' and 1 and true in sql is OK. 1 in bindArgs is OK.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);

    RelationalRowDataSet rowDataSet {};
    ContinueToken token = nullptr;

    PreparedStmt prepStmt(PreparedStmt::QUERY, "SELECT * FROM " + g_tableName + " WHERE value=?", {"1"});
    EXPECT_EQ(store->ExecuteQuery(prepStmt, DBConstant::MAX_MTU_SIZE, rowDataSet, token), E_OK);
    EXPECT_EQ(rowDataSet.GetSize(), 3);

    prepStmt = { PreparedStmt::QUERY, "SELECT * FROM " + g_tableName + " WHERE value='1'", {} };
    EXPECT_EQ(store->ExecuteQuery(prepStmt, DBConstant::MAX_MTU_SIZE, rowDataSet, token), E_OK);
    EXPECT_EQ(rowDataSet.GetSize(), 3);

    prepStmt = { PreparedStmt::QUERY, "SELECT * FROM " + g_tableName + " WHERE value=1", {} };
    EXPECT_EQ(store->ExecuteQuery(prepStmt, DBConstant::MAX_MTU_SIZE, rowDataSet, token), E_OK);
    EXPECT_EQ(rowDataSet.GetSize(), 3);

    prepStmt = { PreparedStmt::QUERY, "SELECT * FROM " + g_tableName + " WHERE value=true", {} };
    EXPECT_EQ(store->ExecuteQuery(prepStmt, DBConstant::MAX_MTU_SIZE, rowDataSet, token), E_OK);
    EXPECT_EQ(rowDataSet.GetSize(), 3);

    prepStmt = { PreparedStmt::QUERY, "SELECT * FROM " + g_tableName + " WHERE value=?", {"'1'"} };
    EXPECT_EQ(store->ExecuteQuery(prepStmt, DBConstant::MAX_MTU_SIZE, rowDataSet, token), E_OK);
    EXPECT_EQ(rowDataSet.GetSize(), 0);

    prepStmt = { PreparedStmt::QUERY, "SELECT * FROM " + g_tableName + " WHERE value='true'", {} };
    EXPECT_EQ(store->ExecuteQuery(prepStmt, DBConstant::MAX_MTU_SIZE, rowDataSet, token), E_OK);
    EXPECT_EQ(rowDataSet.GetSize(), 0);

    prepStmt = { PreparedStmt::QUERY, "SELECT * FROM " + g_tableName + " WHERE value=?", {"true"} };
    EXPECT_EQ(store->ExecuteQuery(prepStmt, DBConstant::MAX_MTU_SIZE, rowDataSet, token), E_OK);
    EXPECT_EQ(rowDataSet.GetSize(), 0);

    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: BlobQuery1
 * @tc.desc: Query blob.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalRemoteQueryTest, BlobQuery1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step1. Put data.
     * @tc.expected: Succeed, return OK.
     */
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    ASSERT_NE(db, nullptr);

    EXPECT_EQ(ExecSql(db, "INSERT INTO " + g_tableName + " values(1, x'10101010001000');"), 0);
    sqlite3_close(db);

    /**
     * @tc.steps: step2. Check blob.
     * @tc.expected: x'xxx' in sql is OK. Not ok in bindArgs.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);

    RelationalRowDataSet rowDataSet {};
    ContinueToken token = nullptr;

    PreparedStmt prepStmt(PreparedStmt::QUERY, "SELECT * FROM " + g_tableName + " WHERE value=x'10101010001000'", {});
    EXPECT_EQ(store->ExecuteQuery(prepStmt, DBConstant::MAX_MTU_SIZE, rowDataSet, token), E_OK);
    EXPECT_EQ(rowDataSet.GetSize(), 1);
    EXPECT_FALSE(rowDataSet.GetColNames().empty());

    prepStmt = { PreparedStmt::QUERY, "SELECT * FROM " + g_tableName + " WHERE value=?", {"x'10101010001000'"} };
    EXPECT_EQ(store->ExecuteQuery(prepStmt, DBConstant::MAX_MTU_SIZE, rowDataSet, token), E_OK);
    EXPECT_EQ(rowDataSet.GetSize(), 0);
    EXPECT_TRUE(rowDataSet.GetColNames().empty());

    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: InsertQuery1
 * @tc.desc: Query with insert statement.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalRemoteQueryTest, InsertQuery1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step1. Put data.
     * @tc.expected: Succeed, return OK.
     */
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    ASSERT_NE(db, nullptr);

    EXPECT_EQ(ExecSql(db, "INSERT INTO " + g_tableName + " values(1, x'10101010001000');"), 0);
    sqlite3_close(db);

    /**
     * @tc.steps: step2. Query with a insert statement.
     * @tc.expected: Query failed.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);

    RelationalRowDataSet rowDataSet {};
    ContinueToken token = nullptr;

    /**
     * @tc.steps: step3. Query with INSERT opcode.
     * @tc.expected: Query failed. Return E_INVALID_ARGS.
     */
    PreparedStmt prepStmt(PreparedStmt::INSERT, "INSERT INTO " + g_tableName + " values(2, x'10101010001000');", {});
    EXPECT_EQ(store->ExecuteQuery(prepStmt, DBConstant::MAX_MTU_SIZE, rowDataSet, token), -E_INVALID_ARGS);
    EXPECT_EQ(rowDataSet.GetSize(), 0);

    /**
     * @tc.steps: step4. Query with QUERY opcode and a insert sql.
     * @tc.expected: Query failed. Return E_DENIED_SQL.
     */
    prepStmt = { PreparedStmt::QUERY, "INSERT INTO " + g_tableName + " values(2, x'10101010001000');", {} };
    EXPECT_EQ(store->ExecuteQuery(prepStmt, DBConstant::MAX_MTU_SIZE, rowDataSet, token), -E_DENIED_SQL);
    EXPECT_EQ(rowDataSet.GetSize(), 0);

    /**
     * @tc.steps: step5. Query with QUERY opcode and two sql(one select and one insert).
     * @tc.expected: Query OK. The second sql is ignored.
     */
    prepStmt = { PreparedStmt::QUERY, "SELECT * FROM " + g_tableName + " WHERE value=x'10101010001000';"
                                      "INSERT INTO " + g_tableName + " values(2, x'10101010001000');", {} };
    EXPECT_EQ(store->ExecuteQuery(prepStmt, DBConstant::MAX_MTU_SIZE, rowDataSet, token), E_OK);
    EXPECT_EQ(rowDataSet.GetSize(), 1);

    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: NonexistentTable1
 * @tc.desc: Nonexistent table or column.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalRemoteQueryTest, NonexistentTable1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step1. Query when nonexistent table or nonexistent column.
     * @tc.expected: Failed, return SQLITE_ERROR
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);

    RelationalRowDataSet rowDataSet {};
    ContinueToken token = nullptr;

    PreparedStmt prepStmt(PreparedStmt::QUERY, "SELECT * FROM nonexistent_table WHERE value=1;", {});
    EXPECT_EQ(store->ExecuteQuery(prepStmt, DBConstant::MAX_MTU_SIZE, rowDataSet, token), -SQLITE_ERROR);

    prepStmt = { PreparedStmt::QUERY, "SELECT * FROM " + g_tableName + " WHERE nonexistent_column=1;", {} };
    EXPECT_EQ(store->ExecuteQuery(prepStmt, DBConstant::MAX_MTU_SIZE, rowDataSet, token), -SQLITE_ERROR);

    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: PreparedStmtSerialize
 * @tc.desc: Test the serialization and deserialization of PreparedStmt.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalRemoteQueryTest, PreparedStmtSerialize, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create a prepared stmt;
     * @tc.expected: OK.
     */
    PreparedStmt prepStmt1(PreparedStmt::QUERY, "SELECT * FROM test WHERE value=? or value=?", {"1", "2"});
    int len = prepStmt1.CalcLength();

    /**
     * @tc.steps: step2. Serialize the prepared stmt;
     * @tc.expected: Serialize OK.
     */
    std::vector<uint8_t> buffer(len);
    Parcel parcel1(buffer.data(), buffer.size());
    ASSERT_EQ(prepStmt1.Serialize(parcel1), E_OK);
    ASSERT_FALSE(parcel1.IsError());

    /**
     * @tc.steps: step3. Deserialize the prepared stmt;
     * @tc.expected: Deserialize OK.
     */
    PreparedStmt prepStmt2;
    Parcel parcel2(buffer.data(), buffer.size());
    ASSERT_EQ(prepStmt2.DeSerialize(parcel2), E_OK);
    ASSERT_FALSE(parcel2.IsError());

    /**
     * @tc.steps: step4. Compare the deserialized prepared stmt with the original;
     * @tc.expected: Compare OK. They are the same.
     */
    EXPECT_EQ(prepStmt1.GetOpCode(), prepStmt2.GetOpCode());
    EXPECT_EQ(prepStmt1.GetSql(), prepStmt2.GetSql());
    EXPECT_EQ(prepStmt1.GetBindArgs(), prepStmt2.GetBindArgs());
}

/**
 * @tc.name: ComplexQuery1
 * @tc.desc: Complex query with join or aggregate func.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalRemoteQueryTest, ComplexQuery1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    ASSERT_NE(db, nullptr);

    EXPECT_EQ(ExecSql(db, "INSERT INTO " + g_tableName + " values(1, 1);"
                          "CREATE TABLE IF NOT EXISTS r(key INTEGER, value INTEGER);"
                          "INSERT INTO r VALUES(1, 1);"), 0);
    sqlite3_close(db);

    /**
     * @tc.steps: step1. Query with a complex sql.
     * @tc.expected: Succeed.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);

    RelationalRowDataSet rowDataSet {};
    ContinueToken token = nullptr;

    PreparedStmt prepStmt(PreparedStmt::QUERY,
        "SELECT COUNT(*) FROM " + g_tableName + " INNER JOIN r ON " + g_tableName + ".key=r.key "
        "WHERE r.value IN (1,2,3);", {});
    EXPECT_EQ(store->ExecuteQuery(prepStmt, DBConstant::MAX_MTU_SIZE, rowDataSet, token), E_OK);
    EXPECT_EQ(rowDataSet.GetSize(), 1);

    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: LargeAmountOfData1
 * @tc.desc: Large Amount Of Data.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalRemoteQueryTest, LargeAmountOfData1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    ASSERT_NE(db, nullptr);

    /**
     * @tc.steps: step1. Insert 20 data.
     * @tc.expected: Succeed.
     */
    int i = 10;  // 10 for test.
    while (i-- > 0) {
        ASSERT_EQ(InsertData(db, 500), 0);  // 500 for test.
        ASSERT_EQ(InsertData(db, 1500), 0); // 1500 for test.
    }
    sqlite3_close(db);

    /**
     * @tc.steps: step2. Query.
     * @tc.expected: Get 20 data.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);

    ContinueToken token = nullptr;
    PreparedStmt prepStmt(PreparedStmt::QUERY, "SELECT * FROM " + g_tableName, {});

    size_t totalCnt = 0;
    RelationalRowDataSet allRowDataSet {};
    do {
        RelationalRowDataSet rowDataSet {};
        EXPECT_EQ(store->ExecuteQuery(prepStmt, 1024, rowDataSet, token), E_OK);  // 1024 is min mtu.
        totalCnt += rowDataSet.GetSize();
        EXPECT_EQ(allRowDataSet.Merge(std::move(rowDataSet)), E_OK);
    } while (token != nullptr);

    EXPECT_EQ(totalCnt, 20u);
    EXPECT_EQ(allRowDataSet.GetSize(), 20);

    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: OverSize1
 * @tc.desc: Over size.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalRemoteQueryTest, OverSize1, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    ASSERT_NE(db, nullptr);

    /**
     * @tc.steps: step1. Insert over 4M data.
     * @tc.expected: Succeed.
     */
    int i = 4;  // 4MB
    while (i-- > 0) {
        ASSERT_EQ(InsertData(db, 1024 * 1024), 0);  // 1024*1024 means 1MB.
    }
    sqlite3_close(db);

    /**
     * @tc.steps: step2. Query.
     * @tc.expected: Failed, return E_REMOTE_OVER_SIZE.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);

    ContinueToken token = nullptr;
    PreparedStmt prepStmt(PreparedStmt::QUERY, "SELECT * FROM " + g_tableName, {});

    RelationalRowDataSet rowDataSet {};
    EXPECT_EQ(store->ExecuteQuery(prepStmt, 1200, rowDataSet, token), -E_REMOTE_OVER_SIZE);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: RemoteQueryForNotDistributedDb
 * @tc.desc: Not create distributed table, exec remote query.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalRemoteQueryTest, RemoteQueryForNotDistributedDb, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, RelationalStoreDelegate::Option {}, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);

    std::shared_ptr<ResultSet> result = nullptr;
    EXPECT_EQ(g_delegate->RemoteQuery("deviceA", RemoteCondition {}, 1000, result), DBStatus::NOT_SUPPORT);
}
#endif
