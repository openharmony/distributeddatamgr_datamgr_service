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

#ifdef RELATIONAL_STORE
#include <gtest/gtest.h>

#include "distributeddb_tools_unit_test.h"
#include "sqlite_utils.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    constexpr const char* DB_SUFFIX = ".db";
    constexpr const char* STORE_ID = "Relational_Store_ID";
    string g_testDir;
    string g_dbDir;
    const std::string NORMAL_CREATE_TABLE_SQL = "CREATE TABLE IF NOT EXISTS sync_data(" \
        "key         BLOB NOT NULL UNIQUE," \
        "value       BLOB," \
        "timestamp   INT  NOT NULL," \
        "flag        INT  NOT NULL," \
        "device      BLOB," \
        "ori_device  BLOB," \
        "hash_key    BLOB PRIMARY KEY NOT NULL," \
        "w_timestamp INT," \
        "UNIQUE(device, ori_device));" \
        "CREATE INDEX key_index ON sync_data (key, flag);";


    const std::string MULTI_PRIMARI_KEY_SQL = "CREATE TABLE IF NOT EXISTS sync_data(" \
        "key         BLOB NOT NULL UNIQUE," \
        "value       BLOB," \
        "timestamp   INT  NOT NULL," \
        "flag        INT  NOT NULL," \
        "device      BLOB," \
        "ori_device  BLOB," \
        "hash_key    BLOB NOT NULL," \
        "w_timestamp INT," \
        "PRIMARY KEY(hash_key, device)" \
        "UNIQUE(device, ori_device));" \
        "CREATE INDEX key_index ON sync_data (key, flag);";

    const std::string NO_PRIMARI_KEY_SQL = "CREATE TABLE IF NOT EXISTS t1 (a INT, b INT, c INT);";
}

class DistributedDBRelationalSchemaAnalysisTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBRelationalSchemaAnalysisTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
    LOGI("The test db is:%s", g_testDir.c_str());
    g_dbDir = g_testDir + "/";
}

void DistributedDBRelationalSchemaAnalysisTest::TearDownTestCase(void)
{}

void DistributedDBRelationalSchemaAnalysisTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

void DistributedDBRelationalSchemaAnalysisTest::TearDown(void)
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
    return;
}


namespace {
TableInfo AnalysisTable(const std::string &name, const std::string &sql)
{
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);

    TableInfo table;
    int errcode = SQLiteUtils::AnalysisSchema(db, name, table);
    EXPECT_EQ(errcode, E_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    return table;
}
}

/**
 * @tc.name: AnalysisTable001
 * @tc.desc: Analysis with table which has one primary key
 * @tc.type: FUNC
 * @tc.require: AR000H2PKN
 * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBRelationalSchemaAnalysisTest, AnalysisTable001, TestSize.Level1)
{
    TableInfo table = AnalysisTable("sync_data", NORMAL_CREATE_TABLE_SQL);
    EXPECT_EQ(table.GetPrimaryKey().size(), 1u);
}

/**
 * @tc.name: AnalysisTable002
 * @tc.desc: Analysis with table which has multi primary key
 * @tc.type: FUNC
 * @tc.require: AR000H2PKN
 * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBRelationalSchemaAnalysisTest, AnalysisTable002, TestSize.Level1)
{
    TableInfo table = AnalysisTable("sync_data", MULTI_PRIMARI_KEY_SQL);
    EXPECT_EQ(table.GetPrimaryKey().size(), 2u);
}

/**
 * @tc.name: AnalysisTable003
 * @tc.desc: Analysis with table which has no primary key
 * @tc.type: FUNC
 * @tc.require: AR000H2PKN
 * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBRelationalSchemaAnalysisTest, AnalysisTable003, TestSize.Level1)
{
    TableInfo table = AnalysisTable("t1", NO_PRIMARI_KEY_SQL);
    EXPECT_EQ(table.GetPrimaryKey().size(), 1u);
}
#endif // RELATIONAL_STORE