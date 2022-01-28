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

#include "distributeddb_tools_unit_test.h"
#include "message.h"
#include "mock_communicator.h"
#include "mock_single_ver_state_machine.h"
#include "mock_sync_task_context.h"
#include "virtual_single_ver_sync_db_Interface.h"

using namespace testing::ext;
using namespace testing;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
void Init(MockSingleVerStateMachine &stateMachine, MockSyncTaskContext &syncTaskContext,
    MockCommunicator &communicator, VirtualSingleVerSyncDBInterface &dbSyncInterface)
{
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
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