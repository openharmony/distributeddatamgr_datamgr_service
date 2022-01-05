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
    const std::string g_syncTableName = "naturalbase_rdb_aux_" + 
        g_tableName + "_" + DBCommon::TransferStringToHex(DBCommon::TransferHashString(DEVICE_B));

    RelationalStoreManager g_mgr(APP_ID, USER_ID);
    std::string g_testDir;
    std::string g_dbDir;
    std::string g_id;
    DistributedDBToolsUnitTest g_tool;
    RelationalStoreDelegate* g_kvDelegatePtr = nullptr;
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
    RelationalVirtualDevice *g_deviceB = nullptr;
    std::vector<FieldInfo> g_fieldInfoList;

    void OpenStore()
    {
        RelationalStoreDelegate::Option option;
        g_mgr.OpenStore(g_dbDir, STORE_ID_1, option, g_kvDelegatePtr);
        ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    }

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
        for (int i = 0; i < static_cast<int>(rowData.size()); ++i) {
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
        sqlite3_finalize(stmt);
    }

    void GenerateValue(RowData &rowData, std::map<std::string, DataValue> &dataMap)
    {
        int64_t id = 0;
        dataMap["ID"] = id;
        rowData.push_back(dataMap["ID"]);
        std::string val = "test";
        dataMap["NAME"].SetText(val);
        rowData.push_back(dataMap["NAME"]);
        dataMap["AGE"] = INT64_MAX;
        rowData.push_back(dataMap["AGE"]);
    }

    void InsertFieldInfo()
    {
        g_fieldInfoList.clear();
        FieldInfo columnFirst;
        columnFirst.SetFieldName("ID");
        columnFirst.SetStorageType(StorageType::STORAGE_TYPE_INTEGER);
        FieldInfo columnSecond;
        columnSecond.SetFieldName("NAME");
        columnSecond.SetStorageType(StorageType::STORAGE_TYPE_TEXT);
        FieldInfo columnThird;
        columnThird.SetFieldName("AGE");
        columnThird.SetStorageType(StorageType::STORAGE_TYPE_INTEGER);
        g_fieldInfoList.push_back(columnFirst);
        g_fieldInfoList.push_back(columnSecond);
        g_fieldInfoList.push_back(columnThird);
        g_deviceB->SetLocalFieldInfo(g_fieldInfoList);
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

    int PrepareSelect(sqlite3 *db, sqlite3_stmt *&statement, const std::string &table)
    {
        const std::string sql = "SELECT * FROM " + table;
        return sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr);
    }

    void GetDataValue(sqlite3_stmt *statement, int row, int col, DataValue &dataValue)
    {
        int type = sqlite3_column_type(statement, col);
        switch (type) {
            case SQLITE_INTEGER: {
                dataValue = static_cast<int64_t>(sqlite3_column_int64(statement, col));
                break;    
            }
            case SQLITE_FLOAT: {
                dataValue = sqlite3_column_double(statement, col);
                break;
            }
            case SQLITE_TEXT: {
                dataValue.SetText(std::string(reinterpret_cast<const char*>(sqlite3_column_text(statement, col))));
                break;
            }
            case SQLITE_BLOB: {
                std::vector<uint8_t> blobValue;
                (void)SQLiteUtils::GetColumnBlobValue(statement, col, blobValue);
                Blob blob;
                blob.WriteBlob(blobValue.data(), static_cast<uint32_t>(blobValue.size()));
                dataValue.SetBlob(blob);
                break;
            }
            case SQLITE_NULL:
                break;
            default:
                LOGW("unknown type[%d] row[%llu] column[%d] ignore", type, row, col);
        }
    }

    void GetSyncDataStep(std::vector<RowData> &dataList, sqlite3_stmt *statement)
    {
        int columnCount = sqlite3_column_count(statement);
        RowData rowData;
        for (int col = 0; col < columnCount; ++col) {
            DataValue dataValue;
            GetDataValue(statement, static_cast<int>(dataList.size() + 1), col, dataValue);
            rowData.push_back(std::move(dataValue));
        }
        dataList.push_back(rowData);
    }

    void GetSyncData(sqlite3 *db, std::vector<RowData> &dataList)
    {
        sqlite3_stmt *statement = nullptr;
        EXPECT_EQ(PrepareSelect(db, statement, g_syncTableName), SQLITE_OK);
        while (true) {
            int rc = sqlite3_step(statement);
            if (rc != SQLITE_ROW) {
                LOGD("GetSyncData Exist by code[%d]", rc);
                break;
            }
            GetSyncDataStep(dataList, statement);
        }
    }

    void InsertValueToDB(RowData &rowData)
    {
        sqlite3 *db = nullptr;
        EXPECT_EQ(GetDB(db), SQLITE_OK);
        InsertValue(db, rowData);
        sqlite3_close(db);
    }

    void PrepareEnvironment(std::map<std::string, DataValue> dataMap)
    {
        sqlite3 *db = nullptr;
        EXPECT_EQ(GetDB(db), SQLITE_OK);
        EXPECT_EQ(CreateTable(db), SQLITE_OK);

        EXPECT_EQ(g_kvDelegatePtr->CreateDistributedTable(g_tableName), OK);

        sqlite3_close(db);

        InsertFieldInfo();
        RowData rowData;
        GenerateValue(rowData, dataMap);
        InsertValueToDB(rowData);
    }

    void PrepareVirtualEnvironment(RowData &rowData)
    {
        sqlite3 *db = nullptr;
        EXPECT_EQ(GetDB(db), SQLITE_OK);
        EXPECT_EQ(CreateTable(db), SQLITE_OK);
        EXPECT_EQ(g_kvDelegatePtr->CreateDistributedTable(g_tableName), OK);

        sqlite3_close(db);

        std::map<std::string, DataValue> dataMap;
        GenerateValue(rowData, dataMap);
        VirtualRowData virtualRowData;
        for (const auto &item : dataMap) {
            virtualRowData.objectData.PutDataValue(item.first, item.second);
        }
        virtualRowData.logInfo.timestamp = 1;
        g_deviceB->PutData(g_tableName, {virtualRowData});
        InsertFieldInfo();
    }

    void CheckData(RowData &rowData)
    {
        std::vector<RowData> dataList;
        sqlite3 *db = nullptr;
        EXPECT_EQ(GetDB(db), SQLITE_OK);
        GetSyncData(db, dataList);
        sqlite3_close(db);

        ASSERT_EQ(dataList.size(), static_cast<size_t>(1));
        for (size_t j = 0; j < dataList[0].size(); ++j) {
            EXPECT_TRUE(dataList[0][j] == rowData[j]);
        }
    }

    void CheckVirtualData(std::map<std::string, DataValue> data)
    {
        std::vector<VirtualRowData> targetData;
        g_deviceB->GetAllSyncData(g_tableName, targetData);

        ASSERT_EQ(targetData.size(), 1u);
        for (auto &[field, value] : data) {
            DataValue target;
            EXPECT_EQ(targetData[0].objectData.GetDataValue(field, target), E_OK);
            LOGD("field %s actual_val[%s] except_val[%s]",
                    field.c_str(), target.ToString().c_str(), value.ToString().c_str());
            EXPECT_TRUE(target == value);
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

    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);

    g_id = g_mgr.GetRelationalStoreIdentifier(APP_ID, USER_ID, STORE_ID_1);
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
    sqlite3 *db = nullptr;
    ASSERT_EQ(GetDB(db), SQLITE_OK);
    sqlite3_close(db);
    OpenStore();
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
* @tc.require: AR000GK58N
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBRelationalVerP2PSyncTest, NormalSync001, TestSize.Level1)
{
    std::map<std::string, DataValue> dataMap;
    PrepareEnvironment(dataMap);
    BlockSync(SyncMode::SYNC_MODE_PUSH_ONLY, OK);

    CheckVirtualData(dataMap);
}

/**
* @tc.name: Normal Sync 002
* @tc.desc: Test normal pull sync for add data.
* @tc.type: FUNC
* @tc.require: AR000GK58N
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBRelationalVerP2PSyncTest, NormalSync002, TestSize.Level1)
{
    std::map<std::string, DataValue> dataMap;
    PrepareEnvironment(dataMap);

    Query query = Query::Select(g_tableName);
    g_deviceB->GenericVirtualDevice::Sync(DistributedDB::SYNC_MODE_PULL_ONLY, query, true);

    CheckVirtualData(dataMap);
}

/**
* @tc.name: Normal Sync 003
* @tc.desc: Test normal push sync for update data.
* @tc.type: FUNC
* @tc.require: AR000GK58N
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBRelationalVerP2PSyncTest, NormalSync003, TestSize.Level1)
{
    std::map<std::string, DataValue> dataMap;
    PrepareEnvironment(dataMap);

    BlockSync(SyncMode::SYNC_MODE_PUSH_ONLY, OK);

    CheckVirtualData(dataMap);

    int64_t val = 0;

    RowData rowData;
    GenerateValue(rowData, dataMap);
    EXPECT_EQ(rowData[rowData.size() - 1].GetInt64(val), E_OK);
    rowData[rowData.size() - 1] = static_cast<int64_t>(1);
    InsertValueToDB(rowData);
    BlockSync(SyncMode::SYNC_MODE_PUSH_ONLY, OK);

    dataMap["AGE"] = static_cast<int64_t>(1);
    CheckVirtualData(dataMap);
}

/**
* @tc.name: Normal Sync 004
* @tc.desc: Test normal push sync for delete data.
* @tc.type: FUNC
* @tc.require: AR000GK58N
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBRelationalVerP2PSyncTest, NormalSync004, TestSize.Level1)
{
    std::map<std::string, DataValue> dataMap;
    PrepareEnvironment(dataMap);

    BlockSync(SyncMode::SYNC_MODE_PUSH_ONLY, OK);

    CheckVirtualData(dataMap);

    sqlite3 *db = nullptr;
    EXPECT_EQ(GetDB(db), SQLITE_OK);
    std::string sql = "DELETE FROM TEST_TABLE WHERE ID = 0";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    sqlite3_close(db);

    BlockSync(SyncMode::SYNC_MODE_PUSH_ONLY, OK);

    std::vector<VirtualRowData> dataList;
    EXPECT_EQ(g_deviceB->GetAllSyncData(g_tableName, dataList), E_OK);
    EXPECT_EQ(static_cast<int>(dataList.size()), 1);
    for (const auto &item : dataList) {
        EXPECT_EQ(item.logInfo.flag, DataItem::DELETE_FLAG);
    }
}

/**
* @tc.name: Normal Sync 005
* @tc.desc: Test normal push sync for add data.
* @tc.type: FUNC
* @tc.require: AR000GK58N
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBRelationalVerP2PSyncTest, NormalSync005, TestSize.Level1)
{
    RowData rowData;
    PrepareVirtualEnvironment(rowData);

    Query query = Query::Select(g_tableName);
    g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, true);

    CheckData(rowData);
}

/**
* @tc.name: Normal Sync 006
* @tc.desc: Test normal pull sync for add data.
* @tc.type: FUNC
* @tc.require: AR000GK58N
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBRelationalVerP2PSyncTest, NormalSync006, TestSize.Level1)
{
    RowData rowData;
    PrepareVirtualEnvironment(rowData);

    BlockSync(SYNC_MODE_PULL_ONLY, OK);

    CheckData(rowData);
}

/**
* @tc.name: AutoLaunchSync 001
* @tc.desc: Test rdb autoLaunch success when callback return true.
* @tc.type: FUNC
* @tc.require: AR000GK58N
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBRelationalVerP2PSyncTest, AutoLaunchSync001, TestSize.Level3)
{
    /**
     * @tc.steps: step1. open rdb store, create distribute table and insert data
     */
    RowData rowData;
    PrepareVirtualEnvironment(rowData);

    /**
     * @tc.steps: step2. set auto launch callBack
     */
    const AutoLaunchRequestCallback callback = [](const std::string &identifier, AutoLaunchParam &param){
        if (g_id != identifier) {
            return false;
        }
        param.path    = g_dbDir;
        param.appId   = APP_ID;
        param.userId  = USER_ID;
        param.storeId = STORE_ID_1;
        return true;
    };
    g_mgr.SetAutoLaunchRequestCallback(callback);
    /**
     * @tc.steps: step3. close store ensure communicator has closed
     */
    g_mgr.CloseStore(g_kvDelegatePtr);
    g_kvDelegatePtr = nullptr;
    /**
     * @tc.steps: step4. RunCommunicatorLackCallback to autolaunch store
     */
    LabelType labelType(g_id.begin(), g_id.end());
    g_communicatorAggregator->RunCommunicatorLackCallback(labelType);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    /**
     * @tc.steps: step5. Call sync expect sync successful
     */
    Query query = Query::Select(g_tableName);
    EXPECT_EQ(g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, true), E_OK);
    /**
     * @tc.steps: step6. check sync data ensure sync successful
     */
    CheckData(rowData);

    RelationalStoreDelegate::Option option;
    g_mgr.OpenStore(g_dbDir, STORE_ID_1, option, g_kvDelegatePtr);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);

    OpenStore();
    std::this_thread::sleep_for(std::chrono::minutes(1));
}

/**
* @tc.name: AutoLaunchSync 002
* @tc.desc: Test rdb autoLaunch failed when callback return false.
* @tc.type: FUNC
* @tc.require: AR000GK58N
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBRelationalVerP2PSyncTest, AutoLaunchSync002, TestSize.Level3)
{
    /**
     * @tc.steps: step1. open rdb store, create distribute table and insert data
     */
    RowData rowData;
    PrepareVirtualEnvironment(rowData);

    /**
     * @tc.steps: step2. set auto launch callBack
     */
    const AutoLaunchRequestCallback callback = [](const std::string &identifier, AutoLaunchParam &param){
        return false;
    };
    g_mgr.SetAutoLaunchRequestCallback(callback);
    /**
     * @tc.steps: step2. close store ensure communicator has closed
     */
    g_mgr.CloseStore(g_kvDelegatePtr);
    g_kvDelegatePtr = nullptr;
    /**
     * @tc.steps: step3. store cann't autoLaunch because callback return false
     */
    LabelType labelType(g_id.begin(), g_id.end());
    g_communicatorAggregator->RunCommunicatorLackCallback(labelType);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    /**
     * @tc.steps: step4. Call sync expect sync fail
     */
    Query query = Query::Select(g_tableName);
    SyncOperation::UserCallback callBack = [](const std::map<std::string, int> &statusMap) {
        for (const auto &entry : statusMap) {
            EXPECT_EQ(entry.second, static_cast<int>(SyncOperation::OP_COMM_ABNORMAL));
        }
    };
    EXPECT_EQ(g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, callBack, true), E_OK);

    OpenStore();
    std::this_thread::sleep_for(std::chrono::minutes(1));
}

/**
* @tc.name: AutoLaunchSync 003
* @tc.desc: Test rdb autoLaunch failed when callback is nullptr.
* @tc.type: FUNC
* @tc.require: AR000GK58N
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBRelationalVerP2PSyncTest, AutoLaunchSync003, TestSize.Level3)
{
    /**
     * @tc.steps: step1. open rdb store, create distribute table and insert data
     */
    RowData rowData;
    PrepareVirtualEnvironment(rowData);

    g_mgr.SetAutoLaunchRequestCallback(nullptr);
    /**
     * @tc.steps: step2. close store ensure communicator has closed
     */
    g_mgr.CloseStore(g_kvDelegatePtr);
    g_kvDelegatePtr = nullptr;
    /**
     * @tc.steps: step3. store cann't autoLaunch because callback is nullptr
     */
    LabelType labelType(g_id.begin(), g_id.end());
    g_communicatorAggregator->RunCommunicatorLackCallback(labelType);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    /**
     * @tc.steps: step4. Call sync expect sync fail
     */
    Query query = Query::Select(g_tableName);
    SyncOperation::UserCallback callBack = [](const std::map<std::string, int> &statusMap) {
        for (const auto &entry : statusMap) {
            EXPECT_EQ(entry.second, static_cast<int>(SyncOperation::OP_COMM_ABNORMAL));
        }
    };
    EXPECT_EQ(g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, callBack, true), E_OK);

    OpenStore();
    std::this_thread::sleep_for(std::chrono::minutes(1));
}
#endif