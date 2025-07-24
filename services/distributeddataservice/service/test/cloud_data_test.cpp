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
#define LOG_TAG "CloudDataTest"
#include <gtest/gtest.h>
#include <unistd.h>

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "bootstrap.h"
#include "checker_mock.h"
#include "cloud/cloud_mark.h"
#include "cloud/change_event.h"
#include "cloud/cloud_event.h"
#include "cloud/cloud_last_sync_info.h"
#include "cloud/cloud_report.h"
#include "cloud/cloud_server.h"
#include "cloud/cloud_share_event.h"
#include "cloud/make_query_event.h"
#include "cloud/schema_meta.h"
#include "cloud_data_translate.h"
#include "cloud_service_impl.h"
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
#include "mock/db_store_mock.h"
#include "mock/general_store_mock.h"
#include "network/network_delegate.h"
#include "network_delegate_mock.h"
#include "rdb_query.h"
#include "rdb_service.h"
#include "rdb_service_impl.h"
#include "rdb_types.h"
#include "store/auto_cache.h"
#include "store/general_value.h"
#include "store/store_info.h"
#include "sync_manager.h"
#include "token_setproc.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Querykey = OHOS::CloudData::QueryKey;
using QueryLastResults = OHOS::CloudData::QueryLastResults;
using CloudSyncInfo = OHOS::CloudData::CloudSyncInfo;
using SharingCfm = OHOS::CloudData::SharingUtil::SharingCfm;
using Confirmation = OHOS::CloudData::Confirmation;
using CenterCode = OHOS::DistributedData::SharingCenter::SharingCode;
using Status = OHOS::CloudData::CloudService::Status;
using CloudSyncScene = OHOS::CloudData::CloudServiceImpl::CloudSyncScene;
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
static constexpr const int32_t SCHEMA_VERSION = 101;
static constexpr const int32_t EVT_USER = 102;
static constexpr const char *TEST_TRACE_ID = "123456789";
static constexpr const char *TEST_CLOUD_BUNDLE = "test_cloud_bundleName";
static constexpr const char *TEST_CLOUD_APPID = "test_cloud_appid";
static constexpr const char *TEST_CLOUD_STORE = "test_cloud_store";
static constexpr const char *TEST_CLOUD_STORE_1 = "test_cloud_store1";
static constexpr const char *TEST_CLOUD_ID = "test_cloud_id";
static constexpr const char *TEST_CLOUD_TABLE = "teat_cloud_table";
static constexpr const char *COM_EXAMPLE_TEST_CLOUD = "com.example.testCloud";
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
class CloudDataTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static SchemaMeta schemaMeta_;
    static std::shared_ptr<CloudData::CloudServiceImpl> cloudServiceImpl_;

protected:
    static void InitMetaData();
    static void InitSchemaMeta();
    static void InitCloudInfo();
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static StoreMetaData metaData_;
    static CloudInfo cloudInfo_;
    static DistributedData::CheckerMock checker_;
    static NetworkDelegateMock delegate_;
};

class CloudServerMock : public CloudServer {
public:
    std::pair<int32_t, CloudInfo> GetServerInfo(int32_t userId, bool needSpaceInfo) override;
    std::pair<int32_t, SchemaMeta> GetAppSchema(int32_t userId, const std::string &bundleName) override;
    virtual ~CloudServerMock() = default;
    static constexpr uint64_t REMAINSPACE = 1000;
    static constexpr uint64_t TATALSPACE = 2000;
    static constexpr int32_t INVALID_USER_ID = -1;
};

std::pair<int32_t, CloudInfo> CloudServerMock::GetServerInfo(int32_t userId, bool needSpaceInfo)
{
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

std::pair<int32_t, SchemaMeta> CloudServerMock::GetAppSchema(int32_t userId, const std::string &bundleName)
{
    if (userId == INVALID_USER_ID) {
        return { E_ERROR, CloudDataTest::schemaMeta_ };
    }

    if (bundleName.empty()) {
        SchemaMeta schemaMeta;
        return { E_OK, schemaMeta };
    }
    return { E_OK, CloudDataTest::schemaMeta_ };
}

std::shared_ptr<DBStoreMock> CloudDataTest::dbStoreMock_ = std::make_shared<DBStoreMock>();
SchemaMeta CloudDataTest::schemaMeta_;
StoreMetaData CloudDataTest::metaData_;
CloudInfo CloudDataTest::cloudInfo_;
std::shared_ptr<CloudData::CloudServiceImpl> CloudDataTest::cloudServiceImpl_ =
    std::make_shared<CloudData::CloudServiceImpl>();
DistributedData::CheckerMock CloudDataTest::checker_;
NetworkDelegateMock CloudDataTest::delegate_;

void CloudDataTest::InitMetaData()
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

void CloudDataTest::InitSchemaMeta()
{
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

    schemaMeta_.version = 1;
    schemaMeta_.bundleName = TEST_CLOUD_BUNDLE;
    schemaMeta_.databases.emplace_back(database);
    database.alias = TEST_CLOUD_DATABASE_ALIAS_2;
    schemaMeta_.databases.emplace_back(database);
    schemaMeta_.e2eeEnable = false;
}

void CloudDataTest::InitCloudInfo()
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

void CloudDataTest::SetUpTestCase(void)
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
        { "CloudDataTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    Bootstrap::GetInstance().LoadCheckers();
    auto dmExecutor = std::make_shared<ExecutorPool>(max, min);
    DeviceManagerAdapter::GetInstance().Init(dmExecutor);
    NetworkDelegate::RegisterNetworkInstance(&delegate_);
    InitCloudInfo();
    InitMetaData();
    InitSchemaMeta();
}

void CloudDataTest::TearDownTestCase()
{
    SetSelfTokenID(g_selfTokenID);
}

void CloudDataTest::SetUp()
{
    MetaDataManager::GetInstance().SaveMeta(cloudInfo_.GetKey(), cloudInfo_, true);
    MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true);
    StoreMetaMapping storeMetaMapping(metaData_);
    MetaDataManager::GetInstance().SaveMeta(storeMetaMapping.GetKey(), storeMetaMapping, true);
    MetaDataManager::GetInstance().SaveMeta(cloudInfo_.GetSchemaKey(TEST_CLOUD_BUNDLE), schemaMeta_, true);
}

void CloudDataTest::TearDown()
{
    EventCenter::GetInstance().Unsubscribe(CloudEvent::LOCAL_CHANGE);
    MetaDataManager::GetInstance().DelMeta(cloudInfo_.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true);
    StoreMetaMapping storeMetaMapping(metaData_);
    MetaDataManager::GetInstance().DelMeta(storeMetaMapping.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(cloudInfo_.GetSchemaKey(TEST_CLOUD_BUNDLE), true);
}

/**
* @tc.name: GetSchema
* @tc.desc: GetSchema from cloud when no schema in meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(CloudDataTest, GetSchema, TestSize.Level1)
{
    auto cloudServerMock = std::make_shared<CloudServerMock>();
    auto user = AccountDelegate::GetInstance()->GetUserByToken(OHOS::IPCSkeleton::GetCallingTokenID());
    auto [status, cloudInfo] = cloudServerMock->GetServerInfo(user, true);
    ASSERT_TRUE(MetaDataManager::GetInstance().DelMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), true));
    SchemaMeta schemaMeta;
    ASSERT_FALSE(MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), schemaMeta, true));
    StoreInfo storeInfo{ OHOS::IPCSkeleton::GetCallingTokenID(), TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE, 0 };
    auto event = std::make_unique<CloudEvent>(CloudEvent::GET_SCHEMA, storeInfo);
    EventCenter::GetInstance().PostEvent(std::move(event));
    auto ret = MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), schemaMeta, true);
    ASSERT_TRUE(ret);
}

/**
* @tc.name: QueryStatistics
* @tc.desc: The query interface failed because cloudInfo could not be found from the metadata.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudDataTest, QueryStatistics001, TestSize.Level0)
{
    // prepare MetaDta
    MetaDataManager::GetInstance().DelMeta(cloudInfo_.GetKey(), true);

    auto [status, result] = cloudServiceImpl_->QueryStatistics(TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, "");
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
    EXPECT_TRUE(result.empty());
}

/**
* @tc.name: QueryStatistics
* @tc.desc: The query interface failed because SchemaMeta could not be found from the metadata.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudDataTest, QueryStatistics002, TestSize.Level0)
{
    // prepare MetaDta
    MetaDataManager::GetInstance().DelMeta(cloudInfo_.GetSchemaKey(TEST_CLOUD_BUNDLE), true);
    MetaDataManager::GetInstance().SaveMeta(cloudInfo_.GetKey(), cloudInfo_, true);

    auto [status, result] = cloudServiceImpl_->QueryStatistics("", "", "");
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
    EXPECT_TRUE(result.empty());
    std::tie(status, result) = cloudServiceImpl_->QueryStatistics(TEST_CLOUD_ID, "", "");
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
    EXPECT_TRUE(result.empty());
    std::tie(status, result) = cloudServiceImpl_->QueryStatistics(TEST_CLOUD_ID, TEST_CLOUD_STORE, "");
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
    EXPECT_TRUE(result.empty());
    std::tie(status, result) = cloudServiceImpl_->QueryStatistics(TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, "");
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
    EXPECT_TRUE(result.empty());
}

/**
* @tc.name: QueryStatistics
* @tc.desc: Query the statistics of cloud records in a specified database.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudDataTest, QueryStatistics003, TestSize.Level1)
{
    // Construct the statisticInfo data
    auto creator = [](const StoreMetaData &metaData) -> GeneralStore* {
        auto store = new (std::nothrow) GeneralStoreMock();
        if (store != nullptr) {
            std::map<std::string, Value> entry = { { "inserted", 1 }, { "updated", 2 }, { "normal", 3 } };
            store->MakeCursor(entry);
            store->SetEqualIdentifier("", "");
        }
        return store;
    };
    AutoCache::GetInstance().RegCreator(DistributedRdb::RDB_DEVICE_COLLABORATION, creator);

    auto [status, result] =
        cloudServiceImpl_->QueryStatistics(TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, TEST_CLOUD_DATABASE_ALIAS_1);
    ASSERT_EQ(status, CloudData::CloudService::SUCCESS);
    ASSERT_EQ(result.size(), 1);
    for (const auto &it : result) {
        ASSERT_EQ(it.first, TEST_CLOUD_DATABASE_ALIAS_1);
        auto statisticInfos = it.second;
        ASSERT_FALSE(statisticInfos.empty());
        for (const auto &info : statisticInfos) {
            EXPECT_EQ(info.inserted, 1);
            EXPECT_EQ(info.updated, 2);
            EXPECT_EQ(info.normal, 3);
        }
    }
}

/**
* @tc.name: QueryStatistics
* @tc.desc: Query the statistics of all local database cloud records.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudDataTest, QueryStatistics004, TestSize.Level1)
{
    // Construct the statisticInfo data
    auto creator = [](const StoreMetaData &metaData) -> GeneralStore* {
        auto store = new (std::nothrow) GeneralStoreMock();
        if (store != nullptr) {
            std::map<std::string, Value> entry = { { "inserted", 1 }, { "updated", 2 }, { "normal", 3 } };
            store->MakeCursor(entry);
        }
        return store;
    };
    AutoCache::GetInstance().RegCreator(DistributedRdb::RDB_DEVICE_COLLABORATION, creator);

    auto [status, result] = cloudServiceImpl_->QueryStatistics(TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, "");
    ASSERT_EQ(status, CloudData::CloudService::SUCCESS);
    ASSERT_EQ(result.size(), 2);
    for (const auto &it : result) {
        auto statisticInfos = it.second;
        ASSERT_FALSE(statisticInfos.empty());
        for (const auto &info : statisticInfos) {
            EXPECT_EQ(info.inserted, 1);
            EXPECT_EQ(info.updated, 2);
            EXPECT_EQ(info.normal, 3);
        }
    }
}

/**
* @tc.name: QueryLastSyncInfo001
* @tc.desc: The query last sync info interface failed because account is false.
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, QueryLastSyncInfo001, TestSize.Level0)
{
    auto [status, result] =
        cloudServiceImpl_->QueryLastSyncInfo("accountId", TEST_CLOUD_BUNDLE, TEST_CLOUD_DATABASE_ALIAS_1);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_TRUE(result.empty());
}

/**
* @tc.name: QueryLastSyncInfo002
* @tc.desc: The query last sync info interface failed because bundleName is false.
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, QueryLastSyncInfo002, TestSize.Level0)
{
    auto [status, result] =
        cloudServiceImpl_->QueryLastSyncInfo(TEST_CLOUD_ID, "bundleName", TEST_CLOUD_DATABASE_ALIAS_1);
    EXPECT_EQ(status, CloudData::CloudService::Status::INVALID_ARGUMENT);
    EXPECT_TRUE(result.empty());
}

/**
* @tc.name: QueryLastSyncInfo003
* @tc.desc: The query last sync info interface failed because storeId is false.
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, QueryLastSyncInfo003, TestSize.Level0)
{
    auto [status, result] = cloudServiceImpl_->QueryLastSyncInfo(TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, "storeId");
    EXPECT_EQ(status, CloudData::CloudService::INVALID_ARGUMENT);
    EXPECT_TRUE(result.empty());
}

/**
* @tc.name: QueryLastSyncInfo004
* @tc.desc: The query last sync info interface
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, QueryLastSyncInfo004, TestSize.Level1)
{
    auto ret = cloudServiceImpl_->DisableCloud(TEST_CLOUD_ID);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
    cloudServiceImpl_->OnReady(DeviceManagerAdapter::CLOUD_DEVICE_UUID);

    sleep(1);

    auto [status, result] =
        cloudServiceImpl_->QueryLastSyncInfo(TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, TEST_CLOUD_DATABASE_ALIAS_1);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_TRUE(!result.empty());
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_1].code == E_BLOCKED_BY_NETWORK_STRATEGY);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_1].startTime != 0);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_1].finishTime != 0);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_1].syncStatus == 1);
}

/**
* @tc.name: QueryLastSyncInfo005
* @tc.desc: The query last sync info interface
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, QueryLastSyncInfo005, TestSize.Level1)
{
    std::map<std::string, int32_t> switches;
    switches.emplace(TEST_CLOUD_ID, true);
    CloudInfo info;
    MetaDataManager::GetInstance().LoadMeta(cloudInfo_.GetKey(), info, true);
    info.apps[TEST_CLOUD_BUNDLE].cloudSwitch = false;
    MetaDataManager::GetInstance().SaveMeta(info.GetKey(), info, true);
    cloudServiceImpl_->OnReady(DeviceManagerAdapter::CLOUD_DEVICE_UUID);
    sleep(1);

    auto [status, result] =
        cloudServiceImpl_->QueryLastSyncInfo(TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, TEST_CLOUD_DATABASE_ALIAS_1);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_TRUE(!result.empty());
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_1].code == E_BLOCKED_BY_NETWORK_STRATEGY);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_1].startTime != 0);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_1].finishTime != 0);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_1].syncStatus == 1);
}

/**
* @tc.name: QueryLastSyncInfo006
* @tc.desc: The query last sync info interface
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, QueryLastSyncInfo006, TestSize.Level0)
{
    MetaDataManager::GetInstance().DelMeta(cloudInfo_.GetSchemaKey(TEST_CLOUD_BUNDLE), true);
    auto [status, result] =
        cloudServiceImpl_->QueryLastSyncInfo(TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, TEST_CLOUD_DATABASE_ALIAS_1);
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
    EXPECT_TRUE(result.empty());
    SchemaMeta meta;
    meta.bundleName = "test";
    MetaDataManager::GetInstance().SaveMeta(cloudInfo_.GetSchemaKey(TEST_CLOUD_BUNDLE), meta, true);
    std::tie(status, result) =
        cloudServiceImpl_->QueryLastSyncInfo(TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, TEST_CLOUD_DATABASE_ALIAS_1);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_TRUE(result.empty());
}

/**
* @tc.name: QueryLastSyncInfo007
* @tc.desc: The query last sync info interface
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, QueryLastSyncInfo007, TestSize.Level0)
{
    int32_t user = 100;
    int64_t startTime = 123456789;
    int64_t finishTime = 123456799;
    CloudLastSyncInfo lastSyncInfo;
    lastSyncInfo.id = TEST_CLOUD_ID;
    lastSyncInfo.storeId = TEST_CLOUD_DATABASE_ALIAS_1;
    lastSyncInfo.startTime = startTime;
    lastSyncInfo.finishTime = finishTime;
    lastSyncInfo.syncStatus = 1;
    MetaDataManager::GetInstance().SaveMeta(CloudLastSyncInfo::GetKey(user, TEST_CLOUD_BUNDLE,
        TEST_CLOUD_DATABASE_ALIAS_1), lastSyncInfo, true);

    std::vector<CloudLastSyncInfo> lastSyncInfos;
    MetaDataManager::GetInstance().LoadMeta(CloudLastSyncInfo::GetKey(user, TEST_CLOUD_BUNDLE,
        TEST_CLOUD_DATABASE_ALIAS_1), lastSyncInfos, true);
    EXPECT_TRUE(!lastSyncInfos.empty());

    CloudData::SyncManager sync;
    auto [status, result] = sync.QueryLastSyncInfo({ { user, TEST_CLOUD_ID, TEST_CLOUD_BUNDLE,
                                                       TEST_CLOUD_DATABASE_ALIAS_1 } });
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_TRUE(!result.empty());
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_1].code == -1);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_1].startTime == startTime);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_1].finishTime == finishTime);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_1].syncStatus == 1);
}

/**
* @tc.name: QueryLastSyncInfo008
* @tc.desc: The query last sync info interface
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, QueryLastSyncInfo008, TestSize.Level0)
{
    int32_t user = 100;
    int64_t startTime = 123456789;
        int64_t finishTime = 123456799;
    CloudLastSyncInfo lastSyncInfo;
    lastSyncInfo.id = TEST_CLOUD_ID;
    lastSyncInfo.storeId = TEST_CLOUD_DATABASE_ALIAS_1;
    lastSyncInfo.startTime = startTime;
    lastSyncInfo.finishTime = finishTime;
    lastSyncInfo.syncStatus = 1;
    lastSyncInfo.code = 0;
    MetaDataManager::GetInstance().SaveMeta(CloudLastSyncInfo::GetKey(user, TEST_CLOUD_BUNDLE,
        TEST_CLOUD_DATABASE_ALIAS_1), lastSyncInfo, true);
    CloudLastSyncInfo lastSyncInfo1;
    lastSyncInfo1.id = TEST_CLOUD_ID;
    lastSyncInfo1.storeId = TEST_CLOUD_DATABASE_ALIAS_2;
    lastSyncInfo1.startTime = startTime;
    lastSyncInfo1.finishTime = finishTime;
    lastSyncInfo1.syncStatus = 1;
    lastSyncInfo1.code = 0;
    MetaDataManager::GetInstance().SaveMeta(CloudLastSyncInfo::GetKey(user, TEST_CLOUD_BUNDLE,
        TEST_CLOUD_DATABASE_ALIAS_2), lastSyncInfo1, true);

    std::vector<CloudLastSyncInfo> lastSyncInfos;
    MetaDataManager::GetInstance().LoadMeta(CloudLastSyncInfo::GetKey(user, TEST_CLOUD_BUNDLE,
        ""), lastSyncInfos, true);
    EXPECT_TRUE(!lastSyncInfos.empty());

    CloudData::SyncManager sync;
    auto [status, result] = sync.QueryLastSyncInfo({ { user, TEST_CLOUD_ID, TEST_CLOUD_BUNDLE,
        TEST_CLOUD_DATABASE_ALIAS_1 }, { user, TEST_CLOUD_ID, TEST_CLOUD_BUNDLE,
        TEST_CLOUD_DATABASE_ALIAS_2} });

    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_TRUE(result.size() == 2);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_1].code == 0);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_1].startTime == startTime);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_1].finishTime == finishTime);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_1].syncStatus == 1);

    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_2].code == 0);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_2].startTime == startTime);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_2].finishTime == finishTime);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_2].syncStatus == 1);
}

/**
* @tc.name: QueryLastSyncInfo009
* @tc.desc: The query last sync info interface failed when schema is invalid.
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, QueryLastSyncInfo009, TestSize.Level0)
{
    int32_t user = 100;
    int64_t startTime = 123456789;
        int64_t finishTime = 123456799;
    CloudLastSyncInfo lastSyncInfo;
    lastSyncInfo.id = TEST_CLOUD_ID;
    lastSyncInfo.storeId = TEST_CLOUD_DATABASE_ALIAS_1;
    lastSyncInfo.startTime = startTime;
    lastSyncInfo.finishTime = finishTime;
    lastSyncInfo.syncStatus = 1;
    lastSyncInfo.code = 0;
    MetaDataManager::GetInstance().SaveMeta(CloudLastSyncInfo::GetKey(user, TEST_CLOUD_BUNDLE,
        TEST_CLOUD_DATABASE_ALIAS_1), lastSyncInfo, true);
    CloudLastSyncInfo lastSyncInfo1;
    lastSyncInfo1.id = TEST_CLOUD_ID;
    lastSyncInfo1.storeId = TEST_CLOUD_DATABASE_ALIAS_2;
    lastSyncInfo1.startTime = startTime;
    lastSyncInfo1.finishTime = finishTime;
    lastSyncInfo1.syncStatus = 1;
    lastSyncInfo1.code = 0;
    MetaDataManager::GetInstance().SaveMeta(CloudLastSyncInfo::GetKey(user, TEST_CLOUD_BUNDLE,
        TEST_CLOUD_DATABASE_ALIAS_2), lastSyncInfo1, true);

    std::vector<CloudLastSyncInfo> lastSyncInfos;
    MetaDataManager::GetInstance().LoadMeta(CloudLastSyncInfo::GetKey(user, TEST_CLOUD_BUNDLE, ""),
        lastSyncInfos, true);
    EXPECT_TRUE(!lastSyncInfos.empty());

    CloudData::SyncManager sync;
    auto [status, result] = sync.QueryLastSyncInfo({ { user, TEST_CLOUD_ID, TEST_CLOUD_BUNDLE,
                                                       TEST_CLOUD_DATABASE_ALIAS_2} });
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_TRUE(result.size() == 1);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_2].code == 0);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_2].startTime == startTime);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_2].finishTime == finishTime);
    EXPECT_TRUE(result[TEST_CLOUD_DATABASE_ALIAS_2].syncStatus == 1);
}

/**
* @tc.name: QueryLastSyncInfo010
* @tc.desc: The query last sync info interface failed
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, QueryLastSyncInfo010, TestSize.Level0)
{
    int32_t user = 100;
    int64_t startTime = 123456789;
        int64_t finishTime = 123456799;
    CloudLastSyncInfo lastSyncInfo;
    lastSyncInfo.id = TEST_CLOUD_ID;
    lastSyncInfo.storeId = TEST_CLOUD_DATABASE_ALIAS_1;
    lastSyncInfo.startTime = startTime;
    lastSyncInfo.finishTime = finishTime;
    lastSyncInfo.syncStatus = 1;
    lastSyncInfo.code = 0;
    MetaDataManager::GetInstance().SaveMeta(CloudLastSyncInfo::GetKey(user, TEST_CLOUD_BUNDLE,
        TEST_CLOUD_DATABASE_ALIAS_1), lastSyncInfo, true);
    CloudLastSyncInfo lastSyncInfo1;
    lastSyncInfo1.id = TEST_CLOUD_ID;
    lastSyncInfo1.storeId = TEST_CLOUD_DATABASE_ALIAS_2;
    lastSyncInfo1.startTime = startTime;
    lastSyncInfo1.finishTime = finishTime;
    lastSyncInfo1.syncStatus = 1;
    lastSyncInfo1.code = 0;
    MetaDataManager::GetInstance().SaveMeta(CloudLastSyncInfo::GetKey(user, TEST_CLOUD_BUNDLE,
        TEST_CLOUD_DATABASE_ALIAS_2), lastSyncInfo1, true);

    CloudData::SyncManager sync;
    CloudData::SyncManager::SyncInfo info(user, TEST_CLOUD_BUNDLE, TEST_CLOUD_DATABASE_ALIAS_1);
    auto [status, result] = sync.QueryLastSyncInfo({ { user, TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, "1234"} });
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_TRUE(result.empty());
}

/**
* @tc.name: QueryLastSyncInfo011
* @tc.desc: The query last sync info interface
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, QueryLastSyncInfo011, TestSize.Level1)
{
    schemaMeta_.databases[1].name = TEST_CLOUD_STORE_1;
    MetaDataManager::GetInstance().SaveMeta(cloudInfo_.GetSchemaKey(TEST_CLOUD_BUNDLE), schemaMeta_, true);
    int32_t user = 100;
    CloudData::SyncManager sync;
    CloudData::SyncManager::SyncInfo info(user, TEST_CLOUD_BUNDLE, TEST_CLOUD_DATABASE_ALIAS_1);
    info.syncId_ = 0;
    CloudInfo cloud;
    cloud.user = info.user_;
    auto cloudSyncInfos = sync.GetCloudSyncInfo(info, cloud);
    sync.UpdateStartSyncInfo(cloudSyncInfos);
    auto [status, result] = sync.QueryLastSyncInfo({ { user, TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE },
                                           { user, TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE_1} });
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_TRUE(result.size() == 2);
    EXPECT_TRUE(result[TEST_CLOUD_STORE].code == 0);
    EXPECT_TRUE(result[TEST_CLOUD_STORE].startTime != 0);
    EXPECT_TRUE(result[TEST_CLOUD_STORE].finishTime == 0);
    EXPECT_TRUE(result[TEST_CLOUD_STORE].syncStatus == 0);

    EXPECT_TRUE(result[TEST_CLOUD_STORE_1].code == 0);
    EXPECT_TRUE(result[TEST_CLOUD_STORE_1].startTime != 0);
    EXPECT_TRUE(result[TEST_CLOUD_STORE_1].finishTime == 0);
    EXPECT_TRUE(result[TEST_CLOUD_STORE_1].syncStatus == 0);
}

/**
* @tc.name: QueryLastSyncInfo012
* @tc.desc: The query last sync info interface failed.
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, QueryLastSyncInfo012, TestSize.Level0)
{
    int32_t user = 100;
    int64_t startTime = 123456789;
    int64_t finishTime = 123456799;
    CloudLastSyncInfo lastSyncInfo;
    lastSyncInfo.id = TEST_CLOUD_ID;
    lastSyncInfo.storeId = TEST_CLOUD_DATABASE_ALIAS_1;
    lastSyncInfo.startTime = startTime;
    lastSyncInfo.finishTime = finishTime;
    lastSyncInfo.syncStatus = 1;
    MetaDataManager::GetInstance().SaveMeta(CloudLastSyncInfo::GetKey(user, TEST_CLOUD_BUNDLE,
        TEST_CLOUD_DATABASE_ALIAS_1), lastSyncInfo, true);

    std::vector<CloudLastSyncInfo> lastSyncInfos;
    MetaDataManager::GetInstance().LoadMeta(CloudLastSyncInfo::GetKey(user, TEST_CLOUD_BUNDLE,
         TEST_CLOUD_DATABASE_ALIAS_1), lastSyncInfos, true);
    EXPECT_TRUE(!lastSyncInfos.empty());

    CloudData::SyncManager sync;
    CloudData::SyncManager::SyncInfo info(user, TEST_CLOUD_BUNDLE, TEST_CLOUD_DATABASE_ALIAS_1);
    auto [status, result] = sync.QueryLastSyncInfo({ { user, TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, "1234"} });
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_TRUE(result.empty());
}

/**
* @tc.name: GetStores
* @tc.desc: Test GetStores function
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, GetStores, TestSize.Level0)
{
    auto cloudServerMock = std::make_shared<CloudServerMock>();
    auto user = AccountDelegate::GetInstance()->GetUserByToken(OHOS::IPCSkeleton::GetCallingTokenID());
    auto [status, cloudInfo] = cloudServerMock->GetServerInfo(user, true);
    SchemaMeta schemaMeta;
    MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), schemaMeta, true);
    EXPECT_TRUE(!schemaMeta.GetStores().empty());
}

/**
* @tc.name: UpdateStartSyncInfo
* @tc.desc: Test UpdateStartSyncInfo
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, UpdateStartSyncInfo, TestSize.Level0)
{
    int32_t user = 100;
    CloudData::SyncManager sync;
    CloudData::SyncManager::SyncInfo info(user, TEST_CLOUD_BUNDLE, TEST_CLOUD_DATABASE_ALIAS_1);
    CloudInfo cloud;
    cloud.user = info.user_;
    auto cloudSyncInfos = sync.GetCloudSyncInfo(info, cloud);
    sync.UpdateStartSyncInfo(cloudSyncInfos);
    std::vector<CloudLastSyncInfo> lastSyncInfos;
    MetaDataManager::GetInstance().LoadMeta(CloudLastSyncInfo::GetKey(user, TEST_CLOUD_BUNDLE,
         ""), lastSyncInfos, true);
    EXPECT_TRUE(!lastSyncInfos.empty());
    printf("code: %d", lastSyncInfos[0].code);
    EXPECT_TRUE(lastSyncInfos[0].code == -1);
    EXPECT_TRUE(lastSyncInfos[0].startTime != 0);
    EXPECT_TRUE(lastSyncInfos[0].finishTime != 0);
    EXPECT_TRUE(lastSyncInfos[0].syncStatus == 1);
}

/**
* @tc.name: UpdateStartSyncInfo
* @tc.desc: Test UpdateStartSyncInfo
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, UpdateFinishSyncInfo, TestSize.Level0)
{
    int32_t user = 100;
    CloudData::SyncManager sync;
    CloudData::SyncManager::SyncInfo info(user, TEST_CLOUD_BUNDLE, TEST_CLOUD_DATABASE_ALIAS_1);
    CloudInfo cloud;
    cloud.user = info.user_;
    auto cloudSyncInfos = sync.GetCloudSyncInfo(info, cloud);
    sync.UpdateStartSyncInfo(cloudSyncInfos);
    sync.UpdateFinishSyncInfo({ user, TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, TEST_CLOUD_DATABASE_ALIAS_1 }, 0, 0);
    std::vector<CloudLastSyncInfo> lastSyncInfos;
    MetaDataManager::GetInstance().LoadMeta(CloudLastSyncInfo::GetKey(user, TEST_CLOUD_BUNDLE,
         ""), lastSyncInfos, true);
    EXPECT_TRUE(!lastSyncInfos.empty());
}

/**
* @tc.name: Share
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, Share001, TestSize.Level1)
{
    std::string sharingRes = "";
    CloudData::Participants participants{};
    CloudData::Results results;
    auto ret = cloudServiceImpl_->Share(sharingRes, participants, results);
    EXPECT_EQ(ret, CloudData::CloudService::NOT_SUPPORT);
}

/**
* @tc.name: Unshare
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, Unshare001, TestSize.Level1)
{
    std::string sharingRes = "";
    CloudData::Participants participants{};
    CloudData::Results results;
    auto ret = cloudServiceImpl_->Unshare(sharingRes, participants, results);
    EXPECT_EQ(ret, CloudData::CloudService::NOT_SUPPORT);
}

/**
* @tc.name: ChangePrivilege
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, ChangePrivilege001, TestSize.Level1)
{
    std::string sharingRes = "";
    CloudData::Participants participants{};
    CloudData::Results results;
    auto ret = cloudServiceImpl_->ChangePrivilege(sharingRes, participants, results);
    EXPECT_EQ(ret, CloudData::CloudService::NOT_SUPPORT);
}

/**
* @tc.name: ChangeConfirmation
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, ChangeConfirmation001, TestSize.Level1)
{
    std::string sharingRes = "";
    int32_t confirmation = 0;
    std::pair<int32_t, std::string> result;
    auto ret = cloudServiceImpl_->ChangeConfirmation(sharingRes, confirmation, result);
    EXPECT_EQ(ret, CloudData::CloudService::NOT_SUPPORT);
}

/**
* @tc.name: ConfirmInvitation
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, ConfirmInvitation001, TestSize.Level1)
{
    std::string sharingRes = "";
    int32_t confirmation = 0;
    std::tuple<int32_t, std::string, std::string> result;
    auto ret = cloudServiceImpl_->ConfirmInvitation(sharingRes, confirmation, result);
    EXPECT_EQ(ret, CloudData::CloudService::NOT_SUPPORT);
}

/**
* @tc.name: Exit
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, Exit001, TestSize.Level1)
{
    std::string sharingRes = "";
    std::pair<int32_t, std::string> result;
    auto ret = cloudServiceImpl_->Exit(sharingRes, result);
    EXPECT_EQ(ret, CloudData::CloudService::NOT_SUPPORT);
}

/**
* @tc.name: Query
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, Query001, TestSize.Level1)
{
    std::string sharingRes = "";
    CloudData::QueryResults result;
    auto ret = cloudServiceImpl_->Query(sharingRes, result);
    EXPECT_EQ(ret, CloudData::CloudService::NOT_SUPPORT);
}

/**
* @tc.name: QueryByInvitation
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, QueryByInvitation001, TestSize.Level1)
{
    std::string invitation = "";
    CloudData::QueryResults result;
    auto ret = cloudServiceImpl_->QueryByInvitation(invitation, result);
    EXPECT_EQ(ret, CloudData::CloudService::NOT_SUPPORT);
}

/**
* @tc.name: AllocResourceAndShare
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, AllocResourceAndShare001, TestSize.Level0)
{
    DistributedRdb::PredicatesMemo predicates;
    predicates.tables_.push_back(TEST_CLOUD_BUNDLE);
    std::vector<std::string> columns;
    CloudData::Participants participants;
    auto [ret, _] = cloudServiceImpl_->AllocResourceAndShare(TEST_CLOUD_STORE, predicates, columns, participants);
    EXPECT_EQ(ret, E_ERROR);
    EventCenter::GetInstance().Subscribe(CloudEvent::MAKE_QUERY, [](const Event &event) {
        auto &evt = static_cast<const DistributedData::MakeQueryEvent &>(event);
        auto callback = evt.GetCallback();
        if (!callback) {
            return;
        }
        auto predicate = evt.GetPredicates();
        auto rdbQuery = std::make_shared<DistributedRdb::RdbQuery>();
        rdbQuery->MakeQuery(*predicate);
        rdbQuery->SetColumns(evt.GetColumns());
        callback(rdbQuery);
    });
    std::tie(ret, _) = cloudServiceImpl_->AllocResourceAndShare(TEST_CLOUD_STORE, predicates, columns, participants);
    EXPECT_EQ(ret, E_ERROR);
}

/**
* @tc.name: SetGlobalCloudStrategy
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, SetGlobalCloudStrategy001, TestSize.Level1)
{
    std::vector<CommonType::Value> values;
    values.push_back(CloudData::NetWorkStrategy::WIFI);
    CloudData::Strategy strategy = CloudData::Strategy::STRATEGY_BUTT;
    auto ret = cloudServiceImpl_->SetGlobalCloudStrategy(strategy, values);
    EXPECT_EQ(ret, CloudData::CloudService::INVALID_ARGUMENT);
    strategy = CloudData::Strategy::STRATEGY_NETWORK;
    ret = cloudServiceImpl_->SetGlobalCloudStrategy(strategy, values);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
* @tc.name: SetCloudStrategy
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, SetCloudStrategy001, TestSize.Level1)
{
    std::vector<CommonType::Value> values;
    values.push_back(CloudData::NetWorkStrategy::WIFI);
    CloudData::Strategy strategy = CloudData::Strategy::STRATEGY_BUTT;
    auto ret = cloudServiceImpl_->SetCloudStrategy(strategy, values);
    EXPECT_EQ(ret, CloudData::CloudService::INVALID_ARGUMENT);
    strategy = CloudData::Strategy::STRATEGY_NETWORK;
    ret = cloudServiceImpl_->SetCloudStrategy(strategy, values);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
* @tc.name: CloudSync001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, CloudSync001, TestSize.Level0)
{
    int32_t syncMode = DistributedData::GeneralStore::NEARBY_BEGIN;
    uint32_t seqNum = 10;
    // invalid syncMode
    auto ret = cloudServiceImpl_->CloudSync("bundleName", "storeId", { syncMode, seqNum }, nullptr);
    EXPECT_EQ(ret, CloudData::CloudService::INVALID_ARGUMENT);
}

/**
* @tc.name: CloudSync002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, CloudSync002, TestSize.Level0)
{
    int32_t syncMode = DistributedData::GeneralStore::NEARBY_PULL_PUSH;
    uint32_t seqNum = 10;
    // invalid syncMode
    auto ret = cloudServiceImpl_->CloudSync("bundleName", "storeId", { syncMode, seqNum }, nullptr);
    EXPECT_EQ(ret, CloudData::CloudService::INVALID_ARGUMENT);
}

/**
* @tc.name: CloudSync003
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, CloudSync003, TestSize.Level1)
{
    int32_t syncMode = DistributedData::GeneralStore::CLOUD_BEGIN;
    uint32_t seqNum = 10;
    auto ret = cloudServiceImpl_->CloudSync("bundleName", "storeId", { syncMode, seqNum }, nullptr);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
* @tc.name: CloudSync004
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, CloudSync004, TestSize.Level1)
{
    int32_t syncMode = DistributedData::GeneralStore::CLOUD_TIME_FIRST;
    uint32_t seqNum = 10;
    auto ret = cloudServiceImpl_->CloudSync("bundleName", "storeId", { syncMode, seqNum }, nullptr);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
* @tc.name: CloudSync005
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, CloudSync005, TestSize.Level1)
{
    int32_t syncMode = DistributedData::GeneralStore::CLOUD_NATIVE_FIRST;
    uint32_t seqNum = 10;
    auto ret = cloudServiceImpl_->CloudSync("bundleName", "storeId", { syncMode, seqNum }, nullptr);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
* @tc.name: CloudSync006
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, CloudSync006, TestSize.Level1)
{
    int32_t syncMode = DistributedData::GeneralStore::CLOUD_CLOUD_FIRST;
    uint32_t seqNum = 10;
    auto ret = cloudServiceImpl_->CloudSync("bundleName", "storeId", { syncMode, seqNum }, nullptr);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
* @tc.name: CloudSync007
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, CloudSync007, TestSize.Level0)
{
    int32_t syncMode = DistributedData::GeneralStore::CLOUD_END;
    uint32_t seqNum = 10;
    auto ret = cloudServiceImpl_->CloudSync("bundleName", "storeId", { syncMode, seqNum }, nullptr);
    EXPECT_EQ(ret, CloudData::CloudService::INVALID_ARGUMENT);
}

/**
* @tc.name: InitNotifier001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, InitNotifier001, TestSize.Level0)
{
    sptr<IRemoteObject> notifier = nullptr;
    auto ret = cloudServiceImpl_->InitNotifier(notifier);
    EXPECT_EQ(ret, CloudData::CloudService::INVALID_ARGUMENT);

    ret = cloudServiceImpl_->InitNotifier(notifier);
    EXPECT_EQ(ret, CloudData::CloudService::INVALID_ARGUMENT);
}

/**
* @tc.name: Clean
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, Clean001, TestSize.Level0)
{
    std::map<std::string, int32_t> actions;
    actions.insert_or_assign(TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_BUTT);
    std::string id = "testId";
    std::string bundleName = "testBundleName";
    auto ret = cloudServiceImpl_->Clean(id, actions);
    EXPECT_EQ(ret, CloudData::CloudService::ERROR);
    ret = cloudServiceImpl_->Clean(TEST_CLOUD_ID, actions);
    EXPECT_EQ(ret, CloudData::CloudService::ERROR);
    actions.insert_or_assign(TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_INFO);
    actions.insert_or_assign(bundleName, CloudData::CloudService::Action::CLEAR_CLOUD_DATA_AND_INFO);
    ret = cloudServiceImpl_->Clean(TEST_CLOUD_ID, actions);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
* @tc.name: Clean
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, Clean002, TestSize.Level0)
{
    MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true);
    std::map<std::string, int32_t> actions;
    actions.insert_or_assign(TEST_CLOUD_BUNDLE, CloudData::CloudService::Action::CLEAR_CLOUD_INFO);
    auto ret = cloudServiceImpl_->Clean(TEST_CLOUD_ID, actions);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
    StoreMetaDataLocal localMeta;
    localMeta.isPublic = false;
    MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyLocal(), localMeta, true);
    ret = cloudServiceImpl_->Clean(TEST_CLOUD_ID, actions);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
    localMeta.isPublic = true;
    MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyLocal(), localMeta, true);
    ret = cloudServiceImpl_->Clean(TEST_CLOUD_ID, actions);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
    metaData_.user = "0";
    MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true);
    ret = cloudServiceImpl_->Clean(TEST_CLOUD_ID, actions);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
    MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true);
    metaData_.user = std::to_string(AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId));
    MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true);
}

/**
* @tc.name: NotifyDataChange
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, NotifyDataChange001, TestSize.Level0)
{
    auto ret = cloudServiceImpl_->NotifyDataChange(TEST_CLOUD_ID, TEST_CLOUD_BUNDLE);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
* @tc.name: NotifyDataChange
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, NotifyDataChange002, TestSize.Level0)
{
    constexpr const int32_t invalidUserId = -1;
    std::string extraData;
    auto ret = cloudServiceImpl_->NotifyDataChange("", extraData, invalidUserId);
    EXPECT_EQ(ret, CloudData::CloudService::INVALID_ARGUMENT);
    ret = cloudServiceImpl_->NotifyDataChange(CloudData::DATA_CHANGE_EVENT_ID, extraData, invalidUserId);
    EXPECT_EQ(ret, CloudData::CloudService::INVALID_ARGUMENT);
    extraData = "{data:test}";
    ret = cloudServiceImpl_->NotifyDataChange(CloudData::DATA_CHANGE_EVENT_ID, extraData, invalidUserId);
    EXPECT_EQ(ret, CloudData::CloudService::INVALID_ARGUMENT);
    extraData = "{\"data\":\"{\\\"accountId\\\":\\\"id\\\",\\\"bundleName\\\":\\\"test_cloud_"
                "bundleName\\\",\\\"containerName\\\":\\\"test_cloud_database_alias_1\\\", \\\"databaseScopes\\\": "
                "\\\"[\\\\\\\"private\\\\\\\", "
                "\\\\\\\"shared\\\\\\\"]\\\",\\\"recordTypes\\\":\\\"[\\\\\\\"test_cloud_table_alias\\\\\\\"]\\\"}\"}";
    ret = cloudServiceImpl_->NotifyDataChange(CloudData::DATA_CHANGE_EVENT_ID, extraData, invalidUserId);
    EXPECT_EQ(ret, CloudData::CloudService::INVALID_ARGUMENT);
    extraData = "{\"data\":\"{\\\"accountId\\\":\\\"test_cloud_id\\\",\\\"bundleName\\\":\\\"cloud_"
                "bundleName_test\\\",\\\"containerName\\\":\\\"test_cloud_database_alias_1\\\", "
                "\\\"databaseScopes\\\": "
                "\\\"[\\\\\\\"private\\\\\\\", "
                "\\\\\\\"shared\\\\\\\"]\\\",\\\"recordTypes\\\":\\\"[\\\\\\\"test_cloud_table_alias\\\\\\\"]\\\"}\"}";
    ret = cloudServiceImpl_->NotifyDataChange(CloudData::DATA_CHANGE_EVENT_ID, extraData, invalidUserId);
    EXPECT_EQ(ret, CloudData::CloudService::INVALID_ARGUMENT);
}

/**
* @tc.name: NotifyDataChange
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, NotifyDataChange003, TestSize.Level1)
{
    constexpr const int32_t userId = 100;
    constexpr const int32_t defaultUserId = 0;
    std::string extraData = "{\"data\":\"{\\\"accountId\\\":\\\"test_cloud_id\\\",\\\"bundleName\\\":\\\"test_cloud_"
                            "bundleName\\\",\\\"containerName\\\":\\\"test_cloud_database_alias_1\\\", "
                            "\\\"databaseScopes\\\": "
                            "\\\"[\\\\\\\"private\\\\\\\", "
                            "\\\\\\\"shared\\\\\\\"]\\\",\\\"recordTypes\\\":\\\"[\\\\\\\"\\\\\\\"]\\\"}\"}";
    auto ret = cloudServiceImpl_->NotifyDataChange(CloudData::DATA_CHANGE_EVENT_ID, extraData, defaultUserId);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
    extraData = "{\"data\":\"{\\\"accountId\\\":\\\"test_cloud_id\\\",\\\"bundleName\\\":\\\"test_cloud_"
                "bundleName\\\",\\\"containerName\\\":\\\"test_cloud_database_alias_1\\\", \\\"databaseScopes\\\": "
                "\\\"[\\\\\\\"private\\\\\\\", "
                "\\\\\\\"shared\\\\\\\"]\\\",\\\"recordTypes\\\":\\\"[\\\\\\\"test_cloud_table_alias\\\\\\\"]\\\"}\"}";
    ret = cloudServiceImpl_->NotifyDataChange(CloudData::DATA_CHANGE_EVENT_ID, extraData, userId);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
* @tc.name: OnReady
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnReady001, TestSize.Level0)
{
    std::string device = "test";
    auto ret = cloudServiceImpl_->OnReady(device);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
    ret = cloudServiceImpl_->OnReady(DeviceManagerAdapter::CLOUD_DEVICE_UUID);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
* @tc.name: Offline
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, Offline001, TestSize.Level0)
{
    std::string device = "test";
    auto ret = cloudServiceImpl_->Offline(device);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
    ret = cloudServiceImpl_->Offline(DeviceManagerAdapter::CLOUD_DEVICE_UUID);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
* @tc.name: CloudShare
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, CloudShare001, TestSize.Level0)
{
    StoreInfo storeInfo{ OHOS::IPCSkeleton::GetCallingTokenID(), TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE, 0 };
    std::pair<int32_t, std::shared_ptr<Cursor>> result;
    CloudShareEvent::Callback asyncCallback = [&result](int32_t status, std::shared_ptr<Cursor> cursor) {
        result.first = status;
        result.second = cursor;
    };
    auto event = std::make_unique<CloudShareEvent>(storeInfo, nullptr, nullptr);
    EventCenter::GetInstance().PostEvent(std::move(event));
    auto event1 = std::make_unique<CloudShareEvent>(storeInfo, nullptr, asyncCallback);
    EventCenter::GetInstance().PostEvent(std::move(event1));
    EXPECT_EQ(result.first, GeneralError::E_ERROR);
    auto rdbQuery = std::make_shared<DistributedRdb::RdbQuery>();
    auto event2 = std::make_unique<CloudShareEvent>(storeInfo, rdbQuery, nullptr);
    EventCenter::GetInstance().PostEvent(std::move(event2));
    auto event3 = std::make_unique<CloudShareEvent>(storeInfo, rdbQuery, asyncCallback);
    EventCenter::GetInstance().PostEvent(std::move(event3));
    EXPECT_EQ(result.first, GeneralError::E_ERROR);
}

/**
* @tc.name: OnUserChange
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnUserChange001, TestSize.Level0)
{
    constexpr const uint32_t ACCOUNT_DEFAULT = 2;
    constexpr const uint32_t ACCOUNT_DELETE = 3;
    constexpr const uint32_t ACCOUNT_SWITCHED = 4;
    constexpr const uint32_t ACCOUNT_UNLOCKED = 5;
    auto ret = cloudServiceImpl_->OnUserChange(ACCOUNT_DEFAULT, "0", "test");
    EXPECT_EQ(ret, GeneralError::E_OK);
    ret = cloudServiceImpl_->OnUserChange(ACCOUNT_DELETE, "0", "test");
    EXPECT_EQ(ret, GeneralError::E_OK);
    ret = cloudServiceImpl_->OnUserChange(ACCOUNT_SWITCHED, "0", "test");
    EXPECT_EQ(ret, GeneralError::E_OK);
    ret = cloudServiceImpl_->OnUserChange(ACCOUNT_UNLOCKED, "0", "test");
    EXPECT_EQ(ret, GeneralError::E_OK);
}

/**
* @tc.name: DisableCloud
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, DisableCloud001, TestSize.Level0)
{
    auto ret = cloudServiceImpl_->DisableCloud("test");
    EXPECT_EQ(ret, CloudData::CloudService::INVALID_ARGUMENT);
    ret = cloudServiceImpl_->DisableCloud(TEST_CLOUD_ID);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
* @tc.name: ChangeAppSwitch
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, ChangeAppSwitch, TestSize.Level0)
{
    std::string id = "testId";
    std::string bundleName = "testName";
    auto ret = cloudServiceImpl_->ChangeAppSwitch(id, bundleName, CloudData::CloudService::SWITCH_ON);
    EXPECT_EQ(ret, CloudData::CloudService::INVALID_ARGUMENT);
    ret = cloudServiceImpl_->ChangeAppSwitch(TEST_CLOUD_ID, bundleName, CloudData::CloudService::SWITCH_ON);
    EXPECT_EQ(ret, CloudData::CloudService::INVALID_ARGUMENT);
    ret = cloudServiceImpl_->ChangeAppSwitch(TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, CloudData::CloudService::SWITCH_OFF);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
* @tc.name: EnableCloud
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, EnableCloud, TestSize.Level0)
{
    std::string bundleName = "testName";
    std::map<std::string, int32_t> switches;
    switches.insert_or_assign(TEST_CLOUD_BUNDLE, CloudData::CloudService::SWITCH_ON);
    switches.insert_or_assign(bundleName, CloudData::CloudService::SWITCH_ON);
    auto ret = cloudServiceImpl_->EnableCloud(TEST_CLOUD_ID, switches);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
}

/**
* @tc.name: OnEnableCloud
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnEnableCloud, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_ENABLE_CLOUD, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    std::string id = "testId";
    std::map<std::string, int32_t> switches;
    ITypesUtil::Marshal(data, id, switches);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_ENABLE_CLOUD, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnDisableCloud
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnDisableCloud, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_DISABLE_CLOUD, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    data.WriteString(TEST_CLOUD_ID);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_DISABLE_CLOUD, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnChangeAppSwitch
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnChangeAppSwitch, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_CHANGE_APP_SWITCH, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    data.WriteString(TEST_CLOUD_ID);
    data.WriteString(TEST_CLOUD_BUNDLE);
    data.WriteInt32(CloudData::CloudService::SWITCH_ON);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_CHANGE_APP_SWITCH, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnClean
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnClean, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_CLEAN, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    std::string id = TEST_CLOUD_ID;
    std::map<std::string, int32_t> actions;
    ITypesUtil::Marshal(data, id, actions);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_CLEAN, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnNotifyDataChange
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnNotifyDataChange, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_NOTIFY_DATA_CHANGE, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    data.WriteString(TEST_CLOUD_ID);
    data.WriteString(TEST_CLOUD_BUNDLE);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_NOTIFY_DATA_CHANGE, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnNotifyChange
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnNotifyChange, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_NOTIFY_DATA_CHANGE_EXT, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    data.WriteString(TEST_CLOUD_ID);
    data.WriteString(TEST_CLOUD_BUNDLE);
    int32_t userId = 100;
    data.WriteInt32(userId);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_NOTIFY_DATA_CHANGE_EXT, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnQueryStatistics
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnQueryStatistics, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_QUERY_STATISTICS, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    data.WriteString(TEST_CLOUD_ID);
    data.WriteString(TEST_CLOUD_BUNDLE);
    data.WriteString(TEST_CLOUD_STORE);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_QUERY_STATISTICS, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnQueryLastSyncInfo
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnQueryLastSyncInfo, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_QUERY_LAST_SYNC_INFO, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    data.WriteString(TEST_CLOUD_ID);
    data.WriteString(TEST_CLOUD_BUNDLE);
    data.WriteString(TEST_CLOUD_STORE);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_QUERY_LAST_SYNC_INFO, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: UpdateClearWaterMark001
* @tc.desc: Test UpdateClearWaterMark001 the database.version not found.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudDataTest, UpdateClearWaterMark001, TestSize.Level0)
{
    ASSERT_NE(cloudServiceImpl_, nullptr);
    CloudData::CloudServiceImpl::HapInfo hapInfo = {
       .instIndex = 0, .bundleName = TEST_CLOUD_BUNDLE, .user = cloudInfo_.user
    };
    SchemaMeta::Database database;
    database.name = TEST_CLOUD_STORE;
    database.version = 1;
    SchemaMeta schemaMeta;
    schemaMeta.version = 1;
    schemaMeta.databases.push_back(database);

    SchemaMeta::Database database1;
    database1.name = TEST_CLOUD_STORE_1;
    database1.version = 2;
    SchemaMeta newSchemaMeta;
    newSchemaMeta.version = 0;
    newSchemaMeta.databases.push_back(database1);
    cloudServiceImpl_->UpdateClearWaterMark(hapInfo, newSchemaMeta, schemaMeta);

    CloudMark metaData;
    metaData.bundleName = hapInfo.bundleName;
    metaData.userId = hapInfo.user;
    metaData.index = hapInfo.instIndex;
    metaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData.storeId = database1.name;
    ASSERT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData, true));
}

/**
* @tc.name: UpdateClearWaterMark002
* @tc.desc: Test UpdateClearWaterMark002 the same database.version
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudDataTest, UpdateClearWaterMark002, TestSize.Level0)
{
    ASSERT_NE(cloudServiceImpl_, nullptr);
    CloudData::CloudServiceImpl::HapInfo hapInfo = {
       .instIndex = 0, .bundleName = TEST_CLOUD_BUNDLE, .user = cloudInfo_.user
    };
    SchemaMeta::Database database;
    database.name = TEST_CLOUD_STORE;
    database.version = 1;
    SchemaMeta schemaMeta;
    schemaMeta.version = 1;
    schemaMeta.databases.push_back(database);

    SchemaMeta::Database database1;
    database1.name = TEST_CLOUD_STORE;
    database1.version = 1;
    SchemaMeta newSchemaMeta;
    newSchemaMeta.version = 0;
    newSchemaMeta.databases.push_back(database1);
    cloudServiceImpl_->UpdateClearWaterMark(hapInfo, newSchemaMeta, schemaMeta);

    CloudMark metaData;
    metaData.bundleName = hapInfo.bundleName;
    metaData.userId = hapInfo.user;
    metaData.index = hapInfo.instIndex;
    metaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData.storeId = database1.name;
    ASSERT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData, true));
}

/**
* @tc.name: UpdateClearWaterMark003
* @tc.desc: Test UpdateClearWaterMark003 the different database.version
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudDataTest, UpdateClearWaterMark003, TestSize.Level0)
{
    ASSERT_NE(cloudServiceImpl_, nullptr);
    CloudData::CloudServiceImpl::HapInfo hapInfo = {
       .instIndex = 0, .bundleName = TEST_CLOUD_BUNDLE, .user = cloudInfo_.user
    };
    SchemaMeta::Database database;
    database.name = TEST_CLOUD_STORE;
    database.version = 1;
    SchemaMeta schemaMeta;
    schemaMeta.version = 1;
    schemaMeta.databases.push_back(database);

    SchemaMeta::Database database1;
    database1.name = TEST_CLOUD_STORE;
    database1.version = 2;
    SchemaMeta newSchemaMeta;
    newSchemaMeta.version = 0;
    newSchemaMeta.databases.push_back(database1);
    cloudServiceImpl_->UpdateClearWaterMark(hapInfo, newSchemaMeta, schemaMeta);

    CloudMark metaData;
    metaData.bundleName = hapInfo.bundleName;
    metaData.userId = hapInfo.user;
    metaData.index = hapInfo.instIndex;
    metaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData.storeId = database1.name;
    ASSERT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData, true));
    EXPECT_TRUE(metaData.isClearWaterMark);
    MetaDataManager::GetInstance().DelMeta(metaData.GetKey(), true);
}

/**
* @tc.name: GetPrepareTraceId
* @tc.desc: Test GetPrepareTraceId && GetUser
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudDataTest, GetPrepareTraceId, TestSize.Level0)
{
    SyncParam syncParam;
    syncParam.prepareTraceId = TEST_TRACE_ID;
    syncParam.user = EVT_USER;
    auto async = [](const GenDetails &details) {};
    SyncEvent::EventInfo eventInfo(syncParam, true, nullptr, async);
    StoreInfo storeInfo;
    SyncEvent evt(storeInfo, std::move(eventInfo));
    EXPECT_EQ(evt.GetUser(), EVT_USER);
    EXPECT_EQ(evt.GetPrepareTraceId(), TEST_TRACE_ID);
}

/**
* @tc.name: TryUpdateDeviceId001
* @tc.desc: TryUpdateDeviceId test
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudDataTest, TryUpdateDeviceId001, TestSize.Level1)
{
    DistributedRdb::RdbServiceImpl rdbServiceImpl;
    DistributedRdb::RdbSyncerParam param{ .bundleName_ = TEST_CLOUD_BUNDLE, .storeName_ = TEST_CLOUD_STORE };
    StoreMetaData oldMeta;
    oldMeta.deviceId = "oldUuidtest";
    oldMeta.user = "100";
    oldMeta.bundleName = "test_appid_001";
    oldMeta.storeId = "test_storeid_001";
    oldMeta.isNeedUpdateDeviceId = true;
    oldMeta.storeType = StoreMetaData::StoreType::STORE_RELATIONAL_BEGIN;
    bool isSuccess = MetaDataManager::GetInstance().SaveMeta(oldMeta.GetKeyWithoutPath(), oldMeta);
    EXPECT_EQ(isSuccess, true);
    StoreMetaData meta1 = oldMeta;
    auto ret = rdbServiceImpl.TryUpdateDeviceId(param, oldMeta, meta1);
    EXPECT_EQ(ret, true);
    MetaDataManager::GetInstance().DelMeta(oldMeta.GetKeyWithoutPath());
}

/**
* @tc.name: TryUpdateDeviceId002
* @tc.desc: TryUpdateDeviceId test
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudDataTest, TryUpdateDeviceId002, TestSize.Level1)
{
    DistributedRdb::RdbServiceImpl rdbServiceImpl;
    DistributedRdb::RdbSyncerParam param{ .bundleName_ = TEST_CLOUD_BUNDLE, .storeName_ = TEST_CLOUD_STORE };
    StoreMetaData oldMeta;
    oldMeta.deviceId = "oldUuidtest";
    oldMeta.user = "100";
    oldMeta.bundleName = "test_appid_001";
    oldMeta.storeId = "test_storeid_001";
    oldMeta.isNeedUpdateDeviceId = false;
    oldMeta.storeType = StoreMetaData::StoreType::STORE_RELATIONAL_BEGIN;
    bool isSuccess = MetaDataManager::GetInstance().SaveMeta(oldMeta.GetKeyWithoutPath(), oldMeta);
    EXPECT_EQ(isSuccess, true);
    StoreMetaData meta1 = oldMeta;
    auto ret = rdbServiceImpl.TryUpdateDeviceId(param, oldMeta, meta1);
    EXPECT_EQ(ret, true);
    MetaDataManager::GetInstance().DelMeta(oldMeta.GetKeyWithoutPath());
}

/**
* @tc.name: TryUpdateDeviceId003
* @tc.desc: TryUpdateDeviceId test
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudDataTest, TryUpdateDeviceId003, TestSize.Level1)
{
    DistributedRdb::RdbServiceImpl rdbServiceImpl;
    DistributedRdb::RdbSyncerParam param{ .bundleName_ = TEST_CLOUD_BUNDLE, .storeName_ = TEST_CLOUD_STORE };
    StoreMetaData oldMeta;
    oldMeta.deviceId = "oldUuidtest";
    oldMeta.user = "100";
    oldMeta.bundleName = "test_appid_001";
    oldMeta.storeId = "test_storeid_001";
    oldMeta.isNeedUpdateDeviceId = true;
    oldMeta.storeType = StoreMetaData::StoreType::STORE_RELATIONAL_END;
    bool isSuccess = MetaDataManager::GetInstance().SaveMeta(oldMeta.GetKeyWithoutPath(), oldMeta);
    EXPECT_EQ(isSuccess, true);
    StoreMetaData meta1 = oldMeta;
    auto ret = rdbServiceImpl.TryUpdateDeviceId(param, oldMeta, meta1);
    EXPECT_EQ(ret, true);
    MetaDataManager::GetInstance().DelMeta(oldMeta.GetKeyWithoutPath());
}

/**
* @tc.name: TryUpdateDeviceId004
* @tc.desc: TryUpdateDeviceId test
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudDataTest, TryUpdateDeviceId004, TestSize.Level1)
{
    DistributedRdb::RdbServiceImpl rdbServiceImpl;
    DistributedRdb::RdbSyncerParam param{ .bundleName_ = TEST_CLOUD_BUNDLE, .storeName_ = TEST_CLOUD_STORE };
    StoreMetaData oldMeta;
    oldMeta.deviceId = "oldUuidtest";
    oldMeta.user = "100";
    oldMeta.bundleName = "test_appid_001";
    oldMeta.storeId = "test_storeid_001";
    oldMeta.isNeedUpdateDeviceId = false;
    oldMeta.storeType = StoreMetaData::StoreType::STORE_RELATIONAL_END;
    bool isSuccess = MetaDataManager::GetInstance().SaveMeta(oldMeta.GetKeyWithoutPath(), oldMeta);
    EXPECT_EQ(isSuccess, true);
    StoreMetaData meta1 = oldMeta;
    auto ret = rdbServiceImpl.TryUpdateDeviceId(param, oldMeta, meta1);
    EXPECT_EQ(ret, true);
    MetaDataManager::GetInstance().DelMeta(oldMeta.GetKeyWithoutPath());
}

/**
* @tc.name: OnInitialize
* @tc.desc: OnInitialize test
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudDataTest, OnInitialize, TestSize.Level1)
{
    auto code = cloudServiceImpl_->OnInitialize();
    EXPECT_EQ(code, E_OK);
}

/**
* @tc.name: CleanWaterVersion
* @tc.desc: CleanWaterVersion test
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudDataTest, CleanWaterVersion, TestSize.Level1)
{
    auto ret = cloudServiceImpl_->CleanWaterVersion(200);
    EXPECT_FALSE(ret);
    ret = cloudServiceImpl_->CleanWaterVersion(
        AccountDelegate::GetInstance()->GetUserByToken(IPCSkeleton::GetCallingTokenID()));
    EXPECT_TRUE(ret);
}

/**
* @tc.name: ConvertGenDetailsCode
* @tc.desc: Test ConvertGenDetailsCode function.
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, ConvertGenDetailsCode, TestSize.Level0)
{
    DistributedData::GenDetails result;
    GenProgressDetail detail;
    detail.progress = GenProgress::SYNC_IN_PROGRESS;
    detail.code = 100;
    result.insert(std::make_pair("test", detail));
    auto details = CloudData::SyncManager::ConvertGenDetailsCode(result);
    EXPECT_TRUE(details["test"].code == E_ERROR);

    DistributedData::GenDetails result1;
    GenProgressDetail detail1;
    detail1.progress = GenProgress::SYNC_IN_PROGRESS;
    detail1.code = E_ERROR;
    result1.insert(std::make_pair("test", detail1));
    details = CloudData::SyncManager::ConvertGenDetailsCode(result1);
    EXPECT_TRUE(details["test"].code == E_ERROR);

    DistributedData::GenDetails result2;
    GenProgressDetail detail2;
    detail2.progress = GenProgress::SYNC_IN_PROGRESS;
    detail2.code = E_OK;
    result2.insert(std::make_pair("test", detail2));
    details = CloudData::SyncManager::ConvertGenDetailsCode(result2);
    EXPECT_TRUE(details["test"].code == E_OK);

    DistributedData::GenDetails result3;
    GenProgressDetail detail3;
    detail3.progress = GenProgress::SYNC_IN_PROGRESS;
    detail3.code = E_BLOCKED_BY_NETWORK_STRATEGY;
    result3.insert(std::make_pair("test", detail3));
    details = CloudData::SyncManager::ConvertGenDetailsCode(result3);
    EXPECT_TRUE(details["test"].code == E_BLOCKED_BY_NETWORK_STRATEGY);

    DistributedData::GenDetails result4;
    GenProgressDetail detail4;
    detail4.progress = GenProgress::SYNC_IN_PROGRESS;
    detail4.code = E_BUSY;
    result4.insert(std::make_pair("test", detail4));
    details = CloudData::SyncManager::ConvertGenDetailsCode(result4);
    EXPECT_TRUE(details["test"].code == E_ERROR);
}

/**
* @tc.name: ConvertValidGeneralCode
* @tc.desc: Test ConvertValidGeneralCode function.
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, GetValidGeneralCode, TestSize.Level0)
{
    auto ret = CloudData::SyncManager::ConvertValidGeneralCode(E_OK);
    EXPECT_TRUE(ret == E_OK);
    ret = CloudData::SyncManager::ConvertValidGeneralCode(E_ERROR);
    EXPECT_TRUE(ret == E_ERROR);
    ret = CloudData::SyncManager::ConvertValidGeneralCode(E_NETWORK_ERROR);
    EXPECT_TRUE(ret == E_NETWORK_ERROR);
    ret = CloudData::SyncManager::ConvertValidGeneralCode(E_CLOUD_DISABLED);
    EXPECT_TRUE(ret == E_CLOUD_DISABLED);
    ret = CloudData::SyncManager::ConvertValidGeneralCode(E_LOCKED_BY_OTHERS);
    EXPECT_TRUE(ret == E_LOCKED_BY_OTHERS);
    ret = CloudData::SyncManager::ConvertValidGeneralCode(E_RECODE_LIMIT_EXCEEDED);
    EXPECT_TRUE(ret == E_RECODE_LIMIT_EXCEEDED);
    ret = CloudData::SyncManager::ConvertValidGeneralCode(E_NO_SPACE_FOR_ASSET);
    EXPECT_TRUE(ret == E_NO_SPACE_FOR_ASSET);
    ret = CloudData::SyncManager::ConvertValidGeneralCode(E_BLOCKED_BY_NETWORK_STRATEGY);
    EXPECT_TRUE(ret == E_BLOCKED_BY_NETWORK_STRATEGY);
    ret = CloudData::SyncManager::ConvertValidGeneralCode(E_BUSY);
    EXPECT_TRUE(ret == E_ERROR);
    ret = CloudData::SyncManager::ConvertValidGeneralCode(E_SYNC_TASK_MERGED);
    EXPECT_TRUE(ret == E_ERROR);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test