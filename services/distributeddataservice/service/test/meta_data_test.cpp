/*
* Copyright (c) 2022 Huawei Device Co., Ltd.
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
#define LOG_TAG "MetaDataTest"
#include <gtest/gtest.h>
#include "log_print.h"
#include "ipc_skeleton.h"
#include "device_matrix.h"
#include "executor_pool.h"
#include "accesstoken_kit.h"
#include "bootstrap.h"
#include "token_setproc.h"
#include "nativetoken_kit.h"
#include "kvstore_meta_manager.h"
#include "device_manager_adapter.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "kvdb_service_impl.h"
#include "metadata/store_meta_data_local.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Status = OHOS::DistributedKv::Status;
using Options = OHOS::DistributedKv::Options;
static OHOS::DistributedKv::StoreId storeId = { "meta_test_storeid" };
static OHOS::DistributedKv::AppId appId = { "ohos.test.metadata" };
static constexpr const char *TEST_USER = "0";
namespace OHOS::Test {
namespace DistributedDataTest {
class MetaDataTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    StoreMetaData metaData_;
    Options options_;
    std::shared_ptr<DistributedKv::KVDBServiceImpl> kvdbServiceImpl_;
    int32_t GetInstIndex(uint32_t tokenId, const DistributedKv::AppId &appId);
};

int32_t MetaDataTest::GetInstIndex(uint32_t tokenId, const DistributedKv::AppId &appId)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != TOKEN_HAP) {
        return 0;
    }

    HapTokenInfo tokenInfo;
    tokenInfo.instIndex = -1;
    int errCode = AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (errCode != RET_SUCCESS) {
        return -1;
    }
    return tokenInfo.instIndex;
}

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
    delete []perms;
}

void MetaDataTest::SetUpTestCase(void)
{
    DistributedData::Bootstrap::GetInstance().LoadComponents();
    DistributedData::Bootstrap::GetInstance().LoadDirectory();
    DistributedData::Bootstrap::GetInstance().LoadCheckers();
    GrantPermissionNative();

    size_t max = 12;
    size_t min = 5;
    auto executors = std::make_shared<OHOS::ExecutorPool>(max, min);
    DmAdapter::GetInstance().Init(executors);
    DistributedKv::KvStoreMetaManager::GetInstance().BindExecutor(executors);
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaParameter();
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaListener();
}

void MetaDataTest::TearDownTestCase() {}

void MetaDataTest::SetUp()
{
    options_.isNeedCompress = true;
    kvdbServiceImpl_ = std::make_shared<DistributedKv::KVDBServiceImpl>();
    metaData_.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData_.bundleName = appId.appId;
    metaData_.storeId = storeId.storeId;
    metaData_.user = TEST_USER;
    metaData_.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    metaData_.instanceId = GetInstIndex(metaData_.tokenId, appId);
    metaData_.version = 1;
    MetaDataManager::GetInstance().DelMeta(metaData_.GetKey());
}

void MetaDataTest::TearDown() {}

/**
* @tc.name: SaveLoadMateData
* @tc.desc: save meta data
* @tc.type: FUNC
* @tc.require:
* @tc.author: yl
*/
HWTEST_F(MetaDataTest, SaveLoadMateData, TestSize.Level0)
{
    ZLOGI("SaveLoadMateData start");
    StoreMetaData metaData;
    std::vector<uint8_t> password {};
    auto status = kvdbServiceImpl_->AfterCreate(appId, storeId, options_, password);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData_.GetKey(), metaData));
    ASSERT_TRUE(metaData.isNeedCompress);
}

/**
* @tc.name: MetaDataChanged
* @tc.desc: meta data changed
* @tc.type: FUNC
* @tc.require:
* @tc.author: yl
*/
HWTEST_F(MetaDataTest, MateDataChanged, TestSize.Level0)
{
    ZLOGI("MateDataChangeed start");
    options_.isNeedCompress = false;
    StoreMetaData metaData;
    std::vector<uint8_t> password {};
    auto status = kvdbServiceImpl_->AfterCreate(appId, storeId, options_, password);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData_.GetKey(), metaData));
    ASSERT_FALSE(metaData.isNeedCompress);
    ASSERT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey()));
}
} // namespace DistributedDataTest
} // namespace OHOS::Test
