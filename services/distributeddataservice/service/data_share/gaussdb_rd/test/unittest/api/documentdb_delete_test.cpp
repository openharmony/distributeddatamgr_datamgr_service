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

#include <gtest/gtest.h>

#include "documentdb_test_utils.h"
#include "grd_base/grd_db_api.h"
#include "grd_base/grd_error.h"
#include "grd_base/grd_resultset_api.h"
#include "grd_base/grd_type_export.h"
#include "grd_document/grd_document_api.h"
#include "grd_resultset_inner.h"
#include "grd_type_inner.h"

using namespace testing::ext;
using namespace DocumentDBUnitTest;
namespace {
constexpr const char *COLLECTION_NAME = "student";
constexpr const char *NULL_JSON_STR = "{}";
const int MAX_COLLECTION_LENS = 511;
std::string g_path = "./document.db";
GRD_DB *g_db = nullptr;

class DocumentDBDeleteTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    void InsertDoc(const char *collectionName, const char *document);
};
void DocumentDBDeleteTest::SetUpTestCase(void)
{
    int status = GRD_DBOpen(g_path.c_str(), nullptr, GRD_DB_OPEN_CREATE, &g_db);
    EXPECT_EQ(status, GRD_OK);
}

void DocumentDBDeleteTest::TearDownTestCase(void)
{
    EXPECT_EQ(GRD_DBClose(g_db, 0), GRD_OK);
    DocumentDBTestUtils::RemoveTestDbFiles(g_path);
}

void DocumentDBDeleteTest::SetUp(void)
{
    /**
     * @tc.steps:step1. Create Collection
     * @tc.expected: step1. GRD_OK
    */
    EXPECT_EQ(GRD_CreateCollection(g_db, "student", "", 0), GRD_OK);
    /**
     * @tc.steps:step2. Insert many document in order to delete
     * @tc.expected: step2. GRD_OK
    */
    const char *document1 = "{  \
        \"_id\" : \"1\", \
        \"name\": \"xiaoming\", \
        \"address\": \"beijing\", \
        \"age\" : 15, \
        \"friend\" : {\"name\" : \"David\", \"sex\" : \"female\", \"age\" : 90}, \
        \"subject\": [\"math\", \"English\", \"music\", {\"info\" : \"exam\"}] \
    }";
    const char *document2 = "{  \
        \"_id\" : \"2\", \
        \"name\": \"ori\", \
        \"address\": \"beijing\", \
        \"age\" : 15, \
        \"friend\" : {\"name\" : \"David\", \"sex\" : \"female\", \"age\" : 90}, \
        \"subject\": [\"math\", \"English\", \"music\"] \
    }";
    const char *document3 = "{  \
        \"_id\" : \"3\", \
        \"name\": \"David\", \
        \"address\": \"beijing\", \
        \"age\" : 15, \
        \"friend\" : {\"name\" : \"David\", \"sex\" : \"female\", \"age\" : 90}, \
        \"subject\": [\"Sing\", \"Jump\", \"Rap\", \"BasketBall\"] \
    }";
    DocumentDBDeleteTest::InsertDoc(COLLECTION_NAME, document1);
    DocumentDBDeleteTest::InsertDoc(COLLECTION_NAME, document2);
    DocumentDBDeleteTest::InsertDoc(COLLECTION_NAME, document3);
}

void DocumentDBDeleteTest::TearDown(void)
{
    /**
     * @tc.steps:step1. Call GRD_DropCollection to drop the collection
     * @tc.expected: step1. GRD_OK
    */
    EXPECT_EQ(GRD_DropCollection(g_db, COLLECTION_NAME, 0), GRD_OK);
}

static void ChkDeleteResWithFilter(const char *filter)
{
    /**
     * @tc.steps:step1. Try to find the deleted document
     * @tc.expected: step1. GRD_OK
    */
    Query query;
    query.filter = filter;
    const char *projection = "{}";
    query.projection = projection;
    GRD_ResultSet *resultSet = nullptr;
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_OK);
    /**
     * @tc.steps:step2. The resultset should be NULL
     * @tc.expected: step2. GRD_OK
    */
    EXPECT_EQ(GRD_Next(resultSet), GRD_NO_DATA);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

void DocumentDBDeleteTest::InsertDoc(const char *collectionName, const char *document)
{
    EXPECT_EQ(GRD_InsertDoc(g_db, collectionName, document, 0), GRD_OK);
}

/**
  * @tc.name: DocumentDelete001
  * @tc.desc: Delete with NULL filter
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBDeleteTest, DeleteDBTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Delete all the document
     * @tc.expected: step1. GRD_INVALID_ARGS
    */
    EXPECT_EQ(GRD_DeleteDoc(g_db, COLLECTION_NAME, NULL_JSON_STR, 0), 1);
}

/**
  * @tc.name: DocumentDelete002
  * @tc.desc: Delete with filter which has no _id
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBDeleteTest, DeleteDBTest002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Delete with filter which has no _id
     * @tc.expected: step1. GRD_INVALID_ARGS
    */
    const char *filter = "{\"age\" : 15}";
    EXPECT_EQ(GRD_DeleteDoc(g_db, COLLECTION_NAME, filter, 0), 1);
}

/**
  * @tc.name: DocumentDelete003
  * @tc.desc: Delete with filter which has more than one fields.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBDeleteTest, DeleteDBTest003, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Delete with filter which has more than one fields.
     * @tc.expected: step1. GRD_INVALID_ARGS
    */
    const char *filter = "{\"_id\" : \"1\", \"age\" : 15}";
    EXPECT_EQ(GRD_DeleteDoc(g_db, COLLECTION_NAME, filter, 0), 1);
}

/**
  * @tc.name: DocumentDelete004
  * @tc.desc: Test delete with invalid input
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBDeleteTest, DeleteDBTest004, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Test delete with un-zero flags
     * @tc.expected: step1. GRD_INVALID_ARGS
    */
    const char *filter1 = "{\"_id\" : \"1\"}";
    EXPECT_EQ(GRD_DeleteDoc(g_db, COLLECTION_NAME, filter1, 1), GRD_INVALID_ARGS);
    /**
     * @tc.steps:step2. Test delete with NULL collection name
     * @tc.expected: step2. GRD_INVALID_ARGS
    */
    const char *filter2 = "{\"_id\" : \"1\"}";
    EXPECT_EQ(GRD_DeleteDoc(g_db, NULL, filter2, 0), GRD_INVALID_ARGS);
    /**
     * @tc.steps:step3. Test delete with empty collection name
     * @tc.expected: step3. GRD_INVALID_ARGS
    */
    const char *filter3 = "{\"_id\" : \"1\"}";
    EXPECT_EQ(GRD_DeleteDoc(g_db, "", filter3, 1), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentDelete005
  * @tc.desc: Test delete with same collection name
  * but one is uppercase(delete) and the other is lowercase(insert)
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBDeleteTest, DeleteDBTest005, TestSize.Level1)
{
    /**
      * @tc.step1: Test delete with same collection name
      * but one is uppercase(delete) and the other is lowercase(insert)
      * @tc.expected: step1. GRD_INVALID_ARGS
      */
    const char *filter = "{\"_id\" : \"1\"}";
    EXPECT_EQ(GRD_DeleteDoc(g_db, COLLECTION_NAME, filter, 0), 1);
    /**
      * @tc.step2: Check whether doc has been deleted compeletely
      * @tc.expected: step2. GRD_OK
      */
    ChkDeleteResWithFilter(filter);
}

/**
  * @tc.name: DocumentDelete006
  * @tc.desc: Test delete after calling find interface
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBDeleteTest, DeleteDBTest006, TestSize.Level1)
{
    /**
      * @tc.step1: Create filter with _id and get the record according to filter condition.
      * @tc.expected: step1. GRD_OK
      */
    const char *filter = "{\"_id\" : \"1\"}";
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_DeleteDoc(g_db, COLLECTION_NAME, filter, 0), 1);
    /**
      * @tc.step2: Invoke GRD_Next to get the next matching value. Release resultSet.
      * @tc.expected: step2. Cannot get next record, return GRD_NO_DATA.
      */
    EXPECT_EQ(GRD_Next(resultSet), GRD_NO_DATA);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentDelete007
  * @tc.desc: Test delete with too long collectionName.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBDeleteTest, DeleteDBTest007, TestSize.Level1)
{
    const char *filter = "{\"_id\" : \"1\"}";
    string collectionName1(MAX_COLLECTION_LENS, 'a');
    EXPECT_EQ(GRD_CreateCollection(g_db, collectionName1.c_str(), "", 0), GRD_OK);
    EXPECT_EQ(GRD_DeleteDoc(g_db, collectionName1.c_str(), filter, 0), 0);
    EXPECT_EQ(GRD_DropCollection(g_db, collectionName1.c_str(), 0), GRD_OK);

    string collectionName2(MAX_COLLECTION_LENS + 1, 'a');
    EXPECT_EQ(GRD_DeleteDoc(g_db, collectionName2.c_str(), filter, 0), GRD_OVER_LIMIT);
}

/**
  * @tc.name: DocumentDelete008
  * @tc.desc: Test delete with invalid NULL input for all parameters.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBDeleteTest, DeleteDBTest008, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Delete with filter which has more than one fields.
     * @tc.expected: step1. GRD_INVALID_ARGS
    */
    const char *filter = "{\"_id\" : \"1\"}";
    EXPECT_EQ(GRD_DeleteDoc(NULL, COLLECTION_NAME, filter, 0), GRD_INVALID_ARGS);
    EXPECT_EQ(GRD_DeleteDoc(g_db, NULL, filter, 0), GRD_INVALID_ARGS);
    EXPECT_EQ(GRD_DeleteDoc(g_db, "", filter, 0), GRD_INVALID_ARGS);
    EXPECT_EQ(GRD_DeleteDoc(g_db, COLLECTION_NAME, NULL, 0), GRD_INVALID_ARGS);
    EXPECT_EQ(GRD_DeleteDoc(g_db, COLLECTION_NAME, "", 0), GRD_INVALID_ARGS);
    EXPECT_EQ(GRD_DeleteDoc(g_db, "notExisted", filter, 0), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentDelete010
  * @tc.desc: Test delete document when filter _id is int and string
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBDeleteTest, DeleteDBTest010, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Test delete document when filter _id is int and string.
     * @tc.expected: step1. GRD_INVALID_ARGS
    */
    std::vector<std::string> filterVec = { R"({"_id" : 1})", R"({"_id":[1, 2]})", R"({"_id" : {"t1" : 1}})",
        R"({"_id":null})", R"({"_id":true})", R"({"_id" : 1.333})", R"({"_id" : -2.0})" };
    for (const auto &item : filterVec) {
        EXPECT_EQ(GRD_DeleteDoc(g_db, COLLECTION_NAME, item.c_str(), 0), GRD_INVALID_ARGS);
    }
    const char *filter = "{\"_id\" : \"1\"}";
    EXPECT_EQ(GRD_DeleteDoc(g_db, COLLECTION_NAME, filter, 0), 1);
}

/**
  * @tc.name: DocumentDelete011
  * @tc.desc:
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBDeleteTest, DeleteDBTest011, TestSize.Level1)
{
    /**
      * @tc.step1: Create filter with _id and get the record according to filter condition.
      * @tc.expected: step1. GRD_OK
      */
    const char *filter = "{\"_id\" : \"1\"}";
    const char *filter2 = "{\"subject.info\" : \"exam\"}";
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_DeleteDoc(g_db, COLLECTION_NAME, filter2, 0), 1);
    /**
      * @tc.step2: Invoke GRD_Next to get the next matching value. Release resultSet.
      * @tc.expected: step2. Cannot get next record, return GRD_NO_DATA.
      */
    EXPECT_EQ(GRD_Next(resultSet), GRD_NO_DATA);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentDelete012
  * @tc.desc:
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBDeleteTest, DeleteDBTest012, TestSize.Level1)
{
    /**
      * @tc.step1: Create filter with _id and get the record according to filter condition.
      * @tc.expected: step1. GRD_OK
      */
    const char *filter = "{\"_id\" : \"1\"}";
    const char *filter2 = "{\"subject.info\" : \"exam\"}";
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_DeleteDoc(g_db, COLLECTION_NAME, filter2, 0), 1);
    /**
      * @tc.step2: Invoke GRD_Next to get the next matching value. Release resultSet.
      * @tc.expected: step2. Cannot get next record, return GRD_NO_DATA.
      */
    EXPECT_EQ(GRD_Next(resultSet), GRD_NO_DATA);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}
} // namespace