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
#include "account/account_delegate.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "cloud/cloud_server.h"
#include "gtest/gtest.h"
#include "cloud/cloud_event.h"
#include "eventcenter/event_center.h"
#include "communicator/device_manager_adapter.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
class CloudServerMock : public CloudServer {
public:
    CloudInfo GetServerInfo(int32_t userId) override;
    SchemaMeta GetAppSchema(int32_t userId, const std::string &bundleName) override;
    static constexpr uint64_t REMAINSPACE = 1000;
    static constexpr uint64_t TATALSPACE = 2000;
};

class CloudDataTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        auto cloudServerMock = new CloudServerMock();
        ASSERT_TRUE(CloudServer::RegisterCloudInstance(cloudServerMock));
    }
    static void TearDownTestCase(void)
    {
    }
    void SetUp()
    {
    }
    void TearDown()
    {
    }
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
    appInfo.bundleName = "test_cloud_bundleName";
    appInfo.appId = "test_cloud_appid";
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
    field2.alias  = "test_cloud_field_alias2";

    SchemaMeta::Table table;
    table.name = "test_cloud_table_name";
    table.alias = "test_cloud_table_alias";
    table.fields.emplace_back(field1);
    table.fields.emplace_back(field2);

    SchemaMeta::Database database;
    database.name = "test_cloud_database_name";
    database.alias = "test_cloud_database_alias";
    database.tables.emplace_back(table);

    SchemaMeta schemaMeta;
    schemaMeta.version = 1;
    schemaMeta.databases.emplace_back(database);

    return schemaMeta;
}

HWTEST_F(CloudDataTest, GlobalConfig, TestSize.Level0)
{
    CloudEvent::StoreInfo storeInfo{ 0, "test_cloud_bundleName", "test_cloud_database_name", 0, 1 };
    auto event = std::make_unique<CloudEvent>(CloudEvent::GET_SCHEMA, std::move(storeInfo), "test_service");
    EventCenter::GetInstance().PostEvent(move(event));
    StoreMetaData storeMetaData;
    storeMetaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    storeMetaData.user = OHOS::DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(0);
    storeMetaData.bundleName = "test_cloud_bundleName";
    storeMetaData.storeId = "test_cloud_database_name";
    storeMetaData.instanceId = 0;
    ASSERT_TRUE(MetaDataManager::GetInstance().DelMeta(storeMetaData.GetKey(), true));
    ASSERT_TRUE(MetaDataManager::GetInstance().LoadMeta(storeMetaData.GetKey(), storeMetaData, true));
}

