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

#include <memory>
#include <vector>

#include "gtest/gtest.h"
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

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedKv;
using namespace OHOS::Security::AccessToken;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using MetaDataManager = OHOS::DistributedData::MetaDataManager;
using KvStoreDataService = OHOS::DistributedKv::KvStoreDataService;
namespace OHOS::Test {
class KvStoreDataServiceClearTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

    void SetUp();
    void TearDown();

    NativeTokenInfoParams infoInstance {0};
protected:
    static constexpr const char *TEST_USER = "100";
    static constexpr const char *TEST_BUNDLE = "ohos.test.demo";
    static constexpr const char *TEST_STORE = "test_store";
    static constexpr int32_t TEST_UID = 2000000;
    static constexpr int32_t TEST_USERID = 100;
    static constexpr const char *BUNDLE_NAME = "ohos.test.demo";
    static constexpr const char *BUNDLENAME_NO = "com.sample.helloworld";
    static constexpr int32_t USER_ID = 100;
    static constexpr int32_t USERID_NO = 10;
    static constexpr int32_t APP_INDEX = 0;
    static constexpr int32_t APPINDEX_NO = 2;
    static constexpr int32_t INVALID_TOKEN = 222;

    DistributedData::StoreMetaData metaData_;
    DistributedData::StoreMetaDataLocal localMeta_;

    void InitMetaData();
};

void KvStoreDataServiceClearTest::SetUpTestCase(void)
{
}

void KvStoreDataServiceClearTest::TearDownTestCase(void)
{
}

void KvStoreDataServiceClearTest::SetUp(void)
{
    DistributedData::Bootstrap::GetInstance().LoadComponents();
    DistributedData::Bootstrap::GetInstance().LoadDirectory();
    DistributedData::Bootstrap::GetInstance().LoadCheckers();

    infoInstance.dcapsNum = 0;
    infoInstance.permsNum = 0;
    infoInstance.aclsNum = 0;
    infoInstance.dcaps = nullptr;
    infoInstance.perms = nullptr;
    infoInstance.acls = nullptr;
    infoInstance.processName = "KvStoreDataServiceClearTest";
    infoInstance.aplStr = "system_core";

    HapInfoParams info = {
        .userID = TEST_USERID,
        .bundleName = TEST_BUNDLE,
        .instIndex = 0,
        .appIDDesc = TEST_BUNDLE
    };
    PermissionDef infoManagerTestPermDef = {
        .permissionName = "ohos.permission.test",
        .bundleName = TEST_BUNDLE,
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
}

void KvStoreDataServiceClearTest::TearDown(void)
{
    auto tokenId = AccessTokenKit::GetHapTokenID(TEST_USERID, TEST_BUNDLE, 0);
    AccessTokenKit::DeleteToken(tokenId);
}

void KvStoreDataServiceClearTest::InitMetaData()
{
    metaData_.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData_.user = TEST_USER;
    metaData_.bundleName = TEST_BUNDLE;
    metaData_.storeId = TEST_STORE;
    metaData_.appId = TEST_BUNDLE;
    metaData_.tokenId = AccessTokenKit::GetHapTokenID(TEST_USERID, TEST_BUNDLE, 0);
    metaData_.uid = TEST_UID;
    metaData_.storeType = 1;
    metaData_.area = EL1;
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.version = 1;
    metaData_.appType = "default";
    metaData_.dataDir = "/data/service/el1/public/database/kvstore_data_service_clear_test";

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
HWTEST_F(KvStoreDataServiceClearTest, ClearAppStorage001, TestSize.Level1)
{
    auto kvDataService = OHOS::DistributedKv::KvStoreDataService();
    auto tokenIdOk = AccessTokenKit::GetHapTokenID(TEST_USERID, TEST_BUNDLE, 0);
    auto ret =
        kvDataService.ClearAppStorage(BUNDLE_NAME, USER_ID, APP_INDEX, INVALID_TOKEN);
    EXPECT_EQ(ret, Status::ERROR);

    ret = kvDataService.ClearAppStorage(BUNDLENAME_NO, USER_ID, APP_INDEX, tokenIdOk);
    EXPECT_EQ(ret, Status::ERROR);

    ret = kvDataService.ClearAppStorage(BUNDLE_NAME, USERID_NO, APP_INDEX, tokenIdOk);
    EXPECT_EQ(ret, Status::ERROR);

    ret = kvDataService.ClearAppStorage(BUNDLE_NAME, USER_ID, APPINDEX_NO, tokenIdOk);
    EXPECT_EQ(ret, Status::ERROR);
}

/**
 * @tc.name: ClearAppStorage002
 * @tc.desc: The parameters are valid but have no metaData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(KvStoreDataServiceClearTest, ClearAppStorage002, TestSize.Level1)
{
    auto kvDataService = OHOS::DistributedKv::KvStoreDataService();
    auto tokenIdOk = AccessTokenKit::GetHapTokenID(TEST_USERID, TEST_BUNDLE, 0);

    auto ret =
        kvDataService.ClearAppStorage(BUNDLE_NAME, USER_ID, APP_INDEX, tokenIdOk);
    EXPECT_EQ(ret, Status::ERROR);
}

/**
 * @tc.name: ClearAppStorage003
 * @tc.desc: Test that the cleanup is implemented
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(KvStoreDataServiceClearTest, ClearAppStorage003, TestSize.Level1)
{
    auto executors = std::make_shared<ExecutorPool>(12, 5);
    // Create an object of the ExecutorPool class and pass 12 and 5 as arguments to the constructor of the class
    KvStoreMetaManager::GetInstance().BindExecutor(executors);
    KvStoreMetaManager::GetInstance().InitMetaParameter();
    DmAdapter::GetInstance().Init(executors);
    KvStoreMetaManager::GetInstance().InitMetaListener();

    InitMetaData();

    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_));
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData_.GetSecretKey(), metaData_, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData_.GetStrategyKey(), metaData_));
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData_.appId, metaData_, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyLocal(), localMeta_, true));

    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData_.GetKey(), metaData_));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData_.GetSecretKey(), metaData_, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData_.GetStrategyKey(), metaData_));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData_.appId, metaData_, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData_.GetKeyLocal(), localMeta_, true));

    auto tokenIdOk = AccessTokenKit::GetHapTokenID(TEST_USERID, TEST_BUNDLE, 0);
    auto kvDataService = OHOS::DistributedKv::KvStoreDataService();
    auto ret =
        kvDataService.ClearAppStorage(BUNDLE_NAME, USER_ID, APP_INDEX, tokenIdOk);
    EXPECT_EQ(ret, Status::SUCCESS);

    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData_.GetKey(), metaData_));
    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData_.GetSecretKey(), metaData_, true));
    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData_.GetStrategyKey(), metaData_));
    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData_.appId, metaData_, true));
    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData_.GetKeyLocal(), localMeta_, true));

    MetaDataManager::GetInstance().DelMeta(metaData_.GetKey());
    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData_.GetKey(), metaData_));
}
} // namespace OHOS::Test