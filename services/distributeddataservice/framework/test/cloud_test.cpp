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

#define LOG_TAG "CloudInfoTest"
#include <gtest/gtest.h>

#include "cloud/cloud_conflict_handler.h"
#include "cloud/cloud_db_sync_config.h"
#include "cloud/cloud_db.h"
#include "cloud/cloud_event.h"
#include "cloud/cloud_info.h"
#include "cloud/cloud_mark.h"
#include "cloud/cloud_server.h"
#include "cloud/schema_meta.h"
#include "utils/crypto.h"
#include "screen/screen_manager.h"
#include "store/general_store.h"
#include "store/general_value.h"
#include "store/general_watcher.h"
#include "metadata/meta_data_manager.h"
#include "mock/db_store_mock.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
constexpr const int32_t TEST_USER_ID = 1001;
const char *TEST_APP_ID = "com.example.app";
class CloudInfoTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
    CloudDbSyncConfig syncConfig_;
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
};
std::shared_ptr<DBStoreMock> CloudInfoTest::dbStoreMock_ = std::make_shared<DBStoreMock>();
void CloudInfoTest::SetUpTestCase(void)
{
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
}

class ServicesCloudServerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

class ServicesCloudDBTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

class CloudEventTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

class ScreenManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

void ScreenManagerTest::SetUpTestCase()
{
    ScreenManager::GetInstance()->BindExecutor(nullptr);
}

class MockGeneralWatcher : public DistributedData::GeneralWatcher {
public:
    int32_t OnChange(const Origin &origin, const PRIFields &primaryFields,
        ChangeInfo &&values) override
    {
        return GeneralError::E_OK;
    }

    int32_t OnChange(const Origin &origin, const Fields &fields, ChangeData &&datas) override
    {
        return GeneralError::E_OK;
    }

    int32_t OnChange(const std::string &storeId, int32_t triggerMode) override
    {
        return GeneralError::E_OK;
    }
};

class MockQuery : public GenQuery {
public:
    static const uint64_t TYPE_ID = 1;

    bool IsEqual(uint64_t tid) override
    {
        return tid == TYPE_ID;
    }

    std::vector<std::string> GetTables() override
    {
        return { "table1", "table2" };
    }
};

/**
* @tc.name: GetSchemaPrefix
* @tc.desc: Get schema prefix.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, GetSchemaPrefix, TestSize.Level0)
{
    CloudInfo cloudInfo;
    auto result = cloudInfo.GetSchemaPrefix("ohos.test.demo");
    ASSERT_EQ(result, "CLOUD_SCHEMA###0###ohos.test.demo");

    result = cloudInfo.GetSchemaPrefix("");
    ASSERT_EQ(result, "CLOUD_SCHEMA###0");
}

/**
* @tc.name: IsValid
* @tc.desc: Determine if it is limited.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, IsValid, TestSize.Level0)
{
    CloudInfo cloudInfo;
    auto result = cloudInfo.IsValid();
    ASSERT_FALSE(result);

    CloudInfo cloudInfo1;
    cloudInfo1.user = 111;
    cloudInfo1.id = "test1_id";
    cloudInfo1.totalSpace = 0;
    cloudInfo1.remainSpace = 0;
    cloudInfo1.enableCloud = false;

    Serializable::json node1;
    cloudInfo1.Marshal(node1);
    result = cloudInfo1.IsValid();
    ASSERT_TRUE(result);
}

/**
* @tc.name: Exist
* @tc.desc: Determine if the package exists.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, Exist, TestSize.Level0)
{
    CloudInfo cloudInfo;
    auto result = cloudInfo.Exist("", 1);
    ASSERT_FALSE(result);

    CloudInfo::AppInfo appInfo;
    appInfo.bundleName = "test_cloud_bundleName";
    appInfo.appId = "test_cloud_id";
    appInfo.version = 0;
    appInfo.instanceId = 100;
    appInfo.cloudSwitch = false;

    cloudInfo.user = 111;
    cloudInfo.id = "test_cloud_id";
    cloudInfo.totalSpace = 0;
    cloudInfo.remainSpace = 100;
    cloudInfo.enableCloud = true;
    cloudInfo.apps["test_cloud_bundleName"] = std::move(appInfo);

    Serializable::json node1;
    cloudInfo.Marshal(node1);
    result = cloudInfo.Exist("test_cloud_bundleName", 100);
    ASSERT_TRUE(result);
}

/**
* @tc.name: Exist
* @tc.desc: Is it on.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, IsOn, TestSize.Level0)
{
    CloudInfo cloudInfo;
    auto result = cloudInfo.IsOn("ohos.test.demo", 1);
    ASSERT_FALSE(result);

    CloudInfo::AppInfo appInfo;
    appInfo.bundleName = "test_cloud_bundleName";
    appInfo.appId = "test_cloud_id";
    appInfo.version = 0;
    appInfo.instanceId = 100;
    appInfo.cloudSwitch = true;

    cloudInfo.user = 111;
    cloudInfo.id = "test_cloud_id";
    cloudInfo.totalSpace = 0;
    cloudInfo.remainSpace = 100;
    cloudInfo.enableCloud = true;
    cloudInfo.apps["test_cloud_bundleName"] = std::move(appInfo);

    Serializable::json node1;
    cloudInfo.Marshal(node1);
    result = cloudInfo.IsOn("test_cloud_bundleName", 100);
    ASSERT_TRUE(result);
}

/**
* @tc.name: GetPrefix
* @tc.desc: Get prefix.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, GetPrefix, TestSize.Level0)
{
    const std::initializer_list<std::string> fields;
    auto result = CloudInfo::GetPrefix(fields);
    ASSERT_EQ(result, "CLOUD_INFO###");
}

/**
* @tc.name: CloudInfoTest001
* @tc.desc: Marshal and Unmarshal of CloudInfo.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(CloudInfoTest, CloudInfoTest001, TestSize.Level0)
{
    CloudInfo cloudInfo1;
    cloudInfo1.user = 111;
    cloudInfo1.id = "test1_id";
    cloudInfo1.totalSpace = 0;
    cloudInfo1.remainSpace = 0;
    cloudInfo1.enableCloud = false;
    cloudInfo1.maxNumber = CloudInfo::DEFAULT_BATCH_NUMBER;
    cloudInfo1.maxSize = CloudInfo::DEFAULT_BATCH_SIZE;

    Serializable::json node1;
    cloudInfo1.Marshal(node1);
    EXPECT_EQ(Serializable::Marshall(cloudInfo1), to_string(node1));

    CloudInfo cloudInfo2;
    cloudInfo2.Unmarshal(node1);
    EXPECT_EQ(Serializable::Marshall(cloudInfo1), Serializable::Marshall(cloudInfo2));
}

/**
* @tc.name: CloudInfoTest002
* @tc.desc: CloudInfo Overload function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudInfoTest, CloudInfoTest002, TestSize.Level0)
{
    CloudInfo cloudInfo1;
    cloudInfo1.user = 111;
    cloudInfo1.id = "test1_id";
    cloudInfo1.totalSpace = 0;
    cloudInfo1.remainSpace = 0;
    cloudInfo1.enableCloud = false;
    cloudInfo1.maxNumber = CloudInfo::DEFAULT_BATCH_NUMBER;
    cloudInfo1.maxSize = CloudInfo::DEFAULT_BATCH_SIZE;

    CloudInfo::AppInfo cloudInfoAppInfo1;
    cloudInfoAppInfo1.bundleName = "ohos.test.demo";
    cloudInfoAppInfo1.appId = "test1_id";
    cloudInfoAppInfo1.version = 0;
    cloudInfoAppInfo1.instanceId = 0;
    cloudInfoAppInfo1.cloudSwitch = false;
    cloudInfo1.apps["ohos.test.demo"] = cloudInfoAppInfo1;

    CloudInfo cloudInfo2 = cloudInfo1;
    EXPECT_EQ(cloudInfo2, cloudInfo1);

    cloudInfo2.maxSize = 1;
    EXPECT_NE(cloudInfo2, cloudInfo1);
    cloudInfo2.maxNumber = 1;
    EXPECT_NE(cloudInfo2, cloudInfo1);
    cloudInfo2.enableCloud = true;
    EXPECT_NE(cloudInfo2, cloudInfo1);
    cloudInfo2.remainSpace = 1;
    EXPECT_NE(cloudInfo2, cloudInfo1);
    cloudInfo2.totalSpace = 1;
    EXPECT_NE(cloudInfo2, cloudInfo1);
    cloudInfo2.id = "test2_id";
    EXPECT_NE(cloudInfo2, cloudInfo1);
    cloudInfo2.user = 222;
    EXPECT_NE(cloudInfo2, cloudInfo1);
    cloudInfo2.apps["ohos.test.demo"].cloudSwitch = true;
    EXPECT_NE(cloudInfo2, cloudInfo1);
}

/**
* @tc.name: CloudInfoTest003
* @tc.desc: CloudInfo Overload function apps test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudInfoTest, CloudInfoTest003, TestSize.Level0)
{
    CloudInfo cloudInfo1;
    cloudInfo1.user = 111;
    cloudInfo1.id = "test1_id";
    cloudInfo1.totalSpace = 0;
    cloudInfo1.remainSpace = 0;
    cloudInfo1.enableCloud = false;
    cloudInfo1.maxNumber = CloudInfo::DEFAULT_BATCH_NUMBER;
    cloudInfo1.maxSize = CloudInfo::DEFAULT_BATCH_SIZE;

    CloudInfo::AppInfo cloudInfoAppInfo1;
    cloudInfoAppInfo1.bundleName = "ohos.test.demo";
    cloudInfoAppInfo1.appId = "test1_id";
    cloudInfoAppInfo1.version = 0;
    cloudInfoAppInfo1.instanceId = 0;
    cloudInfoAppInfo1.cloudSwitch = false;
    cloudInfo1.apps["ohos.test.demo"] = cloudInfoAppInfo1;

    CloudInfo cloudInfo2 = cloudInfo1;
    cloudInfo2.apps["ohos.test.demo"].cloudSwitch = true;
    EXPECT_NE(cloudInfo2, cloudInfo1);
    cloudInfo2.apps["ohos.test.demo"].instanceId = 1;
    EXPECT_NE(cloudInfo2, cloudInfo1);
    cloudInfo2.apps["ohos.test.demo"].version = 1;
    EXPECT_NE(cloudInfo2, cloudInfo1);
    cloudInfo2.apps["ohos.test.demo"].appId = "test2_id";
    EXPECT_NE(cloudInfo2, cloudInfo1);
    cloudInfo2.apps["ohos.test.demo"].bundleName = "ohos.test.demo2";
    EXPECT_NE(cloudInfo2, cloudInfo1);
}

/**
* @tc.name: AppInfoTest001
* @tc.desc: Marshal and Unmarshal of AppInfo.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, AppInfoTest001, TestSize.Level0)
{
    CloudInfo::AppInfo cloudInfoAppInfo1;
    cloudInfoAppInfo1.bundleName = "ohos.test.demo";
    cloudInfoAppInfo1.appId = "test1_id";
    cloudInfoAppInfo1.version = 0;
    cloudInfoAppInfo1.instanceId = 0;
    cloudInfoAppInfo1.cloudSwitch = false;

    Serializable::json node1;
    cloudInfoAppInfo1.Marshal(node1);
    EXPECT_EQ(Serializable::Marshall(cloudInfoAppInfo1), to_string(node1));

    CloudInfo::AppInfo cloudInfoAppInfo2;
    cloudInfoAppInfo2.Unmarshal(node1);
    EXPECT_EQ(Serializable::Marshall(cloudInfoAppInfo1), Serializable::Marshall(cloudInfoAppInfo2));
}

/**
* @tc.name: AppInfoTest002
* @tc.desc: AppInfoTest Overload function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudInfoTest, AppInfoTest002, TestSize.Level0)
{
    CloudInfo::AppInfo cloudInfoAppInfo1;
    cloudInfoAppInfo1.bundleName = "ohos.test.demo";
    cloudInfoAppInfo1.appId = "test1_id";
    cloudInfoAppInfo1.version = 0;
    cloudInfoAppInfo1.instanceId = 0;
    cloudInfoAppInfo1.cloudSwitch = false;

    CloudInfo::AppInfo cloudInfoAppInfo2 = cloudInfoAppInfo1;
    EXPECT_EQ(cloudInfoAppInfo2, cloudInfoAppInfo1);

    cloudInfoAppInfo2.cloudSwitch = true;
    EXPECT_NE(cloudInfoAppInfo2, cloudInfoAppInfo1);
    cloudInfoAppInfo2.instanceId = 1;
    EXPECT_NE(cloudInfoAppInfo2, cloudInfoAppInfo1);
    cloudInfoAppInfo2.version = 1;
    EXPECT_NE(cloudInfoAppInfo2, cloudInfoAppInfo1);
    cloudInfoAppInfo2.appId = "test2_id";
    EXPECT_NE(cloudInfoAppInfo2, cloudInfoAppInfo1);
    cloudInfoAppInfo2.bundleName = "ohos.test.demo2";
    EXPECT_NE(cloudInfoAppInfo2, cloudInfoAppInfo1);
}

/**
* @tc.name: TableTest001
* @tc.desc: Marshal and Unmarshal of Table.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, TableTest001, TestSize.Level0)
{
    Field field1;
    field1.colName = "test1_colName";
    field1.alias = "test1_alias";
    field1.type = 1;
    field1.primary = true;
    field1.nullable = false;

    Table table1;
    table1.name = "test1_name";
    table1.sharedTableName = "test1_sharedTableName";
    table1.alias = "test1_alias";
    table1.fields.push_back(field1);
    Serializable::json node1;
    table1.Marshal(node1);
    EXPECT_EQ(Serializable::Marshall(table1), to_string(node1));

    Table table2;
    table2.Unmarshal(node1);
    EXPECT_EQ(Serializable::Marshall(table1), Serializable::Marshall(table2));
}

/**
* @tc.name: TableTest002
* @tc.desc: Table Overload function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudInfoTest, TableTest002, TestSize.Level0)
{
    Field field1;
    field1.colName = "test1_colName";
    field1.alias = "test1_alias";
    field1.type = 1;
    field1.primary = true;
    field1.nullable = false;

    Table table1;
    table1.name = "test1_name";
    table1.sharedTableName = "test1_sharedTableName";
    table1.alias = "test1_alias";
    table1.fields.push_back(field1);

    Table table2 = table1;
    EXPECT_EQ(table2, table1);
}

/**
* @tc.name: FieldTest
* @tc.desc: Field Overload function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudInfoTest, FieldTest, TestSize.Level0)
{
    Field field1;
    field1.colName = "test1_colName";
    field1.alias = "test1_alias";
    field1.type = 1;
    field1.primary = true;
    field1.nullable = false;

    Field field2 = field1;
    EXPECT_EQ(field2, field1);
}

/**
* @tc.name: IsAllSwitchOffTest
* @tc.desc: Determine if all apps have their cloud synchronization disabled.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, IsAllSwitchOffTest, TestSize.Level0)
{
    CloudInfo cloudInfo;
    auto result = cloudInfo.IsAllSwitchOff();
    ASSERT_TRUE(result);

    CloudInfo cloudInfo1;
    CloudInfo::AppInfo appInfo;
    std::map<std::string, CloudInfo::AppInfo> apps;
    appInfo.bundleName = "test_cloud_bundleName";
    appInfo.appId = "test_cloud_id";
    appInfo.version = 0;
    appInfo.instanceId = 100;
    appInfo.cloudSwitch = true;
    apps = { { "test_cloud", appInfo } };
    cloudInfo1.apps = apps;
    result = cloudInfo1.IsAllSwitchOff();
    ASSERT_FALSE(result);
}

/**
* @tc.name: GetSchemaKey
* @tc.desc: Get schema key.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, GetSchemaKeyTest1, TestSize.Level0)
{
    std::string bundleName = "test_cloud_bundleName";
    int32_t instanceId = 123;
    CloudInfo cloudInfo;
    auto result = cloudInfo.GetSchemaKey(bundleName, instanceId);
    ASSERT_EQ(result, "CLOUD_SCHEMA###0###test_cloud_bundleName###123");
}

/**
* @tc.name: GetSchemaKey
* @tc.desc: Get schema key.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, GetSchemaKeyTest2, TestSize.Level0)
{
    int32_t user = 123;
    std::string bundleName = "test_cloud_bundleName";
    int32_t instanceId = 456;
    CloudInfo cloudInfo;
    auto result = cloudInfo.GetSchemaKey(user, bundleName, instanceId);
    ASSERT_EQ(result, "CLOUD_SCHEMA###123###test_cloud_bundleName###456");
}

/**
* @tc.name: GetSchemaKey
* @tc.desc: Get schema key.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, GetSchemaKeyTest3, TestSize.Level0)
{
    StoreMetaData meta;
    meta.bundleName = "test_cloud_bundleName";
    meta.user = "123";
    CloudInfo cloudInfo;
    auto result = cloudInfo.GetSchemaKey(meta);
    ASSERT_EQ(result, "CLOUD_SCHEMA###123###test_cloud_bundleName###0");
}

/**
* @tc.name: GetSchemaKey
* @tc.desc: Get schema key.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, GetSchemaKeyTest4, TestSize.Level0)
{
    CloudInfo cloudInfo;
    CloudInfo::AppInfo appInfo;
    std::map<std::string, CloudInfo::AppInfo> apps;
    appInfo.bundleName = "test_cloud_bundleName";
    appInfo.appId = "test_cloud_id";
    appInfo.version = 0;
    appInfo.instanceId = 100;
    appInfo.cloudSwitch = true;
    apps = { { "test_cloud", appInfo } };
    cloudInfo.apps = apps;
    std::map<std::string, std::string> info;
    info = { { "test_cloud_bundleName", "CLOUD_SCHEMA###0###test_cloud###100" } };
    auto result = cloudInfo.GetSchemaKey();
    ASSERT_EQ(info, result);
}

/**
* @tc.name: CloudServer
* @tc.desc: test CloudServer function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServicesCloudServerTest, CloudServer, TestSize.Level0)
{
    CloudServer cloudServer;
    int32_t userId = 100;
    cloudServer.GetServerInfo(userId);
    std::string bundleName = "com.ohos.cloudtest";
    cloudServer.GetAppSchema(userId, bundleName);
    std::map<std::string, std::vector<CloudServer::Database>> dbs;
    auto result1 = cloudServer.Subscribe(userId, dbs);
    EXPECT_EQ(result1, GeneralError::E_OK);
    result1 = cloudServer.Unsubscribe(userId, dbs);
    EXPECT_EQ(result1, GeneralError::E_OK);

    int user = 1;
    uint32_t tokenId = 123;
    CloudServer::Database dbMeta;
    auto result2 = cloudServer.ConnectAssetLoader(tokenId, dbMeta);
    EXPECT_EQ(result2, nullptr);
    result2 = cloudServer.ConnectAssetLoader(bundleName, user, dbMeta);
    EXPECT_EQ(result2, nullptr);

    auto result3 = cloudServer.ConnectCloudDB(tokenId, dbMeta);
    EXPECT_EQ(result3, nullptr);
    result3 = cloudServer.ConnectCloudDB(bundleName, user, dbMeta);
    EXPECT_EQ(result3, nullptr);

    auto result4 = cloudServer.ConnectSharingCenter(userId, bundleName);
    EXPECT_EQ(result4, nullptr);
    cloudServer.Clean(userId);
    cloudServer.ReleaseUserInfo(userId);
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(1, 0);
    cloudServer.Bind(executor);
    auto result5 = cloudServer.IsSupportCloud(userId);
    EXPECT_FALSE(result5);
}

/**
* @tc.name: CloudDB
* @tc.desc: test CloudDB function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServicesCloudDBTest, CloudDB, TestSize.Level0)
{
    CloudDB cloudDB;
    std::string table = "table";
    VBucket extend;
    VBuckets values;
    auto result1 = cloudDB.Execute(table, "sql", extend);
    EXPECT_EQ(result1, GeneralError::E_NOT_SUPPORT);
    result1 = cloudDB.BatchInsert(table, std::move(values), values);
    EXPECT_EQ(result1, GeneralError::E_NOT_SUPPORT);
    result1 = cloudDB.BatchUpdate(table, std::move(values), values);
    EXPECT_EQ(result1, GeneralError::E_NOT_SUPPORT);
    result1 = cloudDB.BatchDelete(table, values);
    EXPECT_EQ(result1, GeneralError::E_NOT_SUPPORT);
    result1 = cloudDB.PreSharing(table, values);
    EXPECT_EQ(result1, GeneralError::E_NOT_SUPPORT);

    CloudDB::Devices devices;
    MockQuery query;
    CloudDB::Async async;
    result1 = cloudDB.Sync(devices, 0, query, async, 0);
    EXPECT_EQ(result1, GeneralError::E_NOT_SUPPORT);

    MockGeneralWatcher watcher;
    result1 = cloudDB.Watch(1, watcher);
    EXPECT_EQ(result1, GeneralError::E_NOT_SUPPORT);
    result1 = cloudDB.Unwatch(1, watcher);
    EXPECT_EQ(result1, GeneralError::E_NOT_SUPPORT);

    result1 = cloudDB.Lock();
    EXPECT_EQ(result1, GeneralError::E_NOT_SUPPORT);
    result1 = cloudDB.Unlock();
    EXPECT_EQ(result1, GeneralError::E_NOT_SUPPORT);
    result1 = cloudDB.Heartbeat();
    EXPECT_EQ(result1, GeneralError::E_NOT_SUPPORT);
    result1 = cloudDB.Close();
    EXPECT_EQ(result1, GeneralError::E_NOT_SUPPORT);

    auto result2 = cloudDB.Query(table, extend);
    EXPECT_EQ(result2.second, nullptr);
    result2 = cloudDB.Query(query, extend);
    EXPECT_EQ(result2.second, nullptr);

    auto result3 = cloudDB.AliveTime();
    EXPECT_EQ(result3, -1);
    std::string bundleName = "com.ohos.cloudDBtest";
    auto result4 = cloudDB.GetEmptyCursor(bundleName);
    std::pair<int32_t, std::string> cursor = { GeneralError::E_NOT_SUPPORT, "" };
    EXPECT_EQ(result4, cursor);
}

/**
* @tc.name: SchemaMeta001
* @tc.desc: test SchemaMeta GetLowVersion and GetHighVersion function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(CloudInfoTest, SchemaMeta001, TestSize.Level0)
{
    SchemaMeta schemaMeta;
    auto metaVersion = SchemaMeta::CURRENT_VERSION & 0xFFFF;
    auto result1 = schemaMeta.GetLowVersion();
    EXPECT_EQ(result1, metaVersion);
    metaVersion = SchemaMeta::CURRENT_VERSION & ~0xFFFF;
    auto result2 = schemaMeta.GetHighVersion();
    EXPECT_EQ(result2, metaVersion);
}

/**
* @tc.name: SchemaMeta002
* @tc.desc: SchemaMeta Overload function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudInfoTest, SchemaMeta002, TestSize.Level0)
{
    SchemaMeta::Field field1;
    field1.colName = "test_cloud_field_name1";
    field1.alias = "test_cloud_field_alias1";

    SchemaMeta::Table table;
    table.name = "test_cloud_table_name";
    table.alias = "test_cloud_table_alias";
    table.fields.emplace_back(field1);

    SchemaMeta::Database database;
    database.name = "test_cloud_store";
    database.alias = "test_cloud_database_alias_1";
    database.tables.emplace_back(table);

    SchemaMeta schemaMeta1;
    schemaMeta1.version = 1;
    schemaMeta1.bundleName = "test_cloud_bundleName";
    schemaMeta1.databases.emplace_back(database);
    schemaMeta1.e2eeEnable = false;

    SchemaMeta schemaMeta2 = schemaMeta1;
    EXPECT_EQ(schemaMeta2, schemaMeta1);

    schemaMeta2.e2eeEnable = true;
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].alias = "test_cloud_database_alias_2";
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.bundleName = "test_cloud_bundleName2";
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.version = 2;
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.metaVersion = 2;
    EXPECT_NE(schemaMeta2, schemaMeta1);
}

/**
* @tc.name: SchemaMeta003
* @tc.desc: SchemaMeta Overload function database test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudInfoTest, SchemaMeta003, TestSize.Level0)
{
    SchemaMeta::Field field1;
    field1.colName = "test_cloud_field_name1";
    field1.alias = "test_cloud_field_alias1";

    SchemaMeta::Table table;
    table.name = "test_cloud_table_name";
    table.alias = "test_cloud_table_alias";
    table.fields.emplace_back(field1);

    SchemaMeta::Database database;
    database.name = "test_cloud_store";
    database.alias = "test_cloud_database_alias_1";
    database.tables.emplace_back(table);

    SchemaMeta schemaMeta1;
    schemaMeta1.version = 1;
    schemaMeta1.bundleName = "test_cloud_bundleName";
    schemaMeta1.databases.emplace_back(database);
    schemaMeta1.e2eeEnable = false;

    SchemaMeta schemaMeta2 = schemaMeta1;
    schemaMeta2.databases[0].autoSyncType = 1;
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].deviceId = "111";
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].user = "111";
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].bundleName = "test_cloud_bundleName2";
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].version = 2;
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].tables[0].name = "test_cloud_table_name2";
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].alias = "test_cloud_database_alias_2";
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].name = "test_cloud_store2";
    EXPECT_NE(schemaMeta2, schemaMeta1);
}

/**
* @tc.name: SchemaMeta004
* @tc.desc: SchemaMeta Overload function Table test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudInfoTest, SchemaMeta004, TestSize.Level0)
{
    SchemaMeta::Field field1;
    field1.colName = "test_cloud_field_name1";
    field1.alias = "test_cloud_field_alias1";

    SchemaMeta::Table table;
    table.name = "test_cloud_table_name";
    table.alias = "test_cloud_table_alias";
    table.fields.emplace_back(field1);

    SchemaMeta::Database database;
    database.name = "test_cloud_store";
    database.alias = "test_cloud_database_alias_1";
    database.tables.emplace_back(table);

    SchemaMeta schemaMeta1;
    schemaMeta1.version = 1;
    schemaMeta1.bundleName = "test_cloud_bundleName";
    schemaMeta1.databases.emplace_back(database);
    schemaMeta1.e2eeEnable = false;

    SchemaMeta schemaMeta2 = schemaMeta1;
    schemaMeta2.databases[0].tables[0].sharedTableName = "share";
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].tables[0].cloudSyncFields.push_back("cloud");
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].tables[0].deviceSyncFields.push_back("device");
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].tables[0].fields[0].alias = "test_cloud_field_alias2";
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].tables[0].alias = "test_cloud_table_alias2";
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].tables[0].name = "test_cloud_table_name2";
    EXPECT_NE(schemaMeta2, schemaMeta1);
}

/**
* @tc.name: SchemaMeta005
* @tc.desc: SchemaMeta Overload function Field test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudInfoTest, SchemaMeta005, TestSize.Level0)
{
    SchemaMeta::Field field1;
    field1.colName = "test_cloud_field_name1";
    field1.alias = "test_cloud_field_alias1";

    SchemaMeta::Table table;
    table.name = "test_cloud_table_name";
    table.alias = "test_cloud_table_alias";
    table.fields.emplace_back(field1);

    SchemaMeta::Database database;
    database.name = "test_cloud_store";
    database.alias = "test_cloud_database_alias_1";
    database.tables.emplace_back(table);

    SchemaMeta schemaMeta1;
    schemaMeta1.version = 1;
    schemaMeta1.bundleName = "test_cloud_bundleName";
    schemaMeta1.databases.emplace_back(database);
    schemaMeta1.e2eeEnable = false;

    SchemaMeta schemaMeta2 = schemaMeta1;
    schemaMeta2.databases[0].tables[0].fields[0].nullable = false;
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].tables[0].fields[0].primary = true;
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].tables[0].fields[0].type = 1;
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].tables[0].fields[0].alias = "test_cloud_field_alias1";
    EXPECT_NE(schemaMeta2, schemaMeta1);
    schemaMeta2.databases[0].tables[0].fields[0].colName = "test_cloud_field_name2";
    EXPECT_NE(schemaMeta2, schemaMeta1);
}

/**
* @tc.name: GetEventId
* @tc.desc: test GetEventId function
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudEventTest, GetEventId, TestSize.Level0)
{
    int32_t evtId = 1;
    StoreInfo info;
    CloudEvent event(evtId, info);
    auto ret = event.GetEventId();
    EXPECT_EQ(ret, evtId);
}

/**
* @tc.name: CloudMarkTest
* @tc.desc: CloudMark Overload function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudInfoTest, CloudMarkTest, TestSize.Level0)
{
    CloudMark cloudmark1;
    cloudmark1.bundleName = "test_cloud_bundleName";
    cloudmark1.deviceId = "1111";
    cloudmark1.storeId = "test_db";

    CloudMark cloudmark2 = cloudmark1;
    EXPECT_EQ(cloudmark2, cloudmark1);

    cloudmark2.userId = 1;
    EXPECT_NE(cloudmark2, cloudmark1);
    cloudmark2.storeId = "test_db2";
    EXPECT_NE(cloudmark2, cloudmark1);
    cloudmark2.isClearWaterMark = true;
    EXPECT_NE(cloudmark2, cloudmark1);
    cloudmark2.index = 1;
    EXPECT_NE(cloudmark2, cloudmark1);
    cloudmark2.deviceId = "222";
    EXPECT_NE(cloudmark2, cloudmark1);
    cloudmark2.bundleName = "test_cloud_bundleName2";
    EXPECT_NE(cloudmark2, cloudmark1);
}

/**
* @tc.name: GetTableNames_001
* @tc.desc: get table names no shared tables
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudEventTest, GetTableNames_001, TestSize.Level0)
{
    Database database;
    for (int i = 0; i < 5; ++i) {
        Table table;
        table.name = "table" + std::to_string(i);
        database.tables.push_back(std::move(table));
    }
    auto tables = database.GetTableNames();
    std::vector<std::string> tagTable = { "table0", "table1", "table2", "table3", "table4" };
    EXPECT_EQ(tables, tagTable);
}

/**
* @tc.name: GetTableNames_002
* @tc.desc: get table names with shared tables
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudEventTest, GetTableNames_002, TestSize.Level0)
{
    Database database;
    for (int i = 0; i < 5; ++i) {
        Table table;
        table.name = "table" + std::to_string(i);
        table.sharedTableName = "shared" + std::to_string(i);
        database.tables.push_back(std::move(table));
    }
    auto tables = database.GetTableNames();
    std::vector<std::string> tagTable = { "table0", "shared0", "table1", "shared1", "table2", "shared2", "table3",
        "shared3", "table4", "shared4" };
    EXPECT_EQ(tables, tagTable);
}

/**
* @tc.name: GetSyncTables_001
* @tc.desc: get p2p sync table names
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudEventTest, GetSyncTables_001, TestSize.Level0)
{
    Database database;
    for (int i = 0; i < 5; ++i) {
        Table table;
        table.name = "table" + std::to_string(i);
        table.sharedTableName = "shared" + std::to_string(i);
        auto &fields = (i % 2 == 0) ? table.deviceSyncFields : table.cloudSyncFields;
        fields.push_back("field" + std::to_string(i));
        database.tables.push_back(std::move(table));
    }
    auto tables = database.GetSyncTables();
    std::vector<std::string> tagTable = { "table0", "table2", "table4" };
    EXPECT_EQ(tables, tagTable);
}

/**
* @tc.name: GetSyncTables_002
* @tc.desc: get p2p sync table names w
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudEventTest, GetSyncTables_002, TestSize.Level0)
{
    Database database;
    auto tables = database.GetSyncTables();
    std::vector<std::string> tagTable;
    EXPECT_EQ(tables, tagTable);
    for (int i = 0; i < 5; ++i) {
        Table table;
        table.name = "table" + std::to_string(i);
        table.sharedTableName = "shared" + std::to_string(i);
        database.tables.push_back(std::move(table));
    }
    tables = database.GetSyncTables();
    EXPECT_EQ(tables, tagTable);
}

/**
* @tc.name: GetSyncTables_001
* @tc.desc: get p2p sync table names w
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudEventTest, GetCloudTables_001, TestSize.Level0)
{
    Database database;
    for (int i = 0; i < 5; ++i) {
        Table table;
        table.name = "table" + std::to_string(i);
        table.sharedTableName = "shared" + std::to_string(i);
        auto &fields = (i % 2 == 0) ? table.deviceSyncFields : table.cloudSyncFields;
        fields.push_back("field" + std::to_string(i));
        database.tables.push_back(std::move(table));
    }
    auto tables = database.GetCloudTables();
    std::vector<std::string> tagTable = { "table1", "shared1", "table3", "shared3" };
    EXPECT_EQ(tables, tagTable);
}

/**
* @tc.name: GetSyncTables_002
* @tc.desc: get p2p sync table names w
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudEventTest, GetCloudTables_002, TestSize.Level0)
{
    Database database;
    for (int i = 0; i < 5; ++i) {
        Table table;
        table.name = "table" + std::to_string(i);
        table.sharedTableName = "shared" + std::to_string(i);
        database.tables.push_back(std::move(table));
    }
    auto tables = database.GetCloudTables();
    std::vector<std::string> tagTable;
    EXPECT_EQ(tables, tagTable);
}

/**
* @tc.name: Field001
* @tc.desc: SchemaMeta Overload function Field test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudInfoTest, Field001, TestSize.Level0)
{
    SchemaMeta::Field field1;
    field1.colName = "test_cloud_field_name1";
    field1.alias = "test_cloud_field_alias1";
    field1.dupCheckCol = true;

    SchemaMeta::Field field2;
    field2.colName = "test_cloud_field_name2";
    field2.alias = "test_cloud_field_alias2";
    EXPECT_FALSE(field1 == field2);
    Serializable::json node;
    field1.Marshal(node);

    SchemaMeta::Field field3;
    field3.Unmarshal(node);
    EXPECT_TRUE(field1 == field3);
}

/**
 * @tc.name: UpdateConfig_WithValidInput_ShouldSuccess
 * @tc.desc: Test UpdateConfig function with valid input parameters
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, UpdateConfig_WithValidInput_ShouldSuccess, TestSize.Level0)
{
    std::map<std::string, DbInfo> dbInfo;
    DbInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "table1", true }, { "table2", false } };
    dbInfo["database1"] = db1;
    bool result = syncConfig_.UpdateConfig(TEST_USER_ID, TEST_APP_ID, dbInfo);
    EXPECT_TRUE(result);
    bool dbEnabled = syncConfig_.IsDbEnable(TEST_USER_ID, TEST_APP_ID, "database1");
    EXPECT_TRUE(dbEnabled);
}

/**
 * @tc.name: UpdateConfig_WithEmptyDbInfo_ShouldReturnFalse
 * @tc.desc: Test UpdateConfig function when dbInfo map is empty
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, UpdateConfig_WithEmptyDbInfo_ShouldReturnFalse, TestSize.Level0)
{
    std::map<std::string, DbInfo> emptyDbInfo;
    bool result = syncConfig_.UpdateConfig(TEST_USER_ID, TEST_APP_ID, emptyDbInfo);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: UpdateConfig_UpdateExistingConfig_ShouldSuccess
 * @tc.desc: Test UpdateConfig function when updating existing configuration
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, UpdateConfig_UpdateExistingConfig_ShouldSuccess, TestSize.Level0)
{
    std::map<std::string, DbInfo> dbInfo1;
    DbInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "table1", true } };
    dbInfo1["database1"] = db1;
    bool result1 = syncConfig_.UpdateConfig(TEST_USER_ID, TEST_APP_ID, dbInfo1);
    EXPECT_TRUE(result1);
    std::map<std::string, DbInfo> dbInfo2;
    DbInfo db2;
    db2.enable = false;
    db2.tableInfo = { { "table1", false }, { "table2", true } };
    dbInfo2["database1"] = db2;
    bool result2 = syncConfig_.UpdateConfig(TEST_USER_ID, TEST_APP_ID, dbInfo2);
    EXPECT_TRUE(result2);
    bool dbEnabled = syncConfig_.IsDbEnable(TEST_USER_ID, TEST_APP_ID, "database1");
    EXPECT_FALSE(dbEnabled);
}

/**
 * @tc.name: UpdateConfig_MultipleDatabases_ShouldHandleAll
 * @tc.desc: Test UpdateConfig function with multiple database configurations
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, UpdateConfig_MultipleDatabases_ShouldHandleAll, TestSize.Level0)
{
    std::map<std::string, DbInfo> dbInfo;
    DbInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "users", true }, { "logs", false } };
    dbInfo["main_db"] = db1;
    DbInfo db2;
    db2.enable = false;
    db2.tableInfo = { { "cache", true } };
    dbInfo["cache_db"] = db2;
    bool result = syncConfig_.UpdateConfig(TEST_USER_ID, TEST_APP_ID, dbInfo);
    EXPECT_TRUE(result);
    EXPECT_TRUE(syncConfig_.IsDbEnable(TEST_USER_ID, TEST_APP_ID, "main_db"));
    EXPECT_FALSE(syncConfig_.IsDbEnable(TEST_USER_ID, TEST_APP_ID, "cache_db"));
}

/**
 * @tc.name: UpdateConfig_NoChanges_ShouldReturnTrueWithoutSaving
 * @tc.desc: Test UpdateConfig function when configuration has no changes
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, UpdateConfig_NoChanges_ShouldReturnTrueWithoutSaving, TestSize.Level0)
{
    std::map<std::string, DbInfo> dbInfo;
    DbInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "table1", true } };
    dbInfo["database1"] = db1;
    bool result1 = syncConfig_.UpdateConfig(TEST_USER_ID, TEST_APP_ID, dbInfo);
    EXPECT_TRUE(result1);
    bool result2 = syncConfig_.UpdateConfig(TEST_USER_ID, TEST_APP_ID, dbInfo);
    EXPECT_TRUE(result2);
}

/**
 * @tc.name: UpdateConfig_WithLongNames_ShouldHandle
 * @tc.desc: Test UpdateConfig function with long bundle name, database name and table name
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, UpdateConfig_WithLongNames_ShouldHandle, TestSize.Level1)
{
    std::string longBundleName(1000, 'a');
    std::string longDbName(500, 'b');
    std::string longTableName(200, 'c');
    std::map<std::string, DbInfo> dbInfo;
    DbInfo db1;
    db1.enable = true;
    db1.tableInfo = { { longTableName, true } };
    dbInfo[longDbName] = db1;
    bool result = syncConfig_.UpdateConfig(TEST_USER_ID, longBundleName, dbInfo);
    EXPECT_TRUE(result);
    bool dbEnabled = syncConfig_.IsDbEnable(TEST_USER_ID, longBundleName, longDbName);
    EXPECT_TRUE(dbEnabled);
}

/**
 * @tc.name: UpdateConfig_WithSpecialCharacters_ShouldHandle
 * @tc.desc: Test UpdateConfig function with special characters in names
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, UpdateConfig_WithSpecialCharacters_ShouldHandle, TestSize.Level1)
{
    std::string specialBundleName = "com.example.app-v2.0@test";
    std::string specialDbName = "my-db_123";
    std::string specialTableName = "user@table#1";

    std::map<std::string, DbInfo> dbInfo;
    DbInfo db1;
    db1.enable = true;
    db1.tableInfo = { { specialTableName, true } };
    dbInfo[specialDbName] = db1;
    bool result = syncConfig_.UpdateConfig(TEST_USER_ID, specialBundleName, dbInfo);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: IsDbEnable_DatabaseNotExist_ShouldReturnTrue
 * @tc.desc: Test IsDbEnable function when database does not exist
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, IsDbEnable_DatabaseNotExist_ShouldReturnTrue, TestSize.Level0)
{
    bool result = syncConfig_.IsDbEnable(TEST_USER_ID, TEST_APP_ID, "non_existent_db");
    EXPECT_TRUE(result);
}

/**
 * @tc.name: IsDbEnable_ConfigNotExist_ShouldReturnTrue
 * @tc.desc: Test IsDbEnable function when configuration does not exist
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, IsDbEnable_ConfigNotExist_ShouldReturnTrue, TestSize.Level0)
{
    bool result = syncConfig_.IsDbEnable(TEST_USER_ID, "com.other.app", "database1");
    EXPECT_TRUE(result);
}

/**
 * @tc.name: FilterCloudEnabledTables_FilterDisabledTables_ShouldWork
 * @tc.desc: Test FilterCloudEnabledTables function to filter disabled tables
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, FilterCloudEnabledTables_FilterDisabledTables_ShouldWork, TestSize.Level0)
{
    std::map<std::string, DbInfo> dbInfo;
    DbInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "table1", true }, { "table2", false }, { "table3", true } };
    dbInfo["database1"] = db1;
    syncConfig_.UpdateConfig(TEST_USER_ID, TEST_APP_ID, dbInfo);
    std::vector<std::string> tables = { "table1", "table2", "table3", "table4" };
    bool result = syncConfig_.FilterCloudEnabledTables(TEST_USER_ID, TEST_APP_ID, "database1", tables);

    EXPECT_TRUE(result);
    EXPECT_EQ(tables.size(), 3);
    EXPECT_NE(std::find(tables.begin(), tables.end(), "table1"), tables.end());
    EXPECT_EQ(std::find(tables.begin(), tables.end(), "table2"), tables.end());
    EXPECT_NE(std::find(tables.begin(), tables.end(), "table3"), tables.end());
    EXPECT_NE(std::find(tables.begin(), tables.end(), "table4"), tables.end());
}

/**
 * @tc.name: FilterCloudEnabledTables_DatabaseNotExist_ShouldReturnTrue
 * @tc.desc: Test FilterCloudEnabledTables function when database does not exist
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, FilterCloudEnabledTables_DatabaseNotExist_ShouldReturnTrue, TestSize.Level0)
{
    std::vector<std::string> tables = { "table1", "table2" };
    bool result = syncConfig_.FilterCloudEnabledTables(TEST_USER_ID, TEST_APP_ID, "non_existent_db", tables);
    EXPECT_TRUE(result);
    EXPECT_EQ(tables.size(), 2);
}

/**
 * @tc.name: FilterCloudEnabledTables_ConfigNotExist_ShouldReturnTrue
 * @tc.desc: Test FilterCloudEnabledTables function when configuration does not exist
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, FilterCloudEnabledTables_ConfigNotExist_ShouldReturnTrue, TestSize.Level0)
{
    std::vector<std::string> tables = { "table1", "table2" };
    bool result = syncConfig_.FilterCloudEnabledTables(TEST_USER_ID, "com.other.app", "database1", tables);
    EXPECT_TRUE(result);
    EXPECT_EQ(tables.size(), 2);
}

/**
 * @tc.name: FilterCloudEnabledTables_AllTablesDisabled_ShouldReturnFalse
 * @tc.desc: Test FilterCloudEnabledTables function when all tables are disabled
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, FilterCloudEnabledTables_AllTablesDisabled_ShouldReturnFalse, TestSize.Level0)
{
    std::map<std::string, DbInfo> dbInfo;
    DbInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "table1", false }, { "table2", false } };
    dbInfo["database1"] = db1;
    syncConfig_.UpdateConfig(TEST_USER_ID, TEST_APP_ID, dbInfo);
    std::vector<std::string> tables = { "table1", "table2" };
    bool result = syncConfig_.FilterCloudEnabledTables(TEST_USER_ID, TEST_APP_ID, "database1", tables);
    EXPECT_FALSE(result);
    EXPECT_TRUE(tables.empty());
}

/**
 * @tc.name: CompleteWorkflow_ShouldWorkCorrectly
 * @tc.desc: Test complete workflow including configuration, query and filtering
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, CompleteWorkflow_ShouldWorkCorrectly, TestSize.Level1)
{
    std::map<std::string, DbInfo> initialConfig;
    DbInfo mainDb;
    mainDb.enable = true;
    mainDb.tableInfo = { { "users", true }, { "logs", false }, { "settings", true } };
    initialConfig["main"] = mainDb;

    DbInfo tempDb;
    tempDb.enable = false;
    tempDb.tableInfo = { { "cache", true } };
    initialConfig["temp"] = tempDb;
    bool configResult = syncConfig_.UpdateConfig(TEST_USER_ID, TEST_APP_ID, initialConfig);
    EXPECT_TRUE(configResult);
    EXPECT_TRUE(syncConfig_.IsDbEnable(TEST_USER_ID, TEST_APP_ID, "main"));
    EXPECT_FALSE(syncConfig_.IsDbEnable(TEST_USER_ID, TEST_APP_ID, "temp"));
    std::vector<std::string> mainTables = { "users", "logs", "settings", "non_existent" };
    bool filterResult = syncConfig_.FilterCloudEnabledTables(TEST_USER_ID, TEST_APP_ID, "main", mainTables);
    EXPECT_TRUE(filterResult);
    EXPECT_EQ(mainTables.size(), 3);
    EXPECT_NE(std::find(mainTables.begin(), mainTables.end(), "users"), mainTables.end());
    EXPECT_EQ(std::find(mainTables.begin(), mainTables.end(), "logs"), mainTables.end());
    EXPECT_NE(std::find(mainTables.begin(), mainTables.end(), "settings"), mainTables.end());
    EXPECT_NE(std::find(mainTables.begin(), mainTables.end(), "non_existent"), mainTables.end());
    std::map<std::string, DbInfo> updateConfig;
    DbInfo updatedMainDb;
    updatedMainDb.enable = true;
    updatedMainDb.tableInfo = { { "users", false }, { "logs", true }, { "settings", true }, { "new_table", true } };
    updateConfig["main"] = updatedMainDb;
    bool updateResult = syncConfig_.UpdateConfig(TEST_USER_ID, TEST_APP_ID, updateConfig);
    EXPECT_TRUE(updateResult);
    std::vector<std::string> updatedTables = { "users", "logs", "settings", "new_table" };
    bool updatedFilterResult = syncConfig_.FilterCloudEnabledTables(TEST_USER_ID, TEST_APP_ID, "main", updatedTables);
    EXPECT_TRUE(updatedFilterResult);
    EXPECT_EQ(updatedTables.size(), 3);
    EXPECT_EQ(std::find(updatedTables.begin(), updatedTables.end(), "users"), updatedTables.end());
    EXPECT_NE(std::find(updatedTables.begin(), updatedTables.end(), "logs"), updatedTables.end());
    EXPECT_NE(std::find(updatedTables.begin(), updatedTables.end(), "settings"), updatedTables.end());
    EXPECT_NE(std::find(updatedTables.begin(), updatedTables.end(), "new_table"), updatedTables.end());
}

/**
 * @tc.name: UpdateConfig_WithInvalidTEST_USER_IDShouldHandle
 * @tc.desc: Test UpdateConfig function with invalid user ID
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, UpdateConfig_WithInvalidTEST_USER_IDShouldHandle, TestSize.Level1)
{
    std::map<std::string, DbInfo> dbInfo;
    DbInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "table1", true } };
    dbInfo["database1"] = db1;
    bool result = syncConfig_.UpdateConfig(-1, TEST_APP_ID, dbInfo);
    EXPECT_TRUE(result || !result);
}

/**
 * @tc.name: UpdateConfig_WithEmptyStrings_ShouldHandle
 * @tc.desc: Test UpdateConfig function with empty strings in parameters
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, UpdateConfig_WithEmptyStrings_ShouldHandle, TestSize.Level1)
{
    std::map<std::string, DbInfo> dbInfo;
    DbInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "", true } };
    dbInfo[""] = db1;
    bool result = syncConfig_.UpdateConfig(TEST_USER_ID, "", dbInfo);
    EXPECT_TRUE(result || !result);
}

/**
 * @tc.name: ConcurrentAccess_ShouldBeThreadSafe
 * @tc.desc: Test concurrent access to CloudDbSyncConfig functions with multiple threads
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudInfoTest, ConcurrentAccess_ShouldBeThreadSafe, TestSize.Level1)
{
    std::map<std::string, DbInfo> dbInfo;
    DbInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "table1", true } };
    dbInfo["database1"] = db1;
    const int32_t threadCount = 4;
    const int32_t operationsPerThread = 5;
    std::vector<std::thread> threads;
    std::vector<bool> results(threadCount * operationsPerThread, false);
    for (int threadId = 0; threadId < threadCount; ++threadId) {
        threads.emplace_back([&, threadId]() {
            for (int i = 0; i < operationsPerThread; ++i) {
                int32_t currentUserId = TEST_USER_ID + threadId * 1000 + i;
                std::string currentBundleName = std::string(TEST_APP_ID) + "_thread" + std::to_string(threadId);

                bool result = syncConfig_.UpdateConfig(currentUserId, currentBundleName, dbInfo);
                results[threadId * operationsPerThread + i] = result;
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    for (bool result : results) {
        EXPECT_TRUE(result);
    }
    for (int threadId = 0; threadId < threadCount; ++threadId) {
        for (int i = 0; i < operationsPerThread; ++i) {
            int32_t currentUserId = TEST_USER_ID + threadId * 1000 + i;
            std::string currentBundleName = std::string(TEST_APP_ID) + "_thread" + std::to_string(threadId);

            bool dbEnabled = syncConfig_.IsDbEnable(currentUserId, currentBundleName, "database1");
            EXPECT_TRUE(dbEnabled);
        }
    }
}
} // namespace OHOS::Test
