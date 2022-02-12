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

#include "db_common.h"
#include "db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "generic_single_ver_kv_entry.h"
#include "kv_store_nb_delegate.h"
#include "kv_virtual_device.h"
#include "platform_specific.h"
#include "query.h"
#include "query_sync_object.h"
#include "single_ver_data_sync.h"
#include "single_ver_serialize_manager.h"
#include "sync_types.h"
#include "virtual_communicator.h"
#include "virtual_communicator_aggregator.h"
#include "virtual_single_ver_sync_db_Interface.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    const string STORE_ID = "kv_store_sync_test";
    const string SCHEMA_STORE_ID = "kv_store_sync_schema_test";
    const std::string DEVICE_B = "deviceB";

    KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
    KvStoreDelegateManager g_schemaMgr(SCHEMA_APP_ID, USER_ID);
    KvStoreConfig g_config;
    DistributedDBToolsUnitTest g_tool;
    DBStatus g_kvDelegateStatus = INVALID_ARGS;
    DBStatus g_schemaKvDelegateStatus = INVALID_ARGS;
    KvStoreNbDelegate* g_kvDelegatePtr = nullptr;
    KvStoreNbDelegate* g_schemaKvDelegatePtr = nullptr;
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
    KvVirtualDevice *g_deviceB = nullptr;

    // the type of g_kvDelegateCallback is function<void(DBStatus, KvStoreDelegate*)>
    auto g_kvDelegateCallback = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(g_kvDelegateStatus), std::ref(g_kvDelegatePtr));
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

class DistributedDBSingleVerP2PQuerySyncTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBSingleVerP2PQuerySyncTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Init datadir and Virtual Communicator.
     */
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
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

void DistributedDBSingleVerP2PQuerySyncTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void DistributedDBSingleVerP2PQuerySyncTest::SetUp(void)
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
}

void DistributedDBSingleVerP2PQuerySyncTest::TearDown(void)
{
    /**
     * @tc.teardown: Release device A, B
     */
    if (g_kvDelegatePtr != nullptr) {
        ASSERT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
        g_kvDelegatePtr = nullptr;
        DBStatus status = g_mgr.DeleteKvStore(STORE_ID);
        LOGD("delete kv store status %d", status);
        ASSERT_TRUE(status == OK);
    }
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
    PermissionCheckCallbackV2 nullCallback;
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(nullCallback), OK);
}

void InitNormalDb()
{
    g_config.dataDir = g_testDir;
    g_mgr.SetKvStoreConfig(g_config);
    KvStoreNbDelegate::Option option;
    g_mgr.GetKvStore(STORE_ID, option, g_kvDelegateCallback);
    ASSERT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
}

void InitSchemaDb()
{
    g_config.dataDir = g_testDir;
    g_schemaMgr.SetKvStoreConfig(g_config);
    KvStoreNbDelegate::Option option;
    option.schema = SCHEMA_STRING;
    g_schemaMgr.GetKvStore(SCHEMA_STORE_ID, option, g_schemaKvDelegateCallback);
    ASSERT_TRUE(g_schemaKvDelegateStatus == OK);
    ASSERT_TRUE(g_schemaKvDelegatePtr != nullptr);
}

/**
 * @tc.name: Normal Sync 001
 * @tc.desc: Test normal push sync for keyprefix data.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, NormalSync001, TestSize.Level1)
{
    InitNormalDb();
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k0, v0} - {k9, v9}
     */
    Key key = {'1'};
    Value value = {'1'};
    const int dataSize = 10;
    for (int i = 0; i < dataSize; i++) {
        key.push_back(i);
        value.push_back(i);
        status = g_kvDelegatePtr->Put(key, value);
        ASSERT_TRUE(status == OK);
        key.pop_back();
        value.pop_back();
    }
    Key key2 = {'2'};
    Value value2 = {'2'};
    status = g_kvDelegatePtr->Put(key2, value2);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step2. deviceA call query sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    Query query = Query::Select().PrefixKey(key);
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result, query);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step3. onComplete should be called, DeviceB have {k1,v1} - {k9, v9}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    VirtualDataItem item;
    for (int i = 0; i < dataSize; i++) {
        key.push_back(i);
        value.push_back(i);
        g_deviceB->GetData(key, item);
        EXPECT_TRUE(item.value == value);
        key.pop_back();
        value.pop_back();
    }
    EXPECT_TRUE(g_deviceB->GetData(key2, item) != E_OK);
}

/**
 * @tc.name: Normal Sync 002
 * @tc.desc: Test normal push sync for limit and offset.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, NormalSync002, TestSize.Level1)
{
    InitNormalDb();
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k0, v0} - {k9, v9}
     */
    Key key = {'1'};
    Value value = {'1'};
    const int dataSize = 10;
    for (int i = 0; i < dataSize; i++) {
        key.push_back(i);
        value.push_back(i);
        status = g_kvDelegatePtr->Put(key, value);
        ASSERT_TRUE(status == OK);
        key.pop_back();
        value.pop_back();
    }

    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    const int limit = 5;
    const int offset = 4;
    Query query = Query::Select().PrefixKey(key).Limit(limit, offset);
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result, query);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step3. onComplete should be called, DeviceB have {k4,v4} {k8, v8}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }

    VirtualDataItem item;
    for (int i = limit - 1; i < limit + offset; i++) {
        key.push_back(i);
        value.push_back(i);
        g_deviceB->GetData(key, item);
        EXPECT_TRUE(item.value == value);
        key.pop_back();
        value.pop_back();
    }
}

/**
 * @tc.name: Normal Sync 001
 * @tc.desc: Test normal push_and_pull sync for keyprefix data.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, NormalSync003, TestSize.Level1)
{
    InitNormalDb();
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k, v}, {b, v}
     */
    Key key = {'1'};
    Value value = {'1'};
    const int dataSize = 10;
    status = g_kvDelegatePtr->Put(key, value);
    Key key2 = {'2'};
    Value value2 = {'2'};
    status = g_kvDelegatePtr->Put(key2, value2);

    /**
     * @tc.steps: step2. deviceB put {b0, v0} - {b9, v9}, {c, v}
     */
    for (int i = 0; i < dataSize; i++) {
        key2.push_back(i);
        value2.push_back(i);
        g_deviceB->PutData(key2, value2, 10 + i, 0);
        key2.pop_back();
        value2.pop_back();
    }
    Key key3 = {'3'};
    Value value3 = {'3'};
    g_deviceB->PutData(key3, value3, 20, 0);

    /**
     * @tc.steps: step2. deviceA call query sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    Query query = Query::Select().PrefixKey(key2);
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result, query);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step3. onComplete should be called, DeviceA have {b0, v0} - {b9, v9}, DeviceB have {b, v}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    VirtualDataItem item;
    Value tmpValue;
    for (int i = 0; i < dataSize; i++) {
        key2.push_back(i);
        value2.push_back(i);
        g_kvDelegatePtr->Get(key2, tmpValue);
        EXPECT_TRUE(tmpValue == value2);
        key2.pop_back();
        value2.pop_back();
    }
    EXPECT_TRUE(g_deviceB->GetData(key, item) != E_OK);
    EXPECT_TRUE(g_deviceB->GetData(key2, item) == E_OK);
    g_kvDelegatePtr->Get(key3, tmpValue);
    EXPECT_TRUE(tmpValue != value3);
}

/**
 * @tc.name: Normal Sync 001
 * @tc.desc: Test normal pull sync for keyprefix data.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, NormalSync004, TestSize.Level1)
{
    InitNormalDb();
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    /**
     * @tc.steps: step1. deviceB put {k1, v1} - {k9, k9}, {b0, v0} - {b9, v9}
     */
    Key key = {'1'};
    Value value = {'1'};
    const int dataSize = 10;
    Key key2 = {'2'};
    Value value2 = {'2'};
    vector<std::pair<Key, Value>> key1Vec;
    vector<std::pair<Key, Value>> key2Vec;
    for (int i = 0; i < dataSize; i++) {
        Key tmpKey(key);
        Value tmpValue(value);
        tmpKey.push_back(i);
        tmpValue.push_back(i);
        key1Vec.push_back(pair<Key, Value> {tmpKey, tmpValue});
    }
    for (int i = 0; i < dataSize; i++) {
        Key tmpKey(key2);
        Value tmpValue(value2);
        tmpKey.push_back(i);
        tmpValue.push_back(i);
        key2Vec.push_back(pair<Key, Value> {tmpKey, tmpValue});
    }
    for (int i = 0; i < dataSize; i++) {
        g_deviceB->PutData(key2Vec[i].first, key2Vec[i].second, 20 + i, 0);
        g_deviceB->PutData(key1Vec[i].first, key1Vec[i].second, 10 + i, 0);
    }

    /**
     * @tc.steps: step2. deviceA call query sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    Query query = Query::Select().PrefixKey(key2);
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result, query);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step3. onComplete should be called, DeviceA have {b0, v0} - {b9, v9}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        EXPECT_TRUE(pair.second == OK);
    }
    VirtualDataItem item;
    Value tmpValue;
    for (int i = 0; i < dataSize; i++) {
        g_kvDelegatePtr->Get(key2Vec[i].first, tmpValue);
        EXPECT_TRUE(tmpValue == key2Vec[i].second);
        g_kvDelegatePtr->Get(key1Vec[i].first, tmpValue);
        EXPECT_TRUE(tmpValue != key1Vec[i].second);
    }
}

/**
 * @tc.name: NormalSync005
 * @tc.desc: Test normal push sync for inkeys query.
 * @tc.type: FUNC
 * @tc.require: AR000GOHO7
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, NormalSync005, TestSize.Level1)
{
    InitNormalDb();
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put K1-K5
     */
    ASSERT_EQ(g_kvDelegatePtr->PutBatch(
        {{KEY_1, VALUE_1}, {KEY_2, VALUE_2}, {KEY_3, VALUE_3}, {KEY_4, VALUE_4}, {KEY_5, VALUE_5}}), OK);

    /**
     * @tc.steps: step2. deviceA sync K2,K4 and wait
     * @tc.expected: step2. sync should return OK.
     */
    Query query = Query::Select().InKeys({KEY_2, KEY_4});
    std::map<std::string, DBStatus> result;
    ASSERT_EQ(g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result, query), OK);

    /**
     * @tc.expected: step3. onComplete should be called.
     */
    ASSERT_EQ(result.size(), devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_EQ(pair.second, OK);
    }

    /**
     * @tc.steps: step4. deviceB have K2K4 and have no K1K3K5.
     * @tc.expected: step4. sync should return OK.
     */
    VirtualDataItem item;
    EXPECT_EQ(g_deviceB->GetData(KEY_2, item), E_OK);
    EXPECT_EQ(item.value, VALUE_2);
    EXPECT_EQ(g_deviceB->GetData(KEY_4, item), E_OK);
    EXPECT_EQ(item.value, VALUE_4);
    EXPECT_EQ(g_deviceB->GetData(KEY_1, item), -E_NOT_FOUND);
    EXPECT_EQ(g_deviceB->GetData(KEY_3, item), -E_NOT_FOUND);
    EXPECT_EQ(g_deviceB->GetData(KEY_5, item), -E_NOT_FOUND);

    /**
     * @tc.steps: step5. deviceA sync with invalid inkeys query
     * @tc.expected: step5. sync failed and the rc is right.
     */
    query = Query::Select().InKeys({});
    result.clear();
    ASSERT_EQ(g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result, query), INVALID_ARGS);

    std::set<Key> keys;
    for (uint8_t i = 0; i < DBConstant::MAX_BATCH_SIZE + 1; i++) {
        Key key = { i };
        keys.emplace(key);
    }
    query = Query::Select().InKeys(keys);
    result.clear();
    ASSERT_EQ(g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result, query), OVER_MAX_LIMITS);

    query = Query::Select().InKeys({{}});
    result.clear();
    ASSERT_EQ(g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result, query), INVALID_ARGS);
}

HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, QueryRequestPacketTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. prepare a QuerySyncRequestPacket.
     */
    auto packet = new (std::nothrow) DataRequestPacket;
    ASSERT_TRUE(packet != nullptr);
    auto kvEntry = new (std::nothrow) GenericSingleVerKvEntry;
    ASSERT_TRUE(kvEntry != nullptr);
    kvEntry->SetTimestamp(1);
    SyncEntry syncData {.entries = {kvEntry}};
#ifndef OMIT_ZLIB
    ASSERT_TRUE(GenericSingleVerKvEntry::Compress(syncData.entries, syncData.compressedEntries,
        {CompressAlgorithm::ZLIB, SOFTWARE_VERSION_CURRENT}) == E_OK);
    packet->SetCompressAlgo(CompressAlgorithm::ZLIB);
    packet->SetFlag(4); // set IS_COMPRESS_DATA flag true
#endif
    packet->SetBasicInfo(-E_NOT_SUPPORT, SOFTWARE_VERSION_CURRENT, SyncModeType::QUERY_PUSH_PULL);
    packet->SetData(syncData.entries);
    packet->SetCompressData(syncData.compressedEntries);
    packet->SetEndWaterMark(INT8_MAX);
    packet->SetWaterMark(INT16_MAX, INT32_MAX, INT64_MAX);
    QuerySyncObject syncQuery(Query::Select().PrefixKey({'2'}));
    packet->SetQuery(syncQuery);
    packet->SetQueryId(syncQuery.GetIdentify());
    packet->SetReserved(std::vector<uint64_t> {INT8_MAX});

    /**
     * @tc.steps: step2. put the QuerySyncRequestPacket into a message.
     */
    Message msg;
    msg.SetExternalObject(packet);
    msg.SetMessageId(QUERY_SYNC_MESSAGE);
    msg.SetMessageType(TYPE_REQUEST);

    /**
     * @tc.steps: step3. Serialization the message to a buffer.
     */
    int len = SingleVerSerializeManager::CalculateLen(&msg);
    vector<uint8_t> buffer(len);
    ASSERT_EQ(SingleVerSerializeManager::Serialization(buffer.data(), buffer.size(), &msg), E_OK);

    /**
     * @tc.steps: step4. DeSerialization the buffer to a message.
     */
    Message outMsg(QUERY_SYNC_MESSAGE);
    outMsg.SetMessageType(TYPE_REQUEST);
    ASSERT_EQ(SingleVerSerializeManager::DeSerialization(buffer.data(), buffer.size(), &outMsg), E_OK);

    /**
     * @tc.steps: step5. checkout the outMsg.
     * @tc.expected: step5. outMsg equal the the in msg
     */
    auto outPacket = outMsg.GetObject<DataRequestPacket>();
    EXPECT_EQ(outPacket->GetVersion(), SOFTWARE_VERSION_CURRENT);
    EXPECT_EQ(outPacket->GetMode(), SyncModeType::QUERY_PUSH_PULL);
    EXPECT_EQ(outPacket->GetEndWaterMark(), static_cast<uint64_t>(INT8_MAX));
    EXPECT_EQ(outPacket->GetLocalWaterMark(), static_cast<uint64_t>(INT16_MAX));
    EXPECT_EQ(outPacket->GetPeerWaterMark(), static_cast<uint64_t>(INT32_MAX));
    EXPECT_EQ(outPacket->GetDeletedWaterMark(), static_cast<uint64_t>(INT64_MAX));
#ifndef OMIT_ZLIB
    EXPECT_EQ(outPacket->GetFlag(), static_cast<uint32_t>(4)); // check IS_COMPRESS_DATA flag true
#endif
    EXPECT_EQ(outPacket->GetQueryId(), syncQuery.GetIdentify());
    EXPECT_EQ(outPacket->GetReserved(), std::vector<uint64_t> {INT8_MAX});
    EXPECT_EQ(outPacket->GetSendCode(), -E_NOT_SUPPORT);
    EXPECT_EQ(outPacket->GetData()[0]->GetTimestamp(), 1u);
}

HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, QueryAckPacketTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. prepare a QuerySyncAckPacket.
     */
    DataAckPacket packet;
    packet.SetVersion(SOFTWARE_VERSION_CURRENT);
    packet.SetData(INT64_MAX);
    packet.SetRecvCode(-E_NOT_SUPPORT);
    std::vector<uint64_t> reserved = {INT8_MAX};
    packet.SetReserved(reserved);

    /**
     * @tc.steps: step2. put the QuerySyncAckPacket into a message.
     */
    Message msg;
    msg.SetCopiedObject(packet);
    msg.SetMessageId(QUERY_SYNC_MESSAGE);
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
    outMsg.SetMessageId(QUERY_SYNC_MESSAGE);
    outMsg.SetMessageType(TYPE_RESPONSE);
    errCode = SingleVerSerializeManager::DeSerialization(buffer, len, &outMsg);
    ASSERT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step5. checkout the outMsg.
     * @tc.expected: step5. outMsg equal the the in msg
     */
    auto outPacket = outMsg.GetObject<DataAckPacket>();
    EXPECT_EQ(outPacket->GetVersion(), SOFTWARE_VERSION_CURRENT);
    EXPECT_EQ(outPacket->GetData(), static_cast<uint64_t>(INT64_MAX));
    std::vector<uint64_t> reserved2 = {INT8_MAX};
    EXPECT_EQ(outPacket->GetReserved(), reserved2);
    EXPECT_EQ(outPacket->GetRecvCode(), -E_NOT_SUPPORT);
    delete[] buffer;
}

/**
 * @tc.name: GetQueryWaterMark 001
 * @tc.desc: Test metaData save and get queryWaterMark.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, GetQueryWaterMark001, TestSize.Level1)
{
    VirtualSingleVerSyncDBInterface storage;
    Metadata meta;

    /**
     * @tc.steps: step1. initialize meta with storage
     * @tc.expected: step1. E_OK
     */
    int errCode = meta.Initialize(&storage);
    ASSERT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step2. save receive and send watermark
     * @tc.expected: step2. E_OK
     */
    WaterMark w1 = 1;
    EXPECT_EQ(meta.SetRecvQueryWaterMark("Q1", "D1", w1), E_OK);
    EXPECT_EQ(meta.SetSendQueryWaterMark("Q1", "D1", w1), E_OK);

    /**
     * @tc.steps: step3. get receive and send watermark
     * @tc.expected: step3. E_OK and get the latest value
     */
    WaterMark w = 0;
    EXPECT_EQ(meta.GetRecvQueryWaterMark("Q1", "D1", w), E_OK);
    EXPECT_EQ(w1, w);
    EXPECT_EQ(meta.GetSendQueryWaterMark("Q1", "D1", w), E_OK);
    EXPECT_EQ(w1, w);

    /**
     * @tc.steps: step4. set peer and local watermark
     * @tc.expected: step4. E_OK
     */
    WaterMark w2 = 2;
    EXPECT_EQ(meta.SaveLocalWaterMark("D1", w2), E_OK);
    EXPECT_EQ(meta.SavePeerWaterMark("D1", w2, true), E_OK);

    /**
     * @tc.steps: step5. get receive and send watermark
     * @tc.expected: step5. E_OK and get the w1
     */
    EXPECT_EQ(meta.GetRecvQueryWaterMark("Q1", "D1", w), E_OK);
    EXPECT_EQ(w2, w);
    EXPECT_EQ(meta.GetSendQueryWaterMark("Q1", "D1", w), E_OK);
    EXPECT_EQ(w2, w);

    /**
     * @tc.steps: step6. set peer and local watermark
     * @tc.expected: step6. E_OK
     */
    WaterMark w3 = 3;
    EXPECT_EQ(meta.SaveLocalWaterMark("D2", w3), E_OK);
    EXPECT_EQ(meta.SavePeerWaterMark("D2", w3, true), E_OK);

    /**
     * @tc.steps: step7. get receive and send watermark
     * @tc.expected: step7. E_OK and get the w3
     */
    EXPECT_EQ(meta.GetRecvQueryWaterMark("Q2", "D2", w), E_OK);
    EXPECT_EQ(w3, w);
    EXPECT_EQ(meta.GetSendQueryWaterMark("Q2", "D2", w), E_OK);
    EXPECT_EQ(w3, w);

    /**
     * @tc.steps: step8. get not exit receive and send watermark
     * @tc.expected: step8. E_OK and get the 0
     */
    EXPECT_EQ(meta.GetRecvQueryWaterMark("Q3", "D3", w), E_OK);
    EXPECT_EQ(w, 0u);
    EXPECT_EQ(meta.GetSendQueryWaterMark("Q3", "D3", w), E_OK);
    EXPECT_EQ(w, 0u);
}

/**
 * @tc.name: GetQueryWaterMark 002
 * @tc.desc: Test metaData save and get queryWaterMark after push or pull mode.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, GetQueryWaterMark002, TestSize.Level1)
{
    VirtualSingleVerSyncDBInterface storage;
    Metadata meta;

    /**
     * @tc.steps: step1. initialize meta with storage
     * @tc.expected: step1. E_OK
     */
    int errCode = meta.Initialize(&storage);
    ASSERT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step2. set peer and local watermark
     * @tc.expected: step2. E_OK
     */
    WaterMark w1 = 2;
    EXPECT_EQ(meta.SaveLocalWaterMark("D1", w1), E_OK);
    EXPECT_EQ(meta.SavePeerWaterMark("D1", w1, true), E_OK);

    /**
     * @tc.steps: step2. save receive and send watermark
     * @tc.expected: step2. E_OK
     */
    WaterMark w2 = 1;
    EXPECT_EQ(meta.SetRecvQueryWaterMark("Q1", "D1", w2), E_OK);
    EXPECT_EQ(meta.SetSendQueryWaterMark("Q1", "D1", w2), E_OK);

    /**
     * @tc.steps: step3. get receive and send watermark
     * @tc.expected: step3. E_OK and get the bigger value
     */
    WaterMark w = 0;
    EXPECT_EQ(meta.GetRecvQueryWaterMark("Q1", "D1", w), E_OK);
    EXPECT_EQ(w1, w);
    EXPECT_EQ(meta.GetSendQueryWaterMark("Q1", "D1", w), E_OK);
    EXPECT_EQ(w1, w);
}

/**
 * @tc.name: ClearQueryWaterMark 001
 * @tc.desc: Test metaData clear watermark function.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, ClearQueryWaterMark001, TestSize.Level1)
{
    VirtualSingleVerSyncDBInterface storage;
    Metadata meta;

    /**
     * @tc.steps: step1. initialize meta with storage
     * @tc.expected: step1. E_OK
     */
    int errCode = meta.Initialize(&storage);
    ASSERT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step2. save receive watermark
     * @tc.expected: step2. E_OK
     */
    WaterMark w1 = 1;
    EXPECT_EQ(meta.SetRecvQueryWaterMark("Q1", "D1", w1), E_OK);

    /**
     * @tc.steps: step3. erase peer watermark
     * @tc.expected: step3. E_OK
     */
    EXPECT_EQ(meta.EraseDeviceWaterMark("D1", true), E_OK);

    /**
     * @tc.steps: step4. get receive watermark
     * @tc.expected: step4. E_OK receive watermark is zero
     */
    WaterMark w2 = -1;
    EXPECT_EQ(meta.GetRecvQueryWaterMark("Q1", "D1", w2), E_OK);
    EXPECT_EQ(w2, 0u);

    /**
     * @tc.steps: step5. set peer watermark
     * @tc.expected: step5. E_OK
     */
    WaterMark w3 = 2;
    EXPECT_EQ(meta.SavePeerWaterMark("D1", w3, true), E_OK);

    /**
     * @tc.steps: step6. get receive watermark
     * @tc.expected: step6. E_OK receive watermark is peer watermark
     */
    WaterMark w4 = -1;
    EXPECT_EQ(meta.GetRecvQueryWaterMark("Q1", "D1", w4), E_OK);
    EXPECT_EQ(w4, w3);
}

/**
 * @tc.name: ClearQueryWaterMark 002
 * @tc.desc: Test metaData clear watermark function.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, ClearQueryWaterMark002, TestSize.Level1)
{
    VirtualSingleVerSyncDBInterface storage;
    Metadata meta;

    /**
     * @tc.steps: step1. initialize meta with storage
     * @tc.expected: step1. E_OK
     */
    int errCode = meta.Initialize(&storage);
    ASSERT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step2. save receive watermark
     * @tc.expected: step2. E_OK
     */
    WaterMark w1 = 1;
    EXPECT_EQ(meta.SetRecvQueryWaterMark("Q1", "D1", w1), E_OK);
    EXPECT_EQ(meta.SetRecvQueryWaterMark("Q2", "D1", w1), E_OK);
    EXPECT_EQ(meta.SetRecvQueryWaterMark("Q1", "D2", w1), E_OK);

    /**
     * @tc.steps: step3. erase peer watermark, make sure data remove in db
     * @tc.expected: step3. E_OK
     */
    Metadata anotherMeta;
    ASSERT_EQ(anotherMeta.Initialize(&storage), E_OK);
    EXPECT_EQ(anotherMeta.EraseDeviceWaterMark("D1", true), E_OK);

    /**
     * @tc.steps: step4. get receive watermark
     * @tc.expected: step4. E_OK receive watermark is zero
     */
    WaterMark w2 = -1;
    EXPECT_EQ(anotherMeta.GetRecvQueryWaterMark("Q1", "D1", w2), E_OK);
    EXPECT_EQ(w2, 0u);
    w2 = -1;
    EXPECT_EQ(anotherMeta.GetRecvQueryWaterMark("Q2", "D1", w2), E_OK);
    EXPECT_EQ(w2, 0u);
    w2 = -1;
    EXPECT_EQ(anotherMeta.GetRecvQueryWaterMark("Q1", "D2", w2), E_OK);
    EXPECT_EQ(w2, w1);
}

/**
 * @tc.name: GetDeleteKeyWaterMark 001
 * @tc.desc: Test metaData save and get deleteWaterMark.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, GetDeleteKeyWaterMark001, TestSize.Level1)
{
    VirtualSingleVerSyncDBInterface storage;
    Metadata meta;

    /**
     * @tc.steps: step1. initialize meta with storage
     * @tc.expected: step1. E_OK
     */
    int errCode = meta.Initialize(&storage);
    ASSERT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step2. save receive and send watermark
     * @tc.expected: step2. E_OK
     */
    WaterMark w1 = 1;
    EXPECT_EQ(meta.SetRecvDeleteSyncWaterMark("D1", w1), E_OK);
    EXPECT_EQ(meta.SetSendDeleteSyncWaterMark("D1", w1), E_OK);

    /**
     * @tc.steps: step3. get receive and send watermark
     * @tc.expected: step3. E_OK and get the latest value
     */
    WaterMark w = 0;
    EXPECT_EQ(meta.GetRecvDeleteSyncWaterMark("D1", w), E_OK);
    EXPECT_EQ(w1, w);
    EXPECT_EQ(meta.GetSendDeleteSyncWaterMark("D1", w), E_OK);
    EXPECT_EQ(w1, w);

    /**
     * @tc.steps: step4. set peer and local watermark
     * @tc.expected: step4. E_OK
     */
    WaterMark w2 = 2;
    EXPECT_EQ(meta.SaveLocalWaterMark("D1", w2), E_OK);
    EXPECT_EQ(meta.SavePeerWaterMark("D1", w2, true), E_OK);

    /**
     * @tc.steps: step5. get receive and send watermark
     * @tc.expected: step5. E_OK and get the w1
     */
    EXPECT_EQ(meta.GetRecvDeleteSyncWaterMark("D1", w), E_OK);
    EXPECT_EQ(w2, w);
    EXPECT_EQ(meta.GetSendDeleteSyncWaterMark("D1", w), E_OK);
    EXPECT_EQ(w2, w);

    /**
     * @tc.steps: step6. set peer and local watermark
     * @tc.expected: step6. E_OK
     */
    WaterMark w3 = 3;
    EXPECT_EQ(meta.SaveLocalWaterMark("D2", w3), E_OK);
    EXPECT_EQ(meta.SavePeerWaterMark("D2", w3, true), E_OK);

    /**
     * @tc.steps: step7. get receive and send watermark
     * @tc.expected: step7. E_OK and get the w3
     */
    EXPECT_EQ(meta.GetRecvDeleteSyncWaterMark("D2", w), E_OK);
    EXPECT_EQ(w3, w);
    EXPECT_EQ(meta.GetSendDeleteSyncWaterMark("D2", w), E_OK);
    EXPECT_EQ(w3, w);

    /**
     * @tc.steps: step8. get not exit receive and send watermark
     * @tc.expected: step8. E_OK and get the 0
     */
    EXPECT_EQ(meta.GetRecvDeleteSyncWaterMark("D3", w), E_OK);
    EXPECT_EQ(w, 0u);
    EXPECT_EQ(meta.GetSendDeleteSyncWaterMark("D3", w), E_OK);
    EXPECT_EQ(w, 0u);
}

/**
 * @tc.name: GetDeleteKeyWaterMark 002
 * @tc.desc: Test metaData save and get deleteWaterMark after push or pull mode.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, GetDeleteKeyWaterMark002, TestSize.Level1)
{
    VirtualSingleVerSyncDBInterface storage;
    Metadata meta;

    /**
     * @tc.steps: step1. initialize meta with storage
     * @tc.expected: step1. E_OK
     */
    int errCode = meta.Initialize(&storage);
    ASSERT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step2. set peer and local watermark
     * @tc.expected: step2. E_OK
     */
    WaterMark w1 = 3;
    EXPECT_EQ(meta.SaveLocalWaterMark("D1", w1), E_OK);
    EXPECT_EQ(meta.SavePeerWaterMark("D1", w1, true), E_OK);

    /**
     * @tc.steps: step2. save receive and send watermark
     * @tc.expected: step2. E_OK
     */
    WaterMark w2 = 1;
    EXPECT_EQ(meta.SetRecvDeleteSyncWaterMark("D1", w2), E_OK);
    EXPECT_EQ(meta.SetSendDeleteSyncWaterMark("D1", w2), E_OK);

    /**
     * @tc.steps: step3. get receive and send watermark
     * @tc.expected: step3. E_OK and get the bigger value
     */
    WaterMark w = 0;
    EXPECT_EQ(meta.GetRecvDeleteSyncWaterMark("D1", w), E_OK);
    EXPECT_EQ(w1, w);
    EXPECT_EQ(meta.GetSendDeleteSyncWaterMark("D1", w), E_OK);
    EXPECT_EQ(w1, w);
}

/**
 * @tc.name: ClearDeleteKeyWaterMark 001
 * @tc.desc: Test metaData clear watermark function.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, ClearDeleteKeyWaterMark001, TestSize.Level1)
{
    VirtualSingleVerSyncDBInterface storage;
    Metadata meta;

    /**
     * @tc.steps: step1. initialize meta with storage
     * @tc.expected: step1. E_OK
     */
    int errCode = meta.Initialize(&storage);
    ASSERT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step2. save receive watermark
     * @tc.expected: step2. E_OK
     */
    WaterMark w1 = 1;
    EXPECT_EQ(meta.SetRecvDeleteSyncWaterMark("D1", w1), E_OK);

    /**
     * @tc.steps: step3. erase peer watermark
     * @tc.expected: step3. E_OK
     */
    EXPECT_EQ(meta.EraseDeviceWaterMark("D1", true), E_OK);

    /**
     * @tc.steps: step4. get receive watermark
     * @tc.expected: step4. E_OK receive watermark is zero
     */
    WaterMark w2 = -1;
    EXPECT_EQ(meta.GetRecvDeleteSyncWaterMark("D1", w2), E_OK);
    EXPECT_EQ(w2, 0u);

    /**
     * @tc.steps: step5. set peer watermark
     * @tc.expected: step5. E_OK
     */
    WaterMark w3 = 2;
    EXPECT_EQ(meta.SavePeerWaterMark("D1", w3, true), E_OK);

    /**
     * @tc.steps: step6. get receive watermark
     * @tc.expected: step6. E_OK receive watermark is peer watermark
     */
    WaterMark w4 = -1;
    EXPECT_EQ(meta.GetRecvDeleteSyncWaterMark("D1", w4), E_OK);
    EXPECT_EQ(w4, w3);
}

/**
 * @tc.name: VerifyCacheAndDb 001
 * @tc.desc: Test metaData watermark cache and db are consistent and correct.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, VerifyMetaDataQuerySync001, TestSize.Level1)
{
    Metadata meta;
    VirtualSingleVerSyncDBInterface storage;

    /**
     * @tc.steps: step1. initialize meta with storage
     * @tc.expected: step1. E_OK
     */
    int errCode = meta.Initialize(&storage);
    ASSERT_EQ(errCode, E_OK);

    const std::string deviceId = "D1";
    const std::string queryId = "Q1";

    /**
    * @tc.steps: step2. save deleteSync watermark
    * @tc.expected: step2. E_OK
    */
    WaterMark deleteWaterMark = 1;
    EXPECT_EQ(meta.SetRecvDeleteSyncWaterMark(deviceId, deleteWaterMark), E_OK);
    EXPECT_EQ(meta.SetSendDeleteSyncWaterMark(deviceId, deleteWaterMark), E_OK);

    /**
    * @tc.steps: step3. save querySync watermark
    * @tc.expected: step2. E_OK
    */
    WaterMark queryWaterMark = 2;
    EXPECT_EQ(meta.SetRecvQueryWaterMark(queryId, deviceId, queryWaterMark), E_OK);
    EXPECT_EQ(meta.SetSendQueryWaterMark(queryId, deviceId, queryWaterMark), E_OK);

    /**
    * @tc.steps: step4. initialize meta with storage
    * @tc.expected: step4. E_OK
    */
    Metadata anotherMeta;
    ASSERT_EQ(anotherMeta.Initialize(&storage), E_OK);

    /**
    * @tc.steps: step5. verify delete sync data
    * @tc.expected: step5. E_OK and waterMark equal to deleteWaterMark
    */
    WaterMark waterMark;
    EXPECT_EQ(anotherMeta.GetRecvDeleteSyncWaterMark(deviceId, waterMark), E_OK);
    EXPECT_EQ(waterMark, deleteWaterMark);
    EXPECT_EQ(anotherMeta.GetSendDeleteSyncWaterMark(deviceId, waterMark), E_OK);
    EXPECT_EQ(waterMark, deleteWaterMark);

    /**
    * @tc.steps: step6. verify query sync data
    * @tc.expected: step6. E_OK and waterMark equal to queryWaterMark
    */
    EXPECT_EQ(anotherMeta.GetRecvQueryWaterMark(queryId, deviceId, waterMark), E_OK);
    EXPECT_EQ(waterMark, queryWaterMark);
    EXPECT_EQ(anotherMeta.GetSendQueryWaterMark(queryId, deviceId, waterMark), E_OK);
    EXPECT_EQ(waterMark, queryWaterMark);
}

/**
 * @tc.name: VerifyLruMap 001
 * @tc.desc: Test metaData watermark cache lru ability.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, VerifyLruMap001, TestSize.Level1)
{
    LruMap lruMap;
    const int maxCacheItems = 200;

    /**
    * @tc.steps: step1. fill items to LruMap
    * @tc.expected: step1. E_OK
    */
    const int startCount = 0;
    for (int i = startCount; i < maxCacheItems; i++) {
        std::string key = std::to_string(i);
        QueryWaterMark value;
        value.recvWaterMark = i + 1;
        EXPECT_EQ(lruMap.Put(key, value), E_OK);
    }

    /**
    * @tc.steps: step2. get the first item
    * @tc.expected: step2. E_OK first item will move to last
    */
    std::string firstItemKey = std::to_string(startCount);
    QueryWaterMark firstItemValue;
    EXPECT_EQ(lruMap.Get(firstItemKey, firstItemValue), E_OK);
    EXPECT_EQ(firstItemValue.recvWaterMark, 1u);

    /**
    * @tc.steps: step3. insert new items to LruMap
    * @tc.expected: step3. the second items was removed
    */
    std::string key = std::to_string(maxCacheItems);
    QueryWaterMark value;
    value.recvWaterMark = maxCacheItems;
    EXPECT_EQ(lruMap.Put(key, value), E_OK);

    /**
    * @tc.steps: step4. get the second item
    * @tc.expected: step4. E_NOT_FOUND it was removed by algorithm
    */
    std::string secondItemKey = std::to_string(startCount + 1);
    QueryWaterMark secondItemValue;
    EXPECT_EQ(lruMap.Get(secondItemKey, secondItemValue), -E_NOT_FOUND);
    EXPECT_EQ(secondItemValue.recvWaterMark, 0u);
}

/**
 * @tc.name: VerifyMetaDataInit 001
 * @tc.desc: Test metaData init correctly
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, VerifyMetaDataInit001, TestSize.Level1)
{
    Metadata meta;
    VirtualSingleVerSyncDBInterface storage;

    /**
    * @tc.steps: step1. initialize meta with storage
    * @tc.expected: step1. E_OK
    */
    ASSERT_EQ(meta.Initialize(&storage), E_OK);

    DeviceID deviceA = "DeviceA";
    DeviceID deviceB = "DeviceA";
    WaterMark setWaterMark = 1;

    /**
    * @tc.steps: step2. meta save and get waterMark
    * @tc.expected: step2. expect get the same waterMark
    */
    EXPECT_EQ(meta.SaveLocalWaterMark(deviceA, setWaterMark), E_OK);
    EXPECT_EQ(meta.SaveLocalWaterMark(deviceB, setWaterMark), E_OK);
    WaterMark getWaterMark = 0;
    meta.GetLocalWaterMark(deviceA, getWaterMark);
    EXPECT_EQ(getWaterMark, setWaterMark);
    meta.GetLocalWaterMark(deviceB, getWaterMark);
    EXPECT_EQ(getWaterMark, setWaterMark);


    /**
    * @tc.steps: step3. init again
    * @tc.expected: step3. E_OK
    */
    Metadata anotherMeta;
    ASSERT_EQ(anotherMeta.Initialize(&storage), E_OK);

    /**
    * @tc.steps: step4. get waterMark again
    * @tc.expected: step4. expect get the same waterMark
    */
    anotherMeta.GetLocalWaterMark(deviceA, getWaterMark);
    EXPECT_EQ(getWaterMark, setWaterMark);
    anotherMeta.GetLocalWaterMark(deviceB, getWaterMark);
    EXPECT_EQ(getWaterMark, setWaterMark);
}

namespace {
void InitVerifyStorageEnvironment(Metadata &meta, VirtualSingleVerSyncDBInterface &storage,
    const std::string &deviceId, const int &startCount, const uint32_t &maxStoreItems)
{
    /**
    * @tc.steps: step1. initialize meta with storage
    * @tc.expected: step1. E_OK
    */
    ASSERT_EQ(meta.Initialize(&storage), E_OK);

    /**
    * @tc.steps: step2. fill items to metadata
    * @tc.expected: step2. E_OK
    */
    for (uint32_t i = startCount; i < maxStoreItems; i++) {
        std::string queryId = std::to_string(i);
        WaterMark recvWaterMark = i + 1;
        EXPECT_EQ(meta.SetRecvQueryWaterMark(queryId, deviceId, recvWaterMark), E_OK);
    }
}
}

/**
 * @tc.name: VerifyManagerQuerySyncStorage 001
 * @tc.desc: Test metaData remove least used querySync storage items.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, VerifyManagerQuerySyncStorage001, TestSize.Level3)
{
    Metadata meta;
    VirtualSingleVerSyncDBInterface storage;
    const uint32_t maxStoreItems = 100000;
    const int startCount = 0;
    const std::string deviceId = "Device";

    InitVerifyStorageEnvironment(meta, storage, deviceId, startCount, maxStoreItems);

    /**
    * @tc.steps: step3. insert new items to metadata
    * @tc.expected: step3. E_OK
    */
    std::string newQueryId = std::to_string(maxStoreItems);
    WaterMark newWaterMark = maxStoreItems + 1;
    EXPECT_EQ(meta.SetRecvQueryWaterMark(newQueryId, deviceId, newWaterMark), E_OK);

    /**
    * @tc.steps: step4. touch the first item
    * @tc.expected: step4. E_OK update first item used time
    */
    std::string firstItemKey = std::to_string(startCount);
    WaterMark firstWaterMark = 11u;
    EXPECT_EQ(meta.SetRecvQueryWaterMark(firstItemKey, deviceId, firstWaterMark), E_OK);

    /**
    * @tc.steps: step5. initialize new meta with storage
    * @tc.expected: step5. the second item will be removed
    */
    Metadata newMeta;
    ASSERT_EQ(newMeta.Initialize(&storage), E_OK);

    /**
    * @tc.steps: step6. touch the first item
    * @tc.expected: step6. E_OK it still exist
    */
    WaterMark exceptWaterMark;
    EXPECT_EQ(newMeta.GetRecvQueryWaterMark(firstItemKey, deviceId, exceptWaterMark), E_OK);
    EXPECT_EQ(exceptWaterMark, firstWaterMark);

    /**
    * @tc.steps: step7. get the second item
    * @tc.expected: step7. NOT_FOUND secondWaterMark is zero
    */
    WaterMark secondWaterMark;
    std::string secondQueryId = std::to_string(startCount + 1);
    EXPECT_EQ(newMeta.GetRecvQueryWaterMark(secondQueryId, deviceId, secondWaterMark), E_OK);
    EXPECT_EQ(secondWaterMark, 0u);
}

/**
 * @tc.name: VerifyMetaDbCreateTime 001
 * @tc.desc: Test metaData get and set cbCreateTime.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, VerifyMetaDbCreateTime001, TestSize.Level1)
{
    Metadata meta;
    VirtualSingleVerSyncDBInterface storage;
    /**
     * @tc.steps: step1. initialize meta with storage
     * @tc.expected: step1. E_OK
     */
    int errCode = meta.Initialize(&storage);
    ASSERT_EQ(errCode, E_OK);
    /**
     * @tc.steps: step2. set local and peer watermark and dbCreateTime
     * @tc.expected: step4. E_OK
     */
    WaterMark value = 2;
    EXPECT_EQ(meta.SaveLocalWaterMark("D1", value), E_OK);
    EXPECT_EQ(meta.SavePeerWaterMark("D1", value, true), E_OK);
    EXPECT_EQ(meta.SetDbCreateTime("D1", 10u, true), E_OK);
    /**
     * @tc.steps: step3. check peer and local watermark and dbCreateTime
     * @tc.expected: step4. E_OK
     */
    WaterMark curValue = 0;
    meta.GetLocalWaterMark("D1", curValue);
    EXPECT_EQ(value, curValue);
    meta.GetPeerWaterMark("D1", curValue);
    EXPECT_EQ(value, curValue);
    uint64_t curDbCreatTime = 0;
    meta.GetDbCreateTime("D1", curDbCreatTime);
    EXPECT_EQ(curDbCreatTime, 10u);
    /**
     * @tc.steps: step3. change dbCreateTime and check
     * @tc.expected: step4. E_OK
     */
    EXPECT_EQ(meta.SetDbCreateTime("D1", 20u, true), E_OK);
    uint64_t clearDeviceDataMark = INT_MAX;
    meta.GetRemoveDataMark("D1", clearDeviceDataMark);
    EXPECT_EQ(clearDeviceDataMark, 1u);
    EXPECT_EQ(meta.ResetMetaDataAfterRemoveData("D1"), E_OK);
    meta.GetRemoveDataMark("D1", clearDeviceDataMark);
    EXPECT_EQ(clearDeviceDataMark, 0u);
    meta.GetDbCreateTime("D1", curDbCreatTime);
    EXPECT_EQ(curDbCreatTime, 20u);
}

/**
 * @tc.name: VerifyManagerQuerySyncStorage 002
 * @tc.desc: Test metaData remove least used querySync storage items when exit wrong data.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, VerifyManagerQuerySyncStorage002, TestSize.Level3)
{
    Metadata meta;
    VirtualSingleVerSyncDBInterface storage;
    const uint32_t maxStoreItems = 100000;
    const int startCount = 0;
    const std::string deviceId = "Device";

    InitVerifyStorageEnvironment(meta, storage, deviceId, startCount, maxStoreItems);

    /**
    * @tc.steps: step3. insert a wrong Value
    * @tc.expected: step3. E_OK
    */
    std::string newQueryId = std::to_string(maxStoreItems);
    Key dbKey;
    DBCommon::StringToVector(QuerySyncWaterMarkHelper::GetQuerySyncPrefixKey()
        + DBCommon::TransferHashString(deviceId) + newQueryId, dbKey);
    Value wrongValue;
    EXPECT_EQ(storage.PutMetaData(dbKey, wrongValue), E_OK);

    /**
    * @tc.steps: step4. initialize new meta with storage
    * @tc.expected: step4. E_OK
    */
    Metadata newMeta;
    ASSERT_EQ(newMeta.Initialize(&storage), E_OK);

    /**
    * @tc.steps: step5. touch the first item
    * @tc.expected: step5. E_OK still exit
    */
    std::string firstItemKey = std::to_string(startCount);
    WaterMark exceptWaterMark;
    EXPECT_EQ(newMeta.GetRecvQueryWaterMark(firstItemKey, deviceId, exceptWaterMark), E_OK);
    EXPECT_EQ(exceptWaterMark, 1u);
}

/**
 * @tc.name: AllPredicateQuerySync001
 * @tc.desc: Test normal push sync for AllPredicate data.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, AllPredicateQuerySync001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. InitSchemaDb
     */
    InitSchemaDb();
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step2. deviceA put {key11, SCHEMA_VALUE1} - {key19, SCHEMA_VALUE1}
                         {key21, SCHEMA_VALUE2} - {key29, SCHEMA_VALUE2}
     */
    Value value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end());
    Value value2(SCHEMA_VALUE2.begin(), SCHEMA_VALUE2.end());
    Key key = {'1'};
    Key key2 = {'2'};
    const int dataSize = 4000;
    for (int i = 0; i < dataSize; i++) {
        key.push_back(i);
        key2.push_back(i);
        status = g_schemaKvDelegatePtr->Put(key, value);
        status = g_schemaKvDelegatePtr->Put(key2, value2);
        ASSERT_TRUE(status == OK);
        key.pop_back();
        key2.pop_back();
    }
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step3. deviceA call query sync and wait
     * @tc.expected: step3. sync should return OK.
     */
    Query query = Query::Select().EqualTo("$.field_name1", 1);
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_schemaKvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result, query);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step4. onComplete should be called, DeviceB have {key11, SCHEMA_VALUE1} - {key19, SCHEMA_VALUE1}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    VirtualDataItem item;
    VirtualDataItem item2;
    for (int i = 0; i < dataSize; i++) {
        key.push_back(i);
        key2.push_back(i);
        g_deviceB->GetData(key, item);
        EXPECT_TRUE(g_deviceB->GetData(key2, item2) != E_OK);
        EXPECT_TRUE(item.value == value);
        key.pop_back();
        key2.pop_back();
    }
}

/**
 * @tc.name: AllPredicateQuerySync002
 * @tc.desc: Test wrong query param push sync for AllPredicate data.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, AllPredicateQuerySync002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. InitSchemaDb
     */
    InitSchemaDb();
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step2. deviceA call query sync and wait
     * @tc.expected: step2. sync should return INVALID_QUERY_FIELD
     */
    Query query = Query::Select().GreaterThan("field_name11", 10);
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_schemaKvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result, query);
    ASSERT_TRUE(status == INVALID_QUERY_FIELD);
    status = g_tool.SyncTest(g_schemaKvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result, query);
    ASSERT_TRUE(status == INVALID_QUERY_FIELD);
    status = g_tool.SyncTest(g_schemaKvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result, query);
    ASSERT_TRUE(status == INVALID_QUERY_FIELD);
}

/**
 * @tc.name: AllPredicateQuerySync003
 * @tc.desc: Test normal push sync for AllPredicate data with limit
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, AllPredicateQuerySync003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. InitSchemaDb
     */
    InitSchemaDb();
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step2. deviceA put {key1, SCHEMA_VALUE1} - {key9, SCHEMA_VALUE1}
     */
    Value value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end());
    Value value2(SCHEMA_VALUE2.begin(), SCHEMA_VALUE2.end());
    Key key = {'1'};
    Key key2 = {'2'};
    const int dataSize = 10;
    for (int i = 0; i < dataSize; i++) {
        key.push_back(i);
        key2.push_back(i);
        status = g_schemaKvDelegatePtr->Put(key, value);
        status = g_schemaKvDelegatePtr->Put(key2, value2);
        ASSERT_TRUE(status == OK);
        key.pop_back();
        key2.pop_back();
    }
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step3. deviceA call query sync with limit and wait
     * @tc.expected: step3. sync should return OK.
     */
    Query query = Query::Select().EqualTo("$.field_name1", 1).Limit(20, 0);
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_schemaKvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result, query);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step4. onComplete should be called, DeviceB have {key1, SCHEMA_VALUE1} - {key9, SCHEMA_VALUE1}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    VirtualDataItem item;
    VirtualDataItem item2;
    for (int i = 0; i < dataSize; i++) {
        key.push_back(i);
        key2.push_back(i);
        g_deviceB->GetData(key, item);
        EXPECT_TRUE(g_deviceB->GetData(key2, item2) != E_OK);
        EXPECT_TRUE(item.value == value);
        key.pop_back();
        key2.pop_back();
    }
}

/**
 * @tc.name: AllPredicateQuerySync004
 * @tc.desc: Test normal pull sync for AllPredicate data.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PQuerySyncTest, AllPredicateQuerySync004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. InitSchemaDb
     */
    InitSchemaDb();
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step2. deviceB put {key11, SCHEMA_VALUE1} - {key19, SCHEMA_VALUE1}
     */
    Value value(SCHEMA_VALUE1.begin(), SCHEMA_VALUE1.end());
    Key key = {'1'};
    const int dataSize = 10;
    for (int i = 0; i < dataSize; i++) {
        key.push_back(i);
        g_deviceB->PutData(key, value, 10 + i, 0);
        ASSERT_TRUE(status == OK);
        key.pop_back();
    }
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step3. deviceA call query sync and wait
     * @tc.expected: step3. sync should return OK.
     */
    Query query = Query::Select().EqualTo("$.field_name1", 1);
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_schemaKvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result, query);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step4. onComplete should be called, DeviceA have {key11, SCHEMA_VALUE1} - {key19, SCHEMA_VALUE1}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    Value item;
    Value item2;
    for (int i = 0; i < dataSize; i++) {
        key.push_back(i);
        g_schemaKvDelegatePtr->Get(key, item);
        EXPECT_TRUE(item == value);
        key.pop_back();
    }
}