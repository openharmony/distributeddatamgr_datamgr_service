
/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You not obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include "cloud/schema_meta.h"
#include <nlohmann/json.hpp>

using namespace OHOS::DistributedData;
using json = nlohmann::json;

class SchemaMetaTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override {}
    void TearDown() override {}
};

// Test SchemaMeta::Marshal
TEST_F(SchemaMetaTest, MarshalTest)
{
    SchemaMeta schemaMeta;
    schemaMeta.metaVersion = 1;
    schemaMeta.version = 2;
    schemaMeta.bundleName = "testBundle";
    schemaMeta.e2eeEnable = true;
    
    Database db;
    db.name = "testDB";
    schemaMeta.databases.push_back(db);
    
    json node;
    bool result = schemaMeta.Marshal(node);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(node["metaVersion"], 1);
    EXPECT_EQ(node["version"], 2);
    EXPECT_EQ(node["bundleName"], "testBundle");
    EXPECT_EQ(node["e2eeEnable"], true);
    EXPECT_EQ(node["databases"].size(), 1);
    EXPECT_EQ(node["databases"][0]["name"], "testDB");
}

// Test SchemaMeta::Unmarshal
TEST_F(SchemaMetaTest, UnmarshalTest)
{
    json node;
    node["metaVersion"] = 1;
    node["version"] = 2;
    node["bundleName"] = "testBundle";
    node["e2eeEnable"] = true;
    
    json dbNode;
    dbNode["name"] = "testDB";
    node["databases"] = json::array({dbNode});
    
    SchemaMeta schemaMeta;
    bool result = schemaMeta.Unmarshal(node);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(schemaMeta.metaVersion, 1);
    EXPECT_EQ(schemaMeta.version, 2);
    EXPECT_EQ(schemaMeta.bundleName, "testBundle");
    EXPECT_EQ(schemaMeta.e2eeEnable, true);
    EXPECT_EQ(schemaMeta.databases.size(), 1);
    EXPECT_EQ(schemaMeta.databases[0].name, "testDB");
}

// Test SchemaMeta::Unmarshal with missing metaVersion
TEST_F(SchemaMetaTest, UnmarshalWithoutMetaVersionTest)
{
    json node;
    node["version"] = 2;
    node["bundleName"] = "testBundle";
    
    SchemaMeta schemaMeta;
    schemaMeta.metaVersion = 5; // Set initial value
    bool result = schemaMeta.Unmarshal(node);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(schemaMeta.metaVersion, 0); // Should be reset to 0
    EXPECT_EQ(schemaMeta.version, 2);
    EXPECT_EQ(schemaMeta.bundleName, "testBundle");
}

// Test SchemaMeta::operator==
TEST_F(SchemaMetaTest, EqualityTest)
{
    SchemaMeta schemaMeta1;
    schemaMeta1.metaVersion = 1;
    schemaMeta1.version = 2;
    schemaMeta1.bundleName = "testBundle";
    schemaMeta1.e2eeEnable = true;
    
    SchemaMeta schemaMeta2;
    schemaMeta2.metaVersion = 1;
    schemaMeta2.version = 2;
    schemaMeta2.bundleName = "testBundle";
    schemaMeta2.e2eeEnable = true;
    
    EXPECT_TRUE(schemaMeta1 == schemaMeta2);
    
    schemaMeta2.version = 3;
    EXPECT_FALSE(schemaMeta1 == schemaMeta2);
}

// Test SchemaMeta::operator!=
TEST_F(SchemaMetaTest, InequalityTest)
{
    SchemaMeta schemaMeta1;
    schemaMeta1.metaVersion = 1;
    schemaMeta1.version = 2;
    schemaMeta1.bundleName = "testBundle";
    
    SchemaMeta schemaMeta2;
    schemaMeta2.metaVersion = 1;
    schemaMeta2.version = 3;
    schemaMeta2.bundleName = "testBundle";
    
    EXPECT_TRUE(schemaMeta1 != schemaMeta2);
    
    schemaMeta2.version = 2;
    EXPECT_FALSE(schemaMeta1 != schemaMeta2);
}

// Test SchemaMeta::GetDataBase
TEST_F(SchemaMetaTest, GetDataBaseTest)
{
    SchemaMeta schemaMeta;
    
    Database db1;
    db1.name = "db1";
    
    Database db2;
    db2.name = "db2";
    
    schemaMeta.databases.push_back(db1);
    schemaMeta.databases.push_back(db2);
    
    Database result = schemaMeta.GetDataBase("db1");
    EXPECT_EQ(result.name, "db1");
    
    Database emptyResult = schemaMeta.GetDataBase("db3");
    EXPECT_EQ(emptyResult.name, "");
}

// Test SchemaMeta::IsValid
TEST_F(SchemaMetaTest, IsValidTest)
{
    SchemaMeta schemaMeta1;
    schemaMeta1.bundleName = "testBundle";
    
    Database db;
    db.name = "testDB";
    schemaMeta1.databases.push_back(db);
    
    EXPECT_TRUE(schemaMeta1.IsValid());
    
    SchemaMeta schemaMeta2;
    EXPECT_FALSE(schemaMeta2.IsValid());
    
    SchemaMeta schemaMeta3;
    schemaMeta3.bundleName = "testBundle";
    EXPECT_FALSE(schemaMeta3.IsValid());
}

// Test SchemaMeta::GetStores
TEST_F(SchemaMetaTest, GetStoresTest)
{
    SchemaMeta schemaMeta;
    
    Database db1;
    db1.name = "db1";
    
    Database db2;
    db2.name = "db2";
    
    schemaMeta.databases.push_back(db1);
    schemaMeta.databases.push_back(db2);
    
    std::vector<std::string> stores = schemaMeta.GetStores();
    EXPECT_EQ(stores.size(), 2);
    EXPECT_EQ(stores[0], "db1");
    EXPECT_EQ(stores[1], "db2");
}

// Test Database::GetTableNames
TEST_F(SchemaMetaTest, GetTableNamesTest)
{
    Database database;
    
    Table table1;
    table1.name = "table1";
    
    Table table2;
    table2.name = "table2";
    table2.sharedTableName = "sharedTable2";
    
    database.tables.push_back(table1);
    database.tables.push_back(table2);
    
    std::vector<std::string> tableNames = database.GetTableNames();
    EXPECT_EQ(tableNames.size(), 3);
    EXPECT_EQ(tableNames[0], "table1");
    EXPECT_EQ(tableNames[1], "table2");
    EXPECT_EQ(tableNames[2], "sharedTable2");
}

// Test Database::GetSyncTables
TEST_F(SchemaMetaTest, GetSyncTablesTest)
{
    Database database;
    
    Table table1;
    table1.name = "table1";
    table1.deviceSyncFields = {"field1", "field2"};
    
    Table table2;
    table2.name = "table2";
    // No deviceSyncFields
    
    Table table3;
    table3.name = "table3";
    table3.deviceSyncFields = {"field3"};
    
    database.tables.push_back(table1);
    database.tables.push_back(table2);
    database.tables.push_back(table3);
    
    std::vector<std::string> syncTables = database.GetSyncTables();
    EXPECT_EQ(syncTables.size(), 2);
    EXPECT_EQ(syncTables[0], "table1");
    EXPECT_EQ(syncTables[1], "table3");
}

// Test Database::GetCloudTables
TEST_F(SchemaMetaTest, GetCloudTablesTest)
{
    Database database;
    
    Table table1;
    table1.name = "table1";
    table1.cloudSyncFields = {"field1", "field2"};
    
    Table table2;
    table2.name = "table2";
    // No cloudSyncFields
    
    Table table3;
    table3.name = "table3";
    table3.cloudSyncFields = {"field3"};
    table3.sharedTableName = "sharedTable3";
    
    database.tables.push_back(table1);
    database.tables.push_back(table2);
    database.tables.push_back(table3);
    
    std::vector<std::string> cloudTables = database.GetCloudTables();
    EXPECT_EQ(cloudTables.size(), 3);
    EXPECT_EQ(cloudTables[0], "table1");
    EXPECT_EQ(cloudTables[1], "table3");
    EXPECT_EQ(cloudTables[2], "sharedTable3");
}

// Test Database::GetKey
TEST_F(SchemaMetaTest, GetKeyTest)
{
    Database database;
    database.user = "user1";
    database.bundleName = "bundle1";
    database.name = "db1";
    
    std::string key = database.GetKey();
    EXPECT_EQ(key, "DistributedSchema###user1###default###bundle1###db1");
}

// Test Database::GetPrefix
TEST_F(SchemaMetaTest, GetPrefixTest)
{
    std::string prefix = Database::GetPrefix({"field1", "field2"});
    EXPECT_EQ(prefix, "DistributedSchema###field1###field2###");
}

// Test Database::Marshal
TEST_F(SchemaMetaTest, DatabaseMarshalTest)
{
    Database database;
    database.name = "testDB";
    database.alias = "testAlias";
    database.version = 1;
    database.bundleName = "testBundle";
    database.user = "testUser";
    database.deviceId = "testDevice";
    database.autoSyncType = 2;
    
    Table table;
    table.name = "testTable";
    database.tables.push_back(table);
    
    json node;
    bool result = database.Marshal(node);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(node["name"], "testDB");
    EXPECT_EQ(node["alias"], "testAlias");
    EXPECT_EQ(node["version"], 1);
    EXPECT_EQ(node["bundleName"], "testBundle");
    EXPECT_EQ(node["user"], "testUser");
    EXPECT_EQ(node["deviceId"], "testDevice");
    EXPECT_EQ(node["autoSyncType"], 2);
    EXPECT_EQ(node["tables"].size(), 1);
}

// Test Database::Unmarshal
TEST_F(SchemaMetaTest, DatabaseUnmarshalTest)
{
    json node;
    node["name"] = "testDB";
    node["alias"] = "testAlias";
    node["version"] = 1;
    node["bundleName"] = "testBundle";
    node["user"] = "testUser";
    node["deviceId"] = "testDevice";
    node["autoSyncType"] = 2;
    
    json tableNode;
    tableNode["name"] = "testTable";
    node["tables"] = json::array({tableNode});
    
    Database database;
    bool result = database.Unmarshal(node);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(database.name, "testDB");
    EXPECT_EQ(database.alias, "testAlias");
    EXPECT_EQ(database.version, 1);
    EXPECT_EQ(database.bundleName, "testBundle");
    EXPECT_EQ(database.user, "testUser");
    EXPECT_EQ(database.deviceId, "testDevice");
    EXPECT_EQ(database.autoSyncType, 2);
    EXPECT_EQ(database.tables.size(), 1);
    EXPECT_EQ(database.tables[0].name, "testTable");
}

// Test Database::operator==
TEST_F(SchemaMetaTest, DatabaseEqualityTest)
{
    Database db1;
    db1.name = "testDB";
    db1.alias = "testAlias";
    db1.version = 1;
    
    Database db2;
    db2.name = "testDB";
    db2.alias = "testAlias";
    db2.version = 1;
    
    EXPECT_TRUE(db1 == db2);
    
    db2.version = 2;
    EXPECT_FALSE(db1 == db2);
}

// Test Table::Marshal
TEST_F(SchemaMetaTest, TableMarshalTest)
{
    Table table;
    table.name = "testTable";
    table.sharedTableName = "sharedTestTable";
    table.alias = "testAlias";
    
    Field field;
    field.colName = "testField";
    table.fields.push_back(field);
    
    table.deviceSyncFields = {"field1", "field2"};
    table.cloudSyncFields = {"field3", "field4"};
    
    json node;
    bool result = table.Marshal(node);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(node["name"], "testTable");
    EXPECT_EQ(node["sharedTableName"], "sharedTestTable");
    EXPECT_EQ(node["alias"], "testAlias");
    EXPECT_EQ(node["fields"].size(), 1);
    EXPECT_EQ(node["deviceSyncFields"].size(), 2);
    EXPECT_EQ(node["cloudSyncFields"].size(), 2);
}

// Test Table::Unmarshal
TEST_F(SchemaMetaTest, TableUnmarshalTest)
{
    json node;
    node["name"] = "testTable";
    node["sharedTableName"] = "sharedTestTable";
    node["alias"] = "testAlias";
    
    json fieldNode;
    fieldNode["colName"] = "testField";
    node["fields"] = json::array({fieldNode});
    
    node["deviceSyncFields"] = {"field1", "field2"};
    node["cloudSyncFields"] = {"field3", "field4"};
    
    Table table;
    bool result = table.Unmarshal(node);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(table.name, "testTable");
    EXPECT_EQ(table.sharedTableName, "sharedTestTable");
    EXPECT_EQ(table.alias, "testAlias");
    EXPECT_EQ(table.fields.size(), 1);
    EXPECT_EQ(table.deviceSyncFields.size(), 2);
    EXPECT_EQ(table.cloudSyncFields.size(), 2);
}

// Test Table::operator==
TEST_F(SchemaMetaTest, TableEqualityTest)
{
    Table table1;
    table1.name = "testTable";
    table1.alias = "testAlias";
    
    Table table2;
    table2.name = "testTable";
    table2.alias = "testAlias";
    
    EXPECT_TRUE(table1 == table2);
    
    table2.alias = "otherAlias";
    EXPECT_FALSE(table1 == table2);
}

// Test Field::Marshal
TEST_F(SchemaMetaTest, FieldMarshalTest)
{
    Field field;
    field.colName = "testField";
    field.alias = "testAlias";
    field.type = "TEXT";
    field.primary = true;
    field.nullable = false;
    
    json node;
    bool result = field.Marshal(node);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(node["colName"], "testField");
    EXPECT_EQ(node["alias"], "testAlias");
    EXPECT_EQ(node["type"], "TEXT");
    EXPECT_EQ(node["primary"], true);
    EXPECT_EQ(node["nullable"], false);
}

// Test Field::Unmarshal
TEST_F(SchemaMetaTest, FieldUnmarshalTest)
{
    json node;
    node["colName"] = "testField";
    node["alias"] = "testAlias";
    node["type"] = "TEXT";
    node["primary"] = true;
    node["nullable"] = false;
    
    Field field;
    bool result = field.Unmarshal(node);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(field.colName, "testField");
    EXPECT_EQ(field.alias, "testAlias");
    EXPECT_EQ(field.type, "TEXT");
    EXPECT_EQ(field.primary, true);
    EXPECT_EQ(field.nullable, false);
}

// Test Field::operator==
TEST_F(SchemaMetaTest, FieldEqualityTest)
{
    Field field1;
    field1.colName = "testField";
    field1.type = "TEXT";
    field1.primary = true;
    
    Field field2;
    field2.colName = "testField";
    field2.type = "TEXT";
    field2.primary = true;
    
    EXPECT_TRUE(field1 == field2);
    
    field2.primary = false;
    EXPECT_FALSE(field1 == field2);
}



#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "sync_manager.h"
#include "mock_event_center.h"
#include "mock_bootstrap.h"
#include "mock_checker_manager.h"
#include "mock_meta_data_manager.h"
#include "mock_network_delegate.h"
#include "mock_account.h"
#include "mock_executor_pool.h"
#include "mock_cloud_server.h"
#include "mock_auto_cache.h"
#include "mock_radar_reporter.h"
#include "mock_reporter.h"

using namespace testing;

class SyncManagerTest : public Test {
protected:
    void SetUp() override {
        // 初始化Mock对象
        mockEventCenter = std::make_shared<MockEventCenter>();
        mockBootstrap = std::make_shared<MockBootstrap>();
        mockCheckerManager = std::make_shared<MockCheckerManager>();
        mockMetaDataManager = std::make_shared<MockMetaDataManager>();
        mockNetworkDelegate = std::make_shared<MockNetworkDelegate>();
        mockAccount = std::make_shared<MockAccount>();
        mockExecutorPool = std::make_shared<MockExecutorPool>();
        mockCloudServer = std::make_shared<MockCloudServer>();
        mockAutoCache = std::make_shared<MockAutoCache>();
        mockRadarReporter = std::make_shared<MockRadarReporter>();
        mockReporter = std::make_shared<MockReporter>();

        // 设置单例返回
        EventCenter::SetInstance(mockEventCenter);
        Bootstrap::SetInstance(mockBootstrap);
        CheckerManager::SetInstance(mockCheckerManager);
        MetaDataManager::SetInstance(mockMetaDataManager);
        NetworkDelegate::SetInstance(mockNetworkDelegate);
        Account::SetInstance(mockAccount);
        CloudServer::SetInstance(mockCloudServer);
        AutoCache::SetInstance(mockAutoCache);
        RadarReporter::SetInstance(mockRadarReporter);
        Reporter::SetInstance(mockReporter);
    }

    void TearDown() override {
        // 清理单例
        EventCenter::ResetInstance();
        Bootstrap::ResetInstance();
        CheckerManager::ResetInstance();
        MetaDataManager::ResetInstance();
        NetworkDelegate::ResetInstance();
        Account::ResetInstance();
        CloudServer::ResetInstance();
        AutoCache::ResetInstance();
        RadarReporter::ResetInstance();
        Reporter::ResetInstance();
    }

    std::shared_ptr<MockEventCenter> mockEventCenter;
    std::shared_ptr<MockBootstrap> mockBootstrap;
    std::shared_ptr<MockCheckerManager> mockCheckerManager;
    std::shared_ptr<MockMetaDataManager> mockMetaDataManager;
    std::shared_ptr<MockNetworkDelegate> mockNetworkDelegate;
    std::shared_ptr<MockAccount> mockAccount;
    std::shared_ptr<MockExecutorPool> mockExecutorPool;
    std::shared_ptr<MockCloudServer> mockCloudServer;
    std::shared_ptr<MockAutoCache> mockAutoCache;
    std::shared_ptr<MockRadarReporter> mockRadarReporter;
    std::shared_ptr<MockReporter> mockReporter;
};

// 测试构造函数
TEST_F(SyncManagerTest, Constructor_SubscribesToEvents) {
    // 设置期望
    EXPECT_CALL(*mockEventCenter, Subscribe(CloudEvent::LOCK_CLOUD_CONTAINER, _)).Times(1);
    EXPECT_CALL(*mockEventCenter, Subscribe(CloudEvent::UNLOCK_CLOUD_CONTAINER, _)).Times(1);
    EXPECT_CALL(*mockEventCenter, Subscribe(CloudEvent::LOCAL_CHANGE, _)).Times(1);
    EXPECT_CALL(*mockBootstrap, GetProcessLabel()).WillOnce(Return("test_process"));

    std::vector<StoreMetaData> staticStores, dynamicStores;
    EXPECT_CALL(*mockCheckerManager, GetStaticStores()).WillOnce(Return(staticStores));
    EXPECT_CALL(*mockCheckerManager, GetDynamicStores()).WillOnce(Return(dynamicStores));

    // 执行测试
    SyncManager syncManager;

    // 验证结果（构造函数执行完成即表示订阅成功）
}

// 测试Bind方法
TEST_F(SyncManagerTest, Bind_SetsExecutor) {
    SyncManager syncManager;

    // 执行测试
    int32_t result = syncManager.Bind(mockExecutorPool);

    // 验证结果
    EXPECT_EQ(result, E_OK);
}

// 测试DoCloudSync方法 - executor为空
TEST_F(SyncManagerTest, DoCloudSync_ExecutorNull_ReturnsNotInit) {
    SyncManager syncManager;

    SyncInfo syncInfo;
    syncInfo.user_ = 1;

    // 执行测试
    int32_t result = syncManager.DoCloudSync(syncInfo);

    // 验证结果
    EXPECT_EQ(result, E_NOT_INIT);
}

// 测试DoCloudSync方法 - 正常执行
TEST_F(SyncManagerTest, DoCloudSync_ExecutorSet_ReturnsOk) {
    SyncManager syncManager;
    syncManager.Bind(mockExecutorPool);

    // 设置期望
    EXPECT_CALL(*mockExecutorPool, Execute(_)).WillOnce(Return(TaskId(1)));

    SyncInfo syncInfo;
    syncInfo.user_ = 1;

    // 执行测试
    int32_t result = syncManager.DoCloudSync(syncInfo);

    // 验证结果
    EXPECT_EQ(result, E_OK);
}

// 测试StopCloudSync方法 - executor为空
TEST_F(SyncManagerTest, StopCloudSync_ExecutorNull_ReturnsNotInit) {
    SyncManager syncManager;

    // 执行测试
    int32_t result = syncManager.StopCloudSync(1);

    // 验证结果
    EXPECT_EQ(result, E_NOT_INIT);
}

// 测试StopCloudSync方法 - 正常执行
TEST_F(SyncManagerTest, StopCloudSync_ExecutorSet_ReturnsOk) {
    SyncManager syncManager;
    syncManager.Bind(mockExecutorPool);

    // 先添加一个任务
    EXPECT_CALL(*mockExecutorPool, Execute(_)).WillOnce(Return(TaskId(1)));
    SyncInfo syncInfo;
    syncInfo.user_ = 1;
    syncManager.DoCloudSync(syncInfo);

    // 设置期望
    EXPECT_CALL(*mockExecutorPool, Remove(TaskId(1))).Times(1);

    // 执行测试
    int32_t result = syncManager.StopCloudSync(1);

    // 验证结果
    EXPECT_EQ(result, E_OK);
}

// 测试IsValid方法 - Meta数据加载失败
TEST_F(SyncManagerTest, IsValid_MetaDataLoadFailed_ReturnsCloudDisabled) {
    // 设置期望
    EXPECT_CALL(*mockMetaDataManager, LoadMeta(_, _, true)).WillOnce(Return(false));

    SyncManager syncManager;
    SyncInfo syncInfo;
    CloudInfo cloudInfo;

    // 执行测试
    GeneralError result = syncManager.IsValid(syncInfo, cloudInfo);

    // 验证结果
    EXPECT_EQ(result, E_CLOUD_DISABLED);
}

// 测试IsValid方法 - Cloud未启用
TEST_F(SyncManagerTest, IsValid_CloudNotEnabled_ReturnsCloudDisabled) {
    // 设置期望
    EXPECT_CALL(*mockMetaDataManager, LoadMeta(_, _, true)).WillOnce(Return(true));
    EXPECT_CALL(*mockNetworkDelegate, IsNetworkAvailable()).WillOnce(Return(true));
    EXPECT_CALL(*mockAccount, IsVerified(_)).WillOnce(Return(true));

    SyncManager syncManager;
    SyncInfo syncInfo;
    CloudInfo cloudInfo;
    cloudInfo.enableCloud = false;

    // 执行测试
    GeneralError result = syncManager.IsValid(syncInfo, cloudInfo);

    // 验证结果
    EXPECT_EQ(result, E_CLOUD_DISABLED);
}

// 测试IsValid方法 - 网络不可用
TEST_F(SyncManagerTest, IsValid_NetworkUnavailable_ReturnsNetworkError) {
    // 设置期望
    EXPECT_CALL(*mockMetaDataManager, LoadMeta(_, _, true)).WillOnce(Return(true));
    EXPECT_CALL(*mockNetworkDelegate, IsNetworkAvailable()).WillOnce(Return(false));

    SyncManager syncManager;
    SyncInfo syncInfo;
    CloudInfo cloudInfo;
    cloudInfo.enableCloud = true;

    // 执行测试
    GeneralError result = syncManager.IsValid(syncInfo, cloudInfo);

    // 验证结果
    EXPECT_EQ(result, E_NETWORK_ERROR);
}

// 测试IsValid方法 - 用户未验证
TEST_F(SyncManagerTest, IsValid_UserNotVerified_ReturnsError) {
    // 设置期望
    EXPECT_CALL(*mockMetaDataManager, LoadMeta(_, _, true)).WillOnce(Return(true));
    EXPECT_CALL(*mockNetworkDelegate, IsNetworkAvailable()).WillOnce(Return(true));
    EXPECT_CALL(*mockAccount, IsVerified(_)).WillOnce(Return(false));

    SyncManager syncManager;
    SyncInfo syncInfo;
    syncInfo.user_ = 1;
    CloudInfo cloudInfo;
    cloudInfo.enableCloud = true;

    // 执行测试
    GeneralError result = syncManager.IsValid(syncInfo, cloudInfo);

    // 验证结果
    EXPECT_EQ(result, E_ERROR);
}

// 测试IsValid方法 - 验证通过
TEST_F(SyncManagerTest, IsValid_AllValid_ReturnsOk) {
    // 设置期望
    EXPECT_CALL(*mockMetaDataManager, LoadMeta(_, _, true)).WillOnce(Return(true));
    EXPECT_CALL(*mockNetworkDelegate, IsNetworkAvailable()).WillOnce(Return(true));
    EXPECT_CALL(*mockAccount, IsVerified(_)).WillOnce(Return(true));

    SyncManager syncManager;
    SyncInfo syncInfo;
    syncInfo.user_ = 1;
    CloudInfo cloudInfo;
    cloudInfo.enableCloud = true;

    // 执行测试
    GeneralError result = syncManager.IsValid(syncInfo, cloudInfo);

    // 验证结果
    EXPECT_EQ(result, E_OK);
}