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
#include <gtest/gtest.h>

#include "distributeddb_nb_test_tools.h"
#include "process_communicator_test_stub.h"

using namespace testing;
#if defined TESTCASES_USING_GTEST_EXT
using namespace testing::ext;
#endif
using namespace std;
using namespace DistributedDB;
using namespace DistributedDBDataGenerator;

namespace DistributedbNbDbDamage {
KvStoreDelegateManager *g_manager = nullptr;

class DistributedbNbDbDamageTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedbNbDbDamageTest::SetUpTestCase(void)
{
}

void DistributedbNbDbDamageTest::TearDownTestCase(void)
{
}

void DistributedbNbDbDamageTest::SetUp(void)
{
    RemoveDir(DistributedDBConstant::NB_DIRECTOR);
    UnitTest *test = UnitTest::GetInstance();
    ASSERT_NE(test, nullptr);
    const TestInfo *testinfo = test->current_test_info();
    ASSERT_NE(testinfo, nullptr);
    string testCaseName = string(testinfo->name());
    MST_LOG("[SetUp] test case %s is start to run", testCaseName.c_str());
}

void DistributedbNbDbDamageTest::TearDown(void)
{
}

/*
 * @tc.name: DbDamageRecover 001
 * @tc.desc: Verify that set isNeedIntegrityCheck and isNeedRmCorruptedDb when open db, if open failed the calback will
 *  be triggered  when has register corruption callback.
 * @tc.type: FUNC
 * @tc.require: SR000D4878
 * @tc.author: fengxiaoyun
 */
#ifdef NB_DBDAMAGE
HWTEST_F(DistributedbNbDbDamageTest, DbDamageRecover001, TestSize.Level1)
{
    Option option = g_option;
    KvStoreNbDelegate *delegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_NE(g_manager, nullptr);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(g_manager->CloseKvStore(delegate), OK);
    delegate = nullptr;

    /**
     * @tc.steps: step1. device A call SetKvStoreCorruptionHandler and make db file corrupted.
     * @tc.expected: step1. operate successfully.
     */
    KvStoreNbCorruptInfo corruptInfo;
    bool isCalled = false;
    auto notifier = bind(&KvStoreNbCorruptInfo::CorruptCallBack, &corruptInfo,
        placeholders::_1, placeholders::_2, placeholders::_3, std::ref(isCalled));
    g_manager->SetKvStoreCorruptionHandler(notifier);
    std::string dbFliePath = DistributedDBNbTestTools::GetKvNbStoreDirectory(g_dbParameter1);
    EXPECT_TRUE(DistributedDBNbTestTools::ModifyDatabaseFile(dbFliePath));

    /**
     * @tc.steps: step2. set isNeedIntegrityCheck = true, isNeedRmCorruptedDb = false when open db and check the
     *  corruption callback is whether triggered or not.
     * @tc.expected: step2. open db failed and the callback is triggered.
     */
    option.isNeedIntegrityCheck = true;
    option.isNeedRmCorruptedDb = false;
    DBStatus status;
    KvStoreDelegateManager *manager = nullptr;
    EXPECT_EQ(DistributedDBNbTestTools::GetNbDelegateStatus(manager, status, g_dbParameter1, option), nullptr);
    EXPECT_EQ(status, INVALID_PASSWD_OR_CORRUPTED_DB);
    EXPECT_TRUE(isCalled);

    /**
     * @tc.steps: step3. set isNeedIntegrityCheck = true, isNeedRmCorruptedDb = true when open db and check the
     *  corruption callback is whether triggered or not.
     * @tc.expected: step3. open db success and the callback isn't triggered.
     */
    option.isNeedRmCorruptedDb = true;
    isCalled = false;
    delegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && delegate != nullptr);
    EXPECT_FALSE(isCalled);
    g_manager->SetKvStoreCorruptionHandler(nullptr);
    EXPECT_TRUE(EndCaseDeleteDB(g_manager, delegate, STORE_ID_1, option.isMemoryDb));
}

/*
 * @tc.name: DbDamageRecover 002
 * @tc.desc: Verify that set isNeedIntegrityCheck and isNeedRmCorruptedDb when open db, if open successfully, the data
 * insert before db damaged is lost and the db can CRUD normally.
 * @tc.type: FUNC
 * @tc.require: SR000D4878
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributedbNbDbDamageTest, DbDamageRecover002, TestSize.Level1)
{
    Option option = g_option;
    /**
     * @tc.steps: step1. device A put (k1,v1), close db and call SetKvStoreCorruptionHandler and make db file corrupted.
     * @tc.expected: step1. operate successfully.
     */
    KvStoreNbDelegate *delegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_NE(g_manager, nullptr);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_1, VALUE_1), OK);
    EXPECT_EQ(g_manager->CloseKvStore(delegate), OK);
    delegate = nullptr;
    KvStoreNbCorruptInfo corruptInfo;
    bool isCalled = false;
    auto notifier = bind(&KvStoreNbCorruptInfo::CorruptCallBack, &corruptInfo,
        placeholders::_1, placeholders::_2, placeholders::_3, std::ref(isCalled));
    g_manager->SetKvStoreCorruptionHandler(notifier);
    std::string dbFliePath = DistributedDBNbTestTools::GetKvNbStoreDirectory(g_dbParameter1);
    EXPECT_TRUE(DistributedDBNbTestTools::ModifyDatabaseFile(dbFliePath));

    /**
     * @tc.steps: step2. set isNeedIntegrityCheck = false, isNeedRmCorruptedDb = true when open db and check the
     *  corruption callback is whether triggered or not.
     * @tc.expected: step2. open db success and the callback isn't triggered.
     */
    option.isNeedIntegrityCheck = false;
    option.isNeedRmCorruptedDb = true;
    delegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && delegate != nullptr);
    EXPECT_FALSE(isCalled);

    /**
     * @tc.steps: step4. Get(k1), put(k1,v1)(k2,v2), delete(k1), put(k2,v3), Get(k1)(k2).
     * @tc.expected: step4. Get(k1)=null, put and delete successfully, Get(k1)=v1, Get(k2)=v3.
     */
    EXPECT_TRUE(DistributedDBNbTestTools::CheckNbRecord(delegate, KEY_1, VALUE_EMPTY));
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_1, VALUE_1), OK);
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_2, VALUE_2), OK);
    EXPECT_EQ(DistributedDBNbTestTools::Delete(*delegate, KEY_2), OK);
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_1, VALUE_2), OK);
    EXPECT_TRUE(DistributedDBNbTestTools::CheckNbRecord(delegate, KEY_1, VALUE_2));
    EXPECT_TRUE(DistributedDBNbTestTools::CheckNbRecord(delegate, KEY_2, VALUE_EMPTY));
    g_manager->SetKvStoreCorruptionHandler(nullptr);
    EXPECT_TRUE(EndCaseDeleteDB(g_manager, delegate, STORE_ID_1, option.isMemoryDb));
}
#endif
/*
 * @tc.name: DbDamageRecover 003
 * @tc.desc: Verify that the db is damaged after db is opened, open db again won't trigger damage recover even set
 *  isNeedIntegrityCheck and isNeedRmCorruptedDb.
 * @tc.type: FUNC
 * @tc.require: SR000D4878
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributedbNbDbDamageTest, DbDamageRecover003, TestSize.Level1)
{
    Option option = g_option;
    /**
     * @tc.steps: step1. device A open delegate1 and don't close, call SetKvStoreCorruptionHandler and make db file
     *  corrupted.
     * @tc.expected: step1. operate successfully.
     */
    KvStoreDelegateManager *manager1 = nullptr;
    KvStoreNbDelegate *delegate1 = DistributedDBNbTestTools::GetNbDelegateSuccess(manager1, g_dbParameter1, option);
    ASSERT_NE(manager1, nullptr);
    ASSERT_NE(delegate1, nullptr);

    KvStoreNbCorruptInfo corruptInfo;
    bool isCalled = false;
    auto notifier = bind(&KvStoreNbCorruptInfo::CorruptCallBack, &corruptInfo,
        placeholders::_1, placeholders::_2, placeholders::_3, std::ref(isCalled));
    g_manager->SetKvStoreCorruptionHandler(notifier);
    std::string dbFliePath = DistributedDBNbTestTools::GetKvNbStoreDirectory(g_dbParameter1);
    EXPECT_TRUE(DistributedDBNbTestTools::ModifyDatabaseFile(dbFliePath));
    EXPECT_TRUE(DistributedDBNbTestTools::ModifyDatabaseFile(dbFliePath + "-wal"));

    /**
     * @tc.steps: step2. set isNeedIntegrityCheck = true, isNeedRmCorruptedDb = false when open db and check the
     *  corruption callback is whether triggered or not.
     * @tc.expected: step2. open db successfully and the callback isn't triggered.
     */
    option.isNeedIntegrityCheck = true;
    option.isNeedRmCorruptedDb = false;
    KvStoreDelegateManager *manager2 = nullptr;
    KvStoreNbDelegate *delegate2 = DistributedDBNbTestTools::GetNbDelegateSuccess(manager2, g_dbParameter1, option);
    ASSERT_NE(manager2, nullptr);
    ASSERT_NE(delegate2, nullptr);
    EXPECT_FALSE(isCalled);

    /**
     * @tc.steps: step3. close delegate1 and do step 2 again.
     * @tc.expected: step3. open db success and the callback isn't triggered.
     */
    EXPECT_TRUE(DistributedTestTools::CloseAndRelease(manager1, delegate1));
    delegate1 = DistributedDBNbTestTools::GetNbDelegateSuccess(manager1, g_dbParameter1, option);
    ASSERT_NE(manager1, nullptr);
    ASSERT_NE(delegate1, nullptr);
    EXPECT_FALSE(isCalled);

    /**
     * @tc.steps: step4. device A put(k1,v1) and check the corruption callback.
     * @tc.expected: step4. put failed and return INVALID_PASSWD_OR_CORRUPTED_DB, the callback is triggered.
     */
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate1, KEY_1, VALUE_1), INVALID_PASSWD_OR_CORRUPTED_DB);
    std::this_thread::sleep_for(std::chrono::seconds(UNIQUE_SECOND)); // wait for callback complete for 1 second.
    EXPECT_TRUE(isCalled);

    /**
     * @tc.steps: step5. close delegate1,delegate2, then open db with isNeedIntegrityCheck = true,
     *  isNeedRmCorruptedDb = false, and check the corruption callback.
     * @tc.expected: step5. open failed and return INVALID_PASSWD_OR_CORRUPTED_DB, the callback is triggered.
     */
    EXPECT_TRUE(DistributedTestTools::CloseAndRelease(manager1, delegate1));
    EXPECT_TRUE(DistributedTestTools::CloseAndRelease(manager2, delegate2));
    DBStatus status;
    isCalled = false;
    EXPECT_EQ(DistributedDBNbTestTools::GetNbDelegateStatus(manager1, status, g_dbParameter1, option), nullptr);
    EXPECT_EQ(status, INVALID_PASSWD_OR_CORRUPTED_DB);
    EXPECT_TRUE(isCalled);
    KvStoreDelegateManager Manager(APP_ID_1, USER_ID_1);
    Manager.SetKvStoreCorruptionHandler(nullptr);
    RemoveDir(DistributedDBConstant::NB_DIRECTOR);
}
} // end of namespace DistributedbNbDbDamageTest