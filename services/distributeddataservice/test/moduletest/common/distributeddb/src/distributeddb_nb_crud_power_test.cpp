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
#include <gtest/gtest.h>
#include <random>
#include <string>
#include <thread>

#include "distributed_test_tools.h"
#include "distributeddb_data_generator.h"
#include "distributeddb_nb_test_tools.h"
#include "distributeddb_schema_test_tools.h"
#include "kv_store_delegate.h"
#include "kv_store_delegate_manager.h"
#include "kv_store_nb_delegate.h"

using namespace std;
using namespace chrono;
using namespace testing;
#if defined TESTCASES_USING_GTEST_EXT
using namespace testing::ext;
#endif
using namespace DistributedDB;
using namespace DistributedDBDataGenerator;

namespace DistributeddbNbCrudPower {
KvStoreNbDelegate *g_nbDelegate = nullptr;
KvStoreDelegateManager *g_manager = nullptr;

class DistributeddbNbCrudPowerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
private:
};

void DistributeddbNbCrudPowerTest::SetUpTestCase(void)
{
}

void DistributeddbNbCrudPowerTest::TearDownTestCase(void)
{
}

void DistributeddbNbCrudPowerTest::SetUp(void)
{
    RemoveDir(DistributedDBConstant::NB_DIRECTOR);

    UnitTest *test = UnitTest::GetInstance();
    ASSERT_NE(test, nullptr);
    const TestInfo *testinfo = test->current_test_info();
    ASSERT_NE(testinfo, nullptr);
    string testCaseName = string(testinfo->name());
    MST_LOG("[SetUp] test case %s is start to run", testCaseName.c_str());

    Option option;
    option.isMemoryDb = false;
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, PERF_SCHEMA_DEFINE,
        PERF_SCHEMA_SIX_INDEXES, SKIP_SIZE);
    g_nbDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbDelegate != nullptr);
}

void DistributeddbNbCrudPowerTest::TearDown(void)
{
    MST_LOG("TearDownTestCase after case.");
    ASSERT_NE(g_manager, nullptr);
    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbDelegate, STORE_ID_1, false));
    RemoveDir(DistributedDBConstant::NB_DIRECTOR);
}

#ifndef LOW_LEVEL_MEM_DEV
void RepeatExeCrud(const vector<Entry> &putEntry, const vector<Entry> &updateEntry)
{
    ASSERT_TRUE(g_nbDelegate != nullptr);
    EXPECT_EQ(DistributedDBNbTestTools::PutBatch(*g_nbDelegate, putEntry), OK);
    vector<Entry> entries;
    for (int cnt = 0; cnt < 5; cnt++) { // the loop times are 5.
        EXPECT_EQ(g_nbDelegate->GetEntries({'p'}, entries), OK);
        entries.clear();
    }
    EXPECT_EQ(DistributedDBNbTestTools::PutBatch(*g_nbDelegate, updateEntry), OK);
    for (int cnt = 0; cnt < 5; cnt++) { // the loop times are 5.
        EXPECT_EQ(g_nbDelegate->GetEntries({'u'}, entries), OK);
        entries.clear();
    }
    for (int cnt = 0; cnt < 2; cnt++) { // the loop times are 2.
        EXPECT_EQ(g_nbDelegate->GetEntries(KEY_EMPTY, entries), OK);
        entries.clear();
    }
    for (int cnt = 0; cnt < 200; cnt++) { // the delete records are 200.
        EXPECT_EQ(g_nbDelegate->Delete(updateEntry[cnt].key), OK);
    }
}

/*
 * @tc.name: PowerConsume 001
 * @tc.desc: Test power consume of CRUD.
 * @tc.type: Power
 * @tc.require: AR000DR9K2
 * @tc.author: fengxiaoyun
 */
#ifdef NB_CRUD_POWER
HWTEST_F(DistributeddbNbCrudPowerTest, PowerConsume001, TestSize.Level4)
{
    vector<Entry> entriesBatch;
    vector<Key> allKeys;
    EntrySize entrySize = {VALUE_ONE_HUNDRED_BYTE, TWO_POINT_FOUR_LONG};
    /**
     * @tc.steps: step1-2. preset 10000 items of (keys,values) then CRUD frequently.
     * @tc.expected: step1-2. operate successfully.
     */
    entriesBatch = DistributedDBSchemaTestTools::GenerateFixedJsonSchemaRecords(TEN_THOUSAND_RECORDS,
        entrySize, 'k', '0', allKeys);
    EXPECT_EQ(DistributedDBNbTestTools::PutBatch(*g_nbDelegate, entriesBatch), OK);
    for (int cnt = 0; cnt < 2; cnt++) { // the loop times are 2.
        for (const auto &iter : allKeys) {
            Value valueGot;
            EXPECT_EQ(g_nbDelegate->Get(iter, valueGot), OK);
        }
    }
    for (int cnt = 0; cnt < 10; cnt++) { // the loop times are 10.
        int queryValue = GetRandInt(1, 9000); // get rand value between 1 and 9000.
        Query query = Query::Select().GreaterThanOrEqualTo("$.field1", queryValue).And().LessThanOrEqualTo("$.field1",
            queryValue + 1000); // query the 1000 records.
        vector<DistributedDB::Entry> entriesGot;
        EXPECT_EQ(g_nbDelegate->GetEntries(query, entriesGot), OK);
        entriesGot.clear();
    }
    vector<Entry> entriesPut, entriesUpdate;
    vector<Key> allKeys1, allKeys2;
    entriesPut = DistributedDBSchemaTestTools::GenerateFixedJsonSchemaRecords(TEN_THOUSAND_RECORDS,
        entrySize, 'p', 't', allKeys1);
    entriesUpdate = DistributedDBSchemaTestTools::GenerateFixedJsonSchemaRecords(TEN_THOUSAND_RECORDS,
        entrySize, 'u', 'e', allKeys2);
    for (int cnt = 0; cnt < 10; cnt++) { // the loop times are 10.
        RepeatExeCrud(entriesPut, entriesUpdate);
        std::this_thread::sleep_for(std::chrono::seconds(TWENTY_SECONDS));
    }
}
#endif
#endif
}
