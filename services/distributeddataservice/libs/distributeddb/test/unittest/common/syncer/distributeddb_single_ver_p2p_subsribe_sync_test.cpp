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
#include <thread>

#include "db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "kv_store_nb_delegate.h"
#include "kv_virtual_device.h"
#include "platform_specific.h"
#include "query.h"
#include "query_sync_object.h"
#include "single_ver_data_sync.h"
#include "single_ver_serialize_manager.h"
#include "subscribe_manager.h"
#include "sync_types.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    const string SCHEMA_STORE_ID = "kv_store_sync_schema_test";
    const std::string DEVICE_A = "deviceA";
    const std::string DEVICE_B = "deviceB";
    const std::string DEVICE_C = "deviceC";

    KvStoreDelegateManager g_schemaMgr(SCHEMA_APP_ID, USER_ID);
    KvStoreConfig g_config;
    DistributedDBToolsUnitTest g_tool;
    DBStatus g_schemaKvDelegateStatus = INVALID_ARGS;
    KvStoreNbDelegate* g_schemaKvDelegatePtr = nullptr;
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
    KvVirtualDevice* g_deviceB = nullptr;
    KvVirtualDevice* g_deviceC = nullptr;

    // the type of g_kvDelegateCallback is function<void(DBStatus, KvStoreDelegate*)>
    auto g_schemaKvDelegateCallback = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(g_schemaKvDelegateStatus), std::ref(g_schemaKvDelegatePtr));
    const string SCHEMA_STRING =
    "{\"SCHEMA_VERSION\":\"1.0\","
    "\"SCHEMA_MODE\":\"STRICT\","
    "\"SCHEMA_DEFINE\":{"
    "\"field_name1\":\"BOOL\","
    "\"field_name2\":\"BOOL\","
    "\"field_name3\":\"INTEGER, NOT NULL\","
    "\"field_name4\":\"LONG, DEFAULT 100\","
    "\"field_name5\":\"DOUBLE, NOT NULL, DEFAULT 3.14\","
    "\"field_name6\":\"STRING, NOT NULL, DEFAULT '3.1415'\","
    "\"field_name7\":\"LONG, DEFAULT 100\","
    "\"field_name8\":\"LONG, DEFAULT 100\","
    "\"field_name9\":\"LONG, DEFAULT 100\","
    "\"field_name10\":\"LONG, DEFAULT 100\""
    "},"
    "\"SCHEMA_INDEXES\":[\"$.field_name1\", \"$.field_name2\"]}";

    const std::string SCHEMA_VALUE1 =
    "{\"field_name1\":true,"
    "\"field_name2\":false,"
    "\"field_name3\":10,"
    "\"field_name4\":20,"
    "\"field_name5\":3.14,"
    "\"field_name6\":\"3.1415\","
    "\"field_name7\":100,"
    "\"field_name8\":100,"
    "\"field_name9\":100,"
    "\"field_name10\":100}";

    const std::string SCHEMA_VALUE2 =
    "{\"field_name1\":false,"
    "\"field_name2\":true,"
    "\"field_name3\":100,"
    "\"field_name4\":200,"
    "\"field_name5\":3.14,"
    "\"field_name6\":\"3.1415\","
    "\"field_name7\":100,"
    "\"field_name8\":100,"
    "\"field_name9\":100,"
    "\"field_name10\":100}";
}

class DistributedDBSingleVerP2PSubscribeSyncTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBSingleVerP2PSubscribeSyncTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Init datadir and Virtual Communicator.
     */
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_config.dataDir = g_testDir;
    g_schemaMgr.SetKvStoreConfig(g_config);

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

void DistributedDBSingleVerP2PSubscribeSyncTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void DistributedDBSingleVerP2PSubscribeSyncTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    /**
     * @tc.setup: create virtual device B and get a KvStoreNbDelegate as deviceA
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

void DistributedDBSingleVerP2PSubscribeSyncTest::TearDown(void)
{
    /**
     * @tc.teardown: Release device A, B
     */
    if (g_schemaKvDelegatePtr != nullptr) {
        ASSERT_EQ(g_schemaMgr.CloseKvStore(g_schemaKvDelegatePtr), OK);
        g_schemaKvDelegatePtr = nullptr;
        DBStatus status = g_schemaMgr.DeleteKvStore(SCHEMA_STORE_ID);
        LOGD("delete kv store status %d", status);
        ASSERT_TRUE(status == OK);
    }
    if (g_deviceB != nullptr) {
        delete g_deviceB;
        g_deviceB = nullptr;
    }
    if (g_deviceC != nullptr) {
        delete g_deviceC;
        g_deviceC = nullptr;
    }
    PermissionCheckCallbackV2 nullCallback;
    EXPECT_EQ(g_schemaMgr.SetPermissionCheckCallback(nullCallback), OK);
}

void InitSubSchemaDb()
{
    g_config.dataDir = g_testDir;
    g_schemaMgr.SetKvStoreConfig(g_config);
    KvStoreNbDelegate::Option option;
    option.schema = SCHEMA_STRING;
    g_schemaMgr.GetKvStore(SCHEMA_STORE_ID, option, g_schemaKvDelegateCallback);
    ASSERT_TRUE(g_schemaKvDelegateStatus == OK);
    ASSERT_TRUE(g_schemaKvDelegatePtr != nullptr);
}

void CheckUnFinishedMap(uint32_t sizeA, uint32_t sizeB, std::vector<std::string> &deviceAQueies,
    std::vector<std::string> &deviceBQueies, SubscribeManager &subManager)
{
    std::map<std::string, std::vector<QuerySyncObject>> allSyncQueries;
    subManager.GetAllUnFinishSubQueries(allSyncQueries);
    ASSERT_TRUE(allSyncQueries[DEVICE_A].size() == sizeA);
    ASSERT_TRUE(allSyncQueries[DEVICE_B].size() == sizeB);
    for (auto &item : allSyncQueries[DEVICE_A]) {
        std::string queryId = item.GetIdentify();
        ASSERT_TRUE(std::find(deviceAQueies.begin(), deviceAQueies.end(), queryId) != deviceAQueies.end());
    }
    for (auto &item : allSyncQueries[DEVICE_B]) {
        std::string queryId = item.GetIdentify();
        ASSERT_TRUE(std::find(deviceBQueies.begin(), deviceBQueies.end(), queryId) != deviceBQueies.end());
    }
}

void InitLocalSubscribeMap(QuerySyncObject &queryCommonObj, std::map<std::string, QuerySyncObject> &queryMap,
    std::vector<std::string> &deviceAQueies, std::vector<std::string> &deviceBQueies, SubscribeManager &subManager)
{
    ASSERT_TRUE(subManager.ReserveLocalSubscribeQuery(DEVICE_A, queryCommonObj) == E_OK);
    ASSERT_TRUE(subManager.ActiveLocalSubscribeQuery(DEVICE_A, queryCommonObj) == E_OK);
    ASSERT_TRUE(subManager.ReserveLocalSubscribeQuery(DEVICE_B, queryCommonObj) == E_OK);
    ASSERT_TRUE(subManager.ActiveLocalSubscribeQuery(DEVICE_B, queryCommonObj) == E_OK);
    queryMap[queryCommonObj.GetIdentify()] = queryCommonObj;
    deviceAQueies.push_back(queryCommonObj.GetIdentify());
    deviceBQueies.push_back(queryCommonObj.GetIdentify());
    for (int i = 0; i < 3; i++) { // 3 subscribe
        QuerySyncObject querySyncObj(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a' + i)}));
        deviceAQueies.push_back(querySyncObj.GetIdentify());
        queryMap[querySyncObj.GetIdentify()] = querySyncObj;
        ASSERT_TRUE(subManager.ReserveLocalSubscribeQuery(DEVICE_A, querySyncObj) == E_OK);
        ASSERT_TRUE(subManager.ActiveLocalSubscribeQuery(DEVICE_A, querySyncObj) == E_OK);
    }
    for (int i = 0; i < 1; i++) {
        QuerySyncObject querySyncObj(Query::Select().PrefixKey({'a', static_cast<uint8_t>('b' + i)}));
        deviceBQueies.push_back(querySyncObj.GetIdentify());
        queryMap[querySyncObj.GetIdentify()] = querySyncObj;
        ASSERT_TRUE(subManager.ReserveLocalSubscribeQuery(DEVICE_B, querySyncObj) == E_OK);
        ASSERT_TRUE(subManager.ActiveLocalSubscribeQuery(DEVICE_B, querySyncObj) == E_OK);
    }
}
/**
 * @tc.name: SubscribeRequestTest001
 * @tc.desc: test Serialize/DoSerialize SubscribeRequest
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PSubscribeSyncTest, SubscribeRequestTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. prepare a SubscribeRequest.
     */
    auto packet = new (std::nothrow) SubscribeRequest;
    ASSERT_TRUE(packet != nullptr);
    packet->SetPacketHead(100, SOFTWARE_VERSION_CURRENT, SUBSCRIBE_QUERY_CMD, 1);
    Query query = Query::Select().EqualTo("$.field_name1", 1);
    QuerySyncObject syncQuery(query);
    packet->SetQuery(syncQuery);

    /**
     * @tc.steps: step2. put the SubscribeRequest Packet into a message.
     */
    Message msg;
    msg.SetExternalObject(packet);
    msg.SetMessageId(CONTROL_SYNC_MESSAGE);
    msg.SetMessageType(TYPE_REQUEST);

    /**
     * @tc.steps: step3. Serialization the message to a buffer.
     */
    int len = SingleVerSerializeManager::CalculateLen(&msg);
    LOGE("test leng = %d", len);
    uint8_t *buffer = new (nothrow) uint8_t[len];
    ASSERT_TRUE(buffer != nullptr);
    ASSERT_EQ(SingleVerSerializeManager::Serialization(buffer, len, &msg), E_OK);

    /**
     * @tc.steps: step4. DeSerialization the buffer to a message.
     */
    Message outMsg;
    outMsg.SetMessageId(CONTROL_SYNC_MESSAGE);
    outMsg.SetMessageType(TYPE_REQUEST);
    ASSERT_EQ(SingleVerSerializeManager::DeSerialization(buffer, len, &outMsg), E_OK);

    /**
     * @tc.steps: step5. checkout the outMsg.
     * @tc.expected: step5. outMsg equal the the in msg
     */
    auto outPacket = outMsg.GetObject<SubscribeRequest>();
    EXPECT_EQ(outPacket->GetVersion(), SOFTWARE_VERSION_CURRENT);
    EXPECT_EQ(outPacket->GetSendCode(), 100);
    EXPECT_EQ(outPacket->GetcontrolCmdType(), SUBSCRIBE_QUERY_CMD);
    EXPECT_EQ(outPacket->GetFlag(), 1u);
    EXPECT_EQ(outPacket->GetQuery().GetIdentify(), syncQuery.GetIdentify());
    delete[] buffer;
}

/**
 * @tc.name: ControlAckTest001
 * @tc.desc: test Serialize/DoSerialize ControlAckPacket
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PSubscribeSyncTest, ControlAckTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. prepare a ControlAckPacket.
     */
    ControlAckPacket packet;
    packet.SetPacketHead(-E_NOT_SUPPORT, SOFTWARE_VERSION_CURRENT, SUBSCRIBE_QUERY_CMD, 1);

    /**
     * @tc.steps: step2. put the QuerySyncAckPacket into a message.
     */
    Message msg;
    msg.SetCopiedObject(packet);
    msg.SetMessageId(CONTROL_SYNC_MESSAGE);
    msg.SetMessageType(TYPE_RESPONSE);

    /**
     * @tc.steps: step3. Serialization the message to a buffer.
     */
    int len = SingleVerSerializeManager::CalculateLen(&msg);
    LOGE("test leng = %d", len);
    uint8_t *buffer = new (nothrow) uint8_t[len];
    ASSERT_TRUE(buffer != nullptr);
    int errCode = SingleVerSerializeManager::Serialization(buffer, len, &msg);
    ASSERT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step4. DeSerialization the buffer to a message.
     */
    Message outMsg;
    outMsg.SetMessageId(CONTROL_SYNC_MESSAGE);
    outMsg.SetMessageType(TYPE_RESPONSE);
    errCode = SingleVerSerializeManager::DeSerialization(buffer, len, &outMsg);
    ASSERT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step5. checkout the outMsg.
     * @tc.expected: step5. outMsg equal the the in msg
     */
    auto outPacket = outMsg.GetObject<ControlAckPacket>();
    EXPECT_EQ(outPacket->GetVersion(), SOFTWARE_VERSION_CURRENT);
    EXPECT_EQ(outPacket->GetRecvCode(), -E_NOT_SUPPORT);
    EXPECT_EQ(outPacket->GetcontrolCmdType(), SUBSCRIBE_QUERY_CMD);
    EXPECT_EQ(outPacket->GetFlag(), 1u);
    delete[] buffer;
}

/**
 * @tc.name: subscribeManager001
 * @tc.desc: test subscribe class subscribe local function with one device
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PSubscribeSyncTest, subscribeManager001, TestSize.Level1)
{
    SubscribeManager subManager;
    std::string device = "device_A";
    /**
     * @tc.steps: step1. test one device limit four subscribe queries in local map
     */
    LOGI("============step 1============");
    for (int i = 0; i < 4; i++) {
        QuerySyncObject querySyncObj(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a' + i)}));
        ASSERT_TRUE(subManager.ReserveLocalSubscribeQuery(device, querySyncObj) == E_OK);
        ASSERT_TRUE(subManager.ActiveLocalSubscribeQuery(device, querySyncObj) == E_OK);
    }
    std::vector<QuerySyncObject> subscribeQueries;
    subManager.GetLocalSubscribeQueries(device, subscribeQueries);
    ASSERT_TRUE(subscribeQueries.size() == 4);
    subscribeQueries.clear();
    QuerySyncObject querySyncObj1(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a' + 4)}));
    int errCode = subManager.ReserveLocalSubscribeQuery(device, querySyncObj1);
    ASSERT_TRUE(errCode != E_OK);
    /**
     * @tc.steps: step2. allow to subscribe existed query
     */
    LOGI("============step 2============");
    QuerySyncObject querySyncObj2(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a' + 3)}));
    ASSERT_TRUE(subManager.ReserveLocalSubscribeQuery(device, querySyncObj2) == E_OK);
    ASSERT_TRUE(subManager.ActiveLocalSubscribeQuery(device, querySyncObj2) == E_OK);
    subManager.GetLocalSubscribeQueries(device, subscribeQueries);
    ASSERT_TRUE(subscribeQueries.size() == 4);
    subscribeQueries.clear();
    /**
     * @tc.steps: step3. unsubscribe no existed queries
     */
    LOGI("============step 3============");
    subManager.RemoveLocalSubscribeQuery(device, querySyncObj1);
    subManager.GetLocalSubscribeQueries(device, subscribeQueries);
    ASSERT_TRUE(subscribeQueries.size() == 4);
    subscribeQueries.clear();
    /**
     * @tc.steps: step4. unsubscribe queries
     */
    LOGI("============step 4============");
    for (int i = 0; i < 4; i++) {
        QuerySyncObject querySyncObj(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a' + i)}));
        subManager.RemoveLocalSubscribeQuery(device, querySyncObj);
    }
    subManager.GetLocalSubscribeQueries(device, subscribeQueries);
    ASSERT_TRUE(subscribeQueries.size() == 0);

    /**
     * @tc.steps: step5. reserve twice while subscribe queries
     */
    LOGI("============step 5============");
    ASSERT_TRUE(subManager.ReserveLocalSubscribeQuery(device, querySyncObj2) == E_OK);
    ASSERT_TRUE(subManager.ReserveLocalSubscribeQuery(device, querySyncObj2) == E_OK);
    ASSERT_TRUE(subManager.ActiveLocalSubscribeQuery(device, querySyncObj2) == E_OK);
    subManager.GetLocalSubscribeQueries(device, subscribeQueries);
    ASSERT_TRUE(subscribeQueries.size() == 1);
    subscribeQueries.clear();
    subManager.RemoveLocalSubscribeQuery(device, querySyncObj2);
    subManager.GetLocalSubscribeQueries(device, subscribeQueries);
    ASSERT_TRUE(subscribeQueries.size() == 0);
}

/**
 * @tc.name: subscribeManager002
 * @tc.desc: test subscribe class subscribe remote function with one device
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PSubscribeSyncTest, subscribeManager002, TestSize.Level1)
{
    SubscribeManager subManager;
    std::string device = "device_A";
    /**
     * @tc.steps: step1. test one device limit four subscribe queries in remote map
     */
    LOGI("============step 1============");
    for (int i = 0; i < 4; i++) {
        QuerySyncObject querySyncObj(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a' + i)}));
        ASSERT_TRUE(subManager.ReserveRemoteSubscribeQuery(device, querySyncObj) == E_OK);
        ASSERT_TRUE(subManager.ActiveRemoteSubscribeQuery(device, querySyncObj) == E_OK);
    }
    QuerySyncObject querySyncObj1(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a' + 4)}));
    ASSERT_TRUE(subManager.ReserveRemoteSubscribeQuery(device, querySyncObj1) != E_OK);
    std::vector<std::string> subscribeQueryId;
    subManager.GetRemoteSubscribeQueryIds(device, subscribeQueryId);
ASSERT_TRUE(subscribeQueryId.size() == 4);
    subscribeQueryId.clear();
    /**
     * @tc.steps: step2. allow to subscribe existed query
     */
    LOGI("============step 2============");
    QuerySyncObject querySyncObj2(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a' + 3)}));
    ASSERT_TRUE(subManager.ReserveRemoteSubscribeQuery(device, querySyncObj2) == E_OK);
    ASSERT_TRUE(subManager.ActiveRemoteSubscribeQuery(device, querySyncObj2) == E_OK);
    subManager.GetRemoteSubscribeQueryIds(device, subscribeQueryId);
    ASSERT_TRUE(subscribeQueryId.size() == 4);
    subscribeQueryId.clear();
    /**
     * @tc.steps: step3. unsubscribe no existed queries
     */
    LOGI("============step 3============");
    subManager.RemoveRemoteSubscribeQuery(device, querySyncObj1);
    subManager.GetRemoteSubscribeQueryIds(device, subscribeQueryId);
    ASSERT_TRUE(subscribeQueryId.size() == 4);
    subscribeQueryId.clear();
    /**
     * @tc.steps: step4. unsubscribe queries
     */
    LOGI("============step 4============");
    for (int i = 0; i < 4; i++) {
        QuerySyncObject querySyncObj(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a' + i)}));
        subManager.RemoveRemoteSubscribeQuery(device, querySyncObj);
    }
    subManager.GetRemoteSubscribeQueryIds(device, subscribeQueryId);
    ASSERT_TRUE(subscribeQueryId.size() == 0);
}

/**
 * @tc.name: subscribeManager003
 * @tc.desc: test subscribe class subscribe remote function with multi device
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PSubscribeSyncTest, subscribeManager003, TestSize.Level1)
{
    SubscribeManager subManager;
    std::string device = "device_";
    std::vector<QuerySyncObject> subscribeQueries;
    /**
     * @tc.steps: step1. test mutil device limit 32 devices in remote map and check each device has one subscribe
     */
    LOGI("============step 1============");
    QuerySyncObject querySyncObj(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a' + 1)}));
    for (int i = 0; i < 32; i++) {
        ASSERT_TRUE(subManager.ReserveLocalSubscribeQuery(device + std::to_string(i), querySyncObj) == E_OK);
        ASSERT_TRUE(subManager.ActiveLocalSubscribeQuery(device + std::to_string(i), querySyncObj) == E_OK);
    }
    ASSERT_TRUE(subManager.ReserveLocalSubscribeQuery(device + std::to_string(33), querySyncObj) != E_OK);
    for (int i = 0; i < 32; i++) {
        subManager.GetLocalSubscribeQueries(device + std::to_string(i), subscribeQueries);
        ASSERT_TRUE(subscribeQueries.size() == 1);
        subscribeQueries.clear();
    }
    /**
     * @tc.steps: step2. clear remote subscribe query map and check each device has no subscribe
     */
    LOGI("============step 2============");
    for (int i = 0; i < 32; i++) {
        subManager.ClearLocalSubscribeQuery(device + std::to_string(i));
        subManager.GetLocalSubscribeQueries(device + std::to_string(i), subscribeQueries);
        ASSERT_TRUE(subscribeQueries.size() == 0);
        subscribeQueries.clear();
    }
    /**
     * @tc.steps: step3. test mutil device limit 8 queries in db and check each device has one subscribe
     */
    LOGI("============step 3============");
    for (int i = 0; i < 8; i++) {
        QuerySyncObject querySyncObj(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a' + i)}));
        ASSERT_TRUE(subManager.ReserveLocalSubscribeQuery(device + std::to_string(i), querySyncObj) == E_OK);
        ASSERT_TRUE(subManager.ActiveLocalSubscribeQuery(device + std::to_string(i), querySyncObj) == E_OK);
    }
    QuerySyncObject querySyncObj1(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a' + 8)}));
    ASSERT_TRUE(subManager.ReserveLocalSubscribeQuery(device + std::to_string(8), querySyncObj1) != E_OK);
}

/**
 * @tc.name: subscribeManager004
 * @tc.desc: test subscribe class subscribe remote function with multi device
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PSubscribeSyncTest, subscribeManager004, TestSize.Level1)
{
    SubscribeManager subManager;
    std::string device = "device_";
    std::vector<std::string> subscribeQueryId;
    /**
     * @tc.steps: step1. test mutil device limit 32 devices in remote map and check each device has one subscribe
     */
    LOGI("============step 1============");
    QuerySyncObject querySyncObj(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a' + 1)}));
    for (int i = 0; i < 32; i++) {
        ASSERT_TRUE(subManager.ReserveRemoteSubscribeQuery(device + std::to_string(i), querySyncObj) == E_OK);
        ASSERT_TRUE(subManager.ActiveRemoteSubscribeQuery(device + std::to_string(i), querySyncObj) == E_OK);
    }
    ASSERT_TRUE(subManager.ReserveRemoteSubscribeQuery(device + std::to_string(33), querySyncObj) != E_OK);
    for (int i = 0; i < 32; i++) {
        subManager.GetRemoteSubscribeQueryIds(device + std::to_string(i), subscribeQueryId);
        ASSERT_TRUE(subscribeQueryId.size() == 1);
        subscribeQueryId.clear();
    }
    /**
     * @tc.steps: step2. clear remote subscribe query map and check each device has no subscribe
     */
    LOGI("============step 2============");
    for (int i = 0; i < 32; i++) {
        subManager.ClearRemoteSubscribeQuery(device + std::to_string(i));
        subManager.GetRemoteSubscribeQueryIds(device + std::to_string(i), subscribeQueryId);
        ASSERT_TRUE(subscribeQueryId.size() == 0);
        subscribeQueryId.clear();
    }
    subManager.ClearRemoteSubscribeQuery(device);
    /**
     * @tc.steps: step3. test mutil device limit 8 queries in db and check each device has one subscribe
     */
    LOGI("============step 3============");
    for (int i = 0; i < 8; i++) {
        QuerySyncObject querySyncObj(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a' + i)}));
        ASSERT_TRUE(subManager.ReserveRemoteSubscribeQuery(device + std::to_string(i), querySyncObj) == E_OK);
        ASSERT_TRUE(subManager.ActiveRemoteSubscribeQuery(device + std::to_string(i), querySyncObj) == E_OK);
    }
    QuerySyncObject querySyncObj1(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a' + 8)}));
    ASSERT_TRUE(subManager.ReserveRemoteSubscribeQuery(device + std::to_string(8), querySyncObj1) != E_OK);
}

/**
 * @tc.name: subscribeManager005
 * @tc.desc: test subscribe class subscribe remote function with put into unfinished map
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PSubscribeSyncTest, subscribeManager005, TestSize.Level1)
{
    SubscribeManager subManager;
    std::vector<QuerySyncObject> subscribeQueries;
    std::map<std::string, QuerySyncObject> queryMap;
    std::vector<std::string> deviceAQueies;
    std::vector<std::string> deviceBQueies;
    QuerySyncObject queryCommonObj(Query::Select().PrefixKey({'a'}));
    /**
     * @tc.steps: step1. test one devices has 4 subscribes and another has 2 in local map, put into unfinished map
     */
    LOGI("============step 1============");
    InitLocalSubscribeMap(queryCommonObj, queryMap, deviceAQueies, deviceBQueies, subManager);
    /**
     * @tc.steps: step2. check all device unFinished subscribe queries and put into unfinished map
     */
    LOGI("============step 2============");
    subManager.GetLocalSubscribeQueries(DEVICE_A, subscribeQueries);
    ASSERT_TRUE(subscribeQueries.size() == 4);
    subManager.PutLocalUnFiniedSubQueries(DEVICE_A, subscribeQueries);
    subscribeQueries.clear();
    subManager.GetLocalSubscribeQueries(DEVICE_B, subscribeQueries);
    ASSERT_TRUE(subscribeQueries.size() == 2);
    subManager.PutLocalUnFiniedSubQueries(DEVICE_B, subscribeQueries);
    subscribeQueries.clear();
    /**
     * @tc.steps: step3. get all device unFinished subscribe queries and check
     */
    LOGI("============step 3============");
    CheckUnFinishedMap(4, 2, deviceAQueies, deviceBQueies, subManager);
    /**
     * @tc.steps: step4. active some subscribe queries
     */
    LOGI("============step 4============");
    subManager.ActiveLocalSubscribeQuery(DEVICE_A, queryCommonObj);
    subManager.ActiveLocalSubscribeQuery(DEVICE_A, queryMap[deviceAQueies[3]]);
    subManager.ActiveLocalSubscribeQuery(DEVICE_B, queryMap[deviceBQueies[1]]);
    deviceAQueies.erase(deviceAQueies.begin() + 3);
    deviceAQueies.erase(deviceAQueies.begin());
    queryMap.erase(queryMap[deviceBQueies[1]].GetIdentify());
    deviceBQueies.erase(deviceBQueies.begin() + 1);
    /**
     * @tc.steps: step5. get all device unFinished subscribe queries and check
     */
    LOGI("============step 5============");
    CheckUnFinishedMap(2, 1, deviceAQueies, deviceBQueies, subManager);
    /**
     * @tc.steps: step6. remove left subscribe queries
     */
    LOGI("============step 6============");
    for (int i = 0; i < 2; i++) {
        QuerySyncObject querySyncObj(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a' + i)}));
        subManager.RemoveLocalSubscribeQuery(DEVICE_A, querySyncObj);
    }
    subManager.RemoveLocalSubscribeQuery(DEVICE_A, queryCommonObj);
    subManager.RemoveLocalSubscribeQuery(DEVICE_B, queryCommonObj);
    /**
     * @tc.steps: step7. get all device unFinished subscribe queries and check
     */
    LOGI("============step 7============");
    CheckUnFinishedMap(0, 0, deviceAQueies, deviceBQueies, subManager);
}

/**
 * @tc.name: subscribeSync001
 * @tc.desc: test subscribe normal sync
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PSubscribeSyncTest, subscribeSync001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. InitSchemaDb
    */
    LOGI("============step 1============");
    InitSubSchemaDb();
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    Query query = Query::Select().EqualTo("$.field_name1", 1);
    QuerySyncObject querySyncObj(query);

    /**
     * @tc.steps: step2. deviceB subscribe query to deviceA
    */
    LOGI("============step 2============");
    g_deviceB->Subscribe(querySyncObj, true, 1);

    /**
     * @tc.steps: step3. deviceA put {key1, SCHEMA_VALUE1} and wait 1s
    */
    LOGI("============step 3============");
    Value value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end());
    Key key = {'1'};
    status = g_schemaKvDelegatePtr->Put(key, value);
    EXPECT_EQ(status, OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    /**
     * @tc.steps: step4. deviceB has {key11, SCHEMA_VALUE1}
    */
    LOGI("============step 4============");
    VirtualDataItem item;
    g_deviceB->GetData(key, item);
    EXPECT_TRUE(item.value == value);

    /**
     * @tc.steps: step5. deviceB unsubscribe query to deviceA
    */
    g_deviceB->UnSubscribe(querySyncObj, true, 2);

    /**
     * @tc.steps: step5. deviceA put {key2, SCHEMA_VALUE1} and wait 1s
    */
    LOGI("============step 5============");
    Value value2(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end());
    Key key2 = {'2'};
    status = g_schemaKvDelegatePtr->Put(key2, value2);
    EXPECT_EQ(status, OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    /**
     * @tc.steps: step6. deviceB don't has {key2, SCHEMA_VALUE1}
    */
    LOGI("============step 6============");
    VirtualDataItem item2;
    EXPECT_TRUE(g_deviceB->GetData(key2, item2) != E_OK);
}

/**
 * @tc.name: subscribeSync002
 * @tc.desc: test subscribe sync over 32 devices,limit,orderBy
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PSubscribeSyncTest, subscribeSync002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. InitSchemaDb
    */
    LOGI("============step 1============");
    InitSubSchemaDb();
    std::vector<std::string> devices;
    std::string device = "device_";
    Query query = Query::Select().EqualTo("$.field_name1", 1);

    /**
     * @tc.steps: step2. deviceA subscribe query to 33 devices, and return overlimit
    */
    LOGI("============step 2============");
    for (int i = 0; i < 33; i++) {
        devices.push_back(device + std::to_string(i));
    }
    EXPECT_TRUE(g_schemaKvDelegatePtr->SubscribeRemoteQuery(devices, nullptr, query, true) == OVER_MAX_LIMITS);

    /**
     * @tc.steps: step3. deviceA subscribe query with limit
    */
    LOGI("============step 3============");
    devices.clear();
    devices.push_back("device_B");
    Query query2 = Query::Select().EqualTo("$.field_name1", 1).Limit(20, 0);
    EXPECT_TRUE(g_schemaKvDelegatePtr->SubscribeRemoteQuery(devices, nullptr, query2, true) == NOT_SUPPORT);

    /**
     * @tc.steps: step4. deviceA subscribe query with orderBy
    */
    LOGI("============step 4============");
    Query query3 = Query::Select().EqualTo("$.field_name1", 1).OrderBy("$.field_name7");
    EXPECT_TRUE(g_schemaKvDelegatePtr->SubscribeRemoteQuery(devices, nullptr, query3, true) == NOT_SUPPORT);
}

/**
 * @tc.name: subscribeSync003
 * @tc.desc: test subscribe sync with inkeys query
 * @tc.type: FUNC
 * @tc.require: AR000GOHO7
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBSingleVerP2PSubscribeSyncTest, subscribeSync003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. InitSchemaDb
     */
    LOGI("============step 1============");
    InitSubSchemaDb();
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step2. deviceB subscribe inkeys(k2k4) query to deviceA
     */
    LOGI("============step 2============");
    Query query = Query::Select().InKeys({KEY_2, KEY_4});
    g_deviceB->Subscribe(QuerySyncObject(query), true, 1);

    /**
     * @tc.steps: step3. deviceA put k1-k5 and wait
     */
    LOGI("============step 3============");
    EXPECT_EQ(OK, g_schemaKvDelegatePtr->PutBatch({
        {KEY_1, Value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end())},
        {KEY_2, Value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end())},
        {KEY_3, Value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end())},
        {KEY_4, Value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end())},
        {KEY_5, Value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end())},
    }));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    /**
     * @tc.steps: step4. deviceB has k2k4, has no k1k3k5
     */
    LOGI("============step 4============");
    VirtualDataItem item;
    EXPECT_EQ(g_deviceB->GetData(KEY_2, item), E_OK);
    EXPECT_EQ(item.value, Value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end()));
    EXPECT_EQ(g_deviceB->GetData(KEY_4, item), E_OK);
    EXPECT_EQ(item.value, Value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end()));
    EXPECT_EQ(g_deviceB->GetData(KEY_1, item), -E_NOT_FOUND);
    EXPECT_EQ(g_deviceB->GetData(KEY_3, item), -E_NOT_FOUND);
    EXPECT_EQ(g_deviceB->GetData(KEY_5, item), -E_NOT_FOUND);
}

/**
 * @tc.name: subscribeSync004
 * @tc.desc: test subscribe sync with inkeys query
 * @tc.type: FUNC
 * @tc.require: AR000GOHO7
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBSingleVerP2PSubscribeSyncTest, subscribeSync004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. InitSchemaDb
     */
    LOGI("============step 1============");
    InitSubSchemaDb();
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step2. deviceB subscribe inkeys(k3k5) and equal to query to deviceA
     */
    LOGI("============step 2============");
    Query query = Query::Select().InKeys({KEY_3, KEY_5}).EqualTo("$.field_name3", 100); // 100 for test.
    g_deviceB->Subscribe(QuerySyncObject(query), true, 2);

    /**
     * @tc.steps: step3. deviceA put k1v2,k3v2,k5v1 and wait
     */
    LOGI("============step 3============");
    EXPECT_EQ(OK, g_schemaKvDelegatePtr->PutBatch({
        {KEY_1, Value(SCHEMA_VALUE2.begin(), SCHEMA_VALUE2.end())},
        {KEY_3, Value(SCHEMA_VALUE2.begin(), SCHEMA_VALUE2.end())},
        {KEY_5, Value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end())},
    }));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    /**
     * @tc.steps: step4. deviceB has k3, has no k1k5
     */
    LOGI("============step 4============");
    VirtualDataItem item;
    EXPECT_EQ(g_deviceB->GetData(KEY_3, item), E_OK);
    EXPECT_EQ(item.value, Value(SCHEMA_VALUE2.begin(), SCHEMA_VALUE2.end()));
    EXPECT_EQ(g_deviceB->GetData(KEY_1, item), -E_NOT_FOUND);
    EXPECT_EQ(g_deviceB->GetData(KEY_5, item), -E_NOT_FOUND);
}

/**
 * @tc.name: subscribeSync005
 * @tc.desc: test subscribe sync with inkeys query
 * @tc.type: FUNC
 * @tc.require: AR000GOHO7
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBSingleVerP2PSubscribeSyncTest, subscribeSync005, TestSize.Level1)
{
    /**
     * @tc.steps: step1. InitSchemaDb
     */
    LOGI("============step 1============");
    InitSubSchemaDb();
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step2. deviceB subscribe inkeys(k1, key6) and prefix key "k" query to deviceA
     */
    LOGI("============step 2============");
    Key key6 { 'k', '6' };
    Query query = Query::Select().InKeys({KEY_1, key6}).PrefixKey({ 'k' });
    g_deviceB->Subscribe(QuerySyncObject(query), true, 3);

    /**
     * @tc.steps: step3. deviceA put k1,key6 and wait
     */
    LOGI("============step 3============");
    EXPECT_EQ(OK, g_schemaKvDelegatePtr->PutBatch({
        {key6, Value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end())},
        {KEY_1, Value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end())},
    }));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    /**
     * @tc.steps: step4. deviceB has key6, has no k1
     */
    LOGI("============step 4============");
    VirtualDataItem item;
    EXPECT_EQ(g_deviceB->GetData(key6, item), E_OK);
    EXPECT_EQ(item.value, Value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end()));
    EXPECT_EQ(g_deviceB->GetData(KEY_1, item), -E_NOT_FOUND);
}


/**
 * @tc.name: subscribeSync006
 * @tc.desc: test one device unsubscribe no effect other device
 * @tc.type: FUNC
 * @tc.require: AR000GOHO7
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSubscribeSyncTest, SubscribeSync006, TestSize.Level1)
{
    /**
     * @tc.steps: step1. InitSchemaDb
     */
    LOGI("============step 1============");
    InitSubSchemaDb();
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step2. deviceB unsubscribe inkeys(k1, key6) and prefix key "k" query to deviceA
     */
    LOGI("============step 2============");
    Key key6 { 'k', '6' };
    Query query = Query::Select().InKeys({KEY_1, key6}).PrefixKey({ 'k' });
    g_deviceB->Online();
    g_deviceC->Online();
    g_deviceB->Subscribe(QuerySyncObject(query), true, 3);
    g_deviceC->Subscribe(QuerySyncObject(query), true, 3);

    /**
     * @tc.steps: step3. deviceC unsubscribe
     */
    LOGI("============step 3============");
    g_deviceC->UnSubscribe(QuerySyncObject(query), true, 3);

    /**
     * @tc.steps: step4. deviceA put k1,key6 and wait
     */
    LOGI("============step 4============");
    EXPECT_EQ(OK, g_schemaKvDelegatePtr->PutBatch({
        {key6, Value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end())},
        {KEY_1, Value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end())},
    }));
    std::this_thread::sleep_for(std::chrono::seconds(1));

    /**
     * @tc.steps: step5. deviceB has key6, has no k1
     */
    LOGI("============step 5============");
    VirtualDataItem item;
    EXPECT_EQ(g_deviceB->GetData(key6, item), E_OK);
    EXPECT_EQ(item.value, Value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end()));
    EXPECT_EQ(g_deviceB->GetData(KEY_1, item), -E_NOT_FOUND);
}