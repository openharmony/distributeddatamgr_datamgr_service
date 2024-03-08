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
#include "token_setproc.h"
#define private public
#include "store/runtime_store.h"
#undef private
using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
using namespace OHOS::UDMF;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Entry = DistributedDB::Entry;
using Key = DistributedDB::Key;
using Value = DistributedDB::Value;

namespace OHOS::Test {
namespace DistributedDataTest {

void GrantPermissionNative()
{
    const char **perms = new const char *[2];
    perms[0] = "ohos.permission.DISTRIBUTED_DATASYNC";
    perms[1] = "ohos.permission.ACCESS_SERVICE_DM";
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 2,
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
    EXPECT_EQ(E_DB_ERROR, status);
    entries.clear();
    status = store->GetEntries(KEY_PREFIX, entries);
    EXPECT_EQ(E_OK, status);
    EXPECT_EQ(0, entries.size());
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
    EXPECT_EQ(E_DB_ERROR, status);
    status = store->GetEntries(KEY_PREFIX, entries);
    EXPECT_EQ(E_OK, status);
    EXPECT_EQ(0, entries.size());
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
}
}; // namespace DistributedDataTest
}; // namespace OHOS::Test