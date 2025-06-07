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

#include "metadata/meta_data_manager.h"
#define LOG_TAG "UdmfServiceImplTest"
#include "udmf_service_impl.h"
#include "gtest/gtest.h"
#include "error_code.h"
#include "text.h"
#include "accesstoken_kit.h"
#include "bootstrap.h"
#include "device_manager_adapter.h"
#include "executor_pool.h"
#include "ipc_skeleton.h"
#include "mock/access_token_mock.h"
#include "mock/meta_data_manager_mock.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "kvstore_meta_manager.h"
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
using namespace OHOS::UDMF;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using namespace testing::ext;

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

class UdmfServiceImplTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    static inline std::shared_ptr<AccessTokenKitMock> accTokenMock = nullptr;
    static inline std::shared_ptr<MetaDataManagerMock> metaDataManagerMock = nullptr;
    static inline std::shared_ptr<MetaDataMock<StoreMetaData>> metaDataMock = nullptr;
    void SetUp() {}
    void TearDown() {}
};
void UdmfServiceImplTest::SetUpTestCase() 
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

    accTokenMock = std::make_shared<AccessTokenKitMock>();
    BAccessTokenKit::accessTokenkit = accTokenMock;
    metaDataManagerMock = std::make_shared<MetaDataManagerMock>();
    BMetaDataManager::metaDataManager = metaDataManagerMock;
    metaDataMock = std::make_shared<MetaDataMock<StoreMetaData>>();
    BMetaData<StoreMetaData>::metaDataManager = metaDataMock;
}

void UdmfServiceImplTest::TearDownTestCase(void)
{
    accTokenMock = nullptr;
    BAccessTokenKit::accessTokenkit = nullptr;
    metaDataManagerMock = nullptr;
    BMetaDataManager::metaDataManager = nullptr;
    metaDataMock = nullptr;
    BMetaData<StoreMetaData>::metaDataManager = nullptr;
}
/**
* @tc.name: SaveData001
* @tc.desc: Abnormal test of SaveData, unifiedData is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SaveData001, TestSize.Level0)
{
    CustomOption option;
    UnifiedData data;
    std::string key = "";
    option.intention = UD_INTENTION_BASE;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.SaveData(option, data, key);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: RetrieveData001
* @tc.desc: Abnormal test of RetrieveData, query.key is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, RetrieveData001, TestSize.Level0)
{
    QueryOption query;
    UnifiedData data;
    query.intention = UD_INTENTION_BASE;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.RetrieveData(query, data);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: IsPermissionInCache002
* @tc.desc: Abnormal test of IsPermissionInCache, privilegeCache_ has no query.key
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, IsPermissionInCache002, TestSize.Level0)
{
    QueryOption query;
    UdmfServiceImpl udmfServiceImpl;
    bool ret = udmfServiceImpl.IsPermissionInCache(query);
    EXPECT_EQ(ret, false);
}

/**
* @tc.name: UpdateData001
* @tc.desc: Abnormal test of UpdateData, query.key is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, UpdateData001, TestSize.Level0)
{
    QueryOption query;
    UnifiedData data;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.UpdateData(query, data);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: GetSummary001
* @tc.desc: Abnormal test of UpdateData, query.key is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, GetSummary001, TestSize.Level0)
{
    QueryOption query;
    Summary summary;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.GetSummary(query, summary);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: Sync001
* @tc.desc: Abnormal test of Sync, query.key is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, Sync001, TestSize.Level0)
{
    QueryOption query;
    std::vector<std::string> devices = {"device1"};
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.Sync(query, devices);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: IsRemoteData001
* @tc.desc: Abnormal test of IsRemoteData, query.key is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, IsRemoteData001, TestSize.Level0)
{
    QueryOption query;
    bool result = false;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.IsRemoteData(query, result);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: SetAppShareOption001
* @tc.desc: Abnormal test of SetAppShareOption, intention is empty
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SetAppShareOption001, TestSize.Level0)
{
    std::string intention = "";
    int32_t shareOption = 1;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.SetAppShareOption(intention, shareOption);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}


/**
* @tc.name: SetAppShareOption002
* @tc.desc: Abnormal test of SetAppShareOption, shareOption > SHARE_OPTIONS_BUTT
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SetAppShareOption002, TestSize.Level0)
{
    std::string intention = "intention";
    int32_t shareOption = 4;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.SetAppShareOption(intention, shareOption);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: SetAppShareOption003
* @tc.desc: Abnormal test of SetAppShareOption, shareOption = SHARE_OPTIONS_BUTT
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SetAppShareOption003, TestSize.Level0)
{
    std::string intention = "intention";
    int32_t shareOption = 3;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.SetAppShareOption(intention, shareOption);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: SetAppShareOption004
* @tc.desc: Abnormal test of SetAppShareOption, shareOption < IN_APP
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SetAppShareOption004, TestSize.Level0)
{
    std::string intention = "intention";
    int32_t shareOption = -1;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.SetAppShareOption(intention, shareOption);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: OnUserChangeTest001
* @tc.desc: OnUserChange test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, OnUserChangeTest001, TestSize.Level0)
{
    uint32_t code = 4;
    std::string user = "OH_USER_test";
    std::string account = "OH_ACCOUNT_test";
    UdmfServiceImpl udmfServiceImpl;
    auto status = udmfServiceImpl.OnUserChange(code, user, account);
    ASSERT_EQ(status, UDMF::E_OK);
    auto sizeAfter = StoreCache::GetInstance().stores_.Size();
    ASSERT_EQ(sizeAfter, 0);
}

/**
* @tc.name: TransferToEntriesIfNeedTest001
* @tc.desc: TransferToEntriesIfNeed test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, TransferToEntriesIfNeedTest001, TestSize.Level0)
{
    UnifiedData data;
    QueryOption query;
    auto record1 = std::make_shared<UnifiedRecord>();
    auto record2 = std::make_shared<UnifiedRecord>();
    data.AddRecord(record1);
    data.AddRecord(record2);
    auto properties = std::make_shared<UnifiedDataProperties>();
    properties->tag = "records_to_entries_data_format";
    data.SetProperties(properties);
    query.tokenId = 1;
    UdmfServiceImpl udmfServiceImpl;
    udmfServiceImpl.TransferToEntriesIfNeed(query, data);
    EXPECT_TRUE(data.IsNeedTransferToEntries());
    int recordSize = 2;
    EXPECT_EQ(data.GetRecords().size(), recordSize);
}

/**
* @tc.name: IsNeedMetaSyncTest001
* @tc.desc: IsNeedMetaSync test
* @tc.type: FUNC
*/
HWTEST_F(UdmfServiceImplTest, IsNeedMetaSyncTest001, TestSize.Level0)
{
    UdmfServiceImpl udmfServiceImpl;
    StoreMetaData meta = StoreMetaData("100", "distributeddata", "drag");
    std::vector<std::string> devices = {"remote_device"};

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(false));
    auto isNeedSync = udmfServiceImpl.IsNeedMetaSync(meta, devices);
    EXPECT_EQ(isNeedSync, true);

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(true));
    isNeedSync = udmfServiceImpl.IsNeedMetaSync(meta, devices);
    EXPECT_EQ(isNeedSync, false);

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));
    isNeedSync = udmfServiceImpl.IsNeedMetaSync(meta, devices);
    EXPECT_EQ(isNeedSync, false);

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(true));
    isNeedSync = udmfServiceImpl.IsNeedMetaSync(meta, devices);
    EXPECT_EQ(isNeedSync, false);
}

/**
* @tc.name: SyncTest001
* @tc.desc: IsNeedMetaSync test matrix mask
* @tc.type: FUNC
*/
HWTEST_F(UdmfServiceImplTest, IsNeedMetaSyncTest002, TestSize.Level0)
{
    QueryOption query;
    query.key = "test_key";
    query.tokenId = 1;
    query.intention  = UD_INTENTION_DRAG;
    UdmfServiceImpl udmfServiceImpl;
    StoreMetaData meta = StoreMetaData("100", "distributeddata", "drag");
    std::vector<std::string> devices = {"remote_device"};

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(false));
    auto isNeedSync = udmfServiceImpl.IsNeedMetaSync(meta, devices);
    EXPECT_EQ(isNeedSync, true);

    // mock mask
}

/**
* @tc.name: SyncTest001
* @tc.desc: sync test
* @tc.type: FUNC
*/
HWTEST_F(UdmfServiceImplTest, SyncTest001, TestSize.Level0)
{
    QueryOption query;
    query.key = "test_key";
    query.tokenId = 1;
    query.intention  = UD_INTENTION_DRAG;
    UdmfServiceImpl udmfServiceImpl;
    StoreMetaData meta = StoreMetaData("100", "distributeddata", "drag");
    std::vector<std::string> devices = {"remote_device"};

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(false));
    auto ret = udmfServiceImpl.Sync(query, devices);
    EXPECT_EQ(ret, UDMF::E_OK);
}

/**
 * @tc.name: ResolveAutoLaunch001
 * @tc.desc: ResolveAutoLaunch test.
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, ResolveAutoLaunch001, TestSize.Level0)
{
    auto store = StoreCache::GetInstance().GetStore("drag");
    auto ret = store->Init();
    EXPECT_EQ(ret, UDMF::E_OK);

    DistributedDB::AutoLaunchParam param {
        .userId = "100",
        .appId = "distributeddata",
        .storeId = "drag",
    };
    std::string identifier = "identifier";
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    ret = udmfServiceImpl->ResolveAutoLaunch(identifier, param);
    EXPECT_EQ(ret, UDMF::E_OK);
}
} // DistributedDataTest
}; 