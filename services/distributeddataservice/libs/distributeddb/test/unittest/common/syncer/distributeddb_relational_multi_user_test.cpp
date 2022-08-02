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

#include "db_constant.h"
#include "db_properties.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "platform_specific.h"
#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "relational_virtual_device.h"
#include "runtime_config.h"
#include "virtual_relational_ver_sync_db_interface.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    const string STORE_ID = "rdb_stroe_sync_test";
    const string USER_ID_1 = "userId1";
    const string USER_ID_2 = "userId2";
    const std::string DEVICE_B = "deviceB";
    const std::string DEVICE_C = "deviceC";
    const int WAIT_TIME = 1000; // 1000ms
    const std::string g_tableName = "TEST_TABLE";
    std::string g_identifier;

    RelationalStoreManager g_mgr1(APP_ID, USER_ID_1);
    RelationalStoreManager g_mgr2(APP_ID, USER_ID_2);
    KvStoreConfig g_config;
    DistributedDBToolsUnitTest g_tool;
    RelationalStoreDelegate* g_rdbDelegatePtr1 = nullptr;
    RelationalStoreDelegate* g_rdbDelegatePtr2 = nullptr;
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
    RelationalVirtualDevice *g_deviceB = nullptr;
    RelationalVirtualDevice *g_deviceC = nullptr;
    std::string g_dbDir;
    std::string g_storePath1;
    std::string g_storePath2;
    RelationalStoreObserverUnitTest *g_observer = nullptr;

    auto g_syncActivationCheckCallback1 = [] (const std::string &userId, const std::string &appId,
        const std::string &storeId)-> bool {
        if (userId == USER_ID_2) {
            LOGE("active call back1, active user2");
            return true;
        } else {
            LOGE("active call back1, no need to active user1");
            return false;
        }
    };
    auto g_syncActivationCheckCallback2 = [] (const std::string &userId, const std::string &appId,
        const std::string &storeId)-> bool {
        if (userId == USER_ID_1) {
            LOGE("active call back2, active user1");
            return true;
        } else {
            LOGE("active call back2, no need to active user2");
            return false;
        }
    };

    int DropTable(sqlite3 *db, const std::string &tableName)
    {
        std::string sql = "DROP TABLE IF EXISTS " + tableName + ";";
        char *errMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
        if (rc != 0) {
            LOGE("failed to drop table: %s, errMsg: %s", tableName.c_str(), errMsg);
        }
        return rc;
    }

    int CreateTable(sqlite3 *db, const std::string &tableName)
    {
        std::string sql = "CREATE TABLE " + tableName + "(id int, name text);";
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
        return rc;
    }

    int InsertValue(sqlite3 *db, const std::string &tableName)
    {
        std::string sql = "insert into " + tableName + " values(1, 'aaa');";
        return sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
    }

    int GetDB(sqlite3 *&db, const std::string &dbPath)
    {
        int flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
        int rc = sqlite3_open_v2(dbPath.c_str(), &db, flag, nullptr);
        if (rc != SQLITE_OK) {
            return rc;
        }
        EXPECT_EQ(SQLiteUtils::RegisterCalcHash(db), E_OK);
        EXPECT_EQ(SQLiteUtils::RegisterGetSysTime(db), E_OK);
        EXPECT_EQ(sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr), SQLITE_OK);
        return rc;
    }

    void PrepareVirtualDeviceEnv(const std::string &tableName, const std::string &dbPath,
        std::vector<RelationalVirtualDevice *> &remoteDeviceVec)
    {
        sqlite3 *db = nullptr;
        EXPECT_EQ(GetDB(db, dbPath), SQLITE_OK);
        TableInfo tableInfo;
        SQLiteUtils::AnalysisSchema(db, tableName, tableInfo);
        for (auto &dev : remoteDeviceVec) {
            std::vector<FieldInfo> fieldInfoList = tableInfo.GetFieldInfos();
            dev->SetLocalFieldInfo(fieldInfoList);
            dev->SetTableInfo(tableInfo);
        }
    }

    void PrepareData(const std::string &tableName, const std::string &dbPath)
    {
        sqlite3 *db = nullptr;
        EXPECT_EQ(GetDB(db, dbPath), SQLITE_OK);
        EXPECT_EQ(InsertValue(db, tableName), SQLITE_OK);
    }

    void OpenStore1(bool syncDualTupleMode = true)
    {
        if (g_observer == nullptr) {
            g_observer = new (std::nothrow) RelationalStoreObserverUnitTest();
        }
        RelationalStoreDelegate::Option option = {g_observer};
        option.syncDualTupleMode = syncDualTupleMode;
        g_mgr1.OpenStore(g_storePath1, STORE_ID_1, option, g_rdbDelegatePtr1);
        ASSERT_TRUE(g_rdbDelegatePtr1 != nullptr);
    }

    void OpenStore2(bool syncDualTupleMode = true)
    {
        if (g_observer == nullptr) {
            g_observer = new (std::nothrow) RelationalStoreObserverUnitTest();
        }
        RelationalStoreDelegate::Option option = {g_observer};
        option.syncDualTupleMode = syncDualTupleMode;
        g_mgr2.OpenStore(g_storePath2, STORE_ID_2, option, g_rdbDelegatePtr2);
        ASSERT_TRUE(g_rdbDelegatePtr2 != nullptr);
    }

    void CloseStore()
    {
        if (g_rdbDelegatePtr1 != nullptr) {
            ASSERT_EQ(g_mgr1.CloseStore(g_rdbDelegatePtr1), OK);
            g_rdbDelegatePtr1 = nullptr;
            LOGD("delete rdb store");
        }
        if (g_rdbDelegatePtr2 != nullptr) {
            ASSERT_EQ(g_mgr2.CloseStore(g_rdbDelegatePtr2), OK);
            g_rdbDelegatePtr2 = nullptr;
            LOGD("delete rdb store");
        }
    }

    void PrepareEnvironment(const std::string &tableName, const std::string &dbPath,
        RelationalStoreDelegate* rdbDelegate)
    {
        sqlite3 *db = nullptr;
        EXPECT_EQ(GetDB(db, dbPath), SQLITE_OK);
        EXPECT_EQ(DropTable(db, tableName), SQLITE_OK);
        EXPECT_EQ(CreateTable(db, tableName), SQLITE_OK);
        EXPECT_EQ(rdbDelegate->CreateDistributedTable(tableName), OK);
        sqlite3_close(db);
    }

    void CheckSyncTest(DBStatus status1, DBStatus status2, std::vector<std::string> &devices)
    {
        std::map<std::string, std::vector<TableStatus>> statusMap;
        SyncStatusCallback callBack = [&statusMap](
            const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            statusMap = devicesMap;
        };
        Query query = Query::Select(g_tableName);
        DBStatus status = g_rdbDelegatePtr1->Sync(devices, SYNC_MODE_PUSH_ONLY, query, callBack, true);
        LOGE("expect status is: %d, actual is %d", status1, status);
        ASSERT_TRUE(status == status1);
        if (status == OK) {
            for (const auto &pair : statusMap) {
                for (const auto &tableStatus : pair.second) {
                    LOGD("dev %s, status %d", pair.first.c_str(), tableStatus.status);
                    EXPECT_TRUE(tableStatus.status == OK);
                }
            }
        }
        statusMap.clear();

        std::map<std::string, std::vector<TableStatus>> statusMap2;
        SyncStatusCallback callBack2 = [&statusMap2](
            const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            LOGE("call back devicesMap.size = %d", devicesMap.size());
            statusMap2 = devicesMap;
        };
        status = g_rdbDelegatePtr2->Sync(devices, SYNC_MODE_PUSH_ONLY, query, callBack2, true);
        LOGE("expect status2 is: %d, actual is %d, statusMap2.size = %d", status2, status, statusMap2.size());
        ASSERT_TRUE(status == status2);
        if (status == OK) {
            for (const auto &pair : statusMap2) {
                for (const auto &tableStatus : pair.second) {
                    LOGE("*********** rdb dev %s, status %d", pair.first.c_str(), tableStatus.status);
                    EXPECT_TRUE(tableStatus.status == OK);
                }
            }
        }
        statusMap.clear();
    }

    void DoRemoteQuery()
    {
        RemoteCondition condition;
        condition.sql = "SELECT * FROM " + g_tableName;
        std::shared_ptr<ResultSet> result = std::make_shared<RelationalResultSetImpl>();
        EXPECT_EQ(g_rdbDelegatePtr1->RemoteQuery(g_deviceB->GetDeviceId(), condition,
            DBConstant::MAX_TIMEOUT, result), USER_CHANGED);
        EXPECT_EQ(result, nullptr);
        g_communicatorAggregator->RegOnDispatch(nullptr);
        CloseStore();
    }

    void CheckSyncResult(bool wait, bool isRemoteQuery)
    {
        std::mutex syncLock_{};
        std::condition_variable syncCondVar_{};
        std::map<std::string, std::vector<TableStatus>> statusMap;
        SyncStatusCallback callBack = [&statusMap, &syncLock_, &syncCondVar_, wait](
            const std::map<std::string, std::vector<TableStatus>> &devicesMap) {
            statusMap = devicesMap;
            if (!wait) {
                std::unique_lock<std::mutex> innerlock(syncLock_);
                syncCondVar_.notify_one();
            }
        };
        Query query = Query::Select(g_tableName);
        std::vector<std::string> devices;
        devices.push_back(g_deviceB->GetDeviceId());
        std::vector<RelationalVirtualDevice *> remoteDev;
        remoteDev.push_back(g_deviceB);
        PrepareVirtualDeviceEnv(g_tableName, g_storePath1, remoteDev);

        DBStatus status = DB_ERROR;
        if (isRemoteQuery) {
            DoRemoteQuery();
            return;
        }

        status = g_rdbDelegatePtr1->Sync(devices, SYNC_MODE_PUSH_ONLY, query, callBack, wait);
        EXPECT_EQ(status, OK);

        if (!wait) {
            std::unique_lock<std::mutex> lock(syncLock_);
            syncCondVar_.wait(lock, [status, &statusMap]() {
                if (status != OK) {
                    return true;
                }
                return !statusMap.empty();
            });
        }

        g_communicatorAggregator->RegOnDispatch(nullptr);
        EXPECT_EQ(statusMap.size(), devices.size());
        for (const auto &pair : statusMap) {
            for (const auto &tableStatus : pair.second) {
                EXPECT_TRUE(tableStatus.status == USER_CHANGED);
            }
        }
        CloseStore();
    }

    void TestSyncWithUserChange(bool wait, bool isRemoteQuery)
    {
        /**
         * @tc.steps: step1. set SyncActivationCheckCallback and only userId1 can active
         */
        RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback2);
        /**
         * @tc.steps: step2. openstore1 in dual tuple sync mode and openstore2 in normal sync mode
         * @tc.expected: step2. only user2 sync mode is active
         */
        OpenStore1(true);
        OpenStore2(true);

        /**
         * @tc.steps: step3. prepare environment
         */
        PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
        PrepareEnvironment(g_tableName, g_storePath2, g_rdbDelegatePtr2);

        /**
         * @tc.steps: step4. prepare data
         */
        PrepareData(g_tableName, g_storePath1);
        PrepareData(g_tableName, g_storePath2);

        /**
         * @tc.steps: step5. set SyncActivationCheckCallback and only userId2 can active
         */
        RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback1);

        /**
         * @tc.steps: step6. call NotifyUserChanged and block sync db concurrently
         * @tc.expected: step6. return OK
         */
        CipherPassword passwd;
        bool startSync = false;
        std::condition_variable cv;
        thread subThread([&]() {
            std::mutex notifyLock;
            std::unique_lock<std::mutex> lck(notifyLock);
            cv.wait(lck, [&startSync]() { return startSync; });
            EXPECT_TRUE(RuntimeConfig::NotifyUserChanged() == OK);
        });
        subThread.detach();
        g_communicatorAggregator->RegOnDispatch([&](const std::string&, Message *inMsg) {
            if (!startSync) {
                startSync = true;
                cv.notify_all();
            }
        });

        /**
         * @tc.steps: step7. deviceA call sync and wait
         * @tc.expected: step7. sync should return OK.
         */
        CheckSyncResult(wait, isRemoteQuery);
    }

    int PrepareSelect(sqlite3 *db, sqlite3_stmt *&statement, const std::string &table)
    {
        std::string dis_tableName = RelationalStoreManager::GetDistributedTableName(DEVICE_B, table);
        const std::string sql = "SELECT * FROM " + dis_tableName;
        LOGE("exec sql: %s", sql.c_str());
        return sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr);
    }

    void CheckDataInRealDevice()
    {
        sqlite3 *db = nullptr;
        EXPECT_EQ(GetDB(db, g_storePath2), SQLITE_OK);

        sqlite3_stmt *statement = nullptr;
        EXPECT_EQ(PrepareSelect(db, statement, g_tableName), SQLITE_OK);
        int rowCount = 0;
        while (true) {
            int rc = sqlite3_step(statement);
            if (rc != SQLITE_ROW) {
                LOGD("GetSyncData Exist by code[%d]", rc);
                break;
            }
            int columnCount = sqlite3_column_count(statement);
            EXPECT_EQ(columnCount, 2); // 2： result index
            rowCount++;
        }
        EXPECT_EQ(rowCount, 1);
        sqlite3_finalize(statement);
    }
}

class DistributedDBRelationalMultiUserTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBRelationalMultiUserTest::SetUpTestCase(void)
{
    /**
    * @tc.setup: Init datadir and Virtual Communicator.
    */
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_storePath1 = g_testDir + "/test1.db";
    g_storePath2 = g_testDir + "/test2.db";
    sqlite3 *db1 = nullptr;
    ASSERT_EQ(GetDB(db1, g_storePath1), SQLITE_OK);
    sqlite3_close(db1);

    sqlite3 *db2 = nullptr;
    ASSERT_EQ(GetDB(db2, g_storePath2), SQLITE_OK);
    sqlite3_close(db2);

    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);
}

void DistributedDBRelationalMultiUserTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void DistributedDBRelationalMultiUserTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    /**
     * @tc.setup: create virtual device B
     */
    g_deviceB = new (std::nothrow) RelationalVirtualDevice(DEVICE_B);
    ASSERT_TRUE(g_deviceB != nullptr);
    VirtualRelationalVerSyncDBInterface *syncInterfaceB = new (std::nothrow) VirtualRelationalVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceB != nullptr);
    ASSERT_EQ(g_deviceB->Initialize(g_communicatorAggregator, syncInterfaceB), E_OK);
    g_deviceC = new (std::nothrow) RelationalVirtualDevice(DEVICE_C);
    ASSERT_TRUE(g_deviceC != nullptr);
    VirtualRelationalVerSyncDBInterface *syncInterfaceC = new (std::nothrow) VirtualRelationalVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceC != nullptr);
    ASSERT_EQ(g_deviceC->Initialize(g_communicatorAggregator, syncInterfaceC), E_OK);
}

void DistributedDBRelationalMultiUserTest::TearDown(void)
{
    /**
     * @tc.teardown: Release device A, B, C
     */
    if (g_deviceB != nullptr) {
        delete g_deviceB;
        g_deviceB = nullptr;
    }
    if (g_deviceC != nullptr) {
        delete g_deviceC;
        g_deviceC = nullptr;
    }
    SyncActivationCheckCallback callback = nullptr;
    RuntimeConfig::SetSyncActivationCheckCallback(callback);
}

/**
 * @tc.name: multi user 001
 * @tc.desc: Test multi user change for rdb
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set SyncActivationCheckCallback and only userId2 can active
     */
    RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback1);

    /**
     * @tc.steps: step2. openstore1 and openstore2
     * @tc.expected: step2. only user2 sync mode is active
     */
    OpenStore1();
    OpenStore2();

    /**
     * @tc.steps: step3. prepare environment
     */
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    PrepareEnvironment(g_tableName, g_storePath2, g_rdbDelegatePtr2);

    /**
     * @tc.steps: step4. prepare data
     */
    PrepareData(g_tableName, g_storePath1);
    PrepareData(g_tableName, g_storePath2);

    /**
     * @tc.steps: step5. g_rdbDelegatePtr1 and g_rdbDelegatePtr2 call sync
     * @tc.expected: step5. g_rdbDelegatePtr2 call success
     */
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::vector<RelationalVirtualDevice *> remoteDev;
    remoteDev.push_back(g_deviceB);
    remoteDev.push_back(g_deviceC);
    PrepareVirtualDeviceEnv(g_tableName, g_storePath1, remoteDev);
    CheckSyncTest(NOT_ACTIVE, OK, devices);

    /**
     * @tc.expected: step6. onComplete should be called, DeviceB have {k1,v1}
     */
    std::vector<VirtualRowData> targetData;
    g_deviceB->GetAllSyncData(g_tableName, targetData);
    EXPECT_EQ(targetData.size(), 1u);

    /**
     * @tc.expected: step7. user change
     */
    RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback2);
    RuntimeConfig::NotifyUserChanged();
    /**
     * @tc.steps: step8. g_kvDelegatePtr1 and g_kvDelegatePtr2 call sync
     * @tc.expected: step8. g_kvDelegatePtr1 call success
     */
    devices.clear();
    devices.push_back(g_deviceC->GetDeviceId());
    CheckSyncTest(OK, NOT_ACTIVE, devices);
    /**
     * @tc.expected: step9. onComplete should be called, DeviceC have {k1,v1}
     */
    targetData.clear();
    g_deviceC->GetAllSyncData(g_tableName, targetData);
    EXPECT_EQ(targetData.size(), 1u);
    CloseStore();
}

/**
 * @tc.name: multi user 002
 * @tc.desc: Test multi user not change for rdb
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set SyncActivationCheckCallback and only userId2 can active
     */
    RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback1);

    /**
     * @tc.steps: step2. openstore1 and openstore2
     * @tc.expected: step2. only user2 sync mode is active
     */
    OpenStore1();
    OpenStore2();

    /**
     * @tc.steps: step3. prepare environment
     */
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    PrepareEnvironment(g_tableName, g_storePath2, g_rdbDelegatePtr2);

    /**
     * @tc.steps: step4. prepare data
     */
    PrepareData(g_tableName, g_storePath1);
    PrepareData(g_tableName, g_storePath2);

    /**
     * @tc.steps: step4. GetRelationalStoreIdentifier success when userId is invalid
     */
    std::string userId;
    EXPECT_TRUE(g_mgr1.GetRelationalStoreIdentifier(userId, APP_ID, USER_ID_2, true) != "");
    userId.resize(130);
    EXPECT_TRUE(g_mgr1.GetRelationalStoreIdentifier(userId, APP_ID, USER_ID_2, true) != "");

    /**
     * @tc.steps: step5. g_rdbDelegatePtr1 and g_rdbDelegatePtr2 call sync
     * @tc.expected: step5. g_rdbDelegatePtr2 call success
     */
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::vector<RelationalVirtualDevice *> remoteDev;
    remoteDev.push_back(g_deviceB);
    remoteDev.push_back(g_deviceC);
    PrepareVirtualDeviceEnv(g_tableName, g_storePath1, remoteDev);
    CheckSyncTest(NOT_ACTIVE, OK, devices);

    /**
     * @tc.expected: step6. onComplete should be called, DeviceB have {k1,v1}
     */
    std::vector<VirtualRowData> targetData;
    g_deviceB->GetAllSyncData(g_tableName, targetData);
    EXPECT_EQ(targetData.size(), 1u);

    /**
     * @tc.expected: step7. user not change
     */
    RuntimeConfig::NotifyUserChanged();

    /**
     * @tc.steps: step8. g_kvDelegatePtr1 and g_kvDelegatePtr2 call sync
     * @tc.expected: step8. g_kvDelegatePtr1 call success
     */
    CheckSyncTest(NOT_ACTIVE, OK, devices);

    /**
     * @tc.expected: step9. onComplete should be called, DeviceC have {k1,v1}
     */
    targetData.clear();
    g_deviceB->GetAllSyncData(g_tableName, targetData);
    EXPECT_EQ(targetData.size(), 1u);
    CloseStore();
}

/**
 * @tc.name: multi user 003
 * @tc.desc: enhancement callback return true in multiuser mode for rdb
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser003, TestSize.Level3)
{
    /**
     * @tc.steps: step1. prepare environment
     */
    OpenStore1();
    OpenStore2();
    PrepareEnvironment(g_tableName, g_storePath1, g_rdbDelegatePtr1);
    PrepareEnvironment(g_tableName, g_storePath2, g_rdbDelegatePtr2);
    CloseStore();

    /**
     * @tc.steps: step2. set SyncActivationCheckCallback and only userId2 can active
     */
    RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback1);

    const AutoLaunchRequestCallback callback = [](const std::string &identifier, AutoLaunchParam &param) {
        if (g_identifier != identifier) {
            LOGE("g_identifier(%s) != identifier(%s)", g_identifier, identifier);
            return false;
        }
        param.path    = g_testDir + "/test2.db";
        param.appId   = APP_ID;
        param.storeId = STORE_ID;
        CipherPassword passwd;
        param.option = {true, false, CipherType::DEFAULT, passwd, "", false, g_testDir, nullptr,
        0, nullptr};
        param.notifier = nullptr;
        param.option.syncDualTupleMode = true;
        return true;
    };

    /**
     * @tc.steps: step3. SetAutoLaunchRequestCallback
     * @tc.expected: step3. success.
     */
    g_mgr1.SetAutoLaunchRequestCallback(callback);

    /**
     * @tc.steps: step4. RunCommunicatorLackCallback
     * @tc.expected: step4. success.
     */
    g_identifier = g_mgr1.GetRelationalStoreIdentifier(USER_ID_2, APP_ID, STORE_ID, true);
    EXPECT_TRUE(g_identifier == g_mgr1.GetRelationalStoreIdentifier(USER_ID_1, APP_ID, STORE_ID, true));
    std::vector<uint8_t> label(g_identifier.begin(), g_identifier.end());
    g_communicatorAggregator->SetCurrentUserId(USER_ID_2);
    g_communicatorAggregator->RunCommunicatorLackCallback(label);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));

    /**
     * @tc.steps: step5. device B put one data
     * @tc.expected: step5. success.
     */
    VirtualRowData virtualRowData;
    DataValue d1;
    d1 = (int64_t)1;
    virtualRowData.objectData.PutDataValue("id", d1);
    DataValue d2;
    d2.SetText("hello");
    virtualRowData.objectData.PutDataValue("name", d2);
    virtualRowData.logInfo.timestamp = 1;
    g_deviceB->PutData(g_tableName, {virtualRowData});

    std::vector<RelationalVirtualDevice *> remoteDev;
    remoteDev.push_back(g_deviceB);
    PrepareVirtualDeviceEnv(g_tableName, g_storePath1, remoteDev);

    /**
     * @tc.steps: step6. device B push sync to A
     * @tc.expected: step6. success.
     */
    Query query = Query::Select(g_tableName);
    EXPECT_EQ(g_deviceB->GenericVirtualDevice::Sync(SYNC_MODE_PUSH_ONLY, query, true), OK);

    /**
     * @tc.steps: step7. deviceA have {k1,v1}
     * @tc.expected: step7. success.
     */
    CheckDataInRealDevice();
    std::this_thread::sleep_for(std::chrono::seconds(70)); // the store will close after 60 sec aotumatically
    g_mgr1.SetAutoLaunchRequestCallback(nullptr);
    CloseStore();
}

/**
 * @tc.name: multi user 004
 * @tc.desc: test NotifyUserChanged func when all db in normal sync mode for rdb
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. openstore1 and openstore2 in normal sync mode
     * @tc.expected: step1. only user2 sync mode is active
     */
    OpenStore1(false);
    OpenStore2(false);

    /**
     * @tc.steps: step2. call NotifyUserChanged
     * @tc.expected: step2. return OK
     */
    EXPECT_TRUE(RuntimeConfig::NotifyUserChanged() == OK);
    CloseStore();

     /**
     * @tc.steps: step3. openstore1 open normal sync mode and and openstore2 in dual tuple
     * @tc.expected: step3. only user2 sync mode is active
     */
    OpenStore1(false);
    OpenStore2();

    /**
     * @tc.steps: step4. call NotifyUserChanged
     * @tc.expected: step4. return OK
     */
    EXPECT_TRUE(RuntimeConfig::NotifyUserChanged() == OK);
    CloseStore();
}

/**
 * @tc.name: multi user 005
 * @tc.desc: test NotifyUserChanged and close db concurrently for rdb
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set SyncActivationCheckCallback and only userId1 can active
     */
    RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback2);

    /**
     * @tc.steps: step2. openstore1 in dual tuple sync mode and openstore2 in normal sync mode
     * @tc.expected: step2. only user2 sync mode is active
     */
    OpenStore1(true);
    OpenStore2(false);

    /**
     * @tc.steps: step3. set SyncActivationCheckCallback and only userId2 can active
     */
    RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback1);

    /**
     * @tc.steps: step4. call NotifyUserChanged and close db concurrently
     * @tc.expected: step4. return OK
     */
    thread subThread([&]() {
        EXPECT_TRUE(RuntimeConfig::NotifyUserChanged() == OK);
    });
    subThread.detach();
    EXPECT_EQ(g_mgr1.CloseStore(g_rdbDelegatePtr1), OK);
    g_rdbDelegatePtr1 = nullptr;
    CloseStore();
}

/**
 * @tc.name: multi user 006
 * @tc.desc: test NotifyUserChanged and block sync concurrently for rdb
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser006, TestSize.Level0)
{
    TestSyncWithUserChange(true, false);
}

/**
 * @tc.name: multi user 007
 * @tc.desc: test NotifyUserChanged and non-block sync concurrently for rdb
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser007, TestSize.Level0)
{
    TestSyncWithUserChange(false, false);
}

/**
 * @tc.name: multi user 008
 * @tc.desc: test NotifyUserChanged and remote query concurrently for rdb
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser008, TestSize.Level1)
{
    /**
     * @tc.steps: step1. set SyncActivationCheckCallback and only userId2 can active
     */
    RuntimeConfig::SetSyncActivationCheckCallback(g_syncActivationCheckCallback1);

    /**
     * @tc.steps: step2. openstore1 in dual tuple sync mode and openstore2 in normal sync mode
     * @tc.expected: step2. only user2 sync mode is active
     */
    OpenStore1(true);
    OpenStore2(true);

    /**
     * @tc.steps: step3. user1 call remote query
     * @tc.expected: step3. sync should return NOT_ACTIVE.
     */
    std::vector<RelationalVirtualDevice *> remoteDev;
    remoteDev.push_back(g_deviceB);
    PrepareVirtualDeviceEnv(g_tableName, g_storePath1, remoteDev);
    RemoteCondition condition;
    condition.sql = "SELECT * FROM " + g_tableName;
    std::shared_ptr<ResultSet> result = std::make_shared<RelationalResultSetImpl>();
    DBStatus status = g_rdbDelegatePtr1->RemoteQuery(g_deviceB->GetDeviceId(), condition,
        DBConstant::MAX_TIMEOUT, result);
    EXPECT_EQ(status, NOT_ACTIVE);
    EXPECT_EQ(result, nullptr);
    CloseStore();
}

/**
 * @tc.name: multi user 009
 * @tc.desc: test NotifyUserChanged and remote query concurrently for rdb
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBRelationalMultiUserTest, RdbMultiUser009, TestSize.Level0)
{
    TestSyncWithUserChange(false, true);
}