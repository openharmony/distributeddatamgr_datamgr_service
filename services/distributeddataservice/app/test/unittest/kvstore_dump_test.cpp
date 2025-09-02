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

#include "account_delegate_mock.h"
#include "db_store_mock.h"
#include "device_manager_adapter.h"
#include "device_manager_adapter_mock.h"
#include "metadata/store_meta_data.h"
#include "gtest/gtest.h"
#include "kvstore_data_service.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::AppDistributedKv;
using namespace OHOS::DistributedData;

namespace OHOS::Test {
static constexpr const char *TEST_BUNDLE_NAME = "TestApplication";
static constexpr const char *TEST_STORE_NAME = "TestStore";
static constexpr const char *TEST_UUID = "ABCD";
static int32_t userid = 0;

class KvStoreDumpTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static void ConfigSendParameters(bool isCancel);
    static MessageInfo msgInfo_;
    static std::string foregroundUserId_;
    static AppDistributedKv::DeviceInfo localDeviceInfo_;
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static StoreMetaData metaData_;
    static inline AccountDelegateMock *accountDelegateMock = nullptr;
    static inline std::shared_ptr<DeviceManagerAdapterMock> deviceManagerAdapterMock = nullptr;
};

std::string KvStoreDumpTest::foregroundUserId_ = "0";
AppDistributedKv::DeviceInfo KvStoreDumpTest::localDeviceInfo_;
std::shared_ptr<DBStoreMock> KvStoreDumpTest::dbStoreMock_;

void KvStoreDumpTest::SetUpTestCase(void)
{
    dbStoreMock_ = std::make_shared<DBStoreMock>();
    accountDelegateMock = new (std::nothrow) AccountDelegateMock();
    if (accountDelegateMock != nullptr) {
    AccountDelegate::instance_ = nullptr;
    AccountDelegate::RegisterAccountInstance(accountDelegateMock);
    }
    deviceManagerAdapterMock = std::make_shared<DeviceManagerAdapterMock>();
    BDeviceManagerAdapter::deviceManagerAdapter = deviceManagerAdapterMock;
    DeviceInfo deviceInfo;
    deviceInfo.uuid = TEST_UUID;
    EXPECT_CALL(*deviceManagerAdapterMock, GetLocalDevice()).WillRepeatedly(Return(deviceInfo));
}

void KvStoreDumpTest::TearDownTestCase(void)
{
    if (accountDelegateMock != nullptr) {
    delete accountDelegateMock;
    accountDelegateMock = nullptr;
    }
    deviceManagerAdapterMock = nullptr;
    BDeviceManagerAdapter::deviceManagerAdapter = nullptr;
}

void KvStoreDumpTest::SetUp(void)
{}

void KvStoreDumpTest::TearDown(void)
{
    ConfigSendParameters(true);
}

void KvStoreDumpTest::ConfigSendParameters(bool isCancel)
{
    StoreMetaData localMetaData;
    localMetaData.deviceId = TEST_UUID;
    localMetaData.user = foregroundUserId_;
    localMetaData.bundleName = TEST_BUNDLE_NAME;
    localMetaData.storeId = TEST_STORE_NAME;

    if (isCancel) {
        MetaDataManager::GetInstance().DelMeta(localMetaData.GetKeyWithoutPath());
    } else {
        MetaDataManager::GetInstance().SaveMeta(localMetaData.GetKeyWithoutPath(), localMetaData);
    }
}

/**
@tc.name: DumpStoreInfo001
@tc.desc: test DumpStoreInfo function
@tc.type: FUNC
*/
    HWTEST_F(KvStoreDumpTest, DumpStoreInfo001, TestSize.Level0)
{
    DistributedKv::KvStoreDataService KvStoreDumpTest;
    int fd = 0;
    std::map<std::string, std::vector<std::string>> params = {};
    EXPECT_EQ(accountDelegateMock->QueryForegroundUserId(userid), false);
    EXPECT_NO_FATAL_FAILURE(KvStoreDumpTest.DumpStoreInfo(fd, params));
}

/**

@tc.name: DumpBundleInfo001
@tc.desc: test DumpBundleInfo function
@tc.type: FUNC
*/
HWTEST_F(KvStoreDumpTest, DumpBundleInfo001, TestSize.Level0)
{
    DistributedKv::KvStoreDataService KvStoreDumpTest;
    int fd = 0;
    std::map<std::string, std::vector<std::string>> params = {};
    EXPECT_EQ(accountDelegateMock->QueryForegroundUserId(userid), false);
    EXPECT_NO_FATAL_FAILURE(KvStoreDumpTest.DumpBundleInfo(fd, params));
}
/**
@tc.name: DumpStoreInfo002
@tc.desc: test DumpStoreInfo function
@tc.type: FUNC
*/
HWTEST_F(KvStoreDumpTest, DumpStoreInfo002, TestSize.Level0)
{
    DistributedKv::KvStoreDataService KvStoreDumpTest;
    int fd = 1;
    std::map<std::string, std::vector<std::string>> params = {};
    if (accountDelegateMock != nullptr) {
        EXPECT_CALL(*accountDelegateMock, QueryForegroundUserId(userid)).WillOnce(Return(true));
    }
    ConfigSendParameters(true);
    std::vector<StoreMetaData> metas;
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ TEST_UUID, foregroundUserId_ }), metas,
        true), false);
    EXPECT_NO_FATAL_FAILURE(KvStoreDumpTest.DumpStoreInfo(fd, params));
}

/**
@tc.name: DumpBundleInfo002
@tc.desc: test DumpBundleInfo function
@tc.type: FUNC
*/
HWTEST_F(KvStoreDumpTest, DumpBundleInfo002, TestSize.Level0)
{
    DistributedKv::KvStoreDataService KvStoreDumpTest;
    int fd = 1;
    std::map<std::string, std::vector<std::string>> params = {};
    if (accountDelegateMock != nullptr) {
        EXPECT_CALL(*accountDelegateMock, QueryForegroundUserId(_)).WillOnce(Return(true));
    }
    ConfigSendParameters(true);
    std::vector<StoreMetaData> metas;
    auto res = MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ TEST_UUID, foregroundUserId_ }),
        metas, true);
    EXPECT_EQ(res, false);
    EXPECT_NO_FATAL_FAILURE(KvStoreDumpTest.DumpStoreInfo(fd, params));
}

/**
@tc.name: DumpStoreInfo003
@tc.desc: test DumpStoreInfo function
@tc.type: FUNC
*/
HWTEST_F(KvStoreDumpTest, DumpStoreInfo003, TestSize.Level0)
{
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    DistributedKv::KvStoreDataService KvStoreDumpTest;
    int fd = 1;
    std::map<std::string, std::vector<std::string>> params = {};
    if (accountDelegateMock != nullptr) {
        EXPECT_CALL(*accountDelegateMock, QueryForegroundUserId(_)).WillOnce(Return(true));
    }
    ConfigSendParameters(false);
    std::vector<StoreMetaData> metas;
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ TEST_UUID, foregroundUserId_ }), metas,
        true), true);
    EXPECT_NO_FATAL_FAILURE(KvStoreDumpTest.DumpStoreInfo(fd, params));
}

/**
@tc.name: DumpBundleInfo003
@tc.desc: test DumpBundleInfo function
@tc.type: FUNC
*/
HWTEST_F(KvStoreDumpTest, DumpBundleInfo003, TestSize.Level0)
{
    DistributedKv::KvStoreDataService KvStoreDumpTest;
    int fd = 1;
    std::map<std::string, std::vector<std::string>> params = {};
    if (accountDelegateMock != nullptr) {
    EXPECT_CALL(*accountDelegateMock, QueryForegroundUserId(_)).WillOnce(Return(true));
    }
    ConfigSendParameters(false);
    std::vector<StoreMetaData> metas;
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ TEST_UUID, foregroundUserId_ }), metas,
        true), true);
    EXPECT_NO_FATAL_FAILURE(KvStoreDumpTest.DumpStoreInfo(fd, params));
}
}