/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "rdb_service_impl.h"

#include <gtest/gtest.h>

#include "bootstrap.h"
#include "metadata/meta_data_manager.h"
#include "mock/db_store_mock.h"
#include "mock/device_manager_adapter_mock.h"
#include "mock/general_store_mock.h"
#include "rdb_types.h"
#include "store/auto_cache.h"

using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
using namespace DistributedDB;
using namespace testing;
using namespace testing::ext;

namespace OHOS::Test {
namespace DistributedRDBTest {

static constexpr const char *TEST_BUNDLE = "test_rdb_service_impl_bundleName";

class RdbServiceImplReplicaPathTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        deviceManagerAdapterMock_ = std::make_shared<DeviceManagerAdapterMock>();
        previousDeviceManagerAdapter_ = BDeviceManagerAdapter::deviceManagerAdapter;
        BDeviceManagerAdapter::deviceManagerAdapter = deviceManagerAdapterMock_;
        DeviceInfo deviceInfo;
        deviceInfo.uuid = "ABCD";
        EXPECT_CALL(*deviceManagerAdapterMock_, GetLocalDevice()).WillRepeatedly(Return(deviceInfo));
        Bootstrap::GetInstance().LoadCheckers();
        dbStoreMock_ = std::make_shared<DBStoreMock>();
        MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    }

    static void TearDownTestCase()
    {
        BDeviceManagerAdapter::deviceManagerAdapter = previousDeviceManagerAdapter_;
        deviceManagerAdapterMock_ = nullptr;
        previousDeviceManagerAdapter_ = nullptr;
        dbStoreMock_ = nullptr;
    }

private:
    static inline std::shared_ptr<DeviceManagerAdapterMock> deviceManagerAdapterMock_ = nullptr;
    static inline std::shared_ptr<BDeviceManagerAdapter> previousDeviceManagerAdapter_ = nullptr;
    static inline std::shared_ptr<DBStoreMock> dbStoreMock_ = nullptr;
};

/**
 * @tc.name: GetStoreMetaDataWithReplicaPath
 * @tc.desc: Test that GetStoreMetaData preserves the custom replica path
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(RdbServiceImplReplicaPathTest, GetStoreMetaDataWithReplicaPath, TestSize.Level1)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.storeName_ = "replica_path_test";
    param.bundleName_ = TEST_BUNDLE;
    param.user_ = "100";
    param.hapName_ = "test_hap";
    param.replicaPath_ = "/data/service/el1/public/database/replica";

    auto metaData = service.GetStoreMetaData(param);

    EXPECT_EQ(metaData.replicaPath, param.replicaPath_);
}

/**
 * @tc.name: AfterOpenReplicaPathChangeInvalidatesCache
 * @tc.desc: AfterOpen invalidates the cached store when the persisted replica path changes
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(RdbServiceImplReplicaPathTest, AfterOpenReplicaPathChangeInvalidatesCache, TestSize.Level1)
{
    constexpr int32_t MOCK_STORE_TYPE = 8;
    auto regResult = AutoCache::GetInstance().RegCreator(MOCK_STORE_TYPE,
        [](const StoreMetaData &metaData, const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
            auto *store = new (std::nothrow) GeneralStoreMock();
            if (store == nullptr) {
                return { GeneralError::E_ERROR, nullptr };
            }
            return { GeneralError::E_OK, store };
        });
    ASSERT_EQ(regResult, E_OK);

    RdbServiceImpl service;
    RdbSyncerParam param;
    param.storeName_ = "replica_path_cache_test";
    param.bundleName_ = TEST_BUNDLE;
    param.user_ = "100";
    param.hapName_ = "test_hap";
    param.type_ = MOCK_STORE_TYPE;

    auto old = service.GetStoreMetaData(param);
    old.replicaPath = "/data/service/el1/public/database/replica";
    ASSERT_TRUE(MetaDataManager::GetInstance().SaveMeta(old.GetKey(), old, true));

    auto store = service.GetStore(old);
    ASSERT_NE(store, nullptr);
    EXPECT_FALSE(AutoCache::GetInstance().GetStoresIfPresent(old.tokenId, old.dataDir, old.storeId).empty());

    // An empty path differs from the saved one and skips the token-type checks in IsValidReplicaPath.
    param.replicaPath_ = "";
    EXPECT_EQ(service.AfterOpen(param), RDB_OK);
    EXPECT_TRUE(AutoCache::GetInstance().GetStoresIfPresent(old.tokenId, old.dataDir, old.storeId).empty());

    MetaDataManager::GetInstance().DelMeta(old.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(old.GetKeyWithoutPath(), true);
}

} // namespace DistributedRDBTest
} // namespace OHOS::Test
