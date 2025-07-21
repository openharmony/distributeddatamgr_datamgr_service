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
* @tc.name: OnSetGlobalCloudStrategy
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnSetGlobalCloudStrategy, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret =
        cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_SET_GLOBAL_CLOUD_STRATEGY, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    uint32_t strategy = 0;
    std::vector<CommonType::Value> values;
    ITypesUtil::Marshal(data, strategy, values);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_SET_GLOBAL_CLOUD_STRATEGY, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnAllocResourceAndShare
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnAllocResourceAndShare, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(
        CloudData::CloudService::TRANS_ALLOC_RESOURCE_AND_SHARE, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    std::string storeId = "storeId";
    DistributedRdb::PredicatesMemo predicates;
    std::vector<std::string> columns;
    std::vector<CloudData::Participant> participants;
    ITypesUtil::Marshal(data, storeId, predicates, columns, participants);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_ALLOC_RESOURCE_AND_SHARE, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnShare
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnShare, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_SHARE, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    std::string sharingRes;
    CloudData::Participants participants;
    CloudData::Results results;
    ITypesUtil::Marshal(data, sharingRes, participants, results);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_SHARE, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnUnshare
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnUnshare, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_UNSHARE, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    std::string sharingRes;
    CloudData::Participants participants;
    CloudData::Results results;
    ITypesUtil::Marshal(data, sharingRes, participants, results);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_UNSHARE, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnExit
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnExit, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_EXIT, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    std::string sharingRes;
    std::pair<int32_t, std::string> result;
    ITypesUtil::Marshal(data, sharingRes, result);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_EXIT, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnChangePrivilege
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnChangePrivilege, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_CHANGE_PRIVILEGE, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    std::string sharingRes;
    CloudData::Participants participants;
    CloudData::Results results;
    ITypesUtil::Marshal(data, sharingRes, participants, results);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_CHANGE_PRIVILEGE, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnQuery
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnQuery, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_QUERY, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    std::string sharingRes;
    CloudData::QueryResults results;
    ITypesUtil::Marshal(data, sharingRes, results);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_QUERY, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnQueryByInvitation
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnQueryByInvitation, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_QUERY_BY_INVITATION, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    std::string invitation;
    CloudData::QueryResults results;
    ITypesUtil::Marshal(data, invitation, results);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_QUERY_BY_INVITATION, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnConfirmInvitation
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnConfirmInvitation, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_CONFIRM_INVITATION, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    std::string invitation;
    int32_t confirmation = 0;
    std::tuple<int32_t, std::string, std::string> result;
    ITypesUtil::Marshal(data, invitation, confirmation, result);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_CONFIRM_INVITATION, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnChangeConfirmation
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnChangeConfirmation, TestSize.Level1)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_CHANGE_CONFIRMATION, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    std::string sharingRes;
    int32_t confirmation = 0;
    std::pair<int32_t, std::string> result;
    ITypesUtil::Marshal(data, sharingRes, confirmation, result);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_CHANGE_CONFIRMATION, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnSetCloudStrategy
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnSetCloudStrategy, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_SET_CLOUD_STRATEGY, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    uint32_t strategy = 0;
    std::vector<CommonType::Value> values;
    ITypesUtil::Marshal(data, strategy, values);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_SET_CLOUD_STRATEGY, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnCloudSync
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnCloudSync, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_CLOUD_SYNC, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    std::string bundleName = "bundleName";
    std::string storeId = "storeId";
    CloudData::CloudService::Option option;
    option.syncMode = 4;
    option.seqNum = 1;
    ITypesUtil::Marshal(data, bundleName, storeId, option);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_CLOUD_SYNC, data, reply);
    EXPECT_EQ(ret, ERR_NONE);
}

/**
* @tc.name: OnInitNotifier
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnInitNotifier, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    auto ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_INIT_NOTIFIER, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    data.WriteInterfaceToken(cloudServiceImpl_->GetDescriptor());
    std::string bundleName = "bundleName";
    sptr<IRemoteObject> notifier = nullptr;
    ITypesUtil::Marshal(data, bundleName, notifier);
    ret = cloudServiceImpl_->OnRemoteRequest(CloudData::CloudService::TRANS_INIT_NOTIFIER, data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
}

/**
* @tc.name: SharingUtil001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, SharingUtil001, TestSize.Level0)
{
    auto cfm = CloudData::SharingUtil::Convert(Confirmation::CFM_UNKNOWN);
    EXPECT_EQ(cfm, SharingCfm::CFM_UNKNOWN);
    cfm = CloudData::SharingUtil::Convert(Confirmation::CFM_ACCEPTED);
    EXPECT_EQ(cfm, SharingCfm::CFM_ACCEPTED);
    cfm = CloudData::SharingUtil::Convert(Confirmation::CFM_REJECTED);
    EXPECT_EQ(cfm, SharingCfm::CFM_REJECTED);
    cfm = CloudData::SharingUtil::Convert(Confirmation::CFM_SUSPENDED);
    EXPECT_EQ(cfm, SharingCfm::CFM_SUSPENDED);
    cfm = CloudData::SharingUtil::Convert(Confirmation::CFM_UNAVAILABLE);
    EXPECT_EQ(cfm, SharingCfm::CFM_UNAVAILABLE);
    cfm = CloudData::SharingUtil::Convert(Confirmation::CFM_BUTT);
    EXPECT_EQ(cfm, SharingCfm::CFM_UNKNOWN);
}

/**
* @tc.name: SharingUtil002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, SharingUtil002, TestSize.Level0)
{
    auto cfm = CloudData::SharingUtil::Convert(SharingCfm::CFM_UNKNOWN);
    EXPECT_EQ(cfm, Confirmation::CFM_UNKNOWN);
    cfm = CloudData::SharingUtil::Convert(SharingCfm::CFM_ACCEPTED);
    EXPECT_EQ(cfm, Confirmation::CFM_ACCEPTED);
    cfm = CloudData::SharingUtil::Convert(SharingCfm::CFM_REJECTED);
    EXPECT_EQ(cfm, Confirmation::CFM_REJECTED);
    cfm = CloudData::SharingUtil::Convert(SharingCfm::CFM_SUSPENDED);
    EXPECT_EQ(cfm, Confirmation::CFM_SUSPENDED);
    cfm = CloudData::SharingUtil::Convert(SharingCfm::CFM_UNAVAILABLE);
    EXPECT_EQ(cfm, Confirmation::CFM_UNAVAILABLE);
    cfm = CloudData::SharingUtil::Convert(SharingCfm::CFM_BUTT);
    EXPECT_EQ(cfm, Confirmation::CFM_UNKNOWN);
}

/**
* @tc.name: SharingUtil003
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, SharingUtil003, TestSize.Level0)
{
    auto status = CloudData::SharingUtil::Convert(CenterCode::IPC_ERROR);
    EXPECT_EQ(status, Status::IPC_ERROR);
    status = CloudData::SharingUtil::Convert(CenterCode::NOT_SUPPORT);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: SharingUtil004
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, SharingUtil004, TestSize.Level0)
{
    auto status = CloudData::SharingUtil::Convert(GenErr::E_OK);
    EXPECT_EQ(status, Status::SUCCESS);
    status = CloudData::SharingUtil::Convert(GenErr::E_ERROR);
    EXPECT_EQ(status, Status::ERROR);
    status = CloudData::SharingUtil::Convert(GenErr::E_INVALID_ARGS);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
    status = CloudData::SharingUtil::Convert(GenErr::E_BLOCKED_BY_NETWORK_STRATEGY);
    EXPECT_EQ(status, Status::STRATEGY_BLOCKING);
    status = CloudData::SharingUtil::Convert(GenErr::E_CLOUD_DISABLED);
    EXPECT_EQ(status, Status::CLOUD_DISABLE);
    status = CloudData::SharingUtil::Convert(GenErr::E_NETWORK_ERROR);
    EXPECT_EQ(status, Status::NETWORK_ERROR);
    status = CloudData::SharingUtil::Convert(GenErr::E_BUSY);
    EXPECT_EQ(status, Status::ERROR);
}

/**
* @tc.name: DoCloudSync
* @tc.desc: Test the executor_ uninitialized and initialized scenarios
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, DoCloudSync, TestSize.Level0)
{
    int32_t user = 100;
    CloudData::SyncManager sync;
    CloudData::SyncManager::SyncInfo info(user);
    auto ret = sync.DoCloudSync(info);
    EXPECT_EQ(ret, GenErr::E_NOT_INIT);
    ret = sync.StopCloudSync(user);
    EXPECT_EQ(ret, GenErr::E_NOT_INIT);
    size_t max = 12;
    size_t min = 5;
    sync.executor_ = std::make_shared<ExecutorPool>(max, min);
    ret = sync.DoCloudSync(info);
    EXPECT_EQ(ret, GenErr::E_OK);
    int32_t invalidUser = -1;
    sync.StopCloudSync(invalidUser);
    ret = sync.StopCloudSync(user);
    EXPECT_EQ(ret, GenErr::E_OK);
}

/**
* @tc.name: GetPostEventTask
* @tc.desc: Test the interface to verify the package name and table name
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, GetPostEventTask, TestSize.Level0)
{
    std::vector<SchemaMeta> schemas;
    schemaMeta_.databases[0].name = "test";
    schemas.push_back(schemaMeta_);
    schemaMeta_.bundleName = "test";
    schemas.push_back(schemaMeta_);

    int32_t user = 100;
    CloudData::SyncManager::SyncInfo info(user);
    std::vector<std::string> value;
    info.tables_.insert_or_assign(TEST_CLOUD_STORE, value);

    CloudData::SyncManager sync;
    std::map<std::string, std::string> traceIds;
    auto task = sync.GetPostEventTask(schemas, cloudInfo_, info, true, traceIds);
    task();
    std::vector<CloudLastSyncInfo> lastSyncInfos;
    MetaDataManager::GetInstance().LoadMeta(CloudLastSyncInfo::GetKey(user, "test", "test"), lastSyncInfos, true);
    EXPECT_TRUE(lastSyncInfos.size() == 0);
}

/**
* @tc.name: GetRetryer001
* @tc.desc: Test the input parameters of different interfaces
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, GetRetryer001, TestSize.Level0)
{
    int32_t user = 100;
    CloudData::SyncManager::SyncInfo info(user);
    CloudData::SyncManager sync;
    CloudData::SyncManager::Duration duration;
    std::string prepareTraceId;
    auto ret = sync.GetRetryer(CloudData::SyncManager::RETRY_TIMES, info, user)(duration, E_OK, E_OK, prepareTraceId);
    EXPECT_TRUE(ret);
    ret = sync.GetRetryer(CloudData::SyncManager::RETRY_TIMES, info, user)(
        duration, E_SYNC_TASK_MERGED, E_SYNC_TASK_MERGED, prepareTraceId);
    EXPECT_TRUE(ret);
    ret = sync.GetRetryer(0, info, user)(duration, E_OK, E_OK, prepareTraceId);
    EXPECT_TRUE(ret);
    ret = sync.GetRetryer(0, info, user)(duration, E_SYNC_TASK_MERGED, E_SYNC_TASK_MERGED, prepareTraceId);
    EXPECT_TRUE(ret);
}

/**
* @tc.name: GetRetryer002
* @tc.desc: Test the executor_ is nullptr scenarios.
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, GetRetryer002, TestSize.Level0)
{
    int32_t user = 100;
    CloudData::SyncManager::SyncInfo info(user);
    CloudData::SyncManager sync;
    CloudData::SyncManager::Duration duration;
    std::string prepareTraceId;
    sync.executor_ = nullptr;
    int32_t evtId = 100;
    auto event = std::make_unique<CloudData::SyncManager::Event>(evtId);
    auto handler = sync.GetClientChangeHandler();
    handler(*event);
    auto ret = sync.GetRetryer(0, info, user)(duration, E_CLOUD_DISABLED, E_CLOUD_DISABLED, prepareTraceId);
    EXPECT_FALSE(ret);
}

/**
* @tc.name: GetCallback
* @tc.desc: Test the processing logic of different progress callbacks
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, GetCallback, TestSize.Level0)
{
    int32_t user = 100;
    CloudData::SyncManager::SyncInfo info(user);
    CloudData::SyncManager sync;
    DistributedData::GenDetails result;
    StoreInfo storeInfo;
    storeInfo.user = user;
    storeInfo.bundleName = "testBundleName";
    int32_t triggerMode = MODE_DEFAULT;
    std::string prepareTraceId;
    GenAsync async = nullptr;
    sync.GetCallback(async, storeInfo, triggerMode, prepareTraceId, user)(result);
    int32_t process = 0;
    async = [&process](const GenDetails &details) {
        process = details.begin()->second.progress;
    };
    GenProgressDetail detail;
    detail.progress = GenProgress::SYNC_IN_PROGRESS;
    result.insert_or_assign("test", detail);
    sync.GetCallback(async, storeInfo, triggerMode, prepareTraceId, user)(result);
    EXPECT_EQ(process, GenProgress::SYNC_IN_PROGRESS);
    detail.progress = GenProgress::SYNC_FINISH;
    result.insert_or_assign("test", detail);
    storeInfo.user = -1;
    sync.GetCallback(async, storeInfo, triggerMode, prepareTraceId, user)(result);
    storeInfo.user = user;
    sync.GetCallback(async, storeInfo, triggerMode, prepareTraceId, user)(result);
    EXPECT_EQ(process, GenProgress::SYNC_FINISH);
}

/**
* @tc.name: GetInterval
* @tc.desc: Test the Interval transformation logic of the interface
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, GetInterval, TestSize.Level0)
{
    CloudData::SyncManager sync;

    auto ret = sync.GetInterval(E_LOCKED_BY_OTHERS);
    EXPECT_EQ(ret, CloudData::SyncManager::LOCKED_INTERVAL);
    ret = sync.GetInterval(E_BUSY);
    EXPECT_EQ(ret, CloudData::SyncManager::BUSY_INTERVAL);
    ret = sync.GetInterval(E_OK);
    EXPECT_EQ(ret, CloudData::SyncManager::RETRY_INTERVAL);
}

/**
* @tc.name: GetCloudSyncInfo
* @tc.desc: Test get cloudInfo
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, GetCloudSyncInfo, TestSize.Level0)
{
    CloudData::SyncManager sync;
    CloudInfo cloud;
    cloud.user = cloudInfo_.user;
    cloud.enableCloud = false;
    CloudData::SyncManager::SyncInfo info(cloudInfo_.user);
    MetaDataManager::GetInstance().DelMeta(cloudInfo_.GetKey(), true);
    info.bundleName_ = TEST_CLOUD_BUNDLE;
    auto ret = sync.GetCloudSyncInfo(info, cloud);
    EXPECT_TRUE(!ret.empty());
}

/**
* @tc.name: RetryCallback
* @tc.desc: Test the retry logic
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, RetryCallback, TestSize.Level0)
{
    int32_t user = 100;
    std::string prepareTraceId;
    CloudData::SyncManager sync;
    StoreInfo storeInfo;
    int32_t retCode = -1;
    CloudData::SyncManager::Retryer retry = [&retCode](CloudData::SyncManager::Duration interval, int32_t code,
                                                int32_t dbCode, const std::string &prepareTraceId) {
        retCode = code;
        return true;
    };
    DistributedData::GenDetails result;
    auto task = sync.RetryCallback(storeInfo, retry, MODE_DEFAULT, prepareTraceId, user);
    task(result);
    GenProgressDetail detail;
    detail.progress = GenProgress::SYNC_IN_PROGRESS;
    detail.code = 100;
    result.insert_or_assign("test", detail);
    task = sync.RetryCallback(storeInfo, retry, MODE_DEFAULT, prepareTraceId, user);
    task(result);
    EXPECT_EQ(retCode, detail.code);
}

/**
* @tc.name: UpdateCloudInfoFromServer
* @tc.desc: Test updating cloudinfo from the server
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, UpdateCloudInfoFromServer, TestSize.Level0)
{
    auto ret = cloudServiceImpl_->UpdateCloudInfoFromServer(cloudInfo_.user);
    EXPECT_EQ(ret, E_OK);
}

/**
* @tc.name: GetCloudInfo
* @tc.desc: Test get cloudInfo
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, GetCloudInfo, TestSize.Level1)
{
    MetaDataManager::GetInstance().DelMeta(cloudInfo_.GetKey(), true);
    auto ret = cloudServiceImpl_->GetCloudInfo(cloudInfo_.user);
    EXPECT_EQ(ret.first, CloudData::SUCCESS);
}

/**
* @tc.name: UpdateSchemaFromServer_001
* @tc.desc: Test get UpdateSchemaFromServer
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, UpdateSchemaFromServer_001, TestSize.Level1)
{
    auto status = cloudServiceImpl_->UpdateSchemaFromServer(cloudInfo_.user);
    EXPECT_EQ(status, CloudData::SUCCESS);
}

/**
 * @tc.name: OnAppInstallTest
 * @tc.desc: Test the OnAppInstallTest
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudDataTest, OnAppInstallTest, TestSize.Level1)
{
    ZLOGI("CloudDataTest OnAppInstallTest start");
    ASSERT_NE(cloudServiceImpl_, nullptr);
    ASSERT_NE(cloudServiceImpl_->factory_.staticActs_, nullptr);
    int32_t index = 0;
    auto status = cloudServiceImpl_->factory_.staticActs_->OnAppInstall(TEST_CLOUD_BUNDLE, cloudInfo_.user, index);
    EXPECT_EQ(status, GeneralError::E_OK);
}

/**
 * @tc.name: OnAppUpdateTest
 * @tc.desc: Test the OnAppUpdateTest
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudDataTest, OnAppUpdateTest, TestSize.Level1)
{
    ZLOGI("CloudDataTest OnAppUpdateTest start");
    ASSERT_NE(cloudServiceImpl_, nullptr);
    ASSERT_NE(cloudServiceImpl_->factory_.staticActs_, nullptr);
    int32_t index = 0;
    auto status = cloudServiceImpl_->factory_.staticActs_->OnAppUpdate(TEST_CLOUD_BUNDLE, cloudInfo_.user, index);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
}

/**
* @tc.name: UpdateE2eeEnableTest
* @tc.desc: Test the UpdateE2eeEnable
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudDataTest, UpdateE2eeEnableTest, TestSize.Level1)
{
    SchemaMeta schemaMeta;
    std::string schemaKey = CloudInfo::GetSchemaKey(cloudInfo_.user, TEST_CLOUD_BUNDLE, 0);
    ASSERT_TRUE(MetaDataManager::GetInstance().LoadMeta(schemaKey, schemaMeta, true));
    EXPECT_EQ(schemaMeta.e2eeEnable, schemaMeta_.e2eeEnable);

    ASSERT_NE(cloudServiceImpl_, nullptr);
    cloudServiceImpl_->UpdateE2eeEnable(schemaKey, false, TEST_CLOUD_BUNDLE);
    ASSERT_TRUE(MetaDataManager::GetInstance().LoadMeta(schemaKey, schemaMeta, true));
    EXPECT_EQ(schemaMeta.e2eeEnable, schemaMeta_.e2eeEnable);
    cloudServiceImpl_->UpdateE2eeEnable(schemaKey, true, TEST_CLOUD_BUNDLE);
    ASSERT_TRUE(MetaDataManager::GetInstance().LoadMeta(schemaKey, schemaMeta, true));
    EXPECT_EQ(schemaMeta.e2eeEnable, true);
}

/**
* @tc.name: SubTask
* @tc.desc: Test the subtask execution logic
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, SubTask, TestSize.Level0)
{
    DistributedData::Subscription sub;
    cloudServiceImpl_->InitSubTask(sub, 0);
    MetaDataManager::GetInstance().LoadMeta(Subscription::GetKey(cloudInfo_.user), sub, true);
    cloudServiceImpl_->InitSubTask(sub, 0);
    int32_t userId = 0;
    CloudData::CloudServiceImpl::Task task = [&userId]() {
        userId = cloudInfo_.user;
    };
    cloudServiceImpl_->GenSubTask(task, cloudInfo_.user)();
    EXPECT_EQ(userId, cloudInfo_.user);
}

/**
* @tc.name: ConvertCursor
* @tc.desc: Test the cursor conversion logic when the ResultSet is empty and non-null
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, ConvertCursor, TestSize.Level0)
{
    std::map<std::string, DistributedData::Value> entry;
    entry.insert_or_assign("test", "entry");
    auto resultSet = std::make_shared<CursorMock::ResultSet>(1, entry);
    auto cursor = std::make_shared<CursorMock>(resultSet);
    auto result = cloudServiceImpl_->ConvertCursor(cursor);
    EXPECT_TRUE(!result.empty());
    auto resultSet1 = std::make_shared<CursorMock::ResultSet>();
    auto cursor1 = std::make_shared<CursorMock>(resultSet1);
    auto result1 = cloudServiceImpl_->ConvertCursor(cursor1);
    EXPECT_TRUE(result1.empty());
}

/**
* @tc.name: GetDbInfoFromExtraData
* @tc.desc: Test the GetDbInfoFromExtraData function input parameters of different parameters
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, GetDbInfoFromExtraData, TestSize.Level0)
{
    SchemaMeta::Database database;
    database.name = TEST_CLOUD_STORE;
    database.alias = TEST_CLOUD_DATABASE_ALIAS_1;

    SchemaMeta schemaMeta;
    schemaMeta.databases.push_back(database);

    SchemaMeta::Table table;
    table.name = "test_cloud_table_name";
    table.alias = "test_cloud_table_alias";
    database.tables.push_back(table);
    SchemaMeta::Table table1;
    table1.name = "test_cloud_table_name1";
    table1.alias = "test_cloud_table_alias1";
    table1.sharedTableName = "test_share_table_name1";
    database.tables.emplace_back(table1);

    database.alias = TEST_CLOUD_DATABASE_ALIAS_2;
    schemaMeta.databases.push_back(database);

    ExtraData extraData;
    extraData.info.containerName = TEST_CLOUD_DATABASE_ALIAS_2;
    auto result = cloudServiceImpl_->GetDbInfoFromExtraData(extraData, schemaMeta);
    EXPECT_EQ(result.begin()->first, TEST_CLOUD_STORE);

    std::string tableName = "test_cloud_table_alias2";
    extraData.info.tables.emplace_back(tableName);
    result = cloudServiceImpl_->GetDbInfoFromExtraData(extraData, schemaMeta);
    EXPECT_EQ(result.begin()->first, TEST_CLOUD_STORE);

    std::string tableName1 = "test_cloud_table_alias1";
    extraData.info.tables.emplace_back(tableName1);
    extraData.info.scopes.emplace_back(DistributedData::ExtraData::SHARED_TABLE);
    result = cloudServiceImpl_->GetDbInfoFromExtraData(extraData, schemaMeta);
    EXPECT_EQ(result.begin()->first, TEST_CLOUD_STORE);
}

/**
* @tc.name: QueryTableStatistic
* @tc.desc: Test the QueryTableStatistic function input parameters of different parameters
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, QueryTableStatistic, TestSize.Level0)
{
    auto store = std::make_shared<GeneralStoreMock>();
    if (store != nullptr) {
        std::map<std::string, Value> entry = { { "inserted", "TEST" }, { "updated", "TEST" }, { "normal", "TEST" } };
        store->MakeCursor(entry);
    }
    auto [ret, result] = cloudServiceImpl_->QueryTableStatistic("test", store);
    EXPECT_TRUE(ret);
    if (store != nullptr) {
        std::map<std::string, Value> entry = { { "Test", 1 } };
        store->MakeCursor(entry);
    }
    std::tie(ret, result) = cloudServiceImpl_->QueryTableStatistic("test", store);
    EXPECT_TRUE(ret);

    if (store != nullptr) {
        store->cursor_ = nullptr;
    }
    std::tie(ret, result) = cloudServiceImpl_->QueryTableStatistic("test", store);
    EXPECT_FALSE(ret);
}

/**
* @tc.name: GetSchemaMeta
* @tc.desc: Test the GetSchemaMeta function input parameters of different parameters
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, GetSchemaMeta, TestSize.Level0)
{
    int32_t userId = 101;
    int32_t instanceId = 0;
    CloudInfo cloudInfo;
    cloudInfo.user = userId;
    cloudInfo.id = TEST_CLOUD_ID;
    cloudInfo.enableCloud = true;

    CloudInfo::AppInfo appInfo;
    appInfo.bundleName = TEST_CLOUD_BUNDLE;
    appInfo.appId = TEST_CLOUD_APPID;
    appInfo.version = 1;
    appInfo.cloudSwitch = true;

    cloudInfo.apps[TEST_CLOUD_BUNDLE] = std::move(appInfo);
    MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true);
    std::string bundleName = "testName";
    auto [status, meta] = cloudServiceImpl_->GetSchemaMeta(userId, bundleName, instanceId);
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
    bundleName = TEST_CLOUD_BUNDLE;
    DistributedData::SchemaMeta schemeMeta;
    schemeMeta.bundleName = TEST_CLOUD_BUNDLE;
    schemeMeta.metaVersion = DistributedData::SchemaMeta::CURRENT_VERSION + 1;
    MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE, instanceId), schemeMeta, true);
    std::tie(status, meta) = cloudServiceImpl_->GetSchemaMeta(userId, bundleName, instanceId);
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
    schemeMeta.metaVersion = DistributedData::SchemaMeta::CURRENT_VERSION;
    MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE, instanceId), schemeMeta, true);
    std::tie(status, meta) = cloudServiceImpl_->GetSchemaMeta(userId, bundleName, instanceId);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_EQ(meta.metaVersion, DistributedData::SchemaMeta::CURRENT_VERSION);
    MetaDataManager::GetInstance().DelMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE, instanceId), true);
    MetaDataManager::GetInstance().DelMeta(cloudInfo.GetKey(), true);
}

/**
* @tc.name: GetAppSchemaFromServer
* @tc.desc: Test the GetAppSchemaFromServer function input parameters of different parameters
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, GetAppSchemaFromServer, TestSize.Level0)
{
    int32_t userId = CloudServerMock::INVALID_USER_ID;
    std::string bundleName;
    delegate_.isNetworkAvailable_ = false;
    auto [status, meta] = cloudServiceImpl_->GetAppSchemaFromServer(userId, bundleName);
    EXPECT_EQ(status, CloudData::CloudService::NETWORK_ERROR);
    delegate_.isNetworkAvailable_ = true;
    std::tie(status, meta) = cloudServiceImpl_->GetAppSchemaFromServer(userId, bundleName);
    EXPECT_EQ(status, CloudData::CloudService::SCHEMA_INVALID);
    userId = 100;
    std::tie(status, meta) = cloudServiceImpl_->GetAppSchemaFromServer(userId, bundleName);
    EXPECT_EQ(status, CloudData::CloudService::SCHEMA_INVALID);
    bundleName = TEST_CLOUD_BUNDLE;
    std::tie(status, meta) = cloudServiceImpl_->GetAppSchemaFromServer(userId, bundleName);
    EXPECT_EQ(status, CloudData::CloudService::SUCCESS);
    EXPECT_EQ(meta.bundleName, schemaMeta_.bundleName);
}

/**
* @tc.name: OnAppUninstall
* @tc.desc: Test the OnAppUninstall function delete the subscription data
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, OnAppUninstall, TestSize.Level0)
{
    CloudData::CloudServiceImpl::CloudStatic cloudStatic;
    int32_t userId = 1001;
    Subscription sub;
    sub.expiresTime.insert_or_assign(TEST_CLOUD_BUNDLE, 0);
    MetaDataManager::GetInstance().SaveMeta(Subscription::GetKey(userId), sub, true);
    CloudInfo cloudInfo;
    cloudInfo.user = userId;
    CloudInfo::AppInfo appInfo;
    cloudInfo.apps.insert_or_assign(TEST_CLOUD_BUNDLE, appInfo);
    MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true);
    int32_t index = 1;
    auto ret = cloudStatic.OnAppUninstall(TEST_CLOUD_BUNDLE, userId, index);
    EXPECT_EQ(ret, E_OK);
    Subscription sub1;
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(Subscription::GetKey(userId), sub1, true));
    EXPECT_EQ(sub1.expiresTime.size(), 0);
}

/**
* @tc.name: GetCloudInfo
* @tc.desc: Test the GetCloudInfo with invalid parameters
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, GetCloudInfo001, TestSize.Level1)
{
    int32_t userId = 1000;
    auto [status, cloudInfo] = cloudServiceImpl_->GetCloudInfo(userId);
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
    MetaDataManager::GetInstance().DelMeta(cloudInfo_.GetKey(), true);
    delegate_.isNetworkAvailable_ = false;
    std::tie(status, cloudInfo) = cloudServiceImpl_->GetCloudInfo(cloudInfo_.user);
    EXPECT_EQ(status, CloudData::CloudService::NETWORK_ERROR);
    delegate_.isNetworkAvailable_ = true;
}

/**
* @tc.name: PreShare
* @tc.desc: Test the PreShare with invalid parameters
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, PreShare, TestSize.Level0)
{
    int32_t userId = 1000;
    StoreInfo info;
    info.instanceId = 0;
    info.bundleName = TEST_CLOUD_BUNDLE;
    info.storeName = TEST_CLOUD_BUNDLE;
    info.user = userId;
    info.path = TEST_CLOUD_PATH;
    StoreMetaData meta(info);
    meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true);
    DistributedRdb::RdbQuery query;
    auto [status, cursor] = cloudServiceImpl_->PreShare(info, query);
    EXPECT_EQ(status, GeneralError::E_ERROR);
}

/**
* @tc.name: InitSubTask
* @tc.desc: Test the InitSubTask with invalid parameters
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, InitSubTask, TestSize.Level0)
{
    uint64_t minInterval = 0;
    uint64_t expire = 24 * 60 * 60 * 1000; // 24hours, ms
    ExecutorPool::TaskId taskId = 100;
    Subscription sub;
    sub.expiresTime.insert_or_assign(TEST_CLOUD_BUNDLE, expire);
    std::shared_ptr<ExecutorPool> executor = std::move(cloudServiceImpl_->executor_);
    cloudServiceImpl_->executor_ = nullptr;
    cloudServiceImpl_->InitSubTask(sub, minInterval);
    EXPECT_EQ(sub.GetMinExpireTime(), expire);
    cloudServiceImpl_->executor_ = std::move(executor);
    cloudServiceImpl_->subTask_ = taskId;
    cloudServiceImpl_->InitSubTask(sub, minInterval);
    EXPECT_NE(cloudServiceImpl_->subTask_, taskId);
    cloudServiceImpl_->subTask_ = taskId;
    cloudServiceImpl_->expireTime_ = 0;
    cloudServiceImpl_->InitSubTask(sub, minInterval);
    EXPECT_EQ(cloudServiceImpl_->subTask_, taskId);
    cloudServiceImpl_->subTask_ = ExecutorPool::INVALID_TASK_ID;
    cloudServiceImpl_->InitSubTask(sub, minInterval);
    EXPECT_NE(cloudServiceImpl_->subTask_, ExecutorPool::INVALID_TASK_ID);
}

/**
* @tc.name: DoSubscribe
* @tc.desc: Test DoSubscribe functions with invalid parameter.
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, DoSubscribe, TestSize.Level0)
{
    Subscription sub;
    sub.userId = cloudInfo_.user;
    MetaDataManager::GetInstance().SaveMeta(sub.GetKey(), sub, true);
    int user = cloudInfo_.user;
    auto status = cloudServiceImpl_->DoSubscribe(user, CloudSyncScene::ENABLE_CLOUD);
    EXPECT_FALSE(status);
    sub.id = "testId";
    MetaDataManager::GetInstance().SaveMeta(sub.GetKey(), sub, true);
    status = cloudServiceImpl_->DoSubscribe(user, CloudSyncScene::ENABLE_CLOUD);
    EXPECT_FALSE(status);
    sub.id = TEST_CLOUD_APPID;
    MetaDataManager::GetInstance().SaveMeta(sub.GetKey(), sub, true);
    status = cloudServiceImpl_->DoSubscribe(user, CloudSyncScene::ENABLE_CLOUD);
    EXPECT_FALSE(status);
    MetaDataManager::GetInstance().DelMeta(cloudInfo_.GetKey(), true);
    status = cloudServiceImpl_->DoSubscribe(user, CloudSyncScene::ENABLE_CLOUD);
    EXPECT_FALSE(status);
}

/**
* @tc.name: Report
* @tc.desc: Test Report.
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, Report, TestSize.Level0)
{
    auto cloudReport = std::make_shared<DistributedData::CloudReport>();
    auto prepareTraceId = cloudReport->GetPrepareTraceId(100);
    EXPECT_EQ(prepareTraceId, "");
    auto requestTraceId = cloudReport->GetRequestTraceId(100);
    EXPECT_EQ(requestTraceId, "");
    ReportParam reportParam{ 100, TEST_CLOUD_BUNDLE };
    auto ret = cloudReport->Report(reportParam);
    EXPECT_TRUE(ret);
}

/**
* @tc.name: IsOn
* @tc.desc: Test IsOn.
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, IsOn, TestSize.Level0)
{
    auto cloudServerMock = std::make_shared<CloudServerMock>();
    auto user = AccountDelegate::GetInstance()->GetUserByToken(OHOS::IPCSkeleton::GetCallingTokenID());
    auto [status, cloudInfo] = cloudServerMock->GetServerInfo(user, true);
    int32_t instanceId = 0;
    auto ret = cloudInfo.IsOn("", instanceId);
    EXPECT_FALSE(ret);
}

/**
* @tc.name: IsAllSwitchOff
* @tc.desc: Test IsAllSwitchOff.
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, IsAllSwitchOff, TestSize.Level0)
{
    auto cloudServerMock = std::make_shared<CloudServerMock>();
    auto user = AccountDelegate::GetInstance()->GetUserByToken(OHOS::IPCSkeleton::GetCallingTokenID());
    auto [status, cloudInfo] = cloudServerMock->GetServerInfo(user, true);
    auto ret = cloudInfo.IsAllSwitchOff();
    EXPECT_FALSE(ret);
}

/**
* @tc.name: GetMinExpireTime
* @tc.desc: Test GetMinExpireTime.
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, GetMinExpireTime, TestSize.Level0)
{
    uint64_t expire = 0;
    Subscription sub;
    sub.expiresTime.insert_or_assign(TEST_CLOUD_BUNDLE, expire);
    sub.GetMinExpireTime();
    expire = 24 * 60 * 60 * 1000;
    sub.expiresTime.insert_or_assign(TEST_CLOUD_BUNDLE, expire);
    expire = 24 * 60 * 60;
    sub.expiresTime.insert_or_assign("test_cloud_bundleName1", expire);
    EXPECT_EQ(sub.GetMinExpireTime(), expire);
}

 /**
* @tc.name: GetTableNames
* @tc.desc: Test GetTableNames.
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataTest, GetTableNames, TestSize.Level0)
{
    SchemaMeta::Database database;
    SchemaMeta::Table table;
    table.name = "test_cloud_table_name";
    table.alias = "test_cloud_table_alias";
    table.sharedTableName = "test_share_table_name";
    database.tables.emplace_back(table);
    auto tableNames = database.GetTableNames();
    EXPECT_EQ(tableNames.size(), 2);
}

/**
* @tc.name: BlobToAssets
* @tc.desc: cloud_data_translate BlobToAsset error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(CloudDataTest, BlobToAssets, TestSize.Level1)
{
    CloudData::RdbCloudDataTranslate rdbTranslate;
    DistributedDB::Asset asset = {
        .name = "",
        .assetId = "",
        .subpath = "",
        .uri = "",
        .modifyTime = "",
        .createTime = "",
        .size = "",
        .hash = ""
    };
    std::vector<uint8_t> blob;
    auto result = rdbTranslate.BlobToAsset(blob);
    EXPECT_EQ(result, asset);

    DistributedDB::Assets assets;
    blob = rdbTranslate.AssetsToBlob(assets);
    auto results = rdbTranslate.BlobToAssets(blob);
    EXPECT_EQ(results, assets);
}

/**
* @tc.name: GetPriorityLevel001
* @tc.desc: GetPriorityLevel test
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudDataTest, GetPriorityLevel001, TestSize.Level1)
{
    EventCenter::GetInstance().Subscribe(CloudEvent::LOCAL_CHANGE, [](const Event &event) {
        auto &evt = static_cast<const SyncEvent &>(event);
        auto mode = evt.GetMode();
        EXPECT_EQ(GeneralStore::GetPriorityLevel(GeneralStore::GetHighMode(static_cast<uint32_t>(mode))), 2);
    });
    DistributedRdb::RdbServiceImpl rdbServiceImpl;
    DistributedRdb::RdbSyncerParam param{ .bundleName_ = TEST_CLOUD_BUNDLE, .storeName_ = TEST_CLOUD_STORE };
    DistributedRdb::RdbService::Option option{ .mode = GeneralStore::SyncMode::CLOUD_CLOUD_FIRST, .isAsync = true };
    DistributedRdb::PredicatesMemo memo;
    memo.tables_ = { TEST_CLOUD_TABLE };
    rdbServiceImpl.DoCloudSync(param, option, memo, nullptr);
}

/**
* @tc.name: GetPriorityLevel002
* @tc.desc: GetPriorityLevel test
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudDataTest, GetPriorityLevel002, TestSize.Level1)
{
    EventCenter::GetInstance().Subscribe(CloudEvent::LOCAL_CHANGE, [](const Event &event) {
        auto &evt = static_cast<const SyncEvent &>(event);
        auto mode = evt.GetMode();
        EXPECT_EQ(GeneralStore::GetPriorityLevel(GeneralStore::GetHighMode(static_cast<uint32_t>(mode))), 0);
    });
    DistributedRdb::RdbServiceImpl rdbServiceImpl;
    DistributedRdb::RdbSyncerParam param{ .bundleName_ = TEST_CLOUD_BUNDLE, .storeName_ = TEST_CLOUD_STORE };
    DistributedRdb::RdbService::Option option{ .mode = GeneralStore::SyncMode::CLOUD_TIME_FIRST, .isAsync = true };
    DistributedRdb::PredicatesMemo memo;
    memo.tables_ = { TEST_CLOUD_TABLE };
    rdbServiceImpl.DoCloudSync(param, option, memo, nullptr);
}

/**
* @tc.name: GetPriorityLevel003
* @tc.desc: GetPriorityLevel test
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudDataTest, GetPriorityLevel003, TestSize.Level1)
{
    EventCenter::GetInstance().Subscribe(CloudEvent::LOCAL_CHANGE, [](const Event &event) {
        auto &evt = static_cast<const SyncEvent &>(event);
        auto mode = evt.GetMode();
        EXPECT_EQ(GeneralStore::GetPriorityLevel(GeneralStore::GetHighMode(static_cast<uint32_t>(mode))), 0);
    });
    DistributedRdb::RdbServiceImpl rdbServiceImpl;
    DistributedRdb::RdbSyncerParam param{ .bundleName_ = TEST_CLOUD_BUNDLE, .storeName_ = TEST_CLOUD_STORE };
    DistributedRdb::RdbService::Option option{ .mode = GeneralStore::SyncMode::CLOUD_CLOUD_FIRST, .isAsync = true };
    DistributedRdb::PredicatesMemo memo;
    rdbServiceImpl.DoCloudSync(param, option, memo, nullptr);
}

/**
* @tc.name: GetPriorityLevel004
* @tc.desc: GetPriorityLevel test
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudDataTest, GetPriorityLevel004, TestSize.Level1)
{
    EventCenter::GetInstance().Subscribe(CloudEvent::LOCAL_CHANGE, [](const Event &event) {
        auto &evt = static_cast<const SyncEvent &>(event);
        auto mode = evt.GetMode();
        EXPECT_EQ(GeneralStore::GetPriorityLevel(GeneralStore::GetHighMode(static_cast<uint32_t>(mode))), 1);
    });
    DistributedRdb::RdbServiceImpl rdbServiceImpl;
    DistributedRdb::RdbSyncerParam param{ .bundleName_ = TEST_CLOUD_BUNDLE, .storeName_ = TEST_CLOUD_STORE };
    DistributedRdb::RdbService::Option option{ .mode = GeneralStore::SyncMode::CLOUD_CLOUD_FIRST,
        .seqNum = 0,
        .isAsync = true,
        .isAutoSync = true };
    DistributedRdb::PredicatesMemo memo;
    rdbServiceImpl.DoCloudSync(param, option, memo, nullptr);
}

/**
* @tc.name: UpdateSchemaFromHap001
* @tc.desc: Test the UpdateSchemaFromHap with invalid user
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudDataTest, UpdateSchemaFromHap001, TestSize.Level1)
{
    ASSERT_NE(cloudServiceImpl_, nullptr);
    CloudData::CloudServiceImpl::HapInfo info = { .instIndex = 0, .bundleName = TEST_CLOUD_BUNDLE, .user = -1 };
    auto ret = cloudServiceImpl_->UpdateSchemaFromHap(info);
    EXPECT_EQ(ret, Status::ERROR);
}

/**
* @tc.name: UpdateSchemaFromHap002
* @tc.desc: Test the UpdateSchemaFromHap with invalid bundleName
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudDataTest, UpdateSchemaFromHap002, TestSize.Level1)
{
    ASSERT_NE(cloudServiceImpl_, nullptr);
    CloudData::CloudServiceImpl::HapInfo info = { .instIndex = 0, .bundleName = "", .user = cloudInfo_.user };
    auto ret = cloudServiceImpl_->UpdateSchemaFromHap(info);
    EXPECT_EQ(ret, Status::ERROR);
}

/**
* @tc.name: UpdateSchemaFromHap003
* @tc.desc: Test the UpdateSchemaFromHap with the schema application is not configured
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudDataTest, UpdateSchemaFromHap003, TestSize.Level1)
{
    ASSERT_NE(cloudServiceImpl_, nullptr);
    CloudData::CloudServiceImpl::HapInfo info = {
        .instIndex = 0, .bundleName = TEST_CLOUD_BUNDLE, .user = cloudInfo_.user
    };
    auto ret = cloudServiceImpl_->UpdateSchemaFromHap(info);
    EXPECT_EQ(ret, Status::SUCCESS);
    SchemaMeta schemaMeta;
    std::string schemaKey = CloudInfo::GetSchemaKey(info.user, info.bundleName, info.instIndex);
    ASSERT_TRUE(MetaDataManager::GetInstance().LoadMeta(schemaKey, schemaMeta, true));
    EXPECT_EQ(schemaMeta.version, schemaMeta_.version);
}

/**
* @tc.name: UpdateSchemaFromHap004
* @tc.desc: Test the UpdateSchemaFromHap with valid parameter
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudDataTest, UpdateSchemaFromHap004, TestSize.Level1)
{
    ASSERT_NE(cloudServiceImpl_, nullptr);
    CloudInfo::AppInfo exampleAppInfo;
    exampleAppInfo.bundleName = COM_EXAMPLE_TEST_CLOUD;
    exampleAppInfo.appId = COM_EXAMPLE_TEST_CLOUD;
    exampleAppInfo.version = 1;
    exampleAppInfo.cloudSwitch = true;
    CloudInfo cloudInfo;
    MetaDataManager::GetInstance().LoadMeta(cloudInfo_.GetKey(), cloudInfo, true);
    cloudInfo.apps[COM_EXAMPLE_TEST_CLOUD] = std::move(exampleAppInfo);
    MetaDataManager::GetInstance().SaveMeta(cloudInfo_.GetKey(), cloudInfo, true);
    CloudData::CloudServiceImpl::HapInfo info = {
        .instIndex = 0, .bundleName = COM_EXAMPLE_TEST_CLOUD, .user = cloudInfo_.user
    };
    auto ret = cloudServiceImpl_->UpdateSchemaFromHap(info);
    EXPECT_EQ(ret, Status::SUCCESS);
    SchemaMeta schemaMeta;
    std::string schemaKey = CloudInfo::GetSchemaKey(info.user, info.bundleName, info.instIndex);
    ASSERT_TRUE(MetaDataManager::GetInstance().LoadMeta(schemaKey, schemaMeta, true));
    EXPECT_EQ(schemaMeta.version, SCHEMA_VERSION);
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