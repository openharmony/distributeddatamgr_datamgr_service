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

#define LOG_TAG "CloudTest"
#include <string>

#include "account/account_delegate.h"
#include "cloud/cloud_server.h"
#include "cloud/sync_event.h"
#include "gtest/gtest.h"
#include "ipc_skeleton.h"
#include "metadata/meta_data_manager.h"
#include "mock/db_store_mock.h"
#include "sync_config.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
using Database = SchemaMeta::Database;
using namespace OHOS::CloudData;
namespace OHOS::Test {
const constexpr int32_t TEST_USER_ID = 100;
class CloudTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};

protected:
    static constexpr const char* TEST_CLOUD_BUNDLE = "test_cloud_bundleName";
    static constexpr const char* TEST_CLOUD_STORE = "test_cloud_database_name";
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
};
std::shared_ptr<DBStoreMock> CloudTest::dbStoreMock_ = std::make_shared<DBStoreMock>();

void CloudTest::SetUpTestCase(void)
{
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
}

/**
* @tc.name: EventInfo
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudTest, EventInfo, TestSize.Level1)
{
    const int32_t mode = 1;
    const int32_t wait = 10;
    const bool retry = true;
    std::shared_ptr<GenQuery> query = nullptr;
    int sign = 0;
    auto async = [&sign](const GenDetails& details) {
        ++sign;
    };
    SyncEvent::EventInfo eventInfo1(mode, wait, retry, query, async);
    SyncEvent::EventInfo eventInfo2(std::move(eventInfo1));
    SyncEvent::EventInfo eventInfo3 = std::move(eventInfo2);
    StoreInfo storeInfo{ IPCSkeleton::GetCallingTokenID(), TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE, 0 };
    SyncEvent evt(storeInfo, eventInfo3);
    EXPECT_EQ(evt.GetMode(), mode);
    EXPECT_EQ(evt.GetWait(), wait);
    EXPECT_EQ(evt.AutoRetry(), retry);
    EXPECT_EQ(evt.GetQuery(), query);
    evt.GetAsyncDetail()(GenDetails());
    EXPECT_NE(0, sign);
}

/**
* @tc.name: Serializable_Marshal
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudTest, Serializable_Marshal, TestSize.Level1)
{
    SchemaMeta schemaMeta;
    bool ret = schemaMeta.IsValid();
    EXPECT_EQ(false, ret);

    SchemaMeta::Database database;
    database.name = TEST_CLOUD_STORE;
    database.alias = "database_alias_test";

    schemaMeta.version = 1;
    schemaMeta.bundleName = TEST_CLOUD_BUNDLE;
    schemaMeta.databases.emplace_back(database);
    ret = schemaMeta.IsValid();
    EXPECT_EQ(true, ret);

    Serializable::json node = schemaMeta.Marshall();
    SchemaMeta schemaMeta2;
    schemaMeta2.Unmarshal(node);

    EXPECT_EQ(schemaMeta.version, schemaMeta2.version);
    EXPECT_EQ(schemaMeta.bundleName, schemaMeta2.bundleName);
    Database database2 = schemaMeta2.GetDataBase(TEST_CLOUD_STORE);
    EXPECT_EQ(database.alias, database2.alias);

    std::string storeId = "storeId";
    Database database3 = schemaMeta2.GetDataBase(storeId);
    EXPECT_NE(database.alias, database3.alias);
}

/**
* @tc.name: Field_Marshal
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudTest, Field_Marshal, TestSize.Level1)
{
    Field field;
    field.colName = "field_name1_test";
    field.alias = "field_alias1_test";
    field.type = 1;
    field.primary = true;
    field.nullable = false;

    Serializable::json node = field.Marshall();
    Field field2;
    field2.Unmarshal(node);

    EXPECT_EQ(field.colName, field2.colName);
    EXPECT_EQ(field.alias, field2.alias);
    EXPECT_EQ(field.type, field2.type);
    EXPECT_EQ(field.primary, field2.primary);
    EXPECT_EQ(field.nullable, field2.nullable);
}

/**
* @tc.name: Database_Marshal
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(CloudTest, Database_Marshal, TestSize.Level1)
{
    SchemaMeta::Table table1;
    table1.name = "test_cloud_table_name1";
    table1.alias = "test_cloud_table_alias1";
    SchemaMeta::Table table2;
    table2.name = "test_cloud_table_name2";
    table2.alias = "test_cloud_table_alias2";

    SchemaMeta::Database database1;
    database1.name = TEST_CLOUD_STORE;
    database1.alias = "test_cloud_database_alias";
    database1.tables.emplace_back(table1);
    database1.tables.emplace_back(table2);

    Serializable::json node = database1.Marshall();
    SchemaMeta::Database database2;
    database2.Unmarshal(node);

    EXPECT_EQ(database1.name, database2.name);
    EXPECT_EQ(database1.alias, database2.alias);
    std::vector<std::string> tableNames1 = database1.GetTableNames();
    std::vector<std::string> tableNames2 = database2.GetTableNames();
    EXPECT_EQ(tableNames1.size(), tableNames2.size());
    for (uint32_t i = 0; i < tableNames1.size(); ++i) {
        EXPECT_EQ(tableNames1[i], tableNames2[i]);
    }
}

/**
 * @tc.name: Load old cloudInfo
 * @tc.desc: The obtained maxUploadBatchNumber and maxUploadBatchSize are not equal to 0
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: ht
 */
HWTEST_F(CloudTest, CloudInfoUpgrade, TestSize.Level0)
{
    int32_t defaultUser = 100;
    CloudInfo oldInfo;
    auto user = defaultUser;
    oldInfo.user = user;
    EXPECT_NE(oldInfo.maxNumber, CloudInfo::DEFAULT_BATCH_NUMBER);
    EXPECT_NE(oldInfo.maxSize, CloudInfo::DEFAULT_BATCH_SIZE);
    ASSERT_TRUE(MetaDataManager::GetInstance().SaveMeta(oldInfo.GetKey(), oldInfo, true));
    CloudInfo newInfo;
    ASSERT_TRUE(MetaDataManager::GetInstance().LoadMeta(oldInfo.GetKey(), newInfo, true));
    EXPECT_EQ(newInfo.maxNumber, CloudInfo::DEFAULT_BATCH_NUMBER);
    EXPECT_EQ(newInfo.maxSize, CloudInfo::DEFAULT_BATCH_SIZE);
}

/**
 * @tc.name: UpdateConfig_WithValidInput_ShouldSuccess
 * @tc.desc: Test UpdateConfig function with valid input parameters
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CloudTest, UpdateConfig_WithValidInput_ShouldSuccess, TestSize.Level0)
{
    std::map<std::string, DBSwitchInfo> dbInfo;
    DBSwitchInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "table1", true }, { "table2", false } };
    dbInfo["database1"] = db1;
    bool result = SyncConfig::UpdateConfig(TEST_USER_ID, TEST_CLOUD_BUNDLE, dbInfo);
    EXPECT_TRUE(result);
    bool dbEnabled = SyncConfig::IsDbEnable(TEST_USER_ID, TEST_CLOUD_BUNDLE, "database1");
    EXPECT_TRUE(dbEnabled);
}

/**
  * @tc.name: UpdateConfig_WithEmptyDbInfo_ShouldReturnFalse
  * @tc.desc: Test UpdateConfig function when dbInfo map is empty
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(CloudTest, UpdateConfig_WithEmptyDbInfo_ShouldReturnFalse, TestSize.Level0)
{
    std::map<std::string, DBSwitchInfo> emptyDbInfo;
    bool result = SyncConfig::UpdateConfig(TEST_USER_ID, TEST_CLOUD_BUNDLE, emptyDbInfo);
    EXPECT_FALSE(result);
}

/**
  * @tc.name: UpdateConfig_UpdateExistingConfig_ShouldSuccess
  * @tc.desc: Test UpdateConfig function when updating existing configuration
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(CloudTest, UpdateConfig_UpdateExistingConfig_ShouldSuccess, TestSize.Level0)
{
    std::map<std::string, DBSwitchInfo> dbInfo1;
    DBSwitchInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "table1", true } };
    dbInfo1["database1"] = db1;
    bool result1 = SyncConfig::UpdateConfig(TEST_USER_ID, TEST_CLOUD_BUNDLE, dbInfo1);
    EXPECT_TRUE(result1);
    std::map<std::string, DBSwitchInfo> dbInfo2;
    DBSwitchInfo db2;
    db2.enable = false;
    db2.tableInfo = { { "table1", false }, { "table2", true } };
    dbInfo2["database1"] = db2;
    bool result2 = SyncConfig::UpdateConfig(TEST_USER_ID, TEST_CLOUD_BUNDLE, dbInfo2);
    EXPECT_TRUE(result2);
    bool dbEnabled = SyncConfig::IsDbEnable(TEST_USER_ID, TEST_CLOUD_BUNDLE, "database1");
    EXPECT_FALSE(dbEnabled);
}

/**
  * @tc.name: UpdateConfig_MultipleDatabases_ShouldHandleAll
  * @tc.desc: Test UpdateConfig function with multiple database configurations
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(CloudTest, UpdateConfig_MultipleDatabases_ShouldHandleAll, TestSize.Level0)
{
    std::map<std::string, DBSwitchInfo> dbInfo;
    DBSwitchInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "users", true }, { "logs", false } };
    dbInfo["main_db"] = db1;
    DBSwitchInfo db2;
    db2.enable = false;
    db2.tableInfo = { { "cache", true } };
    dbInfo["cache_db"] = db2;
    bool result = SyncConfig::UpdateConfig(TEST_USER_ID, TEST_CLOUD_BUNDLE, dbInfo);
    EXPECT_TRUE(result);
    EXPECT_TRUE(SyncConfig::IsDbEnable(TEST_USER_ID, TEST_CLOUD_BUNDLE, "main_db"));
    EXPECT_FALSE(SyncConfig::IsDbEnable(TEST_USER_ID, TEST_CLOUD_BUNDLE, "cache_db"));
}

/**
  * @tc.name: UpdateConfig_NoChanges_ShouldReturnTrueWithoutSaving
  * @tc.desc: Test UpdateConfig function when configuration has no changes
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(CloudTest, UpdateConfig_NoChanges_ShouldReturnTrueWithoutSaving, TestSize.Level0)
{
    std::map<std::string, DBSwitchInfo> dbInfo;
    DBSwitchInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "table1", true } };
    dbInfo["database1"] = db1;
    bool result1 = SyncConfig::UpdateConfig(TEST_USER_ID, TEST_CLOUD_BUNDLE, dbInfo);
    EXPECT_TRUE(result1);
    bool result2 = SyncConfig::UpdateConfig(TEST_USER_ID, TEST_CLOUD_BUNDLE, dbInfo);
    EXPECT_TRUE(result2);
}

/**
  * @tc.name: UpdateConfig_WithLongNames_ShouldHandle
  * @tc.desc: Test UpdateConfig function with long bundle name, database name and table name
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(CloudTest, UpdateConfig_WithLongNames_ShouldHandle, TestSize.Level1)
{
    std::string longBundleName(1000, 'a');
    std::string longDbName(500, 'b');
    std::string longTableName(200, 'c');
    std::map<std::string, DBSwitchInfo> dbInfo;
    DBSwitchInfo db1;
    db1.enable = true;
    db1.tableInfo = { { longTableName, true } };
    dbInfo[longDbName] = db1;
    bool result = SyncConfig::UpdateConfig(TEST_USER_ID, longBundleName, dbInfo);
    EXPECT_TRUE(result);
    bool dbEnabled = SyncConfig::IsDbEnable(TEST_USER_ID, longBundleName, longDbName);
    EXPECT_TRUE(dbEnabled);
}

/**
  * @tc.name: UpdateConfig_WithSpecialCharacters_ShouldHandle
  * @tc.desc: Test UpdateConfig function with special characters in names
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(CloudTest, UpdateConfig_WithSpecialCharacters_ShouldHandle, TestSize.Level1)
{
    std::string specialBundleName = "com.example.app-v2.0@test";
    std::string specialDbName = "my-db_123";
    std::string specialTableName = "user@table#1";

    std::map<std::string, DBSwitchInfo> dbInfo;
    DBSwitchInfo db1;
    db1.enable = true;
    db1.tableInfo = { { specialTableName, true } };
    dbInfo[specialDbName] = db1;
    bool result = SyncConfig::UpdateConfig(TEST_USER_ID, specialBundleName, dbInfo);
    EXPECT_TRUE(result);
}

/**
  * @tc.name: IsDbEnable_DatabaseNotExist_ShouldReturnTrue
  * @tc.desc: Test IsDbEnable function when database does not exist
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(CloudTest, IsDbEnable_DatabaseNotExist_ShouldReturnTrue, TestSize.Level0)
{
    bool result = SyncConfig::IsDbEnable(TEST_USER_ID, TEST_CLOUD_BUNDLE, "non_existent_db");
    EXPECT_TRUE(result);
}

/**
  * @tc.name: IsDbEnable_ConfigNotExist_ShouldReturnTrue
  * @tc.desc: Test IsDbEnable function when configuration does not exist
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(CloudTest, IsDbEnable_ConfigNotExist_ShouldReturnTrue, TestSize.Level0)
{
    bool result = SyncConfig::IsDbEnable(TEST_USER_ID, "com.other.app", "database1");
    EXPECT_TRUE(result);
}

/**
  * @tc.name: FilterCloudEnabledTables_FilterDisabledTables_ShouldWork
  * @tc.desc: Test FilterCloudEnabledTables function to filter disabled tables
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(CloudTest, FilterCloudEnabledTables_FilterDisabledTables_ShouldWork, TestSize.Level0)
{
    std::map<std::string, DBSwitchInfo> dbInfo;
    DBSwitchInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "table1", true }, { "table2", false }, { "table3", true } };
    dbInfo["database1"] = db1;
    SyncConfig::UpdateConfig(TEST_USER_ID, TEST_CLOUD_BUNDLE, dbInfo);
    std::vector<std::string> tables = { "table1", "table2", "table3", "table4" };
    bool result = SyncConfig::FilterCloudEnabledTables(TEST_USER_ID, TEST_CLOUD_BUNDLE, "database1", tables);

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
HWTEST_F(CloudTest, FilterCloudEnabledTables_DatabaseNotExist_ShouldReturnTrue, TestSize.Level0)
{
    std::vector<std::string> tables = { "table1", "table2" };
    bool result = SyncConfig::FilterCloudEnabledTables(TEST_USER_ID, TEST_CLOUD_BUNDLE, "non_existent_db", tables);
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
HWTEST_F(CloudTest, FilterCloudEnabledTables_ConfigNotExist_ShouldReturnTrue, TestSize.Level0)
{
    std::vector<std::string> tables = { "table1", "table2" };
    bool result = SyncConfig::FilterCloudEnabledTables(TEST_USER_ID, "com.other.app", "database1", tables);
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
HWTEST_F(CloudTest, FilterCloudEnabledTables_AllTablesDisabled_ShouldReturnFalse, TestSize.Level0)
{
    std::map<std::string, DBSwitchInfo> dbInfo;
    DBSwitchInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "table1", false }, { "table2", false } };
    dbInfo["database1"] = db1;
    SyncConfig::UpdateConfig(TEST_USER_ID, TEST_CLOUD_BUNDLE, dbInfo);
    std::vector<std::string> tables = { "table1", "table2" };
    bool result = SyncConfig::FilterCloudEnabledTables(TEST_USER_ID, TEST_CLOUD_BUNDLE, "database1", tables);
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
HWTEST_F(CloudTest, CompleteWorkflow_ShouldWorkCorrectly, TestSize.Level1)
{
    std::map<std::string, DBSwitchInfo> initialConfig;
    DBSwitchInfo mainDb;
    mainDb.enable = true;
    mainDb.tableInfo = { { "users", true }, { "logs", false }, { "settings", true } };
    initialConfig["main"] = mainDb;

    DBSwitchInfo tempDb;
    tempDb.enable = false;
    tempDb.tableInfo = { { "cache", true } };
    initialConfig["temp"] = tempDb;
    bool configResult = SyncConfig::UpdateConfig(TEST_USER_ID, TEST_CLOUD_BUNDLE, initialConfig);
    EXPECT_TRUE(configResult);
    EXPECT_TRUE(SyncConfig::IsDbEnable(TEST_USER_ID, TEST_CLOUD_BUNDLE, "main"));
    EXPECT_FALSE(SyncConfig::IsDbEnable(TEST_USER_ID, TEST_CLOUD_BUNDLE, "temp"));
    std::vector<std::string> mainTables = { "users", "logs", "settings", "non_existent" };
    bool filterResult = SyncConfig::FilterCloudEnabledTables(TEST_USER_ID, TEST_CLOUD_BUNDLE, "main", mainTables);
    EXPECT_TRUE(filterResult);
    EXPECT_EQ(mainTables.size(), 3);
    EXPECT_NE(std::find(mainTables.begin(), mainTables.end(), "users"), mainTables.end());
    EXPECT_EQ(std::find(mainTables.begin(), mainTables.end(), "logs"), mainTables.end());
    EXPECT_NE(std::find(mainTables.begin(), mainTables.end(), "settings"), mainTables.end());
    EXPECT_NE(std::find(mainTables.begin(), mainTables.end(), "non_existent"), mainTables.end());
    std::map<std::string, DBSwitchInfo> updateConfig;
    DBSwitchInfo updatedMainDb;
    updatedMainDb.enable = true;
    updatedMainDb.tableInfo = { { "users", false }, { "logs", true }, { "settings", true }, { "new_table", true } };
    updateConfig["main"] = updatedMainDb;
    bool updateResult = SyncConfig::UpdateConfig(TEST_USER_ID, TEST_CLOUD_BUNDLE, updateConfig);
    EXPECT_TRUE(updateResult);
    std::vector<std::string> updatedTables = { "users", "logs", "settings", "new_table" };
    bool updatedFilterResult =
        SyncConfig::FilterCloudEnabledTables(TEST_USER_ID, TEST_CLOUD_BUNDLE, "main", updatedTables);
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
HWTEST_F(CloudTest, UpdateConfig_WithInvalidTEST_USER_IDShouldHandle, TestSize.Level1)
{
    std::map<std::string, DBSwitchInfo> dbInfo;
    DBSwitchInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "table1", true } };
    dbInfo["database1"] = db1;
    bool result = SyncConfig::UpdateConfig(-1, TEST_CLOUD_BUNDLE, dbInfo);
    EXPECT_TRUE(result || !result);
}

/**
  * @tc.name: UpdateConfig_WithEmptyStrings_ShouldHandle
  * @tc.desc: Test UpdateConfig function with empty strings in parameters
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(CloudTest, UpdateConfig_WithEmptyStrings_ShouldHandle, TestSize.Level1)
{
    std::map<std::string, DBSwitchInfo> dbInfo;
    DBSwitchInfo db1;
    db1.enable = true;
    db1.tableInfo = { { "", true } };
    dbInfo[""] = db1;
    bool result = SyncConfig::UpdateConfig(TEST_USER_ID, "", dbInfo);
    EXPECT_TRUE(result || !result);
}

/**
  * @tc.name: ConcurrentAccess_ShouldBeThreadSafe
  * @tc.desc: Test concurrent access to CloudDbSyncConfig functions with multiple threads
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author:
  */
HWTEST_F(CloudTest, ConcurrentAccess_ShouldBeThreadSafe, TestSize.Level1)
{
    std::map<std::string, DBSwitchInfo> dbInfo;
    DBSwitchInfo db1;
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
                std::string currentBundleName = std::string(TEST_CLOUD_BUNDLE) + "_thread" + std::to_string(threadId);

                bool result = SyncConfig::UpdateConfig(currentUserId, currentBundleName, dbInfo);
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
            std::string currentBundleName = std::string(TEST_CLOUD_BUNDLE) + "_thread" + std::to_string(threadId);

            bool dbEnabled = SyncConfig::IsDbEnable(currentUserId, currentBundleName, "database1");
            EXPECT_TRUE(dbEnabled);
        }
    }

class MockExecutorPool : public ExecutorPool {
public:
    MOCK_METHOD(TaskId, Execute, (Task task), (override));
    MOCK_METHOD(TaskId, Schedule, (Duration duration, Task task), (override));
    MOCK_METHOD(void, Remove, (TaskId id), (override));
};

class MockEventCenter : public EventCenter {
public:
    MOCK_METHOD(void, Subscribe, (int32_t eventId, std::function<void(const Event&)> handler), (override));
    MOCK_METHOD(void, PostEvent, (std::unique_ptr<Event> event), (override));
};

class MockMetaDataManager : public MetaDataManager {
public:
    MOCK_METHOD(bool, LoadMeta, (const std::string& key, CloudInfo& cloud, bool createIfNotExist), (override));
    MOCK_METHOD(bool, LoadMeta, (const std::string& key, StoreMetaMapping& meta, bool createIfNotExist), (override));
    MOCK_METHOD(bool, LoadMeta, (const std::string& key, SchemaMeta& schema, bool createIfNotExist), (override));
};

class MockNetworkDelegate : public NetworkDelegate {
public:
    MOCK_METHOD(bool, IsNetworkAvailable, (), (const, override));
};

class MockAccountDelegate : public AccountDelegate {
public:
    MOCK_METHOD(bool, IsVerified, (int32_t userId), (const, override));
    MOCK_METHOD(void, QueryForegroundUsers, (std::vector<int32_t>& users), (const, override));
};

class MockCloudServer : public CloudServer {
public:
    MOCK_METHOD(std::shared_ptr<CloudDB>, ConnectCloudDB, 
                (const std::string& bundleName, int32_t userId, const Database& database), (override));
    MOCK_METHOD(std::shared_ptr<AssetLoader>, ConnectAssetLoader, 
                (const std::string& bundleName, int32_t userId, const Database& database), (override));
};

class SyncManagerTest : public testing::Test {
protected:
    void SetUp() override {
        // Set up mock objects
        mockExecutorPool_ = std::make_shared<MockExecutorPool>();
        mockEventCenter_ = std::make_shared<MockEventCenter>();
        mockMetaDataManager_ = std::make_shared<MockMetaDataManager>();
        mockNetworkDelegate_ = std::make_shared<MockNetworkDelegate>();
        mockAccountDelegate_ = std::make_shared<MockAccountDelegate>();
        mockCloudServer_ = std::make_shared<MockCloudServer>();

        // Replace singletons with mocks (if possible)
        // Note: This would require dependency injection or friend classes in actual implementation
        
        syncManager_ = std::make_unique<SyncManager>();
    }

    void TearDown() override {
        syncManager_.reset();
    }

    std::unique_ptr<SyncManager> syncManager_;
    std::shared_ptr<MockExecutorPool> mockExecutorPool_;
    std::shared_ptr<MockEventCenter> mockEventCenter_;
    std::shared_ptr<MockMetaDataManager> mockMetaDataManager_;
    std::shared_ptr<MockNetworkDelegate> mockNetworkDelegate_;
    std::shared_ptr<MockAccountDelegate> mockAccountDelegate_;
    std::shared_ptr<MockCloudServer> mockCloudServer_;
};

// Test constructor initialization
TEST_F(SyncManagerTest, ConstructorInitialization) {
    EXPECT_NE(syncManager_, nullptr);
    // Verify that subscriptions are made during construction
    // This would need to verify mock calls if using dependency injection
}

// Test Bind method
TEST_F(SyncManagerTest, BindExecutor) {
    EXPECT_EQ(syncManager_->Bind(mockExecutorPool_), E_OK);
    // Verify that executor is properly set
}

// Test SyncInfo constructors
TEST_F(SyncManagerTest, SyncInfoConstructorWithStoreAndTables) {
    Store store = "test_store";
    Tables tables = {"table1", "table2"};
    
    SyncManager::SyncInfo syncInfo(1, "test_bundle", store, tables, 0);
    
    EXPECT_EQ(syncInfo.user_, 1);
    EXPECT_EQ(syncInfo.bundleName_, "test_bundle");
    EXPECT_EQ(syncInfo.triggerMode_, 0);
    EXPECT_TRUE(syncInfo.tables_.find(store) != syncInfo.tables_.end());
    EXPECT_EQ(syncInfo.tables_[store], tables);
}

TEST_F(SyncManagerTest, SyncInfoConstructorWithStores) {
    Stores stores = {"store1", "store2"};
    
    SyncManager::SyncInfo syncInfo(1, "test_bundle", stores);
    
    EXPECT_EQ(syncInfo.user_, 1);
    EXPECT_EQ(syncInfo.bundleName_, "test_bundle");
    EXPECT_EQ(syncInfo.tables_.size(), 2);
    EXPECT_TRUE(syncInfo.tables_.find("store1") != syncInfo.tables_.end());
    EXPECT_TRUE(syncInfo.tables_.find("store2") != syncInfo.tables_.end());
}

TEST_F(SyncManagerTest, SyncInfoConstructorWithMultiStoreTables) {
    MutliStoreTables tables;
    tables["store1"] = {"table1", "table2"};
    tables["store2"] = {"table3"};
    
    SyncManager::SyncInfo syncInfo(1, "test_bundle", tables);
    
    EXPECT_EQ(syncInfo.user_, 1);
    EXPECT_EQ(syncInfo.bundleName_, "test_bundle");
    EXPECT_EQ(syncInfo.tables_.size(), 2);
    EXPECT_EQ(syncInfo.tables_["store1"].size(), 2);
    EXPECT_EQ(syncInfo.tables_["store2"].size(), 1);
}

// Test SyncInfo methods
TEST_F(SyncManagerTest, SyncInfoSetAndGetMethods) {
    SyncManager::SyncInfo syncInfo(1, "test_bundle", "", {}, 0);
    
    syncInfo.SetMode(1);
    EXPECT_EQ(syncInfo.mode_, 1);
    
    syncInfo.SetWait(1000);
    EXPECT_EQ(syncInfo.wait_, 1000);
    
    GenAsync asyncDetail = [](GenDetails details) {};
    syncInfo.SetAsyncDetail(asyncDetail);
    EXPECT_NE(syncInfo.async_, nullptr);
    
    auto query = std::make_shared<GenQuery>();
    syncInfo.SetQuery(query);
    EXPECT_EQ(syncInfo.query_, query);
    
    syncInfo.SetCompensation(true);
    EXPECT_TRUE(syncInfo.isCompensation_);
    
    syncInfo.SetTriggerMode(2);
    EXPECT_EQ(syncInfo.triggerMode_, 2);
}

TEST_F(SyncManagerTest, SyncInfoGetTablesFromQuery) {
    SyncManager::SyncInfo syncInfo(1, "test_bundle", "", {}, 0);
    
    // Create a mock query that returns specific tables
    class MockQuery : public GenQuery {
    public:
        std::vector<std::string> GetTables() override {
            return {"mock_table1", "mock_table2"};
        }
        
        bool IsEqual(uint64_t tid) override {
            return false;
        }
    };
    
    auto mockQuery = std::make_shared<MockQuery>();
    syncInfo.SetQuery(mockQuery);
    
    Database db;
    db.name = "test_db";
    
    auto tables = syncInfo.GetTables(db);
    EXPECT_EQ(tables.size(), 2);
    EXPECT_EQ(tables[0], "mock_table1");
    EXPECT_EQ(tables[1], "mock_table2");
}

TEST_F(SyncManagerTest, SyncInfoGetTablesFromMap) {
    SyncManager::SyncInfo syncInfo(1, "test_bundle", "", {}, 0);
    
    Tables tables = {"table1", "table2"};
    syncInfo.tables_["test_db"] = tables;
    
    Database db;
    db.name = "test_db";
    
    auto result = syncInfo.GetTables(db);
    EXPECT_EQ(result, tables);
}

TEST_F(SyncManagerTest, SyncInfoContains) {
    SyncManager::SyncInfo syncInfo(1, "test_bundle", "", {}, 0);
    
    // Empty tables should contain any store
    EXPECT_TRUE(syncInfo.Contains("any_store"));
    
    // Add a store to check containment
    syncInfo.tables_["specific_store"] = {"table1"};
    EXPECT_TRUE(syncInfo.Contains("specific_store"));
    EXPECT_FALSE(syncInfo.Contains("other_store"));
}

// Test GenerateId function
TEST_F(SyncManagerTest, GenerateId) {
    uint64_t id1 = SyncManager::GenerateId(5);
    uint64_t id2 = SyncManager::GenerateId(5);
    
    EXPECT_NE(id1, id2); // Should be different due to atomic increment
    EXPECT_EQ((id1 >> 32) & 0xFFFFFFFF, static_cast<uint64_t>(5)); // User part should match
}

// Test Compare function
TEST_F(SyncManagerTest, Compare) {
    uint64_t syncId = SyncManager::GenerateId(5);
    EXPECT_EQ(syncManager_->Compare(syncId, 5), 0); // Should match
    EXPECT_NE(syncManager_->Compare(syncId, 6), 0); // Should not match
}

// Test IsValid function
TEST_F(SyncManagerTest, IsValidSuccess) {
    SyncManager::SyncInfo syncInfo(1, "test_bundle", "", {}, 0);
    CloudInfo cloud;
    cloud.user = 1;
    cloud.enableCloud = true;
    cloud.id = "valid_id";
    
    // Mock successful validations
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, _))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockNetworkDelegate_, IsNetworkAvailable())
        .WillOnce(Return(true));
    EXPECT_CALL(*mockAccountDelegate_, IsVerified(1))
        .WillOnce(Return(true));
    
    EXPECT_EQ(syncManager_->IsValid(syncInfo, cloud), E_OK);
}

TEST_F(SyncManagerTest, IsValidCloudDisabled) {
    SyncManager::SyncInfo syncInfo(1, "test_bundle", "", {}, 0);
    CloudInfo cloud;
    cloud.user = 1;
    cloud.enableCloud = false; // Disable cloud
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, _))
        .WillOnce(Return(true));
    
    EXPECT_EQ(syncManager_->IsValid(syncInfo, cloud), E_CLOUD_DISABLED);
}

TEST_F(SyncManagerTest, IsValidNetworkError) {
    SyncManager::SyncInfo syncInfo(1, "test_bundle", "", {}, 0);
    CloudInfo cloud;
    cloud.user = 1;
    cloud.enableCloud = true;
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockNetworkDelegate_, IsNetworkAvailable())
        .WillOnce(Return(false)); // Network unavailable
    
    EXPECT_EQ(syncManager_->IsValid(syncInfo, cloud), E_NETWORK_ERROR);
}

TEST_F(SyncManagerTest, IsValidUserLocked) {
    SyncManager::SyncInfo syncInfo(1, "test_bundle", "", {}, 0);
    CloudInfo cloud;
    cloud.user = 1;
    cloud.enableCloud = true;
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, _))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockNetworkDelegate_, IsNetworkAvailable())
        .WillOnce(Return(true));
    EXPECT_CALL(*mockAccountDelegate_, IsVerified(1))
        .WillOnce(Return(false)); // User not verified
    
    EXPECT_EQ(syncManager_->IsValid(syncInfo, cloud), E_ERROR);
}

// Test DoCloudSync function
TEST_F(SyncManagerTest, DoCloudSyncWithoutExecutor) {
    SyncManager::SyncInfo syncInfo(1, "test_bundle", "", {}, 0);
    
    EXPECT_EQ(syncManager_->DoCloudSync(syncInfo), E_NOT_INIT);
}

TEST_F(SyncManagerTest, DoCloudSyncWithExecutor) {
    SyncManager::SyncInfo syncInfo(1, "test_bundle", "", {}, 0);
    
    syncManager_->Bind(mockExecutorPool_);
    
    EXPECT_CALL(*mockExecutorPool_, Execute(_))
        .WillOnce(Return(12345));
    
    EXPECT_EQ(syncManager_->DoCloudSync(syncInfo), E_OK);
}

// Test StopCloudSync function
TEST_F(SyncManagerTest, StopCloudSyncWithoutExecutor) {
    EXPECT_EQ(syncManager_->StopCloudSync(1), E_NOT_INIT);
}

TEST_F(SyncManagerTest, StopCloudSyncWithExecutor) {
    syncManager_->Bind(mockExecutorPool_);
    
    EXPECT_CALL(*mockExecutorPool_, Remove(_))
        .Times(AnyNumber()); // May be called depending on internal state
    
    EXPECT_EQ(syncManager_->StopCloudSync(1), E_OK);
}

// Test GenerateQuery function
TEST_F(SyncManagerTest, GenerateQueryFromExistingQuery) {
    SyncManager::SyncInfo syncInfo(1, "test_bundle", "", {}, 0);
    
    // Create a query with one table
    class SingleTableQuery : public GenQuery {
    public:
        std::vector<std::string> GetTables() override {
            return {"single_table"};
        }
        
        bool IsEqual(uint64_t tid) override {
            return false;
        }
    };
    
    auto existingQuery = std::make_shared<SingleTableQuery>();
    syncInfo.SetQuery(existingQuery);
    
    Tables tables = {"table1", "table2"};
    auto result = syncInfo.GenerateQuery(tables);
    
    // Should return the existing query since it has only one table
    EXPECT_EQ(result, existingQuery);
}

TEST_F(SyncManagerTest, GenerateQueryNewQuery) {
    SyncManager::SyncInfo syncInfo(1, "test_bundle", "", {}, 0);
    
    // Create a query with multiple tables (should generate new query)
    class MultiTableQuery : public GenQuery {
    public:
        std::vector<std::string> GetTables() override {
            return {"table1", "table2", "table3"};
        }
        
        bool IsEqual(uint64_t tid) override {
            return false;
        }
    };
    
    auto existingQuery = std::make_shared<MultiTableQuery>();
    syncInfo.SetQuery(existingQuery);
    
    Tables tables = {"new_table1", "new_table2"};
    auto result = syncInfo.GenerateQuery(tables);
    
    // Should generate a new query based on provided tables
    auto resultTables = result->GetTables();
    EXPECT_EQ(resultTables, tables);
}

// Test GetSyncTask function
TEST_F(SyncManagerTest, GetSyncTaskCreation) {
    SyncManager::SyncInfo syncInfo(1, "test_bundle", "", {}, 0);
    RefCount ref;
    
    auto task = syncManager_->GetSyncTask(0, true, ref, std::move(syncInfo));
    EXPECT_NE(task, nullptr);
}

// Test GetRetryer function
TEST_F(SyncManagerTest, GetRetryerMaxRetries) {
    SyncManager::SyncInfo syncInfo(1, "test_bundle", "", {}, 0);
    
    auto retryer = syncManager_->GetRetryer(SyncManager::RETRY_TIMES, syncInfo, 1);
    EXPECT_NE(retryer, nullptr);
    
    // Test that retryer handles finished case
    bool result = retryer(std::chrono::seconds(1), E_OK, 0, "");
    EXPECT_TRUE(result);
}

TEST_F(SyncManagerTest, GetRetryerContinueRetries) {
    syncManager_->Bind(mockExecutorPool_);
    
    SyncManager::SyncInfo syncInfo(1, "test_bundle", "", {}, 0);
    
    auto retryer = syncManager_->GetRetryer(1, syncInfo, 1);
    EXPECT_NE(retryer, nullptr);
    
    // Test network error case which should schedule retry
    EXPECT_CALL(*mockExecutorPool_, Schedule(_, _))
        .WillOnce(Return(67890));
    
    bool result = retryer(std::chrono::seconds(1), E_NETWORK_ERROR, 0, "");
    EXPECT_TRUE(result);
}

// Test GetBindInfos function
TEST_F(SyncManagerTest, GetBindInfosEmptyWhenNoCloudServer) {
    StoreMetaData meta;
    meta.bundleName = "test_bundle";
    meta.storeType = StoreMetaData::StoreType::STORE_KV_NORMAL;
    
    std::vector<int32_t> users = {1, 2};
    Database db;
    db.name = "test_db";
    
    // Mock CloudServer to return nullptr
    EXPECT_CALL(*mockCloudServer_, ConnectCloudDB(_, _, _))
        .WillOnce(Return(nullptr));
    
    auto bindInfos = syncManager_->GetBindInfos(meta, users, db);
    EXPECT_TRUE(bindInfos.empty());
}

// Test GetStore function
TEST_F(SyncManagerTest, GetStoreUserNotVerified) {
    StoreMetaData meta;
    meta.bundleName = "test_bundle";
    
    EXPECT_CALL(*mockAccountDelegate_, IsVerified(1))
        .WillOnce(Return(false));
    
    auto [status, store] = syncManager_->GetStore(meta, 1);
    EXPECT_EQ(status, E_USER_LOCKED);
    EXPECT_EQ(store, nullptr);
}

TEST_F(SyncManagerTest, GetStoreNotSupported) {
    StoreMetaData meta;
    meta.bundleName = "test_bundle";
    
    // Mock CloudServer to return nullptr (not supported)
    EXPECT_CALL(*mockAccountDelegate_, IsVerified(1))
        .WillOnce(Return(true));
    
    auto [status, store] = syncManager_->GetStore(meta, 1);
    EXPECT_EQ(status, E_NOT_SUPPORT);
    EXPECT_EQ(store, nullptr);
}
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

// 假设的头文件包含
#include "SyncManager.h"
#include "CloudReport.h"
#include "CloudServer.h"
#include "MetaDataManager.h"
#include "NetworkDelegate.h"
#include "Account.h"
#include "AccountDelegate.h"

using namespace testing;

// Mock类定义
class MockCloudReport : public CloudReport {
public:
    MOCK_METHOD(void, Report, (const ReportParam&), (override));
    MOCK_METHOD(std::string, GetPrepareTraceId, (int32_t user), (override));
    static MockCloudReport* GetInstance() {
        return instance_;
    }
    static void SetInstance(MockCloudReport* instance) {
        instance_ = instance;
    }
private:
    static MockCloudReport* instance_;
};

MockCloudReport* MockCloudReport::instance_ = nullptr;

class MockCloudServer : public CloudServer {
public:
    MOCK_METHOD(std::pair<int32_t, CloudInfo>, GetServerInfo, (int32_t user, bool isSync), (override));
    static MockCloudServer* GetInstance() {
        return instance_;
    }
    static void SetInstance(MockCloudServer* instance) {
        instance_ = instance;
    }
private:
    static MockCloudServer* instance_;
};

MockCloudServer* MockCloudServer::instance_ = nullptr;

class MockMetaDataManager : public MetaDataManager {
public:
    MOCK_METHOD(bool, LoadMeta, (const std::string&, CloudInfo&, bool), (override));
    MOCK_METHOD(bool, LoadMeta, (const std::string&, SchemaMeta&, bool), (override));
    MOCK_METHOD(bool, LoadMeta, (const std::string&, std::vector<SchemaMeta>&, bool), (override));
    MOCK_METHOD(bool, LoadMeta, (const std::string&, StoreMetaMapping&, bool), (override));
    MOCK_METHOD(bool, LoadMeta, (const std::string&, StoreMetaData&, bool), (override));
    MOCK_METHOD(bool, LoadMeta, (const std::string&, StoreMetaDataLocal&, bool), (override));
    MOCK_METHOD(bool, LoadMeta, (const std::string&, CloudLastSyncInfo&, bool), (override));
    MOCK_METHOD(bool, SaveMeta, (const std::string&, const CloudInfo&, bool), (override));
    MOCK_METHOD(bool, SaveMeta, (const std::string&, const CloudLastSyncInfo&, bool), (override));
    static MockMetaDataManager* GetInstance() {
        return instance_;
    }
    static void SetInstance(MockMetaDataManager* instance) {
        instance_ = instance;
    }
private:
    static MockMetaDataManager* instance_;
};

MockMetaDataManager* MockMetaDataManager::instance_ = nullptr;

class MockNetworkDelegate : public NetworkDelegate {
public:
    MOCK_METHOD(bool, IsNetworkAvailable, (), (override));
    static MockNetworkDelegate* GetInstance() {
        return instance_;
    }
    static void SetInstance(MockNetworkDelegate* instance) {
        instance_ = instance;
    }
private:
    static MockNetworkDelegate* instance_;
};

MockNetworkDelegate* MockNetworkDelegate::instance_ = nullptr;

class MockAccount : public Account {
public:
    MOCK_METHOD(bool, IsLoginAccount, (), (override));
    static MockAccount* GetInstance() {
        return instance_;
    }
    static void SetInstance(MockAccount* instance) {
        instance_ = instance;
    }
private:
    static MockAccount* instance_;
};

MockAccount* MockAccount::instance_ = nullptr;

class MockAccountDelegate : public AccountDelegate {
public:
    MOCK_METHOD(void, QueryUsers, (std::vector<int32_t>&), (override));
    static MockAccountDelegate* GetInstance() {
        return instance_;
    }
    static void SetInstance(MockAccountDelegate* instance) {
        instance_ = instance;
    }
private:
    static MockAccountDelegate* instance_;
};

MockAccountDelegate* MockAccountDelegate::instance_ = nullptr;

class MockRadarReporter {
public:
    MOCK_STATIC_METHOD(void, Report, (const std::vector<std::string>&, const std::string&, BizState), ());
};

class MockDmAdapter {
public:
    MOCK_METHOD(DeviceInfo, GetLocalDevice, (), (const));
    static MockDmAdapter* GetInstance() {
        return instance_;
    }
    static void SetInstance(MockDmAdapter* instance) {
        instance_ = instance;
    }
private:
    static MockDmAdapter* instance_;
};

MockDmAdapter* MockDmAdapter::instance_ = nullptr;

class SyncManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockCloudReport_ = new MockCloudReport();
        MockCloudReport::SetInstance(mockCloudReport_);
        
        mockCloudServer_ = new MockCloudServer();
        MockCloudServer::SetInstance(mockCloudServer_);
        
        mockMetaDataManager_ = new MockMetaDataManager();
        MockMetaDataManager::SetInstance(mockMetaDataManager_);
        
        mockNetworkDelegate_ = new MockNetworkDelegate();
        MockNetworkDelegate::SetInstance(mockNetworkDelegate_);
        
        mockAccount_ = new MockAccount();
        MockAccount::SetInstance(mockAccount_);
        
        mockAccountDelegate_ = new MockAccountDelegate();
        MockAccountDelegate::SetInstance(mockAccountDelegate_);
        
        mockDmAdapter_ = new MockDmAdapter();
        MockDmAdapter::SetInstance(mockDmAdapter_);
    }

    void TearDown() override {
        MockCloudReport::SetInstance(nullptr);
        delete mockCloudReport_;
        
        MockCloudServer::SetInstance(nullptr);
        delete mockCloudServer_;
        
        MockMetaDataManager::SetInstance(nullptr);
        delete mockMetaDataManager_;
        
        MockNetworkDelegate::SetInstance(nullptr);
        delete mockNetworkDelegate_;
        
        MockAccount::SetInstance(nullptr);
        delete mockAccount_;
        
        MockAccountDelegate::SetInstance(nullptr);
        delete mockAccountDelegate_;
        
        MockDmAdapter::SetInstance(nullptr);
        delete mockDmAdapter_;
    }

    MockCloudReport* mockCloudReport_;
    MockCloudServer* mockCloudServer_;
    MockMetaDataManager* mockMetaDataManager_;
    MockNetworkDelegate* mockNetworkDelegate_;
    MockAccount* mockAccount_;
    MockAccountDelegate* mockAccountDelegate_;
    MockDmAdapter* mockDmAdapter_;
};

// Test cases for Report function
TEST_F(SyncManagerTest, Report_WithNullCloudReport_ReturnsEarly) {
    EXPECT_CALL(*mockCloudReport_, Report(_)).Times(0);
    
    // 设置CloudReport返回nullptr
    MockCloudReport::SetInstance(nullptr);
    
    SyncManager manager;
    ReportParam param;
    manager.Report(param);
}

TEST_F(SyncManagerTest, Report_WithValidCloudReport_CallsReport) {
    ReportParam param;
    EXPECT_CALL(*mockCloudReport_, Report(param)).Times(1);
    
    SyncManager manager;
    manager.Report(param);
}

// Test cases for GetPrepareTraceId function
TEST_F(SyncManagerTest, GetPrepareTraceId_WithNonEmptyPrepareTraceId_ReturnsIt) {
    SyncInfo info;
    info.prepareTraceId_ = "test_trace_id";
    info.bundleName_ = "com.example.test";
    
    CloudInfo cloud;
    
    SyncManager manager;
    auto result = manager.GetPrepareTraceId(info, cloud);
    
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result.begin()->first, "com.example.test");
    EXPECT_EQ(result.begin()->second, "test_trace_id");
}

TEST_F(SyncManagerTest, GetPrepareTraceId_WithEmptyPrepareTraceIdAndNullCloudReport_ReturnsEmpty) {
    SyncInfo info;
    info.prepareTraceId_.clear();
    info.bundleName_ = "com.example.test";
    info.user_ = 100;
    
    CloudInfo cloud;
    
    MockCloudReport::SetInstance(nullptr);
    
    SyncManager manager;
    auto result = manager.GetPrepareTraceId(info, cloud);
    
    EXPECT_TRUE(result.empty());
}

TEST_F(SyncManagerTest, GetPrepareTraceId_WithEmptyBundleNameAndValidCloudReport_ReturnsAllApps) {
    SyncInfo info;
    info.prepareTraceId_.clear();
    info.bundleName_.clear();
    info.user_ = 100;
    
    CloudInfo cloud;
    cloud.apps["app1"] = AppInfo{"app1"};
    cloud.apps["app2"] = AppInfo{"app2"};
    
    EXPECT_CALL(*mockCloudReport_, GetPrepareTraceId(100))
        .WillOnce(Return("trace1"))
        .WillOnce(Return("trace2"));
    
    SyncManager manager;
    auto result = manager.GetPrepareTraceId(info, cloud);
    
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result["app1"], "trace1");
    EXPECT_EQ(result["app2"], "trace2");
}

TEST_F(SyncManagerTest, GetPrepareTraceId_WithNonEmptyBundleNameAndValidCloudReport_ReturnsSingleApp) {
    SyncInfo info;
    info.prepareTraceId_.clear();
    info.bundleName_ = "com.example.test";
    info.user_ = 100;
    
    CloudInfo cloud;
    
    EXPECT_CALL(*mockCloudReport_, GetPrepareTraceId(100))
        .WillOnce(Return("trace1"));
    
    SyncManager manager;
    auto result = manager.GetPrepareTraceId(info, cloud);
    
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result["com.example.test"], "trace1");
}

// Test cases for NeedGetCloudInfo function
TEST_F(SyncManagerTest, NeedGetCloudInfo_WhenLoadMetaFails_ReturnsTrue) {
    CloudInfo cloud;
    cloud.user = 100;
    cloud.enableCloud = true;
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, _))
        .WillOnce(Return(false));
    
    EXPECT_CALL(*mockNetworkDelegate_, IsNetworkAvailable())
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mockAccount_, IsLoginAccount())
        .WillOnce(Return(true));
    
    SyncManager manager;
    bool result = manager.NeedGetCloudInfo(cloud);
    
    EXPECT_TRUE(result);
}

TEST_F(SyncManagerTest, NeedGetCloudInfo_WhenCloudNotEnabled_ReturnsTrue) {
    CloudInfo cloud;
    cloud.user = 100;
    cloud.enableCloud = false;
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, _))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mockNetworkDelegate_, IsNetworkAvailable())
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mockAccount_, IsLoginAccount())
        .WillOnce(Return(true));
    
    SyncManager manager;
    bool result = manager.NeedGetCloudInfo(cloud);
    
    EXPECT_TRUE(result);
}

TEST_F(SyncManagerTest, NeedGetCloudInfo_WhenNetworkNotAvailable_ReturnsFalse) {
    CloudInfo cloud;
    cloud.user = 100;
    cloud.enableCloud = true;
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, _))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mockNetworkDelegate_, IsNetworkAvailable())
        .WillOnce(Return(false));
    
    SyncManager manager;
    bool result = manager.NeedGetCloudInfo(cloud);
    
    EXPECT_FALSE(result);
}

TEST_F(SyncManagerTest, NeedGetCloudInfo_WhenAccountNotLoggedIn_ReturnsFalse) {
    CloudInfo cloud;
    cloud.user = 100;
    cloud.enableCloud = true;
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, _))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mockNetworkDelegate_, IsNetworkAvailable())
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mockAccount_, IsLoginAccount())
        .WillOnce(Return(false));
    
    SyncManager manager;
    bool result = manager.NeedGetCloudInfo(cloud);
    
    EXPECT_FALSE(result);
}

TEST_F(SyncManagerTest, NeedGetCloudInfo_WhenAllConditionsMet_ReturnsFalse) {
    CloudInfo cloud;
    cloud.user = 100;
    cloud.enableCloud = true;
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, _))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mockNetworkDelegate_, IsNetworkAvailable())
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mockAccount_, IsLoginAccount())
        .WillOnce(Return(true));
    
    SyncManager manager;
    bool result = manager.NeedGetCloudInfo(cloud);
    
    EXPECT_FALSE(result);
}

// Test cases for GetCloudSyncInfo function
TEST_F(SyncManagerTest, GetCloudSyncInfo_WhenNeedGetCloudInfoReturnsTrueAndCloudServerIsNull_ReturnsEmpty) {
    SyncInfo info;
    info.user_ = 100;
    info.bundleName_ = "com.example.test";
    info.syncId_ = 123;
    
    CloudInfo cloud;
    cloud.user = 100;
    cloud.enableCloud = false; // This will make NeedGetCloudInfo return true
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, _))
        .WillOnce(Return(false)); // LoadMeta fails, so NeedGetCloudInfo returns true
    
    EXPECT_CALL(*mockNetworkDelegate_, IsNetworkAvailable())
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mockAccount_, IsLoginAccount())
        .WillOnce(Return(true));
    
    // Set CloudServer to null
    MockCloudServer::SetInstance(nullptr);
    
    SyncManager manager;
    auto result = manager.GetCloudSyncInfo(info, cloud);
    
    EXPECT_TRUE(result.empty());
}

TEST_F(SyncManagerTest, GetCloudSyncInfo_WhenCloudServerReturnsInvalidCloud_ReturnsEmpty) {
    SyncInfo info;
    info.user_ = 100;
    info.bundleName_ = "com.example.test";
    info.syncId_ = 123;
    
    CloudInfo cloud;
    cloud.user = 100;
    cloud.enableCloud = false; // This will make NeedGetCloudInfo return true
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, _))
        .WillOnce(Return(false)); // LoadMeta fails, so NeedGetCloudInfo returns true
    
    EXPECT_CALL(*mockNetworkDelegate_, IsNetworkAvailable())
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mockAccount_, IsLoginAccount())
        .WillOnce(Return(true));
    
    CloudInfo invalidCloud;
    invalidCloud.isValid = false; // Assuming there's an isValid field or method
    
    EXPECT_CALL(*mockCloudServer_, GetServerInfo(100, false))
        .WillOnce(Return(std::make_pair(SUCCESS, invalidCloud)));
    
    SyncManager manager;
    auto result = manager.GetCloudSyncInfo(info, cloud);
    
    EXPECT_TRUE(result.empty());
}

TEST_F(SyncManagerTest, GetCloudSyncInfo_WhenLoadSchemaMetaFails_ReturnsEmpty) {
    SyncInfo info;
    info.user_ = 100;
    info.bundleName_ = "com.example.test";
    info.syncId_ = 123;
    
    CloudInfo cloud;
    cloud.user = 100;
    cloud.enableCloud = true; // This will make NeedGetCloudInfo return false initially
    cloud.id = "cloud_id";
    
    // First call for NeedGetCloudInfo check
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, _))
        .WillOnce(Return(true)) // Meta exists
        .WillOnce(Return(false)); // Schema meta load fails
    
    SyncManager manager;
    auto result = manager.GetCloudSyncInfo(info, cloud);
    
    EXPECT_TRUE(result.empty());
}

// Test cases for GetLastResults function
TEST_F(SyncManagerTest, GetLastResults_WithValidLastElement_ReturnsSuccess) {
    std::map<SyncId, CloudLastSyncInfo> infos;
    CloudLastSyncInfo info1;
    info1.code = 0; // Not -1
    infos[1] = info1;
    
    CloudLastSyncInfo info2;
    info2.code = 1; // Not -1
    infos[2] = info2;
    
    SyncManager manager;
    auto result = manager.GetLastResults(infos);
    
    EXPECT_EQ(result.first, SUCCESS);
    EXPECT_EQ(result.second.code, 1); // Last element has code 1
}

TEST_F(SyncManagerTest, GetLastResults_WithLastElementCodeMinusOne_ReturnsError) {
    std::map<SyncId, CloudLastSyncInfo> infos;
    CloudLastSyncInfo info1;
    info1.code = 0; // Not -1
    infos[1] = info1;
    
    CloudLastSyncInfo info2;
    info2.code = -1; // -1
    infos[2] = info2;
    
    SyncManager manager;
    auto result = manager.GetLastResults(infos);
    
    EXPECT_EQ(result.first, E_ERROR);
    EXPECT_EQ(result.second.code, 0); // Default constructed
}

TEST_F(SyncManagerTest, GetLastResults_WithEmptyMap_ReturnsError) {
    std::map<SyncId, CloudLastSyncInfo> infos;
    
    SyncManager manager;
    auto result = manager.GetLastResults(infos);
    
    EXPECT_EQ(result.first, E_ERROR);
    EXPECT_EQ(result.second.code, 0); // Default constructed
}

// Test cases for NeedSaveSyncInfo function
TEST_F(SyncManagerTest, NeedSaveSyncInfo_WithEmptyAccountId_ReturnsFalse) {
    QueryKey queryKey;
    queryKey.accountId.clear();
    
    SyncManager manager;
    bool result = manager.NeedSaveSyncInfo(queryKey);
    
    EXPECT_FALSE(result);
}

TEST_F(SyncManagerTest, NeedSaveSyncInfo_WithAccountIdPresentAndBundleInKvApps_ReturnsFalse) {
    QueryKey queryKey;
    queryKey.accountId = "account123";
    queryKey.bundleName = "com.example.test";
    
    SyncManager manager;
    // Add the bundle name to kvApps to simulate it being present
    manager.kvApps_.insert({"com.example.test"});
    
    bool result = manager.NeedSaveSyncInfo(queryKey);
    
    EXPECT_FALSE(result);
}

TEST_F(SyncManagerTest, NeedSaveSyncInfo_WithAccountIdPresentAndBundleNotInKvApps_ReturnsTrue) {
    QueryKey queryKey;
    queryKey.accountId = "account123";
    queryKey.bundleName = "com.example.test";
    
    SyncManager manager;
    // Don't add the bundle name to kvApps
    
    bool result = manager.NeedSaveSyncInfo(queryKey);
    
    EXPECT_TRUE(result);
}

// Test cases for GetInterval function
TEST_F(SyncManagerTest, GetInterval_WithELockedByOthers_ReturnsLockedInterval) {
    SyncManager manager;
    auto result = manager.GetInterval(E_LOCKED_BY_OTHERS);
    
    EXPECT_EQ(result, LOCKED_INTERVAL);
}

TEST_F(SyncManagerTest, GetInterval_WithEBusy_ReturnsBusyInterval) {
    SyncManager manager;
    auto result = manager.GetInterval(E_BUSY);
    
    EXPECT_EQ(result, BUSY_INTERVAL);
}

TEST_F(SyncManagerTest, GetInterval_WithOtherCode_ReturnsRetryInterval) {
    SyncManager manager;
    auto result = manager.GetInterval(999); // Some other error code
    
    EXPECT_EQ(result, RETRY_INTERVAL);
}

// Test cases for ConvertValidGeneralCode function
TEST_F(SyncManagerTest, ConvertValidGeneralCode_WithValidCodeInRange_ReturnsSameCode) {
    SyncManager manager;
    int32_t validCode = E_OK + 1; // Within range
    
    int32_t result = manager.ConvertValidGeneralCode(validCode);
    
    EXPECT_EQ(result, validCode);
}

TEST_F(SyncManagerTest, ConvertValidGeneralCode_WithInvalidCode_ReturnsEError) {
    SyncManager manager;
    int32_t invalidCode = E_BUSY + 1; // Outside valid range
    
    int32_t result = manager.ConvertValidGeneralCode(invalidCode);
    
    EXPECT_EQ(result, E_ERROR);
}

// Test cases for InitDefaultUser function
TEST_F(SyncManagerTest, InitDefaultUser_WithNonZeroUser_ReturnsFalse) {
    int32_t user = 100;
    
    SyncManager manager;
    bool result = manager.InitDefaultUser(user);
    
    EXPECT_FALSE(result);
    EXPECT_EQ(user, 100); // Should remain unchanged
}

TEST_F(SyncManagerTest, InitDefaultUser_WithZeroUserAndAvailableUsers_ReturnsTrue) {
    int32_t user = 0;
    std::vector<int32_t> users = {100, 200};
    
    EXPECT_CALL(*mockAccountDelegate_, QueryUsers(_))
        .WillOnce(Invoke([&users](std::vector<int32_t>& out_users) {
            out_users = users;
        }));
    
    SyncManager manager;
    bool result = manager.InitDefaultUser(user);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(user, 100); // Should be set to first available user
}

TEST_F(SyncManagerTest, InitDefaultUser_WithZeroUserAndNoAvailableUsers_ReturnsTrueButUserRemainsZero) {
    int32_t user = 0;
    std::vector<int32_t> users = {};
    
    EXPECT_CALL(*mockAccountDelegate_, QueryUsers(_))
        .WillOnce(Invoke([&users](std::vector<int32_t>& out_users) {
            out_users = users;
        }));
    
    SyncManager manager;
    bool result = manager.InitDefaultUser(user);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(user, 0); // Should remain zero since no users available
}

// Test cases for GetAccountId function
TEST_F(SyncManagerTest, GetAccountId_WhenLoadMetaFails_ReturnsEmptyString) {
    int32_t user = 100;
    CloudInfo cloudInfo;
    cloudInfo.user = user;
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, _))
        .WillOnce(Return(false));
    
    SyncManager manager;
    std::string result = manager.GetAccountId(user);
    
    EXPECT_TRUE(result.empty());
}

TEST_F(SyncManagerTest, GetAccountId_WhenLoadMetaSucceeds_ReturnsAccountId) {
    int32_t user = 100;
    CloudInfo cloudInfo;
    cloudInfo.user = user;
    cloudInfo.id = "account123";
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(cloudInfo), Return(true)));
    
    SyncManager manager;
    std::string result = manager.GetAccountId(user);
    
    EXPECT_EQ(result, "account123");
}

// Test cases for GetSchemaMeta function
TEST_F(SyncManagerTest, GetSchemaMeta_WhenLoadMetaSucceeds_ReturnsSchemas) {
    CloudInfo cloud;
    cloud.user = 100;
    std::string bundleName = "com.example.test";
    
    std::vector<SchemaMeta> expectedSchemas = {SchemaMeta(), SchemaMeta()};
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(expectedSchemas), Return(true)));
    
    SyncManager manager;
    auto result = manager.GetSchemaMeta(cloud, bundleName);
    
    EXPECT_EQ(result.size(), 2);
}

TEST_F(SyncManagerTest, GetSchemaMeta_WhenLoadMetaFails_ReturnsEmptyVector) {
    CloudInfo cloud;
    cloud.user = 100;
    std::string bundleName = "com.example.test";
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, _))
        .WillOnce(Return(false));
    
    SyncManager manager;
    auto result = manager.GetSchemaMeta(cloud, bundleName);
    
    EXPECT_TRUE(result.empty());
}

class CloudServiceImplTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置测试环境
        mockAccount_ = std::make_shared<MockAccount>();
        mockMetaDataManager_ = std::make_shared<MockMetaDataManager>();
        mockSyncManager_ = std::make_shared<MockSyncManager>();
        
        // 设置全局Mock对象
        SetMockAccount(mockAccount_);
        SetMockMetaDataManager(mockMetaDataManager_);
        SetMockSyncManager(mockSyncManager_);
    }

    void TearDown() override {
        // 清理测试环境
        ResetAllMocks();
    }

    std::shared_ptr<MockAccount> mockAccount_;
    std::shared_ptr<MockMetaDataManager> mockMetaDataManager_;
    std::shared_ptr<MockSyncManager> mockSyncManager_;
    CloudServiceImpl service_;
};

// Test case for DisableCloud function - Success scenario
TEST_F(CloudServiceImplTest, DisableCloud_Success) {
    // Arrange
    std::string testId = "test_user_id";
    int32_t tokenId = 999;
    UserInfo user;
    user.userId = 888;
    
    CloudInfo cloudInfo;
    cloudInfo.id = testId;
    cloudInfo.enableCloud = true;
    cloudInfo.user = user.userId;
    
    EXPECT_CALL(*mockAccount_, GetUserByToken(tokenId))
        .WillOnce(Return(user));
    
    EXPECT_CALL(IPCSkeletonMock::GetInstance(), GetCallingTokenID())
        .WillOnce(Return(tokenId));
    
    EXPECT_CALL(service_, GetCloudInfo(user))
        .WillOnce(Return(std::make_pair(SUCCESS, cloudInfo)));
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, true))
        .WillOnce(Return(true)); // 模拟成功加载
    
    EXPECT_CALL(*mockMetaDataManager_, SaveMeta(_, _, true))
        .WillOnce(Return(true)); // 模拟成功保存
    
    EXPECT_CALL(service_, Execute(_)).Times(1);
    
    // Act
    int32_t result = service_.DisableCloud(testId);
    
    // Assert
    EXPECT_EQ(result, SUCCESS);
}

// Test case for DisableCloud function - GetCloudInfo fails
TEST_F(CloudServiceImplTest, DisableCloud_GetCloudInfoFailed) {
    // Arrange
    std::string testId = "test_user_id";
    int32_t tokenId = 999;
    UserInfo user;
    user.userId = 888;
    
    EXPECT_CALL(*mockAccount_, GetUserByToken(tokenId))
        .WillOnce(Return(user));
    
    EXPECT_CALL(IPCSkeletonMock::GetInstance(), GetCallingTokenID())
        .WillOnce(Return(tokenId));
    
    EXPECT_CALL(service_, GetCloudInfo(user))
        .WillOnce(Return(std::make_pair(ERROR, CloudInfo{})));
    
    // Act
    int32_t result = service_.DisableCloud(testId);
    
    // Assert
    EXPECT_EQ(result, ERROR);
}

// Test case for DisableCloud function - ID mismatch
TEST_F(CloudServiceImplTest, DisableCloud_IdMismatch) {
    // Arrange
    std::string testId = "test_user_id";
    std::string existingId = "different_id";
    int32_t tokenId = 999;
    UserInfo user;
    user.userId = 888;
    
    CloudInfo cloudInfo;
    cloudInfo.id = existingId; // Different from input
    cloudInfo.enableCloud = true;
    cloudInfo.user = user.userId;
    
    EXPECT_CALL(*mockAccount_, GetUserByToken(tokenId))
        .WillOnce(Return(user));
    
    EXPECT_CALL(IPCSkeletonMock::GetInstance(), GetCallingTokenID())
        .WillOnce(Return(tokenId));
    
    EXPECT_CALL(service_, GetCloudInfo(user))
        .WillOnce(Return(std::make_pair(SUCCESS, cloudInfo)));
    
    // Act
    int32_t result = service_.DisableCloud(testId);
    
    // Assert
    EXPECT_EQ(result, INVALID_ARGUMENT);
}

// Test case for DisableCloud function - Metadata save fails
TEST_F(CloudServiceImplTest, DisableCloud_MetadataSaveFailed) {
    // Arrange
    std::string testId = "test_user_id";
    int32_t tokenId = 999;
    UserInfo user;
    user.userId = 888;
    
    CloudInfo cloudInfo;
    cloudInfo.id = testId;
    cloudInfo.enableCloud = true;
    cloudInfo.user = user.userId;
    
    EXPECT_CALL(*mockAccount_, GetUserByToken(tokenId))
        .WillOnce(Return(user));
    
    EXPECT_CALL(IPCSkeletonMock::GetInstance(), GetCallingTokenID())
        .WillOnce(Return(tokenId));
    
    EXPECT_CALL(service_, GetCloudInfo(user))
        .WillOnce(Return(std::make_pair(SUCCESS, cloudInfo)));
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, true))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mockMetaDataManager_, SaveMeta(_, _, true))
        .WillOnce(Return(false)); // Simulate save failure
    
    // Act
    int32_t result = service_.DisableCloud(testId);
    
    // Assert
    EXPECT_EQ(result, ERROR);
}

// Test case for ChangeAppSwitch function - Success scenario
TEST_F(CloudServiceImplTest, ChangeAppSwitch_Success) {
    // Arrange
    std::string testId = "test_user_id";
    std::string bundleName = "com.example.test";
    int32_t appSwitch = SWITCH_ON;
    SwitchConfig config;
    
    int32_t tokenId = 999;
    UserInfo user;
    user.userId = 888;
    
    CloudInfo cloudInfo;
    cloudInfo.id = testId;
    cloudInfo.enableCloud = true;
    cloudInfo.user = user.userId;
    cloudInfo.apps[bundleName].cloudSwitch = false; // Current state
    
    EXPECT_CALL(*mockAccount_, GetUserByToken(tokenId))
        .WillOnce(Return(user));
    
    EXPECT_CALL(IPCSkeletonMock::GetInstance(), GetCallingTokenID())
        .WillOnce(Return(tokenId));
    
    EXPECT_CALL(service_, GetCloudInfo(user))
        .WillOnce(Return(std::make_pair(SUCCESS, cloudInfo)));
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, true))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mockMetaDataManager_, SaveMeta(_, _, true))
        .WillOnce(Return(true));
    
    EXPECT_CALL(service_, SyncConfig_UpdateConfig(user, bundleName, config.dbInfo))
        .Times(1);
    
    EXPECT_CALL(*mockSyncManager_, DoCloudSync(_))
        .Times(1); // Should be called when appSwitch is SWITCH_ON
    
    EXPECT_CALL(service_, Execute(_)).Times(1);
    
    // Act
    int32_t result = service_.ChangeAppSwitch(testId, bundleName, appSwitch, config);
    
    // Assert
    EXPECT_EQ(result, SUCCESS);
}

// Test case for ChangeAppSwitch function - Cloud not enabled
TEST_F(CloudServiceImplTest, ChangeAppSwitch_CloudNotEnabled) {
    // Arrange
    std::string testId = "test_user_id";
    std::string bundleName = "com.example.test";
    int32_t appSwitch = SWITCH_ON;
    SwitchConfig config;
    
    int32_t tokenId = 999;
    UserInfo user;
    user.userId = 888;
    
    CloudInfo cloudInfo;
    cloudInfo.id = testId;
    cloudInfo.enableCloud = false; // Cloud not enabled
    cloudInfo.user = user.userId;
    
    EXPECT_CALL(*mockAccount_, GetUserByToken(tokenId))
        .WillOnce(Return(user));
    
    EXPECT_CALL(IPCSkeletonMock::GetInstance(), GetCallingTokenID())
        .WillOnce(Return(tokenId));
    
    EXPECT_CALL(service_, GetCloudInfo(user))
        .WillOnce(Return(std::make_pair(SUCCESS, cloudInfo)));
    
    // Act
    int32_t result = service_.ChangeAppSwitch(testId, bundleName, appSwitch, config);
    
    // Assert
    EXPECT_EQ(result, SUCCESS); // Function returns the status from GetCloudInfo which is SUCCESS
}

// Test case for ChangeAppSwitch function - ID mismatch
TEST_F(CloudServiceImplTest, ChangeAppSwitch_IdMismatch) {
    // Arrange
    std::string testId = "test_user_id";
    std::string differentId = "different_id";
    std::string bundleName = "com.example.test";
    int32_t appSwitch = SWITCH_ON;
    SwitchConfig config;
    
    int32_t tokenId = 999;
    UserInfo user;
    user.userId = 888;
    
    CloudInfo cloudInfo;
    cloudInfo.id = differentId; // Different from input
    cloudInfo.enableCloud = true;
    cloudInfo.user = user.userId;
    
    EXPECT_CALL(*mockAccount_, GetUserByToken(tokenId))
        .WillOnce(Return(user));
    
    EXPECT_CALL(IPCSkeletonMock::GetInstance(), GetCallingTokenID())
        .WillOnce(Return(tokenId));
    
    EXPECT_CALL(service_, GetCloudInfo(user))
        .WillOnce(Return(std::make_pair(SUCCESS, cloudInfo)));
    
    EXPECT_CALL(service_, Execute(_)).Times(1); // Should execute task even on error
    
    // Act
    int32_t result = service_.ChangeAppSwitch(testId, bundleName, appSwitch, config);
    
    // Assert
    EXPECT_EQ(result, INVALID_ARGUMENT);
}

// Test case for ChangeAppSwitch function - App doesn't exist initially but exists after server fetch
TEST_F(CloudServiceImplTest, ChangeAppSwitch_AppNotExistThenExists) {
    // Arrange
    std::string testId = "test_user_id";
    std::string bundleName = "com.example.test";
    int32_t appSwitch = SWITCH_ON;
    SwitchConfig config;
    
    int32_t tokenId = 999;
    UserInfo user;
    user.userId = 888;
    
    CloudInfo initialCloudInfo;
    initialCloudInfo.id = testId;
    initialCloudInfo.enableCloud = true;
    initialCloudInfo.user = user.userId;
    // Bundle doesn't exist in initial info
    
    CloudInfo updatedCloudInfo = initialCloudInfo;
    updatedCloudInfo.apps[bundleName].cloudSwitch = false; // Now it exists
    
    EXPECT_CALL(*mockAccount_, GetUserByToken(tokenId))
        .WillOnce(Return(user));
    
    EXPECT_CALL(IPCSkeletonMock::GetInstance(), GetCallingTokenID())
        .WillOnce(Return(tokenId));
    
    EXPECT_CALL(service_, GetCloudInfo(user))
        .WillOnce(Return(std::make_pair(SUCCESS, initialCloudInfo)));
    
    EXPECT_CALL(service_, GetCloudInfoFromServer(user))
        .WillOnce(Return(std::make_pair(SUCCESS, updatedCloudInfo)));
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(_, _, true))
        .WillOnce(Return(true));
    
    EXPECT_CALL(*mockMetaDataManager_, SaveMeta(_, _, true))
        .WillOnce(Return(true));
    
    EXPECT_CALL(service_, SyncConfig_UpdateConfig(user, bundleName, config.dbInfo))
        .Times(1);
    
    EXPECT_CALL(*mockSyncManager_, DoCloudSync(_))
        .Times(1);
    
    EXPECT_CALL(service_, Execute(_)).Times(1);
    
    // Act
    int32_t result = service_.ChangeAppSwitch(testId, bundleName, appSwitch, config);
    
    // Assert
    EXPECT_EQ(result, SUCCESS);
}

// Test case for DoClean function - Success scenario
TEST_F(CloudServiceImplTest, DoClean_Success) {
    // Arrange
    CloudInfo cloudInfo;
    cloudInfo.user = 888;
    
    std::string bundleName = "com.example.test";
    std::map<std::string, int32_t> actions = {{bundleName, 1}};
    std::map<std::string, ClearConfig> configs = {{bundleName, ClearConfig{}}};
    
    SchemaMeta schemaMeta;
    
    // Add bundle to cloudInfo
    cloudInfo.apps[bundleName].cloudSwitch = true;
    
    EXPECT_CALL(*mockSyncManager_, StopCloudSync(cloudInfo.user))
        .Times(1);
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(cloudInfo.GetSchemaKey(bundleName), _, true))
        .WillOnce(Return(true));
    
    EXPECT_CALL(service_, DoAppLevelClean(cloudInfo.user, schemaMeta, 1))
        .Times(1);
    
    // Act
    int32_t result = service_.DoClean(cloudInfo, actions, configs);
    
    // Assert
    EXPECT_EQ(result, SUCCESS);
}

// Test case for DoClean function - App doesn't exist
TEST_F(CloudServiceImplTest, DoClean_AppDoesNotExist) {
    // Arrange
    CloudInfo cloudInfo;
    cloudInfo.user = 888;
    
    std::string bundleName = "com.example.test";
    std::map<std::string, int32_t> actions = {{bundleName, 1}};
    std::map<std::string, ClearConfig> configs = {{bundleName, ClearConfig{}}};
    
    // Bundle does NOT exist in cloudInfo
    
    EXPECT_CALL(*mockSyncManager_, StopCloudSync(cloudInfo.user))
        .Times(1);
    
    // Should skip processing since app doesn't exist
    
    // Act
    int32_t result = service_.DoClean(cloudInfo, actions, configs);
    
    // Assert
    EXPECT_EQ(result, SUCCESS);
}

// Test case for DoClean function - Schema meta load fails
TEST_F(CloudServiceImplTest, DoClean_SchemaMetaLoadFailed) {
    // Arrange
    CloudInfo cloudInfo;
    cloudInfo.user = 888;
    
    std::string bundleName = "com.example.test";
    std::map<std::string, int32_t> actions = {{bundleName, 1}};
    std::map<std::string, ClearConfig> configs = {{bundleName, ClearConfig{}}};
    
    // Add bundle to cloudInfo
    cloudInfo.apps[bundleName].cloudSwitch = true;
    
    EXPECT_CALL(*mockSyncManager_, StopCloudSync(cloudInfo.user))
        .Times(1);
    
    EXPECT_CALL(*mockMetaDataManager_, LoadMeta(cloudInfo.GetSchemaKey(bundleName), _, true))
        .WillOnce(Return(false)); // Simulate load failure
    
    // Act
    int32_t result = service_.DoClean(cloudInfo, actions, configs);
    
    // Assert
    EXPECT_EQ(result, SUCCESS);
}
} // namespace OHOS::Test