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

#include "ability_sync.h"
#include "distributeddb_tools_unit_test.h"
#include "sync_types.h"
#include "version.h"
#include "virtual_communicator_aggregator.h"
#include "virtual_single_ver_sync_db_Interface.h"
#include "virtual_relational_ver_sync_db_interface.h"

using namespace std;
using namespace testing::ext;
using namespace DistributedDB;

namespace {
    const std::string DEVICE_A = "deviceA";
    const std::string DEVICE_B = "deviceB";
    const std::string TEST_SCHEMA = "{\"SCHEMA_DEFINE\":{\"value\":\"LONG\"},\"SCHEMA_MODE\":\"COMPATIBLE\","
        "\"SCHEMA_VERSION\":\"1.0\"}";

    KvDBInterface *g_iInterface = nullptr;
    VirtualCommunicatorAggregator *g_Comaggre = nullptr;

    ICommunicator *g_communicatorA = nullptr;
    ICommunicator *g_communicatorB = nullptr;
    std::shared_ptr<Metadata> g_meta = nullptr;
}

class KvDBAbilitySyncTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void KvDBAbilitySyncTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: NA
     */
}

void KvDBAbilitySyncTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: NA
     */
}

void KvDBAbilitySyncTest::SetUp(void)
{
    DistributedDBUnitTest::DistributedDBToolsUnitTest::PrintTestCaseInfo();
    /**
     * @tc.setup: create the instance for virtual communicator, virtual storage
     */
    g_iInterface = new (std::nothrow) KvDBInterface();
    EXCEPT_TRUE(g_iInterface != nullptr);
    g_iInterface->SetSchemaInfo(TEST_SCHEMA);
    g_Comaggre = new (std::nothrow) VirtualCommunicatorAggregator;
    EXCEPT_TRUE(g_Comaggre != nullptr);
    int errCode = E_OK;
    g_communicatorA = g_Comaggre->AllocCommunicator(DEVICE_A, errCode);
    EXCEPT_TRUE(g_communicatorA != nullptr);
    g_communicatorB = g_Comaggre->AllocCommunicator(DEVICE_B, errCode);
    EXCEPT_TRUE(g_communicatorB != nullptr);
    g_meta = std::make_shared<Metadata>();
    g_meta->Initialize(g_iInterface);
}

void KvDBAbilitySyncTest::TearDown(void)
{
    /**
     * @tc.teardown: delete the ptr for testing
     */
    if (g_communicatorA != nullptr && g_Comaggre != nullptr) {
        g_Comaggre->ReleaseCommunicator(g_communicatorA);
        g_communicatorA = nullptr;
    }
    if (g_communicatorB != nullptr && g_Comaggre != nullptr) {
        g_Comaggre->ReleaseCommunicator(g_communicatorB);
        g_communicatorB = nullptr;
    }
    if (g_Comaggre != nullptr) {
        RefObject::KillAndDecObjRef(g_Comaggre);
        g_Comaggre = nullptr;
    }
    if (g_iInterface != nullptr) {
        delete g_iInterface;
        g_iInterface = nullptr;
    }
}

/**
 * @tc.name: RequestPacketTest001
 * @tc.desc: Verify RequestPacketSerialization and RequestPacketDeSerialization function.
 * @tc.type: FUNC
 * @tc.require: AR000DR9K4
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBAbilitySyncTest, RequestPacketTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create a AbilityRequestPacket requestPacket
     * @tc.steps: step2. set version = ABILITY_SYNC_VERSION_V1. schema = TEST_SCHEMA.
     */
    AbilitySyncRequestPacket requestPacket;
    DbAbility ability1;
#ifndef OMIT_ZLIB
    ability1.SetAbilityItem(SyncConfig::DATABASE_COMPRESSION_ZLIB, SUPPORT_MARK);
#endif
    requestPacket.SetProtocolVersion(ABILITY_SYNC_VERSION_V1);
    requestPacket.SetSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    requestPacket.SetSchema(TEST_SCHEMA);
    requestPacket.SetSendCode(E_OK);
    requestPacket.SetDbAbility(ability1);
    Message msg1(ABILITY_SYNC_MESSAGE);
    msg1.SetMessageType(TYPE_REQUEST);
    msg1.SetCopiedObject(requestPacket);

    /**
     * @tc.steps: step3. call Serialization to Serialization the msg
     * @tc.expected: step3. Serialization return E_OK
     */
    uint32_t bufflen = requestPacket.CalculateLen();
    EXCEPT_TRUE(bufflen != 0);
    std::vector<uint8_t> vec(bufflen, 0);
    EXCEPT_TRUE(AbilitySync::Serialization(vec.data(), bufflen, &msg1) == E_OK);

    /**
     * @tc.steps: step4. call DeSerialization to DeSerialization the vec
     * @tc.expected: step4. DeSerialization return E_OK
     */
    Message msg2(ABILITY_SYNC_MESSAGE);
    msg2.SetMessageType(TYPE_REQUEST);
    EXCEPT_TRUE(AbilitySync::DeSerialization(vec.data(), bufflen, &msg2) == E_OK);
    const AbilitySyncRequestPacket *requestPacket = msg2.GetObject<AbilitySyncRequestPacket>();
    EXCEPT_TRUE(requestPacket != nullptr);

    /**
     * @tc.expected: step5. requestPacket == requestPacket
     */
    EXPECT_TRUE(requestPacket->GetProtocolVersion() == ABILITY_SYNC_VERSION_V1);
    EXPECT_TRUE(requestPacket->GetSoftwareVersion() == SOFTWARE_VERSION_CURRENT);
    EXPECT_TRUE(requestPacket->GetSendCode() == E_OK);
    EXPECT_TRUE(requestPacket->GetDbAbility() == ability1);
    std::string schema = requestPacket->GetSchema();
    EXPECT_EQ(schema, TEST_SCHEMA);
}

/**
 * @tc.name: RequestPacketTest002
 * @tc.desc: Verify RequestPacketSerialization and RequestPacketDeSerialization function when version not support.
 * @tc.type: FUNC
 * @tc.require: AR000DR9K4
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBAbilitySyncTest, RequestPacketTest002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create a AbilityRequestPacket requestPacket
     * @tc.steps: step2. set version = ABILITY_SYNC_VERSION_V1 + 1. schema = TEST_SCHEMA.
     */
    AbilitySyncRequestPacket requestPacket;
    requestPacket.SetProtocolVersion(ABILITY_SYNC_VERSION_V1 + 1);
    requestPacket.SetSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    requestPacket.SetSchema("");
    Message msg1(ABILITY_SYNC_MESSAGE);
    msg1.SetMessageType(TYPE_REQUEST);
    msg1.SetCopiedObject(requestPacket);

    /**
     * @tc.steps: step3. call Serialization to Serialization the msg
     * @tc.expected: step3. Serialization return E_OK
     */
    uint32_t bufflen = requestPacket.CalculateLen();
    EXCEPT_TRUE(bufflen != 0);
    std::vector<uint8_t> vec(bufflen, 0);
    EXCEPT_TRUE(AbilitySync::Serialization(vec.data(), bufflen, &msg1) == E_OK);

    /**
     * @tc.steps: step4. call DeSerialization to DeSerialization the vec
     * @tc.expected: step4. DeSerialization return E_OK
     */
    Message msg2(ABILITY_SYNC_MESSAGE);
    msg2.SetMessageType(TYPE_REQUEST);
    EXCEPT_TRUE(AbilitySync::DeSerialization(vec.data(), bufflen, &msg2) == E_OK);
    const AbilitySyncRequestPacket *requestPacket = msg2.GetObject<AbilitySyncRequestPacket>();
    EXCEPT_TRUE(requestPacket != nullptr);

    /**
     * @tc.expected: step5. requestPacket->GetSendCode() == -E_VERSION_NOT_SUPPORT
     */
    EXPECT_TRUE(requestPacket->GetSendCode() == -E_VERSION_NOT_SUPPORT);
}

/**
 * @tc.name: RequestPacketTest003
 * @tc.desc: Verify RequestPacketSerialization and RequestPacketDeSerialization function.
 * @tc.type: FUNC
 * @tc.require: AR000DR9K4
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBAbilitySyncTest, RequestPacketTest003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create a AbilityRequestPacket requestPacket
     * @tc.steps: step2. set version = ABILITY_SYNC_VERSION_V1. schema = TEST_SCHEMA.
     */
    AbilitySyncRequestPacket requestPacket;
    requestPacket.SetProtocolVersion(ABILITY_SYNC_VERSION_V1);
    requestPacket.SetSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    requestPacket.SetSchema(TEST_SCHEMA);
    requestPacket.SetSendCode(E_OK);
    int secLabel = 3; // label 3
    int secFlag = 1; // flag 1
    requestPacket.SetSecLabel(secLabel);
    requestPacket.SetSecFlag(secFlag);
    Message msg1(ABILITY_SYNC_MESSAGE);
    msg1.SetMessageType(TYPE_REQUEST);
    msg1.SetCopiedObject(requestPacket);

    /**
     * @tc.steps: step3. call Serialization to Serialization the msg
     * @tc.expected: step3. Serialization return E_OK
     */
    uint32_t bufflen = requestPacket.CalculateLen();
    EXCEPT_TRUE(bufflen != 0);
    std::vector<uint8_t> vec(bufflen, 0);
    EXCEPT_TRUE(AbilitySync::Serialization(vec.data(), bufflen, &msg1) == E_OK);

    /**
     * @tc.steps: step4. call DeSerialization to DeSerialization the vec
     * @tc.expected: step4. DeSerialization return E_OK
     */
    Message msg2(ABILITY_SYNC_MESSAGE);
    msg2.SetMessageType(TYPE_REQUEST);
    EXCEPT_TRUE(AbilitySync::DeSerialization(vec.data(), bufflen, &msg2) == E_OK);
    const AbilitySyncRequestPacket *requestPacket = msg2.GetObject<AbilitySyncRequestPacket>();
    EXCEPT_TRUE(requestPacket != nullptr);

    /**
     * @tc.expected: step5. requestPacket == requestPacket
     */
    EXPECT_TRUE(requestPacket->GetProtocolVersion() == ABILITY_SYNC_VERSION_V1);
    EXPECT_TRUE(requestPacket->GetSoftwareVersion() == SOFTWARE_VERSION_CURRENT);
    EXPECT_TRUE(requestPacket->GetSendCode() == E_OK);
    std::string schema = requestPacket->GetSchema();
    EXPECT_EQ(schema, TEST_SCHEMA);
    EXPECT_TRUE(requestPacket->GetSecFlag() == secFlag);
    EXPECT_TRUE(requestPacket->GetSecLabel() == secLabel);
}

/**
 * @tc.name: RequestPacketTest004
 * @tc.desc: Verify RequestPacketSerialization and RequestPacketDeSerialization function.
 * @tc.type: FUNC
 * @tc.require: AR000DR9K4
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBAbilitySyncTest, RequestPacketTest004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create a AbilityRequestPacket requestPacket
     * @tc.steps: step2. set version = ABILITY_SYNC_VERSION_V1. schema = TEST_SCHEMA.
     */
    AbilitySyncRequestPacket requestPacket;
    requestPacket.SetProtocolVersion(ABILITY_SYNC_VERSION_V1);
    requestPacket.SetSoftwareVersion(SOFTWARE_VERSION_RELEASE_2_0);
    requestPacket.SetSchema(TEST_SCHEMA);
    requestPacket.SetSendCode(E_OK);
    int secLabel = 3; // label 3
    int secFlag = 1; // flag 1
    requestPacket.SetSecLabel(secLabel);
    requestPacket.SetSecFlag(secFlag);
    Message msg1(ABILITY_SYNC_MESSAGE);
    msg1.SetMessageType(TYPE_REQUEST);
    msg1.SetCopiedObject(requestPacket);

    /**
     * @tc.steps: step3. call Serialization to Serialization the msg
     * @tc.expected: step3. Serialization return E_OK
     */
    uint32_t bufflen = requestPacket.CalculateLen();
    EXCEPT_TRUE(bufflen != 0);
    std::vector<uint8_t> vec(bufflen, 0);

    /**
     * @tc.steps: step4. call DeSerialization to DeSerialization the vec
     * @tc.expected: step4. DeSerialization return E_OK
     */
    Message msg2(ABILITY_SYNC_MESSAGE);
    msg2.SetMessageType(TYPE_REQUEST);
    EXCEPT_TRUE(AbilitySync::DeSerialization(vec.data(), bufflen, &msg2) == E_OK);
    const AbilitySyncRequestPacket *requestPacket = msg2.GetObject<AbilitySyncRequestPacket>();
    EXCEPT_TRUE(requestPacket != nullptr);

    /**
     * @tc.expected: step5. requestPacket == requestPacket
     */
    EXPECT_TRUE(requestPacket->GetProtocolVersion() == ABILITY_SYNC_VERSION_V1);
    EXPECT_TRUE(requestPacket->GetSoftwareVersion() == SOFTWARE_VERSION_RELEASE_2_0);
    EXPECT_TRUE(requestPacket->GetSendCode() == E_OK);
    std::string schema = requestPacket->GetSchema();
    EXPECT_EQ(schema, TEST_SCHEMA);
    EXPECT_TRUE(requestPacket->GetSecFlag() == 0);
    EXPECT_TRUE(requestPacket->GetSecLabel() == 0);
}

/**
 * @tc.name: AckPacketTest001
 * @tc.desc: Verify AckPacketSerialization and AckPacketDeSerialization function.
 * @tc.type: FUNC
 * @tc.require: AR000DR9K4
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBAbilitySyncTest, AckPacketTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create a AbilityAckPacket requestPacket
     * @tc.steps: step2. set version = ABILITY_SYNC_VERSION_V1. schema = TEST_SCHEMA.
     */
    AbilitySyncAckPacket requestPacket;
    DbAbility ability1;
#ifndef OMIT_ZLIB
    ability1.SetAbilityItem(SyncConfig::DATABASE_COMPRESSION_ZLIB, SUPPORT_MARK);
#endif
    requestPacket.SetProtocolVersion(ABILITY_SYNC_VERSION_V1);
    requestPacket.SetSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    requestPacket.SetSchema(TEST_SCHEMA);
    requestPacket.SetAckCode(E_VERSION_NOT_SUPPORT);
    requestPacket.SetDbAbility(ability1);
    Message msg1(ABILITY_SYNC_MESSAGE);
    msg1.SetMessageType(TYPE_RESPONSE);
    msg1.SetCopiedObject(requestPacket);

    /**
     * @tc.steps: step3. call Serialization to Serialization the msg
     * @tc.expected: step3. Serialization return E_OK
     */
    uint32_t bufflen = requestPacket.CalculateLen();
    EXCEPT_TRUE(bufflen != 0);
    std::vector<uint8_t> vec(bufflen, 0);
    ASSERT_EQ(AbilitySync::Serialization(vec.data(), bufflen, &msg1), E_OK);

    /**
     * @tc.steps: step4. call DeSerialization to DeSerialization the vec
     * @tc.expected: step4. DeSerialization return E_OK
     */
    Message msg2(ABILITY_SYNC_MESSAGE);
    msg2.SetMessageType(TYPE_RESPONSE);
    EXCEPT_TRUE(AbilitySync::DeSerialization(vec.data(), bufflen, &msg2) == E_OK);
    const AbilitySyncAckPacket *requestPacket = msg2.GetObject<AbilitySyncAckPacket>();
    EXCEPT_TRUE(requestPacket != nullptr);

    /**
     * @tc.expected: step5. requestPacket == requestPacket
     */
    EXPECT_TRUE(requestPacket->GetProtocolVersion() == ABILITY_SYNC_VERSION_V1);
    EXPECT_TRUE(requestPacket->GetSoftwareVersion() == SOFTWARE_VERSION_CURRENT);
    EXPECT_TRUE(requestPacket->GetAckCode() == E_VERSION_NOT_SUPPORT);
    EXPECT_TRUE(requestPacket->GetDbAbility() == ability1);
    std::string schema = requestPacket->GetSchema();
    EXCEPT_TRUE(schema == TEST_SCHEMA);
}

/**
 * @tc.name: SyncStartTest001
 * @tc.desc: Verify Ability sync SyncStart function.
 * @tc.type: FUNC
 * @tc.require: AR000DR9K4
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBAbilitySyncTest, SyncStart001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create a AbilitySync
     */
    AbilitySync async;
    async.Initialize(g_communicatorB, g_iInterface, g_meta, DEVICE_A);

    /**
     * @tc.steps: step2. call SyncStart
     * @tc.expected: step2. SyncStart return E_OK
     */
    EXPECT_EQ(async.SyncStart(1, 1, 1), E_OK);

    /**
     * @tc.steps: step3. disable the communicator
     */
    static_cast<VirtualCommunicator *>(g_communicatorB)->Disable();

    /**
     * @tc.steps: step4. call SyncStart
     * @tc.expected: step4. SyncStart return -E_PERIPHERAL_INTERFACE_FAIL
     */
    EXPECT_TRUE(async.SyncStart(1, 1, 1) == -E_PERIPHERAL_INTERFACE_FAIL);
}
#ifndef OMIT_JSON
/**
 * @tc.name: RequestReceiveTest001
 * @tc.desc: Verify Ability RequestReceive callback.
 * @tc.type: FUNC
 * @tc.require: AR000DR9K4
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBAbilitySyncTest, RequestReceiveTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create a AbilitySync
     */
    AbilitySync async;
    async.Initialize(g_communicatorB, g_iInterface, g_meta, DEVICE_A);

    /**
     * @tc.steps: step2. call RequestRecv, set inMsg nullptr  or set TaskContext nullptr
     * @tc.expected: step2. RequestRecv return -E_INVALID_ARGS
     */
    Message msg1(ABILITY_SYNC_MESSAGE);
    msg1.SetMessageType(TYPE_REQUEST);
    SingleVerSyncTaskContext *TaskContext = new (std::nothrow) SingleVerKvSyncTaskContext();
    EXCEPT_TRUE(TaskContext != nullptr);
    EXPECT_EQ(async.RequestRecv(nullptr, TaskContext), -E_INVALID_ARGS);
    EXPECT_EQ(async.RequestRecv(&msg1, nullptr), -E_INVALID_ARGS);

    /**
     * @tc.steps: step3. call RequestRecv, set inMsg with no requestPacket
     * @tc.expected: step3. RequestRecv return -E_INVALID_ARGS
     */
    EXPECT_EQ(async.RequestRecv(&msg1, TaskContext), -E_INVALID_ARGS);

    /**
     * @tc.steps: step4. create a AbilityRequestkPacket requestPacket
     */
    AbilitySyncRequestPacket requestPacket;
    requestPacket.SetProtocolVersion(ABILITY_SYNC_VERSION_V1);
    requestPacket.SetSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    requestPacket.SetSchema(TEST_SCHEMA);
    msg1.SetCopiedObject(requestPacket);

    /**
     * @tc.steps: step5. call RequestRecv, set inMsg with requestPacket
     * @tc.expected: step5. RequestRecv return ok, GetRemoteSoftwareVersion is SOFTWARE_VERSION_CURRENT
     *     IsSchemaCompatible true
     *
     */
    EXPECT_EQ(async.RequestRecv(&msg1, TaskContext), -E_SECURITY_OPTION_CHECK_ERROR);
    EXPECT_TRUE(TaskContext->GetRemoteSoftwareVersion() == SOFTWARE_VERSION_CURRENT);
    EXPECT_TRUE(TaskContext->GetTaskErrCode() != -E_SCHEMA_MISMATCH);

    /**
     * @tc.steps: step6. call RequestRecv, set inMsg sendCode -E_VERSION_NOT_SUPPORT
     * @tc.expected: step6. RequestRecv return E_VERSION_NOT_SUPPORT
     */
    requestPacket.SetSendCode(-E_VERSION_NOT_SUPPORT);
    msg1.SetCopiedObject(requestPacket);
    EXPECT_EQ(async.RequestRecv(&msg1, TaskContext), -E_VERSION_NOT_SUPPORT);

    /**
     * @tc.steps: step7. call RequestRecv, SetSchema ""
     * @tc.expected: step7. IsSchemaCompatible false
     */
    requestPacket.SetSchema("");
    requestPacket.SetSendCode(E_OK);
    msg1.SetCopiedObject(requestPacket);
    EXPECT_EQ(async.RequestRecv(&msg1, TaskContext), -E_SECURITY_OPTION_CHECK_ERROR);
    EXPECT_FALSE(TaskContext->GetTaskErrCode() != -E_SCHEMA_MISMATCH);
    RefObject::KillAndDecObjRef(TaskContext);
}
#endif
/**
 * @tc.name: AckReceiveTest001
 * @tc.desc: Verify Ability AckReceive callback.
 * @tc.type: FUNC
 * @tc.require: AR000DR9K4
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBAbilitySyncTest, AckReceiveTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create a AbilitySync
     */
    AbilitySync async;
    async.Initialize(g_communicatorB, g_iInterface, g_meta, DEVICE_A);

    /**
     * @tc.steps: step2. call AckRecv, set inMsg nullptr or set TaskContext nullptr
     * @tc.expected: step2. AckRecv return -E_INVALID_ARGS
     */
    SingleVerSyncTaskContext *TaskContext = new (std::nothrow) SingleVerKvSyncTaskContext();
    EXCEPT_TRUE(TaskContext != nullptr);
    Message msg1(ABILITY_SYNC_MESSAGE);
    msg1.SetMessageType(TYPE_RESPONSE);
    EXPECT_EQ(async.AckRecv(nullptr, TaskContext), -E_INVALID_ARGS);
    EXPECT_EQ(async.AckRecv(&msg1, nullptr), -E_INVALID_ARGS);

    /**
     * @tc.steps: step3. call AckRecv, set inMsg with no requestPacket
     * @tc.expected: step3. AckRecv return -E_INVALID_ARGS
     */
    EXPECT_EQ(async.AckRecv(&msg1, TaskContext), -E_INVALID_ARGS);
    EXCEPT_TRUE(TaskContext != nullptr);

    /**
     * @tc.steps: step4. create a AbilityAckPacket requestPacket
     */
    AbilitySyncAckPacket requestPacket;
    requestPacket.SetProtocolVersion(ABILITY_SYNC_VERSION_V1);
    requestPacket.SetSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    requestPacket.SetAckCode(E_OK);
    requestPacket.SetSchema(TEST_SCHEMA);
    msg1.SetCopiedObject(requestPacket);

    /**
     * @tc.steps: step5. call AckRecv, set inMsg with requestPacket
     * @tc.expected: step5. AckRecv return ok GetRemoteSoftwareVersion is SOFTWARE_VERSION_CURRENT
     *     IsSchemaCompatible true;
     */
    EXPECT_EQ(async.AckRecv(&msg1, TaskContext), E_OK);
    EXPECT_TRUE(TaskContext->GetRemoteSoftwareVersion() == SOFTWARE_VERSION_CURRENT);
    EXPECT_TRUE(TaskContext->GetTaskErrCode() != -E_SCHEMA_MISMATCH);

    /**
     * @tc.steps: step6. call RequestRecv, SetSchema ""
     * @tc.expected: step6. IsSchemaCompatible false
     */
    requestPacket.SetSchema("");
    msg1.SetCopiedObject(requestPacket);
    EXPECT_EQ(async.AckRecv(&msg1, TaskContext), E_OK);

    /**
     * @tc.steps: step7. call AckRecv, set inMsg sendCode -E_VERSION_NOT_SUPPORT
     * @tc.expected: step7. return -E_VERSION_NOT_SUPPORT
     */
    requestPacket.SetSchema(TEST_SCHEMA);
    requestPacket.SetAckCode(-E_VERSION_NOT_SUPPORT);
    msg1.SetCopiedObject(requestPacket);
    EXPECT_EQ(async.AckRecv(&msg1, TaskContext), -E_VERSION_NOT_SUPPORT);
    RefObject::KillAndDecObjRef(TaskContext);
}

/**
 * @tc.name: AckReceiveTest002
 * @tc.desc: Verify Ability RDB AckReceive callback.
 * @tc.type: FUNC
 * @tc.require: AR000DR9K4
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBAbilitySyncTest, AckReceiveTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create a AbilitySync
     */
    AbilitySync async;
    VirtualRelationalVerSyncDBInterface *DBInterface = new(std::nothrow) VirtualRelationalVerSyncDBInterface();
    ASSERT_NE(DBInterface, nullptr);
    async.Initialize(g_communicatorB, DBInterface, g_meta, DEVICE_A);
    DBInterface->SetPermitCreateDistributedTable(false);

    /**
     * @tc.steps: step2. call AckRecv, set inMsg nullptr or set TaskContext nullptr
     * @tc.expected: step2. AckRecv return -E_INVALID_ARGS
     */
    auto *TaskContext = new (std::nothrow) SingleVerRelationalSyncTaskContext();
    const std::string RDB_SCHEMA = "{\"TABLE_MODE\":\"SPLIT_BY_DEVICE\","
        "\"SCHEMA_TYPE\":\"RELATIVE\","
        "\"SCHEMA_VERSION\":\"2.1\"}";
    EXCEPT_TRUE(TaskContext != nullptr);
    Message msg1(ABILITY_SYNC_MESSAGE);
    msg1.SetMessageType(TYPE_RESPONSE);
    AbilitySyncAckPacket requestPacket;
    requestPacket.SetProtocolVersion(ABILITY_SYNC_VERSION_V1);
    requestPacket.SetSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    requestPacket.SetAckCode(E_OK);
    requestPacket.SetSchema(RDB_SCHEMA);
    requestPacket.SetSchemaType(static_cast<uint32_t>(SchemaType::RELATIVE));
    msg1.SetCopiedObject(requestPacket);
    EXPECT_EQ(async.AckRecv(&msg1, TaskContext), -E_NOT_SUPPORT);
    EXPECT_EQ(TaskContext->GetTaskErrCode(), -E_NOT_SUPPORT);

    RefObject::KillAndDecObjRef(TaskContext);
    delete DBInterface;
}
