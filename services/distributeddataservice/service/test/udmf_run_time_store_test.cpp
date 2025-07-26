/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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

#define LOG_TAG "UdmfRunTimeStoreTest"
#include <openssl/rand.h>

#include "accesstoken_kit.h"
#include "bootstrap.h"
#include "device_manager_adapter.h"
#include "executor_pool.h"
#include "gtest/gtest.h"
#include "ipc_skeleton.h"
#include "kvstore_meta_manager.h"
#include "metadata/meta_data_manager.h"
#include "nativetoken_kit.h"
#include "preprocess_utils.h"
#include "runtime_store.h"
#include "text.h"
#include "token_setproc.h"
#include "store_account_observer.h"
#include "directory_manager.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
using namespace OHOS::UDMF;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Entry = DistributedDB::Entry;
using Key = DistributedDB::Key;
using Value = DistributedDB::Value;
using UnifiedData = OHOS::UDMF::UnifiedData;
using Summary =  OHOS::UDMF::Summary;
namespace OHOS::Test {
namespace DistributedDataTest {

static void GrantPermissionNative()
{
    const char **perms = new const char *[3];
    perms[0] = "ohos.permission.DISTRIBUTED_DATASYNC";
    perms[1] = "ohos.permission.ACCESS_SERVICE_DM";
    perms[2] = "ohos.permission.MONITOR_DEVICE_NETWORK_STATE"; // perms[2] is a permission parameter
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 3,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "distributed_data_test",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    AccessTokenKit::ReloadNativeTokenInfo();
    delete[] perms;
}

class UdmfRunTimeStoreTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        GrantPermissionNative();
        DistributedData::Bootstrap::GetInstance().LoadComponents();
        DistributedData::Bootstrap::GetInstance().LoadDirectory();
        DistributedData::Bootstrap::GetInstance().LoadCheckers();
        size_t max = 2;
        size_t min = 1;
        auto executors = std::make_shared<OHOS::ExecutorPool>(max, min);
        DmAdapter::GetInstance().Init(executors);
        DistributedKv::KvStoreMetaManager::GetInstance().BindExecutor(executors);
        DistributedKv::KvStoreMetaManager::GetInstance().InitMetaParameter();
        DistributedKv::KvStoreMetaManager::GetInstance().InitMetaListener();
    }
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
    void GetRandomKey(std::vector<uint8_t>& key, uint32_t defaultSize);
    void GetRandomValue(std::vector<uint8_t>& value, uint32_t defaultSize);

    const uint32_t MAX_KEY_SIZE = 1024;
    const uint32_t MAX_VALUE_SIZE = 4 * 1024 * 1024;
    const std::string STORE_ID = "drag";
    const std::string KEY_PREFIX = "TEST_";
    const std::string SUMMARY_KEY_PREFIX = "SUMMARY_KEY_PREFIX";
    const std::string EMPTY_DEVICE_ID = "";
    const std::string BUNDLE_NAME = "udmf_test";
    static constexpr size_t tempUdataRecordSize = 1;
};

void UdmfRunTimeStoreTest::GetRandomKey(std::vector<uint8_t>& key, uint32_t defaultSize)
{
    key.resize(defaultSize);
    RAND_bytes(key.data(), defaultSize);

    if (defaultSize >= KEY_PREFIX.length()) {
        vector<uint8_t> vectorPrefix(KEY_PREFIX.begin(), KEY_PREFIX.end());
        std::copy(vectorPrefix.begin(), vectorPrefix.end(), key.begin());
    }
}

void UdmfRunTimeStoreTest::GetRandomValue(std::vector<uint8_t>& value, uint32_t defaultSize)
{
    value.resize(defaultSize);
    RAND_bytes(value.data(), defaultSize);
}

/**
* @tc.name: PutEntries001
* @tc.desc: check for legal parameters, sum size of all entries is smaller than 512M.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, PutEntries001, TestSize.Level1)
{
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_EQ(true, result);

    auto status = store->Delete(KEY_PREFIX);
    EXPECT_EQ(E_OK, status);

    Key key;
    Value value;
    GetRandomValue(value, MAX_KEY_SIZE); // 1K
    vector<Entry> entrysRand;
    for (int i = 0; i < 129; ++i) {
        GetRandomKey(key, MAX_KEY_SIZE); // 1K
        entrysRand.push_back({ key, value });
    }

    status = store->PutEntries(entrysRand);
    EXPECT_EQ(E_OK, status);

    vector<Entry> entries;
    status = store->GetEntries(KEY_PREFIX, entries);
    EXPECT_EQ(E_OK, status);
    EXPECT_EQ(129, entries.size());

    status = store->Delete(KEY_PREFIX);
    EXPECT_EQ(E_OK, status);
}

/**
* @tc.name: PutEntries002
* @tc.desc: check for legal parameters, sum size of all entries is equal to 512M.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, PutEntries002, TestSize.Level1)
{
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_EQ(true, result);

    Key key;
    Value value;
    GetRandomKey(key, MAX_KEY_SIZE);       // 1K
    GetRandomValue(value, MAX_VALUE_SIZE); // 4M
    vector<Entry> entrysRand(127, { key, value });
    Value emptyValue;
    for (int i = 0; i < 3969; i++) {
        entrysRand.push_back({ key, emptyValue });
    }
    int32_t status = store->PutEntries(entrysRand);
    EXPECT_EQ(E_OK, status);

    vector<Entry> entries;
    status = store->GetEntries(KEY_PREFIX, entries);
    EXPECT_EQ(E_OK, status);
    EXPECT_EQ(1, entries.size());

    status = store->Delete(KEY_PREFIX);
    EXPECT_EQ(E_OK, status);
}

/**
* @tc.name: PutEntries003
* @tc.desc: test rollback, sum size of all entries is larger to 512M.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, PutEntries003, TestSize.Level1)
{
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_EQ(true, result);

    Key key;
    Value value;
    GetRandomKey(key, MAX_KEY_SIZE);       // 1K
    GetRandomValue(value, MAX_VALUE_SIZE); // 4M
    vector<Entry> entrysRand(127, { key, value });
    Value emptyValue;
    for (int i = 0; i < 3970; i++) {
        entrysRand.push_back({ key, emptyValue });
    }

    int32_t status = store->PutEntries(entrysRand);
    EXPECT_EQ(E_DB_ERROR, status);

    vector<Entry> entries;
    status = store->GetEntries(KEY_PREFIX, entries);
    EXPECT_EQ(E_OK, status);
    EXPECT_EQ(0, entries.size());

    status = store->Delete(KEY_PREFIX);
    EXPECT_EQ(E_OK, status);
}

/**
* @tc.name: PutEntries004
* @tc.desc: test rollback, illegal size of key or value.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, PutEntries004, TestSize.Level1)
{
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_EQ(true, result);

    Key key;
    Key keyInvalid;
    Value value;
    Value valueInvalid;
    GetRandomKey(key, MAX_KEY_SIZE);                  // 1K
    GetRandomKey(keyInvalid, MAX_KEY_SIZE + 1);       // 1K + 1
    GetRandomValue(value, MAX_VALUE_SIZE);            // 4M
    GetRandomValue(valueInvalid, MAX_VALUE_SIZE + 1); // 4M + 1
    vector<Entry> entrysMix1(1, { keyInvalid, value });
    vector<Entry> entrysMix2(1, { key, valueInvalid });

    int32_t status = store->PutEntries(entrysMix1);
    EXPECT_EQ(E_DB_ERROR, status);
    vector<Entry> entries;
    status = store->GetEntries(KEY_PREFIX, entries);
    EXPECT_EQ(E_OK, status);
    EXPECT_EQ(0, entries.size());

    status = store->PutEntries(entrysMix2);
    EXPECT_EQ(E_OK, status);
    entries.clear();
    status = store->GetEntries(KEY_PREFIX, entries);
    EXPECT_EQ(E_OK, status);
    EXPECT_EQ(1, entries.size());

    status = store->Delete(KEY_PREFIX);
    EXPECT_EQ(E_OK, status);
}

/**
* @tc.name: PutEntries005
* @tc.desc: test rollback, mix illegal entries and legal entries.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, PutEntries005, TestSize.Level1)
{
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_EQ(true, result);

    Key key;
    Key keyInvalid;
    Value value;
    Value valueInvalid;
    vector<Entry> entrysRand;
    for (int i = 0; i < 129; ++i) {
        GetRandomKey(key, MAX_KEY_SIZE);              // 1K
        GetRandomValue(value, MAX_KEY_SIZE);          // 1K
        entrysRand.push_back({ key, value });
    }

    GetRandomKey(keyInvalid, MAX_KEY_SIZE + 1);       // 1K + 1
    GetRandomValue(value, MAX_KEY_SIZE);              // 1K
    entrysRand[128] = { keyInvalid, value };
    int32_t status = store->PutEntries(entrysRand);
    EXPECT_EQ(E_DB_ERROR, status);
    vector<Entry> entries;
    status = store->GetEntries(KEY_PREFIX, entries);
    EXPECT_EQ(E_OK, status);
    EXPECT_EQ(0, entries.size());

    GetRandomKey(key, MAX_KEY_SIZE);                  // 1K
    GetRandomValue(valueInvalid, MAX_VALUE_SIZE + 1); // 4M + 1
    entrysRand[128] = { key, valueInvalid };
    status = store->PutEntries(entrysRand);
    EXPECT_EQ(E_OK, status);
    status = store->GetEntries(KEY_PREFIX, entries);
    EXPECT_EQ(E_OK, status);
    EXPECT_EQ(129, entries.size());

    status = store->Delete(KEY_PREFIX);
    EXPECT_EQ(E_OK, status);
}

/**
* @tc.name: DeleteEntries001
* @tc.desc: check for legal parameters, delete entries success.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, DeleteEntries001, TestSize.Level1)
{
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_EQ(true, result);

    Value value;
    GetRandomValue(value, MAX_KEY_SIZE); // 1K
    vector<Key> keys(129);
    vector<Entry> entrysRand;
    for (int i = 0; i < 129; ++i) {
        GetRandomKey(keys[i], MAX_KEY_SIZE); // 1K
        entrysRand.push_back({ keys[i], value });
    }

    int32_t status = store->PutEntries(entrysRand);
    EXPECT_EQ(E_OK, status);

    vector<Entry> entries;
    status = store->GetEntries(KEY_PREFIX, entries);
    EXPECT_EQ(E_OK, status);
    EXPECT_EQ(129, entries.size());

    status = store->DeleteEntries(keys);
    EXPECT_EQ(E_OK, status);

    entries.clear();
    status = store->GetEntries(KEY_PREFIX, entries);
    EXPECT_EQ(E_OK, status);
    EXPECT_EQ(0, entries.size());

    status = store->Delete(KEY_PREFIX);
    EXPECT_EQ(E_OK, status);
}

/**
* @tc.name: DeleteEntries002
* @tc.desc: check for illegal parameters, delete entries error.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, DeleteEntries002, TestSize.Level1)
{
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);
    Key key;
    Value value;
    GetRandomKey(key, MAX_KEY_SIZE);                  // 1K
    GetRandomValue(value, MAX_VALUE_SIZE);            // 4M
    vector<Entry> entrysRand(127, { key, value });
    vector<Key> keys(3970);
    Value emptyValue;
    for (int i = 0; i < 3970; ++i) {
        GetRandomKey(keys[i], 3970);
        entrysRand.push_back({ keys[i], emptyValue });
    }

    int32_t status = store->PutEntries(entrysRand);
    EXPECT_EQ(E_DB_ERROR, status);

    vector<Entry> entries;
    status = store->GetEntries(KEY_PREFIX, entries);
    EXPECT_EQ(E_OK, status);
    EXPECT_EQ(0, entries.size());

    status = store->DeleteEntries(keys);
    EXPECT_EQ(E_DB_ERROR, status);

    entries.clear();
    status = store->GetEntries(KEY_PREFIX, entries);
    EXPECT_EQ(E_OK, status);
    EXPECT_EQ(0, entries.size());

    status = store->Delete(KEY_PREFIX);
    EXPECT_EQ(E_OK, status);
}

/**
* @tc.name: Init
* @tc.desc: check for illegal parameters, delete entries error.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, Init, TestSize.Level1)
{
    auto dvInfo = DeviceManagerAdapter::GetInstance().GetLocalDevice();
    auto uuid = DeviceManagerAdapter::GetInstance().GetUuidByNetworkId(EMPTY_DEVICE_ID);
    EXPECT_TRUE(uuid.empty());
    dvInfo.uuid = EMPTY_DEVICE_ID;
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);
}

/**
* @tc.name: Get001
* @tc.desc: check for illegal parameters, delete entries error.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, Get001, TestSize.Level1)
{
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);

    Key keyInvalid;
    Value value;
    GetRandomKey(keyInvalid, MAX_KEY_SIZE + 1);       // 1K + 1
    GetRandomValue(value, MAX_VALUE_SIZE);            // 4M
    vector<Entry> entrysMix1(1, { keyInvalid, value });
    int32_t status = store->PutEntries(entrysMix1);
    EXPECT_EQ(E_DB_ERROR, status);
    entrysMix1.clear();
    status = store->GetEntries(KEY_PREFIX, entrysMix1);
    EXPECT_EQ(E_OK, status);
    EXPECT_TRUE(entrysMix1.empty());

    status = store->Delete(KEY_PREFIX);
    EXPECT_EQ(E_OK, status);
}

/**
* @tc.name: Get002
* @tc.desc: check for legal parameters, delete entries error.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, Get002, TestSize.Level1)
{
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);

    Key key;
    Value value;
    GetRandomKey(key, MAX_KEY_SIZE);       // 1K
    GetRandomValue(value, MAX_VALUE_SIZE); // 4M
    vector<Entry> entrysRand(127, { key, value });
    Value emptyValue;
    for (int i = 0; i < 3970; i++) {
        entrysRand.push_back({ key, emptyValue });
    }
    auto status = store->PutEntries(entrysRand);
    EXPECT_EQ(E_DB_ERROR, status);
    entrysRand.clear();
    status = store->GetEntries(KEY_PREFIX, entrysRand);
    EXPECT_EQ(E_OK, status);
    EXPECT_TRUE(entrysRand.empty());

    status = store->Delete(KEY_PREFIX);
    EXPECT_EQ(E_OK, status);
}

/**
* @tc.name: GetDetailsFromUData
* @tc.desc: check for legal parameters, delete entries error.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, GetDetailsFromUData, TestSize.Level1)
{
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);

    Key key;
    Value value;
    GetRandomValue(value, MAX_KEY_SIZE); // 1K
    vector<Entry> entrysRand;
    for (int i = 0; i < 129; ++i) {
        GetRandomKey(key, MAX_KEY_SIZE); // 1K
        entrysRand.push_back({ key, value });
    }

    int32_t status = store->PutEntries(entrysRand);
    EXPECT_EQ(E_OK, status);
    vector<Entry> entries;
    status = store->GetEntries(KEY_PREFIX, entries);
    EXPECT_EQ(E_OK, status);
    EXPECT_EQ(129, entries.size());
    UnifiedData data1;
    UDDetails details1;
    details1.insert({ "udmf_key", "udmf_value" });
    auto records = data1.GetRecords();
    EXPECT_EQ(records.size(), 0);
    status = PreProcessUtils::GetDetailsFromUData(data1, details1);
    EXPECT_FALSE(status);

    status = store->Delete(KEY_PREFIX);
    EXPECT_EQ(E_OK, status);
}

/**
* @tc.name: GetDetailsFromUData01
* @tc.desc: check for legal parameters, delete entries error.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, GetDetailsFromUData01, TestSize.Level1)
{
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);

    Key key;
    Value value;
    GetRandomValue(value, MAX_KEY_SIZE); // 1K
    vector<Entry> entrysRand;
    for (int i = 0; i < 129; ++i) {
        GetRandomKey(key, MAX_KEY_SIZE); // 1K
        entrysRand.push_back({ key, value });
    }

    int32_t status = store->PutEntries(entrysRand);
    EXPECT_EQ(E_OK, status);
    vector<Entry> entries;
    status = store->GetEntries(KEY_PREFIX, entries);
    EXPECT_EQ(E_OK, status);
    status = store->Delete(KEY_PREFIX);
    EXPECT_EQ(E_OK, status);
    UDDetails details1;
    details1.insert({ "udmf_key", "udmf_value" });
    UnifiedData inputData;
    std::vector<std::shared_ptr<UnifiedRecord>> inputRecords;
    for (int32_t i = 0; i < 512; ++i) {
        inputRecords.emplace_back(std::make_shared<Text>());
    }
    inputData.SetRecords(inputRecords);
    UnifiedData outputData;
    auto outputRecords = outputData.GetRecords();
    ASSERT_EQ(inputRecords.size(), 512);
    ASSERT_EQ(0, outputRecords.size());
    status = PreProcessUtils::GetDetailsFromUData(inputData, details1);
    EXPECT_FALSE(status);
}

/**
* @tc.name: Sync01
* @tc.desc: check for legal parameters, delete entries error.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, Sync01, TestSize.Level1)
{
    std::vector<std::string> devices = {"device"};
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);
    Status ret = store->Sync(devices);
    EXPECT_EQ(ret, E_DB_ERROR);
}

/**
* @tc.name: Sync02
* @tc.desc: check for legal parameters, delete entries error.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, Sync02, TestSize.Level1)
{
    std::vector<std::string> devices = { };
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);
    Status ret = store->Sync(devices);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: Sync03
* @tc.desc: check for legal parameters, delete entries error.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, Sync03, TestSize.Level1)
{
    std::vector<std::string> devices = { "device" };
    ProcessCallback callback;
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);
    Status ret = store->Sync(devices, callback);
    EXPECT_EQ(ret, E_DB_ERROR);
}

/**
* @tc.name: Sync04
* @tc.desc: check for legal parameters, delete entries error.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, Sync04, TestSize.Level1)
{
    std::vector<std::string> devices = { };
    ProcessCallback callback;
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);
    Status ret = store->Sync(devices, callback);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: Clear01
* @tc.desc: check for legal parameters, delete entries error.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, Clear01, TestSize.Level1)
{
    static constexpr const char *DATA_PREFIX = "udmf://";
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);
    Status ret = store->Clear();
    EXPECT_EQ(ret, store->Delete(DATA_PREFIX));
}

/**
* @tc.name: GetSummary
* @tc.desc: check for legal parameters, delete entries error.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, GetSummary, TestSize.Level1)
{
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);
    Summary summary;
    summary.summary = {
        { "general.file", 10 },
        { "general.png", 10 },
        { "general.html", 10 },
        { "general.jpeg", 10 },
        { "general.avi", 10},
        { "aabbcc", 10}
    };
    UnifiedKey key(SUMMARY_KEY_PREFIX);
    auto status = store->PutSummary(key, summary);
    EXPECT_EQ(status, E_OK);
    status = store->GetSummary(key, summary);
    ASSERT_EQ(status, E_OK);
}


/**
* @tc.name: GetRuntime001
* @tc.desc: Normal testcase of GetRuntime
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, GetRuntime001, TestSize.Level1)
{
    UnifiedKey udKey(STORE_ID, BUNDLE_NAME, UDMF::PreProcessUtils::GenerateId());
    Runtime runtime{
        .key = udKey
    };
    UnifiedData inputData;
    inputData.SetRuntime(runtime);
    auto key = inputData.GetRuntime()->key.GetUnifiedKey();

    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);
    auto status = store->PutRuntime(key, *inputData.GetRuntime());
    EXPECT_EQ(status, E_OK);

    Runtime outRuntime;
    status = store->GetRuntime(key, outRuntime);
    EXPECT_EQ(status, E_OK);
    EXPECT_EQ(inputData.GetRuntime()->createTime, outRuntime.createTime);
}

/**
* @tc.name: GetRuntime002
* @tc.desc: Abnormal testcase of GetRuntime
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, GetRuntime002, TestSize.Level1)
{
    UnifiedKey udKey(STORE_ID, BUNDLE_NAME, UDMF::PreProcessUtils::GenerateId());
    Runtime runtime{
        .key = udKey
    };
    UnifiedData inputData;
    inputData.SetRuntime(runtime);
    auto key = inputData.GetRuntime()->key.GetUnifiedKey();

    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);

    Runtime outRuntime;
    auto status = store->GetRuntime(key, outRuntime);
    EXPECT_EQ(status, E_NOT_FOUND);
}

/**
* @tc.name: OnAccountChanged001
* @tc.desc: Abnormal testcase of OnAccountChanged
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, OnAccountChanged001, TestSize.Level1)
{
    RuntimeStoreAccountObserver observer;
    const AccountEventInfo eventInfo = {
        .status = AccountStatus::DEVICE_ACCOUNT_DELETE
    };
    DistributedData::StoreMetaData metaData;
    uint32_t token = IPCSkeleton::GetSelfTokenID();
    metaData.bundleName = DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    metaData.appId = DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    metaData.user = eventInfo.userId;
    metaData.tokenId = token;
    metaData.securityLevel = DistributedKv::SecurityLevel::S1;
    metaData.area = DistributedKv::Area::EL1;
    metaData.storeType = DistributedKv::KvStoreType::SINGLE_VERSION;
    metaData.dataDir = DistributedData::DirectoryManager::GetInstance().GetStorePath(metaData);
    std::string userPath = metaData.dataDir.append("/").append(eventInfo.userId);
    observer.OnAccountChanged(eventInfo, 0);
    EXPECT_EQ(access(userPath.c_str(), F_OK), -1);
    SetSelfTokenID(0);
    observer.OnAccountChanged(eventInfo, 0);
}

/**
* @tc.name: MarkWhenCorrupted001
* @tc.desc: Normal testcase of MarkWhenCorrupted
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, MarkWhenCorrupted001, TestSize.Level1)
{
    DistributedDB::DBStatus status = DistributedDB::DBStatus::OK;
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    store->MarkWhenCorrupted(status);
    EXPECT_FALSE(store->isCorrupted_);
}

/**
* @tc.name: MarkWhenCorrupted001
* @tc.desc: Normal testcase of MarkWhenCorrupted
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfRunTimeStoreTest, MarkWhenCorrupted002, TestSize.Level1)
{
    DistributedDB::DBStatus status = DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB;
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    store->MarkWhenCorrupted(status);
    EXPECT_TRUE(store->isCorrupted_);
}
}; // namespace DistributedDataTest
}; // namespace OHOS::Test