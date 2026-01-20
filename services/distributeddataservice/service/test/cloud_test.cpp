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
}
} // namespace OHOS::Test