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
#include <gmock/gmock.h>
#include <thread>

#include "distributeddb_tools_unit_test.h"
#include "message.h"
#include "mock_auto_launch.h"
#include "mock_communicator.h"
#include "mock_meta_data.h"
#include "mock_single_ver_data_sync.h"
#include "mock_single_ver_state_machine.h"
#include "mock_sync_task_context.h"
#include "single_ver_kv_syncer.h"
#include "single_ver_relational_sync_task_context.h"
#include "virtual_communicator_aggregator.h"
#include "virtual_single_ver_sync_db_Interface.h"
#ifdef DATA_SYNC_CHECK_003
#include "virtual_relational_ver_sync_db_interface.h"
#endif

using namespace testing::ext;
using namespace testing;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
void Init(MockSingleVerStateMachine &stateMachine, MockSyncTaskContext &syncTaskContext,
    MockCommunicator &communicator, VirtualSingleVerSyncDBInterface &dbSyncInterface)
{
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    (void)syncTaskContext.Initialize("device", &dbSyncInterface, metadata, &communicator);
    (void)stateMachine.Initialize(&syncTaskContext, &dbSyncInterface, metadata, &communicator);
}
}

class DistributedDBMockSyncModuleTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBMockSyncModuleTest::SetUpTestCase(void)
{
}

void DistributedDBMockSyncModuleTest::TearDownTestCase(void)
{
}

void DistributedDBMockSyncModuleTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

void DistributedDBMockSyncModuleTest::TearDown(void)
{
}

/**
 * @tc.name: StateMachineCheck001
 * @tc.desc: Test machine do timeout when has same timerId.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck001, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);

    TimerId expectId = 0;
    TimerId actualId = expectId;
    EXPECT_CALL(syncTaskContext, GetTimerId()).WillOnce(Return(expectId));
    EXPECT_CALL(stateMachine, SwitchStateAndStep(_)).WillOnce(Return());

    stateMachine.CallStepToTimeout(actualId);
}

/**
 * @tc.name: StateMachineCheck002
 * @tc.desc: Test machine do timeout when has diff timerId.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck002, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);

    TimerId expectId = 0;
    TimerId actualId = 1;
    EXPECT_CALL(syncTaskContext, GetTimerId()).WillOnce(Return(expectId));
    EXPECT_CALL(stateMachine, SwitchStateAndStep(_)).Times(0);

    stateMachine.CallStepToTimeout(actualId);
}

/**
 * @tc.name: StateMachineCheck003
 * @tc.desc: Test machine exec next task when queue not empty.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck003, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);

    EXPECT_CALL(stateMachine, PrepareNextSyncTask()).WillOnce(Return(E_OK));

    EXPECT_CALL(syncTaskContext, IsTargetQueueEmpty()).WillRepeatedly(Return(false));
    EXPECT_CALL(syncTaskContext, MoveToNextTarget()).WillRepeatedly(Return());
    EXPECT_CALL(syncTaskContext, IsCurrentSyncTaskCanBeSkipped())
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    // we expect machine dont change context status when queue not empty
    EXPECT_CALL(syncTaskContext, SetOperationStatus(_)).WillOnce(Return());
    EXPECT_CALL(syncTaskContext, SetTaskExecStatus(_)).Times(0);

    EXPECT_EQ(stateMachine.CallExecNextTask(), E_OK);
}

/**
 * @tc.name: StateMachineCheck004
 * @tc.desc: Test machine deal time sync ack failed.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck004, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);

    DistributedDB::Message *message = new(std::nothrow) DistributedDB::Message();
    ASSERT_NE(message, nullptr);
    message->SetMessageType(TYPE_RESPONSE);
    message->SetSessionId(1u);
    EXPECT_CALL(syncTaskContext, GetRequestSessionId()).WillRepeatedly(Return(1u));
    EXPECT_EQ(stateMachine.CallTimeMarkSyncRecv(message), -E_INVALID_ARGS);
    EXPECT_EQ(syncTaskContext.GetTaskErrCode(), -E_INVALID_ARGS);
    delete message;
}

/**
 * @tc.name: StateMachineCheck005
 * @tc.desc: Test machine recv errCode.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck005, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);
    EXPECT_CALL(stateMachine, SwitchStateAndStep(_)).WillRepeatedly(Return());
    EXPECT_CALL(syncTaskContext, GetRequestSessionId()).WillRepeatedly(Return(0u));

    std::initializer_list<int> testCode = {-E_DISTRIBUTED_SCHEMA_CHANGED, -E_DISTRIBUTED_SCHEMA_NOT_FOUND};
    for (int errCode : testCode) {
        stateMachine.DataRecvErrCodeHandle(0, errCode);
        EXPECT_EQ(syncTaskContext.GetTaskErrCode(), errCode);
        stateMachine.CallDataAckRecvErrCodeHandle(errCode, true);
        EXPECT_EQ(syncTaskContext.GetTaskErrCode(), errCode);
    }
}

/**
 * @tc.name: StateMachineCheck006
 * @tc.desc: Test machine exec next task when queue not empty to empty.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck006, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);

    syncTaskContext.CallSetSyncMode(QUERY_PUSH);
    EXPECT_CALL(syncTaskContext, IsTargetQueueEmpty())
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    EXPECT_CALL(syncTaskContext, IsCurrentSyncTaskCanBeSkipped())
        .WillRepeatedly(Return(syncTaskContext.CallIsCurrentSyncTaskCanBeSkipped()));
    EXPECT_CALL(syncTaskContext, MoveToNextTarget()).WillOnce(Return());
    // we expect machine dont change context status when queue not empty
    EXPECT_CALL(syncTaskContext, SetOperationStatus(_)).WillOnce(Return());
    EXPECT_CALL(syncTaskContext, SetTaskExecStatus(_)).WillOnce(Return());
    EXPECT_CALL(syncTaskContext, Clear()).WillOnce(Return());

    EXPECT_EQ(stateMachine.CallExecNextTask(), -E_NO_SYNC_TASK);
}

/**
 * @tc.name: StateMachineCheck007
 * @tc.desc: Test machine DoSaveDataNotify in another thread.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck007, TestSize.Level3)
{
    MockSingleVerStateMachine stateMachine;
    uint8_t callCount = 0;
    EXPECT_CALL(stateMachine, DoSaveDataNotify(_, _, _))
        .WillRepeatedly([&callCount](uint32_t sessionId, uint32_t sequenceId, uint32_t inMsgId) {
            (void) sessionId;
            (void) sequenceId;
            (void) inMsgId;
            callCount++;
            std::this_thread::sleep_for(std::chrono::seconds(4)); // sleep 4s
        });
    stateMachine.CallStartSaveDataNotify(0, 0, 0);
    std::this_thread::sleep_for(std::chrono::seconds(5)); // sleep 5s
    stateMachine.CallStopSaveDataNotify();
    // timer is called once in 2s, we sleep 5s timer call twice
    EXPECT_EQ(callCount, 2);
    std::this_thread::sleep_for(std::chrono::seconds(10)); // sleep 10s to wait all thread exit
}

/**
 * @tc.name: DataSyncCheck001
 * @tc.desc: Test dataSync recv error ack.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, DataSyncCheck001, TestSize.Level1)
{
    SingleVerDataSync dataSync;
    DistributedDB::Message *message = new(std::nothrow) DistributedDB::Message();
    ASSERT_TRUE(message != nullptr);
    message->SetErrorNo(E_FEEDBACK_COMMUNICATOR_NOT_FOUND);
    EXPECT_EQ(dataSync.AckPacketIdCheck(message), true);
    delete message;
}

/**
 * @tc.name: DataSyncCheck002
 * @tc.desc: Test dataSync recv notify ack.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, DataSyncCheck002, TestSize.Level1)
{
    SingleVerDataSync dataSync;
    DistributedDB::Message *message = new(std::nothrow) DistributedDB::Message();
    ASSERT_TRUE(message != nullptr);
    message->SetMessageType(TYPE_NOTIFY);
    EXPECT_EQ(dataSync.AckPacketIdCheck(message), true);
    delete message;
}
#ifdef DATA_SYNC_CHECK_003
/**
 * @tc.name: DataSyncCheck003
 * @tc.desc: Test dataSync recv notify ack.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, DataSyncCheck003, TestSize.Level1)
{
    MockSingleVerDataSync mockDataSync;
    MockSyncTaskContext mockSyncTaskContext;
    auto mockMetadata = std::make_shared<MockMetadata>();
    SyncTimeRange dataTimeRange = {1, 0, 1, 0};
    mockDataSync.CallUpdateSendInfo(dataTimeRange, &mockSyncTaskContext);

    VirtualRelationalVerSyncDBInterface storage;
    MockCommunicator communicator;
    std::shared_ptr<Metadata> metadata = std::static_pointer_cast<Metadata>(mockMetadata);
    mockDataSync.Initialize(&storage, &communicator, metadata, "deviceId");

    DistributedDB::Message *message = new(std::nothrow) DistributedDB::Message();
    ASSERT_TRUE(message != nullptr);
    DataAckPacket packet;
    message->SetSequenceId(1);
    message->SetCopiedObject(packet);
    mockSyncTaskContext.SetQuerySync(true);

    EXPECT_CALL(*mockMetadata, GetLastQueryTime(_, _, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(*mockMetadata, SetLastQueryTime(_, _, _)).WillOnce([&dataTimeRange](const std::string &queryIdentify,
        const std::string &deviceId, const Timestamp &timestamp) {
        EXPECT_EQ(timestamp, dataTimeRange.endTime);
        return E_OK;
    });
    EXPECT_CALL(mockSyncTaskContext, SetOperationStatus(_)).WillOnce(Return());
    EXPECT_EQ(mockDataSync.TryContinueSync(&mockSyncTaskContext, message), -E_FINISHED);
    delete message;
}
#endif
/**
 * @tc.name: AutoLaunchCheck001
 * @tc.desc: Test autoLaunch close connection.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, AutoLaunchCheck001, TestSize.Level1)
{
    MockAutoLaunch mockAutoLaunch;
    /**
     * @tc.steps: step1. put AutoLaunchItem in cache to simulate a connection was auto launched
     */
    std::string id = "TestAutoLaunch";
    std::string userId = "userId";
    AutoLaunchItem item;
    mockAutoLaunch.SetAutoLaunchItem(id, userId, item);
    EXPECT_CALL(mockAutoLaunch, TryCloseConnection(_)).WillOnce(Return());
    /**
     * @tc.steps: step2. send close signal to simulate a connection was unused in 1 min
     * @tc.expected: 10 thread try to close the connection and one thread close success
     */
    const int loopCount = 10;
    int finishCount = 0;
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    std::condition_variable cv;
    for (int i = 0; i < loopCount; i++) {
        std::thread t = std::thread([&finishCount, &mockAutoLaunch, &id, &userId, &mutex, &cv] {
            mockAutoLaunch.CallExtConnectionLifeCycleCallbackTask(id, userId);
            finishCount++;
            if (finishCount == loopCount) {
                std::unique_lock<std::mutex> lockInner(mutex);
                cv.notify_one();
            }
        });
        t.detach();
    }
    cv.wait(lock, [&finishCount, &loopCount]() {
        return finishCount == loopCount;
    });
}

/**
 * @tc.name: SyncDataSync001
 * @tc.desc: Test request start when RemoveDeviceDataIfNeed failed.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncDataSync001, TestSize.Level1)
{
    MockSyncTaskContext syncTaskContext;
    MockSingleVerDataSync dataSync;

    EXPECT_CALL(dataSync, RemoveDeviceDataIfNeed(_)).WillRepeatedly(Return(-E_BUSY));
    EXPECT_EQ(dataSync.CallRequestStart(&syncTaskContext, PUSH), -E_BUSY);
    EXPECT_EQ(syncTaskContext.GetTaskErrCode(), -E_BUSY);
}

/**
 * @tc.name: SyncDataSync002
 * @tc.desc: Test pull request start when RemoveDeviceDataIfNeed failed.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncDataSync002, TestSize.Level1)
{
    MockSyncTaskContext syncTaskContext;
    MockSingleVerDataSync dataSync;

    EXPECT_CALL(dataSync, RemoveDeviceDataIfNeed(_)).WillRepeatedly(Return(-E_BUSY));
    EXPECT_EQ(dataSync.CallPullRequestStart(&syncTaskContext), -E_BUSY);
    EXPECT_EQ(syncTaskContext.GetTaskErrCode(), -E_BUSY);
}

/**
 * @tc.name: SyncDataSync003
 * @tc.desc: Test call RemoveDeviceDataIfNeed in diff thread.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncDataSync003, TestSize.Level1)
{
    MockSyncTaskContext syncTaskContext;
    MockSingleVerDataSync dataSync;

    VirtualSingleVerSyncDBInterface storage;
    MockCommunicator communicator;
    std::shared_ptr<MockMetadata> mockMetadata = std::make_shared<MockMetadata>();
    std::shared_ptr<Metadata> metadata = std::static_pointer_cast<Metadata>(mockMetadata);
    metadata->Initialize(&storage);
    const std::string deviceId = "deviceId";
    dataSync.Initialize(&storage, &communicator, metadata, deviceId);
    syncTaskContext.SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    syncTaskContext.Initialize(deviceId, &storage, metadata, &communicator);
    syncTaskContext.EnableClearRemoteStaleData(true);

    /**
     * @tc.steps: step1. set diff db createtime for rebuild label in meta
     */
    metadata->SetDbCreateTime(deviceId, 1, true); // 1 is old db createTime
    metadata->SetDbCreateTime(deviceId, 2, true); // 1 is new db createTime

    DistributedDB::Key k1 = {'k', '1'};
    DistributedDB::Value v1 = {'v', '1'};
    DistributedDB::Key k2 = {'k', '2'};
    DistributedDB::Value v2 = {'v', '2'};

    /**
     * @tc.steps: step2. call RemoveDeviceDataIfNeed in diff thread and then put data
     */
    std::thread thread1([&]() {
        (void)dataSync.CallRemoveDeviceDataIfNeed(&syncTaskContext);
        storage.PutDeviceData(deviceId, k1, v1);
        LOGD("PUT FINISH");
    });
    std::thread thread2([&]() {
        (void)dataSync.CallRemoveDeviceDataIfNeed(&syncTaskContext);
        storage.PutDeviceData(deviceId, k2, v2);
        LOGD("PUT FINISH");
    });
    thread1.join();
    thread2.join();

    DistributedDB::Value actualValue;
    storage.GetDeviceData(deviceId, k1, actualValue);
    EXPECT_EQ(v1, actualValue);
    storage.GetDeviceData(deviceId, k2, actualValue);
    EXPECT_EQ(v2, actualValue);
}

/**
 * @tc.name: AbilitySync001
 * @tc.desc: Test abilitySync abort when recv error.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, AbilitySync001, TestSize.Level1)
{
    MockSyncTaskContext syncTaskContext;
    AbilitySync abilitySync;

    DistributedDB::Message *message = new(std::nothrow) DistributedDB::Message();
    ASSERT_TRUE(message != nullptr);
    AbilitySyncAckPacket packet;
    packet.SetAckCode(-E_BUSY);
    message->SetCopiedObject(packet);
    EXPECT_EQ(abilitySync.AckRecv(message, &syncTaskContext), -E_BUSY);
    delete message;
    EXPECT_EQ(syncTaskContext.GetTaskErrCode(), -E_BUSY);
}

/**
 * @tc.name: AbilitySync002
 * @tc.desc: Test abilitySync abort when save meta failed.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, AbilitySync002, TestSize.Level1)
{
    MockSyncTaskContext syncTaskContext;
    AbilitySync abilitySync;
    MockCommunicator comunicator;
    VirtualSingleVerSyncDBInterface syncDBInterface;
    std::shared_ptr<Metadata> metaData = std::make_shared<Metadata>();
    metaData->Initialize(&syncDBInterface);
    abilitySync.Initialize(&comunicator, &syncDBInterface, metaData, "deviceId");

    /**
     * @tc.steps: step1. set AbilitySyncAckPacket ackCode is E_OK for pass the ack check
     */
    DistributedDB::Message *message = new(std::nothrow) DistributedDB::Message();
    ASSERT_TRUE(message != nullptr);
    AbilitySyncAckPacket packet;
    packet.SetAckCode(E_OK);
    packet.SetSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    message->SetCopiedObject(packet);
    /**
     * @tc.steps: step2. set syncDBInterface busy for save data return -E_BUSY
     */
    syncDBInterface.SetBusy(true);
    SyncStrategy mockStrategy = {true, false, false};
    EXPECT_CALL(syncTaskContext, GetSyncStrategy(_)).WillOnce(Return(mockStrategy));
    EXPECT_EQ(abilitySync.AckRecv(message, &syncTaskContext), -E_BUSY);
    delete message;
    EXPECT_EQ(syncTaskContext.GetTaskErrCode(), -E_BUSY);
}

/**
 * @tc.name: AbilitySync002
 * @tc.desc: Test abilitySync when offline.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, AbilitySync003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. set table TEST is permitSync
     */
    SingleVerRelationalSyncTaskContext *context = new (std::nothrow) SingleVerRelationalSyncTaskContext();
    ASSERT_NE(context, nullptr);
    RelationalSyncStrategy strategy;
    const std::string tableName = "TEST";
    strategy[tableName] = {true, true, true};
    context->SetRelationalSyncStrategy(strategy);
    QuerySyncObject query;
    query.SetTableName(tableName);
    /**
     * @tc.steps: step2. set table is need reset ability sync but it still permit sync
     */
    context->SetIsNeedResetAbilitySync(true);
    EXPECT_EQ(context->GetSyncStrategy(query).permitSync, true);
    /**
     * @tc.steps: step3. set table is schema change now it dont permit sync
     */
    context->SchemaChange();
    EXPECT_EQ(context->GetSyncStrategy(query).permitSync, false);
    RefObject::KillAndDecObjRef(context);
}

/**
 * @tc.name: SyncLifeTest001
 * @tc.desc: Test syncer alive when thread still exist.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncLifeTest001, TestSize.Level3)
{
    std::shared_ptr<SingleVerKVSyncer> syncer = std::make_shared<SingleVerKVSyncer>();
    VirtualCommunicatorAggregator *virtualCommunicatorAggregator = new VirtualCommunicatorAggregator();
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(virtualCommunicatorAggregator);
    VirtualSingleVerSyncDBInterface *syncDBInterface = new VirtualSingleVerSyncDBInterface();
    syncer->Initialize(syncDBInterface, true);
    syncer->EnableAutoSync(true);
    for (int i = 0; i < 1000; i++) { // trigger 1000 times auto sync check
        syncer->LocalDataChanged(SQLITE_GENERAL_NS_PUT_EVENT);
    }
    syncer = nullptr;
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    delete syncDBInterface;
}

/**
 * @tc.name: MessageScheduleTest001
 * @tc.desc: Test MessageSchedule stop timer when no message.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, MessageScheduleTest001, TestSize.Level1)
{
    MockSyncTaskContext *context = new MockSyncTaskContext();
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    bool last = false;
    context->OnLastRef([&last]() {
        last = true;
    });
    SingleVerDataMessageSchedule schedule;
    bool isNeedHandle = false;
    bool isNeedContinue = false;
    schedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
    RefObject::KillAndDecObjRef(context);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_TRUE(last);
}