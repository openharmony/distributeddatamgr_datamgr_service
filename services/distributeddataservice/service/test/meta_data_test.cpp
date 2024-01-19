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
#include "device_manager_adapter.h"
#include "mock/db_store_mock.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;

namespace OHOS::Test {
namespace DistributedDataTest {
static constexpr const char *TEST_BUNDLE = "meta_test_bundleName";
static constexpr const char *TEST_APPID = "meta_test_appid";
static constexpr const char *TEST_STORE = "meta_test_storeid";
class MetaDataTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    StoreMetaData metaData_;
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
};

void MetaDataTest::SetUpTestCase(void) {}

void MetaDataTest::TearDownTestCase() {}

void MetaDataTest::SetUp()
{
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, [](const auto &, auto) {
        DeviceMatrix::GetInstance().OnChanged(DeviceMatrix::META_STORE_MASK);
    });
    
    metaData_.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData_.appId = TEST_APPID;
    metaData_.bundleName = TEST_BUNDLE;
    metaData_.storeId = TEST_STORE;
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.storeType = 1;
    metaData_.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    metaData_.isNeedCompress = false;
    MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_);
}

void MetaDataTest::TearDown() {}

/**
* @tc.name: LoadMetaData
* @tc.desc: save and load meta data
* @tc.type: FUNC
* @tc.require:
* @tc.author: yl
*/
HWTEST_F(MetaDataTest, LoadMetaData, TestSize.Level0)
{
    ZLOGI("MetaDataTest start");
    StoreMetaData metaData;
    ASSERT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData_.GetKey(), metaData));
    ASSERT_FALSE(metaData.isNeedCompress);
}

/**
* @tc.name: MetaDataChanged
* @tc.desc: meta data changed and save meta
* @tc.type: FUNC
* @tc.require:
* @tc.author: yl
*/
HWTEST_F(MetaDataTest, MateDataChangeed, TestSize.Level0)
{
    ZLOGI("MetaDataTest start");
    StoreMetaData oldMeta;
    ASSERT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData_.GetKey(), oldMeta));
    StoreMetaData changeMeta;
    changeMeta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    changeMeta.appId = TEST_APPID;
    changeMeta.bundleName = TEST_BUNDLE;
    changeMeta.storeId = TEST_STORE;
    changeMeta.instanceId = 0;
    changeMeta.isAutoSync = true;
    changeMeta.storeType = 1;
    changeMeta.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    changeMeta.isNeedCompress = true;
    ASSERT_NE(changeMeta, oldMeta);
    ASSERT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), changeMeta));
    MetaDataManager::GetInstance().DelMeta(metaData_.GetKey());
}
} // namespace DistributedDataTest
} // namespace OHOS::Test
