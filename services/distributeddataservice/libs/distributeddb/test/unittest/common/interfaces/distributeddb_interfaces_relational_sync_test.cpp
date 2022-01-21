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

#include <gtest/gtest.h>

#include "db_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "log_print.h"
#include "relational_store_manager.h"
#include "virtual_communicator_aggregator.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    constexpr const char* DB_SUFFIX = ".db";
    constexpr const char* STORE_ID = "Relational_Store_ID";
    const std::string DEVICE_A = "device_a";
    std::string g_testDir;
    std::string g_dbDir;
    DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;

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
}

class DistributedDBRelationalSyncTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void) {};
    void SetUp() override ;
    void TearDown() override;
protected:
    sqlite3 *db = nullptr;
    RelationalStoreDelegate *delegate = nullptr;
};

void DistributedDBRelationalSyncTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGD("Test dir is %s", g_testDir.c_str());
    g_dbDir = g_testDir + "/";

    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);
}

void DistributedDBRelationalSyncTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();

    db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);
    RelationalTestUtils::CreateDeviceTable(db, "sync_data", "DEVICE_A");

    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);

    status = delegate->CreateDistributedTable("sync_data");
    EXPECT_EQ(status, OK);
}

void DistributedDBRelationalSyncTest::TearDown()
{
    g_mgr.CloseStore(delegate);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
}

/**
  * @tc.name: RelationalSyncTest001
  * @tc.desc: Test with sync interface, table is not a distributed table
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBRelationalSyncTest, RelationalSyncTest001, TestSize.Level1)
{
    std::vector<std::string> devices = {DEVICE_A};
    Query query = Query::Select("sync_datb");
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);

    EXPECT_EQ(errCode, RELATIONAL_SCHEMA_NOT_FOUND);
}

/**
  * @tc.name: RelationalSyncTest002
  * @tc.desc: Test with sync interface, query is not support
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBRelationalSyncTest, RelationalSyncTest002, TestSize.Level1)
{
    std::vector<std::string> devices = {DEVICE_A};
    Query query = Query::Select("sync_data").Like("value", "abc");
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);

    EXPECT_EQ(errCode, NOT_SUPPORT);
}

/**
  * @tc.name: RelationalSyncTest003
  * @tc.desc: Test with sync interface, query is invalid format
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBRelationalSyncTest, RelationalSyncTest003, TestSize.Level1)
{
    std::vector<std::string> devices = {DEVICE_A};
    Query query = Query::Select("sync_data").And().Or().EqualTo("flag", 2);
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);

    EXPECT_EQ(errCode, INVALID_QUERY_FORMAT);
}

/**
  * @tc.name: RelationalSyncTest004
  * @tc.desc: Test with sync interface, query use invalid field
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBRelationalSyncTest, RelationalSyncTest004, TestSize.Level1)
{
    std::vector<std::string> devices = {DEVICE_A};
    Query query = Query::Select("sync_data").EqualTo("fleg", 2);
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);

    EXPECT_EQ(errCode, INVALID_QUERY_FIELD);
}

/**
  * @tc.name: RelationalSyncTest005
  * @tc.desc: Test with sync interface, query table has been modified
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBRelationalSyncTest, RelationalSyncTest005, TestSize.Level1)
{
    std::string modifySql = "ALTER TABLE sync_data ADD COLUMN add_field INTEGER;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, modifySql), SQLITE_OK);

    std::vector<std::string> devices = {DEVICE_A};
    Query query = Query::Select("sync_data");
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);

    EXPECT_EQ(errCode, RELATIONAL_SCHEMA_CHANGED);
}

/**
  * @tc.name: RelationalSyncTest006
  * @tc.desc: Test with sync interface, query is not set table name
  * @tc.type: FUNC
  * @tc.require: AR000GK58F
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBRelationalSyncTest, RelationalSyncTest006, TestSize.Level1)
{
    std::vector<std::string> devices = {DEVICE_A};
    Query query = Query::Select();
    int errCode = delegate->Sync(devices, SyncMode::SYNC_MODE_PUSH_ONLY, query,
        [&devices](const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            EXPECT_EQ(devicesMap.size(), devices.size());
        }, true);

    EXPECT_EQ(errCode, NOT_SUPPORT);
}