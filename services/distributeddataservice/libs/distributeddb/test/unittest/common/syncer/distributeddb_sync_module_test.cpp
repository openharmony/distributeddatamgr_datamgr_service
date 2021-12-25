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

#include "db_constant.h"
#include "distributeddb_tools_unit_test.h"
#include "mock_communicator.h"
#include "mock_single_ver_data_sync.h"
#include "mock_single_ver_sync_task_context.h"
#include "sliding_window_receiver.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

class DistributedDBSyncModuleTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBSyncModuleTest::SetUpTestCase(void)
{
}

void DistributedDBSyncModuleTest::TearDownTestCase(void)
{
}

void DistributedDBSyncModuleTest::SetUp(void)
{
}

void DistributedDBSyncModuleTest::TearDown(void)
{
}


namespace {
    void InitMessage(DataSyncMessageInfo &info)
    {
        info.messageId_ = DistributedDB::DATA_SYNC_MESSAGE;
        info.messageType_ = DistributedDB::TYPE_REQUEST;
        info.sendCode_ = -DistributedDB::E_UNFINISHED;
        info.mode_ = DistributedDB::PUSH;
    }

    int BuildPushMessage(DataSyncMessageInfo &info, DistributedDB::Message *&message)
    {
        return DistributedDBToolsUnitTest::BuildMessage(info, message);
    }
}

/**
  * @tc.name: SlidingWindow001
  * @tc.desc: Test sliding window receive data when water error happen
  * @tc.type: FUNC
  * @tc.require: AR000EV1G6
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBSyncModuleTest, SlidingWindow001, TestSize.Level1)
{
    std::shared_ptr<SingleVerDataSync> mockSingleVerDataSync = std::make_shared<MockSingleVerDataSync>();
    auto *context = new MockSingleVerSyncTaskContext();

    SlidingWindowReceiver receiver;
    receiver.Initialize(context, mockSingleVerDataSync);
    uint32_t sequenceId = 1;
    uint32_t sessionId = 1;
    uint64_t packedId = 1;
    DataSyncMessageInfo info{};
    InitMessage(info);
    info.sequenceId_ = sequenceId;
    info.sessionId_ = sessionId;
    info.packetId_ = packedId;

    DistributedDB::Message *message = nullptr;
    int status = BuildPushMessage(info, message);
    EXPECT_EQ(status, E_OK);

    /**
     * @tc.steps: step1. sliding window receive data first and now water error happen
     * @tc.expected: step1. receive msg should return -E_NOT_NEED_DELETE_MSG.
     */
    EXPECT_CALL(*context, HandleDataRequestRecv(testing::_)).WillRepeatedly(testing::Return(E_OK));
    EXPECT_CALL(*static_cast<MockSingleVerDataSync*>(mockSingleVerDataSync.get()),
                SendDataAck(testing::_, testing::_, testing::_, testing::_)).WillRepeatedly(testing::Return(E_OK));
    context->SetReceiveWaterMarkErr(true);
    EXPECT_EQ(receiver.Receive(message), -E_NOT_NEED_DELETE_MSG);

    /**
     * @tc.steps: step2. sliding window receive data which is has diff sessionId
     * @tc.expected: step2. receive msg should return -E_NOT_NEED_DELETE_MSG.
     */
    info.sessionId_++;
    status = BuildPushMessage(info, message);
    EXPECT_EQ(status, E_OK);
    EXPECT_EQ(receiver.Receive(message), -E_NOT_NEED_DELETE_MSG);

    /**
     * @tc.steps: step3. sliding window receive data which is has diff sequenceId
     * @tc.expected: step3. receive msg should return -E_NOT_NEED_DELETE_MSG.
     */
    info.sequenceId_++;
    info.packetId_++;
    status = BuildPushMessage(info, message);
    EXPECT_EQ(status, E_OK);
    EXPECT_EQ(receiver.Receive(message), -E_NOT_NEED_DELETE_MSG);

    receiver.Clear();
    // ensure to call DecRefObj in another thread
    std::this_thread::sleep_for(std::chrono::seconds(1));
    RefObject::KillAndDecObjRef(context);
    context = nullptr;
}