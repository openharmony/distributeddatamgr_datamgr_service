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
#include <string>

#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "distributed_rdb_tools.h"

using namespace std;
using namespace testing;
#if defined TESTCASES_USING_GTEST_EXT
using namespace testing::ext;
#endif
using namespace DistributedDB;
using namespace DistributedDBDataGenerator;

// set sqlite3 rdb_A and create 33 dataTables

namespace DistributedRelatinalDBExceptionOperation {
sqlite3 *gdb = nullptr;

RelatetionalStoreManager *g_manager = nullptr;

RelatetionalStoreDelegate *g_delegate = nullptr;

RelatetionalStoreDelegate *g_delegate1 = nullptr;

class DistributedRelatinalDBExceptionOperationTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
private:
};

void DistributedRelatinalDBExceptionOperationTest::SetUpTestCase(void)
{
    bool init1 = InitSqlite3Store(gdb, g_rdbParameter1);
    ASSERT_EQ(init1, true);
    bool init2 = InitTableDataAndTRIGGER(gdb);
    ASSERT_EQ(init2, true);
    g_manager = new (std::nothrow) RelatetionalStoreManager(g_rdbParameter1.appId, g_rdbParameter1.userId);
    DBStatus status1 = DistributedRdbTestTools::GetOpenStoreStatus(g_manager, g_delegate, g_rdbParameter1);
    ASSERT_EQ(status1, DBStatus::OK);
}

void DistributedRelatinalDBExceptionOperationTest::TearDownTestCase(void)
{
    CloseSqlite3Store(gdb);
}

void DistributedRelatinalDBExceptionOperationTest::SetUp(void)
{
    UnitTest *test = UnitTest::GetInstance();
    ASSERT_NE(test, nullptr);
    const TestInfo *testinfo = test->current_test_info();
    ASSERT_NE(testinfo, nullptr);
    string testCaseName = string(testinfo->name());
    MST_LOG("[SetUp] test case %s is start to run", testCaseName.c_str());
}

void DistributedRelatinalDBExceptionOperationTest::TearDown(void)
{
    MST_LOG("TearDownTestCase after case.");
}

/**
 * @tc.name:  dbPathException001
 * @tc.desc: db path does not exist, openStore failed.
 * @tc.type: FUNC
 * @tc.require: SR000DORPP
 * @tc.author: xuhongkang
 */
HWTEST_F(DistributedRelatinalDBExceptionOperationTest, dbPathException001, TestSize.Level4)
{
    DBStatus status = DistributedRdbTestTools::GetOpenStoreStatus(g_manager, g_delegate1, g_rdbParameter6);
    ASSERT_EQ(status, DBStatus::INVALID_ARGS);
}

/**
 * @tc.name:  dbPathException002
 * @tc.desc: db path unreadable, openStore failed.
 * @tc.type: FUNC
 * @tc.require: SR000DORPP
 * @tc.author: xuhongkang
 */
HWTEST_F(DistributedRelatinalDBExceptionOperationTest, dbPathException002, TestSize.Level4)
{
    sqlite3 *db = nullptr;
    bool init1 = InitSqlite3Store(db, g_rdbParameter7);
    ASSERT_EQ(init1, true);
    bool bpermission = DBStributedDB::OS::SetFilePermissions(g_rdbParameter7.path, S_IWUSR | S_IXUSR);
    ASSERT_EQ(bpermission, true);
    DBStatus status = DistributedRdbTestTools::GetOpenStoreStatus(g_manager, g_delegate1, g_rdbParameter7);
    ASSERT_EQ(status, DBStatus::INVALID_ARGS);
    bpermission = DBStributedDB::OS::SetFilePermissions(g_rdbParameter7.path, S_IWUSR | S_IREAD | S_IXUSR);
    ASSERT_EQ(bpermission, true);
    CloseSqlite3Store(db);
}

/**
 * @tc.name:  dbPathException003
 * @tc.desc: db path unwritdable, openStore failed.
 * @tc.type: FUNC
 * @tc.require: SR000DORPP
 * @tc.author: xuhongkang
 */
HWTEST_F(DistributedRelatinalDBExceptionOperationTest, dbPathException003, TestSize.Level4)
{
    sqlite3 *db = nullptr;
    bool init1 = InitSqlite3Store(db, g_rdbParameter8);
    ASSERT_EQ(init1, true);
    bool bpermission = DBStributedDB::OS::SetFilePermissions(g_rdbParameter8.path, S_IREAD  | S_IXUSR);
    ASSERT_EQ(bpermission, true);
    DBStatus status = DistributedRdbTestTools::GetOpenStoreStatus(g_manager, g_delegate1, g_rdbParameter8);
    ASSERT_EQ(status, DBStatus::INVALID_ARGS);
    bpermission = DBStributedDB::OS::SetFilePermissions(g_rdbParameter8.path, S_IWUSR | S_IREAD | S_IXUSR);
    ASSERT_EQ(bpermission, true);
    CloseSqlite3Store(db);
}

/**
 * @tc.name:  storeIdException001
 * @tc.desc: storeId is empty, openStore failed.
 * @tc.type: FUNC
 * @tc.require: SR000DORPP
 * @tc.author: xuhongkang
 */
HWTEST_F(DistributedRelatinalDBExceptionOperationTest, storeIdException001, TestSize.Level4)
{
    DBStatus status = DistributedRdbTestTools::GetOpenStoreStatus(g_manager, g_delegate1, g_rdbParameter2);
    ASSERT_EQ(status, DBStatus::INVALID_ARGS);
}

/**
 * @tc.name:  storeIdException002
 * @tc.desc: storeId value larger 128, openStore failed.
 * @tc.type: FUNC
 * @tc.require: SR000DORPP
 * @tc.author: xuhongkang
 */
HWTEST_F(DistributedRelatinalDBExceptionOperationTest, storeIdException002, TestSize.Level4)
{
    DBStatus status = DistributedRdbTestTools::GetOpenStoreStatus(g_manager, g_delegate1, g_rdbParameter5);
    ASSERT_EQ(status, DBStatus::INVALID_ARGS);
}

/**
 * @tc.name:  storeIdException003
 * @tc.desc: storeId is illegal, openStore failed.
 * @tc.type: FUNC
 * @tc.require: SR000DORPP
 * @tc.author: xuhongkang
 */
HWTEST_F(DistributedRelatinalDBExceptionOperationTest, storeIdException003, TestSize.Level4)
{
    DBStatus status = DistributedRdbTestTools::GetOpenStoreStatus(g_manager, g_delegate1, g_rdbParameter3);
    ASSERT_EQ(status, DBStatus::INVALID_ARGS);
}

/**
 * @tc.name:  dbModeException001
 * @tc.desc: db mode isn't wal or SYNCHRONOUS full, openStore failed.
 * @tc.type: FUNC
 * @tc.require: SR000DORPP
 * @tc.author: xuhongkang
 */
HWTEST_F(DistributedRelatinalDBExceptionOperationTest, dbModeException001, TestSize.Level4)
{
    sqlite3 *db = nullptr;    
    bool init1 = InitSqlite3Store(db, g_rdbParameter4);
    ASSERT_EQ(init1, true);
    bool res1 = Sqlite3ExecOpration(db, SQL_JOURNAL_MODE);
    ASSERT_EQ(res1, true);
    DBStatus status1 = DistributedRdbTestTools::GetOpenStoreStatus(g_manager, g_delegate1, g_rdbParameter4);
    ASSERT_EQ(status1, DBStatus::NOT_SUPPORT);
    CloseSqlite3Store(db);

    bool init2 = InitSqlite3Store(db, g_rdbParameter9);
    ASSERT_EQ(init2, true);
    bool res2 = Sqlite3ExecOpration(db, SQL_SYNCHRONOUS_MODE);
    ASSERT_EQ(res2, true);
    DBStatus status2 = DistributedRdbTestTools::GetOpenStoreStatus(g_manager, g_delegate1, g_rdbParameter4);
    ASSERT_EQ(status2, DBStatus::NOT_SUPPORT);
    CloseSqlite3Store(db);
}

/**
 * @tc.name:  tableException001
 * @tc.desc: db hasn't non_table, createDistributedTable failed.
 * @tc.type: FUNC
 * @tc.require: SR000DORPP
 * @tc.author: xuhongkang
 */
HWTEST_F(DistributedRelatinalDBExceptionOperationTest, tableException001, TestSize.Level4)
{
    DBStatus status2 = DistributedRdbTestTools::GetCreateDistributedTableStatus(g_delegate, NON_EXISTENT_TABLE);
    ASSERT_EQ(status2, DBStatus::INVALID_ARGS);
}

/**
 * @tc.name:  tableException002
 * @tc.desc: db create natrualbase_rdb_A relatinalTable, createDistributedTable failed.
 * @tc.type: FUNC
 * @tc.require: SR000DORPP
 * @tc.author: xuhongkang
 */
HWTEST_F(DistributedRelatinalDBExceptionOperationTest, tableException002, TestSize.Level4)
{
    DBStatus status2 = DistributedRdbTestTools::GetCreateDistributedTableStatus(g_delegate, KEYWORD_START_TABLE);
    ASSERT_EQ(status2, DBStatus::INVALID_ARGS);
}

/**
 * @tc.name:  tableException003
 * @tc.desc: db NORMAL_RDB table has data, createDistributedTable failed.
 * @tc.type: FUNC
 * @tc.require: SR000DORPP
 * @tc.author: xuhongkang
 */
HWTEST_F(DistributedRelatinalDBExceptionOperationTest, tableException003, TestSize.Level4)
{
    DBStatus status2 = DistributedRdbTestTools::GetCreateDistributedTableStatus(g_delegate, NORMAL_TABLE);
    ASSERT_EQ(status2, DBStatus::NOT_SUPPORT);
}

/**
 * @tc.name:  tableMulCreate001
 * @tc.desc: db create RDB_1 RelatinalTable, multiple createDistributedTable successful.
 * @tc.type: FUNC
 * @tc.require: SR000DORPP
 * @tc.author: xuhongkang
 */
HWTEST_F(DistributedRelatinalDBExceptionOperationTest, tableMulCreate001, TestSize.Level4)
{
    for (int i = 1; i <= 5; i++) {
        DBStatus status2 = DistributedRdbTestTools::GetCreateDistributedTableStatus(g_delegate, gtableNameList[0]);
        ASSERT_EQ(status2, DBStatus::OK);
    }
}

/**
 * @tc.name:  mulTableCreate001
 * @tc.desc: dbA createDistributedTable 32 tables, successful 
 * then dbA createDistributedTable the 33'th table failed.
 * @tc.type: FUNC
 * @tc.require: SR000DORPP
 * @tc.author: xuhongkang
 */
HWTEST_F(DistributedRelatinalDBExceptionOperationTest, mulTableCreate001, TestSize.Level4)
{
    for (auto tableName: gtableNameList) {
        DBStatus status = DistributedRdbTestTools::GetCreateDistributedTableStatus(g_delegate, tableName);
        if (tableName == gtableNameList[32]) {
            ASSERT_EQ(status, DBStatus::NOT_SUPPORT);
        } else {
           ASSERT_EQ(status, DBStatus::OK); 
        }   
    }
}

/**
 * @tc.name:  mulTableCreate002
 * @tc.desc: dbA createDistributedTable RDB_1...20 tables, And
 * dbB createDistributedTable RDB_21...32 tables,successful 
 * then dbA createDistributedTable the 33'th table failed.
 * @tc.type: FUNC
 * @tc.require: SR000DORPP
 * @tc.author: xuhongkang
 */
HWTEST_F(DistributedRelatinalDBExceptionOperationTest, mulTableCreate002, TestSize.Level4)
{
    RelatetionalStoreDelegate *delegateA = nullptr;
    RelatetionalStoreDelegate *delegateB = nullptr;  
    DBStatus status1 = DistributedRdbTestTools::GetOpenStoreStatus(g_manager, delegateA, g_rdbParameter1);
    ASSERT_EQ(status1, DBStatus::OK);
    DBStatus status2 = DistributedRdbTestTools::GetOpenStoreStatus(g_manager, delegateB, g_rdbParameter1);
    ASSERT_EQ(status2, DBStatus::OK);
    for (int index = 0; index < gtableNameList.size(); index++) {
        if (index < 20) {
            DBStatus status = DistributedRdbTestTools::GetCreateDistributedTableStatus(delegateA, gtableNameList[index]);
            ASSERT_EQ(status2, DBStatus::OK); 
        } else if (index >= 20 && index < 32)
        {
            DBStatus status = DistributedRdbTestTools::GetCreateDistributedTableStatus(delegateB, gtableNameList[index]);
            ASSERT_EQ(status2, DBStatus::OK); 
        } else {
            ASSERT_EQ(status2, DBStatus::NOT_SUPPORT);
        }
    }
}

/**
 * @tc.name:  relatinalTable001
 * @tc.desc: db has RDB_1 relatinalTable, then alter RDB_1 and createDistributedTable failed.
 * @tc.type: FUNC
 * @tc.require: SR000DORPP
 * @tc.author: xuhongkang
 */
HWTEST_F(DistributedRelatinalDBExceptionOperationTest, relatinalTable001, TestSize.Level4)
{
    bool result = alterTableAttributes(gdb);
    ASSERT_EQ(result, true);
    for (int i = 0; i < 4; i++) {
        DBStatus status2 = DistributedRdbTestTools::GetCreateDistributedTableStatus(g_delegate, gtableNameList[i]);
        ASSERT_EQ(status2, DBStatus::OK);
    }
}

/**
 * @tc.name:  constraintTable001
 * @tc.desc: db has constraint table, then createDistributedTable successful.
 * @tc.type: FUNC
 * @tc.require: SR000DORPP
 * @tc.author: xuhongkang
 */
HWTEST_F(DistributedRelatinalDBExceptionOperationTest, constraintTable001, TestSize.Level4)
{
    DBStatus status2 = DistributedRdbTestTools::GetCreateDistributedTableStatus(g_delegate, CONSTRAINT_TABLE);
    ASSERT_EQ(status2, DBStatus::OK);
}

/**
 * @tc.name:  constraintTable001
 * @tc.desc: db has relatinalTable RDB_5, then add data in local RDB_5 and createDistributedTable successful.
 * @tc.type: FUNC
 * @tc.require: SR000DORPP
 * @tc.author: xuhongkang
 */
HWTEST_F(DistributedRelatinalDBExceptionOperationTest, constraintTable001, TestSize.Level4)
{
    DBStatus status2 = DistributedRdbTestTools::GetCreateDistributedTableStatus(g_delegate, gtableNameList[4]);
    ASSERT_EQ(status2, DBStatus::OK);
    bool res1 = Sqlite3ExecOpration(db, SQL_INSERT_RDB5_TABLE);
    ASSERT_EQ(res1, true);
    DBStatus status2 = DistributedRdbTestTools::GetCreateDistributedTableStatus(g_delegate, gtableNameList[4]);
    ASSERT_EQ(status2, DBStatus::OK);
}
}
