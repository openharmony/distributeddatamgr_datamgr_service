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
#ifndef OMIT_ENCRYPT
#ifdef RELATIONAL_STORE
#include <gtest/gtest.h>

#include "data_transformer.h"
#include "db_common.h"
#include "db_constant.h"
#include "db_errno.h"
#include "db_types.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "generic_single_ver_kv_entry.h"
#include "log_print.h"
#include "relational_store_delegate.h"
#include "relational_store_instance.h"
#include "relational_store_manager.h"
#include "relational_store_sqlite_ext.h"
#include "relational_sync_able_storage.h"
#include "sqlite_relational_store.h"
#include "sqlite_utils.h"
#include "virtual_communicator_aggregator.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
string g_testDir;
string g_storePath;
string g_storeID = "dftStoreID";
string g_tableName = "data";
const string CORRECT_KEY = "a correct key";
CipherPassword g_correctPasswd;
const string REKEY_KEY = "a key after rekey";
CipherPassword g_rekeyPasswd;
const string INCORRECT_KEY = "a incorrect key";
CipherPassword g_incorrectPasswd;
const int DEFAULT_ITER = 5000;
const int CUSTOMIZED_ITER = 10000;

DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
RelationalStoreDelegate *g_delegate = nullptr;
IRelationalStore *g_store = nullptr;

void InitStoreProp(const std::string &storePath, const std::string &appId, const std::string &userId,
    RelationalDBProperties &properties)
{
    properties.SetStringProp(RelationalDBProperties::DATA_DIR, storePath);
    properties.SetStringProp(RelationalDBProperties::APP_ID, appId);
    properties.SetStringProp(RelationalDBProperties::USER_ID, userId);
    properties.SetStringProp(RelationalDBProperties::STORE_ID, g_storeID);
    std::string identifier = userId + "-" + appId + "-" + g_storeID;
    std::string hashIdentifier = DBCommon::TransferHashString(identifier);
    properties.SetStringProp(RelationalDBProperties::IDENTIFIER_DATA, hashIdentifier);
}

const RelationalSyncAbleStorage *GetRelationalStore()
{
    RelationalDBProperties properties;
    InitStoreProp(g_storePath, APP_ID, USER_ID, properties);
    int errCode = E_OK;
    g_store = RelationalStoreInstance::GetDataBase(properties, errCode);
    if (g_store == nullptr) {
        LOGE("Get db failed:%d", errCode);
        return nullptr;
    }
    return static_cast<SQLiteRelationalStore *>(g_store)->GetStorageEngine();
}

void ExecSqlAndAssertOK(sqlite3 *db, const std::string &sql)
{
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
}
}

class DistributedDBRelationalEncryptedDbTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBRelationalEncryptedDbTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_storePath = g_testDir + "/getDataTest.db";
    LOGI("The test db is:%s", g_testDir.c_str());

    auto communicator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(communicator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(communicator);

    g_correctPasswd.SetValue((const uint8_t *)(CORRECT_KEY.data()), CORRECT_KEY.size());
    g_rekeyPasswd.SetValue((const uint8_t *)(REKEY_KEY.data()), REKEY_KEY.size());
    g_incorrectPasswd.SetValue((const uint8_t *)(INCORRECT_KEY.data()), INCORRECT_KEY.size());
}

void DistributedDBRelationalEncryptedDbTest::TearDownTestCase(void)
{
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void DistributedDBRelationalEncryptedDbTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

void DistributedDBRelationalEncryptedDbTest::TearDown(void)
{
    if (g_delegate != nullptr) {
        EXPECT_EQ(g_mgr.CloseStore(g_delegate), DBStatus::OK);
        g_delegate = nullptr;
    }
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
    return;
}

/**
 * @tc.name: OpenEncryptedDBWithoutPasswd_001
 * @tc.desc: Open encrypted db without password in collaboration mode.
 * @tc.type: FUNC
 * @tc.require: AR000H68LL
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalEncryptedDbTest, OpenEncryptedDBWithoutPasswdInCollMode_001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create an encrypted db.
     * @tc.expected: Succeed.
     */
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    string sql =
        "PRAGMA key='" + CORRECT_KEY + "';"
        "PRAGMA codec_kdf_iter=" + std::to_string(DEFAULT_ITER) + ";"
        "PRAGMA journal_mode=WAL;"
        "CREATE TABLE " + g_tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    ExecSqlAndAssertOK(db, sql);

    /**
     * @tc.steps: step2. Open store.
     * @tc.expected: Failed, return INVALID_PASSWD_OR_CORRUPTED_DB.
     */
    RelationalStoreDelegate::Option option { nullptr };
    EXPECT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, option, g_delegate), DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB);
    EXPECT_EQ(g_delegate, nullptr);
    sqlite3_close(db);
}

/**
 * @tc.name: OpenEncryptedDBWithoutPasswdInSplitMode_001
 * @tc.desc: Open encrypted db without password in split mode.
 * @tc.type: FUNC
 * @tc.require: AR000H68LL
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalEncryptedDbTest, OpenEncryptedDBWithoutPasswdInSplitMode_001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create an encrypted db.
     * @tc.expected: Succeed.
     */
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    string sql =
        "PRAGMA key='" + CORRECT_KEY + "';"
        "PRAGMA codec_kdf_iter=" + std::to_string(DEFAULT_ITER) + ";"
        "PRAGMA journal_mode=WAL;"
        "CREATE TABLE " + g_tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    ExecSqlAndAssertOK(db, sql);

    /**
     * @tc.steps: step2. Open store.
     * @tc.expected: Failed, return INVALID_PASSWD_OR_CORRUPTED_DB.
     */
    RelationalStoreDelegate::Option option { nullptr };
    EXPECT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, option, g_delegate), DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB);
    EXPECT_EQ(g_delegate, nullptr);
    sqlite3_close(db);
}

/**
 * @tc.name: OpenEncryptedDBWithPasswdInSplitMode_001
 * @tc.desc: Open encrypted db with password in split mode.
 * @tc.type: FUNC
 * @tc.require: AR000H68LL
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalEncryptedDbTest, OpenEncryptedDBWithPasswdInSplitMode_001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create an encrypted db.
     * @tc.expected: Succeed.
     */
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    string sql =
        "PRAGMA key='" + CORRECT_KEY + "';"
        "PRAGMA codec_kdf_iter=" + std::to_string(DEFAULT_ITER) + ";"
        "PRAGMA journal_mode=WAL;"
        "CREATE TABLE " + g_tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    ExecSqlAndAssertOK(db, sql);

    /**
     * @tc.steps: step2. Open store.
     * @tc.expected: Succeed, return OK.
     */
    RelationalStoreDelegate::Option option {
        nullptr, false, true, CipherType::DEFAULT, g_correctPasswd, DEFAULT_ITER };
    EXPECT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, option, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step3. Insert several data.
     * @tc.expected: Succeed, return OK.
     */
    ExecSqlAndAssertOK(db, "INSERT OR REPLACE INTO " + g_tableName + " VALUES(1, 1);");
    ExecSqlAndAssertOK(db, "INSERT OR REPLACE INTO " + g_tableName + " VALUES(2, 2);");
    ExecSqlAndAssertOK(db, "INSERT OR REPLACE INTO " + g_tableName + " VALUES(3, 3);");

    /**
     * @tc.steps: step4. Get sync data.
     * @tc.expected: Succeed, get 3 data.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    ContinueToken token = nullptr;
    QueryObject query(Query::Select(g_tableName));
    std::vector<SingleVerKvEntry *> entries;
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 3u);

    sqlite3_close(db);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: OpenEncryptedDBWithInvalidParameters_001
 * @tc.desc: Open encrypted db with invalid parameters in split mode.
 * @tc.type: FUNC
 * @tc.require: AR000H68LL
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalEncryptedDbTest, OpenEncryptedDBWithInvalidParameters_001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create an encrypted db.
     * @tc.expected: Succeed.
     */
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    string sql =
        "PRAGMA key='" + CORRECT_KEY + "';"
        "PRAGMA codec_kdf_iter=" + std::to_string(DEFAULT_ITER) + ";"
        "PRAGMA journal_mode=WAL;"
        "CREATE TABLE " + g_tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    ExecSqlAndAssertOK(db, sql);

    /**
     * @tc.steps: step2. Open store with invalid password.
     * @tc.expected: Failed, return INVALID_ARGS.
     */
    RelationalStoreDelegate::Option option1 {
        nullptr, false, true, CipherType::DEFAULT, CipherPassword {}, DEFAULT_ITER };
    EXPECT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, option1, g_delegate), DBStatus::INVALID_ARGS);
    ASSERT_EQ(g_delegate, nullptr);

    /**
     * @tc.steps: step3. Open store with invalid password.
     * @tc.expected: Failed, return INVALID_ARGS.
     */
    RelationalStoreDelegate::Option option2 {
        nullptr, false, true, CipherType::DEFAULT, g_correctPasswd, 0u };
    EXPECT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, option2, g_delegate), DBStatus::INVALID_ARGS);
    ASSERT_EQ(g_delegate, nullptr);
    sqlite3_close(db);
}

/**
 * @tc.name: OpenEncryptedDBWithCustomizedIterTimes_001
 * @tc.desc: Open encrypted db with customized iterate times.
 * @tc.type: FUNC
 * @tc.require: AR000H68LL
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalEncryptedDbTest, OpenEncryptedDBWithCustomizedIterTimes_001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create an encrypted db.
     * @tc.expected: Succeed.
     */
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    string sql =
        "PRAGMA key='" + CORRECT_KEY + "';"
        "PRAGMA codec_kdf_iter=" + std::to_string(CUSTOMIZED_ITER) + ";"
        "PRAGMA journal_mode=WAL;"
        "CREATE TABLE " + g_tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    ExecSqlAndAssertOK(db, sql);

    /**
     * @tc.steps: step2. Open store.
     * @tc.expected: Succeed, return OK.
     */
    RelationalStoreDelegate::Option option1 {
        nullptr, false, true, CipherType::DEFAULT, g_correctPasswd, CUSTOMIZED_ITER };
    EXPECT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, option1, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step3. Insert several data.
     * @tc.expected: Succeed, return OK.
     */
    ExecSqlAndAssertOK(db, "INSERT OR REPLACE INTO " + g_tableName + " VALUES(1, 1);");
    ExecSqlAndAssertOK(db, "INSERT OR REPLACE INTO " + g_tableName + " VALUES(2, 2);");
    ExecSqlAndAssertOK(db, "INSERT OR REPLACE INTO " + g_tableName + " VALUES(3, 3);");

    /**
     * @tc.steps: step4. Get sync data.
     * @tc.expected: Succeed, get 3 data.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    ContinueToken token = nullptr;
    QueryObject query(Query::Select(g_tableName));
    std::vector<SingleVerKvEntry *> entries;
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);
    EXPECT_EQ(entries.size(), 3u);

    sqlite3_close(db);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: RekeyAfterOpenStore_001
 * @tc.desc: Rekey after open store.
 * @tc.type: FUNC
 * @tc.require: AR000H68LL
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalEncryptedDbTest, RekeyAfterOpenStore_001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create an encrypted db.
     * @tc.expected: Succeed.
     */
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    string sql =
        "PRAGMA key='" + CORRECT_KEY + "';"
        "PRAGMA codec_kdf_iter=" + std::to_string(CUSTOMIZED_ITER) + ";"
        "PRAGMA journal_mode=WAL;"
        "CREATE TABLE " + g_tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    ExecSqlAndAssertOK(db, sql);

    /**
     * @tc.steps: step2. Open store.
     * @tc.expected: Succeed, return OK.
     */
    RelationalStoreDelegate::Option option1 {
        nullptr, false, true, CipherType::DEFAULT, g_correctPasswd, CUSTOMIZED_ITER };
    EXPECT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, option1, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    std::thread t1([db] {
        std::string sql = "PARGMA rekey=" + REKEY_KEY;
        EXPECT_EQ(sqlite3_rekey(db, REKEY_KEY.data(), REKEY_KEY.size()), SQLITE_OK);
    });
    t1.join();

    /**
     * @tc.steps: step3. Get sync data.
     * @tc.expected: Failed.
     */
    auto store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    ContinueToken token = nullptr;
    QueryObject query(Query::Select(g_tableName));
    std::vector<SingleVerKvEntry *> entries;
    EXPECT_NE(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);

    RefObject::DecObjRef(g_store);
    g_mgr.CloseStore(g_delegate);
    g_delegate = nullptr;

    /**
     * @tc.steps: step4. Open store with new key.
     * @tc.expected: Succeed.
     */
    option1.passwd = g_rekeyPasswd;
    EXPECT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, option1, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(g_tableName), DBStatus::OK);

    /**
     * @tc.steps: step5. Get sync data.
     * @tc.expected: Succeed.
     */
    store = GetRelationalStore();
    ASSERT_NE(store, nullptr);
    EXPECT_EQ(store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries), E_OK);

    sqlite3_close(db);
    RefObject::DecObjRef(g_store);
}

/**
 * @tc.name: OpenEncryptedDBWithDifferentPasswd_001
 * @tc.desc: Open encrypted db with different password in split mode.
 * @tc.type: FUNC
 * @tc.require: AR000H68LL
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalEncryptedDbTest, OpenEncryptedDBWithDifferentPasswd_001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create an encrypted db.
     * @tc.expected: Succeed.
     */
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(g_storePath.c_str(), &db), SQLITE_OK);
    string sql =
        "PRAGMA key='" + CORRECT_KEY + "';"
        "PRAGMA codec_kdf_iter=" + std::to_string(DEFAULT_ITER) + ";"
        "PRAGMA journal_mode=WAL;"
        "CREATE TABLE " + g_tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    ExecSqlAndAssertOK(db, sql);

    /**
     * @tc.steps: step2. Open store.
     * @tc.expected: Succeed, return OK.
     */
    RelationalStoreDelegate::Option option {
        nullptr, false, true, CipherType::DEFAULT, g_correctPasswd, DEFAULT_ITER };
    EXPECT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, option, g_delegate), DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);

    /**
     * @tc.steps: step3. Open store with different key or iter times.
     * @tc.expected: Failed, return INVALID_PASSWD_OR_CORRUPTED_DB.
     */
    RelationalStoreDelegate *delegate = nullptr;
    option = { nullptr, false, true, CipherType::DEFAULT, g_incorrectPasswd, DEFAULT_ITER };
    EXPECT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, option, delegate), DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB);
    EXPECT_EQ(delegate, nullptr);

    option = { nullptr, false, true, CipherType::DEFAULT, g_incorrectPasswd, CUSTOMIZED_ITER };
    EXPECT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, option, delegate), DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB);
    EXPECT_EQ(delegate, nullptr);

    option = { nullptr, false, false, CipherType::DEFAULT, g_correctPasswd, DEFAULT_ITER };
    EXPECT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, option, delegate), DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB);
    EXPECT_EQ(delegate, nullptr);

    /**
     * @tc.steps: step4. Open store with different cipher.
     * @tc.expected: Succeed, return OK.
     */
    option = { nullptr, false, true, CipherType::AES_256_GCM, g_correctPasswd, DEFAULT_ITER };
    EXPECT_EQ(g_mgr.OpenStore(g_storePath, g_storeID, option, delegate), DBStatus::OK);
    EXPECT_NE(delegate, nullptr);

    EXPECT_EQ(g_mgr.CloseStore(delegate), DBStatus::OK);
    sqlite3_close(db);
}
#endif  // RELATIONAL_STORE
#endif  // OMIT_ENCRYPT
