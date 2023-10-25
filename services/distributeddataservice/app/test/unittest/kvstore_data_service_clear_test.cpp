/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#define LOG_TAG "KvStoreDataServiceClearTest"

#include <memory>
#include <vector>
#include "bootstrap.h"
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "types.h"
#include "device_manager_adapter.h"
#include "kvstore_data_service.h"
#include "kvstore_client_death_observer.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "metadata/appid_meta_data.h"
#include "metadata/secret_key_meta_data.h"
#include "feature/feature_system.h"
#include "hap_token_info.h"
#include "log_print.h"
#include "gtest/gtest.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedKv;
using namespace OHOS::Security::AccessToken;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using MetaDataManager = OHOS::DistributedData::MetaDataManager;
using KvStoreDataService = OHOS::DistributedKv::KvStoreDataService;
namespace OHOS::Test {
class KvStoreDataServiceClearTest : public testing::Test{

    public:
        static void SetUpTestCase(void);
        static void TearDownTestCase(void);

        void SetUp();
        void TearDown();

        NativeTokenInfoParams infoInstance {0};
    protected:       
        KvStoreDataService::StoreMetaData metaData;
        DistributedData::StoreMetaDataLocal localMeta_;

        void InitMetaData();
};

const std::string bundleNameSuccess = "ohos.test.demo";
const std::string bundleNameError = "com.sample.helloworld";
const int32_t userIDSuccess = 100;
const int32_t userIDError = 10;
const int32_t appInDexSuccess = 0;
const int32_t appInDexError = 2;
const int32_t tokenIdError = 222;

void KvStoreDataServiceClearTest::SetUpTestCase(void)
{
    auto executors = std::make_shared<ExecutorPool>(12, 5);
    DistributedData::Bootstrap::GetInstance().LoadComponents();
    DistributedData::Bootstrap::GetInstance().LoadDirectory();
    DistributedData::Bootstrap::GetInstance().LoadCheckers();
    KvStoreMetaManager::GetInstance().BindExecutor(executors);
    KvStoreMetaManager::GetInstance().InitMetaParameter();
    
    DmAdapter::GetInstance().Init(executors);
}

void KvStoreDataServiceClearTest::TearDownTestCase(void)
{
}

void KvStoreDataServiceClearTest::SetUp(void)
{
    infoInstance.dcapsNum = 0;
    infoInstance.permsNum = 0;
    infoInstance.aclsNum = 0;
    infoInstance.dcaps = nullptr;
    infoInstance.perms = nullptr;
    infoInstance.acls = nullptr;
    infoInstance.processName = "foundation";
    infoInstance.aplStr = "system_core";

    HapInfoParams info = {
        .userID = 100,
        .bundleName = "ohos.test.demo",
        .instIndex = 0,
        .appIDDesc = "ohos.test.demo"
    };
    PermissionDef infoManagerTestPermDef = {
        .permissionName = "ohos.permission.test",
        .bundleName = "ohos.test.demo",
        .grantMode = 1,
        .availableLevel = APL_NORMAL,
        .label = "label",
        .labelId = 1,
        .description = "open the door",
        .descriptionId = 1
    };
    PermissionStateFull infoManagerTestState = {
        .permissionName = "ohos.permission.test",
        .isGeneral = true,
        .resDeviceID = {"local"},
        .grantStatus = {PermissionState::PERMISSION_GRANTED},
        .grantFlags = {1}
    };
    HapPolicyParams policy = {
        .apl = APL_NORMAL,
        .domain = "test.domain",
        .permList = {infoManagerTestPermDef},
        .permStateList = {infoManagerTestState}
    };
    AccessTokenKit::AllocHapToken(info, policy);

    InitMetaData();
}

void KvStoreDataServiceClearTest::TearDown(void)
{  
    MetaDataManager::GetInstance().DelMeta(metaData.GetKey());

    auto tokenId = AccessTokenKit::GetHapTokenID(100, "ohos.test.demo", 0);   
    AccessTokenKit::DeleteToken(tokenId);
}

void KvStoreDataServiceClearTest::InitMetaData()
{
    metaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData.user = "100";
    metaData.bundleName = "ohos.test.demo";
    metaData.storeId = "test_store";
    metaData.appId = "ohos.test.demo";
    metaData.tokenId = AccessTokenKit::GetHapTokenID(100, "ohos.test.demo", 0);
    metaData.storeType = 1;
    metaData.area = EL1;
    metaData.instanceId = 0;
    metaData.isAutoSync = true;
    metaData.version = 1;
    metaData.appType = "default";
    metaData.dataDir = "/data/service/el1/public/database/kvstore_data_service_clear_test";

    DistributedData::PolicyValue value;
    value.type = OHOS::DistributedKv::PolicyType::IMMEDIATE_SYNC_ON_ONLINE;
    localMeta_.policies = { std::move(value) };
}

/**
 * @tc.name: ClearAppStorage001
 * @tc.desc: Test that the parameters are entered correctly
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(KvStoreDataServiceClearTest, ClearAppStorage001, TestSize.Level0)
{
    auto kvDataService = OHOS::DistributedKv::KvStoreDataService();
    const int32_t tokenIdSuccess = AccessTokenKit::GetHapTokenID(100, "ohos.test.demo", 0);
    auto dataRetTokenId = 
        kvDataService.ClearAppStorage(bundleNameSuccess, userIDSuccess, appInDexSuccess, tokenIdError);
    EXPECT_EQ(dataRetTokenId, Status::ERROR);
    
    auto dataRetBundleName = 
        kvDataService.ClearAppStorage(bundleNameError, userIDSuccess, appInDexSuccess, tokenIdSuccess);
    EXPECT_EQ(dataRetBundleName, Status::ERROR);
    
    auto dataRetUserId = 
        kvDataService.ClearAppStorage(bundleNameSuccess, userIDError, appInDexSuccess, tokenIdSuccess);
    EXPECT_EQ(dataRetUserId, Status::ERROR);
    
    auto dataRetAppInDex = 
        kvDataService.ClearAppStorage(bundleNameSuccess, userIDSuccess, appInDexError, tokenIdSuccess);
    EXPECT_EQ(dataRetAppInDex, Status::ERROR);
}
/**
 * @tc.name: ClearAppStorage002
 * @tc.desc: The parameters are valid but have no metaData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(KvStoreDataServiceClearTest, ClearAppStorage002, TestSize.Level0)
{
    auto kvDataService = OHOS::DistributedKv::KvStoreDataService();
    const int32_t tokenIdSuccess = AccessTokenKit::GetHapTokenID(100, "ohos.test.demo", 0);

    auto metaDataError = 
        kvDataService.ClearAppStorage(bundleNameSuccess, userIDSuccess, appInDexSuccess, tokenIdSuccess);
    EXPECT_EQ(metaDataError, Status::ERROR);
}
/**
 * @tc.name: ClearAppStorage003
 * @tc.desc: Test that the cleanup is implemented
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(KvStoreDataServiceClearTest, ClearAppStorage003, TestSize.Level0)
{
    KvStoreMetaManager::GetInstance().InitMetaListener(); //

    EXPECT_TRUE(
        MetaDataManager::GetInstance().SaveMeta(metaData.GetKey(), metaData));
    EXPECT_TRUE(
        MetaDataManager::GetInstance().SaveMeta(metaData.GetSecretKey(), metaData));
    EXPECT_TRUE(
        MetaDataManager::GetInstance().SaveMeta(metaData.GetStrategyKey(), metaData));
    EXPECT_TRUE(
        MetaDataManager::GetInstance().SaveMeta(metaData.appId, metaData));
    EXPECT_TRUE(
        MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyLocal(), localMeta_, true));
    EXPECT_TRUE(
        MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData));

    const int32_t tokenIdSuccess = AccessTokenKit::GetHapTokenID(100, "ohos.test.demo", 0);
    auto kvDataService = OHOS::DistributedKv::KvStoreDataService();
    auto metaDataSuccess = 
        kvDataService.ClearAppStorage(bundleNameSuccess, userIDSuccess, appInDexSuccess, tokenIdSuccess);
    EXPECT_EQ(metaDataSuccess, Status::SUCCESS);

    EXPECT_FALSE(
        MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData));
    
    MetaDataManager::GetInstance().DelMeta(metaData.GetKey());
    EXPECT_FALSE(
        MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData));
}
} // namespace OHOS::Test