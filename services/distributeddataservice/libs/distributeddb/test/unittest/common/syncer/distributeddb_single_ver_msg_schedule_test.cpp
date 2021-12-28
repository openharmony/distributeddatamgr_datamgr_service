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

#include "distributeddb_tools_unit_test.h"
#include "single_ver_data_message_schedule.h"
#include "single_ver_data_packet.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
}

class DistributedDBSingleVerMsgScheduleTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBSingleVerMsgScheduleTest::SetUpTestCase(void)
{
}

void DistributedDBSingleVerMsgScheduleTest::TearDownTestCase(void)
{
}

void DistributedDBSingleVerMsgScheduleTest::SetUp(void)
{
}

void DistributedDBSingleVerMsgScheduleTest::TearDown(void)
{
}

/**
 * @tc.name: MsgSchedule001
 * @tc.desc: Test MsgSchedule funtion with normal sequenceId
 * @tc.type: FUNC
 * @tc.require: 
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerMsgScheduleTest, MsgSchedule001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. put msg sequence_3, sequence_2, sequence_1
     * @tc.expected: put msg ok
     */
    SingleVerDataMessageSchedule msgSchedule;
    auto *context = new SingleVerSyncTaskContext();
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    DataSyncMessageInfo info;
    info.sessionId_ = 10;
    bool isNeedHandle = true;
    bool isNeedContinue = true;
    for (uint32_t i = 3; i >= 1; i--) {
        info.sequenceId_ = i;
        info.packetId_ = i;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(info, message);
        msgSchedule.PutMsg(message);
        if (i > 1) {
            Message *msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
            ASSERT_TRUE(msg == nullptr);
        }
    }
    /**
     * @tc.steps: step2. get msg
     * @tc.expected: get msg by sequence_1, sequence_2, sequence_3
     */
    for (uint32_t i = 1; i <= 3; i++) {
        Message *msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
        ASSERT_TRUE(msg != nullptr);
        EXPECT_EQ(isNeedContinue, true);
        EXPECT_EQ(isNeedHandle, true);
        EXPECT_EQ(msg->GetSequenceId(), i);
        msgSchedule.ScheduleInfoHandle(isNeedHandle, false, msg);
        delete msg;
    }
    Message *msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
    ASSERT_TRUE(msg == nullptr);
    RefObject::KillAndDecObjRef(context);
    context = nullptr;
}

/**
 * @tc.name: MsgSchedule002
 * @tc.desc: Test MsgSchedule funtion with by low version
 * @tc.type: FUNC
 * @tc.require: 
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerMsgScheduleTest, MsgSchedule002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. put msg session1_sequence1, session2_sequence1
     * @tc.expected: put msg ok
     */
    SingleVerDataMessageSchedule msgSchedule;
    auto *context = new SingleVerSyncTaskContext();
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_RELEASE_2_0);
    DataSyncMessageInfo info;
    bool isNeedHandle = true;
    bool isNeedContinue = true;
    for (uint32_t i = 1; i <= 2; i++) {
        info.sessionId_ = i;
        info.sequenceId_ = 1;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(info, message);
        msgSchedule.PutMsg(message);
    }
    /**
     * @tc.steps: step2. get msg
     * @tc.expected: get msg by session2_sequence1
     */
    Message *msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
    ASSERT_TRUE(msg != nullptr);
    EXPECT_EQ(isNeedContinue, true);
    EXPECT_EQ(isNeedHandle, true);
    EXPECT_EQ(msg->GetSequenceId(), 1u);
    msgSchedule.ScheduleInfoHandle(false, false, msg);
    delete msg;
    msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
    ASSERT_TRUE(msg == nullptr);
    RefObject::KillAndDecObjRef(context);
    context = nullptr;
}

/**
 * @tc.name: MsgSchedule003
 * @tc.desc: Test MsgSchedule funtion with cross sessionId
 * @tc.type: FUNC
 * @tc.require: 
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerMsgScheduleTest, MsgSchedule003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. put msg session1_seq1, session2_seq1, session1_seq2, session2_seq2, and handle session1_seq1
     * @tc.expected: handle ok
     */
    SingleVerDataMessageSchedule msgSchedule;
    auto *context = new SingleVerSyncTaskContext();
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    DataSyncMessageInfo info;
    bool isNeedHandle = true;
    bool isNeedContinue = true;
    info.sessionId_ = 1;
    info.sequenceId_ = 1;
    info.packetId_ = 1;
    DistributedDB::Message *message = nullptr;
    DistributedDBToolsUnitTest::BuildMessage(info, message);
    msgSchedule.PutMsg(message);
    Message *msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
    ASSERT_TRUE(msg != nullptr);
    EXPECT_EQ(isNeedContinue, true);
    EXPECT_EQ(isNeedHandle, true);
    msgSchedule.ScheduleInfoHandle(isNeedHandle, false, msg);
    delete msg;
    info.sessionId_ = 2;
    info.sequenceId_ = 1;
    info.packetId_ = 1;
    DistributedDBToolsUnitTest::BuildMessage(info, message);
    msgSchedule.PutMsg(message);
    info.sessionId_ = 1;
    info.sequenceId_ = 2;
    info.packetId_ = 2;
    DistributedDBToolsUnitTest::BuildMessage(info, message);
    msgSchedule.PutMsg(message);
    info.sessionId_ = 2;
    info.sequenceId_ = 2;
    info.packetId_ = 2;
    DistributedDBToolsUnitTest::BuildMessage(info, message);
    msgSchedule.PutMsg(message);

    /**
     * @tc.steps: step2. get msg
     * @tc.expected: get msg by session2_seq1, session2_seq2
     */
    for (uint32_t i = 1; i <= 2; i++) {
        msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
        ASSERT_TRUE(msg != nullptr);
        EXPECT_EQ(isNeedContinue, true);
        EXPECT_EQ(isNeedHandle, true);
        EXPECT_EQ(msg->GetSequenceId(), i);
        EXPECT_EQ(msg->GetSessionId(), 2u);
        msgSchedule.ScheduleInfoHandle(isNeedHandle, false, msg);
        delete msg;
    }
    msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
    ASSERT_TRUE(msg == nullptr);
    RefObject::KillAndDecObjRef(context);
    context = nullptr;
}

/**
 * @tc.name: MsgSchedule004
 * @tc.desc: Test MsgSchedule funtion with same sessionId with different packetId
 * @tc.type: FUNC
 * @tc.require: 
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerMsgScheduleTest, MsgSchedule004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. put msg seq2_packet2, seq3_packet3, seq1_packet4, seq2_packet5, seq3_packet6
     * @tc.expected: put msg ok
     */
    SingleVerDataMessageSchedule msgSchedule;
    auto *context = new SingleVerSyncTaskContext();
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    DataSyncMessageInfo info;
    info.sessionId_ = 10;
    bool isNeedHandle = true;
    bool isNeedContinue = true;
    for (uint32_t i = 2; i <= 3; i++) {
        info.sequenceId_ = i;
        info.packetId_ = i;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(info, message);
        msgSchedule.PutMsg(message);
        Message *msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
        ASSERT_TRUE(msg == nullptr);
    }
    for (uint32_t i = 1; i <= 3; i++) {
        info.sequenceId_ = i;
        info.packetId_ = i + 3;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(info, message);
        msgSchedule.PutMsg(message);
    }
    /**
     * @tc.steps: step2. get msg
     * @tc.expected: drop seq2_packet2, seq3_packet3 and get seq1_packet4, seq2_packet5, seq3_packet6
     */
    isNeedHandle = true;
    isNeedContinue = true;
    for (uint32_t i = 1; i <= 3; i++) {
        Message *msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
        ASSERT_TRUE(msg != nullptr);
        EXPECT_EQ(isNeedContinue, true);
        EXPECT_EQ(isNeedHandle, true);
        EXPECT_EQ(msg->GetSequenceId(), i);
        const DataRequestPacket *packet = msg->GetObject<DataRequestPacket>();
        EXPECT_EQ(packet->GetPacketId(), i + 3);
        msgSchedule.ScheduleInfoHandle(isNeedHandle, false, msg);
        delete msg;
    }
    Message *msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
    ASSERT_TRUE(msg == nullptr);
    RefObject::KillAndDecObjRef(context);
    context = nullptr;
}

/**
 * @tc.name: MsgSchedule005
 * @tc.desc: Test MsgSchedule funtion with same sessionId with different packetId
 * @tc.type: FUNC
 * @tc.require: 
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerMsgScheduleTest, MsgSchedule005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. put msg seq1_packet4, seq2_packet5, seq2_packet2, seq3_packet3
     * @tc.expected: put msg ok
     */
    SingleVerDataMessageSchedule msgSchedule;
    auto *context = new SingleVerSyncTaskContext();
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    DataSyncMessageInfo info;
    info.sessionId_ = 10;
    bool isNeedHandle = true;
    bool isNeedContinue = true;
    for (uint32_t i = 1; i <= 2; i++) {
        info.sequenceId_ = i;
        info.packetId_ = i + 3;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(info, message);
        msgSchedule.PutMsg(message);
    }
    for (uint32_t i = 2; i <= 3; i++) {
        info.sequenceId_ = i;
        info.packetId_ = i;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(info, message);
        msgSchedule.PutMsg(message);
    }
    /**
     * @tc.steps: step2. get msg
     * @tc.expected: drop seq2_packet2, seq3_packet3 and get seq1_packet4, seq2_packet5
     */
    isNeedHandle = true;
    isNeedContinue = true;
    for (uint32_t i = 1; i <= 2; i++) {
        Message *msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
        ASSERT_TRUE(msg != nullptr);
        EXPECT_EQ(isNeedContinue, true);
        EXPECT_EQ(isNeedHandle, true);
        EXPECT_EQ(msg->GetSequenceId(), i);
        const DataRequestPacket *packet = msg->GetObject<DataRequestPacket>();
        EXPECT_EQ(packet->GetPacketId(), i + 3);
        msgSchedule.ScheduleInfoHandle(isNeedHandle, false, msg);
        delete msg;
    }
    Message *msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
    ASSERT_TRUE(msg == nullptr);
    RefObject::KillAndDecObjRef(context);
    context = nullptr;
}

/**
 * @tc.name: MsgSchedule006
 * @tc.desc: Test MsgSchedule funtion with same sessionId and same sequenceId and packetId
 * @tc.type: FUNC
 * @tc.require: 
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerMsgScheduleTest, MsgSchedule006, TestSize.Level0)
{
    /**
     * @tc.steps: step1. put msg seq1_packet1, seq2_packet2
     * @tc.expected: put msg ok
     */
    SingleVerDataMessageSchedule msgSchedule;
    auto *context = new SingleVerSyncTaskContext();
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    DataSyncMessageInfo info;
    info.sessionId_ = 10;
    bool isNeedHandle = true;
    bool isNeedContinue = true;
    for (uint32_t i = 1; i <= 2; i++) {
        info.sequenceId_ = i;
        info.packetId_ = i;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(info, message);
        msgSchedule.PutMsg(message);
    }
    for (uint32_t i = 1; i <= 2; i++) {
        Message *msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
        ASSERT_TRUE(msg != nullptr);
        EXPECT_EQ(isNeedContinue, true);
        EXPECT_EQ(isNeedHandle, true);
        EXPECT_EQ(msg->GetSequenceId(), i);
        msgSchedule.ScheduleInfoHandle(isNeedHandle, false, msg);
        delete msg;
    }
    /**
     * @tc.steps: step2. put msg seq1_packet1, seq2_packet2 again and seq3_packet3
     * @tc.expected: get msg ok and get seq1_packet1, seq2_packet2 and seq3_packet3
     */
    for (uint32_t i = 1; i <= 3; i++) {
        info.sequenceId_ = i;
        info.packetId_ = i;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(info, message);
        msgSchedule.PutMsg(message);
    }
    isNeedHandle = true;
    isNeedContinue = true;
    for (uint32_t i = 1; i <= 3; i++) {
        Message *msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
        ASSERT_TRUE(msg != nullptr);
        EXPECT_EQ(isNeedContinue, true);
        EXPECT_EQ(isNeedHandle, (i == 3) ? true : false);
        EXPECT_EQ(msg->GetSequenceId(), i);
        const DataRequestPacket *packet = msg->GetObject<DataRequestPacket>();
        EXPECT_EQ(packet->GetPacketId(), i);
        msgSchedule.ScheduleInfoHandle(isNeedHandle, false, msg);
        delete msg;
    }
    Message *msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
    ASSERT_TRUE(msg == nullptr);
    RefObject::KillAndDecObjRef(context);
    context = nullptr;
}

/**
 * @tc.name: MsgSchedule007
 * @tc.desc: Test MsgSchedule funtion with same sessionId and duplicate sequenceId and low packetId
 * @tc.type: FUNC
 * @tc.require: 
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerMsgScheduleTest, MsgSchedule007, TestSize.Level0)
{
    /**
     * @tc.steps: step1. put msg seq1_packet4, seq2_packet5
     * @tc.expected: put msg ok and get msg seq1_packet4, seq2_packet5
     */
    SingleVerDataMessageSchedule msgSchedule;
    auto *context = new SingleVerSyncTaskContext();
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    DataSyncMessageInfo info;
    info.sessionId_ = 10;
    bool isNeedHandle = true;
    bool isNeedContinue = true;
    for (uint32_t i = 1; i <= 2; i++) {
        info.sequenceId_ = i;
        info.packetId_ = i + 3;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(info, message);
        msgSchedule.PutMsg(message);
    }
    for (uint32_t i = 1; i <= 2; i++) {
        Message *msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
        ASSERT_TRUE(msg != nullptr);
        EXPECT_EQ(isNeedContinue, true);
        EXPECT_EQ(isNeedHandle, true);
        EXPECT_EQ(msg->GetSequenceId(), i);
        const DataRequestPacket *packet = msg->GetObject<DataRequestPacket>();
        EXPECT_EQ(packet->GetPacketId(), i + 3);
        msgSchedule.ScheduleInfoHandle(isNeedHandle, false, msg);
        delete msg;
    }
    Message *msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
    ASSERT_TRUE(msg == nullptr);
    /**
     * @tc.steps: step2. put msg seq1_packet1, seq2_packet2
     * @tc.expected: get nullptr
     */
    for (uint32_t i = 1; i <= 2; i++) {
        info.sequenceId_ = i;
        info.packetId_ = i;
        DistributedDB::Message *message = nullptr;
        DistributedDBToolsUnitTest::BuildMessage(info, message);
        msgSchedule.PutMsg(message);
    }
    isNeedHandle = true;
    isNeedContinue = true;
    for (uint32_t i = 1; i <= 3; i++) {
        Message *msg = msgSchedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
        EXPECT_EQ(isNeedContinue, true);
        ASSERT_TRUE(msg == nullptr);
    }
    RefObject::KillAndDecObjRef(context);
    context = nullptr;
}