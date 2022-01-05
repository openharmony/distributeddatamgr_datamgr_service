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
#include "db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "isyncer.h"
#include "kv_virtual_device.h"
#include "platform_specific.h"
#include "relational_schema_object.h"
#include "relational_store_manager.h"
#include "relational_virtual_device.h"
#include "runtime_config.h"
#include "virtual_relational_ver_sync_db_interface.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
    const std::string DEVICE_B = "deviceB";
    const std::string g_tableName = "TEST_TABLE";

    RelationalStoreManager g_mgr(APP_ID, USER_ID);
    std::string g_testDir;
    std::string g_dbDir;
    DistributedDBToolsUnitTest g_tool;
    DBStatus g_kvDelegateStatus = INVALID_ARGS;
    RelationalStoreDelegate* g_kvDelegatePtr = nullptr;
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
    RelationalVirtualDevice *g_deviceB = nullptr;

    // the type of g_kvDelegateCallback is function<void(DBStatus, KvStoreDelegate*)>
    auto g_kvDelegateCallback = [](DBStatus status, RelationalStoreDelegate *delegatePtr) {
        g_kvDelegateStatus = status;
        g_kvDelegatePtr = delegatePtr;
    };

    int GetDB(sqlite3 *&db)
    {
        int flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
        const auto &dbPath = g_dbDir;
        int rc = sqlite3_open_v2(dbPath.c_str(), &db, flag, nullptr);
        if (rc != SQLITE_OK) {
            return rc;
        }
        EXPECT_EQ(SQLiteUtils::RegisterCalcHash(db), E_OK);
        EXPECT_EQ(SQLiteUtils::RegisterGetSysTime(db), E_OK);
        EXPECT_EQ(sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr), SQLITE_OK);
        return rc;
    }

    int CreateTable(sqlite3 *db)
    {
        char *pErrMsg = nullptr;

        const char *sql1 = "CREATE TABLE TEST_TABLE("  \
                           "ID INT PRIMARY KEY     NOT NULL," \
                           "NAME           TEXT    ," \
                           "AGE            INT);";
        return sqlite3_exec(db, sql1, nullptr, nullptr, &pErrMsg);
    }

    int PrepareInsert(sqlite3 *db, sqlite3_stmt *&statement)
    {
        const char *sql = "INSERT OR REPLACE INTO TEST_TABLE (ID,NAME,AGE) "  \
                          "VALUES (?, ?, ?);";
        return sqlite3_prepare_v2(db, sql, -1, &statement, nullptr);
    }

    int SimulateCommitData(sqlite3 *db, sqlite3_stmt *&statement)
    {
        sqlite3_exec(db, "BEGIN IMMEDIATE TRANSACTION", nullptr, nullptr, nullptr);

        int rc = sqlite3_step(statement);

        sqlite3_exec(db, "COMMIT TRANSACTION", nullptr, nullptr, nullptr);
        return rc;
    }

    void InsertValue(sqlite3 *db, RowData &rowData)
    {
        sqlite3_stmt *stmt = nullptr;
        EXPECT_EQ(PrepareInsert(db, stmt), SQLITE_OK);
        for (int i = 0; i < (int)rowData.size(); ++i) {
            const auto &item = rowData[i];
            const int index = i + 1;
            int errCode;
            if (item.GetType() == StorageType::STORAGE_TYPE_INTEGER) {
                int64_t val;
                (void)item.GetInt64(val);
                errCode = SQLiteUtils::BindInt64ToStatement(stmt, index, val);
            } else if (item.GetType() == StorageType::STORAGE_TYPE_TEXT) {
                std::string val;
                (void)item.GetText(val);
                errCode = SQLiteUtils::BindTextToStatement(stmt, index, val);
            }
            EXPECT_EQ(errCode, E_OK);
        }
        EXPECT_EQ(SimulateCommitData(db, stmt), SQLITE_DONE);
    }

    void GenerateValue(RowData &rowData, std::map<std::string, DataValue> &dataMap)
    {
        int64_t id = 0;
        dataMap["ID"] = id;
        rowData.push_back(dataMap["ID"]);
        std::string val = "test";
        dataMap["NAME"] = val;
        rowData.push_back(dataMap["NAME"]);
        dataMap["AGE"] = INT64_MAX;
        rowData.push_back(dataMap["AGE"]);
    }

    void InsertFieldInfo()
    {
        std::vector<FieldInfo> localFieldInfo;
        FieldInfo columnFirst;
        columnFirst.SetFieldName("ID");
        columnFirst.SetStorageType(StorageType::STORAGE_TYPE_INTEGER);
        FieldInfo columnSecond;
        columnSecond.SetFieldName("NAME");
        columnSecond.SetStorageType(StorageType::STORAGE_TYPE_TEXT);
        FieldInfo columnThird;
        columnThird.SetFieldName("AGE");
        columnThird.SetStorageType(StorageType::STORAGE_TYPE_INTEGER);
        localFieldInfo.push_back(columnFirst);
        localFieldInfo.push_back(columnSecond);
        localFieldInfo.push_back(columnThird);
        g_deviceB->SetLocalFieldInfo(localFieldInfo);
    }

    void BlockSync(SyncMode syncMode, DBStatus exceptStatus)
    {
        std::vector<std::string> devices = {DEVICE_B};
        Query query = Query::Select(g_tableName);
        std::map<std::string, std::vector<TableStatus>> statusMap;
        SyncStatusCallback callBack = [&statusMap](
            const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            statusMap = devicesMap;
        };
        DBStatus callStatus = g_kvDelegatePtr->Sync(devices, syncMode, query, callBack, true);
        EXPECT_EQ(callStatus, OK);
        for (const auto &tablesRes : statusMap) {
            for (const auto &tableStatus : tablesRes.second) {
                EXPECT_EQ(tableStatus.status, exceptStatus);
            }
        }
    }

    void InsertValueToDB(RowData &rowData)
    {
        sqlite3 *db = nullptr;
        EXPECT_EQ(GetDB(db), SQLITE_OK);
        InsertValue(db, rowData);
        sqlite3_close(db);
    }

    void PrepareEnvironment(RowData &rowData)
    {
        sqlite3 *db = nullptr;
        EXPECT_EQ(GetDB(db), SQLITE_OK);
        EXPECT_EQ(CreateTable(db), SQLITE_OK);

        EXPECT_EQ(g_kvDelegatePtr->CreateDistributedTable(g_tableName), OK);

        sqlite3_close(db);

        InsertFieldInfo();
        std::map<std::string, DataValue> dataMap;
        GenerateValue(rowData, dataMap);
        InsertValueToDB(rowData);

        rowData.clear();
        for (const auto &item : dataMap) {
            rowData.push_back(item.second);
        }
    }

    void CheckVirtualData(RowData &rowData)
    {
        std::vector<RowDataWithLog> targetData;
        g_deviceB->GetAllSyncData(g_tableName, targetData);

        for (auto &item : targetData) {
            for (int j = 0; j < (int)item.rowData.size(); ++j) {
                LOGD("index %d actual_val[%s] except_val[%s]",
                    j, item.rowData[j].ToString().c_str(), rowData[j].ToString().c_str());
                EXPECT_TRUE(item.rowData[j] == rowData[j]);
            }
        }
    }
}

class DistributedDBRelationalVerP2PSyncTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void DistributedDBRelationalVerP2PSyncTest::SetUpTestCase()
{
    /**
    * @tc.setup: Init datadir and Virtual Communicator.
    */
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_dbDir = g_testDir + "/test.db";
    sqlite3 *db = nullptr;
    ASSERT_EQ(GetDB(db), SQLITE_OK);
    sqlite3_close(db);

    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);
}

void DistributedDBRelationalVerP2PSyncTest::TearDownTestCase()
{
    /**
    * @tc.teardown: Release virtual Communicator and clear data dir.
    */
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    LOGD("TearDownTestCase FINISH");
}

void DistributedDBRelationalVerP2PSyncTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    /**
    * @tc.setup: create virtual device B, and get a KvStoreNbDelegate as deviceA
    */
    g_kvDelegateStatus = g_mgr.OpenStore(g_dbDir, "Relational_default_id", {}, g_kvDelegatePtr);
    ASSERT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    g_deviceB = new (std::nothrow) RelationalVirtualDevice(DEVICE_B);
    ASSERT_TRUE(g_deviceB != nullptr);
    auto *syncInterfaceB = new (std::nothrow) VirtualRelationalVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceB != nullptr);
    ASSERT_EQ(g_deviceB->Initialize(g_communicatorAggregator, syncInterfaceB), E_OK);

    auto permissionCheckCallback = [] (const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &deviceId, uint8_t flag) -> bool {
        return true;
    };
    EXPECT_EQ(RuntimeConfig::SetPermissionCheckCallback(permissionCheckCallback), OK);
}

void DistributedDBRelationalVerP2PSyncTest::TearDown(void)
{
    /**
    * @tc.teardown: Release device A, B, C
    */
    if (g_kvDelegatePtr != nullptr) {
        LOGD("CloseStore Start");
        ASSERT_EQ(g_mgr.CloseStore(g_kvDelegatePtr), OK);
        g_kvDelegatePtr = nullptr;
        LOGD("DeleteStore Start");
        DBStatus status = g_mgr.DeleteStore(g_dbDir);
        LOGD("delete kv store status %d", status);
        ASSERT_TRUE(status == OK);
    }
    if (g_deviceB != nullptr) {
        delete g_deviceB;
        g_deviceB = nullptr;
    }
    PermissionCheckCallbackV2 nullCallback;
    EXPECT_EQ(RuntimeConfig::SetPermissionCheckCallback(nullCallback), OK);
    LOGD("TearDown FINISH");
}

/**
* @tc.name: Normal Sync 001
* @tc.desc: Test normal push sync for add data.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBRelationalVerP2PSyncTest, NormalSync001, TestSize.Level1)
{
    RowData rowData;
    PrepareEnvironment(rowData);
    BlockSync(SyncMode::SYNC_MODE_PUSH_ONLY, OK);

    CheckVirtualData(rowData);
}

/**
* @tc.name: Normal Sync 002
* @tc.desc: Test normal pull sync for add data.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBRelationalVerP2PSyncTest, NormalSync002, TestSize.Level1)
{
    RowData rowData;
    PrepareEnvironment(rowData);

    std::vector<std::string> devices = {DEVICE_B};
    Query query = Query::Select(g_tableName);
    g_deviceB->GenericVirtualDevice::Sync(DistributedDB::SYNC_MODE_PULL_ONLY, query, true);

    CheckVirtualData(rowData);
}

/**
* @tc.name: Normal Sync 003
* @tc.desc: Test normal push sync for update data.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBRelationalVerP2PSyncTest, NormalSync003, TestSize.Level1)
{
    RowData rowData;
    PrepareEnvironment(rowData);

    BlockSync(SyncMode::SYNC_MODE_PUSH_ONLY, OK);

    CheckVirtualData(rowData);

    int64_t val = 0;

    std::map<std::string, DataValue> dataMap;
    rowData.clear();
    GenerateValue(rowData, dataMap);
    EXPECT_EQ(rowData[rowData.size() - 1].GetInt64(val), E_OK);
    rowData[rowData.size() - 1] = static_cast<int64_t>(1);
    InsertValueToDB(rowData);
    BlockSync(SyncMode::SYNC_MODE_PUSH_ONLY, OK);

    rowData.clear();
    dataMap["AGE"] = static_cast<int64_t>(1);
    for (const auto &item : dataMap) {
        rowData.push_back(item.second);
    }
    CheckVirtualData(rowData);
}

/**
* @tc.name: Normal Sync 004
* @tc.desc: Test normal push sync for delete data.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBRelationalVerP2PSyncTest, NormalSync004, TestSize.Level1)
{
    RowData rowData;
    PrepareEnvironment(rowData);

    BlockSync(SyncMode::SYNC_MODE_PUSH_ONLY, OK);

    CheckVirtualData(rowData);

    sqlite3 *db = nullptr;
    EXPECT_EQ(GetDB(db), SQLITE_OK);
    std::string sql = "DELETE FROM TEST_TABLE WHERE ID = 0";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    sqlite3_close(db);

    BlockSync(SyncMode::SYNC_MODE_PUSH_ONLY, OK);

    std::vector<RowDataWithLog> dataList;
    EXPECT_EQ(g_deviceB->GetAllSyncData(g_tableName, dataList), E_OK);
    EXPECT_EQ(static_cast<int>(dataList.size()), 1);
    for (const auto &item : dataList) {
        EXPECT_EQ(item.logInfo.flag, DataItem::DELETE_FLAG);
    }
}
#endif