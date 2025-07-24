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
} // namespace DistributedDataTest
} // namespace OHOS::Test