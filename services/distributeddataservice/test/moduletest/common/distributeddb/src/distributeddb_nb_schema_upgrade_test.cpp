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

#include "distributeddb_schema_test_tools.h"

using namespace std;
using namespace testing;
#if defined TESTCASES_USING_GTEST_EXT
using namespace testing::ext;
#endif
using namespace DistributedDB;
using namespace DistributedDBDataGenerator;

namespace DistributeddbNbSchemaUpgradeDb {
KvStoreNbDelegate *g_nbSchemaUpgradeDelegate = nullptr;
KvStoreDelegateManager *g_manager = nullptr;
class DistributeddbNbSchemaUpgradeTest : public testing::Test {
public:
    void SetUp();
    void TearDown();
private:
};

void DistributeddbNbSchemaUpgradeTest::SetUp(void)
{
    RemoveDir(DistributedDBConstant::NB_DIRECTOR);

    UnitTest *test = UnitTest::GetInstance();
    ASSERT_NE(test, nullptr);
    const TestInfo *testinfo = test->current_test_info();
    ASSERT_NE(testinfo, nullptr);
    string testCaseName = string(testinfo->name());
    MST_LOG("[SetUp] test case %s is start to run", testCaseName.c_str());
}

void DistributeddbNbSchemaUpgradeTest::TearDown(void)
{
    MST_LOG("TearDownTestCase after case.");
    RemoveDir(DistributedDBConstant::NB_DIRECTOR);
}

/**
 * @tc.name: SchemaNotNullDefault 001
 * @tc.desc: Verify that schema db has the field that with not-null and default Attribute
 *    can normally insert the value has not null field value
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, SchemaNotNullDefault001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create db with valid SCHEMA_DEFINE_NNULL_DEFAULT, strict mode and version 1.0.
     * @tc.expected: step1. create successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_NNULL_DEFAULT, VALID_INDEX_1);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    /**
     * @tc.steps: step2. insert valid value to DB.
     * @tc.expected: step2. insert succeed.
     */
    string stringValue = "{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value(stringValue.begin(), stringValue.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_1, value), OK);

    /**
     * @tc.steps: step3. check the data in db.
     * @tc.expected: step3. can find value in db.
     */
    Query query = Query::Select().EqualTo("$.field1", "schema");
    vector<Entry> entriesGot;
    vector<Entry> entries = {{KEY_1, value}};
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->GetEntries(query, entriesGot), OK);
    EXPECT_TRUE(CompareEntriesVector(entriesGot, entries));

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

void InsertFieldWithNullValue(KvStoreNbDelegate *&delegate, DBStatus expectStatus)
{
    /**
     * @tc.steps: step2. insert invalid value1 which with null field1 to DB.
     * @tc.expected: step2. insert failed.
     */
    string stringValue1 = "{\"field1\":null,\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value1(stringValue1.begin(), stringValue1.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_1, value1), expectStatus);
    /**
     * @tc.steps: step3. insert invalid value2 which with null field3 to DB.
     * @tc.expected: step3. insert failed.
     */
    string stringValue2 = "{\"field1\":\"schema\",\"field2\":{\"field3\":null,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value2(stringValue2.begin(), stringValue2.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_2, value2), expectStatus);
    /**
     * @tc.steps: step4. insert invalid value3 which with null field4 to DB.
     * @tc.expected: step4. insert failed.
     */
    string stringValue3 = "{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":null,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value3(stringValue3.begin(), stringValue3.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_3, value3), expectStatus);
    /**
     * @tc.steps: step5. insert invalid value4 which with null field5 to DB.
     * @tc.expected: step5. insert failed.
     */
    string stringValue4 = "{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":null,\"field2\":{\"field6\":-123.456}}}}";
    Value value4(stringValue4.begin(), stringValue4.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_4, value4), expectStatus);
    /**
     * @tc.steps: step6. insert invalid value5 which with null field6 to DB.
     * @tc.expected: step6. insert failed.
     */
    string stringValue5 = "{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":null}}}}";
    Value value5(stringValue5.begin(), stringValue5.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_5, value5), expectStatus);
    /**
     * @tc.steps: step7. insert invalid value6 all fields of which with null value to DB.
     * @tc.expected: step7. insert failed.
     */
    string stringValue6 = "{\"field1\":null,\"field2\":{\"field3\":null,\"field4\":null,"
        "\"field1\":{\"field5\":null,\"field2\":{\"field6\":null}}}}";
    Value value6(stringValue6.begin(), stringValue6.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_6, value6), expectStatus);
}

/**
 * @tc.name: SchemaNotNullDefault 002
 * @tc.desc: Verify that schema db has the field that with not-null and default Attribute
 *    can't insert the value field of which is null
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, SchemaNotNullDefault002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create db with valid SCHEMA_DEFINE_NNULL_DEFAULT, strict mode and version 1.0.
     * @tc.expected: step1. create successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_NNULL_DEFAULT, VALID_INDEX_1);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    InsertFieldWithNullValue(g_nbSchemaUpgradeDelegate, CONSTRAIN_VIOLATION);

    /**
     * @tc.steps: step8. check the data in db.
     * @tc.expected: step8. can't find any records in db.
     */
    vector<Entry> entriesGot;
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->GetEntries(KEY_EMPTY, entriesGot), NOT_FOUND);

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

void InsertVoidFieldSteps(KvStoreNbDelegate *&delegate, DBStatus expectStatus)
{
    /**
     * @tc.steps: step2. insert valid value1 field1 of which hasn't value to DB.
     * @tc.expected: step2. result as expectStatus.
     */
    string stringValue1 = "{\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value1(stringValue1.begin(), stringValue1.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_1, value1), expectStatus);
    /**
     * @tc.steps: step3. insert valid value2 field3 of which hasn't value to DB.
     * @tc.expected: step3. result as expectStatus.
     */
    string stringValue2 = "{\"field1\":\"schema\",\"field2\":{\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value2(stringValue2.begin(), stringValue2.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_2, value2), expectStatus);
    /**
     * @tc.steps: step4. insert valid value3 field4 of which hasn't value to DB.
     * @tc.expected: step4. result as expectStatus.
     */
    string stringValue3 = "{\"field1\":\"schema\",\"field2\":{\"field3\":123,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value3(stringValue3.begin(), stringValue3.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_3, value3), expectStatus);
    /**
     * @tc.steps: step5. insert valid value4 field5 of which hasn't value to DB.
     * @tc.expected: step5. result as expectStatus.
     */
    string stringValue4 = "{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field2\":{\"field6\":-123.456}}}}";
    Value value4(stringValue4.begin(), stringValue4.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_4, value4), expectStatus);
    /**
     * @tc.steps: step6. insert valid value5 field6 of which hasn't value to DB.
     * @tc.expected: step6. result as expectStatus.
     */
    string stringValue5 = "{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{}}}}";
    Value value5(stringValue5.begin(), stringValue5.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_5, value5), expectStatus);
    /**
     * @tc.steps: step7. insert valid value6 field1, field4, field6 of which hasn't value to DB.
     * @tc.expected: step7. result as expectStatus.
     */
    string stringValue6 = "{\"field2\":{\"field3\":123,\"field1\":{\"field5\":123567,\"field2\":{}}}}";
    Value value6(stringValue6.begin(), stringValue6.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_6, value6), expectStatus);
    /**
     * @tc.steps: step8. insert valid value7 all fields of which have default values to DB.
     * @tc.expected: step8. result as expectStatus.
     */
    string stringValue7 = "{\"field2\":{\"field1\":{\"field2\":{}}}}";
    Value value7(stringValue7.begin(), stringValue7.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_7, value7), expectStatus);
}

void CheckSchemaVoidFieldDefaultResult()
{
    vector<Entry> entriesGot;
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->GetEntries(KEY_EMPTY, entriesGot), OK);
    sort(entriesGot.begin(), entriesGot.end(), DistributedTestTools::CompareKey);
    string stringValue1d(entriesGot[INDEX_ZEROTH].value.begin(), entriesGot[INDEX_ZEROTH].value.end());
    string stringValue2d(entriesGot[INDEX_FIRST].value.begin(), entriesGot[INDEX_FIRST].value.end());
    string stringValue3d(entriesGot[INDEX_SECOND].value.begin(), entriesGot[INDEX_SECOND].value.end());
    string stringValue4d(entriesGot[INDEX_THIRD].value.begin(), entriesGot[INDEX_THIRD].value.end());
    string stringValue5d(entriesGot[INDEX_FORTH].value.begin(), entriesGot[INDEX_FORTH].value.end());
    string stringValue6d(entriesGot[INDEX_FIFTH].value.begin(), entriesGot[INDEX_FIFTH].value.end());
    string stringValue7d(entriesGot[INDEX_SIXTH].value.begin(), entriesGot[INDEX_SIXTH].value.end());
    EXPECT_NE(stringValue1d.find("\"field1\":\"json\""), std::string::npos);
    EXPECT_NE(stringValue2d.find("\"field3\":0"), std::string::npos);
    EXPECT_NE(stringValue3d.find("\"field4\":true"), std::string::npos);
    EXPECT_NE(stringValue4d.find("\"field5\":2021456"), std::string::npos);
    EXPECT_NE(stringValue5d.find("\"field6\":2021.2"), std::string::npos);
    EXPECT_NE(stringValue6d.find("\"field1\":\"json\""), std::string::npos);
    EXPECT_NE(stringValue6d.find("\"field4\":true"), std::string::npos);
    EXPECT_NE(stringValue6d.find("\"field6\":2021.2"), std::string::npos);
    EXPECT_NE(stringValue7d.find("\"field1\":\"json\""), std::string::npos);
    EXPECT_NE(stringValue7d.find("\"field3\":0"), std::string::npos);
    EXPECT_NE(stringValue7d.find("\"field4\":true"), std::string::npos);
    EXPECT_NE(stringValue7d.find("\"field5\":2021456"), std::string::npos);
    EXPECT_NE(stringValue7d.find("\"field6\":2021.2"), std::string::npos);
}

/**
 * @tc.name: SchemaNotNullDefault 003
 * @tc.desc: Verify that schema db has the field that with not-null and default Attribute
 *    can insert the value field of which are not exist
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, SchemaNotNullDefault003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create db with valid SCHEMA_DEFINE_NNULL_DEFAULT, strict mode and version 1.0.
     * @tc.expected: step1. create successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_NNULL_DEFAULT, VALID_INDEX_1);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    InsertVoidFieldSteps(g_nbSchemaUpgradeDelegate, OK);

    /**
     * @tc.steps: step9. check the data in db.
     * @tc.expected: step9. can find all the Entries and all the field which have no value when it inserted
     *    has default value in db.
     */
    CheckSchemaVoidFieldDefaultResult();

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

/**
 * @tc.name: SchemaNotNullDefault 004
 * @tc.desc: Verify that schema db has the field that with not-null but no default value Attribute
 *    can normally insert the value has the right value
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, SchemaNotNullDefault004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create db with valid SCHEMA_DEFINE_NNULL, strict mode and version 1.0.
     * @tc.expected: step1. create successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_NNULL, VALID_INDEX_1);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    /**
     * @tc.steps: step2. insert valid value to DB.
     * @tc.expected: step2. insert succeed.
     */
    string stringValue = "{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value(stringValue.begin(), stringValue.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_1, value), OK);

    /**
     * @tc.steps: step3. check the data in db.
     * @tc.expected: step3. can find value in db.
     */
    Query query = Query::Select().EqualTo("$.field2.field3", 123); // 123 is the value of field3
    vector<Entry> entriesGot;
    vector<Entry> entries = {{KEY_1, value}};
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->GetEntries(query, entriesGot), OK);
    EXPECT_TRUE(CompareEntriesVector(entriesGot, entries));

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

/**
 * @tc.name: SchemaNotNullDefault 005
 * @tc.desc: Verify that schema db has the field that with not-null but no default value Attribute
 *    can't insert the value field of which is null
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, SchemaNotNullDefault005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create db with valid SCHEMA_DEFINE_NNULL, strict mode and version 1.0.
     * @tc.expected: step1. create successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_NNULL, VALID_INDEX_1);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    InsertFieldWithNullValue(g_nbSchemaUpgradeDelegate, CONSTRAIN_VIOLATION);

    /**
     * @tc.steps: step8. check the data in db.
     * @tc.expected: step8. can't find any records in db.
     */
    vector<Entry> entriesGot;
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->GetEntries(KEY_EMPTY, entriesGot), NOT_FOUND);

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

/**
 * @tc.name: SchemaNotNullDefault 006
 * @tc.desc: Verify that schema db has the field that with not-null and default Attribute
 *    can insert the value field of which hasn't value
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, SchemaNotNullDefault006, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create db with valid SCHEMA_DEFINE_NNULL, strict mode and version 1.0.
     * @tc.expected: step1. create successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_NNULL, VALID_INDEX_1);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    InsertVoidFieldSteps(g_nbSchemaUpgradeDelegate, CONSTRAIN_VIOLATION);

    /**
     * @tc.steps: step9. check the data in db.
     * @tc.expected: step9. can't find any data in db
     */
    vector<Entry> entriesGot;
    EXPECT_TRUE(g_nbSchemaUpgradeDelegate->GetEntries(KEY_EMPTY, entriesGot) == NOT_FOUND);

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

/**
 * @tc.name: SchemaNotNullDefault 007
 * @tc.desc: Verify that schema db has the field that has default value but without not-null Attribute
 *    can normally insert the value has the right value
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, SchemaNotNullDefault007, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create db with valid SCHEMA_DEFINE_DEFAULT, strict mode and version 1.0.
     * @tc.expected: step1. create successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_DEFAULT, VALID_INDEX_1);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    /**
     * @tc.steps: step2. insert valid value to DB.
     * @tc.expected: step2. insert succeed.
     */
    string stringValue = "{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value(stringValue.begin(), stringValue.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_1, value), OK);

    /**
     * @tc.steps: step3. check the data in db.
     * @tc.expected: step3. can find value in db.
     */
    Query query = Query::Select().EqualTo("$.field2.field4", false);
    vector<Entry> entriesGot;
    vector<Entry> entries = {{KEY_1, value}};
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->GetEntries(query, entriesGot), OK);
    EXPECT_TRUE(CompareEntriesVector(entriesGot, entries));

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

void CheckNullFieldSucceed(KvStoreNbDelegate *&delegate)
{
    InsertFieldWithNullValue(g_nbSchemaUpgradeDelegate, OK);
    /**
     * @tc.steps: step8. check the data in db.
     * @tc.expected: step8. can find all the records that insert in db, and the value corresponding field is null.
     */
    vector<Entry> entriesGot;
    EXPECT_EQ(delegate->GetEntries(KEY_EMPTY, entriesGot), OK);
    sort(entriesGot.begin(), entriesGot.end(), DistributedTestTools::CompareKey);
    string stringValue1n(entriesGot[INDEX_ZEROTH].value.begin(), entriesGot[INDEX_ZEROTH].value.end());
    string stringValue2n(entriesGot[INDEX_FIRST].value.begin(), entriesGot[INDEX_FIRST].value.end());
    string stringValue3n(entriesGot[INDEX_SECOND].value.begin(), entriesGot[INDEX_SECOND].value.end());
    string stringValue4n(entriesGot[INDEX_THIRD].value.begin(), entriesGot[INDEX_THIRD].value.end());
    string stringValue5n(entriesGot[INDEX_FORTH].value.begin(), entriesGot[INDEX_FORTH].value.end());
    string stringValue6n(entriesGot[INDEX_FIFTH].value.begin(), entriesGot[INDEX_FIFTH].value.end());
    EXPECT_NE(stringValue1n.find("\"field1\":null"), std::string::npos);
    EXPECT_NE(stringValue2n.find("\"field3\":null"), std::string::npos);
    EXPECT_NE(stringValue3n.find("\"field4\":null"), std::string::npos);
    EXPECT_NE(stringValue4n.find("\"field5\":null"), std::string::npos);
    EXPECT_NE(stringValue5n.find("\"field6\":null"), std::string::npos);
    EXPECT_NE(stringValue6n.find("\"field1\":null"), std::string::npos);
    EXPECT_NE(stringValue6n.find("\"field3\":null"), std::string::npos);
    EXPECT_NE(stringValue6n.find("\"field4\":null"), std::string::npos);
    EXPECT_NE(stringValue6n.find("\"field5\":null"), std::string::npos);
    EXPECT_NE(stringValue6n.find("\"field6\":null"), std::string::npos);
}

/**
 * @tc.name: SchemaNotNullDefault 008
 * @tc.desc: Verify that schema db has the field that has default value but without not-null Attribute
 *    can also insert the value field of which is null
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, SchemaNotNullDefault008, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create db with valid SCHEMA_DEFINE_DEFAULT, strict mode and version 1.0.
     * @tc.expected: step1. create successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_DEFAULT, VALID_INDEX_1);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    CheckNullFieldSucceed(g_nbSchemaUpgradeDelegate);

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

/**
 * @tc.name: SchemaNotNullDefault 009
 * @tc.desc: Verify that schema db has the field that has default value but without not-null Attribute
 *    can insert the value field of which hasn't value
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, SchemaNotNullDefault009, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create db with valid SCHEMA_DEFINE_DEFAULT, strict mode and version 1.0.
     * @tc.expected: step1. create successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_DEFAULT, VALID_INDEX_1);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    InsertVoidFieldSteps(g_nbSchemaUpgradeDelegate, OK);

    /**
     * @tc.steps: step9. check the data in db.
     * @tc.expected: step9. can find all the Entries and all the field which have no value when it inserted
     *    has default value in db.
     */
    CheckSchemaVoidFieldDefaultResult();

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

/**
 * @tc.name: SchemaNotNullDefault 010
 * @tc.desc: Verify that schema db has the field that hasn't default value and not-null Attribute
 *    can normally insert the value has the right value
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, SchemaNotNullDefault010, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create db with valid SCHEMA_DEFINE_NON_ATTRI, strict mode and version 1.0.
     * @tc.expected: step1. create successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_NON_ATTRI, VALID_INDEX_1);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    /**
     * @tc.steps: step2. insert valid value to DB.
     * @tc.expected: step2. insert succeed.
     */
    string stringValue = "{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value(stringValue.begin(), stringValue.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_1, value), OK);

    /**
     * @tc.steps: step3. check the data in db.
     * @tc.expected: step3. can find value in db.
     */
    Query query = Query::Select().EqualTo("$.field2.field1.field5", 123567); // 123567 is the value of field5
    vector<Entry> entriesGot;
    vector<Entry> entries = {{KEY_1, value}};
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->GetEntries(query, entriesGot), OK);
    EXPECT_TRUE(CompareEntriesVector(entriesGot, entries));

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

/**
 * @tc.name: SchemaNotNullDefault 011
 * @tc.desc: Verify that schema db has the field that hasn't default value and not-null Attribute
 *    can also insert the value field of which is null
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, SchemaNotNullDefault011, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create db with valid SCHEMA_DEFINE_NON_ATTRI, strict mode and version 1.0.
     * @tc.expected: step1. create successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_NON_ATTRI, VALID_INDEX_1);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    CheckNullFieldSucceed(g_nbSchemaUpgradeDelegate);

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

/**
 * @tc.name: SchemaNotNullDefault 012
 * @tc.desc: Verify that schema db has the field that hasn't default value and not-null Attribute
 *    can insert the value field of which hasn't value
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, SchemaNotNullDefault012, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create db with valid SCHEMA_DEFINE_NON_ATTRI, strict mode and version 1.0.
     * @tc.expected: step1. create successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_NON_ATTRI, VALID_INDEX_1);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    InsertVoidFieldSteps(g_nbSchemaUpgradeDelegate, OK);

    /**
     * @tc.steps: step9. check the data in db.
     * @tc.expected: step9. can find all the Entries and all the value hasn't the field which is void when it inserted
     */
    vector<Entry> entriesGot;
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->GetEntries(KEY_EMPTY, entriesGot), OK);
    sort(entriesGot.begin(), entriesGot.end(), DistributedTestTools::CompareKey);
    string stringValue1v(entriesGot[INDEX_ZEROTH].value.begin(), entriesGot[INDEX_ZEROTH].value.end());
    string stringValue2v(entriesGot[INDEX_FIRST].value.begin(), entriesGot[INDEX_FIRST].value.end());
    string stringValue3v(entriesGot[INDEX_SECOND].value.begin(), entriesGot[INDEX_SECOND].value.end());
    string stringValue4v(entriesGot[INDEX_THIRD].value.begin(), entriesGot[INDEX_THIRD].value.end());
    string stringValue5v(entriesGot[INDEX_FORTH].value.begin(), entriesGot[INDEX_FORTH].value.end());
    string stringValue6v(entriesGot[INDEX_FIFTH].value.begin(), entriesGot[INDEX_FIFTH].value.end());
    string stringValue7v(entriesGot[INDEX_SIXTH].value.begin(), entriesGot[INDEX_SIXTH].value.end());
    EXPECT_EQ(stringValue1v.find("\"field1\":\"json\""), std::string::npos);
    EXPECT_EQ(stringValue2v.find("\"field3\":0"), std::string::npos);
    EXPECT_EQ(stringValue3v.find("\"field4\":true"), std::string::npos);
    EXPECT_EQ(stringValue4v.find("\"field5\":2021456"), std::string::npos);
    EXPECT_EQ(stringValue5v.find("\"field6\":2021.2"), std::string::npos);
    EXPECT_EQ(stringValue6v.find("\"field1\":\"json\""), std::string::npos);
    EXPECT_EQ(stringValue6v.find("\"field4\":true"), std::string::npos);
    EXPECT_EQ(stringValue6v.find("\"field6\":2021.2"), std::string::npos);
    EXPECT_EQ(stringValue7v.find("\"field1\":\"json\""), std::string::npos);
    EXPECT_EQ(stringValue7v.find("\"field3\":0"), std::string::npos);
    EXPECT_EQ(stringValue7v.find("\"field4\":true"), std::string::npos);
    EXPECT_EQ(stringValue7v.find("\"field5\":2021456"), std::string::npos);
    EXPECT_EQ(stringValue7v.find("\"field6\":2021.2"), std::string::npos);

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

/**
 * @tc.name: UpgradeKv 001
 * @tc.desc: Verify that old version db that has the schema format record, and the db can be opened with the
 *    corresponding schema-define which has not-null attribute and default value, and at the same time upgrade the db.
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, UpgradeKv001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create old version db and insert two records one of which has all the
     *    SCHEMA_DEFINE_NNULL_DEFAULT format value the other one only has the SCHEMA_DEFINE_NNULL_DEFAULT
     *    and all the field are void.
     * @tc.expected: step1. create and insert successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    string schemaString1 = "{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value1(schemaString1.begin(), schemaString1.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_1, value1), OK);
    string schemaString2 = "{\"field2\":{\"field1\":{\"field2\":{}}}}";
    Value value2(schemaString2.begin(), schemaString2.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_2, value2), OK);
    /**
     * @tc.steps: step2. close the old db.
     * @tc.expected: step2. close successfully.
     */
    EXPECT_TRUE(DistributedDBNbTestTools::CloseNbAndRelease(g_manager, g_nbSchemaUpgradeDelegate));

    /**
     * @tc.steps: step3. reopen the db again using the schema define option.
     * @tc.expected: step3. create successfully.
     */
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_NNULL_DEFAULT);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    /**
     * @tc.steps: step4. check the data in db with the the query EqualTo("field1","schema").Or().EqualTo("field2.
     *    field4",false).
     * @tc.expected: step4. can find (k1, value1)(k2, value2) and value2 is filt with default value.
     */
    Query query = Query::Select().EqualTo("$.field1", "schema").Or().EqualTo("field2.field4", true);
    vector<Entry> entriesGot;
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->GetEntries(query, entriesGot), OK);
    sort(entriesGot.begin(), entriesGot.end(), DistributedTestTools::CompareKey);
    string stringValue2d(entriesGot[INDEX_FIRST].value.begin(), entriesGot[INDEX_FIRST].value.end());
    EXPECT_EQ(entriesGot[INDEX_ZEROTH].value, value1);
    EXPECT_NE(stringValue2d.find("\"field1\":\"json\""), std::string::npos);
    EXPECT_NE(stringValue2d.find("\"field3\":0"), std::string::npos);
    EXPECT_NE(stringValue2d.find("\"field4\":true"), std::string::npos);
    EXPECT_NE(stringValue2d.find("\"field5\":2021456"), std::string::npos);
    EXPECT_NE(stringValue2d.find("\"field6\":2021.2"), std::string::npos);

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

/**
 * @tc.name: UpgradeKv 002
 * @tc.desc: Verify that old version db that has the schema format record, and the db can be opened with the
 *    corresponding schema-define which has not-null attribute but no default value, and upgrade the db.
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, UpgradeKv002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create old version db and insert one record which has all the SCHEMA_DEFINE_NNULL format value.
     * @tc.expected: step1. create and insert successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    string schemaString = "{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value(schemaString.begin(), schemaString.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_1, value), OK);
    /**
     * @tc.steps: step2. close the old db.
     * @tc.expected: step2. close successfully.
     */
    EXPECT_TRUE(DistributedDBNbTestTools::CloseNbAndRelease(g_manager, g_nbSchemaUpgradeDelegate));

    /**
     * @tc.steps: step3. reopen the db again using the schema define option.
     * @tc.expected: step3. create successfully.
     */
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_NNULL);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    /**
     * @tc.steps: step4. check the data in db with the the query LessThan("field2.field1.field2.field6",0).
     * @tc.expected: step4. can find (k1, value1)(k2, value2) and value2 is filt with default value.
     */
    Query query = Query::Select().LessThan("field2.field1.field2.field6", 0);
    vector<Entry> entriesGot;
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->GetEntries(query, entriesGot), OK);
    EXPECT_EQ(entriesGot[INDEX_ZEROTH].value, value);

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

/**
 * @tc.name: UpgradeKv 003
 * @tc.desc: Verify that old version db that has the schema format record, and the db can be opened with the
 *    corresponding schema-define without not-null attribute but has default value, and upgrade the db.
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, UpgradeKv003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create old version db and insert three records one of which has all the SCHEMA_DEFINE_DEFAULT
     *    format value the second one has the void field6, and the third one only has the SCHEMA_DEFINE_DEFAULT format
     *    and all the field are void.
     * @tc.expected: step1. create and insert successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    string schemaString1 = "{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value1(schemaString1.begin(), schemaString1.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_1, value1), OK);
    string schemaString2 = "{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":null}}}}";
    Value value2(schemaString2.begin(), schemaString2.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_2, value2), OK);
    string schemaString3 = "{\"field2\":{\"field1\":{\"field2\":{}}}}";
    Value value3(schemaString3.begin(), schemaString3.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_3, value3), OK);
    /**
     * @tc.steps: step2. close the old db.
     * @tc.expected: step2. close successfully.
     */
    EXPECT_TRUE(DistributedDBNbTestTools::CloseNbAndRelease(g_manager, g_nbSchemaUpgradeDelegate));

    /**
     * @tc.steps: step3. reopen the db again using the schema define option.
     * @tc.expected: step3. create successfully.
     */
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_DEFAULT);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    /**
     * @tc.steps: step4. check the data in db with the the query EqualTo("field2.field3", 123).Or().
     *    EqualTo("field2.field1.field2.field6", 2021.2).
     * @tc.expected: step4. can find (k1, value1)(k2, value2)(k3, value3) and value2 and value3 are
     *    filt with default value.
     */
    Query query = Query::Select().EqualTo("field2.field3", 123).Or().EqualTo("field2.field1.field2.field6", 2021.2);
    vector<Entry> entriesGot;
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->GetEntries(query, entriesGot), OK);
    EXPECT_EQ(entriesGot[INDEX_ZEROTH].value, value1);
    EXPECT_EQ(entriesGot[INDEX_FIRST].value, value2);
    string stringValue3d(entriesGot[INDEX_SECOND].value.begin(), entriesGot[INDEX_SECOND].value.end());
    EXPECT_NE(stringValue3d.find("\"field5\":2021456"), std::string::npos);

    /**
     * @tc.steps: step5. delete (k2, value2), update (k3, value3) to (k3, value2) and use
     *    GetResultSet(query=IsNotNull("field1")) interface to check the data in db.
     * @tc.expected: step5. delete and update succeed and can only find (k1, value1) (k3, value2) in db.
     */
    EXPECT_EQ(DistributedDBNbTestTools::Delete(*g_nbSchemaUpgradeDelegate, KEY_2), OK);
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_3, value2), OK);
    KvStoreResultSet *resultSet = nullptr;
    Query query2 = Query::Select().IsNotNull("field1");
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->GetEntries(query2, resultSet), OK);
    EXPECT_EQ(resultSet->GetCount(), 2); // 2 records
    Entry entry;
    EXPECT_EQ(resultSet->MoveToNext(), true);
    EXPECT_EQ(resultSet->GetEntry(entry), OK);
    EXPECT_EQ(entry.value, value1);
    EXPECT_EQ(resultSet->MoveToNext(), true);
    EXPECT_EQ(resultSet->GetEntry(entry), OK);
    EXPECT_EQ(entry.value, value2);
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->CloseResultSet(resultSet), OK);
    resultSet = nullptr;

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

/**
 * @tc.name: UpgradeKv 004
 * @tc.desc: Verify that old version db that has the schema format record, and the db can be opened with the
 *    corresponding schema-define without not-null attribute and default value, and upgrade the db.
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, UpgradeKv004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create old version db and insert three records one of which has all the SCHEMA_DEFINE_NON_ATTRI
     *    format value the second one has the void field6, and the third one only has the SCHEMA_DEFINE_DEFAULT format
     *    and all the field are void.
     * @tc.expected: step1. create and insert successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    string schemaString1 = "{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value1(schemaString1.begin(), schemaString1.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_1, value1), OK);
    string schemaString2 = "{\"field1\":null,\"field2\":{\"field3\":null,\"field4\":null,"
        "\"field1\":{\"field5\":null,\"field2\":{\"field6\":null}}}}";
    Value value2(schemaString2.begin(), schemaString2.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_2, value2), OK);
    string schemaString3 = "{\"field2\":{\"field1\":{\"field2\":{}}}}";
    Value value3(schemaString3.begin(), schemaString3.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_3, value3), OK);
    /**
     * @tc.steps: step2. close the old db.
     * @tc.expected: step2. close successfully.
     */
    EXPECT_TRUE(DistributedDBNbTestTools::CloseNbAndRelease(g_manager, g_nbSchemaUpgradeDelegate));

    /**
     * @tc.steps: step3. reopen the db again using the schema define option.
     * @tc.expected: step3. create successfully.
     */
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_NON_ATTRI);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    /**
     * @tc.steps: step4. check the data in db.
     * @tc.expected: step4. can find (k1, value1)(k2, value2)(k3, value3).
     */
    vector<Entry> entriesGot;
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->GetEntries(KEY_EMPTY, entriesGot), OK);
    EXPECT_EQ(entriesGot[INDEX_ZEROTH].value, value1);
    EXPECT_EQ(entriesGot[INDEX_FIRST].value, value2);
    EXPECT_EQ(entriesGot[INDEX_SECOND].value, value3);

    /**
     * @tc.steps: step5. delete (k3, value3), and insert (k3, value1) again and then use
     *    GetResultSet(query=IsNotNull("field1")) interface to check the data in db.
     * @tc.expected: step5. delete and insert succeed and can find (k1, value1) (k2, value2) (k3, value1) in db.
     */
    EXPECT_EQ(DistributedDBNbTestTools::Delete(*g_nbSchemaUpgradeDelegate, KEY_3), OK);
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_3, value1), OK);
    KvStoreResultSet *resultSet = nullptr;
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->GetEntries(KEY_EMPTY, resultSet), OK);
    Entry entry;
    EXPECT_EQ(resultSet->MoveToNext(), true);
    EXPECT_EQ(resultSet->GetEntry(entry), OK);
    EXPECT_EQ(entry.value, value1);
    EXPECT_EQ(resultSet->MoveToNext(), true);
    EXPECT_EQ(resultSet->GetEntry(entry), OK);
    EXPECT_EQ(entry.value, value2);
    EXPECT_EQ(resultSet->MoveToNext(), true);
    EXPECT_EQ(resultSet->GetEntry(entry), OK);
    EXPECT_EQ(entry.value, value1);
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->CloseResultSet(resultSet), OK);

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

/**
 * @tc.name: UpgradeKv 005
 * @tc.desc: Verify that old version db that has the schema format record, and the db can't opened if the schema-define
 *    format is not the same as the value already exist.
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, UpgradeKv005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create old version db and insert one record which has all valid field value
     * @tc.expected: step1. create and insert successfully.
     */
    KvStoreDelegateManager *manager = nullptr;
    Option option1 = g_option;
    option1.isMemoryDb = false;
    KvStoreNbDelegate *delegate = DistributedDBNbTestTools::GetNbDelegateSuccess(manager, g_dbParameter1, option1);
    ASSERT_TRUE(manager != nullptr && delegate != nullptr);

    string schemaString1 = "{\"field1\":null,\"field2\":{\"field3\":null,\"field4\":null,"
        "\"field1\":{\"field5\":null,\"field2\":{\"field6\":null}}}}";
    Value value1(schemaString1.begin(), schemaString1.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_1, value1), OK);
    /**
     * @tc.steps: step2. close the old db.
     * @tc.expected: step2. close successfully.
     */
    EXPECT_TRUE(DistributedDBNbTestTools::CloseNbAndRelease(manager, delegate));
    /**
     * @tc.steps: step3. reopen the db again using the SCHEMA_DEFINE_NNULL_DEFAULT as option.schema.
     * @tc.expected: step3. reopen failed.
     */
    Option option2 = g_option;
    option2.isMemoryDb = false;
    option2.createIfNecessary = false;
    option2.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_NNULL_DEFAULT);
    DBStatus status;
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateStatus(g_manager, status, g_dbParameter1,
        option2);
    ASSERT_TRUE(g_manager == nullptr && g_nbSchemaUpgradeDelegate == nullptr);
    EXPECT_EQ(status, SCHEMA_VIOLATE_VALUE);

    /**
     * @tc.steps: step4. reopen db using old version and delete (k1, value1), and insert (k2, value2) field1 of
     *    which is null
     * @tc.expected: step4. operate successfully.
     */
    option1.createIfNecessary = false;
    delegate = DistributedDBNbTestTools::GetNbDelegateSuccess(manager, g_dbParameter1, option1);
    ASSERT_TRUE(manager != nullptr && delegate != nullptr);

    EXPECT_EQ(DistributedDBNbTestTools::Delete(*delegate, KEY_1), OK);
    string schemaString2 = "{\"field1\":null,\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value2(schemaString2.begin(), schemaString2.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_2, value2), OK);

    /**
     * @tc.steps: step5. close the old db.
     * @tc.expected: step5. close successfully.
     */
    EXPECT_TRUE(DistributedDBNbTestTools::CloseNbAndRelease(manager, delegate));
    /**
     * @tc.steps: step6. reopen the db again using the SCHEMA_DEFINE_NNULL as option.schema.
     * @tc.expected: step6. reopen failed.
     */
    option2.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_2, SCHEMA_DEFINE_NNULL);
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateStatus(g_manager, status, g_dbParameter1,
        option2);
    ASSERT_TRUE(g_manager == nullptr && g_nbSchemaUpgradeDelegate == nullptr);
    EXPECT_EQ(status, SCHEMA_VIOLATE_VALUE);

    /**
     * @tc.steps: step7. reopen db using old version and delete (k2, value2), and insert (k3, value3) all field of
     *    which is null
     * @tc.expected: step7. operate successfully.
     */
    delegate = DistributedDBNbTestTools::GetNbDelegateSuccess(manager, g_dbParameter1, option1);
    ASSERT_TRUE(manager != nullptr && delegate != nullptr);

    EXPECT_EQ(DistributedDBNbTestTools::Delete(*delegate, KEY_2), OK);
    string schemaString3 = "{\"field2\":{\"field1\":{\"field2\":{}}}}";
    Value value3(schemaString3.begin(), schemaString3.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_3, value3), OK);

    /**
     * @tc.steps: step5. close the old db.
     * @tc.expected: step5. close successfully.
     */
    EXPECT_TRUE(manager->CloseKvStore(delegate) == OK);

    /**
     * @tc.steps: step6. reopen the db again using the SCHEMA_DEFINE_NNULL as option.schema.
     * @tc.expected: step6. reopen failed.
     */
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateStatus(g_manager, status, g_dbParameter1,
        option2);
    ASSERT_TRUE(g_manager == nullptr && g_nbSchemaUpgradeDelegate == nullptr);
    EXPECT_EQ(status, SCHEMA_VIOLATE_VALUE);

    EXPECT_TRUE(manager->DeleteKvStore(STORE_ID_1) == OK);
    delete manager;
    manager = nullptr;
}

void CheckAddNewFieldSucceed(KvStoreNbDelegate *&delegate)
{
    /**
     * @tc.steps: step8. check the data in the new version db again.
     * @tc.expected: step8. can find value1, value2 and value3 in db also.
     */
    KvStoreResultSet *resultSet = nullptr;
    Query query = Query::Select().GreaterThanOrEqualTo("field2.field3", 0).OrderBy("$.field1", true);
    EXPECT_EQ(delegate->GetEntries(query, resultSet), OK);
    EXPECT_EQ(resultSet->GetCount(), 3); // 3 records
    vector<Entry> entriesGot;
    Entry entry;
    EXPECT_EQ(resultSet->MoveToNext(), true);
    EXPECT_EQ(resultSet->GetEntry(entry), OK);
    entriesGot.push_back(entry);
    EXPECT_EQ(resultSet->MoveToNext(), true);
    EXPECT_EQ(resultSet->GetEntry(entry), OK);
    entriesGot.push_back(entry);
    EXPECT_EQ(resultSet->MoveToNext(), true);
    EXPECT_EQ(resultSet->GetEntry(entry), OK);
    entriesGot.push_back(entry);
    sort(entriesGot.begin(), entriesGot.end(), DistributedTestTools::CompareKey);

    string stringValue1u(entriesGot[INDEX_ZEROTH].value.begin(), entriesGot[INDEX_ZEROTH].value.end());
    EXPECT_NE(stringValue1u.find("\"field1\":\"schema\""), std::string::npos);
    EXPECT_NE(stringValue1u.find("\"field_add2\":100"), std::string::npos);
    EXPECT_NE(stringValue1u.find("\"field_add3\":false"), std::string::npos);

    string stringValue2u(entriesGot[INDEX_FIRST].value.begin(), entriesGot[INDEX_FIRST].value.end());
    EXPECT_NE(stringValue2u.find("\"field5\":2021456"), std::string::npos);
    EXPECT_NE(stringValue2u.find("\"field_add2\":100"), std::string::npos);
    EXPECT_NE(stringValue2u.find("\"field_add3\":false"), std::string::npos);

    string stringValue3u(entriesGot[INDEX_SECOND].value.begin(), entriesGot[INDEX_SECOND].value.end());
    EXPECT_NE(stringValue3u.find("\"field1\":\"json\""), std::string::npos);
    EXPECT_NE(stringValue3u.find("\"field_add1\":\"new field1\""), std::string::npos);
    EXPECT_NE(stringValue3u.find("\"field_add4\":[\"field1\",\"field2.field3\"]"), std::string::npos);

    EXPECT_EQ(g_nbSchemaUpgradeDelegate->CloseResultSet(resultSet), OK);
    resultSet = nullptr;
}

/**
 * @tc.name: UpgradeField 001
 * @tc.desc: Verify that the compatible mode schema db support to add field and change schema-index, and the new field
 *    can be null or must has default value if can't be null, and can be in any nesting level.
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, UpgradeField001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create db with valid SCHEMA_DEFINE_NNULL_DEFAULT, compatible mode, version 1.0, valid
     *    schema_index and schema_skipsize 1.
     * @tc.expected: step1. create successfully.
     */
    Option option = g_option;
    option.isMemoryDb = false;
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_2, SCHEMA_DEFINE_NNULL_DEFAULT,
        SCHMEA_INDEX_WITH_ONE_JOIN_INDEX, "1");
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    /**
     * @tc.steps: step2. insert valid value1 to DB all field of which has valid value.
     * @tc.expected: step2. insert succeed.
     */
    string stringValue1 = "a{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value1(stringValue1.begin(), stringValue1.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_1, value1), OK);

    /**
     * @tc.steps: step3. insert valid value2 to DB all field has no value.
     * @tc.expected: step3. insert succeed.
     */
    string stringValue2 = "x{\"field2\":{\"field1\":{\"field2\":{}}}}";
    Value value2(stringValue2.begin(), stringValue2.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_2, value2), OK);

    /**
     * @tc.steps: step4. check the data in db.
     * @tc.expected: step4. can find value1 and value2 in db.
     */
    vector<Entry> entriesGot;
    EXPECT_EQ(g_nbSchemaUpgradeDelegate->GetEntries(KEY_EMPTY, entriesGot), OK);
    ASSERT_EQ(static_cast<int>(entriesGot.size()), 2); // 2 records
    EXPECT_EQ(entriesGot[INDEX_ZEROTH].value, value1);
    string stringValue2n(entriesGot[INDEX_FIRST].value.begin(), entriesGot[INDEX_FIRST].value.end());
    EXPECT_NE(stringValue2n.find("\"field6\":2021.2"), std::string::npos);

    /**
     * @tc.steps: step5. close the db but not delete it.
     * @tc.expected: step5. close succeed.
     */
    EXPECT_TRUE(DistributedDBNbTestTools::CloseNbAndRelease(g_manager, g_nbSchemaUpgradeDelegate));

    /**
     * @tc.steps: step6. reopen the db with a new schema-define that include new field one of which hasn't not-null
     *    attribute and default value, the other one has default field value but hasn't not-null attribute, the next
     *    one has also not-null attribute and default field value, the last one has void field array,
     *    and the upgrade schema_index
     * @tc.expected: step6. reopen succeed.
     */
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_2, COMPATIBLE_SCHEMA_DEFINE,
        SCHEMA_INDEX_WITH_MULTI_JOIN_INDEXS, "1");
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateSuccess(g_manager, g_dbParameter1, option);
    ASSERT_TRUE(g_manager != nullptr && g_nbSchemaUpgradeDelegate != nullptr);

    /**
     * @tc.steps: step7. insert valid value3 to DB that include all the new field added.
     * @tc.expected: step7. insert succeed.
     */
    string stringValue3 = "c{\"field2\":{\"field3\":123,\"field4\":false,\"field1\":{\"field5\":123567,"
        "\"field2\":{\"field_add1\":\"new field1\"}}},\"field_add4\":[\"field1\",\"field2.field3\"]}";
    Value value3(stringValue3.begin(), stringValue3.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*g_nbSchemaUpgradeDelegate, KEY_3, value3), OK);

    CheckAddNewFieldSucceed(g_nbSchemaUpgradeDelegate);

    EXPECT_TRUE(EndCaseDeleteDB(g_manager, g_nbSchemaUpgradeDelegate, STORE_ID_1, option.isMemoryDb));
}

/**
 * @tc.name: UpgradeField 002
 * @tc.desc: Verify that the strict mode schema db don't support to add field.
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, UpgradeField002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create db with valid SCHEMA_DEFINE_NON_ATTRI, strict mode, version 1.0, valid
     *    schema_index and schema_skipsize 1.
     * @tc.expected: step1. create successfully.
     */
    KvStoreDelegateManager *manager = nullptr;
    Option option = g_option;
    option.isMemoryDb = false;
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, SCHEMA_DEFINE_NON_ATTRI,
        SCHMEA_INDEX_WITH_ONE_JOIN_INDEX, "1");
    KvStoreNbDelegate *delegate = DistributedDBNbTestTools::GetNbDelegateSuccess(manager, g_dbParameter1, option);
    ASSERT_TRUE(manager != nullptr && delegate != nullptr);

    /**
     * @tc.steps: step2. insert valid value1 to DB all field of which has valid value.
     * @tc.expected: step2. insert succeed.
     */
    string stringValue1 = "a{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value1(stringValue1.begin(), stringValue1.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_1, value1), OK);

    /**
     * @tc.steps: step3. insert valid value2 to DB all field has no value.
     * @tc.expected: step3. insert succeed.
     */
    string stringValue2 = "b{\"field2\":{\"field1\":{\"field2\":{}}}}";
    Value value2(stringValue2.begin(), stringValue2.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_2, value2), OK);

    /**
     * @tc.steps: step4. insert valid value3 to DB all field is null.
     * @tc.expected: step4. insert succeed.
     */
    string stringValue3 = "c{\"field1\":null,\"field2\":{\"field3\":null,\"field4\":null,"
        "\"field1\":{\"field5\":null,\"field2\":{\"field6\":null}}}}";
    Value value3(stringValue3.begin(), stringValue3.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_3, value3), OK);

    /**
     * @tc.steps: step4. check the data in db.
     * @tc.expected: step4. can find value1, value2 and value3 in db.
     */
    vector<Entry> entriesGot;
    EXPECT_EQ(delegate->GetEntries(KEY_EMPTY, entriesGot), OK);
    ASSERT_EQ(static_cast<int>(entriesGot.size()), 3); // 3 records
    string stringValue1n(entriesGot[INDEX_ZEROTH].value.begin(), entriesGot[INDEX_ZEROTH].value.end());
    string stringValue2n(entriesGot[INDEX_FIRST].value.begin(), entriesGot[INDEX_FIRST].value.end());
    string stringValue3n(entriesGot[INDEX_SECOND].value.begin(), entriesGot[INDEX_SECOND].value.end());
    EXPECT_NE(stringValue1n.find("\"field1\":\"schema\""), std::string::npos);
    EXPECT_EQ(stringValue2n, stringValue2);
    EXPECT_NE(stringValue3n.find("\"field1\":null"), std::string::npos);
    EXPECT_NE(stringValue3n.find("\"field4\":null"), std::string::npos);

    /**
     * @tc.steps: step5. close the db but not delete it.
     * @tc.expected: step5. close succeed.
     */
    EXPECT_TRUE(manager->CloseKvStore(delegate) == OK);
    delegate = nullptr;
    /**
     * @tc.steps: step6. reopen the db with a new schema-define that include new field field_add1.
     * @tc.expected: step6. reopen failed.
     */
    std::string schemaDefineUpdate = "{\"field1\":\"STRING\",\"field2\":{\"field3\":\"INTEGER\",\"field4\":\"BOOL\","
        "\"field1\":{\"field5\":\"LONG\",\"field2\":{\"field6\":\"DOUBLE\"}}},\"field_add1\":\"STRING\"}";
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_1, schemaDefineUpdate,
        SCHMEA_INDEX_WITH_ONE_JOIN_INDEX, "1");
    DBStatus status;
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateStatus(g_manager, status, g_dbParameter1,
        option);
    ASSERT_TRUE(g_manager == nullptr && g_nbSchemaUpgradeDelegate == nullptr);
    EXPECT_EQ(status, SCHEMA_MISMATCH);

    EXPECT_TRUE(manager->DeleteKvStore(STORE_ID_1) == OK);
    delete manager;
    manager = nullptr;
}

void UpgradeDbWithInvalidSchemaDefine(Option &option)
{
    /**
     * @tc.steps: step6. reopen the db with a new schema-define that will add field_add1, field_add2, field_add3,
     *    field_add4 but will delete the original field field6.
     * @tc.expected: step6. reopen failed.
     */
    std::string schemaDefineUpdate1 = "{\"field1\":\"STRING,NOT NULL,DEFAULT \'json\'\",\"field2\":{\"field3\":"
        "\"INTEGER,NOT NULL,DEFAULT 0\",\"field4\":\"BOOL,NOT NULL,DEFAULT true\",\"field1\":{\"field5\":"
        "\"LONG,NOT NULL,DEFAULT 2021456\",\"field2\":{\"field_add1\":\"STRING\"},\"field_add2\":"
        "\"INTEGER,DEFAULT 100\"},\"field_add3\":\"BOOL,NOT NULL,DEFAULT false\"},\"field_add4\":[]}";
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_2, schemaDefineUpdate1,
        SCHEMA_INDEX_WITH_MULTI_JOIN_INDEXS, "1");
    DBStatus status;
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateStatus(g_manager, status, g_dbParameter1,
        option);
    EXPECT_TRUE(g_manager == nullptr && g_nbSchemaUpgradeDelegate == nullptr);
    EXPECT_EQ(status, SCHEMA_MISMATCH);

    /**
     * @tc.steps: step7. reopen the db with a new schema-define that want to change default value of the original field,
     *    and at the same time add field_add1, field_add2, field_add3, field_add4. And change the schema index.
     * @tc.expected: step7. reopen failed.
     */
    std::string schemaDefineUpdate2 = "{\"field1\":\"STRING,NOT NULL,DEFAULT \'json\'\",\"field2\":{\"field3\":"
        "\"INTEGER,NOT NULL,DEFAULT 100\",\"field4\":\"BOOL,NOT NULL,DEFAULT true\",\"field1\":{\"field5\":"
        "\"LONG,NOT NULL,DEFAULT 2021456\",\"field2\":{\"field6\":\"DOUBLE,NOT NULL,DEFAULT 2021.2\","
        "\"field_add1\":\"STRING\"},\"field_add2\":\"INTEGER,DEFAULT 100\"},\"field_add3\":"
        "\"BOOL,NOT NULL,DEFAULT false\"},\"field_add4\":[]}";
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_2, schemaDefineUpdate2,
        SCHEMA_INDEX_WITH_MULTI_JOIN_INDEXS, "1");
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateStatus(g_manager, status, g_dbParameter1,
        option);
    EXPECT_TRUE(g_manager == nullptr && g_nbSchemaUpgradeDelegate == nullptr);
    EXPECT_EQ(status, SCHEMA_MISMATCH);

    /**
     * @tc.steps: step8. reopen the db with a new schema-define that want to change not-null attribute of the original
     *    field, and at the same time add field_add1, field_add2, field_add3, field_add4 and change the schema index.
     * @tc.expected: step8. reopen failed.
     */
    std::string schemaDefineUpdate3 = "{\"field1\":\"STRING,NOT NULL,DEFAULT \'json\'\",\"field2\":{\"field3\":"
        "\"INTEGER,NOT NULL,DEFAULT 0\",\"field4\":\"BOOL,DEFAULT true\",\"field1\":{\"field5\":"
        "\"LONG,NOT NULL,DEFAULT 2021456\",\"field2\":{\"field6\":\"DOUBLE,NOT NULL,DEFAULT 2021.2\","
        "\"field_add1\":\"STRING\"},\"field_add2\":\"INTEGER,DEFAULT 100\"},\"field_add3\":"
        "\"BOOL,NOT NULL,DEFAULT false\"},\"field_add4\":[]}";
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_2, schemaDefineUpdate3,
        SCHEMA_INDEX_WITH_MULTI_JOIN_INDEXS, "1");
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateStatus(g_manager, status, g_dbParameter1,
        option);
    EXPECT_TRUE(g_manager == nullptr && g_nbSchemaUpgradeDelegate == nullptr);
    EXPECT_EQ(status, SCHEMA_MISMATCH);

    /**
     * @tc.steps: step9. reopen the db with a new schema-define that add field_add1, field_add2, field_add3, field_add4,
     *    field_add5 which has not-null attribute but hasn't default value, and change the schema index.
     * @tc.expected: step9. reopen failed.
     */
    std::string schemaDefineUpdate4 = "{\"field1\":\"STRING,NOT NULL,DEFAULT \'json\'\",\"field2\":{\"field3\":"
        "\"INTEGER,NOT NULL,DEFAULT 0\",\"field4\":\"BOOL,DEFAULT true\",\"field1\":{\"field5\":"
        "\"LONG,NOT NULL,DEFAULT 2021456\",\"field2\":{\"field6\":\"DOUBLE,NOT NULL,DEFAULT 2021.2\","
        "\"field_add1\":\"STRING\"},\"field_add2\":\"INTEGER,DEFAULT 100\"},\"field_add3\":"
        "\"BOOL,NOT NULL,DEFAULT false\"},\"field_add4\":[],\"field_add5\":\"STRING,NOT NULL\"}";
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_2, schemaDefineUpdate4,
        SCHEMA_INDEX_WITH_MULTI_JOIN_INDEXS, "1");
    g_nbSchemaUpgradeDelegate = DistributedDBNbTestTools::GetNbDelegateStatus(g_manager, status, g_dbParameter1,
        option);
    EXPECT_TRUE(g_manager == nullptr && g_nbSchemaUpgradeDelegate == nullptr);
    EXPECT_EQ(status, SCHEMA_MISMATCH);
}

/**
 * @tc.name: UpgradeField 003
 * @tc.desc: Verify that when update the compatible mode schema db, it can't change the original field and its
 *    attribute and the new added field mustn't has not-null attribute and no default value at the same time.
 * @tc.type: FUNC
 * @tc.require: SR000EUVPV
 * @tc.author: fengxiaoyun
 */
HWTEST_F(DistributeddbNbSchemaUpgradeTest, UpgradeField003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. create db with valid SCHEMA_DEFINE_NNULL_DEFAULT, strict mode, version 1.0, valid
     *    schema_index and schema_skipsize 1.
     * @tc.expected: step1. create successfully.
     */
    KvStoreDelegateManager *manager = nullptr;
    Option option = g_option;
    option.isMemoryDb = false;
    option.schema = SpliceToSchema(VALID_VERSION_1, VALID_MODE_2, SCHEMA_DEFINE_NNULL_DEFAULT,
        SCHMEA_INDEX_WITH_ONE_JOIN_INDEX, "1");
    KvStoreNbDelegate *delegate = DistributedDBNbTestTools::GetNbDelegateSuccess(manager, g_dbParameter1, option);
    ASSERT_TRUE(manager != nullptr && delegate != nullptr);

    /**
     * @tc.steps: step2. insert valid value1 to DB all field of which has valid value.
     * @tc.expected: step2. insert succeed.
     */
    string stringValue1 = "a{\"field1\":\"schema\",\"field2\":{\"field3\":123,\"field4\":false,"
        "\"field1\":{\"field5\":123567,\"field2\":{\"field6\":-123.456}}}}";
    Value value1(stringValue1.begin(), stringValue1.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_1, value1), OK);

    /**
     * @tc.steps: step3. insert valid value2 to DB all field has no value.
     * @tc.expected: step3. insert succeed.
     */
    string stringValue2 = "b{\"field2\":{\"field1\":{\"field2\":{}}}}";
    Value value2(stringValue2.begin(), stringValue2.end());
    EXPECT_EQ(DistributedDBNbTestTools::Put(*delegate, KEY_2, value2), OK);

    /**
     * @tc.steps: step4. check the data in db.
     * @tc.expected: step4. can find value1, value2 and value3 in db.
     */
    vector<Entry> entriesGot;
    EXPECT_EQ(delegate->GetEntries(KEY_EMPTY, entriesGot), OK);
    ASSERT_EQ(static_cast<int>(entriesGot.size()), 2); // 2 records
    EXPECT_EQ(entriesGot[INDEX_ZEROTH].value, value1);
    string stringValue2n(entriesGot[INDEX_FIRST].value.begin(), entriesGot[INDEX_FIRST].value.end());
    EXPECT_NE(stringValue2n.find("\"field1\":\"json\""), std::string::npos);
    EXPECT_NE(stringValue2n.find("\"field3\":0"), std::string::npos);
    EXPECT_NE(stringValue2n.find("\"field4\":true"), std::string::npos);
    EXPECT_NE(stringValue2n.find("\"field5\":2021456"), std::string::npos);
    EXPECT_NE(stringValue2n.find("\"field6\":2021.2"), std::string::npos);

    /**
     * @tc.steps: step5. close the db but not delete it.
     * @tc.expected: step5. close succeed.
     */
    EXPECT_TRUE(manager->CloseKvStore(delegate) == OK);
    delegate = nullptr;

    UpgradeDbWithInvalidSchemaDefine(option);

    EXPECT_TRUE(manager->DeleteKvStore(STORE_ID_1) == OK);
    delete manager;
    manager = nullptr;
}
} // end of namespace DistributeddbNbSchemaUpgradeDb