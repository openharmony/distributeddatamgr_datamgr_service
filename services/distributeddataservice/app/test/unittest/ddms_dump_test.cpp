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

#include "kvstore_data_service.h"
#include "gtest/gtest.h"
#include "db_store_mock.h"
#include "device_manager_adapter.h"
#include "device_manager_adapter_mock.h"
#include "metadata/store_meta_data.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::AppDistributedKv;
using namespace OHOS::DistributedData;

namespace OHOS::Test {
static constexpr const char *TEST_BUNDLE_NAME = "TestApplication";
static constexpr const char *TEST_STORE_NAME = "TestStore";
static constexpr const char *TEST_UUID = "ABCD";

class DDMSDumpTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static void ConfigSendParameters(bool isCancel);
    static std::string foregroundUserId_;
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static StoreMetaData metaData_;
    static DistributedKv::KvStoreDataService dumpTest_;
    static inline std::shared_ptr<DeviceManagerAdapterMock> deviceManagerAdapterMock = nullptr;
};

std::string DDMSDumpTest::foregroundUserId_ = "0";
std::shared_ptr<DBStoreMock> DDMSDumpTest::dbStoreMock_;
DistributedKv::KvStoreDataService DDMSDumpTest::dumpTest_;

void DDMSDumpTest::SetUpTestCase(void)
{
    dbStoreMock_ = std::make_shared<DBStoreMock>();
    deviceManagerAdapterMock = std::make_shared<DeviceManagerAdapterMock>();
    BDeviceManagerAdapter::deviceManagerAdapter = deviceManagerAdapterMock;
    DeviceInfo deviceInfo;
    deviceInfo.uuid = TEST_UUID;
    EXPECT_CALL(*deviceManagerAdapterMock, GetLocalDevice()).WillRepeatedly(Return(deviceInfo));
}

void DDMSDumpTest::TearDownTestCase(void)
{
    deviceManagerAdapterMock = nullptr;
    BDeviceManagerAdapter::deviceManagerAdapter = nullptr;
}

void DDMSDumpTest::SetUp(void)
{}

void DDMSDumpTest::TearDown(void)
{
    ConfigSendParameters(true);
}

void DDMSDumpTest::ConfigSendParameters(bool isCancel)
{
    StoreMetaData localMetaData;
    localMetaData.deviceId = TEST_UUID;
    localMetaData.user = foregroundUserId_;
    localMetaData.bundleName = TEST_BUNDLE_NAME;
    localMetaData.storeId = TEST_STORE_NAME;
    if (isCancel) {
        MetaDataManager::GetInstance().DelMeta(localMetaData.GetKeyWithoutPath());
    } else {
        MetaDataManager::GetInstance().SaveMeta(localMetaData.GetKeyWithoutPath(), localMetaData, true);
    }
}

/**
@tc.name: DumpStoreInfo001
@tc.desc: test DumpStoreInfo function
@tc.type: FUNC
*/
HWTEST_F(DDMSDumpTest, DumpStoreInfo001, TestSize.Level0)
{
    int fd = 0;
    std::map<std::string, std::vector<std::string>> params = {};
    EXPECT_NO_FATAL_FAILURE(dumpTest_.DumpStoreInfo(fd, params));
}

/**
* @tc.name: DumpBundleInfo001
* @tc.desc: test DumpBundleInfo function
* @tc.type: FUNC
*/
HWTEST_F(DDMSDumpTest, DumpBundleInfo001, TestSize.Level0)
{
    int fd = 0;
    std::map<std::string, std::vector<std::string>> params = {};
    EXPECT_NO_FATAL_FAILURE(dumpTest_.DumpBundleInfo(fd, params));
}

/**
* @tc.name: DumpStoreInfo002
* @tc.desc: test DumpStoreInfo function
* @tc.type: FUNC
*/
HWTEST_F(DDMSDumpTest, DumpStoreInfo002, TestSize.Level0)
{
    int fd = 1;
    std::map<std::string, std::vector<std::string>> params = {};
    ConfigSendParameters(true);
    std::vector<StoreMetaData> metas;
    auto res = MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ TEST_UUID, foregroundUserId_ }),
        metas, true);
    EXPECT_EQ(res, false);
    EXPECT_NO_FATAL_FAILURE(dumpTest_.DumpStoreInfo(fd, params));
}

/**
* @tc.name: DumpBundleInfo002
* @tc.desc: test DumpBundleInfo function
* @tc.type: FUNC
*/
HWTEST_F(DDMSDumpTest, DumpBundleInfo002, TestSize.Level0)
{
    int fd = 1;
    std::map<std::string, std::vector<std::string>> params = {};
    ConfigSendParameters(true);
    std::vector<StoreMetaData> metas;
    auto res = MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ TEST_UUID, foregroundUserId_ }),
        metas, true);
    EXPECT_EQ(res, false);
    EXPECT_NO_FATAL_FAILURE(dumpTest_.DumpBundleInfo(fd, params));
}

/**
* @tc.name: DumpStoreInfo003
* @tc.desc: test DumpStoreInfo function
* @tc.type: FUNC
*/
HWTEST_F(DDMSDumpTest, DumpStoreInfo003, TestSize.Level0)
{
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    int fd = 1;
    std::map<std::string, std::vector<std::string>> params = {};
    ConfigSendParameters(false);
    std::vector<StoreMetaData> metas;
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ TEST_UUID, foregroundUserId_ }), metas,
        true), true);
    EXPECT_NO_FATAL_FAILURE(dumpTest_.DumpStoreInfo(fd, params));
}

/**
* @tc.name: DumpBundleInfo003
* @tc.desc: test DumpBundleInfo function
* @tc.type: FUNC
*/
HWTEST_F(DDMSDumpTest, DumpBundleInfo003, TestSize.Level0)
{
    int fd = 1;
    std::map<std::string, std::vector<std::string>> params = {};
    ConfigSendParameters(false);
    std::vector<StoreMetaData> metas;
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ TEST_UUID, foregroundUserId_ }), metas,
        true), true);
    EXPECT_NO_FATAL_FAILURE(dumpTest_.DumpBundleInfo(fd, params));
}
}