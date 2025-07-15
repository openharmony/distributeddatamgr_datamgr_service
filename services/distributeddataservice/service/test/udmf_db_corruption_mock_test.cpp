/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "udmf_service_impl.h"

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
#include "runtime_store.h"
#include "gtest/gtest.h"
#include "kv_store_nb_delegate_corruption_mock.h"
#include "account/account_delegate.h"

using namespace OHOS::UDMF;
using namespace testing::ext;
using namespace testing;
using namespace OHOS::Security::AccessToken;
using namespace OHOS::UDMF;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
namespace OHOS::Test {
namespace DistributedDataTest {
constexpr const char *DATA_HUB_INTENTION = "DataHub";
class UdmfServiceImplDbCorruptionMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

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

void UdmfServiceImplDbCorruptionMockTest::SetUpTestCase(void)
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

/**
* @tc.name: SaveDataTest001
* @tc.desc: Abnormal test of OnRemoteRequest, code is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplDbCorruptionMockTest, SaveDataTest001, TestSize.Level1)
{
    StoreCache::GetInstance().stores_.Clear();
    std::string key = DATA_HUB_INTENTION;
    auto store = std::make_shared<RuntimeStore>(key);
    store->kvStore_ = std::make_shared<DistributedDB::KvStoreNbDelegateCorruptionMock>();
    int foregroundUserId = 0;
    DistributedData::AccountDelegate::GetInstance()->QueryForegroundUserId(foregroundUserId);
    key.append(std::to_string(foregroundUserId));
    StoreCache::GetInstance().stores_.InsertOrAssign(key, store);
    CustomOption option;
    option.intention = Intention::UD_INTENTION_DATA_HUB;
    option.tokenId = 1;
    auto record = std::make_shared<UnifiedRecord>();
    UnifiedData data;
    data.AddRecord(record);
    std::string key1 = "key";
    UdmfServiceImpl serviceImpl;
    auto status = serviceImpl.SaveData(option, data, key1);
    EXPECT_EQ(status, OHOS::UDMF::E_DB_ERROR);
}

/**
* @tc.name: RetrieveData001
* @tc.desc: Abnormal test of OnRemoteRequest, code is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplDbCorruptionMockTest, RetrieveData001, TestSize.Level1)
{
    StoreCache::GetInstance().stores_.Clear();
    std::string intention = "drag";
    auto store = std::make_shared<RuntimeStore>(intention);
    store->kvStore_ = std::make_shared<DistributedDB::KvStoreNbDelegateCorruptionMock>();
    int foregroundUserId = 0;
    DistributedData::AccountDelegate::GetInstance()->QueryForegroundUserId(foregroundUserId);
    intention.append(std::to_string(foregroundUserId));
    StoreCache::GetInstance().stores_.InsertOrAssign(intention, store);
    std::string key = "udmf://drag/get.example.myapplication/L]WQ=JezoKgDc8\\Rz`q6koADcGRdKMnf";
    QueryOption option = {
        .key = key
    };
    UnifiedData data;
    UdmfServiceImpl serviceImpl;
    auto status = serviceImpl.RetrieveData(option, data);
    EXPECT_EQ(status, OHOS::UDMF::E_DB_ERROR);
}

/**
* @tc.name: DeleteData001
* @tc.desc: Abnormal test of OnRemoteRequest, code is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplDbCorruptionMockTest, DeleteData001, TestSize.Level1)
{
    StoreCache::GetInstance().stores_.Clear();
    std::string intention = DATA_HUB_INTENTION;
    auto store = std::make_shared<RuntimeStore>(intention);
    store->kvStore_ = std::make_shared<DistributedDB::KvStoreNbDelegateCorruptionMock>();
    int foregroundUserId = 0;
    DistributedData::AccountDelegate::GetInstance()->QueryForegroundUserId(foregroundUserId);
    intention.append(std::to_string(foregroundUserId));
    StoreCache::GetInstance().stores_.InsertOrAssign(intention, store);
    std::string key = "udmf://DataHub/delete.example.myapplication/L]WQ=JezoKgDc8\\Rz`q6koADcGRdKMnf";
    QueryOption option = {
        .key = key,
    };
    auto record = std::make_shared<UnifiedRecord>();
    UnifiedData data;
    data.AddRecord(record);
    std::vector<UnifiedData> dataList = { data };
    UdmfServiceImpl serviceImpl;
    auto status = serviceImpl.DeleteData(option, dataList);
    EXPECT_EQ(status, OHOS::UDMF::E_DB_ERROR);
}

/**
* @tc.name: GetSummary001
* @tc.desc: Abnormal test of OnRemoteRequest, code is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplDbCorruptionMockTest, GetSummary001, TestSize.Level1)
{
    StoreCache::GetInstance().stores_.Clear();
    std::string intention = DATA_HUB_INTENTION;
    auto store = std::make_shared<RuntimeStore>(intention);
    store->kvStore_ = std::make_shared<DistributedDB::KvStoreNbDelegateCorruptionMock>();
    int foregroundUserId = 0;
    DistributedData::AccountDelegate::GetInstance()->QueryForegroundUserId(foregroundUserId);
    intention.append(std::to_string(foregroundUserId));
    StoreCache::GetInstance().stores_.InsertOrAssign(intention, store);
    std::string key = "udmf://DataHub/summary.example.myapplication/L]WQ=JezoKgDc8\\Rz`q6koADcGRdKMnf";
    QueryOption option = {
        .key = key,
    };
    Summary summary;
    UdmfServiceImpl serviceImpl;
    auto status = serviceImpl.GetSummary(option, summary);
    EXPECT_EQ(status, OHOS::UDMF::E_DB_ERROR);
}

/**
* @tc.name: IsRemoteData001
* @tc.desc: Abnormal test of OnRemoteRequest, code is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplDbCorruptionMockTest, IsRemoteData001, TestSize.Level1)
{
    StoreCache::GetInstance().stores_.Clear();
    std::string intention = DATA_HUB_INTENTION;
    auto store = std::make_shared<RuntimeStore>(intention);
    store->kvStore_ = std::make_shared<DistributedDB::KvStoreNbDelegateCorruptionMock>();
    int foregroundUserId = 0;
    DistributedData::AccountDelegate::GetInstance()->QueryForegroundUserId(foregroundUserId);
    intention.append(std::to_string(foregroundUserId));
    StoreCache::GetInstance().stores_.InsertOrAssign(intention, store);
    std::string key = "udmf://DataHub/remote.example.myapplication/L]WQ=JezoKgDc8\\Rz`q6koADcGRdKMnf";
    QueryOption option = {
        .key = key,
    };
    UdmfServiceImpl serviceImpl;
    bool ret = false;
    auto status = serviceImpl.IsRemoteData(option, ret);
    EXPECT_EQ(status, OHOS::UDMF::E_DB_ERROR);
    EXPECT_FALSE(ret);
}

/**
* @tc.name: GetAppShareOption001
* @tc.desc: Abnormal test of OnRemoteRequest, code is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplDbCorruptionMockTest, GetAppShareOption001, TestSize.Level1)
{
    StoreCache::GetInstance().stores_.Clear();
    std::string intention = DATA_HUB_INTENTION;
    auto store = std::make_shared<RuntimeStore>(intention);
    store->kvStore_ = std::make_shared<DistributedDB::KvStoreNbDelegateCorruptionMock>();
    int foregroundUserId = 0;
    DistributedData::AccountDelegate::GetInstance()->QueryForegroundUserId(foregroundUserId);
    intention.append(std::to_string(foregroundUserId));
    StoreCache::GetInstance().stores_.InsertOrAssign(intention, store);
    UdmfServiceImpl serviceImpl;
    int32_t shareOption = CROSS_APP;
    auto status = serviceImpl.GetAppShareOption("DataHub", shareOption);
    EXPECT_EQ(status, OHOS::UDMF::E_DB_ERROR);
}

/**
* @tc.name: QueryDataCommon001
* @tc.desc: Abnormal test of OnRemoteRequest, code is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplDbCorruptionMockTest, QueryDataCommon001, TestSize.Level1)
{
    StoreCache::GetInstance().stores_.Clear();
    std::string intention = DATA_HUB_INTENTION;
    auto store = std::make_shared<RuntimeStore>(intention);
    store->kvStore_ = std::make_shared<DistributedDB::KvStoreNbDelegateCorruptionMock>();
    int foregroundUserId = 0;
    DistributedData::AccountDelegate::GetInstance()->QueryForegroundUserId(foregroundUserId);
    intention.append(std::to_string(foregroundUserId));
    StoreCache::GetInstance().stores_.InsertOrAssign(intention, store);
    UdmfServiceImpl serviceImpl;
    QueryOption option = {
        .intention = Intention::UD_INTENTION_DATA_HUB,
    };
    std::vector<UnifiedData> dataList;
    std::shared_ptr<Store> store1;
    auto status = serviceImpl.QueryDataCommon(option, dataList, store1);
    EXPECT_EQ(status, OHOS::UDMF::E_DB_ERROR);
}

/**
* @tc.name: PutLocal001
* @tc.desc: Abnormal test of OnRemoteRequest, code is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplDbCorruptionMockTest, PutLocal001, TestSize.Level1)
{
    std::string key = "key";
    std::string value = "value";
    auto store = std::make_shared<RuntimeStore>(DATA_HUB_INTENTION);
    store->kvStore_ = std::make_shared<DistributedDB::KvStoreNbDelegateCorruptionMock>();
    auto status = store->PutLocal(key, value);
    EXPECT_EQ(status, OHOS::UDMF::E_DB_CORRUPTED);
}

/**
* @tc.name: GetLocal001
* @tc.desc: Abnormal test of OnRemoteRequest, code is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplDbCorruptionMockTest, GetLocal001, TestSize.Level1)
{
    std::string key = "key";
    std::string value;
    auto store = std::make_shared<RuntimeStore>(DATA_HUB_INTENTION);
    store->kvStore_ = std::make_shared<DistributedDB::KvStoreNbDelegateCorruptionMock>();
    auto status = store->GetLocal(key, value);
    EXPECT_EQ(status, OHOS::UDMF::E_DB_CORRUPTED);
}

/**
* @tc.name: DeleteLocal001
* @tc.desc: Abnormal test of OnRemoteRequest, code is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplDbCorruptionMockTest, DeleteLocal001, TestSize.Level1)
{
    std::string key = "key";
    auto store = std::make_shared<RuntimeStore>(DATA_HUB_INTENTION);
    store->kvStore_ = std::make_shared<DistributedDB::KvStoreNbDelegateCorruptionMock>();
    auto status = store->DeleteLocal(key);
    EXPECT_EQ(status, OHOS::UDMF::E_DB_CORRUPTED);
}

/**
* @tc.name: PutRuntime001
* @tc.desc: Abnormal test of OnRemoteRequest, code is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplDbCorruptionMockTest, PutRuntime001, TestSize.Level1)
{
    std::string key = "key";
    Runtime runtime;
    auto store = std::make_shared<RuntimeStore>(DATA_HUB_INTENTION);
    store->kvStore_ = std::make_shared<DistributedDB::KvStoreNbDelegateCorruptionMock>();
    auto status = store->PutRuntime(key, runtime);
    EXPECT_EQ(status, OHOS::UDMF::E_DB_CORRUPTED);
}

/**
* @tc.name: GetRuntime001
* @tc.desc: Abnormal test of OnRemoteRequest, code is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplDbCorruptionMockTest, GetRuntime001, TestSize.Level1)
{
    std::string key = "key";
    Runtime runtime;
    auto store = std::make_shared<RuntimeStore>(DATA_HUB_INTENTION);
    store->kvStore_ = std::make_shared<DistributedDB::KvStoreNbDelegateCorruptionMock>();
    auto status = store->GetRuntime(key, runtime);
    EXPECT_EQ(status, OHOS::UDMF::E_DB_CORRUPTED);
}

/**
* @tc.name: Update001
* @tc.desc: Abnormal test of OnRemoteRequest, code is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplDbCorruptionMockTest, Update001, TestSize.Level1)
{
    UnifiedData data;
    auto rumtime = std::make_shared<Runtime>();
    UnifiedKey key(DATA_HUB_INTENTION, "com.demo.test", "111");
    rumtime->key = key;
    data.runtime_ = rumtime;
    auto store = std::make_shared<RuntimeStore>(DATA_HUB_INTENTION);
    store->kvStore_ = std::make_shared<DistributedDB::KvStoreNbDelegateCorruptionMock>();
    auto status = store->Update(data);
    EXPECT_EQ(status, OHOS::UDMF::E_DB_CORRUPTED);
}

/**
* @tc.name: Delete001
* @tc.desc: Abnormal test of OnRemoteRequest, code is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplDbCorruptionMockTest, Delete001, TestSize.Level1)
{
    std::string key = "key";
    auto store = std::make_shared<RuntimeStore>(DATA_HUB_INTENTION);
    store->kvStore_ = std::make_shared<DistributedDB::KvStoreNbDelegateCorruptionMock>();
    auto status = store->Delete(key);
    EXPECT_EQ(status, OHOS::UDMF::E_DB_CORRUPTED);
}
}; // namespace DistributedDataTest
}; // namespace OHOS::Test