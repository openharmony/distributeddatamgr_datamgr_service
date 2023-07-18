/*
* Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <climits>
#include <gtest/gtest.h>

#include "documentdb_test_utils.h"
#include "grd_base/grd_db_api.h"
#include "grd_base/grd_error.h"
#include "grd_document/grd_document_api.h"

using namespace testing::ext;
using namespace DocumentDBUnitTest;

namespace {
std::string g_path = "./document.db";
GRD_DB *g_db = nullptr;
const char *RIGHT_COLLECTION_NAME = "student";
const char *NO_EXIST_COLLECTION_NAME = "no_exisit";
const int MAX_COLLECTION_LENS = 511;
const int MAX_ID_LENS = 899;

static void TestInsertDocIntoCertainColl(const char *collectionName, const char *projection, int expectedResult)
{
    /**
     * @tc.steps: step1. Create Collection
     * @tc.expected: step1. GRD_OK
    */
    EXPECT_EQ(GRD_CreateCollection(g_db, collectionName, "", 0), expectedResult);
    /**
     * @tc.steps: step2. Insert projection into colloction.
     * @tc.expected: step2. GRD_OK
     */
    EXPECT_EQ(GRD_InsertDoc(g_db, collectionName, projection, 0), expectedResult);
    /**
     * @tc.steps: step3. Call GRD_DroCollection to drop the collection.
     * @tc.expected: step3. GRD_OK
     */
    EXPECT_EQ(GRD_DropCollection(g_db, collectionName, 0), expectedResult);
}

class DocumentDBInsertTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DocumentDBInsertTest::SetUpTestCase(void)
{
    int status = GRD_DBOpen(g_path.c_str(), nullptr, GRD_DB_OPEN_CREATE, &g_db);
    EXPECT_EQ(status, GRD_OK);
    EXPECT_EQ(GRD_CreateCollection(g_db, "student", "", 0), GRD_OK);
    EXPECT_NE(g_db, nullptr);
}

void DocumentDBInsertTest::TearDownTestCase(void)
{
    EXPECT_EQ(GRD_DBClose(g_db, 0), GRD_OK);
    DocumentDBTestUtils::RemoveTestDbFiles(g_path);
}

void DocumentDBInsertTest::SetUp(void) {}

void DocumentDBInsertTest::TearDown(void) {}

/**
  * @tc.name: DocumentDBInsertTest001
  * @tc.desc: Insert documents into collection which dose not exist
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1.Insert document into collection which dose not exist
     * @tc.expected:step1.GRD_INVALID_ARGS
    */
    const char *document1 = "{\"_id\" : \"1\", \"name\" : \"Ori\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, NO_EXIST_COLLECTION_NAME, document1, 0), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentDBInsertTest002
  * @tc.desc: Insert documents into collection which _id is not string
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest002, TestSize.Level1)
{
    /**
     * @tc.steps:step1.Insert a document whose _id is integer
     * @tc.expected:step1.GRD_INVALID_ARGS
    */
    const char *document1 = "{\"_id\" : 2, \"name\" : \"Ori\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document1, 0), GRD_INVALID_ARGS);
    /**
     * @tc.steps:step2.Insert a document whose _id is bool
     * @tc.expected:step2.GRD_INVALID_ARGS
    */
    const char *document2 = "{\"_id\" : true, \"name\" : \"Chuan\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document2, 0), GRD_INVALID_ARGS);
    /**
     * @tc.steps:step2.Insert a document whose _id is NULL
     * @tc.expected:step2.GRD_INVALID_ARGS
    */
    const char *document3 = "{\"_id\" : null, \"name\" : \"Chuan\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document3, 0), GRD_INVALID_ARGS);
    /**
     * @tc.steps:step2.Insert a document whose _id is ARRAY
     * @tc.expected:step2.GRD_INVALID_ARGS
    */
    const char *document4 = "{\"_id\" : [\"2\"], \"name\" : \"Chuan\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document4, 0), GRD_INVALID_ARGS);
    /**
     * @tc.steps:step2.Insert a document whose _id is OBJECT
     * @tc.expected:step2.GRD_INVALID_ARGS
    */
    const char *document5 = "{\"_id\" : {\"val\" : \"2\"}, \"name\" : \"Chuan\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document5, 0), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentDBInsertTest003
  * @tc.desc: Insert a document whose _id has appeared before
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest003, TestSize.Level1)
{
    /**
     * @tc.steps:step1.Insert a document whose _id is string
     * @tc.expected:step1.GRD_OK
    */
    const char *document1 = "{\"_id\" : \"3\", \"name\" : \"Ori\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document1, 0), GRD_OK);

    /**
     * @tc.steps:step2.Insert a document whose _id has appeared before
     * @tc.expected:step2.GRD_DATA_CONFLICT
    */
    const char *document2 = "{\"_id\" : \"3\", \"name\" : \"Chuan\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document2, 0), GRD_DATA_CONFLICT);
}

/**
  * @tc.name: DocumentDBInsertTest004
  * @tc.desc: Test Insert with null parameter. parameter db is NULL
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest004, TestSize.Level1)
{
    /**
     * @tc.steps:step1.step1.parameter db is NULL
     * @tc.expected:step1.GRD_INVALID_ARGS
    */
    const char *document1 = "{\"_id\" : \"4\", \"name\" : \"Ori\"}";
    EXPECT_EQ(GRD_InsertDoc(NULL, RIGHT_COLLECTION_NAME, document1, 0), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentDBInsertTest005
  * @tc.desc: Test insert with null parameter. parameter collectionName is NULL.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest005, TestSize.Level1)
{
    /**
     * @tc.steps:step1.Parameter collectionName is NULL
     * @tc.expected:step1.GRD_INVALID_ARGS
    */
    const char *document1 = "{\"_id\" : \"5\", \"name\" : \"Ori\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, NULL, document1, 0), GRD_INVALID_ARGS);
    /**
      * @tc.steps:step2.Parameter collectionName is empty string
      * @tc.expected:step2.GRD_INVALID_ARGS
    */
    const char *document2 = "{\"_id\" : \"5\", \"name\" : \"Chuang\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, "", document2, 0), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentDBInsertTest006
  * @tc.desc: parameter flags is not zero
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest006, TestSize.Level1)
{
    /**
     * @tc.steps:step1.parameter flags is not zero
     * @tc.expected:step1.GRD_INVALID_ARGS
    */
    const char *document1 = "{\"_id\" : \"6\", \"name\" : \"Ori\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document1, 1), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentDBInsertTest007
  * @tc.desc: parameter flags is INT_MAX
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest007, TestSize.Level1)
{
    /**
     * @tc.steps:step1.parameter flags is int_max
     * @tc.expected:step1.GRD_INVALID_ARGS
    */
    const char *document1 = "{\"_id\" : \"7\", \"name\" : \"Ori\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document1, INT_MAX), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentDBInsertTest008
  * @tc.desc: parameter flags is int_min
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest008, TestSize.Level1)
{
    /**
     * @tc.steps:step1.parameter flags is int_min
     * @tc.expected:step1.GRD_INVALID_ARGS
    */
    const char *document1 = "{\"_id\" : \"8\", \"name\" : \"Ori\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document1, INT_MIN), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentDBInsertTest009
  * @tc.desc: parameter collectionName and document is NULL or invalid
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest009, TestSize.Level1)
{
    /**
     * @tc.steps:step1.parameter collectionName and document is NULL;
     * @tc.expected:step1.GRD_INVALID_ARGS
    */
    EXPECT_EQ(GRD_InsertDoc(g_db, NULL, NULL, 0), GRD_INVALID_ARGS);
    /**
     * @tc.steps:step2.parameter collectionName is larger than max_collectionName_lens;
     * @tc.expected:step2.GRD_OVER_LIMIT
    */
    const char *document1 = "{\"_id\" : \"9\", \"name\" : \"Ori\"}";
    std::string collectionName2(MAX_COLLECTION_LENS + 1, 'a');
    EXPECT_EQ(GRD_InsertDoc(g_db, collectionName2.c_str(), document1, 0), GRD_OVER_LIMIT);
}

/**
  * @tc.name: DocumentDBInsertTest010
  * @tc.desc: parameter collectionName contains irregular charactor
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest010, TestSize.Level1)
{
    /**
     * @tc.steps:step1.Create Collection whose parameter collectionName contains irregular charactor
     * @tc.expected:step1.GRD_OK
    */
    const char *collectionName = "collction@!#";
    const char *document1 = "{\"_id\" : \"10\", \"name\" : \"Ori\"}";
    TestInsertDocIntoCertainColl(collectionName, document1, GRD_OK);
}

/**
  * @tc.name: DocumentDBInsertTest011
  * @tc.desc: parameter collectionName is longer than 256 charactors
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest011, TestSize.Level1)
{
    /**
     * @tc.steps:step1.Create Collection whose parameter collectionName contains irregular charactor
     * @tc.expected:step1.GRD_OK
    */
    string collectionName(257, 'k');
    const char *document1 = "{\"_id\" : \"10\", \"name\" : \"Ori\"}";
    TestInsertDocIntoCertainColl(collectionName.c_str(), document1, GRD_OK);
}

/**
  * @tc.name: DocumentDBInsertTest014
  * @tc.desc: Inserted document's JSON depth is larger than 4, which is 5.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest014, TestSize.Level1)
{
    /**
     * @tc.steps:step1.document's JSON depth is larger than 4, which is 5.
     * @tc.expected:step1.GRD_INVALID_ARGS
    */
    const char *document1 = R""({"level1" : {"level2" : {"level3" : {"level4": {"level5" : 1}},
        "level3_2" : "level3_2_val"}},"_id":"14"})"";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document1, 0), GRD_INVALID_ARGS);
    /**
     * @tc.steps:step1.document's JSON depth is larger than 4, which is 5.But with array type.
     * @tc.expected:step1.GRD_INVALID_ARGS
    */
    const char *document2 = R""({"level1" : {"level2" : {"level3" : [{ "level5" : "level5_1val",
        "level5_2":"level5_2_val"}, "level4_val1","level4_val2"], "level3_2" : "level3_2_val"}}, "_id":"14"})"";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document2, 0), GRD_INVALID_ARGS);
    /**
     * @tc.steps:step1.document's JSON depth is 4
     * @tc.expected:step1.GRD_OK
    */
    const char *document3 = R""({"level1" : {"level2" : {"level3" : { "level4" : "level5_1val"},
        "level3_2" : "level3_2_val"}}, "_id":"14"})"";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document3, 0), GRD_OK);
}

/**
  * @tc.name: DocumentDBInsertTest015
  * @tc.desc: Inserted document with all kinds of size
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest015, TestSize.Level1)
{
    /**
     * @tc.steps:step1.document's JSON is bigger than 512k - 1
     * @tc.expected:step1.GRD_INVALID_ARGS
    */
    string documentPart1 = "{ \"_id\" : \"15\", \"textVal\" : \" ";
    string documentPart2 = "\" }";
    string jsonVal = string(512 * 1024 - documentPart1.size() - documentPart2.size(), 'k');
    string document = documentPart1 + jsonVal + documentPart2;
    /**
     * @tc.steps:step2.Insert document's JSON is a large data but lower than 512k - 1
     * @tc.expected:step2.GRD_OK
    */
    string jsonVal2 = string(512 * 1024 - 1 - documentPart1.size() - documentPart2.size(), 'k');
    string document2 = documentPart1 + jsonVal2 + documentPart2;
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document2.c_str(), 0), GRD_OK);
}

/**
  * @tc.name: DocumentDBInsertTest016
  * @tc.desc: document JSON string contains irregular char
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest016, TestSize.Level1)
{
    /**
     * @tc.steps:step1.document JSON string contains irregular char.
     * @tc.expected:step1.GRD_OK
    */
    const char *document1 = "{\"_id\" : \"16\", \"name\" : \"!@#Ori\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document1, 0), GRD_OK);
}

/**
  * @tc.name: DocumentDBInsertTest017
  * @tc.desc: document JSON string contains invalid value type such as BLOB type
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest017, TestSize.Level1)
{
    /**
     * @tc.steps:step1.document JSON string contains invalid value type such as BLOB type.
     * @tc.expected:step1.GRD_INVALID_FORMAT.
    */
    const char *document = "{\"_id\" : \"17\", \"level1\" : {\"level2\" : {\"level3\" : {\"level4\" : x'1234'\
        } } }, \"level1_2\" : \"level1_2Val\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document, 0), GRD_INVALID_FORMAT);
}

/**
  * @tc.name: DocumentDBInsertTest018
  * @tc.desc: The Inserted document is not JSON format
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest018, TestSize.Level1)
{
    /**
     * @tc.steps:step1.The Inserted document is not JSON format
     * @tc.expected:step1.GRD_INVALID_FORMAT.
    */
    const char *document = "some random string not JSON format";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document, 0), GRD_INVALID_FORMAT);
}

/**
  * @tc.name: DocumentDBInsertTest019
  * @tc.desc: Insert a normal documents
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest019, TestSize.Level1)
{
    /**
     * @tc.steps:step1.Insert a normal documents which _id is in the end of the string
     * @tc.expected:step1.GRD_OK.
    */
    const char *document1 = "{\"name\" : \"Jack\", \"age\" : 18, \"friend\" : {\"name\" : \" lucy\"}, \"_id\" : "
                            "\"19\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document1, 0), GRD_OK);
}

/**
  * @tc.name: DocumentDBInsertTest022
  * @tc.desc: parameter collectionName is equal to 256 charactors
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest022, TestSize.Level1)
{
    /**
     * @tc.steps:step1.parameter collectionName is equal to 256 charactors
     * @tc.expected:step1.GRD_OK.
    */
    string collectionName = string(256, 'k');
    string collectionName1(MAX_COLLECTION_LENS, 'a');
    const char *document1 = "{\"_id\" : \"22\", \"name\" : \"Ori\"}";
    TestInsertDocIntoCertainColl(collectionName.c_str(), document1, GRD_OK);
}

/**
  * @tc.name: DocumentDBInsertTest023
  * @tc.desc: parameter collectionName contains upper & lower case charactors,
  * numbers and underline
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest023, TestSize.Level1)
{
    /**
     * @tc.steps:step1.parameter collectionName contains upper & lower case charactors,
     * numbers and underline
     * @tc.expected:step1.GRD_OK.
     */
    string collectionName = "Aads_sd__23Asb_";
    const char *document1 = "{\"_id\" : \"23\", \"name\" : \"Ori\"}";
    TestInsertDocIntoCertainColl(collectionName.c_str(), document1, GRD_OK);
}

/**
  * @tc.name: DocumentDBInsertTest024
  * @tc.desc: parameter collectionName's head is GRD_ or GM_SYS_
  * numbers and underline
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest024, TestSize.Level1)
{
    /**
     * @tc.steps:step1.parameter collectionName's head is GRD_
     * @tc.expected:step1.GRD_INVALID_FORMAT.
     */
    string collectionName = "GRD_collectionName";
    const char *document1 = "{\"_id\" : \"24\", \"name\" : \"Ori\"}";
    GRD_CreateCollection(g_db, collectionName.c_str(), "", 0);
    EXPECT_EQ(GRD_InsertDoc(g_db, collectionName.c_str(), document1, 0), GRD_INVALID_FORMAT);

    /**
     * @tc.steps:step2.parameter collectionName's head is GM_SYS_
     * @tc.expected:step2.GRD_INVALID_FORMAT.
     */
    collectionName = "GM_SYS__collectionName";
    const char *document2 = "{\"_id\" : \"24_2\", \"name\" : \"Ori\"}";
    GRD_CreateCollection(g_db, collectionName.c_str(), "", 0);
    EXPECT_EQ(GRD_InsertDoc(g_db, collectionName.c_str(), document2, 0), GRD_INVALID_FORMAT);

    /**
     * @tc.steps:step3.parameter collectionName's head is grd_
     * @tc.expected:step3.GRD_INVALID_FORMAT.
     */
    collectionName = "grd_collectionName";
    const char *document3 = "{\"_id\" : \"24_3\", \"name\" : \"Ori\"}";
    GRD_CreateCollection(g_db, collectionName.c_str(), "", 0);
    EXPECT_EQ(GRD_InsertDoc(g_db, collectionName.c_str(), document3, 0), GRD_INVALID_FORMAT);

    /**
     * @tc.steps:step4.parameter collectionName's head is gm_sys_
     * @tc.expected:step4.GRD_INVALID_FORMAT.
     */
    collectionName = "gm_sys_collectionName";
    const char *document4 = "{\"_id\" : \"24_4\", \"name\" : \"Ori\"}";
    GRD_CreateCollection(g_db, collectionName.c_str(), "", 0);
    EXPECT_EQ(GRD_InsertDoc(g_db, collectionName.c_str(), document4, 0), GRD_INVALID_FORMAT);

    /**
     * @tc.steps:step5.parameter collectionName's head is gM_sYs_ that has Uppercase and lowercase at the same time.
     * @tc.expected:step5.GRD_INVALID_FORMAT.
     */
    collectionName = "gM_sYs_collectionName";
    const char *document5 = "{\"_id\" : \"24_5\", \"name\" : \"Ori\"}";
    GRD_CreateCollection(g_db, collectionName.c_str(), "", 0);
    EXPECT_EQ(GRD_InsertDoc(g_db, collectionName.c_str(), document5, 0), GRD_INVALID_FORMAT);

    /**
     * @tc.steps:step6.parameter collectionName's head is gRd_ that has Uppercase and lowercase at the same time.
     * @tc.expected:step6.GRD_INVALID_FORMAT.
     */
    collectionName = "gRd_collectionName";
    const char *document6 = "{\"_id\" : \"24_6\", \"name\" : \"Ori\"}";
    GRD_CreateCollection(g_db, collectionName.c_str(), "", 0);
    EXPECT_EQ(GRD_InsertDoc(g_db, collectionName.c_str(), document6, 0), GRD_INVALID_FORMAT);

    /**
     * @tc.steps:step7.parameter collectionName's head is grd@ that has no '_'
     * @tc.expected:step7.GRD_INVALID_FORMAT.
     */
    collectionName = "gRd@collectionName";
    const char *document7 = "{\"_id\" : \"24_7\", \"name\" : \"Ori\"}";
    GRD_CreateCollection(g_db, collectionName.c_str(), "", 0);
    EXPECT_EQ(GRD_InsertDoc(g_db, collectionName.c_str(), document7, 0), GRD_OK);
}

/**
  * @tc.name: DocumentDBInsertTest025
  * @tc.desc: Insert document whose depth is 4, which is allowed
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest025, TestSize.Level1)
{
    /**
     * @tc.steps:step1.documents JSON depth is 4, which is allowed.
     * @tc.expected:step1.GRD_OK.
     */
    const char *document1 = "{\"_id\" : \"25_0\", \"level1\" : {\"level2\" : {\"level3\" :\
        {\"level4\" : \"level4Val\"}}} , \"level1_2\" : \"level1_2Val\" }";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document1, 0), GRD_OK);
    /**
     * @tc.steps:step2.documents JSON depth is exactly 4.
     * @tc.expected:step2.GRD_OK.
     */
    const char *document2 = "{\"_id\" : \"25_1\", \"class_name\" : \"计算机科学一班\", \"signed_info\" : true, \
        \"student_info\" : [{\"name\":\"张三\", \"age\" : 18, \"sex\" : \"男\"}, \
        { \"newName1\" : [\"qw\", \"dr\", 0, \"ab\"] }]}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document2, 0), GRD_OK);
    /**
     * @tc.steps:step3.documents JSON depth is exactly 4, but the last field in array contains leading number
     * @tc.expected:step3.GRD_INVALID_ARGS.
     */
    const char *document3 = "{\"_id\" : \"25_2\", \"class_name\" : \"计算机科学一班\", \"signed_info\" : true, \
        \"student_info\" : [{\"name\":\"张三\", \"age\" : 18, \"sex\" : \"男\"}, \
        [\"qw\", \"dr\", 0, \"ab\", {\"0ab\" : null}]]}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document3, 0), GRD_INVALID_ARGS);
    /**
     * @tc.steps:step4.documents JSON depth is exactly 5.
     * @tc.expected:step4.GRD_INVALID_ARGS.
     */
    const char *document4 = "{\"_id\" : \"25_3\", \"class_name\" : \"计算机科学一班\", \"signed_info\" : true, \
        \"student_info\" : [{\"name\":\"张三\", \"age\" : 18, \"sex\" : \"男\"}, \
        { \"newName1\" : [\"qw\", \"dr\", 0, \"ab\", {\"level5\" : 1}] }]}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document4, 0), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentDBInsertTest026
  * @tc.desc: Insert 100 normal documents continuously
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest, TestSize.Level1)
{
    /**
     * @tc.steps:step1.Insert 100 normal documents continuously
     * @tc.expected:step1.GRD_OK.
     */
    string document1 = "{\"_id\" : ";
    string document2 = "\"";
    string document4 = "\"";
    string document5 = ", \"name\" : \"Ori\"}";
    for (int i = 0; i < 5; i++) {
        string document_midlle = "26" + std::to_string(i);
        string document = document1 + document2 + document_midlle + document4 + document5;
        EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document.c_str(), 0), GRD_OK);
    }
}

/**
  * @tc.name: DocumentDBInsertTest035
  * @tc.desc: Insert a document whose value contains
  * upper &lower case charactors, numbers and underline.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest035, TestSize.Level1)
{
    /**
     * @tc.steps:step1.Insert a document whose value contains
     * upper &lower case charactors, numbers and underline.
     * @tc.expected:step1.GRD_OK.
     */
    const char *document1 = "{\"_id\" : \"35\", \"A_aBdk_324_\" : \"value\", \"name\" : \"Chuan\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document1, 0), GRD_OK);
    /**
     * @tc.steps:step1.Insert a document whose value contains
     * upper &lower case charactors, numbers and underline.
     * But the field started with number, which is not allowed.
     * @tc.expected:step1.GRD_OK.
     */
    const char *document2 = "{\"_id\" : \"35_2\", \"1A_aBdk_324_\" : \"value\", \"name\" : \"Chuan\"}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document2, 0), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentDBInsertTest036
  * @tc.desc: Insert a document whose value contains
  * string, number, bool, null, array and object type
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest036, TestSize.Level1)
{
    /**
     * @tc.steps:step1.Insert a document whose value contains
     * string, number, bool, null, array and object type
     * @tc.expected:step1.GRD_OK.
     */
    const char *document1 = "{\"_id\" : \"36_0\", \"stringType\" : \"stringVal\", \"numType\" : 1, \"BoolType\" : true,\
                          \"nullType\" : null, \"arrayType\" : [1, 2, 3, 4], \"objectType\" : {\"A\" : 3}}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document1, 0), GRD_OK);
}

/**
  * @tc.name: DocumentDBInsertTest038
  * @tc.desc: Insert document whose value is over  the range of double
  * string, number, bool, null, array and object type
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest038, TestSize.Level1)
{
    /**
     * @tc.steps:step1.Insert document whose value is over the range of double
     * @tc.expected:step1.GRD_INVALID_ARGS.
     */
    const char *document1 = R"({"_id" : "38_0", "field2" : 1.79769313486232e308})";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document1, 0), GRD_INVALID_ARGS);
    /**
     * @tc.steps:step2.Insert document whose value is over the range of double
     * @tc.expected:step2.GRD_INVALID_ARGS.
     */
    const char *document2 = R"({"_id" : "38_1", "t1" : {"field2" : 1.79769313486232e308}})";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document2, 0), GRD_INVALID_ARGS);
    /**
     * @tc.steps:step3.Insert document whose value is over the range of double
     * @tc.expected:step3.GRD_INVALID_ARGS.
     */
    const char *document3 = R"({"_id" : "38_2", "t1" : [1, 2, 1.79769313486232e308]})";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document3, 0), GRD_INVALID_ARGS);
    /**
     * @tc.steps:step4.Insert document whose value is over the range of double
     * @tc.expected:step4.GRD_INVALID_ARGS.
     */
    const char *document4 = R"({"_id" : "38_3", "t1" : [1, 2, -1.7976931348623167E+308]})";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document4, 0), GRD_INVALID_ARGS);
    /**
     * @tc.steps:step5.Insert document with minimum double value
     * @tc.expected:step5.GRD_INVALID_ARGS.
     */
    const char *document5 = R"({"_id" : "38_4", "t1" : [1, 2, -1.79769313486231570E+308]})";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document5, 0), GRD_OK);
    /**
     * @tc.steps:step6.Insert document with maxium double value
     * @tc.expected:step6.GRD_INVALID_ARGS.
     */
    const char *document6 = R"({"_id" : "38_5", "t1" : [1, 2, 1.79769313486231570E+308]})";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document6, 0), GRD_OK);
}

/**
  * @tc.name: DocumentDBInsertTest039
  * @tc.desc: Insert a filter which _id value's lens is larger than MAX_ID_LENS
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest039, TestSize.Level1)
{
    /**
     * @tc.steps:step1.Insert a filter which _id value's lens is larger than MAX_ID_LENS.
     * @tc.expected:step1.GRD_OVER_LIMIT.
     */
    string document1 = "{\"_id\" : ";
    string document2 = "\"";
    string document4 = "\"";
    string document5 = ", \"name\" : \"Ori\"}";
    string document_midlle(MAX_ID_LENS + 1, 'k');
    string document = document1 + document2 + document_midlle + document4 + document5;
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document.c_str(), 0), GRD_OVER_LIMIT);

    /**
     * @tc.steps:step1.Insert a filter which _id value's lens is equal as MAX_ID_LENS.
     * @tc.expected:step1.GRD_OK.
     */
    string document_midlle2(MAX_ID_LENS, 'k');
    document = document1 + document2 + document_midlle2 + document4 + document5;
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document.c_str(), 0), GRD_OK);
}

/**
  * @tc.name: DocumentUpdataApiTest040
  * @tc.desc: Insert a filter which _id value's lens is larger than MAX_ID_LENS
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest040, TestSize.Level1)
{
    const char *filter = "{\"_id\" : \"1\"}";
    const char *updata2 = "{\"objectInfo.child.child\" : {\"child\":{\"child\":null}}}";
    EXPECT_EQ(GRD_UpdateDoc(g_db, RIGHT_COLLECTION_NAME, filter, updata2, 0), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentUpdataApiTest041
  * @tc.desc: Insert a filter which _id value's lens is larger than MAX_ID_LENS
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest041, TestSize.Level1)
{
    const char *filter = "{\"_id\" : \"1\"}";
    const char *updata1 = "{\"_id\" : \"6\"}";
    EXPECT_EQ(GRD_UpdateDoc(g_db, RIGHT_COLLECTION_NAME, filter, updata1, 0), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentUpdataApiTest042
  * @tc.desc: Insert a filter which _id value's lens is larger than MAX_ID_LENS
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest042, TestSize.Level1)
{
    const char *filter = "{\"_id\" : \"1\"}";
    const char *updata1 = "{\"age$\" : \"21\"}";
    const char *updata2 = "{\"bonus..traffic\" : 100}";
    const char *updata3 = "{\"0item\" : 100}";
    const char *updata4 = "{\"item\" : 1.79769313486232e308}";
    EXPECT_EQ(GRD_UpdateDoc(g_db, RIGHT_COLLECTION_NAME, filter, updata1, 0), GRD_INVALID_ARGS);
    EXPECT_EQ(GRD_UpdateDoc(g_db, RIGHT_COLLECTION_NAME, filter, updata2, 0), GRD_INVALID_ARGS);
    EXPECT_EQ(GRD_UpdateDoc(g_db, RIGHT_COLLECTION_NAME, filter, updata3, 0), GRD_INVALID_ARGS);
    EXPECT_EQ(GRD_UpdateDoc(g_db, RIGHT_COLLECTION_NAME, filter, updata4, 0), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentUpdataApiTest043
  * @tc.desc: Insert a filter which _id value's lens is larger than MAX_ID_LENS
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest043, TestSize.Level1)
{
    const char *filter = "{\"_id\" : \"1\"}";
    const char *updata1 = "{\"age\" : 21}";
    EXPECT_EQ(GRD_UpdateDoc(g_db, NULL, filter, updata1, 0), GRD_INVALID_ARGS);
    EXPECT_EQ(GRD_UpdateDoc(g_db, "", filter, updata1, 0), GRD_INVALID_ARGS);
    EXPECT_EQ(GRD_UpdateDoc(NULL, RIGHT_COLLECTION_NAME, filter, updata1, 0), GRD_INVALID_ARGS);
    EXPECT_EQ(GRD_UpdateDoc(g_db, RIGHT_COLLECTION_NAME, NULL, updata1, 0), GRD_INVALID_ARGS);
    EXPECT_EQ(GRD_UpdateDoc(g_db, RIGHT_COLLECTION_NAME, filter, NULL, 0), GRD_INVALID_ARGS);
}

HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest044, TestSize.Level1)
{
    const char *document1 = R""({"_id":"0123", "num":"num"})"";
    const char *document2 = R""({"_id":"0123", "NUM":"No.45"})"";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document1, 0), GRD_OK);
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document2, 0), GRD_DATA_CONFLICT);
}

HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest045, TestSize.Level1)
{
    const char *document1 = R""({"_id":"0123", "num.":"num"})"";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document1, 0), GRD_INVALID_ARGS);
}

HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest046, TestSize.Level1)
{
    const char *document1 = R""({})"";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document1, 0), GRD_OK);
}

HWTEST_F(DocumentDBInsertTest, DocumentDBInsertTest047, TestSize.Level1)
{
    const char *document1 = "{\"empty\" : null}";
    EXPECT_EQ(GRD_InsertDoc(g_db, RIGHT_COLLECTION_NAME, document1, 0), GRD_OK);
}
} // namespace
