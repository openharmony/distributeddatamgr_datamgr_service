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

#define LOG_TAG "CloudSyncManagerTest"

#include <gtest/gtest.h>
#include <unistd.h>

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "bootstrap.h"
#include "checker_mock.h"
#include "cloud/change_event.h"
#include "cloud/cloud_last_sync_info.h"
#include "cloud/cloud_server.h"
#include "cloud_data_translate.h"
#include "cloud_service_impl.h"
#include "cloud_types.h"
#include "communicator/device_manager_adapter.h"
#include "device_matrix.h"
#include "eventcenter/event_center.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "mock/db_store_mock.h"
#include "mock/general_store_mock.h"
#include "network/network_delegate.h"
#include "network_delegate_mock.h"
#include "sync_manager.h"
#include "sync_strategies/network_sync_strategy.h"
#include "token_setproc.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::CloudData;
using namespace OHOS::Security::AccessToken;
using Database = SchemaMeta::Database;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Querykey = OHOS::CloudData::QueryKey;
using CloudSyncInfo = OHOS::CloudData::CloudSyncInfo;
using GenErr = OHOS::DistributedData::GeneralError;
uint64_t g_selfTokenID = 0;

void AllocHapToken(const HapPolicyParams &policy)
{
    HapInfoParams info = {
        .userID = 100,
        .bundleName = "test_cloud_bundleName",
        .instIndex = 0,
        .appIDDesc = "test_cloud_bundleName",
        .isSystemApp = true
    };
    auto token = AccessTokenKit::AllocHapToken(info, policy);
    SetSelfTokenID(token.tokenIDEx);
}
namespace OHOS::Test {
namespace DistributedDataTest {
static constexpr const char *TEST_CLOUD_BUNDLE = "test_cloud_bundleName";
static constexpr const char *TEST_CLOUD_APPID = "test_cloud_appid";
static constexpr const char *TEST_CLOUD_STORE = "test_cloud_store";
static constexpr const char *TEST_CLOUD_STORE_1 = "test_cloud_store1";
static constexpr const char *TEST_CLOUD_ID = "test_cloud_id";
static constexpr const char *TEST_CLOUD_DATABASE_ALIAS_1 = "test_cloud_database_alias_1";
static constexpr const char *TEST_CLOUD_DATABASE_ALIAS_2 = "test_cloud_database_alias_2";
static constexpr const char *PERMISSION_CLOUDDATA_CONFIG = "ohos.permission.CLOUDDATA_CONFIG";
static constexpr const char *PERMISSION_GET_NETWORK_INFO = "ohos.permission.GET_NETWORK_INFO";
static constexpr const char *PERMISSION_DISTRIBUTED_DATASYNC = "ohos.permission.DISTRIBUTED_DATASYNC";
static constexpr const char *PERMISSION_ACCESS_SERVICE_DM = "ohos.permission.ACCESS_SERVICE_DM";
static constexpr const char *PERMISSION_MANAGE_LOCAL_ACCOUNTS = "ohos.permission.MANAGE_LOCAL_ACCOUNTS";
static constexpr const char *PERMISSION_GET_BUNDLE_INFO = "ohos.permission.GET_BUNDLE_INFO_PRIVILEGED";
static constexpr const char *TEST_CLOUD_PATH =
    "/data/app/el2/100/database/test_cloud_bundleName/entry/rdb/test_cloud_store";
PermissionDef GetPermissionDef(const std::string &permission)
{
    PermissionDef def = { .permissionName = permission,
        .bundleName = "test_cloud_bundleName",
        .grantMode = 1,
        .availableLevel = APL_SYSTEM_BASIC,
        .label = "label",
        .labelId = 1,
        .description = "test_cloud_bundleName",
        .descriptionId = 1 };
    return def;
}

PermissionStateFull GetPermissionStateFull(const std::string &permission)
{
    PermissionStateFull stateFull = { .permissionName = permission,
        .isGeneral = true,
        .resDeviceID = { "local" },
        .grantStatus = { PermissionState::PERMISSION_GRANTED },
        .grantFlags = { 1 } };
    return stateFull;
}
class CloudSyncManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static SchemaMeta schemaMeta_;
    static std::shared_ptr<CloudData::CloudServiceImpl> cloudServiceImpl_;

protected:
    class CloudServerMock : public CloudServer {
    public:
        std::pair<int32_t, CloudInfo> GetServerInfo(int32_t userId, bool needSpaceInfo) override;
        std::pair<int32_t, SchemaMeta> GetAppSchema(int32_t userId, const std::string &bundle) override;
        virtual ~CloudServerMock() = default;
        static constexpr uint64_t REMAINSPACE = 1000;
        static constexpr uint64_t TATALSPACE = 2000;
        static constexpr int32_t INVALID_USER_ID = -1;
    };

    static void InitMetaData();
    static void InitSchemaMeta();
    static void InitCloudInfo();
    static void PrepareTestEnvironment();
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static StoreMetaData metaData_;
    static CloudInfo cloudInfo_;
    static DistributedData::CheckerMock checker_;
    static NetworkDelegateMock delegate_;
    static int32_t dbStatus_;
};

std::pair<int32_t, CloudInfo> CloudSyncManagerTest::CloudServerMock::GetServerInfo(
    int32_t userId, bool needSpaceInfo)
{
    (void)needSpaceInfo;
    CloudInfo cloudInfo;
    cloudInfo.user = userId;
    cloudInfo.id = TEST_CLOUD_ID;
    cloudInfo.remainSpace = REMAINSPACE;
    cloudInfo.totalSpace = TATALSPACE;
    cloudInfo.enableCloud = true;

    CloudInfo::AppInfo appInfo;
    appInfo.bundleName = TEST_CLOUD_BUNDLE;
    appInfo.appId = TEST_CLOUD_APPID;
    appInfo.version = 1;
    appInfo.cloudSwitch = true;

    cloudInfo.apps[TEST_CLOUD_BUNDLE] = std::move(appInfo);
    return { E_OK, cloudInfo };
}

std::pair<int32_t, SchemaMeta> CloudSyncManagerTest::CloudServerMock::GetAppSchema(
    int32_t userId, const std::string &bundle)
{
    if (userId == INVALID_USER_ID) {
        return { E_ERROR, schemaMeta_ };
    }

    if (bundle.empty()) {
        SchemaMeta schemaMeta;
        return { E_OK, schemaMeta };
    }
    return { E_OK, schemaMeta_ };
}

std::shared_ptr<DBStoreMock> CloudSyncManagerTest::dbStoreMock_ =
    std::make_shared<DBStoreMock>();
SchemaMeta CloudSyncManagerTest::schemaMeta_;
StoreMetaData CloudSyncManagerTest::metaData_;
CloudInfo CloudSyncManagerTest::cloudInfo_;
std::shared_ptr<CloudData::CloudServiceImpl> CloudSyncManagerTest::cloudServiceImpl_ =
    std::make_shared<CloudData::CloudServiceImpl>();
DistributedData::CheckerMock CloudSyncManagerTest::checker_;
NetworkDelegateMock CloudSyncManagerTest::delegate_;
int32_t CloudSyncManagerTest::dbStatus_ = E_OK;

void CloudSyncManagerTest::InitMetaData()
{
    metaData_.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData_.appId = TEST_CLOUD_APPID;
    metaData_.bundleName = TEST_CLOUD_BUNDLE;
    metaData_.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    metaData_.user = std::to_string(AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId));
    metaData_.area = OHOS::DistributedKv::EL1;
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.storeType = DistributedRdb::RDB_DEVICE_COLLABORATION;
    metaData_.storeId = TEST_CLOUD_STORE;
    metaData_.dataDir = TEST_CLOUD_PATH;
    PolicyValue value;
    value.type = OHOS::DistributedKv::PolicyType::IMMEDIATE_SYNC_ON_ONLINE;
}

void CloudSyncManagerTest::InitSchemaMeta()
{
    schemaMeta_.databases.clear();
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
    database.alias = TEST_CLOUD_DATABASE_ALIAS_1;
    database.tables.emplace_back(table);

    SchemaMeta::Database database2;
    database2.name = TEST_CLOUD_STORE_1;
    database2.alias = TEST_CLOUD_DATABASE_ALIAS_2;
    database2.tables.emplace_back(table);
    schemaMeta_.databases.emplace_back(database2);

    schemaMeta_.version = 1;
    schemaMeta_.bundleName = TEST_CLOUD_BUNDLE;
    schemaMeta_.databases.emplace_back(database);
    schemaMeta_.e2eeEnable = false;
}

void CloudSyncManagerTest::InitCloudInfo()
{
    cloudInfo_.user = AccountDelegate::GetInstance()->GetUserByToken(IPCSkeleton::GetCallingTokenID());
    cloudInfo_.id = TEST_CLOUD_ID;
    cloudInfo_.enableCloud = true;

    CloudInfo::AppInfo appInfo;
    appInfo.bundleName = TEST_CLOUD_BUNDLE;
    appInfo.appId = TEST_CLOUD_APPID;
    appInfo.version = 1;
    appInfo.cloudSwitch = true;

    cloudInfo_.apps[TEST_CLOUD_BUNDLE] = std::move(appInfo);
}

void CloudSyncManagerTest::PrepareTestEnvironment()
{
    // Save cloud info
    MetaDataManager::GetInstance().SaveMeta(cloudInfo_.GetKey(), cloudInfo_, true);

    // Save schema metadata
    std::string schemaKey = CloudInfo::GetSchemaKey(cloudInfo_.user, TEST_CLOUD_BUNDLE);
    InitSchemaMeta();
    ASSERT_TRUE(MetaDataManager::GetInstance().SaveMeta(schemaKey, schemaMeta_, true));

    // Save store metadata
    MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true);
}

void CloudSyncManagerTest::SetUpTestCase(void)
{
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    MetaDataManager::GetInstance().SetSyncer([](const auto &, auto) {
        DeviceMatrix::GetInstance().OnChanged(DeviceMatrix::META_STORE_MASK);
    });

    auto cloudServerMock = new CloudServerMock();
    CloudServer::RegisterCloudInstance(cloudServerMock);
    HapPolicyParams policy = { .apl = APL_SYSTEM_BASIC,
        .domain = "test.domain",
        .permList = { GetPermissionDef(PERMISSION_CLOUDDATA_CONFIG), GetPermissionDef(PERMISSION_GET_NETWORK_INFO),
            GetPermissionDef(PERMISSION_DISTRIBUTED_DATASYNC), GetPermissionDef(PERMISSION_ACCESS_SERVICE_DM),
            GetPermissionDef(PERMISSION_MANAGE_LOCAL_ACCOUNTS), GetPermissionDef(PERMISSION_GET_BUNDLE_INFO) },
        .permStateList = { GetPermissionStateFull(PERMISSION_CLOUDDATA_CONFIG),
            GetPermissionStateFull(PERMISSION_GET_NETWORK_INFO),
            GetPermissionStateFull(PERMISSION_DISTRIBUTED_DATASYNC),
            GetPermissionStateFull(PERMISSION_ACCESS_SERVICE_DM),
            GetPermissionStateFull(PERMISSION_MANAGE_LOCAL_ACCOUNTS),
            GetPermissionStateFull(PERMISSION_GET_BUNDLE_INFO)} };
    g_selfTokenID = GetSelfTokenID();
    AllocHapToken(policy);
    size_t max = 12;
    size_t min = 5;

    auto executor = std::make_shared<ExecutorPool>(max, min);
    cloudServiceImpl_->OnBind(
        { "CloudSyncManagerTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()),
            std::move(executor) });

    Bootstrap::GetInstance().LoadCheckers();
    auto dmExecutor = std::make_shared<ExecutorPool>(max, min);
    DeviceManagerAdapter::GetInstance().Init(dmExecutor);
    NetworkDelegate::RegisterNetworkInstance(&delegate_);

    InitCloudInfo();
    InitMetaData();
    InitSchemaMeta();
}

void CloudSyncManagerTest::TearDownTestCase(void)
{
    SetSelfTokenID(g_selfTokenID);

}

void CloudSyncManagerTest::SetUp()
{
    PrepareTestEnvironment();
}

void CloudSyncManagerTest::TearDown()
{
    MetaDataManager::GetInstance().DelMeta(cloudInfo_.GetKey(), true);
    std::string schemaKey = CloudInfo::GetSchemaKey(cloudInfo_.user, TEST_CLOUD_BUNDLE);
    MetaDataManager::GetInstance().DelMeta(schemaKey, true);
    MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true);
}

/**
* @tc.name: DoCloudSync_GetCloudSyncInfo_StoresEmpty_VerifyQueryLastSyncInfo
* @tc.desc: Test GetCloudSyncInfo with info.stores_ is empty, verify QueryLastSyncInfo
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudSyncManagerTest, DoCloudSync_GetCloudSyncInfo_StoresEmpty_VerifyQueryLastSyncInfo, TestSize.Level1)
{
    CloudData::SyncManager syncManager;
    size_t max = 12;
    size_t min = 5;
    auto executor = std::make_shared<ExecutorPool>(max, min);
    ASSERT_EQ(syncManager.Bind(executor), E_OK);

    int32_t user = cloudInfo_.user;
    CloudData::SyncManager::SyncInfo info(user, TEST_CLOUD_BUNDLE);

    auto ret = syncManager.DoCloudSync(std::move(info));
    ASSERT_EQ(ret, E_OK);
    sleep(1);

    std::vector<Querykey> queryKeys;
    Querykey queryKey;
    queryKey.user = user;
    queryKey.accountId = TEST_CLOUD_ID;
    queryKey.bundleName = TEST_CLOUD_BUNDLE;
    queryKey.storeId = TEST_CLOUD_STORE_1;
    queryKeys.push_back(queryKey);

    auto [status, result] = syncManager.QueryLastSyncInfo(queryKeys);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_EQ(result[TEST_CLOUD_STORE_1].code, E_BLOCKED_BY_NETWORK_STRATEGY);
}

/**
* @tc.name: DoCloudSync_GetCloudSyncInfo_StoresNotEmpty_VerifyQueryLastSyncInfo
* @tc.desc: Test GetCloudSyncInfo with specific store, verify QueryLastSyncInfo
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudSyncManagerTest, DoCloudSync_GetCloudSyncInfo_StoresNotEmpty_VerifyQueryLastSyncInfo, TestSize.Level1)
{
    CloudData::SyncManager syncManager;
    size_t max = 12;
    size_t min = 5;
    auto executor = std::make_shared<ExecutorPool>(max, min);
    ASSERT_EQ(syncManager.Bind(executor), E_OK);
    delegate_.networkType_ = NetworkDelegate::WIFI;
    int32_t user = cloudInfo_.user;
    CloudData::SyncManager::SyncInfo info(user, TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE);

    auto ret = syncManager.DoCloudSync(std::move(info));
    ASSERT_EQ(ret, E_OK);

    sleep(1);

    std::vector<Querykey> queryKeys;
    Querykey queryKey;
    queryKey.user = user;
    queryKey.accountId = TEST_CLOUD_ID;
    queryKey.bundleName = TEST_CLOUD_BUNDLE;
    queryKey.storeId = TEST_CLOUD_STORE;
    queryKeys.push_back(queryKey);

    auto [status, result] = syncManager.QueryLastSyncInfo(queryKeys);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_EQ(result[TEST_CLOUD_STORE].code, E_ERROR);
    delegate_.networkType_ = NetworkDelegate::NONE;
}

/**
* @tc.name: DoCloudSync_GetCloudSyncInfo_StoreNotInSchema_VerifyQueryLastSyncInfo
* @tc.desc: Test GetCloudSyncInfo with store not in schema, verify QueryLastSyncInfo
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudSyncManagerTest, DoCloudSync_GetCloudSyncInfo_StoreNotInSchema_VerifyQueryLastSyncInfo, TestSize.Level1)
{
    std::string lastSyncInfoKey = CloudLastSyncInfo::GetKey(cloudInfo_.user, TEST_CLOUD_BUNDLE, "invalid_store_not_in_schema");
    MetaDataManager::GetInstance().DelMeta(lastSyncInfoKey, true);

    CloudData::SyncManager syncManager;
    size_t max = 12;
    size_t min = 5;
    auto executor = std::make_shared<ExecutorPool>(max, min);
    ASSERT_EQ(syncManager.Bind(executor), E_OK);

    int32_t user = cloudInfo_.user;
    const char* invalidStore = "invalid_store_not_in_schema";
    CloudData::SyncManager::SyncInfo info(user, TEST_CLOUD_BUNDLE, invalidStore);

    auto ret = syncManager.DoCloudSync(std::move(info));
    ASSERT_EQ(ret, E_OK);

    sleep(1);

    std::vector<Querykey> queryKeys;
    Querykey queryKey;
    queryKey.user = user;
    queryKey.accountId = TEST_CLOUD_ID;
    queryKey.bundleName = TEST_CLOUD_BUNDLE;
    queryKey.storeId = invalidStore;
    queryKeys.push_back(queryKey);

    auto [status, result] = syncManager.QueryLastSyncInfo(queryKeys);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_EQ(result.size(), 0);
}

/**
* @tc.name: DoCloudSync_ContainsFalse_MultipleStores_VerifyQueryLastSyncInfo
* @tc.desc: Test GetPostEventTask with multiple stores in schema but tables_ only contains one
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudSyncManagerTest, DoCloudSync_ContainsFalse_MultipleStores_VerifyQueryLastSyncInfo, TestSize.Level1)
{
    CloudData::SyncManager syncManager;
    size_t max = 12;
    size_t min = 5;
    auto executor = std::make_shared<ExecutorPool>(max, min);
    ASSERT_EQ(syncManager.Bind(executor), E_OK);

    delegate_.networkType_ = NetworkDelegate::WIFI;
    int32_t user = cloudInfo_.user;
    CloudData::SyncManager::SyncInfo info(user, TEST_CLOUD_BUNDLE);
    std::vector<std::string> tables;
    info.tables_.insert_or_assign(TEST_CLOUD_STORE, tables);

    auto ret = syncManager.DoCloudSync(std::move(info));
    ASSERT_EQ(ret, E_OK);

    sleep(1);

    std::vector<Querykey> queryKeys;
    Querykey queryKey;
    queryKey.user = user;
    queryKey.accountId = TEST_CLOUD_ID;
    queryKey.bundleName = TEST_CLOUD_BUNDLE;
    queryKey.storeId = TEST_CLOUD_STORE;
    queryKeys.push_back(queryKey);

    auto [status, result] = syncManager.QueryLastSyncInfo(queryKeys);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_EQ(result[TEST_CLOUD_STORE].code, E_ERROR);
    delegate_.networkType_ = NetworkDelegate::NONE;
}
} // namespace DistributedDataTest
} // namespace OHOS::Test