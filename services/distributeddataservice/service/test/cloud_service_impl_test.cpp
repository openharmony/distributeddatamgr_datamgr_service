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
#define LOG_TAG "CloudServiceImplTest"
#include "cloud_service_impl.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "bootstrap.h"
#include "checker_mock.h"
#include "cloud/change_event.h"
#include "cloud/cloud_event.h"
#include "cloud/cloud_server.h"
#include "cloud/cloud_share_event.h"
#include "cloud/schema_meta.h"
#include "cloud_types.h"
#include "cloud_types_util.h"
#include "cloud_value_util.h"
#include "communicator/device_manager_adapter.h"
#include "device_matrix.h"
#include "eventcenter/event_center.h"
#include "feature/feature_system.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "mock/account_delegate_mock.h"
#include "mock/db_store_mock.h"
#include "mock/general_store_mock.h"
#include "mock/meta_data_manager_mock.h"
#include "model/component_config.h"
#include "network/network_delegate.h"
#include "network_delegate_mock.h"
#include "rdb_query.h"
#include "rdb_service.h"
#include "rdb_service_impl.h"
#include "rdb_types.h"
#include "store/auto_cache.h"
#include "store/store_info.h"
#include "token_setproc.h"
#include "directory/directory_manager.h"

using namespace testing::ext;
using namespace testing;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
using Confirmation = OHOS::CloudData::Confirmation;
using Status = OHOS::CloudData::CloudService::Status;
using CloudSyncScene = OHOS::CloudData::CloudServiceImpl::CloudSyncScene;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;

namespace OHOS::Test {
namespace DistributedDataTest {
static constexpr const char *TEST_CLOUD_BUNDLE = "test_cloud_bundleName";
static constexpr const char *TEST_CLOUD_APPID = "test_cloud_appid";
static constexpr const char *TEST_CLOUD_STORE = "test_cloud_store";
static constexpr const char *TEST_CLOUD_DATABASE_ALIAS_1 = "test_cloud_database_alias_1";
constexpr const int32_t DISCONNECT_TIME = 21;
constexpr const int32_t MOCK_USER = 200;
class CloudServiceImplTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static void InitMetaData();
    static void CheckDelMeta(StoreMetaMapping &metaMapping, StoreMetaData &meta, StoreMetaData &meta1);
    static std::shared_ptr<CloudData::CloudServiceImpl> cloudServiceImpl_;
    static NetworkDelegateMock delegate_;
    static auto ReturnWithUserList(const std::vector<int>& users);

protected:
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static StoreMetaData metaData_;
    static inline AccountDelegateMock *accountDelegateMock = nullptr;
};
std::shared_ptr<CloudData::CloudServiceImpl> CloudServiceImplTest::cloudServiceImpl_ =
    std::make_shared<CloudData::CloudServiceImpl>();
NetworkDelegateMock CloudServiceImplTest::delegate_;
std::shared_ptr<DBStoreMock> CloudServiceImplTest::dbStoreMock_ = std::make_shared<DBStoreMock>();
StoreMetaData CloudServiceImplTest::metaData_;

void CloudServiceImplTest::CheckDelMeta(StoreMetaMapping &metaMapping, StoreMetaData &meta, StoreMetaData &meta1)
{
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta1.GetKey(), true), true);
    metaMapping.cloudPath = "";
    metaMapping.dataDir = DirectoryManager::GetInstance().GetStorePath(metaMapping) + "/" + metaMapping.storeId;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta1.GetKey(), meta1, true), true);
    meta.user = "100";
    auto res = cloudServiceImpl_->GetStoreMetaData(meta);
    EXPECT_EQ(res, false);
 
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta1.GetKey(), true), true);
    metaMapping.dataDir = "";
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta1.GetKey(), meta1, true), true);
    meta.user = "100";
    res = cloudServiceImpl_->GetStoreMetaData(meta);
    EXPECT_EQ(res, false);
}

void CloudServiceImplTest::InitMetaData()
{
    metaData_.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData_.appId = "test_rdb_cloud_service_impl_appid";
    metaData_.bundleName = "test_rdb_cloud_service_impl_bundleName";
    metaData_.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    metaData_.user = std::to_string(AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId));
    metaData_.area = OHOS::DistributedKv::EL1;
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.storeType = DistributedRdb::RDB_DEVICE_COLLABORATION;
    metaData_.storeId = "test_cloud_service_impl_store";
    metaData_.dataDir = "/test_cloud_service_impl_store";
}

auto CloudServiceImplTest::ReturnWithUserList(const std::vector<int> &users)
{
    return Invoke([=](std::vector<int> &outUsers) -> bool {
        outUsers = users;
        return true;
    });
}

void CloudServiceImplTest::SetUpTestCase(void)
{
    size_t max = 12;
    size_t min = 5;
    auto executor = std::make_shared<ExecutorPool>(max, min);
    cloudServiceImpl_->OnBind(
        { "CloudServiceImplTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    Bootstrap::GetInstance().LoadCheckers();
    CryptoManager::GetInstance().GenerateRootKey();
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    NetworkDelegate::RegisterNetworkInstance(&delegate_);
    InitMetaData();
}

void CloudServiceImplTest::TearDownTestCase() { }

void CloudServiceImplTest::SetUp() { }

void CloudServiceImplTest::TearDown() { }

/**
 * @tc.name: EnableCloud001
 * @tc.desc: Test EnableCloud functions with user is invalid.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, EnableCloud001, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest EnableCloud001 start");
    std::map<std::string, int32_t> switches;
    switches.insert_or_assign(TEST_CLOUD_BUNDLE, CloudData::CloudService::SWITCH_ON);
    auto status = cloudServiceImpl_->EnableCloud(TEST_CLOUD_APPID, switches);
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test