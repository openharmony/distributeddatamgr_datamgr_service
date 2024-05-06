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

#include "account/account_delegate.h"
#include "cloud/cloud_event.h"
#include "cloud/cloud_server.h"
#include "cloud/schema_meta.h"
#include "cloud_service_impl.h"
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
#include "rdb_types.h"
#include "store/auto_cache.h"
#include "store/store_info.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;

namespace OHOS::Test {
namespace DistributedDataTest {
static constexpr const char *TEST_CLOUD_BUNDLE = "test_cloud_bundleName";
static constexpr const char *TEST_CLOUD_APPID = "test_cloud_appid";
static constexpr const char *TEST_CLOUD_STORE = "test_cloud_store";
static constexpr const char *TEST_CLOUD_ID = "test_cloud_id";
static constexpr const char *TEST_CLOUD_DATABASE_ALIAS_1 = "test_cloud_database_alias_1";
static constexpr const char *TEST_CLOUD_DATABASE_ALIAS_2 = "test_cloud_database_alias_2";
class CloudDataTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static SchemaMeta schemaMeta_;

protected:
    static void InitMetaData();
    static void InitSchemaMeta();
    static void InitCloudInfo();
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static StoreMetaData metaData_;
    static CloudInfo cloudInfo_;
};

class CloudServerMock : public CloudServer {
public:
    CloudInfo GetServerInfo(int32_t userId) override;
    SchemaMeta GetAppSchema(int32_t userId, const std::string &bundleName) override;
    virtual ~CloudServerMock() = default;
    static constexpr uint64_t REMAINSPACE = 1000;
    static constexpr uint64_t TATALSPACE = 2000;
};

CloudInfo CloudServerMock::GetServerInfo(int32_t userId)
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
    return cloudInfo;
}

SchemaMeta CloudServerMock::GetAppSchema(int32_t userId, const std::string &bundleName)
{
    return CloudDataTest::schemaMeta_;
}

std::shared_ptr<DBStoreMock> CloudDataTest::dbStoreMock_ = std::make_shared<DBStoreMock>();
SchemaMeta CloudDataTest::schemaMeta_;
StoreMetaData CloudDataTest::metaData_;
CloudInfo CloudDataTest::cloudInfo_;

void CloudDataTest::InitMetaData()
{
    metaData_.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData_.appId = TEST_CLOUD_APPID;
    metaData_.bundleName = TEST_CLOUD_BUNDLE;
    metaData_.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    metaData_.user = std::to_string(DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId));
    metaData_.area = OHOS::DistributedKv::EL1;
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.storeType = DistributedRdb::RDB_DEVICE_COLLABORATION;
    metaData_.storeId = TEST_CLOUD_STORE;
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
}

void CloudDataTest::InitCloudInfo()
{
    cloudInfo_.user = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(IPCSkeleton::GetCallingTokenID());
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
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr);
    MetaDataManager::GetInstance().SetSyncer(
        [](const auto &, auto) { DeviceMatrix::GetInstance().OnChanged(DeviceMatrix::META_STORE_MASK); });

    auto cloudServerMock = new CloudServerMock();
    CloudServer::RegisterCloudInstance(cloudServerMock);
    FeatureSystem::GetInstance().GetCreator("cloud")();
    FeatureSystem::GetInstance().GetCreator("relational_store")();

    InitCloudInfo();
    InitMetaData();
    InitSchemaMeta();
}

void CloudDataTest::TearDownTestCase()
{
}

void CloudDataTest::SetUp()
{
}

void CloudDataTest::TearDown()
{
}

/**
* @tc.name: GetSchema
* @tc.desc: GetSchema from cloud when no schema in meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(CloudDataTest, GetSchema, TestSize.Level0)
{
    ZLOGI("CloudDataTest start");
    auto cloudServerMock = std::make_shared<CloudServerMock>();
    auto user = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(OHOS::IPCSkeleton::GetCallingTokenID());
    auto cloudInfo = cloudServerMock->GetServerInfo(user);
    ASSERT_TRUE(MetaDataManager::GetInstance().DelMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), true));
    SchemaMeta schemaMeta;
    ASSERT_FALSE(MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), schemaMeta, true));
    StoreInfo storeInfo{ OHOS::IPCSkeleton::GetCallingTokenID(), TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE, 0 };
    auto event = std::make_unique<CloudEvent>(CloudEvent::GET_SCHEMA, storeInfo);
    EventCenter::GetInstance().PostEvent(std::move(event));
    ASSERT_FALSE(MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), schemaMeta, true));
}

/**
* @tc.name: QueryStatistics
* @tc.desc: The query interface failed because cloudInfo could not be found from the metadata.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudDataTest, QueryStatistics001, TestSize.Level0)
{
    ZLOGI("CloudDataTest QueryStatistics001 start");
    // prepare MetaDta
    MetaDataManager::GetInstance().DelMeta(cloudInfo_.GetKey(), true);

    auto cloudServiceImpl = std::make_shared<CloudData::CloudServiceImpl>();
    auto [status, result] = cloudServiceImpl->QueryStatistics(TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, "");
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
    ZLOGI("CloudDataTest QueryStatistics002 start");
    // prepare MetaDta
    MetaDataManager::GetInstance().DelMeta(cloudInfo_.GetSchemaKey(TEST_CLOUD_BUNDLE), true);
    MetaDataManager::GetInstance().SaveMeta(cloudInfo_.GetKey(), cloudInfo_, true);

    auto cloudServiceImpl = std::make_shared<CloudData::CloudServiceImpl>();
    auto [status, result] = cloudServiceImpl->QueryStatistics("", "", "");
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
    EXPECT_TRUE(result.empty());
    std::tie(status, result) = cloudServiceImpl->QueryStatistics(TEST_CLOUD_ID, "", "");
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
    EXPECT_TRUE(result.empty());
    std::tie(status, result) = cloudServiceImpl->QueryStatistics(TEST_CLOUD_ID, TEST_CLOUD_STORE, "");
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
    EXPECT_TRUE(result.empty());
    std::tie(status, result) = cloudServiceImpl->QueryStatistics(TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, "");
    EXPECT_EQ(status, CloudData::CloudService::ERROR);
    EXPECT_TRUE(result.empty());
}

/**
* @tc.name: QueryStatistics
* @tc.desc: Query the statistics of cloud records in a specified database.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudDataTest, QueryStatistics003, TestSize.Level0)
{
    ZLOGI("CloudDataTest QueryStatistics003 start");
    // prepare MetaDta
    MetaDataManager::GetInstance().SaveMeta(cloudInfo_.GetKey(), cloudInfo_, true);
    MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true);
    MetaDataManager::GetInstance().SaveMeta(cloudInfo_.GetSchemaKey(TEST_CLOUD_BUNDLE), schemaMeta_, true);

    // Construct the statisticInfo data
    auto creator = [](const StoreMetaData &metaData) -> GeneralStore * {
        auto store = new (std::nothrow) GeneralStoreMock();
        if (store != nullptr) {
            std::map<std::string, Value> entry = { { "inserted", 1 }, { "updated", 2 }, { "normal", 3 } };
            store->MakeCursor(entry);
        }
        return store;
    };
    AutoCache::GetInstance().RegCreator(DistributedRdb::RDB_DEVICE_COLLABORATION, creator);

    auto cloudServiceImpl = std::make_shared<CloudData::CloudServiceImpl>();
    auto [status, result] =
        cloudServiceImpl->QueryStatistics(TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, TEST_CLOUD_DATABASE_ALIAS_1);
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
HWTEST_F(CloudDataTest, QueryStatistics004, TestSize.Level0)
{
    ZLOGI("CloudDataTest QueryStatistics004 start");
    // prepare MetaDta
    MetaDataManager::GetInstance().SaveMeta(cloudInfo_.GetKey(), cloudInfo_, true);
    MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true);
    MetaDataManager::GetInstance().SaveMeta(cloudInfo_.GetSchemaKey(TEST_CLOUD_BUNDLE), schemaMeta_, true);

    // Construct the statisticInfo data
    auto creator = [](const StoreMetaData &metaData) -> GeneralStore * {
        auto store = new (std::nothrow) GeneralStoreMock();
        if (store != nullptr) {
            std::map<std::string, Value> entry = { { "inserted", 1 }, { "updated", 2 }, { "normal", 3 } };
            store->MakeCursor(entry);
        }
        return store;
    };
    AutoCache::GetInstance().RegCreator(DistributedRdb::RDB_DEVICE_COLLABORATION, creator);

    auto cloudServiceImpl = std::make_shared<CloudData::CloudServiceImpl>();
    auto [status, result] = cloudServiceImpl->QueryStatistics(TEST_CLOUD_ID, TEST_CLOUD_BUNDLE, "");
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
} // namespace DistributedDataTest
} // namespace OHOS::Test
