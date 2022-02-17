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

#include <condition_variable>
#include <gtest/gtest.h>
#include <thread>

#include "db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "kv_store_nb_delegate.h"
#include "kv_virtual_device.h"
#include "platform_specific.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    const string STORE_ID = "kv_stroe_sync_test";
    const string USER_ID_1 = "userId1";
    const string USER_ID_2 = "userId2";
    const std::string DEVICE_B = "deviceB";
    const std::string DEVICE_C = "deviceC";
    const int WAIT_TIME = 1000; // 1000ms

    KvStoreDelegateManager g_mgr1(APP_ID, USER_ID_1);
    KvStoreDelegateManager g_mgr2(APP_ID, USER_ID_2);
    KvStoreConfig g_config;
    DistributedDBToolsUnitTest g_tool;
    KvStoreNbDelegate* g_kvDelegatePtr1 = nullptr;
    KvStoreNbDelegate* g_kvDelegatePtr2 = nullptr;
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
    KvVirtualDevice *g_deviceB = nullptr;
    KvVirtualDevice *g_deviceC = nullptr;
    DBStatus g_kvDelegateStatus1 = INVALID_ARGS;
    DBStatus g_kvDelegateStatus2 = INVALID_ARGS;
    std::string g_identifier;

    // the type of g_kvDelegateCallback is function<void(DBStatus, KvStoreDelegate*)>
    auto g_kvDelegateCallback1 = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(g_kvDelegateStatus1), std::ref(g_kvDelegatePtr1));
    auto g_kvDelegateCallback2 = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(g_kvDelegateStatus2), std::ref(g_kvDelegatePtr2));
    auto g_syncActivationCheckCallback1 = [] (const std::string &userId, const std::string &appId,
        const std::string &storeId)-> bool {
        if (userId == USER_ID_2) {
            return true;
        } else {
            return false;
        }
        return true;
    };
    auto g_syncActivationCheckCallback2 = [] (const std::string &userId, const std::string &appId,
        const std::string &storeId)-> bool {
        if (userId == USER_ID_1) {
            return true;
        } else {
            return false;
        }
        return true;
    };
}

class DistributedDBSingleVerMultiUserTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBSingleVerMultiUserTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Init datadir and Virtual Communicator.
     */
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_config.dataDir = g_testDir;
    g_mgr1.SetKvStoreConfig(g_config);
    g_mgr2.SetKvStoreConfig(g_config);

    string dir = g_testDir + "/single_ver";
    DIR* dirTmp = opendir(dir.c_str());
    if (dirTmp == nullptr) {
        OS::MakeDBDirectory(dir);
    } else {
        closedir(dirTmp);
    }

    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);
}

void DistributedDBSingleVerMultiUserTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void DistributedDBSingleVerMultiUserTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    /**
     * @tc.setup: create virtual device B
     */
    g_deviceB = new (std::nothrow) KvVirtualDevice(DEVICE_B);
    ASSERT_TRUE(g_deviceB != nullptr);
    VirtualSingleVerSyncDBInterface *syncInterfaceB = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceB != nullptr);
    ASSERT_EQ(g_deviceB->Initialize(g_communicatorAggregator, syncInterfaceB), E_OK);
    g_deviceC = new (std::nothrow) KvVirtualDevice(DEVICE_C);
    ASSERT_TRUE(g_deviceC != nullptr);
    VirtualSingleVerSyncDBInterface *syncInterfaceC = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceC != nullptr);
    ASSERT_EQ(g_deviceC->Initialize(g_communicatorAggregator, syncInterfaceC), E_OK);
}

void DistributedDBSingleVerMultiUserTest::TearDown(void)
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
    g_mgr1.SetSyncActivationCheckCallback(callback);
}

void OpenStore1()
{
    KvStoreNbDelegate::Option option;
    option.syncDualTupleMode = true;
    g_mgr1.GetKvStore(STORE_ID, option, g_kvDelegateCallback1);
    ASSERT_TRUE(g_kvDelegateStatus1 == OK);
    ASSERT_TRUE(g_kvDelegatePtr1 != nullptr);
}

void OpenStore2()
{
    KvStoreNbDelegate::Option option;
    option.syncDualTupleMode = true;
    g_mgr2.GetKvStore(STORE_ID, option, g_kvDelegateCallback2);
    ASSERT_TRUE(g_kvDelegateStatus2 == OK);
    ASSERT_TRUE(g_kvDelegatePtr2 != nullptr);
}

void CloseStore()
{
    if (g_kvDelegatePtr1 != nullptr) {
        ASSERT_EQ(g_mgr1.CloseKvStore(g_kvDelegatePtr1), OK);
        g_kvDelegatePtr1 = nullptr;
        DBStatus status = g_mgr1.DeleteKvStore(STORE_ID);
        LOGD("delete kv store status %d", status);
        ASSERT_TRUE(status == OK);
    }
    if (g_kvDelegatePtr2 != nullptr) {
        ASSERT_EQ(g_mgr2.CloseKvStore(g_kvDelegatePtr2), OK);
        g_kvDelegatePtr2 = nullptr;
        DBStatus status = g_mgr2.DeleteKvStore(STORE_ID);
        LOGD("delete kv store status %d", status);
        ASSERT_TRUE(status == OK);
    }
}

void CheckSyncTest(DBStatus status1, DBStatus status2, std::vector<std::string> &devices)
{
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr1, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == status1);
    if (status == OK) {
        for (const auto &pair : result) {
            LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
            EXPECT_TRUE(pair.second == OK);
        }
    }
    result.clear();
    status = g_tool.SyncTest(g_kvDelegatePtr2, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == status2);
    if (status == OK) {
        ASSERT_TRUE(result.size() == devices.size());
        for (const auto &pair : result) {
            LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
            EXPECT_TRUE(pair.second == OK);
        }
    }
}

bool AutoLaunchCallBack(const std::string &identifier, AutoLaunchParam &param, KvStoreObserverUnitTest *observer,
    bool ret)
{
    LOGD("int AutoLaunchCallBack");
    EXPECT_TRUE(identifier == g_identifier);
    param.appId = APP_ID;
    param.storeId = STORE_ID;
    CipherPassword passwd;
    param.option = {true, false, CipherType::DEFAULT, passwd, "", false, g_testDir, observer,
        0, nullptr};
    param.notifier = nullptr;
    param.option.syncDualTupleMode = true;
    return ret;
}

/**
 * @tc.name: multi user 001
 * @tc.desc: Test multi user change
 * @tc.type: FUNC
 * @tc.require: AR000CQS3S SR000CQE0B
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerMultiUserTest, MultiUser001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set SyncActivationCheckCallback and only userId2 can active
     */
    g_mgr1.SetSyncActivationCheckCallback(g_syncActivationCheckCallback1);
    /**
     * @tc.steps: step2. openstore1 and openstore2
     * @tc.expected: step2. only user2 sync mode is active
     */

    OpenStore1();
    OpenStore2();
    /**
     * @tc.steps: step3. g_kvDelegatePtr1 and g_kvDelegatePtr2 put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    Value value2 = {'2'};
    ASSERT_TRUE(g_kvDelegatePtr1->Put(key, value2) == OK);
    ASSERT_TRUE(g_kvDelegatePtr2->Put(key, value) == OK);
    /**
     * @tc.steps: step4. g_kvDelegatePtr1 and g_kvDelegatePtr2 call sync
     * @tc.expected: step4. g_kvDelegatePtr2 call success
     */
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    CheckSyncTest(NOT_ACTIVE, OK, devices);
    /**
     * @tc.steps: step5. g_kvDelegatePtr1 support some pragma cmd call
     * @tc.expected: step5. Pragma call success
     */
    int pragmaData = 1;
    PragmaData input = static_cast<PragmaData>(&pragmaData);
    ASSERT_TRUE(g_kvDelegatePtr1->Pragma(AUTO_SYNC, input) == OK);
    pragmaData = 100;
    input = static_cast<PragmaData>(&pragmaData);
    ASSERT_TRUE(g_kvDelegatePtr1->Pragma(SET_QUEUED_SYNC_LIMIT, input) == OK);
    ASSERT_TRUE(g_kvDelegatePtr1->Pragma(GET_QUEUED_SYNC_LIMIT, input) == OK);
    ASSERT_TRUE(input == static_cast<PragmaData>(&pragmaData));
    pragmaData = 1;
    input = static_cast<PragmaData>(&pragmaData);
    ASSERT_TRUE(g_kvDelegatePtr1->Pragma(SET_WIPE_POLICY, input) == OK);
    ASSERT_TRUE(g_kvDelegatePtr1->Pragma(SET_SYNC_RETRY, input) == OK);
    /**
     * @tc.expected: step6. onComplete should be called, DeviceB have {k1,v1}
     */
    VirtualDataItem item;
    g_deviceB->GetData(key, item);
    EXPECT_TRUE(item.value == value);
    /**
     * @tc.expected: step7. user change
     */
    g_mgr1.SetSyncActivationCheckCallback(g_syncActivationCheckCallback2);
    KvStoreDelegateManager::NotifyUSerChanged();
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
    g_deviceC->GetData(key, item);
    EXPECT_TRUE(item.value == value2);
    CloseStore();
}

/**
 * @tc.name: multi user 002
 * @tc.desc: Test multi user not change
 * @tc.type: FUNC
 * @tc.require: AR000CQS3S SR000CQE0B
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerMultiUserTest, MultiUser002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set SyncActivationCheckCallback and only userId2 can active
     */
    g_mgr1.SetSyncActivationCheckCallback(g_syncActivationCheckCallback1);
    /**
     * @tc.steps: step2. openstore1 and openstore2
     * @tc.expected: step2. only user2 sync mode is active
     */

    OpenStore1();
    OpenStore2();
    /**
     * @tc.steps: step3. g_kvDelegatePtr1 and g_kvDelegatePtr2 put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    ASSERT_TRUE(g_kvDelegatePtr1->Put(key, value) == OK);
    ASSERT_TRUE(g_kvDelegatePtr2->Put(key, value) == OK);
    /**
     * @tc.steps: step4. GetKvStoreIdentifier success when userId is invalid
     */
    std::string userId;
    ASSERT_TRUE(g_mgr1.GetKvStoreIdentifier(userId, APP_ID, USER_ID_2, true) == OK);
    userId.resize(130);
    ASSERT_TRUE(g_mgr1.GetKvStoreIdentifier(userId, APP_ID, USER_ID_2, true) == OK);
    /**
     * @tc.steps: step5. g_kvDelegatePtr1 and g_kvDelegatePtr2 call sync
     * @tc.expected: step5. g_kvDelegatePtr2 call success
     */
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    CheckSyncTest(NOT_ACTIVE, OK, devices);
    /**
     * @tc.expected: step6. onComplete should be called, DeviceB have {k1,v1}
     */
    VirtualDataItem item;
    g_deviceB->GetData(key, item);
    EXPECT_TRUE(item.value == value);
    /**
     * @tc.expected: step7. user change
     */
    KvStoreDelegateManager::NotifyUSerChanged();
    /**
     * @tc.steps: step8. g_kvDelegatePtr1 and g_kvDelegatePtr2 put {k2, v2}
     */
    key = {'2'};
    value = {'2'};
    ASSERT_TRUE(g_kvDelegatePtr1->Put(key, value) == OK);
    ASSERT_TRUE(g_kvDelegatePtr2->Put(key, value) == OK);
    /**
     * @tc.steps: step9. g_kvDelegatePtr1 and g_kvDelegatePtr2 call sync
     * @tc.expected: step9. g_kvDelegatePtr2 call success
     */
    devices.clear();
    devices.push_back(g_deviceB->GetDeviceId());
    CheckSyncTest(NOT_ACTIVE, OK, devices);
    /**
     * @tc.expected: step10. onComplete should be called, DeviceB have {k2,v2}
     */
    g_deviceB->GetData(key, item);
    EXPECT_TRUE(item.value == value);
    CloseStore();
}

/**
 * @tc.name: multi user 003
 * @tc.desc: enhancement callback return true in multiuser mode
 * @tc.type: FUNC
 * @tc.require: AR000EPARJ
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerMultiUserTest, MultiUser003, TestSize.Level3)
{
    /**
     * @tc.steps: step1. set SyncActivationCheckCallback and only userId2 can active
     */
    g_mgr1.SetSyncActivationCheckCallback(g_syncActivationCheckCallback1);

    KvStoreObserverUnitTest *observer = new (std::nothrow) KvStoreObserverUnitTest;
    ASSERT_TRUE(observer != nullptr);
    /**
     * @tc.steps: step2. SetAutoLaunchRequestCallback
     * @tc.expected: step2. success.
     */
    g_mgr1.SetAutoLaunchRequestCallback(
        std::bind(AutoLaunchCallBack, std::placeholders::_1, std::placeholders::_2, observer, true));

    /**
     * @tc.steps: step2. RunCommunicatorLackCallback
     * @tc.expected: step2. success.
     */
    g_identifier = g_mgr1.GetKvStoreIdentifier(USER_ID_2, APP_ID, STORE_ID, true);
    EXPECT_TRUE(g_identifier == g_mgr1.GetKvStoreIdentifier(USER_ID_1, APP_ID, STORE_ID, true));
    std::vector<uint8_t> label(g_identifier.begin(), g_identifier.end());
    g_communicatorAggregator->SetCurrentUserId(USER_ID_2);
    g_communicatorAggregator->RunCommunicatorLackCallback(label);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    /**
     * @tc.steps: step3. device B put {k1, v1}
     * @tc.expected: step3. success.
     */
    Key key = {'1'};
    Value value = {'1'};
    TimeStamp currentTime;
    (void)OS::GetCurrentSysTimeInMicrosecond(currentTime);
    EXPECT_TRUE(g_deviceB->PutData(key, value, currentTime, 0) == OK);
    /**
     * @tc.steps: step4. device B push sync to A
     * @tc.expected: step4. success.
     */
    EXPECT_TRUE(g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, true) == OK);
    EXPECT_TRUE(observer->GetCallCount() == 1); // only A
    /**
     * @tc.steps: step5. deviceA have {k1,v1}
     * @tc.expected: step5. success.
     */
    OpenStore2();
    Value actualValue;
    g_kvDelegatePtr2->Get(key, actualValue);
    EXPECT_EQ(actualValue, value);
    std::this_thread::sleep_for(std::chrono::seconds(70));
    g_mgr1.SetAutoLaunchRequestCallback(nullptr);
    CloseStore();
    delete observer;
}

/**
 * @tc.name: MultiUser004
 * @tc.desc: CommunicatorLackCallback in multi user mode
 * @tc.type: FUNC
 * @tc.require: AR000E8S2T
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerMultiUserTest, MultiUser004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set SyncActivationCheckCallback and only userId2 can active
     */
    g_mgr1.SetSyncActivationCheckCallback(g_syncActivationCheckCallback1);

    /**
     * @tc.steps: step2. right param A B enable
     * @tc.expected: step2. success.
     */
    AutoLaunchNotifier notifier = nullptr;
    KvStoreObserverUnitTest *observer = new (std::nothrow) KvStoreObserverUnitTest;
    ASSERT_TRUE(observer != nullptr);
    AutoLaunchOption option;
    CipherPassword passwd;
    option = {true, false, CipherType::DEFAULT, passwd, "", false, g_testDir, observer,
        0, nullptr};
    option.notifier = nullptr;
    option.observer = observer;
    option.syncDualTupleMode = true;
    EXPECT_TRUE(g_mgr1.EnableKvStoreAutoLaunch(USER_ID_2, APP_ID, STORE_ID, option, notifier) == OK);
    EXPECT_TRUE(g_mgr1.EnableKvStoreAutoLaunch(USER_ID_1, APP_ID, STORE_ID, option, notifier) == OK);

    /**
     * @tc.steps: step3. RunCommunicatorLackCallback
     * @tc.expected: step3. userId2 open db successfully.
     */
    g_identifier = g_mgr1.GetKvStoreIdentifier(USER_ID_2, APP_ID, STORE_ID, true);
    std::vector<uint8_t> label(g_identifier.begin(), g_identifier.end());
    g_communicatorAggregator->SetCurrentUserId(USER_ID_2);
    g_communicatorAggregator->RunCommunicatorLackCallback(label);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    /**
     * @tc.steps: step5. device B put {k1, v1}
     * @tc.expected: step5. success.
     */
    Key key = {'1'};
    Value value = {'1'};
    TimeStamp currentTime;
    (void)OS::GetCurrentSysTimeInMicrosecond(currentTime);
    EXPECT_TRUE(g_deviceB->PutData(key, value, currentTime, 0) == OK);
    /**
     * @tc.steps: step6. device B push sync to A
     * @tc.expected: step6. success.
     */
    EXPECT_TRUE(g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, true) == OK);
    EXPECT_TRUE(observer->GetCallCount() == 1); // only A
    /**
     * @tc.steps: step7. deviceA have {k1,v1}
     * @tc.expected: step7. success.
     */
    OpenStore2();
    Value actualValue;
    g_kvDelegatePtr2->Get(key, actualValue);
    EXPECT_EQ(actualValue, value);
    /**
     * @tc.steps: step8. param A B disable
     * @tc.expected: step8. notifier WRITE_CLOSED
     */
    EXPECT_TRUE(g_mgr1.DisableKvStoreAutoLaunch(USER_ID_2, APP_ID, STORE_ID) == OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    EXPECT_TRUE(g_mgr1.DisableKvStoreAutoLaunch(USER_ID_1, APP_ID, STORE_ID) == OK);
    CloseStore();
    delete observer;
}