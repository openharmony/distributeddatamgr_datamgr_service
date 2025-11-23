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
#define LOG_TAG "UdmfServiceImplTest"
#include "udmf_service_impl.h"

#include <gtest/gtest.h>

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "bootstrap.h"
#include "delay_data_acquire_container.h"
#include "delay_data_prepare_container.h"
#include "device_manager_adapter.h"
#include "executor_pool.h"
#include "ipc_skeleton.h"
#include "kvstore_meta_manager.h"
#include "lifecycle_manager.h"
#include "nativetoken_kit.h"
#include "plain_text.h"
#include "preprocess_utils.h"
#include "runtime_store.h"
#include "synced_device_container.h"
#include "token_setproc.h"
#include "uri_permission_manager.h"

using namespace testing::ext;
using namespace OHOS::Security::AccessToken;
using namespace OHOS::UDMF;
using Entry = DistributedDB::Entry;
using Key = DistributedDB::Key;
using Value = DistributedDB::Value;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using UnifiedData = OHOS::UDMF::UnifiedData;
using Summary =  OHOS::UDMF::Summary;
namespace OHOS::Test {
namespace DistributedDataTest {
static constexpr const char *HAP_BUNDLE_NAME = "ohos.mytest.demo";
static constexpr const char *HAP_BUNDLE_NAME1 = "ohos.test.demo1";

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
        AllocTestHapToken();
        AllocTestHapToken1();
    }
    static void TearDownTestCase(void)
    {
        DeleteTestHapToken();
    }
    void SetUp(){};
    void TearDown(){};
    static void AllocTestHapToken();
    static void AllocTestHapToken1();
    static void DeleteTestHapToken();

    static constexpr const char *STORE_ID = "drag";
    static constexpr uint32_t TOKEN_ID = 5;
    static constexpr const char *APP_ID = "appId";
    static constexpr uint32_t userId = 100;
    static constexpr int instIndex = 0;
};

void UdmfServiceImplTest::AllocTestHapToken()
{
    HapInfoParams info = {
        .userID = userId,
        .bundleName = HAP_BUNDLE_NAME,
        .instIndex = instIndex,
        .appIDDesc = "ohos.mytest.demo_09AEF01D"
    };
    HapPolicyParams policy = {
        .apl = APL_NORMAL,
        .domain = "test.domain",
        .permList = {
            {
                .permissionName = "ohos.permission.test",
                .bundleName = HAP_BUNDLE_NAME,
                .grantMode = 1,
                .availableLevel = APL_NORMAL,
                .label = "label",
                .labelId = 1,
                .description = "open the door",
                .descriptionId = 1
            }
        },
        .permStateList = {
            {
                .permissionName = "ohos.permission.test",
                .isGeneral = true,
                .resDeviceID = { "local" },
                .grantStatus = { PermissionState::PERMISSION_GRANTED },
                .grantFlags = { 1 }
            }
        }
    };
    AccessTokenKit::AllocHapToken(info, policy);
}

void UdmfServiceImplTest::AllocTestHapToken1()
{
    HapInfoParams info = {
        .userID = userId,
        .bundleName = HAP_BUNDLE_NAME1,
        .instIndex = instIndex,
        .appIDDesc = "ohos.test.demo1"
    };

    HapPolicyParams policy = {
        .apl = APL_NORMAL,
        .domain = "test.domain",
        .permList = {
            {
                .permissionName = "ohos.permission.test",
                .bundleName = HAP_BUNDLE_NAME1,
                .grantMode = 1,
                .availableLevel = APL_NORMAL,
                .label = "label",
                .labelId = 1,
                .description = "test1",
                .descriptionId = 1
            }
        },
        .permStateList = {
            {
                .permissionName = "ohos.permission.test",
                .isGeneral = true,
                .resDeviceID = { "local" },
                .grantStatus = { PermissionState::PERMISSION_GRANTED },
                .grantFlags = { 1 }
            }
        }
    };
    auto tokenID = AccessTokenKit::AllocHapToken(info, policy);
    SetSelfTokenID(tokenID.tokenIDEx);
}

void UdmfServiceImplTest::DeleteTestHapToken()
{
    auto tokenId = AccessTokenKit::GetHapTokenID(userId, HAP_BUNDLE_NAME, instIndex);
    AccessTokenKit::DeleteToken(tokenId);
    tokenId = AccessTokenKit::GetHapTokenID(userId, HAP_BUNDLE_NAME1, instIndex);
    AccessTokenKit::DeleteToken(tokenId);
}

/**
* @tc.name: SaveData001
* @tc.desc: Abnormal test of SaveData, unifiedData is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SaveData001, TestSize.Level1)
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
HWTEST_F(UdmfServiceImplTest, RetrieveData001, TestSize.Level1)
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
HWTEST_F(UdmfServiceImplTest, IsPermissionInCache002, TestSize.Level1)
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
HWTEST_F(UdmfServiceImplTest, UpdateData001, TestSize.Level1)
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
HWTEST_F(UdmfServiceImplTest, GetSummary001, TestSize.Level1)
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
HWTEST_F(UdmfServiceImplTest, Sync001, TestSize.Level1)
{
    QueryOption query;
    std::vector<std::string> devices = {"device1"};
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.Sync(query, devices);
    EXPECT_EQ(ret, E_NO_PERMISSION);
}

/**
* @tc.name: VerifyPermission001
* @tc.desc: Abnormal test of VerifyPermission, permission is empty
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, VerifyPermission001, TestSize.Level1)
{
    std::string permission;
    uint32_t callerTokenId = 0;
    UdmfServiceImpl udmfServiceImpl;
    bool ret = udmfServiceImpl.VerifyPermission(permission, callerTokenId);
    EXPECT_FALSE(ret);
}

/**
* @tc.name: IsRemoteData001
* @tc.desc: Abnormal test of IsRemoteData, query.key is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, IsRemoteData001, TestSize.Level1)
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
HWTEST_F(UdmfServiceImplTest, SetAppShareOption001, TestSize.Level1)
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
HWTEST_F(UdmfServiceImplTest, SetAppShareOption002, TestSize.Level1)
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
HWTEST_F(UdmfServiceImplTest, SetAppShareOption003, TestSize.Level1)
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
HWTEST_F(UdmfServiceImplTest, SetAppShareOption004, TestSize.Level1)
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
HWTEST_F(UdmfServiceImplTest, OnUserChangeTest001, TestSize.Level1)
{
    // Clear store
    StoreCache::GetInstance().CloseStores();
    auto stores = StoreCache::GetInstance().stores_.Size();
    ASSERT_EQ(stores, 0);
    // Init store
    StoreCache::GetInstance().GetStore("SystemShare");
    stores = StoreCache::GetInstance().stores_.Size();
    ASSERT_EQ(stores, 1);

    uint32_t code = static_cast<uint32_t>(DistributedData::AccountStatus::DEVICE_ACCOUNT_STOPPING);
    std::string user = "OH_USER_test";
    std::string account = "OH_ACCOUNT_test";
    UdmfServiceImpl udmfServiceImpl;
    auto status = udmfServiceImpl.OnUserChange(code, user, account);
    ASSERT_EQ(status, UDMF::E_OK);
    stores = StoreCache::GetInstance().stores_.Size();
    ASSERT_EQ(stores, 0);
}

/**
* @tc.name: OnUserChangeTest002
* @tc.desc: OnUserChange test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, OnUserChangeTest002, TestSize.Level1)
{
    // Clear store
    StoreCache::GetInstance().CloseStores();
    auto stores = StoreCache::GetInstance().stores_.Size();
    ASSERT_EQ(stores, 0);
    // Init store
    StoreCache::GetInstance().GetStore(STORE_ID);
    stores = StoreCache::GetInstance().stores_.Size();
    ASSERT_EQ(stores, 1);

    uint32_t code = static_cast<uint32_t>(DistributedData::AccountStatus::DEVICE_ACCOUNT_STOPPED);
    std::string user = "OH_USER_test";
    std::string account = "OH_ACCOUNT_test";
    UdmfServiceImpl udmfServiceImpl;
    auto status = udmfServiceImpl.OnUserChange(code, user, account);
    ASSERT_EQ(status, UDMF::E_OK);
    stores = StoreCache::GetInstance().stores_.Size();
    ASSERT_EQ(stores, 0);
}

/**
* @tc.name: OnUserChangeTest003
* @tc.desc: OnUserChange test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, OnUserChangeTest003, TestSize.Level1)
{
    // Clear store
    StoreCache::GetInstance().CloseStores();
    auto stores = StoreCache::GetInstance().stores_.Size();
    ASSERT_EQ(stores, 0);
    // Init store
    StoreCache::GetInstance().GetStore(STORE_ID);
    stores = StoreCache::GetInstance().stores_.Size();
    ASSERT_EQ(stores, 1);

    uint32_t code = static_cast<uint32_t>(DistributedData::AccountStatus::DEVICE_ACCOUNT_SWITCHED);
    std::string user = "OH_USER_test";
    std::string account = "OH_ACCOUNT_test";
    UdmfServiceImpl udmfServiceImpl;
    auto status = udmfServiceImpl.OnUserChange(code, user, account);
    ASSERT_EQ(status, UDMF::E_OK);
    stores = StoreCache::GetInstance().stores_.Size();
    ASSERT_EQ(stores, 0);
}

/**
* @tc.name: OnUserChangeTest004
* @tc.desc: OnUserChange test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, OnUserChangeTest004, TestSize.Level1)
{
    // Clear store
    StoreCache::GetInstance().CloseStores();
    auto stores = StoreCache::GetInstance().stores_.Size();
    ASSERT_EQ(stores, 0);
    // Init store
    StoreCache::GetInstance().GetStore(STORE_ID);
    stores = StoreCache::GetInstance().stores_.Size();
    ASSERT_EQ(stores, 1);

    uint32_t code = static_cast<uint32_t>(DistributedData::AccountStatus::DEVICE_ACCOUNT_UNLOCKED);
    std::string user = "OH_USER_test";
    std::string account = "OH_ACCOUNT_test";
    UdmfServiceImpl udmfServiceImpl;
    auto status = udmfServiceImpl.OnUserChange(code, user, account);
    ASSERT_EQ(status, UDMF::E_OK);
    stores = StoreCache::GetInstance().stores_.Size();
    ASSERT_EQ(stores, 1);
}

/**
* @tc.name: SaveMetaData001
* @tc.desc: Abnormal testcase of GetRuntime
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SaveMetaData001, TestSize.Level0)
{
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);
    
    result = store->Init();
    EXPECT_TRUE(result);
}

/**
* @tc.name: SaveMetaData001
* @tc.desc: Abnormal testcase of GetRuntime
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SyncTest001, TestSize.Level0)
{
    QueryOption query;
    query.key = "udmf://drag/ohos.test.demo1/_aS6adWi7<Dehfffffffffffffffff";
    query.tokenId = 1;
    query.intention  = UD_INTENTION_DRAG;
    UdmfServiceImpl udmfServiceImpl;
    DistributedData::StoreMetaData meta = DistributedData::StoreMetaData("100", "distributeddata", "drag");
    std::vector<std::string> devices = {"remote_device"};

    auto ret = udmfServiceImpl.Sync(query, devices);
    EXPECT_EQ(ret, UDMF::E_NO_PERMISSION);
}

/**
* @tc.name: ResolveAutoLaunch001
* @tc.desc: test ResolveAutoLaunch
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, ResolveAutoLaunch001, TestSize.Level0)
{
    auto store = std::make_shared<RuntimeStore>(STORE_ID);
    bool result = store->Init();
    EXPECT_TRUE(result);
    
    DistributedDB::AutoLaunchParam param {
        .userId = "100",
        .appId = "distributeddata",
        .storeId = "drag",
    };
    std::string identifier = "identifier";
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    auto ret = udmfServiceImpl->ResolveAutoLaunch(identifier, param);
    EXPECT_EQ(ret, UDMF::E_OK);
}

/**
* @tc.name: TransferToEntriesIfNeedTest001
* @tc.desc: TransferToEntriesIfNeed test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, TransferToEntriesIfNeedTest001, TestSize.Level1)
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
 * @tc.name: IsValidInput001
 * @tc.desc: invalid unifiedData
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, IsValidInput001, TestSize.Level1)
{
    QueryOption query;
    UnifiedData unifiedData;
    UnifiedKey key;

    UdmfServiceImpl impl;
    bool result = impl.IsValidInput(query, unifiedData, key);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsValidInput002
 * @tc.desc: invalid key
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, IsValidInput002, TestSize.Level1)
{
    QueryOption query;
    UnifiedData unifiedData;
    auto record1 = std::make_shared<UnifiedRecord>();
    auto record2 = std::make_shared<UnifiedRecord>();
    unifiedData.AddRecord(record1);
    unifiedData.AddRecord(record2);
    UnifiedKey key;

    UdmfServiceImpl impl;
    bool result = impl.IsValidInput(query, unifiedData, key);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsValidInput003
 * @tc.desc: valid input
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, IsValidInput003, TestSize.Level1)
{
    QueryOption query;
    query.intention = Intention::UD_INTENTION_DATA_HUB;
    query.key = "udmf://DataHub/aaa/N]2fIEMbrJj@<hH7zpXzzQ>wp:jMuPa7";
    UnifiedData unifiedData;
    auto record1 = std::make_shared<UnifiedRecord>();
    auto record2 = std::make_shared<UnifiedRecord>();
    unifiedData.AddRecord(record1);
    unifiedData.AddRecord(record2);
    UnifiedKey key("udmf://DataHub/aaa/N]2fIEMbrJj@<hH7zpXzzQ>wp:jMuPa7");
    EXPECT_TRUE(key.IsValid());

    UdmfServiceImpl impl;
    bool result = impl.IsValidInput(query, unifiedData, key);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: IsValidInput004
 * @tc.desc: invalid intention
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, IsValidInput004, TestSize.Level1)
{
    QueryOption query;
    query.intention = Intention::UD_INTENTION_DRAG;
    UnifiedData unifiedData;
    UnifiedKey key("udmf://drag/aaa/N]2fIEMbrJj@<hH7zpXzzQ>wp:jMuPa7");

    UdmfServiceImpl impl;
    bool result = impl.IsValidInput(query, unifiedData, key);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: UpdateData002
 * @tc.desc: invalid parameter
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, UpdateData002, TestSize.Level1)
{
    QueryOption query;
    query.intention = Intention::UD_INTENTION_DATA_HUB;
    query.key = "udmf://DataHub/aaa/N]2fIEMbrJj@<hH7zpXzzQ>wp:jMuPa7";
    query.tokenId = 99999;
    UnifiedData unifiedData;
    auto record1 = std::make_shared<UnifiedRecord>();
    auto record2 = std::make_shared<UnifiedRecord>();
    unifiedData.AddRecord(record1);
    unifiedData.AddRecord(record2);

    UdmfServiceImpl impl;
    EXPECT_NE(impl.UpdateData(query, unifiedData), E_OK);
}

/**
 * @tc.name: UpdateData003
 * @tc.desc: invalid parameter
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, UpdateData003, TestSize.Level1)
{
    QueryOption query;
    query.intention = Intention::UD_INTENTION_DATA_HUB;
    query.key = "udmf://DataHub/aaa/N]2fIEMbrJj@<hH7zpXzzQ>wp:jMuPa7";
    query.tokenId = static_cast<uint32_t>(IPCSkeleton::GetCallingTokenID());
    UnifiedData unifiedData;
    auto record1 = std::make_shared<UnifiedRecord>();
    auto record2 = std::make_shared<UnifiedRecord>();
    unifiedData.AddRecord(record1);
    unifiedData.AddRecord(record2);

    UdmfServiceImpl impl;
    EXPECT_NE(impl.UpdateData(query, unifiedData), E_OK);
}

/**
 * @tc.name: UpdateData004
 * @tc.desc: invalid parameter
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, UpdateData004, TestSize.Level1)
{
    QueryOption query;
    UnifiedData unifiedData;

    query.key = "invalid_key";
    UdmfServiceImpl impl;
    EXPECT_EQ(impl.UpdateData(query, unifiedData), E_INVALID_PARAMETERS);
}

/**
* @tc.name: SaveData002
* @tc.desc: invalid parameter
* @tc.type: FUNC
*/
HWTEST_F(UdmfServiceImplTest, SaveData002, TestSize.Level1)
{
    CustomOption option;
    QueryOption query;
    UnifiedData unifiedData;
    std::string key = "";

    UdmfServiceImpl impl;
    EXPECT_EQ(impl.SaveData(option, unifiedData, key), E_INVALID_PARAMETERS);
}

/**
* @tc.name: SaveData003
* @tc.desc: invalid parameter
* @tc.type: FUNC
*/
HWTEST_F(UdmfServiceImplTest, SaveData003, TestSize.Level1)
{
    CustomOption option;
    QueryOption query;
    UnifiedData unifiedData;
    auto record1 = std::make_shared<UnifiedRecord>();
    auto record2 = std::make_shared<UnifiedRecord>();
    unifiedData.AddRecord(record1);
    unifiedData.AddRecord(record2);
    std::string key = "";
    option.intention = Intention::UD_INTENTION_BASE;

    UdmfServiceImpl impl;
    EXPECT_EQ(impl.SaveData(option, unifiedData, key), E_INVALID_PARAMETERS);
}

/**
* @tc.name: SaveData004
* @tc.desc: invalid parameter
* @tc.type: FUNC
*/
HWTEST_F(UdmfServiceImplTest, SaveData004, TestSize.Level1)
{
    CustomOption option;
    QueryOption query;
    UnifiedData unifiedData;
    auto record1 = std::make_shared<UnifiedRecord>();
    auto record2 = std::make_shared<UnifiedRecord>();
    unifiedData.AddRecord(record1);
    unifiedData.AddRecord(record2);
    std::string key = "";
    option.intention = Intention::UD_INTENTION_DATA_HUB;
    option.tokenId = 99999;

    UdmfServiceImpl impl;
    EXPECT_NE(impl.SaveData(option, unifiedData, key), E_OK);
}

/**
 * @tc.name: IsValidInput005
 * @tc.desc: valid input
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, IsValidInput005, TestSize.Level1)
{
    QueryOption query;
    query.intention = Intention::UD_INTENTION_DRAG;
    query.key = "udmf://drag/aaa/N]2fIEMbrJj@<hH7zpXzzQ>wp:jMuPa7";
    UnifiedData unifiedData;
    auto record1 = std::make_shared<UnifiedRecord>();
    auto record2 = std::make_shared<UnifiedRecord>();
    unifiedData.AddRecord(record1);
    unifiedData.AddRecord(record2);
    UnifiedKey key("udmf://drag/aaa/N]2fIEMbrJj@<hH7zpXzzQ>wp:jMuPa7");
    EXPECT_TRUE(key.IsValid());

    UdmfServiceImpl impl;
    bool result = impl.IsValidInput(query, unifiedData, key);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: PushDelayData001
 * @tc.desc: valid input
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, PushDelayData001, TestSize.Level1)
{
    UnifiedData unifiedData;
    auto record1 = std::make_shared<UnifiedRecord>();
    auto record2 = std::make_shared<UnifiedRecord>();
    unifiedData.AddRecord(record1);
    unifiedData.AddRecord(record2);
    std::string key = "invalid key";

    UdmfServiceImpl impl;
    auto result = impl.PushDelayData(key, unifiedData);
    EXPECT_NE(result, E_OK);
}

/**
 * @tc.name: CheckAppId001
 * @tc.desc: invalid bundleName
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, CheckAppId001, TestSize.Level1)
{
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    runtime->appId = "appId";
    std::string bundleName = "ohos.test.demo1";

    UdmfServiceImpl impl;
    int32_t result = impl.CheckAppId(runtime, bundleName);
    EXPECT_EQ(result, E_INVALID_PARAMETERS);
}

/**
 * @tc.name: CheckAppId002
 * @tc.desc: invalid runtime
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, CheckAppId002, TestSize.Level1)
{
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    std::string bundleName = "ohos.test.demo1";

    UdmfServiceImpl impl;
    int32_t result = impl.CheckAppId(runtime, bundleName);
    EXPECT_EQ(result, E_INVALID_PARAMETERS);
}

/**
* @tc.name: CheckDeleteDataPermission001
* @tc.desc: runtime is null
* @tc.type: FUNC
*/
HWTEST_F(UdmfServiceImplTest, CheckDeleteDataPermission001, TestSize.Level1)
{
    std::string appId;
    std::shared_ptr<Runtime> runtime;
    QueryOption query;
    UdmfServiceImpl impl;
    bool ret = impl.CheckDeleteDataPermission(appId, runtime, query);
    EXPECT_FALSE(ret);
}

/**
* @tc.name: CheckDeleteDataPermission002
* @tc.desc: query.tokenId is invalid
* @tc.type: FUNC
*/
HWTEST_F(UdmfServiceImplTest, CheckDeleteDataPermission002, TestSize.Level1)
{
    std::string appId;
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    runtime->tokenId = TOKEN_ID;
    QueryOption query;
    UdmfServiceImpl impl;
    bool ret = impl.CheckDeleteDataPermission(appId, runtime, query);
    EXPECT_FALSE(ret);
}

/**
* @tc.name: CheckDeleteDataPermission003
* @tc.desc: Normal test
* @tc.type: FUNC
*/
HWTEST_F(UdmfServiceImplTest, CheckDeleteDataPermission003, TestSize.Level1)
{
    std::string appId;
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    runtime->tokenId = TOKEN_ID;
    QueryOption query;
    query.tokenId = TOKEN_ID;
    UdmfServiceImpl impl;
    bool ret = impl.CheckDeleteDataPermission(appId, runtime, query);
    EXPECT_TRUE(ret);
}

/**
* @tc.name: CheckDeleteDataPermission004
* @tc.desc: bundleName is empty
* @tc.type: FUNC
*/
HWTEST_F(UdmfServiceImplTest, CheckDeleteDataPermission004, TestSize.Level1)
{
    std::string appId;
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    runtime->appId = APP_ID;
    QueryOption query;
    query.tokenId = TOKEN_ID;
    UdmfServiceImpl impl;
    bool ret = impl.CheckDeleteDataPermission(appId, runtime, query);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: VerifyDataAccessPermission001
 * @tc.desc: no permission
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, VerifyDataAccessPermission001, TestSize.Level1)
{
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    const QueryOption query;
    UnifiedData unifiedData;

    UdmfServiceImpl impl;
    auto result = impl.VerifyDataAccessPermission(runtime, query, unifiedData);
    EXPECT_EQ(result, E_NO_PERMISSION);
}

/**
 * @tc.name: VerifyDataAccessPermission002
 * @tc.desc: runtime is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, VerifyDataAccessPermission002, TestSize.Level1)
{
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    Privilege privilege {
        .tokenId = TOKEN_ID,
        .readPermission = STORE_ID,
        .writePermission = STORE_ID,
    };
    runtime->privileges = { privilege };
    QueryOption query;
    query.tokenId = TOKEN_ID;
    UnifiedData unifiedData;

    UdmfServiceImpl impl;
    auto result = impl.VerifyDataAccessPermission(runtime, query, unifiedData);
    EXPECT_EQ(runtime->privileges[0].tokenId, query.tokenId);
    EXPECT_EQ(result, OHOS::UDMF::E_OK);
}

/**
 * @tc.name: PushDelayData002
 * @tc.desc: DelayData callback and block cache not exist
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, PushDelayData002, TestSize.Level1)
{
    QueryOption query;
    query.key = "k1";
    query.tokenId = 123;
    UnifiedData insertedData;
    insertedData.AddRecord(std::make_shared<UnifiedRecord>());

    UdmfServiceImpl service;
    auto status = service.PushDelayData(query.key, insertedData);
    EXPECT_EQ(status, UDMF::E_INVALID_PARAMETERS);
}

/**
 * @tc.name: SaveData005
 * @tc.desc: test no permission
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, SaveData005, TestSize.Level1)
{
    CustomOption option;
    option.intention = UD_INTENTION_DRAG;
    option.tokenId = Security::AccessToken::AccessTokenKit::GetHapTokenID(100, HAP_BUNDLE_NAME, 0);
    
    std::string key = "";
    UnifiedData unifiedData;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[FILE_URI_PARAM] = "file://error_bundle_name/a.jpeg";
    obj->value_[FILE_TYPE] = "general.image";
    auto record = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj);
    unifiedData.AddRecord(record);

    UdmfServiceImpl impl;
    auto status = impl.SaveData(option, unifiedData, key);
    EXPECT_EQ(status, E_NO_PERMISSION);
}

/**
 * @tc.name: VerifySavedTokenId001
 * @tc.desc: test process uri verify fail
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, VerifySavedTokenId001, TestSize.Level1)
{
    UdmfServiceImpl service;
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    runtime->tokenId = 100000;
    runtime->sourcePackage = "ohos.test.demo";
    auto status = service.VerifySavedTokenId(runtime);
    EXPECT_FALSE(status);
}

/**
 * @tc.name: VerifySavedTokenId002
 * @tc.desc: test process uri verify success
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, VerifySavedTokenId002, TestSize.Level1)
{
    UdmfServiceImpl service;
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    runtime->tokenId = AccessTokenKit::GetHapTokenID(100, HAP_BUNDLE_NAME, 0);
    runtime->sourcePackage = HAP_BUNDLE_NAME;
    auto status = service.VerifySavedTokenId(runtime);
    EXPECT_TRUE(status);
}

/**
 * @tc.name: VerifySavedTokenId003
 * @tc.desc: test process uri verify fail with different bundleName
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, VerifySavedTokenId003, TestSize.Level1)
{
    UdmfServiceImpl service;
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    runtime->tokenId = AccessTokenKit::GetHapTokenID(100, HAP_BUNDLE_NAME, 0);
    runtime->sourcePackage = "com.test.demo111";
    auto status = service.VerifySavedTokenId(runtime);
    EXPECT_FALSE(status);
}

/**
 * @tc.name: ProcessCrossDeviceData001
 * @tc.desc: test ProcessCrossDeviceData with local
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, ProcessCrossDeviceData001, TestSize.Level1)
{
    UdmfServiceImpl service;
    Runtime runtime;
    runtime.deviceId = PreProcessUtils::GetLocalDeviceId();
    UnifiedData unifiedData;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[ORI_URI] = "file://error_bundle_name/a.jpeg";
    obj->value_[FILE_TYPE] = "general.image";
    obj->value_[PERMISSION_POLICY] = static_cast<int32_t>(ONLY_READ);
    auto record = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj);
    unifiedData.AddRecord(record);

    std::shared_ptr<Object> obj1 = std::make_shared<Object>();
    obj1->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj1->value_[ORI_URI] = "https://error_bundle_name/a.jpeg";
    obj1->value_[FILE_TYPE] = "general.image";
    auto record1 = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj1);
    unifiedData.AddRecord(record1);

    std::shared_ptr<Object> obj2 = std::make_shared<Object>();
    obj2->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj2->value_[FILE_TYPE] = "general.image";
    auto record2 = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj2);
    unifiedData.AddRecord(record2);

    std::shared_ptr<Object> obj3 = std::make_shared<Object>();
    obj3->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj3->value_[ORI_URI] = "/error_bundle_name/a.jpeg";
    obj3->value_[FILE_TYPE] = "general.image";
    auto record3 = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj3);
    unifiedData.AddRecord(record3);

    unifiedData.SetRuntime(runtime);
    uint32_t tokenId = AccessTokenKit::GetHapTokenID(100, HAP_BUNDLE_NAME, 0);
    std::vector<Uri> readUris;
    std::vector<Uri> writeUris;
    service.ProcessCrossDeviceData(tokenId, unifiedData, readUris, writeUris);
    EXPECT_EQ(readUris.size(), 1);
    EXPECT_EQ(writeUris.size(), 0);
}

/**
 * @tc.name: ProcessCrossDeviceData002
 * @tc.desc: test ProcessCrossDeviceData with remote
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, ProcessCrossDeviceData002, TestSize.Level1)
{
    UdmfServiceImpl service;
    Runtime runtime;
    runtime.deviceId = "111111";
    UnifiedData unifiedData;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[ORI_URI] = "file://error_bundle_name/a.jpeg";
    obj->value_[REMOTE_URI] = "file://error_bundle_name/a.jpeg";
    obj->value_[FILE_TYPE] = "general.image";
    obj->value_[PERMISSION_POLICY] = static_cast<int32_t>(READ_WRITE);
    auto record = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj);
    unifiedData.AddRecord(record);

    std::shared_ptr<Object> obj1 = std::make_shared<Object>();
    obj1->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj1->value_[ORI_URI] = "file://error_bundle_name/a.jpeg";
    obj1->value_[REMOTE_URI] = "https://error_bundle_name/a.jpeg";
    obj1->value_[FILE_TYPE] = "general.image";
    auto record1 = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj1);
    unifiedData.AddRecord(record1);

    std::shared_ptr<Object> obj2 = std::make_shared<Object>();
    obj2->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj2->value_[ORI_URI] = "file://error_bundle_name/a.jpeg";
    obj2->value_[FILE_TYPE] = "general.image";
    auto record2 = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj2);
    unifiedData.AddRecord(record2);

    std::shared_ptr<Object> obj3 = std::make_shared<Object>();
    obj3->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj3->value_[ORI_URI] = "file://error_bundle_name/a.jpeg";
    obj3->value_[REMOTE_URI] = "/error_bundle_name/a.jpeg";
    obj3->value_[FILE_TYPE] = "general.image";
    auto record3 = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj3);
    unifiedData.AddRecord(record3);

    unifiedData.SetRuntime(runtime);
    uint32_t tokenId = AccessTokenKit::GetHapTokenID(100, HAP_BUNDLE_NAME, 0);
    std::vector<Uri> readUris;
    std::vector<Uri> writeUris;
    service.ProcessCrossDeviceData(tokenId, unifiedData, readUris, writeUris);
    EXPECT_EQ(readUris.size(), 0);
    EXPECT_EQ(writeUris.size(), 1);
}

/**
 * @tc.name: ProcessCrossDeviceData003
 * @tc.desc: test ProcessCrossDeviceData with remote
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, ProcessCrossDeviceData003, TestSize.Level1)
{
    UdmfServiceImpl service;
    Runtime runtime;
    runtime.deviceId = PreProcessUtils::GetLocalDeviceId();
    UnifiedData unifiedData;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[ORI_URI] = "file://error_bundle_name/a.jpeg";
    obj->value_[FILE_TYPE] = "general.image";
    auto record = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj);
    unifiedData.AddRecord(record);

    unifiedData.SetRuntime(runtime);
    uint32_t tokenId = AccessTokenKit::GetHapTokenID(100, HAP_BUNDLE_NAME, 0);
    std::vector<Uri> readUris;
    std::vector<Uri> writeUris;
    auto result = service.ProcessCrossDeviceData(tokenId, unifiedData, readUris, writeUris);
    EXPECT_EQ(result, E_OK);
}

/**
 * @tc.name: ProcessUri001
 * @tc.desc: test process uri verify fail
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, ProcessUri001, TestSize.Level1)
{
    UdmfServiceImpl service;
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    runtime->tokenId = AccessTokenKit::GetHapTokenID(100, HAP_BUNDLE_NAME, 0);
    runtime->sourcePackage = HAP_BUNDLE_NAME;
    runtime->deviceId = PreProcessUtils::GetLocalDeviceId();
    UnifiedData data;
    data.SetRuntime(*runtime);
    QueryOption option;
    option.key = "udmf://drag/com.test.demo/ascdca";
    option.tokenId = AccessTokenKit::GetHapTokenID(100, HAP_BUNDLE_NAME1, 0);
    option.intention = Intention::UD_INTENTION_DRAG;
    auto status = service.ProcessUri(option, data);
    EXPECT_EQ(status, E_OK);
}

/**
 * @tc.name: ProcessUri002
 * @tc.desc: test process uri verify fail
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, ProcessUri002, TestSize.Level1)
{
    UdmfServiceImpl service;
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    runtime->tokenId = AccessTokenKit::GetHapTokenID(100, HAP_BUNDLE_NAME, 0);
    runtime->sourcePackage = HAP_BUNDLE_NAME;
    runtime->deviceId = "11111";
    UnifiedData data;
    data.SetRuntime(*runtime);
    QueryOption option;
    option.key = "udmf://drag/com.test.demo/ascdca";
    option.tokenId = AccessTokenKit::GetHapTokenID(100, HAP_BUNDLE_NAME1, 0);
    option.intention = Intention::UD_INTENTION_DRAG;
    auto status = service.ProcessUri(option, data);
    EXPECT_EQ(status, E_OK);
}

/**
 * @tc.name: ProcessUri003
 * @tc.desc: test process uri verify fail
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, ProcessUri003, TestSize.Level1)
{
    UdmfServiceImpl service;
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    runtime->tokenId = 1111111;
    runtime->sourcePackage = HAP_BUNDLE_NAME;
    runtime->deviceId = PreProcessUtils::GetLocalDeviceId();
    UnifiedData data;
    data.SetRuntime(*runtime);
    QueryOption option;
    option.key = "udmf://drag/com.test.demo/ascdca";
    option.tokenId = 111111;
    option.intention = Intention::UD_INTENTION_DRAG;
    auto status = service.ProcessUri(option, data);
    EXPECT_EQ(status, E_ERROR);
}

/**
 * @tc.name: GrantUriPermission001
 * @tc.desc: GrantUriPermission function test
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, GrantUriPermission001, TestSize.Level1)
{
    std::vector<Uri> readUris;
    std::string strUri = "picture.png";
    readUris.emplace_back(Uri(strUri));
    std::vector<Uri> writeUris;
    auto tokenId = AccessTokenKit::GetHapTokenID(100, HAP_BUNDLE_NAME, 0);
    const std::string queryKey = "udmf://drag/com.test.demo/ascdca";
    auto result = UriPermissionManager::GetInstance().GrantUriPermission(readUris, writeUris, tokenId, tokenId, false);
    EXPECT_EQ(result, E_NO_PERMISSION);
    result = UriPermissionManager::GetInstance().GrantUriPermission(readUris, writeUris, tokenId, tokenId, true);
    EXPECT_EQ(result, E_NO_PERMISSION);
    readUris.clear();
    writeUris.emplace_back(Uri(strUri));
    result = UriPermissionManager::GetInstance().GrantUriPermission(readUris, writeUris, tokenId, tokenId, false);
    EXPECT_EQ(result, E_NO_PERMISSION);
    result = UriPermissionManager::GetInstance().GrantUriPermission(readUris, writeUris, tokenId, tokenId, true);
    EXPECT_EQ(result, E_NO_PERMISSION);
}

/**
 * @tc.name: HandleFileUris001
 * @tc.desc: HandleFileUris function test
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, HandleFileUris001, TestSize.Level1)
{
    UnifiedData unifiedData;
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    runtime->tokenId = AccessTokenKit::GetHapTokenID(100, HAP_BUNDLE_NAME, 0);
    runtime->sourcePackage = HAP_BUNDLE_NAME;
    runtime->deviceId = "11111";
    runtime->createPackage = "test";
    unifiedData.SetRuntime(*runtime);
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[ORI_URI] = "file://ohos.mytest.demo/data/storage/el2/base/haps/103.png";
    obj->value_[FILE_TYPE] = "general.image";
    obj->value_[PERMISSION_POLICY] = static_cast<int32_t>(ONLY_READ);
    auto record = std::make_shared<UnifiedRecord>(UDType::FILE_URI, obj);
    unifiedData.AddRecord(record);
    std::string html = "<img data-ohos='clipboard' src='file:///data/storage/el2/base/haps/102.png'>"
                        "<img data-ohos='clipboard' src='file:///data/storage/el2/base/haps/103.png'>";
    std::shared_ptr<Object> obj1 = std::make_shared<Object>();
    obj1->value_["uniformDataType"] = "general.html";
    obj1->value_["htmlContent"] = html;
    obj1->value_["plainContent"] = "htmlPlainContent";
    auto record1 = std::make_shared<UnifiedRecord>(UDType::HTML, obj1);
    unifiedData.AddRecord(record1);
    auto tokenId = AccessTokenKit::GetHapTokenID(100, HAP_BUNDLE_NAME, 0);
    auto result = PreProcessUtils::HandleFileUris(tokenId, unifiedData);
    EXPECT_EQ(result, E_NO_PERMISSION);
}

/**
 * @tc.name: FillDelayUnifiedData001
 * @tc.desc: FillDelayUnifiedData function test
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, FillDelayUnifiedData001, TestSize.Level1)
{
    UdmfServiceImpl service;
    UnifiedKey key("udmf://drag/com.test.demo/ascdca");
    UnifiedData unifiedData;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.plain-text";
    obj->value_["plainContent"] = "This is a test plain text.";
    auto record = std::make_shared<UnifiedRecord>(UDType::PLAIN_TEXT, obj);
    unifiedData.AddRecord(record);
    EXPECT_TRUE(record->GetUid().empty());
    auto status = service.FillDelayUnifiedData(key, unifiedData);
    EXPECT_EQ(status, E_OK);
    EXPECT_FALSE(record->GetUid().empty());
}

/**
 * @tc.name: UpdateDelayData001
 * @tc.desc: UpdateDelayData function test
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, UpdateDelayData001, TestSize.Level1)
{
    UdmfServiceImpl service;
    std::string key = "udmf://drag/com.test.demo/ascdca";
    UnifiedData unifiedData;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.plain-text";
    obj->value_["plainContent"] = "This is a test plain text.";
    auto record = std::make_shared<UnifiedRecord>(UDType::PLAIN_TEXT, obj);
    unifiedData.AddRecord(record);
    Runtime runtime;
    UnifiedKey unifiedKey(key);
    runtime.key = unifiedKey;
    unifiedData.SetRuntime(runtime);
    auto status = service.UpdateDelayData(key, unifiedData);
    EXPECT_EQ(status, E_ERROR);
}

/**
 * @tc.name: GetDevicesForDelayData001
 * @tc.desc: UpdateDelayData function test
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, GetDevicesForDelayData001, TestSize.Level1)
{
    UdmfServiceImpl service;
    auto devices = service.GetDevicesForDelayData();
    EXPECT_EQ(devices.size(), 0);

    std::string deviceId = "device_001";
    SyncedDeviceContainer::GetInstance().SaveSyncedDeviceInfo(deviceId);
    devices = service.GetDevicesForDelayData();
    EXPECT_EQ(devices.size(), 0);
}

/**
 * @tc.name: PushDelayDataToRemote001
 * @tc.desc: PushDelayDataToRemote function test
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, PushDelayDataToRemote001, TestSize.Level1)
{
    UdmfServiceImpl service;
    std::string key = "udmf://DataHub/com.example.app/1233455";
    QueryOption query;
    query.key = key;
    query.intention = Intention::UD_INTENTION_DRAG;
    std::vector<std::string> deviceIds;
    auto status = service.PushDelayDataToRemote(query, deviceIds);
    EXPECT_EQ(status, E_OK);

    deviceIds = { "device_001", "device_002" };
    status = service.PushDelayDataToRemote(query, deviceIds);
    EXPECT_EQ(status, E_INVALID_PARAMETERS);

    key = "udmf://drag/com.example.app/1233455";
    query.key = key;
    status = service.PushDelayDataToRemote(query, deviceIds);
    EXPECT_EQ(status, E_DB_ERROR);
}

/**
 * @tc.name: HandleRemoteDelayData001
 * @tc.desc: HandleRemoteDelayData function test
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, HandleRemoteDelayData001, TestSize.Level1)
{
    UdmfServiceImpl service;
    std::string key = "udmf://DataHub/com.example.app/1233455";
    auto status = service.HandleRemoteDelayData(key);
    EXPECT_EQ(status, E_ERROR);
}

/**
 * @tc.name: OnAppUninstall001
 * @tc.desc: OnAppUninstall001 function test
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, OnAppUninstall001, TestSize.Level1)
{
    auto tokenId = IPCSkeleton::GetSelfTokenID();
    CustomOption option {UD_INTENTION_DRAG, tokenId};

    UdmfServiceImpl service;
    auto executors = std::make_shared<OHOS::ExecutorPool>(2, 1);
    DistributedData::FeatureSystem::Feature::BindInfo bindInfo;
    bindInfo.executors = executors;
    service.OnBind(bindInfo);
    UnifiedData data;
    data.AddRecord(std::make_shared<PlainText>());
    std::string key;
    auto status = service.SaveData(option, data, key);
    EXPECT_EQ(status, E_OK);
    EXPECT_EQ(1, LifeCycleManager::GetInstance().udKeys_.Size());
    UdmfServiceImpl::UdmfStatic udmfStatic;
    status = udmfStatic.OnAppUninstall("com.demo.test", 1, 1, 123456);
    EXPECT_EQ(status, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(1, LifeCycleManager::GetInstance().udKeys_.Size());
    status = udmfStatic.OnAppUninstall("com.demo.test", 1, 1, tokenId);
    EXPECT_EQ(status, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(0, LifeCycleManager::GetInstance().udKeys_.Size());
}

}; // namespace DistributedDataTest
}; // namespace OHOS::Test
