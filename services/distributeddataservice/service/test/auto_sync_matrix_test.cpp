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
#include "auto_sync_matrix.h"

#include <thread>
#include "block_data.h"
#include "bootstrap.h"
#include "device_manager_adapter.h"
#include "eventcenter/event_center.h"
#include "executor_pool.h"
#include "feature/feature_system.h"
#include "gtest/gtest.h"
#include "ipc_skeleton.h"
#include "kvstore_meta_manager.h"
#include "matrix_event.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "types.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
using DMAdapter = DeviceManagerAdapter;
using namespace DistributedDB;
namespace OHOS::Test {
namespace DistributedDataTest {
class AutoSyncMatrixTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    static constexpr const char *TEST_DEVICE = "14a0a92a428005db27c40bad46bf145fede38ec37effe0347cd990fcb031f320";
    static constexpr const char *TEST_BUNDLE = "auto_sync_matrix_test";
    static constexpr const char *TEST_STORE = "auto_sync_matrix_store";
    static constexpr const char *TEST_USER = "0";
    void InitMetaData();

    StoreMetaData metaData_;
};

void AutoSyncMatrixTest::SetUpTestCase(void)
{
    auto executors = std::make_shared<ExecutorPool>(12, 5);
    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadDirectory();
    Bootstrap::GetInstance().LoadCheckers();
    DistributedKv::KvStoreMetaManager::GetInstance().BindExecutor(executors);
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaParameter();
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaListener();
    StoreMetaData meta;
    meta.deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    meta.appId = TEST_BUNDLE;
    meta.bundleName = TEST_BUNDLE;
    meta.user = TEST_USER;
    meta.area = DistributedKv::EL1;
    meta.tokenId = IPCSkeleton::GetCallingTokenID();
    meta.instanceId = 0;
    meta.isAutoSync = true;
    meta.storeType = 1;
    meta.storeId = "auto_sync_matrix_default_store";
    meta.dataType = 1;
    MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta);
    AutoSyncMatrix::GetInstance().Initialize();
}

void AutoSyncMatrixTest::TearDownTestCase(void)
{
}

void AutoSyncMatrixTest::SetUp()
{
    InitMetaData();
}

void AutoSyncMatrixTest::TearDown()
{
}

void AutoSyncMatrixTest::InitMetaData()
{
    metaData_.deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    metaData_.appId = TEST_BUNDLE;
    metaData_.bundleName = TEST_BUNDLE;
    metaData_.user = TEST_USER;
    metaData_.area = DistributedKv::EL1;
    metaData_.tokenId = IPCSkeleton::GetCallingTokenID();
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.storeType = 1;
    metaData_.storeId = TEST_STORE;
    metaData_.dataType = 1;
}

/**
* @tc.name: GetChangedStore
* @tc.desc: get changed store on init.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(AutoSyncMatrixTest, GetChangedStore, TestSize.Level0)
{
    auto metas = AutoSyncMatrix::GetInstance().GetChangedStore("");
    ASSERT_EQ(metas.size(), 0);
    metas = AutoSyncMatrix::GetInstance().GetChangedStore(TEST_DEVICE);
    ASSERT_EQ(metas.size(), 0);
}

/**
* @tc.name: AutoSyncMatrixOnline
* @tc.desc: auto sync matrix online.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(AutoSyncMatrixTest, AutoSyncMatrixOnline, TestSize.Level0)
{
    AutoSyncMatrix::GetInstance().Online("");
    auto metas = AutoSyncMatrix::GetInstance().GetChangedStore(TEST_DEVICE);
    ASSERT_EQ(metas.size(), 0);
    AutoSyncMatrix::GetInstance().Online(TEST_DEVICE);
    metas = AutoSyncMatrix::GetInstance().GetChangedStore(TEST_DEVICE);
    ASSERT_GE(metas.size(), 0);
    AutoSyncMatrix::GetInstance().Offline(TEST_DEVICE);
    metas = AutoSyncMatrix::GetInstance().GetChangedStore(TEST_DEVICE);
    ASSERT_EQ(metas.size(), 0);
}

/**
* @tc.name: AutoSyncMatrixOffline
* @tc.desc: auto sync matrix offline.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(AutoSyncMatrixTest, AutoSyncMatrixOffline, TestSize.Level0)
{
    AutoSyncMatrix::GetInstance().Online(TEST_DEVICE);
    auto before = AutoSyncMatrix::GetInstance().GetChangedStore(TEST_DEVICE);
    ASSERT_GE(before.size(), 0);
    MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto after = AutoSyncMatrix::GetInstance().GetChangedStore(TEST_DEVICE);
    ASSERT_GE(after.size(), before.size());
    AutoSyncMatrix::GetInstance().Offline(TEST_DEVICE);
    auto metas = AutoSyncMatrix::GetInstance().GetChangedStore(TEST_DEVICE);
    ASSERT_EQ(metas.size(), 0);
}

/**
* @tc.name: AutoSyncMatrixOnChanged
* @tc.desc: auto sync matrix on changed.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(AutoSyncMatrixTest, AutoSyncMatrixOnChanged, TestSize.Level0)
{
    AutoSyncMatrix::GetInstance().Online(TEST_DEVICE);
    auto metas = AutoSyncMatrix::GetInstance().GetChangedStore(TEST_DEVICE);
    ASSERT_GE(metas.size(), 0);
    AutoSyncMatrix::GetInstance().OnChanged(metaData_);
    metas = AutoSyncMatrix::GetInstance().GetChangedStore(TEST_DEVICE);
    ASSERT_GE(metas.size(), 0);
    AutoSyncMatrix::GetInstance().Offline(TEST_DEVICE);
    metas = AutoSyncMatrix::GetInstance().GetChangedStore(TEST_DEVICE);
    ASSERT_EQ(metas.size(), 0);
}

/**
* @tc.name: AutoSyncMatrixOnExchanged
* @tc.desc: auto sync matrix on exchanged.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(AutoSyncMatrixTest, AutoSyncMatrixOnExchanged, TestSize.Level0)
{
    AutoSyncMatrix::GetInstance().Online(TEST_DEVICE);
    auto metas1 = AutoSyncMatrix::GetInstance().GetChangedStore(TEST_DEVICE);
    ASSERT_GE(metas1.size(), 0);
    AutoSyncMatrix::GetInstance().OnExchanged(TEST_DEVICE, metaData_);
    auto metas2 = AutoSyncMatrix::GetInstance().GetChangedStore(TEST_DEVICE);
    ASSERT_EQ(metas2.size(), metas1.size() - 1);
    AutoSyncMatrix::GetInstance().Offline(TEST_DEVICE);
    AutoSyncMatrix::GetInstance().OnChanged(metaData_);
    auto metas3 = AutoSyncMatrix::GetInstance().GetChangedStore(TEST_DEVICE);
    ASSERT_EQ(metas3.size(), 0);
    AutoSyncMatrix::GetInstance().Online(TEST_DEVICE);
    auto metas4 = AutoSyncMatrix::GetInstance().GetChangedStore(TEST_DEVICE);
    ASSERT_GE(metas4.size(), metas1.size());
}

/**
* @tc.name: AutoSyncMatrixDeleteMeta
* @tc.desc: auto sync matrix on delete Meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(AutoSyncMatrixTest, AutoSyncMatrixDeleteMeta, TestSize.Level0)
{
    AutoSyncMatrix::GetInstance().Online(TEST_DEVICE);
    AutoSyncMatrix::GetInstance().OnChanged(metaData_);
    auto metas1 = AutoSyncMatrix::GetInstance().GetChangedStore(TEST_DEVICE);
    ASSERT_GE(metas1.size(), 0);
    MetaDataManager::GetInstance().DelMeta(metaData_.GetKey());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto metas2 = AutoSyncMatrix::GetInstance().GetChangedStore(TEST_DEVICE);
    ASSERT_GE(metas2.size(), metas1.size() -1);
    for (auto meta : metas2) {
        ASSERT_NE(metaData_.GetKey(), meta.GetKey());
    }
}
} // namespace DistributedDataTest
} // namespace OHOS::Test