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

#include "serializable/serializable.h"
#include "cloud/cloud_db.h"
#include "cloud/cloud_event.h"
#include "cloud/cloud_info.h"
#include "cloud/cloud_server.h"
#include "cloud/schema_meta.h"
#include "utils/crypto.h"
#include "screen/screen_manager.h"
#include "store/general_store.h"
#include "store/general_value.h"
#include "store/general_watcher.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class CloudInfoTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class ServicesCloudServerTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class ServicesCloudDBTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class CloudEventTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class ScreenManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
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
        return {"table1", "table2"};
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
    EXPECT_EQ(Serializable::Marshall(cloudInfo1), Serializable::JSONWrapper::to_string(node1));

    CloudInfo cloudInfo2;
    cloudInfo2.Unmarshal(node1);
    EXPECT_EQ(Serializable::Marshall(cloudInfo1), Serializable::Marshall(cloudInfo2));
}

/**
* @tc.name: AppInfoTest
* @tc.desc: Marshal and Unmarshal of AppInfo.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, AppInfoTest, TestSize.Level0)
{
    CloudInfo::AppInfo cloudInfoAppInfo1;
    cloudInfoAppInfo1.bundleName = "ohos.test.demo";
    cloudInfoAppInfo1.appId = "test1_id";
    cloudInfoAppInfo1.version = 0;
    cloudInfoAppInfo1.instanceId = 0;
    cloudInfoAppInfo1.cloudSwitch = false;

    Serializable::json node1;
    cloudInfoAppInfo1.Marshal(node1);
    EXPECT_EQ(Serializable::Marshall(cloudInfoAppInfo1), Serializable::JSONWrapper::to_string(node1));

    CloudInfo::AppInfo cloudInfoAppInfo2;
    cloudInfoAppInfo2.Unmarshal(node1);
    EXPECT_EQ(Serializable::Marshall(cloudInfoAppInfo1), Serializable::Marshall(cloudInfoAppInfo2));
}

/**
* @tc.name: TableTest
* @tc.desc: Marshal and Unmarshal of Table.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, TableTest, TestSize.Level0)
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
    EXPECT_EQ(Serializable::Marshall(table1), Serializable::JSONWrapper::to_string(node1));

    Table table2;
    table2.Unmarshal(node1);
    EXPECT_EQ(Serializable::Marshall(table1), Serializable::Marshall(table2));
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
    info = { {"test_cloud_bundleName", "CLOUD_SCHEMA###0###test_cloud###100"} };
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
* @tc.name: SchemaMeta
* @tc.desc: test SchemaMeta GetLowVersion and GetHighVersion function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(CloudInfoTest, SchemaMeta, TestSize.Level0)
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
* @tc.name: IsLocked
* @tc.desc: test IsLocked function
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(ScreenManagerTest, IsLocked, TestSize.Level0)
{
    ASSERT_FALSE(ScreenManager::GetInstance()->IsLocked());
}
} // namespace OHOS::Test
