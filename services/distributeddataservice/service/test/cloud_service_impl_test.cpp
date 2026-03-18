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
#include "account_delegate_mock.h"
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
using DBSwitchInfo = OHOS::CloudData::DBSwitchInfo;
using SwitchConfig = OHOS::CloudData::SwitchConfig;
using DBActionInfo = OHOS::CloudData::DBActionInfo;
using ClearConfig = OHOS::CloudData::ClearConfig;

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
    static auto ReturnWithUserList(const std::vector<int> &users);
    static void InitCloudInfoAndSchema();

protected:
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static StoreMetaData metaData_;
    static inline AccountDelegateMock *accountDelegateMock = nullptr;

    class SyncInfoObserverMock : public CloudData::ISyncInfoObserver {
    public:
        void OnSyncInfoChanged(const std::map<std::string, CloudData::QueryLastResults> &data) override
        {
            lastData_ = data;
        }

        const std::map<std::string, CloudData::QueryLastResults> &GetLastData() const
        {
            return lastData_;
        }

    private:
        std::map<std::string, CloudData::QueryLastResults> lastData_;
    };
};
std::shared_ptr<CloudData::CloudServiceImpl> CloudServiceImplTest::cloudServiceImpl_ =
    std::make_shared<CloudData::CloudServiceImpl>();
NetworkDelegateMock CloudServiceImplTest::delegate_;
std::shared_ptr<DBStoreMock> CloudServiceImplTest::dbStoreMock_ = std::make_shared<DBStoreMock>();
StoreMetaData CloudServiceImplTest::metaData_;

void CloudServiceImplTest::InitCloudInfoAndSchema()
{
    CloudInfo::AppInfo appInfo;
    appInfo.bundleName = TEST_CLOUD_BUNDLE;
    appInfo.cloudSwitch = true;
    std::map<std::string, CloudInfo::AppInfo> apps;
    apps.emplace(TEST_CLOUD_BUNDLE, appInfo);
    CloudInfo cloudInfo;
    cloudInfo.apps = apps;
    cloudInfo.user = AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId);
    cloudInfo.enableCloud = true;
    cloudInfo.id = TEST_CLOUD_APPID;
    MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true);

    SchemaMeta::Field field1;
    field1.colName = "test_cloud_field_name1";
    field1.alias = "test_cloud_field_alias1";
    SchemaMeta::Field field2;
    field2.colName = "test_cloud_field_name2";
    field2.alias = "test_cloud_field_alias2";

    SchemaMeta::Table table;
    table.name = "test_cloud_table_name";
    table.alias = "test_cloud_table_alias";
    table.fields.emplace_back(field1);
    table.fields.emplace_back(field2);

    SchemaMeta::Database database;
    database.name = TEST_CLOUD_STORE;
    database.alias = TEST_CLOUD_STORE;
    database.tables.emplace_back(table);
    table.name = "test_cloud_table_name1";
    database.tables.emplace_back(table);
    table.name = "test_cloud_table_name2";
    database.tables.emplace_back(table);
    SchemaMeta schemaMeta;
    schemaMeta.databases.emplace_back(database);
    database.name = "TEST_CLOUD_STORE2";
    schemaMeta.databases.emplace_back(database);
    schemaMeta.bundleName = TEST_CLOUD_BUNDLE;
    MetaDataManager::GetInstance().SaveMeta(CloudInfo::GetSchemaKey(cloudInfo.user, TEST_CLOUD_BUNDLE), schemaMeta,
        true);
    StoreMetaData meta;
    meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    meta.user = "0";
    meta.bundleName = TEST_CLOUD_BUNDLE;
    meta.storeId = TEST_CLOUD_STORE;
    meta.dataDir = DirectoryManager::GetInstance().GetStorePath(meta) + "/" + meta.storeId;
    MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true);
    StoreMetaMapping metaMapping(meta);
    MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true);
    StoreMetaDataLocal metaLocal;
    metaLocal.isPublic = true;
    MetaDataManager::GetInstance().SaveMeta(meta.GetKeyLocal(), metaLocal, true);
}

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

void CloudServiceImplTest::TearDownTestCase()
{
    if (accountDelegateMock != nullptr) {
        delete accountDelegateMock;
        accountDelegateMock = nullptr;
    }
}

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

/**
 * @tc.name: DisableCloud001
 * @tc.desc: Test DisableCloud functions with user is invalid.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, DisableCloud001, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest DisableCloud001 start");
    auto status = cloudServiceImpl_->DisableCloud(TEST_CLOUD_APPID);
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
}

/**
 * @tc.name: ChangeAppSwitch001
 * @tc.desc: Test ChangeAppSwitch functions with user is invalid.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, ChangeAppSwitch001, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest ChangeAppSwitch001 start");
    OHOS::CloudData::SwitchConfig config;
    auto status = cloudServiceImpl_->ChangeAppSwitch(TEST_CLOUD_APPID, TEST_CLOUD_BUNDLE,
        CloudData::CloudService::SWITCH_ON, config);
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
}

/**
 * @tc.name: Clean001
 * @tc.desc: Test Clean functions with user is invalid.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, Clean001, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest Clean001 start");
    std::map<std::string, int32_t> actions;
    actions.insert_or_assign(TEST_CLOUD_BUNDLE, CloudData::CloudService::SWITCH_ON);
    std::map<std::string, OHOS::CloudData::ClearConfig> configs;
    auto status = cloudServiceImpl_->Clean(TEST_CLOUD_APPID, actions, configs);
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
}

/**
 * @tc.name: NotifyDataChange001
 * @tc.desc: Test the EnableCloud function in case it doesn't get cloudInfo.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, NotifyDataChange001, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest NotifyDataChange001 start");
    auto status = cloudServiceImpl_->NotifyDataChange(TEST_CLOUD_APPID, TEST_CLOUD_BUNDLE);
    EXPECT_EQ(status, CloudData::CloudService::INVALID_ARGUMENT);
}

/**
 * @tc.name: ExecuteStatistics001
 * @tc.desc: Test the ExecuteStatistics function if the package name does not support CloudSync.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, ExecuteStatistics001, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest ExecuteStatistics001 start");
    CloudInfo cloudInfo;
    CloudInfo::AppInfo appInfo;
    appInfo.bundleName = TEST_CLOUD_BUNDLE;
    cloudInfo.apps[TEST_CLOUD_BUNDLE] = std::move(appInfo);
    SchemaMeta::Database database;
    database.name = TEST_CLOUD_STORE;
    database.alias = TEST_CLOUD_DATABASE_ALIAS_1;
    SchemaMeta schemaMeta;
    schemaMeta.bundleName = "testBundle";
    schemaMeta.databases.emplace_back(database);
    auto result = cloudServiceImpl_->ExecuteStatistics("", cloudInfo, schemaMeta);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: QueryStatistics001
 * @tc.desc: When metadata is not supported store test the QueryStatistics function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryStatistics001, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest QueryStatistics start");
    StoreMetaData metaData;
    metaData.storeType = -1;
    DistributedData::Database database;
    auto result = cloudServiceImpl_->QueryStatistics(metaData, database);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: QueryLastSyncInfo001
 * @tc.desc: Test QueryLastSyncInfo functions with invalid parameter.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryLastSyncInfo001, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest QueryLastSyncInfo start");
    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfo(TEST_CLOUD_APPID, TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE);
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: OnBind001
 * @tc.desc: Test OnBind functions with invalid parameter.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, OnBind001, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest OnBind001 start");
    auto status = cloudServiceImpl_->OnBind({});
    EXPECT_EQ(status, GeneralError::E_INVALID_ARGS);
}

/**
 * @tc.name: UpdateSchema001
 * @tc.desc: Test UpdateSchema001 functions with invalid parameter.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, UpdateSchema001, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest UpdateSchema001 start");
    int user = -1;
    auto status = cloudServiceImpl_->UpdateSchema(user, CloudSyncScene::ENABLE_CLOUD);
    EXPECT_FALSE(status);
}

/**
 * @tc.name: GetAppSchemaFromServer001
 * @tc.desc: Test GetAppSchemaFromServer functions not support CloudService.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, GetAppSchemaFromServer001, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest GetAppSchemaFromServer001 start");
    int user = -1;
    auto [status, result] = cloudServiceImpl_->GetAppSchemaFromServer(user, TEST_CLOUD_BUNDLE);
    EXPECT_EQ(status, CloudData::CloudService::SERVER_UNAVAILABLE);
}

/**
 * @tc.name: GetCloudInfoFromServer001
 * @tc.desc: Test GetCloudInfoFromServer functions not support CloudService.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, GetCloudInfoFromServer001, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest GetCloudInfoFromServer001 start");
    int user = -1;
    auto [status, result] = cloudServiceImpl_->GetCloudInfoFromServer(user);
    EXPECT_EQ(status, CloudData::CloudService::SERVER_UNAVAILABLE);
}

/**
 * @tc.name: GetCloudInfoFromServer002
 * @tc.desc: Test GetCloudInfoFromServer functions with cloudinfo is empty.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, GetCloudInfoFromServer002, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest GetCloudInfoFromServer002 start");
    CloudServer cloudServer;
    CloudServer::RegisterCloudInstance(&cloudServer);
    int user = 100;
    auto [status, result] = cloudServiceImpl_->GetCloudInfoFromServer(user);
    EXPECT_EQ(status, E_ERROR);
    CloudServer::instance_ = nullptr;
}

/**
 * @tc.name: ReleaseUserInfo001
 * @tc.desc: Test ReleaseUserInfo functions with invalid parameter.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, ReleaseUserInfo001, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest ReleaseUserInfo001 start");
    int user = 100;
    auto status = cloudServiceImpl_->ReleaseUserInfo(user, CloudSyncScene::ENABLE_CLOUD);
    EXPECT_TRUE(status);
}

/**
 * @tc.name: DoSubscribe
 * @tc.desc: Test the DoSubscribe with not support CloudService
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, DoSubscribe, TestSize.Level0)
{
    int32_t user = 100;
    auto status = cloudServiceImpl_->DoSubscribe(user, CloudSyncScene::ENABLE_CLOUD);
    EXPECT_TRUE(status);
}

/**
 * @tc.name: OnAppInstallTest
 * @tc.desc: Test the OnAppInstallTest
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, OnAppInstallTest, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest OnAppInstallTest start.");
    ASSERT_NE(cloudServiceImpl_, nullptr);
    ASSERT_NE(cloudServiceImpl_->factory_.staticActs_, nullptr);
    int32_t user = 0;
    int32_t index = 0;
    auto status = cloudServiceImpl_->factory_.staticActs_->OnAppInstall(TEST_CLOUD_BUNDLE, user, index);
    EXPECT_EQ(status, GeneralError::E_ERROR);
}

/**
 * @tc.name: OnAppUpdateTest
 * @tc.desc: Test the OnAppUpdateTest
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, OnAppUpdateTest, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest OnAppUpdateTest start");
    ASSERT_NE(cloudServiceImpl_, nullptr);
    ASSERT_NE(cloudServiceImpl_->factory_.staticActs_, nullptr);
    int32_t user = 0;
    int32_t index = 0;
    auto status = cloudServiceImpl_->factory_.staticActs_->OnAppUpdate(TEST_CLOUD_BUNDLE, user, index);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
}

/**
 * @tc.name: CloudDriverCheckTest
 * @tc.desc: Test the CloudDriverCheck
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, CloudDriverCheckTest, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest CloudDriverCheckTest start");
    ASSERT_NE(cloudServiceImpl_, nullptr);
    ASSERT_NE(cloudServiceImpl_->factory_.staticActs_, nullptr);
    CloudServer cloudServer;
    CloudServer::RegisterCloudInstance(&cloudServer);
    int32_t user = 0;
    auto result = cloudServiceImpl_->factory_.staticActs_->CloudDriverCheck(TEST_CLOUD_BUNDLE, user);
    EXPECT_EQ(result, false);
    CloudServer::instance_ = nullptr;
}

/**
 * @tc.name: UpdateSchemaFromServerTest_001
 * @tc.desc: Test UpdateSchemaFromServer functions
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, UpdateSchemaFromServerTest_001, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest UpdateSchemaFromServerTest_001 start");
    CloudServer cloudServer;
    CloudServer::RegisterCloudInstance(&cloudServer);
    int user = 100;
    auto status = cloudServiceImpl_->UpdateSchemaFromServer(user);
    EXPECT_EQ(status, E_ERROR);
    CloudServer::instance_ = nullptr;
}

/**
 * @tc.name: UpdateSchemaFromServerTest_002
 * @tc.desc: Test UpdateSchemaFromServer functions.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, UpdateSchemaFromServerTest_002, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest UpdateSchemaFromServerTest_002 start");
    CloudServer cloudServer;
    CloudServer::RegisterCloudInstance(&cloudServer);
    int user = 0;
    CloudInfo cloudInfo;
    cloudInfo.user = user;
    DistributedData::CloudInfo::AppInfo appInfo;
    appInfo.bundleName = TEST_CLOUD_BUNDLE;
    appInfo.appId = TEST_CLOUD_APPID;
    appInfo.cloudSwitch = true;
    cloudInfo.apps = {{ TEST_CLOUD_BUNDLE, appInfo }};
    auto status = cloudServiceImpl_->UpdateSchemaFromServer(cloudInfo, user);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    CloudServer::instance_ = nullptr;
}

/**
 * @tc.name: UpdateE2eeEnableTest
 * @tc.desc: Test UpdateE2eeEnable functions.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, UpdateE2eeEnableTest, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest UpdateE2eeEnableTest start");
    ASSERT_NE(cloudServiceImpl_, nullptr);
    CloudServer cloudServer;
    auto result = cloudServer.CloudDriverUpdated(TEST_CLOUD_BUNDLE);
    EXPECT_EQ(result, false);

    std::string schemaKey = "schemaKey";
    cloudServiceImpl_->UpdateE2eeEnable(schemaKey, true, TEST_CLOUD_BUNDLE);
    SchemaMeta schemaMeta;
    ASSERT_FALSE(MetaDataManager::GetInstance().LoadMeta(schemaKey, schemaMeta, true));
}

/**
 * @tc.name: Share001
 * @tc.desc: Test the Share with invalid parameters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, Share001, TestSize.Level0)
{
    std::string sharingRes;
    CloudData::Participants participants;
    CloudData::Results results;
    auto status = cloudServiceImpl_->Share(sharingRes, participants, results);
    EXPECT_EQ(status, GeneralError::E_ERROR);
}

/**
 * @tc.name: Unshare001
 * @tc.desc: Test the Unshare with invalid parameters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, Unshare001, TestSize.Level0)
{
    std::string sharingRes;
    CloudData::Participants participants;
    CloudData::Results results;
    auto status = cloudServiceImpl_->Unshare(sharingRes, participants, results);
    EXPECT_EQ(status, GeneralError::E_ERROR);
}

/**
 * @tc.name: Exit001
 * @tc.desc: Test the Exit with invalid parameters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, Exit001, TestSize.Level0)
{
    std::string sharingRes;
    std::pair<int32_t, std::string> result;
    auto status = cloudServiceImpl_->Exit(sharingRes, result);
    EXPECT_EQ(status, GeneralError::E_ERROR);
}

/**
 * @tc.name: ChangePrivilege001
 * @tc.desc: Test the ChangePrivilege with invalid parameters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, ChangePrivilege001, TestSize.Level0)
{
    std::string sharingRes;
    CloudData::Participants participants;
    CloudData::Results results;
    auto status = cloudServiceImpl_->ChangePrivilege(sharingRes, participants, results);
    EXPECT_EQ(status, GeneralError::E_ERROR);
}

/**
 * @tc.name: Query001
 * @tc.desc: Test the Query with invalid parameters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, Query001, TestSize.Level0)
{
    std::string sharingRes;
    CloudData::QueryResults results;
    auto status = cloudServiceImpl_->Query(sharingRes, results);
    EXPECT_EQ(status, GeneralError::E_ERROR);
}

/**
 * @tc.name: QueryByInvitation001
 * @tc.desc: Test the QueryByInvitation with invalid parameters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryByInvitation001, TestSize.Level0)
{
    std::string invitation;
    CloudData::QueryResults results;
    auto status = cloudServiceImpl_->QueryByInvitation(invitation, results);
    EXPECT_EQ(status, GeneralError::E_ERROR);
}

/**
 * @tc.name: ConfirmInvitation001
 * @tc.desc: Test the ConfirmInvitation with invalid parameters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, ConfirmInvitation001, TestSize.Level0)
{
    int32_t confirmation = 0;
    std::tuple<int32_t, std::string, std::string> result { 0, "", "" };
    std::string invitation;
    auto status = cloudServiceImpl_->ConfirmInvitation(invitation, confirmation, result);
    EXPECT_EQ(status, GeneralError::E_ERROR);
}

/**
 * @tc.name: GetStoreMetaData_001
 * @tc.desc: test GetStoreMetaData LoadMeta success.
 * @tc.type: FUNC
 */
HWTEST_F(CloudServiceImplTest, GetStoreMetaData_001, TestSize.Level1)
{
    StoreMetaData meta;
    meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    meta.user = "0";
    meta.bundleName = "bundleName";
    meta.storeId = "storeName";
    meta.dataDir = DirectoryManager::GetInstance().GetStorePath(meta) + "/" + meta.storeId;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    bool res = cloudServiceImpl_->GetStoreMetaData(meta);
    EXPECT_EQ(res, true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
    meta.instanceId = 1;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    res = cloudServiceImpl_->GetStoreMetaData(meta);
    EXPECT_EQ(res, true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: GetStoreMetaData_002
 * @tc.desc: test GetStoreMetaData LoadMeta failed and user is 0.
 * @tc.type: FUNC
 */
HWTEST_F(CloudServiceImplTest, GetStoreMetaData_002, TestSize.Level1)
{
    StoreMetaMapping metaMapping;
    metaMapping.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaMapping.user = "0";
    metaMapping.bundleName = "bundleName";
    metaMapping.storeId = "storeName";
    metaMapping.cloudPath = DirectoryManager::GetInstance().GetStorePath(metaMapping) + "/" + metaMapping.storeId;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);

    StoreMetaData meta;
    meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    meta.user = "100";
    meta.bundleName = "bundleName";
    meta.storeId = "storeName";
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    bool res = cloudServiceImpl_->GetStoreMetaData(meta);
    EXPECT_EQ(res, false);

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    meta.instanceId = 1;
    metaMapping.instanceId = 1;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);
    meta.user = "100";
    res = cloudServiceImpl_->GetStoreMetaData(meta);
    EXPECT_EQ(res, false);

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    metaMapping.cloudPath = "";
    metaMapping.dataDir = DirectoryManager::GetInstance().GetStorePath(metaMapping) + "/" + metaMapping.storeId;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);
    meta.user = "100";
    res = cloudServiceImpl_->GetStoreMetaData(meta);
    EXPECT_EQ(res, false);

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    metaMapping.dataDir = "";
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    meta.user = "100";
    res = cloudServiceImpl_->GetStoreMetaData(meta);
    EXPECT_EQ(res, false);

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: GetStoreMetaData_003
 * @tc.desc: test GetStoreMetaData First load of metadata failed and the user is not 0,
 * then the load of metadata succeeded.
 * @tc.type: FUNC
 */
HWTEST_F(CloudServiceImplTest, GetStoreMetaData_003, TestSize.Level1)
{
    StoreMetaMapping metaMapping;
    metaMapping.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaMapping.user = "0";
    metaMapping.bundleName = "bundleName";
    metaMapping.storeId = "storeName";
    metaMapping.cloudPath = DirectoryManager::GetInstance().GetStorePath(metaMapping) + "/" + metaMapping.storeId;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);
    StoreMetaData meta;
    meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    meta.user = "100";
    meta.bundleName = "bundleName";
    meta.storeId = "storeName";
    StoreMetaData meta1;
    meta1.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    meta1.user = "0";
    meta1.bundleName = "bundleName";
    meta1.storeId = "storeName";
    meta1.dataDir = DirectoryManager::GetInstance().GetStorePath(meta) + "/" + meta.storeId;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta1.GetKey(), meta1, true), true);
    StoreMetaDataLocal localMetaData;
    bool res = cloudServiceImpl_->GetStoreMetaData(meta);
    EXPECT_EQ(res, false);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta1.GetKey(), true), true);
}

/**
 * @tc.name: GetStoreMetaData_004
 * @tc.desc: test GetStoreMetaData First load of metadata failed and the user is not 0,
 * then the load of metadata succeeded.
 * @tc.type: FUNC
 */
HWTEST_F(CloudServiceImplTest, GetStoreMetaData_004, TestSize.Level1)
{
    StoreMetaMapping metaMapping;
    metaMapping.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaMapping.user = "0";
    metaMapping.bundleName = "bundleName";
    metaMapping.storeId = "storeName";
    metaMapping.instanceId = 1;
    metaMapping.cloudPath = DirectoryManager::GetInstance().GetStorePath(metaMapping) + "/" + metaMapping.storeId;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);
    StoreMetaData meta;
    meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    meta.user = "100";
    meta.bundleName = "bundleName";
    meta.storeId = "storeName";
    meta.instanceId = 1;
    StoreMetaData meta1;
    meta1.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    meta1.user = "0";
    meta1.bundleName = "bundleName";
    meta1.storeId = "storeName";
    meta1.instanceId = 1;
    meta1.dataDir = DirectoryManager::GetInstance().GetStorePath(meta1) + "/" + meta1.storeId;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta1.GetKey(), meta1, true), true);
    StoreMetaDataLocal localMetaData;
    bool res = cloudServiceImpl_->GetStoreMetaData(meta);
    EXPECT_EQ(res, false);
    CheckDelMeta(metaMapping, meta, meta1);
 
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta1.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta1.GetKey(), meta1, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta1.GetKeyLocal(), localMetaData, true), true);
    meta.user = "100";
    res = cloudServiceImpl_->GetStoreMetaData(meta);
    EXPECT_EQ(res, false);

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta1.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta1.GetKeyLocal(), true), true);
    localMetaData.isPublic = true;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta1.GetKey(), meta1, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta1.GetKeyLocal(), localMetaData, true), true);
    meta.user = "100";
    metaMapping.cloudPath = DirectoryManager::GetInstance().GetStorePath(meta1) + "/" + meta1.storeId;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);
    res = cloudServiceImpl_->GetStoreMetaData(meta);
    EXPECT_EQ(res, true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta1.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta1.GetKeyLocal(), true), true);
}

/**
 * @tc.name: PreShare_001
 * @tc.desc: test PreShare is dataDir empty.
 * @tc.type: FUNC
 */
HWTEST_F(CloudServiceImplTest, PreShare_001, TestSize.Level1)
{
    StoreInfo storeInfo;
    storeInfo.bundleName = "bundleName";
    storeInfo.storeName = "storeName";
    storeInfo.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    storeInfo.user = 100;
    StoreMetaMapping storeMetaMapping;
    storeMetaMapping.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    storeMetaMapping.user = "100";
    storeMetaMapping.bundleName = "bundleName";
    storeMetaMapping.storeId = "storeName";
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(storeMetaMapping.GetKey(), storeMetaMapping, true), true);
    std::shared_ptr<GenQuery> query;
    auto [errCode, cursor] = cloudServiceImpl_->PreShare(storeInfo, *query);
    EXPECT_EQ(errCode, E_ERROR);
    ASSERT_EQ(cursor, nullptr);
    storeInfo.path = "path";
    storeMetaMapping.cloudPath = "path1";
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(storeMetaMapping.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(storeMetaMapping.GetKey(), storeMetaMapping, true), true);
    auto [errCode1, cursor1] = cloudServiceImpl_->PreShare(storeInfo, *query);
    EXPECT_EQ(errCode1, E_ERROR);
    ASSERT_EQ(cursor1, nullptr);
    storeInfo.path = "path";
    storeMetaMapping.cloudPath = "path";
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(storeMetaMapping.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(storeMetaMapping.GetKey(), storeMetaMapping, true), true);
    auto [errCode2, cursor2] = cloudServiceImpl_->PreShare(storeInfo, *query);
    EXPECT_EQ(errCode2, E_ERROR);
    ASSERT_EQ(cursor2, nullptr);
    storeInfo.instanceId = 1;
    storeMetaMapping.instanceId = 1;
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(storeMetaMapping.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(storeMetaMapping.GetKey(), storeMetaMapping, true), true);
    auto [errCode3, cursor3] = cloudServiceImpl_->PreShare(storeInfo, *query);
    EXPECT_EQ(errCode3, E_ERROR);
    ASSERT_EQ(cursor3, nullptr);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(storeMetaMapping.GetKey(), true), true);
}

/**
 * @tc.name: ChangeConfirmation001
 * @tc.desc: Test the ChangeConfirmation with invalid parameters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, ChangeConfirmation001, TestSize.Level0)
{
    int32_t confirmation = 0;
    std::string sharingRes;
    std::pair<int32_t, std::string> result;
    auto status = cloudServiceImpl_->ChangeConfirmation(sharingRes, confirmation, result);
    EXPECT_EQ(status, GeneralError::E_ERROR);
}

/**
 * @tc.name: GetSharingHandle001
 * @tc.desc: Test the GetSharingHandle with invalid parameters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, GetSharingHandle001, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest GetSharingHandle001 start");
    CloudData::CloudServiceImpl::HapInfo hapInfo;
    auto status = cloudServiceImpl_->GetSharingHandle(hapInfo);
    EXPECT_EQ(status, nullptr);
}

/**
 * @tc.name: SetCloudStrategy001
 * @tc.desc: Test the SetCloudStrategy with get hapInfo failed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, SetCloudStrategy001, TestSize.Level0)
{
    CloudData::Strategy strategy = CloudData::Strategy::STRATEGY_NETWORK;
    std::vector<CommonType::Value> values;
    values.push_back(CloudData::NetWorkStrategy::WIFI);

    auto status = cloudServiceImpl_->SetCloudStrategy(strategy, values);
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
}

/**
 * @tc.name: SetGlobalCloudStrategy001
 * @tc.desc: Test the SetGlobalCloudStrategy with get hapInfo failed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, SetGlobalCloudStrategy001, TestSize.Level0)
{
    CloudData::Strategy strategy = CloudData::Strategy::STRATEGY_NETWORK;
    std::vector<CommonType::Value> values;
    values.push_back(CloudData::NetWorkStrategy::WIFI);

    auto status = cloudServiceImpl_->SetGlobalCloudStrategy(strategy, values);
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
}

/**
 * @tc.name: CheckNotifyConditions001
 * @tc.desc: Test the CheckNotifyConditions with invalid parameters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, CheckNotifyConditions, TestSize.Level0)
{
    CloudInfo cloudInfo;
    cloudInfo.enableCloud = false;
    cloudInfo.id = TEST_CLOUD_APPID;

    auto status = cloudServiceImpl_->CheckNotifyConditions(TEST_CLOUD_APPID, TEST_CLOUD_BUNDLE, cloudInfo);
    EXPECT_EQ(status, CloudData::CloudService::CLOUD_DISABLE);
    cloudInfo.enableCloud = true;
    CloudInfo::AppInfo appInfo;
    appInfo.bundleName = TEST_CLOUD_BUNDLE;
    appInfo.cloudSwitch = false;
    cloudInfo.apps.insert_or_assign(TEST_CLOUD_BUNDLE, appInfo);
    status = cloudServiceImpl_->CheckNotifyConditions(TEST_CLOUD_APPID, TEST_CLOUD_BUNDLE, cloudInfo);
    EXPECT_EQ(status, CloudData::CloudService::CLOUD_DISABLE_SWITCH);
}

/**
 * @tc.name: GetDfxFaultType
 * @tc.desc: test GetDfxFaultType function
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudServiceImplTest, GetDfxFaultType, TestSize.Level0)
{
    ASSERT_NE(cloudServiceImpl_, nullptr);
    EXPECT_EQ(cloudServiceImpl_->GetDfxFaultType(CloudSyncScene::ENABLE_CLOUD), "ENABLE_CLOUD");
    EXPECT_EQ(cloudServiceImpl_->GetDfxFaultType(CloudSyncScene::DISABLE_CLOUD), "DISABLE_CLOUD");
    EXPECT_EQ(cloudServiceImpl_->GetDfxFaultType(CloudSyncScene::SWITCH_ON), "SWITCH_ON");
    EXPECT_EQ(cloudServiceImpl_->GetDfxFaultType(CloudSyncScene::SWITCH_OFF), "SWITCH_OFF");
    EXPECT_EQ(cloudServiceImpl_->GetDfxFaultType(CloudSyncScene::QUERY_SYNC_INFO), "QUERY_SYNC_INFO");
    EXPECT_EQ(cloudServiceImpl_->GetDfxFaultType(CloudSyncScene::USER_CHANGE), "USER_CHANGE");
    EXPECT_EQ(cloudServiceImpl_->GetDfxFaultType(CloudSyncScene::USER_UNLOCK), "USER_UNLOCK");
    EXPECT_EQ(cloudServiceImpl_->GetDfxFaultType(CloudSyncScene::NETWORK_RECOVERY), "NETWORK_RECOVERY");
    EXPECT_EQ(cloudServiceImpl_->GetDfxFaultType(CloudSyncScene::SERVICE_INIT), "SERVICE_INIT");
    EXPECT_EQ(cloudServiceImpl_->GetDfxFaultType(CloudSyncScene::ACCOUNT_STOP), "SYNC_TASK");
}

class ComponentConfigTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void ComponentConfigTest::SetUpTestCase(void) { }

void ComponentConfigTest::TearDownTestCase() { }

void ComponentConfigTest::SetUp() { }

void ComponentConfigTest::TearDown() { }

/**
 * @tc.name: CapabilityRange
 * @tc.desc: test CapabilityRange Marshal function
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(ComponentConfigTest, ComponentConfig, TestSize.Level0)
{
    DistributedData::ComponentConfig config;
    config.description = "description";
    config.lib = "lib";
    config.constructor = "constructor";
    config.destructor = "destructor";
    config.params = "";
    Serializable::json node;
    EXPECT_EQ(config.params.empty(), true);

    EXPECT_EQ(config.Marshal(node), true);
    EXPECT_EQ(node["description"], config.description);
    EXPECT_EQ(node["lib"], config.lib);
    EXPECT_EQ(node["constructor"], config.constructor);
    EXPECT_EQ(node["destructor"], config.destructor);

    DistributedData::ComponentConfig componentConfig;
    componentConfig.description = "description";
    componentConfig.lib = "lib";
    componentConfig.constructor = "constructor";
    componentConfig.destructor = "destructor";
    componentConfig.params = "params";
    Serializable::json node1;
    EXPECT_EQ(componentConfig.params.empty(), false);

    EXPECT_EQ(componentConfig.Marshal(node1), true);
    EXPECT_EQ(node["description"], componentConfig.description);
    EXPECT_EQ(node["lib"], componentConfig.lib);
    EXPECT_EQ(node["constructor"], componentConfig.constructor);
    EXPECT_EQ(node["destructor"], componentConfig.destructor);
}

/**
 * @tc.name: NetworkRecoveryTest001
 * @tc.desc: test the compensatory sync strategy for network disconnection times of less than 20 hours
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudServiceImplTest, NetworkRecoveryTest001, TestSize.Level0)
{
    ASSERT_NE(cloudServiceImpl_, nullptr);
    accountDelegateMock = new (std::nothrow) AccountDelegateMock();
    ASSERT_NE(accountDelegateMock, nullptr);
    AccountDelegate::instance_ = nullptr;
    AccountDelegate::RegisterAccountInstance(accountDelegateMock);

    EXPECT_CALL(*accountDelegateMock, IsLoginAccount()).WillOnce(Return(true));
    EXPECT_CALL(*accountDelegateMock, IsVerified(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*accountDelegateMock, QueryUsers(_)).WillOnce(ReturnWithUserList({ MOCK_USER }));
    EXPECT_CALL(*accountDelegateMock, QueryForegroundUsers(_)).WillOnce(ReturnWithUserList({ MOCK_USER }));
    EXPECT_CALL(*accountDelegateMock, GetUserByToken(_)).WillRepeatedly(Return(MOCK_USER));
    delegate_.isNetworkAvailable_ = false;
    CloudInfo::AppInfo appInfo;
    appInfo.bundleName = TEST_CLOUD_BUNDLE;
    appInfo.cloudSwitch = true;
    std::map<std::string, CloudInfo::AppInfo> apps;
    apps.emplace(TEST_CLOUD_BUNDLE, appInfo);
    CloudInfo cloudInfo;
    cloudInfo.apps = apps;
    cloudInfo.user = MOCK_USER;
    cloudInfo.enableCloud = true;
    cloudInfo.id = TEST_CLOUD_APPID;
    MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true);
    cloudServiceImpl_->Offline(DeviceManagerAdapter::CLOUD_DEVICE_UUID);

    SchemaMeta schemaMeta;
    schemaMeta.bundleName = TEST_CLOUD_BUNDLE;
    SchemaMeta::Database database;
    database.name = TEST_CLOUD_STORE;
    database.alias = TEST_CLOUD_STORE;
    schemaMeta.databases.emplace_back(database);
    MetaDataManager::GetInstance().SaveMeta(CloudInfo::GetSchemaKey(cloudInfo.user, TEST_CLOUD_BUNDLE), schemaMeta,
        true);
    CloudData::CloudService::Option option;
    option.syncMode = DistributedData::GeneralStore::CLOUD_BEGIN;
    auto async = [](const DistributedRdb::Details &details) {};
    cloudServiceImpl_->CloudSync(TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE, option, async);
    cloudServiceImpl_->CloudSync(TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE, option, async);
    sleep(2);
    MetaDataManager::GetInstance().DelMeta(CloudLastSyncInfo::GetKey(MOCK_USER, TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE),
        true);
    delegate_.isNetworkAvailable_ = true;
    cloudServiceImpl_->OnReady(DeviceManagerAdapter::CLOUD_DEVICE_UUID);
    sleep(1);
    auto [status, result] =
        cloudServiceImpl_->QueryLastSyncInfo(TEST_CLOUD_APPID, TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_TRUE(result.find(TEST_CLOUD_STORE) != result.end());
}

/**
 * @tc.name: NetworkRecoveryTest002
 * @tc.desc: test the compensatory sync strategy for network disconnection times of more than 20 hours
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudServiceImplTest, NetworkRecoveryTest002, TestSize.Level0)
{
    ASSERT_NE(cloudServiceImpl_, nullptr);
    EXPECT_CALL(*accountDelegateMock, IsVerified(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*accountDelegateMock, IsLoginAccount()).WillOnce(Return(true));
    EXPECT_CALL(*accountDelegateMock, QueryUsers(_)).WillOnce(ReturnWithUserList({ MOCK_USER }));
    EXPECT_CALL(*accountDelegateMock, QueryForegroundUsers(_)).WillOnce(ReturnWithUserList({ MOCK_USER }));
    EXPECT_CALL(*accountDelegateMock, GetUserByToken(_)).WillRepeatedly(Return(MOCK_USER));
    MetaDataManager::GetInstance().DelMeta(CloudLastSyncInfo::GetKey(MOCK_USER, TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE),
        true);
    cloudServiceImpl_->Offline(DeviceManagerAdapter::CLOUD_DEVICE_UUID);
    auto &recoveryManager = cloudServiceImpl_->syncManager_.networkRecoveryManager_;
    recoveryManager.currentEvent_->disconnectTime -= std::chrono::hours(DISCONNECT_TIME);
    cloudServiceImpl_->OnReady(DeviceManagerAdapter::CLOUD_DEVICE_UUID);
    sleep(1);
    auto [status, result] =
        cloudServiceImpl_->QueryLastSyncInfo(TEST_CLOUD_APPID, TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_TRUE(result.find(TEST_CLOUD_STORE) != result.end());
}

/**
 * @tc.name: NetworkRecoveryTest003
 * @tc.desc: The test only calls the network connection interface but not disconnect
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudServiceImplTest, NetworkRecoveryTest003, TestSize.Level0)
{
    ASSERT_NE(cloudServiceImpl_, nullptr);
    EXPECT_CALL(*accountDelegateMock, IsVerified(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*accountDelegateMock, IsLoginAccount()).WillOnce(Return(true));
    EXPECT_CALL(*accountDelegateMock, QueryForegroundUsers(_)).WillOnce(ReturnWithUserList({ MOCK_USER }));
    EXPECT_CALL(*accountDelegateMock, GetUserByToken(_)).WillRepeatedly(Return(MOCK_USER));
    delegate_.isNetworkAvailable_ = false;
    CloudData::CloudService::Option option;
    option.syncMode = DistributedData::GeneralStore::CLOUD_BEGIN;
    auto async = [](const DistributedRdb::Details &details) {};
    cloudServiceImpl_->CloudSync(TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE, option, async);
    sleep(1);
    MetaDataManager::GetInstance().DelMeta(CloudLastSyncInfo::GetKey(MOCK_USER, TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE),
        true);
    delegate_.isNetworkAvailable_ = true;
    cloudServiceImpl_->OnReady(DeviceManagerAdapter::CLOUD_DEVICE_UUID);
    auto [status, result] =
        cloudServiceImpl_->QueryLastSyncInfo(TEST_CLOUD_APPID, TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: NetworkRecoveryTest004
 * @tc.desc: The test network connection interface call fails when the load cloudInfo failed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudServiceImplTest, NetworkRecoveryTest004, TestSize.Level0)
{
    ASSERT_NE(cloudServiceImpl_, nullptr);
    EXPECT_CALL(*accountDelegateMock, IsVerified(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*accountDelegateMock, IsLoginAccount()).WillOnce(Return(true));
    EXPECT_CALL(*accountDelegateMock, QueryUsers(_)).WillOnce(ReturnWithUserList({ MOCK_USER }));
    EXPECT_CALL(*accountDelegateMock, QueryForegroundUsers(_)).WillOnce(ReturnWithUserList({ MOCK_USER }));
    EXPECT_CALL(*accountDelegateMock, GetUserByToken(_)).WillRepeatedly(Return(MOCK_USER));
    CloudInfo cloudInfo;
    cloudInfo.user = MOCK_USER;
    MetaDataManager::GetInstance().DelMeta(cloudInfo.GetKey(), true);
    auto &recoveryManager = cloudServiceImpl_->syncManager_.networkRecoveryManager_;
    cloudServiceImpl_->Offline(DeviceManagerAdapter::CLOUD_DEVICE_UUID);
    recoveryManager.currentEvent_->disconnectTime -= std::chrono::hours(DISCONNECT_TIME);
    cloudServiceImpl_->OnReady(DeviceManagerAdapter::CLOUD_DEVICE_UUID);
    auto [status, result] =
        cloudServiceImpl_->QueryLastSyncInfo(TEST_CLOUD_APPID, TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE);
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
    EXPECT_TRUE(result.empty());
}

/*
 * @tc.name: CloudServiceImpl_Clean_DbLevelWithEmptyTableInfo
 * @tc.desc: Test Clean with db level config and empty table info
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, CloudServiceImpl_Clean_DbLevelWithEmptyTableInfo, TestSize.Level1)
{
    InitCloudInfoAndSchema();
    std::map<std::string, int32_t> actions;
    actions.insert({ TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_INFO });
    std::map<std::string, ClearConfig> configs;

    ClearConfig clearConfig;
    DBActionInfo dbActionInfo;
    dbActionInfo.action = CloudData::CloudService::Action::CLEAR_CLOUD_INFO;
    // Empty tableInfo
    clearConfig.dbInfo.insert({ TEST_CLOUD_STORE, dbActionInfo });
    configs.insert({ TEST_CLOUD_BUNDLE, clearConfig });

    auto ret = cloudServiceImpl_->Clean(TEST_CLOUD_APPID, actions, configs);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
 * @tc.name: CloudServiceImpl_Clean_DbLevelWithTableInfo
 * @tc.desc: Test Clean with db level config and table info
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, CloudServiceImpl_Clean_DbLevelWithTableInfo, TestSize.Level1)
{
    std::map<std::string, int32_t> actions;
    actions.insert({ TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_INFO });
    std::map<std::string, ClearConfig> configs;

    ClearConfig clearConfig;
    DBActionInfo dbActionInfo;
    dbActionInfo.action = CloudData::CloudService::Action::CLEAR_CLOUD_INFO;
    dbActionInfo.tableInfo.insert(
        { "test_cloud_table_name", CloudData::CloudService::Action::CLEAR_CLOUD_DATA_AND_INFO });
    dbActionInfo.tableInfo.insert({ "test_cloud_table_name1", CloudData::CloudService::Action::CLEAR_CLOUD_NONE });
    clearConfig.dbInfo.insert({ TEST_CLOUD_STORE, dbActionInfo });
    configs.insert({ TEST_CLOUD_BUNDLE, clearConfig });

    auto ret = cloudServiceImpl_->Clean(TEST_CLOUD_APPID, actions, configs);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
 * @tc.name: CloudServiceImpl_Clean_DbLevelWithDbCloudNone
 * @tc.desc: Test Clean with db level CLOUD_NONE action and app default action
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, CloudServiceImpl_Clean_DbLevelWithDbCloudNone, TestSize.Level1)
{
    std::map<std::string, int32_t> actions;
    actions.insert({ TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_NONE });
    std::map<std::string, ClearConfig> configs;

    actions.insert_or_assign(TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_INFO);

    ClearConfig clearConfig;
    DBActionInfo dbActionInfo;
    dbActionInfo.action = CloudData::CloudService::Action::CLEAR_CLOUD_NONE;
    clearConfig.dbInfo.insert({ TEST_CLOUD_STORE, dbActionInfo });
    configs.insert({ TEST_CLOUD_BUNDLE, clearConfig });

    auto ret = cloudServiceImpl_->Clean(TEST_CLOUD_APPID, actions, configs);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
 * @tc.name: CloudServiceImpl_Clean_MixedActionsAndConfigs
 * @tc.desc: Test Clean with mixed app actions and db configs for multiple bundles
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, CloudServiceImpl_Clean_MixedActionsAndConfigs, TestSize.Level1)
{
    std::map<std::string, int32_t> actions;
    actions.insert({ TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_NONE });
    std::map<std::string, ClearConfig> configs;

    // Bundle1: Only app level action
    actions.insert_or_assign(TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_INFO);

    // Bundle2: Only db level config
    ClearConfig clearConfig;
    DBActionInfo dbActionInfo;
    dbActionInfo.action = CloudData::CloudService::Action::CLEAR_CLOUD_DATA_AND_INFO;
    dbActionInfo.tableInfo.insert({ "test_cloud_table_name", CloudData::CloudService::Action::CLEAR_CLOUD_INFO });
    clearConfig.dbInfo.insert({ TEST_CLOUD_STORE, dbActionInfo });
    configs.insert({ "another_bundle", clearConfig });

    auto ret = cloudServiceImpl_->Clean(TEST_CLOUD_APPID, actions, configs);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
 * @tc.name: CloudServiceImpl_Clean_TableLevelWithDifferentActions
 * @tc.desc: Test Clean with table level actions grouping tables by action type
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, CloudServiceImpl_Clean_TableLevelWithDifferentActions, TestSize.Level1)
{
    std::map<std::string, int32_t> actions;
    actions.insert({ TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_NONE });
    std::map<std::string, ClearConfig> configs;

    ClearConfig clearConfig;
    DBActionInfo dbActionInfo;
    dbActionInfo.action = CloudData::CloudService::Action::CLEAR_CLOUD_INFO; // db default action
    dbActionInfo.tableInfo.insert(
        { "test_cloud_table_name", CloudData::CloudService::Action::CLEAR_CLOUD_DATA_AND_INFO });
    dbActionInfo.tableInfo.insert({ "test_cloud_table_name1", CloudData::CloudService::Action::CLEAR_CLOUD_INFO });
    dbActionInfo.tableInfo.insert(
        { "test_cloud_table_name2", CloudData::CloudService::Action::CLEAR_CLOUD_NONE }); // Should be filtered
    clearConfig.dbInfo.insert({ TEST_CLOUD_STORE, dbActionInfo });
    configs.insert({ TEST_CLOUD_BUNDLE, clearConfig });

    auto ret = cloudServiceImpl_->Clean(TEST_CLOUD_APPID, actions, configs);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
 * @tc.name: CloudServiceImpl_Clean_TableLevelWithDbCloudNone
 * @tc.desc: Test Clean with table level actions when db action is CLOUD_NONE
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, CloudServiceImpl_Clean_TableLevelWithDbCloudNone, TestSize.Level1)
{
    std::map<std::string, int32_t> actions;
    actions.insert({ TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_NONE });
    std::map<std::string, ClearConfig> configs;

    actions.insert_or_assign(TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_DATA_AND_INFO);

    ClearConfig clearConfig;
    DBActionInfo dbActionInfo;
    dbActionInfo.action = CloudData::CloudService::Action::CLEAR_CLOUD_NONE;
    dbActionInfo.tableInfo.insert({ "test_cloud_table_name", CloudData::CloudService::Action::CLEAR_CLOUD_INFO });
    dbActionInfo.tableInfo.insert(
        { "test_cloud_table_name1", CloudData::CloudService::Action::CLEAR_CLOUD_DATA_AND_INFO });
    clearConfig.dbInfo.insert({ TEST_CLOUD_STORE, dbActionInfo });
    configs.insert({ TEST_CLOUD_BUNDLE, clearConfig });

    auto ret = cloudServiceImpl_->Clean(TEST_CLOUD_APPID, actions, configs);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
 * @tc.name: CloudServiceImpl_Clean_TableLevelWithAllCloudNone
 * @tc.desc: Test Clean with all actions set to CLOUD_NONE
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, CloudServiceImpl_Clean_TableLevelWithAllCloudNone, TestSize.Level1)
{
    std::map<std::string, int32_t> actions;
    actions.insert({ TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_NONE });
    std::map<std::string, ClearConfig> configs;

    actions.insert_or_assign(TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_NONE);

    ClearConfig clearConfig;
    DBActionInfo dbActionInfo;
    dbActionInfo.action = CloudData::CloudService::Action::CLEAR_CLOUD_NONE;
    dbActionInfo.tableInfo.insert({ "test_cloud_table_name", CloudData::CloudService::Action::CLEAR_CLOUD_NONE });
    dbActionInfo.tableInfo.insert({ "test_cloud_table_name1", CloudData::CloudService::Action::CLEAR_CLOUD_NONE });
    clearConfig.dbInfo.insert({ TEST_CLOUD_STORE, dbActionInfo });
    configs.insert({ TEST_CLOUD_BUNDLE, clearConfig });

    auto ret = cloudServiceImpl_->Clean(TEST_CLOUD_APPID, actions, configs);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
 * @tc.name: CloudServiceImpl_Clean_NonExistentBundle
 * @tc.desc: Test Clean with non-existent bundle name
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, CloudServiceImpl_Clean_NonExistentBundle, TestSize.Level1)
{
    std::map<std::string, int32_t> actions;
    actions.insert({ TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_NONE });
    std::map<std::string, ClearConfig> configs;

    actions.insert_or_assign("non_existent_bundle", CloudData::CloudService::Action::CLEAR_CLOUD_INFO);

    auto ret = cloudServiceImpl_->Clean(TEST_CLOUD_APPID, actions, configs);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS); // Should succeed but skip non-existent bundle
}

/**
 * @tc.name: CloudServiceImpl_Clean_MissingSchemaMeta
 * @tc.desc: Test Clean when schema meta is missing for a bundle
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, CloudServiceImpl_Clean_MissingSchemaMeta, TestSize.Level1)
{
    std::map<std::string, int32_t> actions;
    actions.insert({ TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_NONE });
    std::map<std::string, ClearConfig> configs;

    // Create a bundle that exists in cloudInfo but has no schema meta
    std::string bundleWithoutSchema = "bundle_without_schema";

    // Add bundle to cloudInfo first
    CloudInfo cloudInfo;
    cloudInfo.user = 100;
    cloudInfo.id = TEST_CLOUD_APPID;
    cloudInfo.apps[bundleWithoutSchema] = CloudInfo::AppInfo{};

    // Save cloudInfo
    MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true);

    actions.insert_or_assign(bundleWithoutSchema, CloudData::CloudService::Action::CLEAR_CLOUD_INFO);

    auto ret = cloudServiceImpl_->Clean(TEST_CLOUD_APPID, actions, configs);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS); // Should succeed but skip bundle without schema
}

/**
 * @tc.name: CloudServiceImpl_Clean_StoreMetaDataNotFound
 * @tc.desc: Test Clean when store meta data is not found for a database
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, CloudServiceImpl_Clean_StoreMetaDataNotFound, TestSize.Level1)
{
    std::map<std::string, int32_t> actions;
    actions.insert({ TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_NONE });
    std::map<std::string, ClearConfig> configs;
    actions.insert_or_assign(TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_INFO);
    auto ret = cloudServiceImpl_->Clean(TEST_CLOUD_APPID, actions, configs);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
 * @tc.name: QueryLastSyncInfoBatch_EmptyAccountId
 * @tc.desc: Test QueryLastSyncInfoBatch with empty accountId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryLastSyncInfoBatch_EmptyAccountId, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest QueryLastSyncInfoBatch_EmptyAccountId start");
    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo info;
    info.bundleName = TEST_CLOUD_BUNDLE;
    bundleInfos.push_back(info);

    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfoBatch("", bundleInfos);
    EXPECT_EQ(status, CloudData::CloudService::INVALID_ARGUMENT_V20);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: QueryLastSyncInfoBatch_EmptyBundleInfosArray
 * @tc.desc: Test QueryLastSyncInfoBatch with empty bundleInfos array
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryLastSyncInfoBatch_EmptyBundleInfosArray, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest QueryLastSyncInfoBatch_EmptyBundleInfosArray start");
    std::vector<CloudData::BundleInfo> bundleInfos;

    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfoBatch(TEST_CLOUD_APPID, bundleInfos);
    EXPECT_EQ(status, CloudData::CloudService::INVALID_ARGUMENT_V20);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: QueryLastSyncInfoBatch_GetCloudInfoFailed
 * @tc.desc: Test QueryLastSyncInfoBatch when GetCloudInfo returns error
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryLastSyncInfoBatch_GetCloudInfoFailed, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest QueryLastSyncInfoBatch_GetCloudInfoFailed start");
    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo info;
    info.bundleName = TEST_CLOUD_BUNDLE;
    bundleInfos.push_back(info);

    // No CloudInfo data prepared
    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfoBatch(TEST_CLOUD_APPID, bundleInfos);
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: QueryLastSyncInfoBatch_EmptyBundleName
 * @tc.desc: Test QueryLastSyncInfoBatch with empty bundleName in BundleInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryLastSyncInfoBatch_EmptyBundleName, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest QueryLastSyncInfoBatch_EmptyBundleName start");
    InitCloudInfoAndSchema();

    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo info;
    info.bundleName = ""; // Empty bundleName
    bundleInfos.push_back(info);

    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfoBatch(TEST_CLOUD_APPID, bundleInfos);
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
}

/**
 * @tc.name: QueryLastSyncInfoBatch_InvalidBundleName
 * @tc.desc: Test QueryLastSyncInfoBatch with invalid bundleName not in cloudInfo.apps
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryLastSyncInfoBatch_InvalidBundleName, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest QueryLastSyncInfoBatch_InvalidBundleName start");
    InitCloudInfoAndSchema();

    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo info;
    info.bundleName = "invalid_bundle_name";
    bundleInfos.push_back(info);

    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfoBatch(TEST_CLOUD_APPID, bundleInfos);
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
}

/**
 * @tc.name: QueryLastSyncInfoBatch_QueryFailed
 * @tc.desc: Test QueryLastSyncInfoBatch when SyncManager::QueryLastSyncInfo returns error
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryLastSyncInfoBatch_QueryFailed, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest QueryLastSyncInfoBatch_QueryFailed start");
    InitCloudInfoAndSchema();

    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo info;
    info.bundleName = TEST_CLOUD_BUNDLE;
    bundleInfos.push_back(info);

    // Mock SyncManager to return error
    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfoBatch(TEST_CLOUD_APPID, bundleInfos);
    // Since we don't have mock for SyncManager, it will return empty result
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
}

/**
 * @tc.name: QueryLastSyncInfoBatch_EmptyStoreResults
 * @tc.desc: Test QueryLastSyncInfoBatch when QueryLastSyncInfo returns empty storeResults
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryLastSyncInfoBatch_EmptyStoreResults, TestSize.Level1)
{
    ZLOGI("CloudServiceImplTest QueryLastSyncInfoBatch_EmptyStoreResults start");
    InitCloudInfoAndSchema();

    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo info;
    info.bundleName = TEST_CLOUD_BUNDLE;
    bundleInfos.push_back(info);

    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfoBatch(TEST_CLOUD_APPID, bundleInfos);
    // QueryLastSyncInfo will return empty results without actual sync data
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
}

/**
 * @tc.name: QueryLastSyncInfoBatch_ValidStoreResults
 * @tc.desc: Test QueryLastSyncInfoBatch when QueryLastSyncInfo returns valid storeResults
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryLastSyncInfoBatch_ValidStoreResults, TestSize.Level1)
{
    ZLOGI("CloudServiceImplTest QueryLastSyncInfoBatch_ValidStoreResults start");
    InitCloudInfoAndSchema();

    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo info;
    info.bundleName = TEST_CLOUD_BUNDLE;
    bundleInfos.push_back(info);

    // Prepare CloudLastSyncInfo data
    CloudLastSyncInfo lastSyncInfo;
    lastSyncInfo.startTime = 1234567890;
    lastSyncInfo.finishTime = 1234567900;
    lastSyncInfo.code = 0;
    lastSyncInfo.syncStatus = 1;
    lastSyncInfo.storeId = TEST_CLOUD_STORE;
    lastSyncInfo.id = TEST_CLOUD_APPID;

    std::string syncInfoKey = CloudLastSyncInfo::GetKey(
        AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId), TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE);
    MetaDataManager::GetInstance().SaveMeta(syncInfoKey, lastSyncInfo, true);

    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfoBatch(TEST_CLOUD_APPID, bundleInfos);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.find(TEST_CLOUD_BUNDLE) != result.end());

    // Clean up
    MetaDataManager::GetInstance().DelMeta(syncInfoKey, true);
}

/**
 * @tc.name: QueryLastSyncInfoBatch_AllBundlesFailed
 * @tc.desc: Test QueryLastSyncInfoBatch when all BundleInfo queries fail
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryLastSyncInfoBatch_AllBundlesFailed, TestSize.Level1)
{
    ZLOGI("CloudServiceImplTest QueryLastSyncInfoBatch_AllBundlesFailed start");
    InitCloudInfoAndSchema();

    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo info1;
    info1.bundleName = ""; // Empty bundleName
    bundleInfos.push_back(info1);

    CloudData::BundleInfo info2;
    info2.bundleName = "invalid_bundle";
    bundleInfos.push_back(info2);

    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfoBatch(TEST_CLOUD_APPID, bundleInfos);
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
}

/**
 * @tc.name: QueryLastSyncInfoBatch_SomeBundlesSucceed
 * @tc.desc: Test QueryLastSyncInfoBatch when some BundleInfo queries succeed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryLastSyncInfoBatch_SomeBundlesSucceed, TestSize.Level1)
{
    ZLOGI("CloudServiceImplTest QueryLastSyncInfoBatch_SomeBundlesSucceed start");
    InitCloudInfoAndSchema();

    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo validInfo;
    validInfo.bundleName = TEST_CLOUD_BUNDLE;
    bundleInfos.push_back(validInfo);

    CloudData::BundleInfo invalidInfo;
    invalidInfo.bundleName = "invalid_bundle";
    bundleInfos.push_back(invalidInfo);

    // Prepare CloudLastSyncInfo data for valid bundle
    CloudLastSyncInfo lastSyncInfo;
    lastSyncInfo.startTime = 1234567890;
    lastSyncInfo.finishTime = 1234567900;
    lastSyncInfo.code = 0;
    lastSyncInfo.syncStatus = 1;
    lastSyncInfo.storeId = TEST_CLOUD_STORE;
    lastSyncInfo.id = TEST_CLOUD_APPID;

    std::string syncInfoKey = CloudLastSyncInfo::GetKey(
        AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId), TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE);
    MetaDataManager::GetInstance().SaveMeta(syncInfoKey, lastSyncInfo, true);

    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfoBatch(TEST_CLOUD_APPID, bundleInfos);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.find(TEST_CLOUD_BUNDLE) != result.end());
    EXPECT_TRUE(result.find("invalid_bundle") != result.end());
    EXPECT_TRUE(result["invalid_bundle"].empty());

    // Clean up
    MetaDataManager::GetInstance().DelMeta(syncInfoKey, true);
}

/**
 * @tc.name: QueryLastSyncInfoBatch_MixedValidInvalid
 * @tc.desc: Test QueryLastSyncInfoBatch with mixed valid and invalid BundleInfo objects
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryLastSyncInfoBatch_MixedValidInvalid, TestSize.Level1)
{
    ZLOGI("CloudServiceImplTest QueryLastSyncInfoBatch_MixedValidInvalid start");
    InitCloudInfoAndSchema();

    std::vector<CloudData::BundleInfo> bundleInfos;

    // Valid BundleInfo
    CloudData::BundleInfo validInfo;
    validInfo.bundleName = TEST_CLOUD_BUNDLE;
    bundleInfos.push_back(validInfo);

    // Empty bundleName
    CloudData::BundleInfo emptyInfo;
    emptyInfo.bundleName = "";
    bundleInfos.push_back(emptyInfo);

    // Invalid bundleName
    CloudData::BundleInfo invalidInfo;
    invalidInfo.bundleName = "invalid_bundle";
    bundleInfos.push_back(invalidInfo);

    // Prepare CloudLastSyncInfo data for valid bundle
    CloudLastSyncInfo lastSyncInfo;
    lastSyncInfo.startTime = 1234567890;
    lastSyncInfo.finishTime = 1234567900;
    lastSyncInfo.code = 0;
    lastSyncInfo.syncStatus = 1;
    lastSyncInfo.storeId = TEST_CLOUD_STORE;
    lastSyncInfo.id = TEST_CLOUD_APPID;

    std::string syncInfoKey = CloudLastSyncInfo::GetKey(
        AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId), TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE);
    MetaDataManager::GetInstance().SaveMeta(syncInfoKey, lastSyncInfo, true);

    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfoBatch(TEST_CLOUD_APPID, bundleInfos);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.size(), 3); // All three should be in results
    EXPECT_TRUE(result.find(TEST_CLOUD_BUNDLE) != result.end());
    EXPECT_FALSE(result[TEST_CLOUD_BUNDLE].empty());

    // Clean up
    MetaDataManager::GetInstance().DelMeta(syncInfoKey, true);
}

/**
 * @tc.name: QueryLastSyncInfoBatch_MultipleBundles
 * @tc.desc: Test QueryLastSyncInfoBatch with multiple valid BundleInfo objects
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryLastSyncInfoBatch_MultipleBundles, TestSize.Level1)
{
    ZLOGI("CloudServiceImplTest QueryLastSyncInfoBatch_MultipleBundles start");
    InitCloudInfoAndSchema();

    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo info1;
    info1.bundleName = TEST_CLOUD_BUNDLE;
    bundleInfos.push_back(info1);

    CloudData::BundleInfo info2;
    info2.bundleName = "test_bundle_2";
    bundleInfos.push_back(info2);

    // Prepare CloudLastSyncInfo data for first bundle
    CloudLastSyncInfo lastSyncInfo;
    lastSyncInfo.startTime = 1234567890;
    lastSyncInfo.finishTime = 1234567900;
    lastSyncInfo.code = 0;
    lastSyncInfo.syncStatus = 1;
    lastSyncInfo.storeId = TEST_CLOUD_STORE;
    lastSyncInfo.id = TEST_CLOUD_APPID;

    std::string syncInfoKey = CloudLastSyncInfo::GetKey(
        AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId), TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE);
    MetaDataManager::GetInstance().SaveMeta(syncInfoKey, lastSyncInfo, true);

    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfoBatch(TEST_CLOUD_APPID, bundleInfos);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.find(TEST_CLOUD_BUNDLE) != result.end());

    // Clean up
    MetaDataManager::GetInstance().DelMeta(syncInfoKey, true);
}

/**
 * @tc.name: QueryLastSyncInfoBatch_WithStoreId
 * @tc.desc: Test QueryLastSyncInfoBatch with BundleInfo containing storeId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryLastSyncInfoBatch_WithStoreId, TestSize.Level1)
{
    ZLOGI("CloudServiceImplTest QueryLastSyncInfoBatch_WithStoreId start");
    InitCloudInfoAndSchema();

    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo info;
    info.bundleName = TEST_CLOUD_BUNDLE;
    info.storeId = TEST_CLOUD_STORE;
    bundleInfos.push_back(info);

    // Prepare CloudLastSyncInfo data
    CloudLastSyncInfo lastSyncInfo;
    lastSyncInfo.startTime = 1234567890;
    lastSyncInfo.finishTime = 1234567900;
    lastSyncInfo.code = 0;
    lastSyncInfo.syncStatus = 1;
    lastSyncInfo.storeId = TEST_CLOUD_STORE;
    lastSyncInfo.id = TEST_CLOUD_APPID;

    std::string syncInfoKey = CloudLastSyncInfo::GetKey(
        AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId), TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE);
    MetaDataManager::GetInstance().SaveMeta(syncInfoKey, lastSyncInfo, true);

    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfoBatch(TEST_CLOUD_APPID, bundleInfos);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.find(TEST_CLOUD_BUNDLE) != result.end());
    EXPECT_TRUE(result[TEST_CLOUD_BUNDLE].find(TEST_CLOUD_STORE) != result[TEST_CLOUD_BUNDLE].end());

    // Clean up
    MetaDataManager::GetInstance().DelMeta(syncInfoKey, true);
}

/**
 * @tc.name: QueryLastSyncInfoBatch_ValidStoreResults_MultipleStores
 * @tc.desc: Test QueryLastSyncInfoBatch when QueryLastSyncInfo returns valid storeResults with multiple stores
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryLastSyncInfoBatch_ValidStoreResults_MultipleStores, TestSize.Level0)
{
    ZLOGI("CloudServiceImplTest QueryLastSyncInfoBatch_ValidStoreResults start");
    InitCloudInfoAndSchema();

    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo info;
    info.bundleName = TEST_CLOUD_BUNDLE;
    bundleInfos.push_back(info);

    // Prepare CloudLastSyncInfo data for multiple stores
    CloudLastSyncInfo lastSyncInfo1;
    lastSyncInfo1.startTime = 1234567890;
    lastSyncInfo1.finishTime = 1234567900;
    lastSyncInfo1.code = 0;
    lastSyncInfo1.syncStatus = 1;
    lastSyncInfo1.storeId = TEST_CLOUD_STORE;
    lastSyncInfo1.id = TEST_CLOUD_APPID;

    std::string syncInfoKey1 = CloudLastSyncInfo::GetKey(
        AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId), TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE);
    MetaDataManager::GetInstance().SaveMeta(syncInfoKey1, lastSyncInfo1, true);

    CloudLastSyncInfo lastSyncInfo2;
    lastSyncInfo2.startTime = 1234567880;
    lastSyncInfo2.finishTime = 1234567895;
    lastSyncInfo2.code = 0;
    lastSyncInfo2.syncStatus = 1;
    lastSyncInfo2.storeId = "TEST_CLOUD_STORE2";
    lastSyncInfo2.id = TEST_CLOUD_APPID;

    std::string syncInfoKey2 = CloudLastSyncInfo::GetKey(
        AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId), TEST_CLOUD_BUNDLE, "TEST_CLOUD_STORE2");
    MetaDataManager::GetInstance().SaveMeta(syncInfoKey2, lastSyncInfo2, true);

    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfoBatch(TEST_CLOUD_APPID, bundleInfos);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.find(TEST_CLOUD_BUNDLE) != result.end());
    EXPECT_EQ(result[TEST_CLOUD_BUNDLE].size(), 2); // Should have 2 stores
    EXPECT_TRUE(result[TEST_CLOUD_BUNDLE].find(TEST_CLOUD_STORE) != result[TEST_CLOUD_BUNDLE].end());
    EXPECT_TRUE(result[TEST_CLOUD_BUNDLE].find("TEST_CLOUD_STORE2") != result[TEST_CLOUD_BUNDLE].end());

    // Verify CloudSyncInfo structure
    auto &cloudSyncInfo1 = result[TEST_CLOUD_BUNDLE][TEST_CLOUD_STORE];
    EXPECT_EQ(cloudSyncInfo1.startTime, lastSyncInfo1.startTime);
    EXPECT_EQ(cloudSyncInfo1.finishTime, lastSyncInfo1.finishTime);
    EXPECT_EQ(cloudSyncInfo1.code, lastSyncInfo1.code);
    EXPECT_EQ(cloudSyncInfo1.syncStatus, lastSyncInfo1.syncStatus);

    // Clean up
    MetaDataManager::GetInstance().DelMeta(syncInfoKey1, true);
    MetaDataManager::GetInstance().DelMeta(syncInfoKey2, true);
}

/**
 * @tc.name: QueryLastSyncInfoBatch_31Items
 * @tc.desc: Test QueryLastSyncInfoBatch with 31 BundleInfo items (should fail)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, QueryLastSyncInfoBatch_31Items, TestSize.Level1)
{
    ZLOGI("CloudServiceImplTest QueryLastSyncInfoBatch_31Items start");
    InitCloudInfoAndSchema();
    
    std::vector<CloudData::BundleInfo> bundleInfos;
    for (int i = 0; i < 31; i++) {
        CloudData::BundleInfo info;
        info.bundleName = TEST_CLOUD_BUNDLE;
        bundleInfos.push_back(info);
    }
    
    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfoBatch(TEST_CLOUD_APPID, bundleInfos);
    EXPECT_EQ(status, CloudData::CloudService::INVALID_ARGUMENT_V20);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: Subscribe_Success
 * @tc.desc: Test Subscribe with valid parameters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, Subscribe_Success, TestSize.Level1)
{
    ZLOGI("CloudServiceImplTest Subscribe_Success start");
    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo bundleInfo;
    bundleInfo.bundleName = TEST_CLOUD_BUNDLE;
    bundleInfo.storeId = TEST_CLOUD_STORE;
    bundleInfos.push_back(bundleInfo);

    auto observer = std::make_shared<SyncInfoObserverMock>();
    auto status = cloudServiceImpl_->Subscribe(CloudData::CloudSubscribeType::SYNC_INFO_CHANGED,
        bundleInfos, observer);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
}

/**
 * @tc.name: Subscribe_EmptyBundleInfos
 * @tc.desc: Test Subscribe with empty bundleInfos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, Subscribe_EmptyBundleInfos, TestSize.Level1)
{
    ZLOGI("CloudServiceImplTest Subscribe_EmptyBundleInfos start");
    std::vector<CloudData::BundleInfo> bundleInfos;
    auto observer = std::make_shared<SyncInfoObserverMock>();
    auto status = cloudServiceImpl_->Subscribe(CloudData::CloudSubscribeType::SYNC_INFO_CHANGED,
        bundleInfos, observer);
    EXPECT_EQ(status, CloudData::CloudService::INVALID_ARGUMENT);
}

/**
 * @tc.name: Subscribe_DuplicateSubscribe
 * @tc.desc: Test Subscribe with duplicate subscription
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, Subscribe_DuplicateSubscribe, TestSize.Level1)
{
    ZLOGI("CloudServiceImplTest Subscribe_DuplicateSubscribe start");
    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo bundleInfo;
    bundleInfo.bundleName = TEST_CLOUD_BUNDLE;
    bundleInfo.storeId = TEST_CLOUD_STORE;
    bundleInfos.push_back(bundleInfo);

    auto observer = std::make_shared<SyncInfoObserverMock>();
    auto status = cloudServiceImpl_->Subscribe(CloudData::CloudSubscribeType::SYNC_INFO_CHANGED,
        bundleInfos, observer);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);

    status = cloudServiceImpl_->Subscribe(CloudData::CloudSubscribeType::SYNC_INFO_CHANGED,
        bundleInfos, observer);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
}

/**
 * @tc.name: Unsubscribe_Success
 * @tc.desc: Test Unsubscribe with valid parameters
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, Unsubscribe_Success, TestSize.Level1)
{
    ZLOGI("CloudServiceImplTest Unsubscribe_Success start");
    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo bundleInfo;
    bundleInfo.bundleName = TEST_CLOUD_BUNDLE;
    bundleInfo.storeId = TEST_CLOUD_STORE;
    bundleInfos.push_back(bundleInfo);

    auto observer = std::make_shared<SyncInfoObserverMock>();
    auto status = cloudServiceImpl_->Subscribe(CloudData::CloudSubscribeType::SYNC_INFO_CHANGED,
        bundleInfos, observer);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);

    status = cloudServiceImpl_->Unsubscribe(CloudData::CloudSubscribeType::SYNC_INFO_CHANGED,
        bundleInfos, observer);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
}

/**
 * @tc.name: Unsubscribe_EmptyBundleInfos
 * @tc.desc: Test Unsubscribe with empty bundleInfos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, Unsubscribe_EmptyBundleInfos, TestSize.Level1)
{
    ZLOGI("CloudServiceImplTest Unsubscribe_EmptyBundleInfos start");
    std::vector<CloudData::BundleInfo> bundleInfos;
    auto observer = std::make_shared<SyncInfoObserverMock>();
    auto status = cloudServiceImpl_->Unsubscribe(CloudData::CloudSubscribeType::SYNC_INFO_CHANGED,
        bundleInfos, observer);
    EXPECT_EQ(status, CloudData::CloudService::INVALID_ARGUMENT);
}

/**
 * @tc.name: Unsubscribe_NotSubscribed
 * @tc.desc: Test Unsubscribe when not subscribed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, Unsubscribe_NotSubscribed, TestSize.Level1)
{
    ZLOGI("CloudServiceImplTest Unsubscribe_NotSubscribed start");
    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo bundleInfo;
    bundleInfo.bundleName = TEST_CLOUD_BUNDLE;
    bundleInfo.storeId = TEST_CLOUD_STORE;
    bundleInfos.push_back(bundleInfo);

    auto observer = std::make_shared<SyncInfoObserverMock>();
    auto status = cloudServiceImpl_->Unsubscribe(CloudData::CloudSubscribeType::SYNC_INFO_CHANGED,
        bundleInfos, observer);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
}

/**
 * @tc.name: Unsubscribe_MultipleBundleInfos
 * @tc.desc: Test Unsubscribe with multiple bundleInfos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceImplTest, Unsubscribe_MultipleBundleInfos, TestSize.Level1)
{
    ZLOGI("CloudServiceImplTest Unsubscribe_MultipleBundleInfos start");
    std::vector<CloudData::BundleInfo> bundleInfos;
    CloudData::BundleInfo bundleInfo1;
    bundleInfo1.bundleName = TEST_CLOUD_BUNDLE;
    bundleInfo1.storeId = TEST_CLOUD_STORE;
    bundleInfos.push_back(bundleInfo1);

    CloudData::BundleInfo bundleInfo2;
    bundleInfo2.bundleName = TEST_CLOUD_BUNDLE;
    bundleInfo2.storeId = "TEST_CLOUD_STORE2";
    bundleInfos.push_back(bundleInfo2);

    auto observer = std::make_shared<SyncInfoObserverMock>();
    auto status = cloudServiceImpl_->Subscribe(CloudData::CloudSubscribeType::SYNC_INFO_CHANGED,
        bundleInfos, observer);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);

    std::vector<CloudData::BundleInfo> unsubscribeBundleInfos;
    unsubscribeBundleInfos.push_back(bundleInfo1);
    status = cloudServiceImpl_->Unsubscribe(CloudData::CloudSubscribeType::SYNC_INFO_CHANGED,
        unsubscribeBundleInfos, observer);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
}

} // namespace DistributedDataTest
} // namespace OHOS::Test