/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <chrono>
#include <functional>
#include <gtest/gtest.h>
#include <thread>

#include "db_common.h"
#include "db_constant.h"
#include "distributeddb_tools_unit_test.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
    const std::string APP_NAME = "app";
    const std::string USER_NAME = "account0";
    const int PASSWD_SIZE = 20;
    const int WAIT_CALLBACK_TIME = 100;
    KvStoreDelegateManager g_mgr(APP_NAME, USER_NAME);
    string g_testDir;
    KvStoreConfig g_config;

    DBStatus g_kvDelegateStatus = INVALID_ARGS;
    DBStatus g_kvNbDelegateStatus = INVALID_ARGS;
    KvStoreDelegate *g_kvDelegatePtr = nullptr;
    KvStoreNbDelegate *g_kvNbDelegatePtr = nullptr;
    auto g_kvDelegateCallback = std::bind(&DistributedDBToolsUnitTest::KvStoreDelegateCallback, std::placeholders::_1,
        std::placeholders::_2, std::ref(g_kvDelegateStatus), std::ref(g_kvDelegatePtr));
    auto g_kvNbDelegateCallback = std::bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        std::placeholders::_1, std::placeholders::_2, std::ref(g_kvNbDelegateStatus), std::ref(g_kvNbDelegatePtr));

    std::string GetKvStoreDirectory(const std::string &storeId, int databaseType)
    {
        std::string identifier = USER_NAME + "-" + APP_NAME + "-" + storeId;
        std::string hashIdentifierName = DBCommon::TransferHashString(identifier);
        std::string identifierName = DBCommon::TransferStringToHex(hashIdentifierName);
        std::string filePath = g_testDir + "/" + identifierName + "/";
        if (databaseType == DBConstant::DB_TYPE_LOCAL) { // local
            filePath += (DBConstant::LOCAL_SUB_DIR + "/" + DBConstant::LOCAL_DATABASE_NAME +
                DBConstant::SQLITE_DB_EXTENSION);
        } else if (databaseType == DBConstant::DB_TYPE_SINGLE_VER) { // single ver
            filePath += (DBConstant::SINGLE_SUB_DIR + "/" + DBConstant::MAINDB_DIR + "/" +
                DBConstant::SINGLE_VER_DATA_STORE + DBConstant::SQLITE_DB_EXTENSION);
        } else if (databaseType == DBConstant::DB_TYPE_MULTI_VER) { // multi ver
            filePath += (DBConstant::MULTI_SUB_DIR + "/" + DBConstant::MULTI_VER_DATA_STORE +
                DBConstant::SQLITE_DB_EXTENSION);
        } else {
            filePath = "";
        }

        return filePath;
    }

    int PutDataIntoDatabase(KvStoreDelegate *kvDelegate, KvStoreNbDelegate *kvNbDelegate)
    {
        if (kvDelegate == nullptr && kvNbDelegate == nullptr) {
            return DBStatus::DB_ERROR;
        }
        Key key;
        Value value;
        DistributedDBToolsUnitTest::GetRandomKeyValue(key);
        DistributedDBToolsUnitTest::GetRandomKeyValue(value);
        DBStatus status = OK;
        if (kvDelegate != nullptr) {
            status = kvDelegate->Put(key, value);
            if (status != OK) {
                return status;
            }
        }
        if (kvNbDelegate != nullptr) {
            status = kvNbDelegate->Put(key, value);
            if (status != OK) {
                return status;
            }
        }
        return status;
    }
}

class DistributedDBInterfacesDatabaseCorruptTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBInterfacesDatabaseCorruptTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_config.dataDir = g_testDir;
    g_mgr.SetKvStoreConfig(g_config);
}

void DistributedDBInterfacesDatabaseCorruptTest::TearDownTestCase(void)
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
}

void DistributedDBInterfacesDatabaseCorruptTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    g_kvDelegateStatus = INVALID_ARGS;
    g_kvDelegatePtr = nullptr;
}

void DistributedDBInterfacesDatabaseCorruptTest::TearDown(void)
{
    g_mgr.SetKvStoreCorruptionHandler(nullptr);
}

/**
  * @tc.name: DatabaseCorruptionHandleTest001
  * @tc.desc: Check the corruption detect without setting the corrupt handler.
  * @tc.type: FUNC
  * @tc.require: AR000D487C SR000D4878
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBInterfacesDatabaseCorruptTest, DatabaseCorruptionHandleTest001, TestSize.Level3)
{
    /**
     * @tc.steps: step1. Obtain the kvStore.
     * @tc.steps: step2. Put one data into the store.
     * @tc.steps: step3. Close the store.
     */
    CipherPassword passwd;
    Key randomPassword;
    DistributedDBToolsUnitTest::GetRandomKeyValue(randomPassword, PASSWD_SIZE);
    int errCode = passwd.SetValue(randomPassword.data(), randomPassword.size());
    ASSERT_EQ(errCode, CipherPassword::ErrorCode::OK);
    KvStoreDelegate::Option option = {true, true, false, CipherType::DEFAULT, passwd};
    g_mgr.GetKvStore("corrupt1", option, g_kvDelegateCallback);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_EQ(PutDataIntoDatabase(g_kvDelegatePtr, nullptr), OK);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
    g_kvDelegatePtr = nullptr;

    /**
     * @tc.steps: step4. Modify the database file.
     */
    std::string filePath = GetKvStoreDirectory("corrupt1", DBConstant::DB_TYPE_LOCAL); // local database.
    DistributedDBToolsUnitTest::ModifyDatabaseFile(filePath);

    /**
     * @tc.steps: step5. Re-obtain the kvStore.
     * @tc.expected: step5. Returns null kvstore.
     */
    g_mgr.GetKvStore("corrupt1", option, g_kvDelegateCallback);
    ASSERT_TRUE(g_kvDelegatePtr == nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == INVALID_PASSWD_OR_CORRUPTED_DB);
    g_mgr.DeleteKvStore("corrupt1");
}

/**
  * @tc.name: DatabaseCorruptionHandleTest002
  * @tc.desc: Get kv store through different parameters for the same storeID.
  * @tc.type: FUNC
  * @tc.require: AR000D487C SR000D4878
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBInterfacesDatabaseCorruptTest, DatabaseCorruptionHandleTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Get the kvStore.
     * @tc.steps: step2. Put data into the store.
     * @tc.steps: step3. Close the store.
     */
    CipherPassword passwd;
    Key randomPassword;
    DistributedDBToolsUnitTest::GetRandomKeyValue(randomPassword, PASSWD_SIZE);
    int errCode = passwd.SetValue(randomPassword.data(), randomPassword.size());
    ASSERT_EQ(errCode, CipherPassword::ErrorCode::OK);
    KvStoreDelegate::Option option = {true, false, false, CipherType::DEFAULT, passwd};
    KvStoreNbDelegate::Option nbOption = {true, false, false, CipherType::DEFAULT, passwd};

    g_mgr.GetKvStore("corrupt2", option, g_kvDelegateCallback);
    g_mgr.GetKvStore("corrupt3", nbOption, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    ASSERT_EQ(PutDataIntoDatabase(g_kvDelegatePtr, g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    g_kvDelegatePtr = nullptr;
    g_kvNbDelegatePtr = nullptr;

    /**
     * @tc.steps: step4. Modify the database file.
     */
    std::string filePath = GetKvStoreDirectory("corrupt2", DBConstant::DB_TYPE_MULTI_VER);
    DistributedDBToolsUnitTest::ModifyDatabaseFile(filePath);
    filePath = GetKvStoreDirectory("corrupt3", DBConstant::DB_TYPE_SINGLE_VER); // single ver database.
    DistributedDBToolsUnitTest::ModifyDatabaseFile(filePath);
    KvStoreCorruptInfo corruptInfo;
    auto notifier = bind(&KvStoreCorruptInfo::CorruptCallBack, &corruptInfo, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3);
    g_mgr.SetKvStoreCorruptionHandler(notifier);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_CALLBACK_TIME));
    /**
     * @tc.steps: step5. Re-obtain the kvStore.
     * @tc.expected: step5. Returns null kvstore.
     */
    g_mgr.GetKvStore("corrupt2", option, g_kvDelegateCallback);
    g_mgr.GetKvStore("corrupt3", nbOption, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvDelegateStatus != OK);
    ASSERT_TRUE(g_kvNbDelegateStatus != OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_CALLBACK_TIME));
    EXPECT_EQ(corruptInfo.GetDatabaseInfoSize(), 2UL); // 2 callback
    EXPECT_EQ(corruptInfo.IsDataBaseCorrupted(APP_NAME, USER_NAME, "corrupt2"), true);
    EXPECT_EQ(corruptInfo.IsDataBaseCorrupted(APP_NAME, USER_NAME, "corrupt3"), true);
    g_mgr.DeleteKvStore("corrupt2");
    g_mgr.DeleteKvStore("corrupt3");
}

/**
  * @tc.name: DatabaseCorruptionHandleTest003
  * @tc.desc: Test the CloseKvStore Interface and check whether the database file can be closed.
  * @tc.type: FUNC
  * @tc.require: AR000D487C SR000D4878
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBInterfacesDatabaseCorruptTest, DatabaseCorruptionHandleTest003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Get the kvStore.
     * @tc.steps: step2. Put data into the store.
     * @tc.steps: step3. Close the store.
     */
    CipherPassword passwd;
    Key randomPassword;
    DistributedDBToolsUnitTest::GetRandomKeyValue(randomPassword, PASSWD_SIZE);
    int errCode = passwd.SetValue(randomPassword.data(), randomPassword.size());
    ASSERT_EQ(errCode, CipherPassword::ErrorCode::OK);
    KvStoreDelegate::Option option = {true, true, false, CipherType::DEFAULT, passwd};
    KvStoreNbDelegate::Option nbOption = {true, false, false, CipherType::DEFAULT, passwd};

    g_mgr.GetKvStore("corrupt4", option, g_kvDelegateCallback);
    g_mgr.GetKvStore("corrupt5", nbOption, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    ASSERT_EQ(PutDataIntoDatabase(g_kvDelegatePtr, g_kvNbDelegatePtr), OK);

    /**
     * @tc.steps: step4. Modify the database file.
     */
    std::string filePath = GetKvStoreDirectory("corrupt4", DBConstant::DB_TYPE_LOCAL); // local database.
    DistributedDBToolsUnitTest::ModifyDatabaseFile(filePath);
    DistributedDBToolsUnitTest::ModifyDatabaseFile(filePath + "-wal");
    filePath = GetKvStoreDirectory("corrupt5", DBConstant::DB_TYPE_SINGLE_VER); // single ver database.
    DistributedDBToolsUnitTest::ModifyDatabaseFile(filePath);
    DistributedDBToolsUnitTest::ModifyDatabaseFile(filePath + "-wal");
    KvStoreCorruptInfo corruptInfo;
    auto notifier = bind(&KvStoreCorruptInfo::CorruptCallBack, &corruptInfo, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3);
    g_mgr.SetKvStoreCorruptionHandler(notifier);

    /**
     * @tc.steps: step5. Put data into the kvStore.
     * @tc.expected: step5. The corrupt handler is called twice.
     */
    ASSERT_NE(PutDataIntoDatabase(g_kvDelegatePtr, nullptr), OK);
    ASSERT_NE(PutDataIntoDatabase(nullptr, g_kvNbDelegatePtr), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_CALLBACK_TIME));
    EXPECT_TRUE(corruptInfo.GetDatabaseInfoSize() >= 2UL); // 2 more callback
    EXPECT_EQ(corruptInfo.IsDataBaseCorrupted(APP_NAME, USER_NAME, "corrupt4"), true);
    EXPECT_EQ(corruptInfo.IsDataBaseCorrupted(APP_NAME, USER_NAME, "corrupt5"), true);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    g_kvDelegatePtr = nullptr;
    g_kvNbDelegatePtr = nullptr;
    EXPECT_EQ(g_mgr.DeleteKvStore("corrupt4"), OK);
    EXPECT_EQ(g_mgr.DeleteKvStore("corrupt5"), OK);
}

/**
  * @tc.name: DatabaseCorruptionHandleTest004
  * @tc.desc: Test the DeleteKvStore Interface and check whether the database files can be removed.
  * @tc.type: FUNC
  * @tc.require: AR000D487C SR000D4878
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBInterfacesDatabaseCorruptTest, DatabaseCorruptionHandleTest004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Get the kvStore.
     * @tc.steps: step2. Put data into the store.
     * @tc.steps: step3. Close the store.
     */
    CipherPassword passwd;
    Key randomPassword;
    DistributedDBToolsUnitTest::GetRandomKeyValue(randomPassword, PASSWD_SIZE);
    int errCode = passwd.SetValue(randomPassword.data(), randomPassword.size());
    ASSERT_EQ(errCode, CipherPassword::ErrorCode::OK);
    KvStoreDelegate::Option option = {true, true, false, CipherType::DEFAULT, passwd};
    KvStoreNbDelegate::Option nbOption = {true, false, false, CipherType::DEFAULT, passwd};

    g_mgr.GetKvStore("corrupt6", option, g_kvDelegateCallback);
    g_mgr.GetKvStore("corrupt7", nbOption, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    ASSERT_EQ(PutDataIntoDatabase(g_kvDelegatePtr, g_kvNbDelegatePtr), OK);

    /**
     * @tc.steps: step4. Modify the database file.
     */
    std::string filePath = GetKvStoreDirectory("corrupt6", DBConstant::DB_TYPE_LOCAL); // local database.
    DistributedDBToolsUnitTest::ModifyDatabaseFile(filePath);
    DistributedDBToolsUnitTest::ModifyDatabaseFile(filePath + "-wal");
    filePath = GetKvStoreDirectory("corrupt7", DBConstant::DB_TYPE_SINGLE_VER); // single ver database.
    DistributedDBToolsUnitTest::ModifyDatabaseFile(filePath);
    DistributedDBToolsUnitTest::ModifyDatabaseFile(filePath + "-wal");
    KvStoreCorruptInfo corruptInfo;
    KvStoreCorruptInfo corruptInfoNew;
    auto notifier = bind(&KvStoreCorruptInfo::CorruptCallBack, &corruptInfo, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3);
    g_mgr.SetKvStoreCorruptionHandler(notifier);
    auto notifierNew = bind(&KvStoreCorruptInfo::CorruptCallBack, &corruptInfoNew, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3);
    g_mgr.SetKvStoreCorruptionHandler(notifierNew);
    /**
     * @tc.steps: step5. Re-obtain the kvStore.
     * @tc.expected: step5. Returns null kvstore.
     */
    ASSERT_NE(PutDataIntoDatabase(g_kvDelegatePtr, nullptr), OK);
    ASSERT_NE(PutDataIntoDatabase(nullptr, g_kvNbDelegatePtr), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_CALLBACK_TIME));
    EXPECT_EQ(corruptInfo.GetDatabaseInfoSize(), 0UL); // no callback
    EXPECT_TRUE(corruptInfoNew.GetDatabaseInfoSize() >= 2UL); // 2 more callback
    EXPECT_EQ(corruptInfoNew.IsDataBaseCorrupted(APP_NAME, USER_NAME, "corrupt6"), true);
    EXPECT_EQ(corruptInfoNew.IsDataBaseCorrupted(APP_NAME, USER_NAME, "corrupt7"), true);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    g_kvDelegatePtr = nullptr;
    g_kvNbDelegatePtr = nullptr;
    EXPECT_EQ(g_mgr.DeleteKvStore("corrupt6"), OK);
    EXPECT_EQ(g_mgr.DeleteKvStore("corrupt7"), OK);
}

namespace {
const uint32_t MODIFY_SIZE = 12; // Modify size is 12 * sizeof(uint32_t);
const uint32_t MODIFY_VALUE = 0xF3F3F3F3; // random value, make sure to destroy the page header.
void TestDatabaseIntegrityCheckOption(const std::string &storeId, bool isEncrypted)
{
    KvStoreNbDelegate::Option nbOption = {true, false, isEncrypted};
    nbOption.isNeedIntegrityCheck = false;
    nbOption.isNeedRmCorruptedDb = false;
    if (isEncrypted) {
        Key randPassword;
        DistributedDBToolsUnitTest::GetRandomKeyValue(randPassword, PASSWD_SIZE);
        ASSERT_EQ(nbOption.passwd.SetValue(randPassword.data(), randPassword.size()), CipherPassword::ErrorCode::OK);
    }
    auto filePath = GetKvStoreDirectory(storeId, DBConstant::DB_TYPE_SINGLE_VER);
    g_mgr.GetKvStore(storeId, nbOption, g_kvNbDelegateCallback);
    ASSERT_EQ(g_kvNbDelegateStatus, OK);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    ASSERT_EQ(PutDataIntoDatabase(nullptr, g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);

    /**
     * @tc.steps: step1. Modify the database file header to destroy the header and call the GetKvStore.
     * @tc.expected: step1. Returns null kv store and the errCode is INVALID_PASSWD_OR_CORRUPTED_DB.
     */
    DistributedDBToolsUnitTest::ModifyDatabaseFile(filePath, 0, MODIFY_SIZE, MODIFY_VALUE);
    g_mgr.GetKvStore(storeId, nbOption, g_kvNbDelegateCallback);
    ASSERT_EQ(g_kvNbDelegateStatus, INVALID_PASSWD_OR_CORRUPTED_DB);
    ASSERT_TRUE(g_kvNbDelegatePtr == nullptr);

    /**
     * @tc.steps: step2. call the GetKvStore with integrity check option is true.
     * @tc.expected: step2. Returns null kv store and the errCode is INVALID_PASSWD_OR_CORRUPTED_DB.
     */
    nbOption.isNeedIntegrityCheck = true;
    g_mgr.GetKvStore(storeId, nbOption, g_kvNbDelegateCallback);
    ASSERT_EQ(g_kvNbDelegateStatus, INVALID_PASSWD_OR_CORRUPTED_DB);
    ASSERT_TRUE(g_kvNbDelegatePtr == nullptr);

    /**
     * @tc.steps: step3. call the GetKvStore with remove corrupted database option is true.
     * @tc.expected: step3. Returns non-null kv store and the errCode is OK.
     */
    nbOption.isNeedIntegrityCheck = false;
    nbOption.isNeedRmCorruptedDb = true;
    g_mgr.GetKvStore(storeId, nbOption, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);

    ASSERT_EQ(PutDataIntoDatabase(nullptr, g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);

    /**
     * @tc.steps: step4. Modify the second page of the database file and Get the kv store.
     * @tc.expected: step4. Returns non-null kv store and the errCode is OK(GetKvStore skip the check of the page 2).
     */
    size_t filePos = isEncrypted ? 1024 : 4096; // 1024 and 4096 is the page size.
    DistributedDBToolsUnitTest::ModifyDatabaseFile(filePath, filePos, MODIFY_SIZE, MODIFY_VALUE);
    nbOption.isNeedRmCorruptedDb = false;
    g_mgr.GetKvStore(storeId, nbOption, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);

    /**
     * @tc.steps: step5. Get the kv store with check the integrity.
     * @tc.expected: step5. Returns null kv store and the errCode is INVALID_PASSWD_OR_CORRUPTED_DB.
     */
    nbOption.isNeedIntegrityCheck = true;
    g_mgr.GetKvStore(storeId, nbOption, g_kvNbDelegateCallback);
    ASSERT_EQ(g_kvNbDelegateStatus, INVALID_PASSWD_OR_CORRUPTED_DB);
    ASSERT_TRUE(g_kvNbDelegatePtr == nullptr);

    /**
     * @tc.steps: step5. Get the kv store with check the integrity and the rm corrupted database option.
     * @tc.expected: step5. Returns non-null kv store and the errCode is OK.
     */
    nbOption.isNeedRmCorruptedDb = true;
    g_mgr.GetKvStore(storeId, nbOption, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.DeleteKvStore(storeId), OK);
}
}

/**
  * @tc.name: DatabaseIntegrityCheck001
  * @tc.desc: Test the integrity check option.
  * @tc.type: FUNC
  * @tc.require: AR000D487C SR000D4878
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBInterfacesDatabaseCorruptTest, DatabaseIntegrityCheck001, TestSize.Level2)
{
    LOGI("Begin to check the unencrypted database");
    TestDatabaseIntegrityCheckOption("integrity_check001", false);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    LOGI("Begin to check the encrypted database");
    TestDatabaseIntegrityCheckOption("integrity_check002", true);
}

/**
  * @tc.name: DatabaseIntegrityCheck002
  * @tc.desc: Test the integrity check interface.
  * @tc.type: FUNC
  * @tc.require: AR000D487C SR000D4878
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBInterfacesDatabaseCorruptTest, DatabaseIntegrityCheck002, TestSize.Level1)
{
    CipherPassword passwd;
    KvStoreNbDelegate::Option nbOption = {true, false, false, CipherType::DEFAULT, passwd};
    nbOption.isNeedIntegrityCheck = true;
    nbOption.isNeedRmCorruptedDb = true;
    auto filePath = GetKvStoreDirectory("integrity021", DBConstant::DB_TYPE_SINGLE_VER);
    for (uint32_t i = 1; i < 4; i++) {
        LOGI("%u th test!", i);
        g_mgr.GetKvStore("integrity021", nbOption, g_kvNbDelegateCallback);
        ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
        ASSERT_EQ(g_kvNbDelegatePtr->CheckIntegrity(), OK);
        ASSERT_EQ(PutDataIntoDatabase(nullptr, g_kvNbDelegatePtr), OK);
        EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
        DistributedDBToolsUnitTest::ModifyDatabaseFile(filePath, i * 4096, MODIFY_SIZE, MODIFY_VALUE); // page size 4096
        g_mgr.GetKvStore("integrity021", nbOption, g_kvNbDelegateCallback);
        ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
        EXPECT_EQ(g_kvNbDelegatePtr->CheckIntegrity(), OK);
        EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
        EXPECT_EQ(g_mgr.DeleteKvStore("integrity021"), OK);
    }
    LOGI("Begin the encrypted check");
    Key randomPassword;
    DistributedDBToolsUnitTest::GetRandomKeyValue(randomPassword, PASSWD_SIZE);
    int errCode = passwd.SetValue(randomPassword.data(), randomPassword.size());
    ASSERT_EQ(errCode, CipherPassword::ErrorCode::OK);
    nbOption = {true, false, true, CipherType::DEFAULT, passwd};
    nbOption.isNeedIntegrityCheck = true;
    nbOption.isNeedRmCorruptedDb = true;
    filePath = GetKvStoreDirectory("integrity022", DBConstant::DB_TYPE_SINGLE_VER);
    for (uint32_t i = 1; i < 4; i++) {
        LOGI("%u th test the encrypted database!", i);
        g_mgr.GetKvStore("integrity022", nbOption, g_kvNbDelegateCallback);
        ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
        ASSERT_EQ(g_kvNbDelegatePtr->CheckIntegrity(), OK);
        ASSERT_EQ(PutDataIntoDatabase(nullptr, g_kvNbDelegatePtr), OK);
        EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
        DistributedDBToolsUnitTest::ModifyDatabaseFile(filePath, i * 1024, MODIFY_SIZE, MODIFY_VALUE); // page size 1024
        g_mgr.GetKvStore("integrity022", nbOption, g_kvNbDelegateCallback);
        ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
        EXPECT_EQ(g_kvNbDelegatePtr->CheckIntegrity(), OK);
        EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
        EXPECT_EQ(g_mgr.DeleteKvStore("integrity022"), OK);
    }
}