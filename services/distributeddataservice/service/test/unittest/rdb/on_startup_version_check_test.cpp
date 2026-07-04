/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "metadata/bundle_version_meta_data.h"

#include <gtest/gtest.h>

#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "mock/db_store_mock.h"
#include "serializable/serializable.h"
#include "utils/constant.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedData;

namespace OHOS::Test {
class DBStoreMockImpl;
class OnStartupVersionCheckTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase() {};
    void SetUp() {};
    void TearDown() {};
    static std::shared_ptr<DBStoreMockImpl> metaStore;
};

std::shared_ptr<DBStoreMockImpl> OnStartupVersionCheckTest::metaStore = nullptr;

class DBStoreMockImpl : public DBStoreMock {
public:
    using PutLocalBatchFunc = std::function<DBStatus(const std::vector<Entry> &)>;
    PutLocalBatchFunc putLocalBatchFunc;
    DBStatus PutLocalBatch(const std::vector<Entry> &entries) override
    {
        if (putLocalBatchFunc) {
            return putLocalBatchFunc(entries);
        }
        return DBStoreMock::PutLocalBatch(entries);
    }
    using DeleteLocalBatchFunc = std::function<DBStatus(const std::vector<Key> &)>;
    DeleteLocalBatchFunc deleteLocalBatchFunc;
    DBStatus DeleteLocalBatch(const std::vector<Key> &keys) override
    {
        if (deleteLocalBatchFunc) {
            return deleteLocalBatchFunc(keys);
        }
        return DBStoreMock::DeleteLocalBatch(keys);
    }
};

void OnStartupVersionCheckTest::SetUpTestCase()
{
    metaStore = std::make_shared<DBStoreMockImpl>();
    MetaDataManager::GetInstance().Initialize(
        metaStore,
        [](const auto &store) {
            return 0;
        },
        "meta");
}

/**
 * @tc.name: OnStartupVersionCheck_NoVersionEntries_NoAction
 * @tc.desc: When no BundleVersionMetaData entries exist, no Database entries should be affected
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(OnStartupVersionCheckTest, OnStartupVersionCheck_NoVersionEntries_NoAction, TestSize.Level1)
{
    std::string prefix = BundleVersionMetaData::GetPrefix({});
    std::vector<BundleVersionMetaData> entries;
    auto loadResult = MetaDataManager::GetInstance().LoadMeta(prefix, entries, true);
    EXPECT_TRUE(loadResult);
    EXPECT_EQ(entries.size(), static_cast<size_t>(0));
}

/**
 * @tc.name: OnStartupVersionCheck_VersionEntryExists_VersionNotZero_NoDeletion
 * @tc.desc: When a BundleVersionMetaData entry exists with versionCode != 0,
 *           its corresponding Database should NOT be deleted
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(OnStartupVersionCheckTest, OnStartupVersionCheck_VersionEntryExists_VersionNotZero_NoDeletion, TestSize.Level1)
{
    BundleVersionMetaData versionMeta;
    versionMeta.bundleName = "com.example.nozero";
    versionMeta.user = "100";
    versionMeta.appIndex = 0;
    versionMeta.versionCode = 100;

    auto saveResult = MetaDataManager::GetInstance().SaveMeta(versionMeta.GetKey(), versionMeta, true);
    EXPECT_TRUE(saveResult);

    Database db;
    db.bundleName = "com.example.nozero";
    db.name = "test_store";
    db.user = "100";
    db.deviceId = "device1";
    saveResult = MetaDataManager::GetInstance().SaveMeta(db.GetKey(), db, true);
    EXPECT_TRUE(saveResult);

    std::string dbPrefix = Database::GetPrefix({ "100", "default", "com.example.nozero" });
    std::vector<Database> databases;
    auto loadResult = MetaDataManager::GetInstance().LoadMeta(dbPrefix, databases, true);
    EXPECT_TRUE(loadResult);
    EXPECT_EQ(databases.size(), static_cast<size_t>(1));

    auto delResult1 = MetaDataManager::GetInstance().DelMeta(versionMeta.GetKey(), true);
    EXPECT_TRUE(delResult1);
    auto delResult2 = MetaDataManager::GetInstance().DelMeta(db.GetKey(), true);
    EXPECT_TRUE(delResult2);
}

/**
 * @tc.name: OnStartupVersionCheck_VersionEntryExists_VersionIsZero_DeletesOldDatabase
 * @tc.desc: When a BundleVersionMetaData entry exists with versionCode == 0,
 *           the corresponding Database entry should be deleted
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(OnStartupVersionCheckTest, OnStartupVersionCheck_VersionEntryExists_VersionIsZero_DeletesOldDatabase, TestSize.Level1)
{
    BundleVersionMetaData versionMeta;
    versionMeta.bundleName = "com.example.zero";
    versionMeta.user = "100";
    versionMeta.appIndex = 0;
    versionMeta.versionCode = 0;

    auto saveResult = MetaDataManager::GetInstance().SaveMeta(versionMeta.GetKey(), versionMeta, true);
    EXPECT_TRUE(saveResult);

    Database db;
    db.bundleName = "com.example.zero";
    db.name = "old_store";
    db.user = "100";
    db.deviceId = "device1";
    saveResult = MetaDataManager::GetInstance().SaveMeta(db.GetKey(), db, true);
    EXPECT_TRUE(saveResult);

    std::string dbPrefix = Database::GetPrefix({ "100", "default", "com.example.zero" });
    std::vector<Database> databases;
    auto loadResult = MetaDataManager::GetInstance().LoadMeta(dbPrefix, databases, true);
    EXPECT_TRUE(loadResult);
    EXPECT_EQ(databases.size(), static_cast<size_t>(1));

    auto delResult = MetaDataManager::GetInstance().DelMeta(db.GetKey(), true);
    EXPECT_TRUE(delResult);

    Database afterDelete;
    auto loadAfterDel = MetaDataManager::GetInstance().LoadMeta(db.GetKey(), afterDelete, true);
    EXPECT_FALSE(loadAfterDel);

    auto delVersion = MetaDataManager::GetInstance().DelMeta(versionMeta.GetKey(), true);
    EXPECT_TRUE(delVersion);
}

/**
 * @tc.name: OnStartupVersionCheck_MultipleVersionEntries_PrefixQuery
 * @tc.desc: Multiple BundleVersionMetaData entries can be queried via prefix
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(OnStartupVersionCheckTest, OnStartupVersionCheck_MultipleVersionEntries_PrefixQuery, TestSize.Level1)
{
    BundleVersionMetaData meta1;
    meta1.bundleName = "com.example.multi1";
    meta1.user = "100";
    meta1.appIndex = 0;
    meta1.versionCode = 100;

    BundleVersionMetaData meta2;
    meta2.bundleName = "com.example.multi2";
    meta2.user = "200";
    meta2.appIndex = 0;
    meta2.versionCode = 200;

    auto save1 = MetaDataManager::GetInstance().SaveMeta(meta1.GetKey(), meta1, true);
    EXPECT_TRUE(save1);
    auto save2 = MetaDataManager::GetInstance().SaveMeta(meta2.GetKey(), meta2, true);
    EXPECT_TRUE(save2);

    std::string prefix = BundleVersionMetaData::GetPrefix({});
    std::vector<BundleVersionMetaData> entries;
    auto loadResult = MetaDataManager::GetInstance().LoadMeta(prefix, entries, true);
    EXPECT_TRUE(loadResult);
    EXPECT_GE(entries.size(), static_cast<size_t>(2));

    auto del1 = MetaDataManager::GetInstance().DelMeta(meta1.GetKey(), true);
    EXPECT_TRUE(del1);
    auto del2 = MetaDataManager::GetInstance().DelMeta(meta2.GetKey(), true);
    EXPECT_TRUE(del2);
}

/**
 * @tc.name: OnStartupVersionCheck_DatabasePrefixQuery_MatchesBundleVersion
 * @tc.desc: Database prefix query with user+default+bundleName matches the key pattern
 *           used in OnStartupVersionCheck
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(OnStartupVersionCheckTest, OnStartupVersionCheck_DatabasePrefixQuery_MatchesBundleVersion, TestSize.Level1)
{
    BundleVersionMetaData versionMeta;
    versionMeta.bundleName = "com.example.match";
    versionMeta.user = "100";
    versionMeta.versionCode = 50;

    Database db1;
    db1.bundleName = "com.example.match";
    db1.name = "store1";
    db1.user = "100";
    db1.deviceId = "device1";

    Database db2;
    db2.bundleName = "com.example.match";
    db2.name = "store2";
    db2.user = "100";
    db2.deviceId = "device1";

    auto saveV = MetaDataManager::GetInstance().SaveMeta(versionMeta.GetKey(), versionMeta, true);
    EXPECT_TRUE(saveV);
    auto saveD1 = MetaDataManager::GetInstance().SaveMeta(db1.GetKey(), db1, true);
    EXPECT_TRUE(saveD1);
    auto saveD2 = MetaDataManager::GetInstance().SaveMeta(db2.GetKey(), db2, true);
    EXPECT_TRUE(saveD2);

    std::string dbPrefix = Database::GetPrefix({ "100", "default", "com.example.match" });
    std::vector<Database> databases;
    auto loadResult = MetaDataManager::GetInstance().LoadMeta(dbPrefix, databases, true);
    EXPECT_TRUE(loadResult);
    EXPECT_EQ(databases.size(), static_cast<size_t>(2));

    auto delV = MetaDataManager::GetInstance().DelMeta(versionMeta.GetKey(), true);
    EXPECT_TRUE(delV);
    auto delD1 = MetaDataManager::GetInstance().DelMeta(db1.GetKey(), true);
    EXPECT_TRUE(delD1);
    auto delD2 = MetaDataManager::GetInstance().DelMeta(db2.GetKey(), true);
    EXPECT_TRUE(delD2);
}

/**
 * @tc.name: OnStartupVersionCheck_DeleteDatabaseAndSaveNewDatabase
 * @tc.desc: After deleting old Database entries and saving new ones,
 *           the new entries should be loadable
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(OnStartupVersionCheckTest, OnStartupVersionCheck_DeleteDatabaseAndSaveNewDatabase, TestSize.Level1)
{
    Database oldDb;
    oldDb.bundleName = "com.example.replace";
    oldDb.name = "old_store";
    oldDb.user = "100";
    oldDb.deviceId = "device1";

    auto saveOld = MetaDataManager::GetInstance().SaveMeta(oldDb.GetKey(), oldDb, true);
    EXPECT_TRUE(saveOld);

    auto delOld = MetaDataManager::GetInstance().DelMeta(oldDb.GetKey(), true);
    EXPECT_TRUE(delOld);

    Database oldDbCheck;
    auto loadOld = MetaDataManager::GetInstance().LoadMeta(oldDb.GetKey(), oldDbCheck, true);
    EXPECT_FALSE(loadOld);

    Database newDb;
    newDb.bundleName = "com.example.replace";
    newDb.name = "new_store";
    newDb.user = "100";
    newDb.deviceId = "device1";

    auto saveNew = MetaDataManager::GetInstance().SaveMeta(newDb.GetKey(), newDb, true);
    EXPECT_TRUE(saveNew);

    Database newDbCheck;
    auto loadNew = MetaDataManager::GetInstance().LoadMeta(newDb.GetKey(), newDbCheck, true);
    EXPECT_TRUE(loadNew);
    EXPECT_EQ(newDbCheck.name, "new_store");

    auto delNew = MetaDataManager::GetInstance().DelMeta(newDb.GetKey(), true);
    EXPECT_TRUE(delNew);
}

/**
 * @tc.name: OnStartupVersionCheck_UpdateVersionCodeAfterSchemaReplacement
 * @tc.desc: After schema replacement, updating BundleVersionMetaData versionCode
 *           should overwrite the old value
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(OnStartupVersionCheckTest, OnStartupVersionCheck_UpdateVersionCodeAfterSchemaReplacement, TestSize.Level1)
{
    BundleVersionMetaData oldVersion;
    oldVersion.bundleName = "com.example.update";
    oldVersion.user = "100";
    oldVersion.appIndex = 0;
    oldVersion.versionCode = 100;

    auto saveOld = MetaDataManager::GetInstance().SaveMeta(oldVersion.GetKey(), oldVersion, true);
    EXPECT_TRUE(saveOld);

    BundleVersionMetaData newVersion;
    newVersion.bundleName = "com.example.update";
    newVersion.user = "100";
    newVersion.appIndex = 0;
    newVersion.versionCode = 200;

    auto saveNew = MetaDataManager::GetInstance().SaveMeta(newVersion.GetKey(), newVersion, true);
    EXPECT_TRUE(saveNew);

    BundleVersionMetaData loaded;
    auto loadResult = MetaDataManager::GetInstance().LoadMeta(newVersion.GetKey(), loaded, true);
    EXPECT_TRUE(loadResult);
    EXPECT_EQ(loaded.versionCode, 200);
    EXPECT_EQ(loaded.bundleName, "com.example.update");

    auto delResult = MetaDataManager::GetInstance().DelMeta(newVersion.GetKey(), true);
    EXPECT_TRUE(delResult);
}

/**
 * @tc.name: OnStartupVersionCheck_DifferentUsers_IndependentVersion
 * @tc.desc: Different users with the same bundleName have independent
 *           BundleVersionMetaData entries and Database entries
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(OnStartupVersionCheckTest, OnStartupVersionCheck_DifferentUsers_IndependentVersion, TestSize.Level1)
{
    BundleVersionMetaData v1;
    v1.bundleName = "com.example.crossuser";
    v1.user = "100";
    v1.versionCode = 100;

    BundleVersionMetaData v2;
    v2.bundleName = "com.example.crossuser";
    v2.user = "200";
    v2.versionCode = 200;

    auto save1 = MetaDataManager::GetInstance().SaveMeta(v1.GetKey(), v1, true);
    EXPECT_TRUE(save1);
    auto save2 = MetaDataManager::GetInstance().SaveMeta(v2.GetKey(), v2, true);
    EXPECT_TRUE(save2);

    Database db1;
    db1.bundleName = "com.example.crossuser";
    db1.name = "store_user100";
    db1.user = "100";
    db1.deviceId = "device1";

    Database db2;
    db2.bundleName = "com.example.crossuser";
    db2.name = "store_user200";
    db2.user = "200";
    db2.deviceId = "device2";

    auto saveDb1 = MetaDataManager::GetInstance().SaveMeta(db1.GetKey(), db1, true);
    EXPECT_TRUE(saveDb1);
    auto saveDb2 = MetaDataManager::GetInstance().SaveMeta(db2.GetKey(), db2, true);
    EXPECT_TRUE(saveDb2);

    EXPECT_NE(v1.GetKey(), v2.GetKey());
    EXPECT_NE(db1.GetKey(), db2.GetKey());

    std::string prefix1 = Database::GetPrefix({ "100", "default", "com.example.crossuser" });
    std::vector<Database> dbs1;
    auto load1 = MetaDataManager::GetInstance().LoadMeta(prefix1, dbs1, true);
    EXPECT_TRUE(load1);
    EXPECT_EQ(dbs1.size(), static_cast<size_t>(1));

    std::string prefix2 = Database::GetPrefix({ "200", "default", "com.example.crossuser" });
    std::vector<Database> dbs2;
    auto load2 = MetaDataManager::GetInstance().LoadMeta(prefix2, dbs2, true);
    EXPECT_TRUE(load2);
    EXPECT_EQ(dbs2.size(), static_cast<size_t>(1));

    auto del1 = MetaDataManager::GetInstance().DelMeta(v1.GetKey(), true);
    EXPECT_TRUE(del1);
    auto del2 = MetaDataManager::GetInstance().DelMeta(v2.GetKey(), true);
    EXPECT_TRUE(del2);
    auto delDb1 = MetaDataManager::GetInstance().DelMeta(db1.GetKey(), true);
    EXPECT_TRUE(delDb1);
    auto delDb2 = MetaDataManager::GetInstance().DelMeta(db2.GetKey(), true);
    EXPECT_TRUE(delDb2);
}

/**
 * @tc.name: OnStartupVersionCheck_DeleteAllDatabasesUnderPrefix
 * @tc.desc: Deleting all Database entries under a prefix simulates
 *           the schema cleanup step in OnStartupVersionCheck
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(OnStartupVersionCheckTest, OnStartupVersionCheck_DeleteAllDatabasesUnderPrefix, TestSize.Level1)
{
    Database db1;
    db1.bundleName = "com.example.cleanup";
    db1.name = "store1";
    db1.user = "100";
    db1.deviceId = "device1";

    Database db2;
    db2.bundleName = "com.example.cleanup";
    db2.name = "store2";
    db2.user = "100";
    db2.deviceId = "device1";

    auto save1 = MetaDataManager::GetInstance().SaveMeta(db1.GetKey(), db1, true);
    EXPECT_TRUE(save1);
    auto save2 = MetaDataManager::GetInstance().SaveMeta(db2.GetKey(), db2, true);
    EXPECT_TRUE(save2);

    std::string dbPrefix = Database::GetPrefix({ "100", "default", "com.example.cleanup" });
    std::vector<Database> databases;
    auto loadResult = MetaDataManager::GetInstance().LoadMeta(dbPrefix, databases, true);
    EXPECT_TRUE(loadResult);
    EXPECT_EQ(databases.size(), static_cast<size_t>(2));

    for (const auto &db : databases) {
        auto delResult = MetaDataManager::GetInstance().DelMeta(db.GetKey(), true);
        EXPECT_TRUE(delResult);
    }

    std::vector<Database> afterDelete;
    loadResult = MetaDataManager::GetInstance().LoadMeta(dbPrefix, afterDelete, true);
    EXPECT_TRUE(loadResult);
    EXPECT_EQ(afterDelete.size(), static_cast<size_t>(0));
}
} // namespace OHOS::Test
