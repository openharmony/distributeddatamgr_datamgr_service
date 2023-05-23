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
#include "log_print.h"
#include "ipc_skeleton.h"
#include "account/account_delegate.h"
#include "cloud/cloud_event.h"
#include "cloud/cloud_server.h"
#include "communicator/device_manager_adapter.h"
#include "device_matrix.h"
#include "eventcenter/event_center.h"
#include "feature/feature_system.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "mock/db_store_mock.h"
#include "rdb_types.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;

namespace OHOS::Test {
namespace DistributedDataTest {
static constexpr const char *TEST_CLOUD_BUNDLE = "test_cloud_bundleName";
static constexpr const char *TEST_CLOUD_APPID = "test_cloud_appid";
static constexpr const char *TEST_CLOUD_STORE = "test_cloud_database_name";
class CloudDataTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    static constexpr const char *TEST_DISTRIBUTEDDATA_BUNDLE = "test_distributeddata";
    static constexpr const char *TEST_DISTRIBUTEDDATA_STORE = "test_service_meta";

    void InitMetaData();
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    StoreMetaData metaData_;
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
    cloudInfo.id = "test_cloud_id";
    cloudInfo.remainSpace = REMAINSPACE;
    cloudInfo.totalSpace = TATALSPACE;
    cloudInfo.enableCloud = true;

    CloudInfo::AppInfo appInfo;
    appInfo.bundleName = TEST_CLOUD_BUNDLE;
    appInfo.appId = TEST_CLOUD_APPID;
    appInfo.version = 1;
    appInfo.cloudSwitch = true;

    cloudInfo.apps.emplace_back(std::move(appInfo));
    return cloudInfo;
}

SchemaMeta CloudServerMock::GetAppSchema(int32_t userId, const std::string &bundleName)
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
    database.alias = "test_cloud_database_alias";
    database.tables.emplace_back(table);

    SchemaMeta schemaMeta;
    schemaMeta.version = 1;
    schemaMeta.databases.emplace_back(database);

    return schemaMeta;
}

std::shared_ptr<DBStoreMock> CloudDataTest::dbStoreMock_ = std::make_shared<DBStoreMock>();

void CloudDataTest::InitMetaData()
{
    metaData_.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData_.appId = TEST_DISTRIBUTEDDATA_BUNDLE;
    metaData_.bundleName = TEST_DISTRIBUTEDDATA_BUNDLE;
    metaData_.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    metaData_.user = std::to_string(DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId));
    metaData_.area = OHOS::DistributedKv::EL1;
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.storeType = 1;
    metaData_.storeId = TEST_DISTRIBUTEDDATA_STORE;
    PolicyValue value;
    value.type = OHOS::DistributedKv::PolicyType::IMMEDIATE_SYNC_ON_ONLINE;
}

void CloudDataTest::SetUpTestCase(void)
{
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, [](const auto &, auto) {
        DeviceMatrix::GetInstance().OnChanged(DeviceMatrix::META_STORE_MASK);
    });

    auto cloudServerMock = new CloudServerMock();
    ASSERT_TRUE(CloudServer::RegisterCloudInstance(cloudServerMock));
    FeatureSystem::GetInstance().GetCreator("cloud")();
    FeatureSystem::GetInstance().GetCreator("relational_store")();
}

void CloudDataTest::TearDownTestCase() {}

void CloudDataTest::SetUp()
{
    InitMetaData();
    MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_);

    StoreMetaData storeMetaData;
    storeMetaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    storeMetaData.bundleName = TEST_CLOUD_BUNDLE;
    storeMetaData.storeId = TEST_CLOUD_STORE;
    storeMetaData.instanceId = 0;
    storeMetaData.isAutoSync = true;
    storeMetaData.storeType = DistributedRdb::RDB_DEVICE_COLLABORATION;
    storeMetaData.area = OHOS::DistributedKv::EL1;
    storeMetaData.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    storeMetaData.user =
        std::to_string(DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(storeMetaData.tokenId));
    MetaDataManager::GetInstance().SaveMeta(storeMetaData.GetKey(), storeMetaData);
}

void CloudDataTest::TearDown() {}

/**
* @tc.name: GetSchema
* @tc.desc: GetSchema from cloud.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(CloudDataTest, GetSchema, TestSize.Level0)
{
    ZLOGI("CloudDataTest start");
    std::shared_ptr<CloudServerMock> cloudServerMock = std::make_shared<CloudServerMock>();
    auto cloudInfo = cloudServerMock->GetServerInfo(
        DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(OHOS::IPCSkeleton::GetCallingTokenID()));
    ASSERT_TRUE(MetaDataManager::GetInstance().DelMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), true));
    StoreMetaData storeMetaData;
    ASSERT_FALSE(
        MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), storeMetaData, true));
    CloudEvent::StoreInfo storeInfo { OHOS::IPCSkeleton::GetCallingTokenID(), TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE, 0 };
    auto event = std::make_unique<CloudEvent>(CloudEvent::GET_SCHEMA, std::move(storeInfo), "test_service");
    EventCenter::GetInstance().PostEvent(move(event));
    ASSERT_TRUE(
        MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), storeMetaData, true));
}
} // namespace DistributedDataTest
} // namespace OHOS::Test
