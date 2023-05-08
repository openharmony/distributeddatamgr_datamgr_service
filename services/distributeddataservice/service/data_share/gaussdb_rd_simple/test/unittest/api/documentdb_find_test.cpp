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

#include "doc_errno.h"
#include "documentdb_test_utils.h"
#include "grd_base/grd_db_api.h"
#include "grd_base/grd_error.h"
#include "grd_base/grd_resultset_api.h"
#include "grd_base/grd_type_export.h"
#include "grd_document/grd_document_api.h"
#include "grd_resultset_inner.h"
#include "grd_type_inner.h"
#include "log_print.h"

using namespace testing::ext;
namespace {
std::string path = "./document.db";
GRD_DB *g_db = nullptr;
constexpr const char *COLLECTION_NAME = "student";
constexpr const char *NULL_JSON_STR = "{}";
const int E_OK = 0;
const int MAX_COLLECTION_NAME = 511;
const int INT_MAX = 2147483647;
const int INT_MIN = -2147483648;
const int MAX_ID_LENS = 899;
static const char *g_document1 = "{\"_id\" : \"1\", \"name\":\"doc1\",\"item\":\"journal\",\"personInfo\":\
    {\"school\":\"AB\", \"age\" : 51}}";
static const char *g_document2 = "{\"_id\" : \"2\", \"name\":\"doc2\",\"item\": 1, \"personInfo\":\
    [1, \"my string\", {\"school\":\"AB\", \"age\" : 51}, true, {\"school\":\"CD\", \"age\" : 15}, false]}";
static const char *g_document3 = "{\"_id\" : \"3\", \"name\":\"doc3\",\"item\":\"notebook\",\"personInfo\":\
    [{\"school\":\"C\", \"age\" : 5}]}";
static const char *g_document4 = "{\"_id\" : \"4\", \"name\":\"doc4\",\"item\":\"paper\",\"personInfo\":\
    {\"grade\" : 1, \"school\":\"A\", \"age\" : 18}}";
static const char *g_document5 = "{\"_id\" : \"5\", \"name\":\"doc5\",\"item\":\"journal\",\"personInfo\":\
    [{\"sex\" : \"woma\", \"school\" : \"B\", \"age\" : 15}, {\"school\":\"C\", \"age\" : 35}]}";
static const char *g_document6 = "{\"_id\" : \"6\", \"name\":\"doc6\",\"item\":false,\"personInfo\":\
    [{\"school\":\"B\", \"teacher\" : \"mike\",\"age\" : 15}, {\"school\":\"C\", \"teacher\" : \"moon\",\"age\" : 20}]}";
static const char *g_document7 = "{\"_id\" : \"7\", \"name\":\"doc7\",\"item\":\"fruit\",\"other_Info\":\
    [{\"school\":\"BX\", \"age\" : 15}, {\"school\":\"C\", \"age\" : 35}]}";
static const char *g_document8 = "{\"_id\" : \"8\", \"name\":\"doc8\",\"item\":true,\"personInfo\":\
    [{\"school\":\"B\", \"age\" : 15}, {\"school\":\"C\", \"age\" : 35}]}";
static const char *g_document9 = "{\"_id\" : \"9\", \"name\":\"doc9\",\"item\": true}";
static const char *g_document10 = "{\"_id\" : \"10\", \"name\":\"doc10\", \"parent\" : \"kate\"}";
static const char *g_document11 = "{\"_id\" : \"11\", \"name\":\"doc11\", \"other\" : \"null\"}";
static const char *g_document12 = "{\"_id\" : \"12\", \"name\":\"doc12\",\"other\" : null}";
static const char *g_document13 = "{\"_id\" : \"13\", \"name\":\"doc13\",\"item\" : \"shoes\",\"personInfo\":\
    {\"school\":\"AB\", \"age\" : 15}}";
static const char *g_document14 = "{\"_id\" : \"14\", \"name\":\"doc14\",\"item\" : true,\"personInfo\":\
    [{\"school\":\"B\", \"age\" : 15}, {\"school\":\"C\", \"age\" : 85}]}";
static const char *g_document15 = "{\"_id\" : \"15\", \"name\":\"doc15\",\"personInfo\":[{\"school\":\"C\", \"age\" : "
                                  "5}]}";
static const char *g_document16 = "{\"_id\" : \"16\", \"name\":\"doc16\", \"nested1\":{\"nested2\":{\"nested3\":\
    {\"nested4\":\"ABC\", \"field2\":\"CCC\"}}}}";
static const char *g_document17 = "{\"_id\" : \"17\", \"name\":\"doc17\",\"personInfo\":\"oh,ok\"}";
static const char *g_document18 = "{\"_id\" : \"18\", \"name\":\"doc18\",\"item\" : \"mobile phone\",\"personInfo\":\
    {\"school\":\"DD\", \"age\":66}, \"color\":\"blue\"}";
static const char *g_document19 = "{\"_id\" : \"19\", \"name\":\"doc19\",\"ITEM\" : true,\"PERSONINFO\":\
    {\"school\":\"AB\", \"age\":15}}";
static const char *g_document20 = "{\"_id\" : \"20\", \"name\":\"doc20\",\"ITEM\" : true,\"personInfo\":\
    [{\"SCHOOL\":\"B\", \"AGE\":15}, {\"SCHOOL\":\"C\", \"AGE\":35}]}";
static const char *g_document23 = "{\"_id\" : \"23\", \"name\":\"doc22\",\"ITEM\" : "
                                  "true,\"personInfo\":[{\"school\":\"b\", \"age\":15}, [{\"school\":\"doc23\"}, 10, "
                                  "{\"school\":\"doc23\"}, true, {\"school\":\"y\"}], {\"school\":\"b\"}]}";
static std::vector<const char *> g_data = { g_document1, g_document2, g_document3, g_document4, g_document5,
    g_document6, g_document7, g_document8, g_document9, g_document10, g_document11, g_document12, g_document13,
    g_document14, g_document15, g_document16, g_document17, g_document18, g_document19, g_document20, g_document23 };

static void InsertData(GRD_DB *g_db, const char *collectionName)
{
    for (const auto &item : g_data) {
        EXPECT_EQ(GRD_InsertDoc(g_db, collectionName, item, 0), GRD_OK);
    }
}

static void CompareValue(const char *value, const char *targetValue)
{
    int errCode;
    DocumentDB::JsonObject valueObj = DocumentDB::JsonObject::Parse(value, errCode);
    EXPECT_EQ(errCode, E_OK);
    DocumentDB::JsonObject targetValueObj = DocumentDB::JsonObject::Parse(targetValue, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(valueObj.Print(), targetValueObj.Print());
}
} // namespace

class DocumentFindApiTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    void InsertDoc(const char *collectionName, const char *document);
};
void DocumentFindApiTest::SetUpTestCase(void)
{
    std::string path = "./document.db";
    int status = GRD_DBOpen(path.c_str(), nullptr, GRD_DB_OPEN_CREATE, &g_db);
    EXPECT_EQ(status, GRD_OK);
    EXPECT_EQ(GRD_CreateCollection(g_db, COLLECTION_NAME, "", 0), GRD_OK);
    EXPECT_NE(g_db, nullptr);
}

void DocumentFindApiTest::TearDownTestCase(void)
{
    EXPECT_EQ(GRD_DBClose(g_db, 0), GRD_OK);
    remove(path.c_str());
}

void DocumentFindApiTest::SetUp(void)
{
    InsertData(g_db, "student");
}

void DocumentFindApiTest::TearDown(void) {}

/**
  * @tc.name: DocumentFindApiTest001
  * @tc.desc: Test Insert document db
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter with _id and get the record according to filter condition.
     * @tc.expected: step1. Succeed to get the record, the matching record is g_document6.
     */
    const char *filter = "{\"_id\" : \"6\"}";
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = NULL;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, g_document6);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    /**
     * @tc.steps: step2. Invoke GRD_Next to get the next matching value. Release resultSet.
     * @tc.expected: step2. Cannot get next record, return GRD_NO_DATA.
     */
    EXPECT_EQ(GRD_Next(resultSet), GRD_NO_DATA);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_NOT_AVAILABLE);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest002
  * @tc.desc: Test filter with multiple fields and _id.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter with multiple and _id. and get the record according to filter condition.
     * @tc.expected: step1. Faild to get the record, the result is GRD_INVALID_ARGS, GRD_GetValue return GRD_NOT_AVAILABLE and GRD_Next return GRD_NO_DATA.
     */
    const char *filter = "{\"_id\" : \"6\", \"name\":\"doc6\"}";
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);
    /**
     * @tc.steps: step2. Invoke GRD_Next to get the next matching value. Release resultSet.
     * @tc.expected: step2. GRD_GetValue return GRD_INVALID_ARGS and GRD_Next return GRD_INVALID_ARGS.
     */
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = NULL;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest004
  * @tc.desc: test filter with string filter without _id.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter without _id and get the record according to filter condition.
     * @tc.expected: step1. Faild to get the record, the result is GRD_INVALID_ARGS,
     */
    const char *filter = "{\"name\":\"doc6\"}";
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);

    /**
     * @tc.steps: step2. Invoke GRD_Next to get the next matching value. Release resultSet.
     * @tc.expected: step2. GRD_GetValue return GRD_INVALID_ARGS and GRD_Next return GRD_INVALID_ARGS.
     */
    char *value = NULL;
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

// /**
//   * @tc.name: DocumentFindApiTest005
//   * @tc.desc: test filter field with other word.
//   * @tc.type: FUNC
//   * @tc.require:
//   * @tc.author: mazhao
//   */
// HWTEST_F(DocumentFindApiTest, DocumentFindApiTest005, TestSize.Level1)
// {
//     /**
//      * @tc.steps: step1. Create filter with _id and number
//      * @tc.expected: step1. Faild to get the record, the result is GRD_INVALID_ARGS,
//      */
//     GRD_ResultSet *resultSet1 = nullptr;
//     const char *filter1 = "{\"_id\" : \"1\", \"info\" : 1}";
//     Query query1 = {filter1, "{}"};
//     EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query1, 1, &resultSet1), GRD_OK);
//     EXPECT_EQ(GRD_FreeResultSet(resultSet1), GRD_OK);

//     /**
//      * @tc.steps: step2. Create filter with two _id
//      * @tc.expected: step2. Faild to get the record, the result is GRD_INVALID_ARGS,
//      */
//     GRD_ResultSet *resultSet2 = nullptr;
//     const char *filter2 = "{\"_id\" : \"1\", \"_id\" : \"2\"}";
//     Query query2 = {filter2, "{}"};
//     EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query2, 1, &resultSet2), GRD_INVALID_ARGS);
//     EXPECT_EQ(GRD_FreeResultSet(resultSet1), GRD_OK);

//     /**
//      * @tc.steps: step3. Create filter with array and _id
//      * @tc.expected: step3. Faild to get the record, the result is GRD_INVALID_ARGS,
//      */
//     GRD_ResultSet *resultSet3 = nullptr;
//     const char *filter3 = "{\"_id\" : \"1\", \"info\" : [\"2\", 1]}";
//     Query query3 = {filter3, "{}"};
//     EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query3, 1, &resultSet3), GRD_OK);

//     /**
//      * @tc.steps: step4. Create filter with object and _id
//      * @tc.expected: step4. Faild to get the record, the result is GRD_INVALID_ARGS,
//      */
//     GRD_ResultSet *resultSet4 = nullptr;
//     const char *filter4 = "{\"_id\" : \"1\", \"info\" : {\"info_val\" : \"1\"}}";
//     Query query4 = {filter4, "{}"};
//     EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query4, 1, &resultSet4), GRD_OK);

//     /**
//      * @tc.steps: step5. Create filter with bool and _id
//      * @tc.expected: step5. Faild to get the record, the result is GRD_INVALID_ARGS,
//      */
//     GRD_ResultSet *resultSet5 = nullptr;
//     const char *filter5 = "{\"_id\" : \"1\", \"info\" : true}";
//     Query query5 = {filter5, "{}"};
//     EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query5, 1, &resultSet5), GRD_OK);

//     /**
//      * @tc.steps: step6. Create filter with null and _id
//      * @tc.expected: step6. Faild to get the record, the result is GRD_INVALID_ARGS,
//      */
//     GRD_ResultSet *resultSet6 = nullptr;
//     const char *filter6 = "{\"_id\" : \"1\", \"info\" : null}";
//     Query query6 = {filter6, "{}"};
//     EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query6, 1, &resultSet6), GRD_OK);
// }

/**
  * @tc.name: DocumentFindApiTest006
  * @tc.desc: test filter field with id which has different type of value.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest006, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter with _id which value is string
     * @tc.expected: step1. Faild to get the record, the result is GRD_INVALID_ARGS,
     */
    GRD_ResultSet *resultSet1 = nullptr;
    const char *filter1 = "{\"_id\" : \"valstring\"}";
    Query query1 = { filter1, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query1, 1, &resultSet1), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet1), GRD_OK);

    /**
     * @tc.steps: step2. Create filter with _id which value is number
     * @tc.expected: step2. Faild to get the record, the result is GRD_INVALID_ARGS,
     */
    GRD_ResultSet *resultSet2 = nullptr;
    const char *filter2 = "{\"_id\" : 1}";
    Query query2 = { filter2, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query2, 1, &resultSet2), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step3. Create filter with _id which value is array
     * @tc.expected: step3. Faild to get the record, the result is GRD_INVALID_ARGS,
     */
    GRD_ResultSet *resultSet3 = nullptr;
    const char *filter3 = "{\"_id\" : [\"2\", 1]}";
    Query query3 = { filter3, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query3, 1, &resultSet3), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step4. Create filter with _id which value is object
     * @tc.expected: step4. Faild to get the record, the result is GRD_INVALID_ARGS,
     */
    GRD_ResultSet *resultSet4 = nullptr;
    const char *filter4 = "{\"_id\" : {\"info_val\" : \"1\"}}";
    Query query4 = { filter4, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query4, 1, &resultSet4), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step5. Create filter with _id which value is bool
     * @tc.expected: step5. Faild to get the record, the result is GRD_INVALID_ARGS,
     */
    GRD_ResultSet *resultSet5 = nullptr;
    const char *filter5 = "{\"_id\" : true}";
    Query query5 = { filter5, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query5, 1, &resultSet5), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step6. Create filter with _id which value is null
     * @tc.expected: step6. Faild to get the record, the result is GRD_INVALID_ARGS,
     */
    GRD_ResultSet *resultSet6 = nullptr;
    const char *filter6 = "{\"_id\" : null}";
    Query query6 = { filter6, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query6, 1, &resultSet6), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentFindApiTest016
  * @tc.desc: Test filter with collection Name is invalid.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest016, TestSize.Level1)
{
    const char *colName1 = "grd_type";
    const char *colName2 = "GM_SYS_sysfff";
    GRD_ResultSet *resultSet = NULL;
    const char *filter = "{\"_id\" : \"1\"}";
    Query query = { filter, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, colName1, query, 1, &resultSet), GRD_INVALID_FORMAT);
    EXPECT_EQ(GRD_FindDoc(g_db, colName2, query, 1, &resultSet), GRD_INVALID_FORMAT);
}

// /**
//   * @tc.name: DocumentFindApiTest017
//   * @tc.desc: Test filter field with large filter
//   * @tc.type: FUNC
//   * @tc.require:
//   * @tc.author: mazhao
//   */
// HWTEST_F(DocumentFindApiTest, DocumentFindApiTest017, TestSize.Level1)
// {
//     GRD_ResultSet *resultSet = nullptr;
//     string documentPart1 = "{\"_id\" : \"18\", \"item\" :\" ";
//     string documentPart2 = "\" }";
//     string jsonVal = string(512 * 1024 - documentPart1.size() - documentPart2.size() - 1, 'k');
//     string document = documentPart1 + jsonVal + documentPart2;
//     string jsonVal1 = string(512 * 1024 - documentPart1.size() - documentPart2.size(), 'k');
//     string document1 = documentPart1 + jsonVal1 + documentPart2;

//     Query query = {document.c_str(), "{}"};
//     EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);
//     EXPECT_EQ(GRD_Next(resultSet), GRD_NO_DATA);
//     char *value = NULL;
//     EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_NOT_AVAILABLE);
//     EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);

//     query = {document1.c_str(), "{}"};
//     EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OVER_LIMIT);
// }

/**
  * @tc.name: DocumentFindApiTest019
  * @tc.desc: Test filter field with no result
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest019, TestSize.Level1)
{
    const char *filter = "{\"_id\" : \"100\"}";
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_NO_DATA);
    char *value = NULL;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_NOT_AVAILABLE);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest023
  * @tc.desc: Test filter field with double find.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest023, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter with _id and get the record according to filter condition.
     * @tc.expected: step1. succeed to get the record, the matching record is g_document6.
     */
    const char *filter = "{\"_id\" : \"6\"}";
    GRD_ResultSet *resultSet = nullptr;
    GRD_ResultSet *resultSet2 = nullptr;
    Query query = { filter, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet2), GRD_RESOURCE_BUSY);

    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = NULL;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, g_document6);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    /**
     * @tc.steps: step2. Invoke GRD_Next to get the next matching value. Release resultSet.
     * @tc.expected: step2. Cannot get next record, return GRD_NO_DATA.
     */
    EXPECT_EQ(GRD_Next(resultSet), GRD_NO_DATA);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_NOT_AVAILABLE);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, g_document6);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest024
  * @tc.desc: Test filter field with multi collections
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest024, TestSize.Level1)
{
    const char *filter = "{\"_id\" : \"6\"}";
    GRD_ResultSet *resultSet = nullptr;
    GRD_ResultSet *resultSet2 = nullptr;
    Query query = { filter, "{}" };
    const char *collectionName = "DocumentFindApiTest024";
    EXPECT_EQ(GRD_CreateCollection(g_db, collectionName, "", 0), GRD_OK);
    InsertData(g_db, collectionName);
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_FindDoc(g_db, collectionName, query, 1, &resultSet2), GRD_OK);

    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = NULL;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, g_document6);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);

    EXPECT_EQ(GRD_Next(resultSet2), GRD_OK);
    char *value2 = NULL;
    EXPECT_EQ(GRD_GetValue(resultSet2, &value2), GRD_OK);
    CompareValue(value2, g_document6);
    EXPECT_EQ(GRD_FreeValue(value2), GRD_OK);

    EXPECT_EQ(GRD_Next(resultSet), GRD_NO_DATA);
    EXPECT_EQ(GRD_Next(resultSet2), GRD_NO_DATA);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_NOT_AVAILABLE);
    EXPECT_EQ(GRD_GetValue(resultSet2, &value), GRD_NOT_AVAILABLE);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet2), GRD_OK);

    EXPECT_EQ(GRD_DropCollection(g_db, collectionName, 0), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest025
  * @tc.desc: Test nested projection, with viewType equals to 1.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest025, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter to match g_document16, _id flag is 0.
     * Create projection to display name,nested4.
     * @tc.expected: step1. resultSet init successfuly, the result is GRD_OK,
     */
    const char *filter = "{\"_id\" : \"16\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"name\": true, \"nested1.nested2.nested3.nested4\":true}";
    const char *targetDocument = "{\"name\":\"doc16\", \"nested1\":{\"nested2\":{\"nested3\":\
        {\"nested4\":\"ABC\"}}}}";
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    /**
     * @tc.steps: step2. After loop, cannot get more record.
     * @tc.expected: step2. Return GRD_NO_DATA.
     */
    EXPECT_EQ(GRD_Next(resultSet), GRD_NO_DATA);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_NOT_AVAILABLE);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
    /**
     * @tc.steps: step3. Create filter to match g_document16, _id flag is 0;
     * Create projection to display name、nested4 with different projection format.
     * @tc.expected: step3. succeed to get the record.
     */
    projectionInfo = "{\"name\": true, \"nested1\":{\"nested2\":{\"nested3\":{\"nested4\":true}}}}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    /**
     * @tc.steps: step4. After loop, cannot get more record.
     * @tc.expected: step4. return GRD_NO_DATA.
     */
    EXPECT_EQ(GRD_Next(resultSet), GRD_NO_DATA);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_NOT_AVAILABLE);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
    /**
     * @tc.steps: step5. Create filter to match g_document16, _id flag is 0.
     * Create projection to conceal name,nested4 with different projection format.
     * @tc.expected: step5. succeed to get the record.
     */
    projectionInfo = "{\"name\": 0, \"nested1.nested2.nested3.nested4\":0}";
    targetDocument = "{\"nested1\":{\"nested2\":{\"nested3\":{\"field2\":\"CCC\"}}}}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}
#include <stdio.h>
/**
  * @tc.name: DocumentFindApiTest026
  * @tc.desc: Test nested projection, with _id flag equals to 1. Projection is 5 level.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest026, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter to match g_document16, _id flag is 0.
     * Create projection to display name,nested5
     * @tc.expected: step1. Error GRD_INVALID_ARGS.
     */
    const char *filter = "{\"_id\" : \"16\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"name\": true, \"nested1.nested2.nested3.nested4.nested5\":true}";
    Query query = { filter, projectionInfo };
    //EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_ARGS);
    // EXPECT_EQ(GRD_Next(resultSet), GRD_INVALID_ARGS);
    // char *value = nullptr;
    // EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_INVALID_ARGS);
    /**
     * @tc.steps: step2. After loop, cannot get more record.
     * @tc.expected: step2. Return GRD_NO_DATA.
     */
    // EXPECT_EQ(GRD_Next(resultSet), GRD_INVALID_ARGS);
    // EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentFindApiTest027
  * @tc.desc: Test projection with invalid field, _id field equals to 1.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest027, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter to match g_document7, _id flag is 0.
     * Create projection to display name, other _info and non existing field.
     * @tc.expected: step1. Match the g_document7 and display name, other_info
     */
    const char *filter = "{\"_id\" : \"7\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"name\": true, \"other_Info\":true, \"non_exist_field\":true}";
    const char *targetDocument = "{\"name\": \"doc7\", \"other_Info\":[{\"school\":\"BX\", \"age\":15},\
        {\"school\":\"C\", \"age\":35}]}";
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);

    /**
     * @tc.steps: step2. Create filter to match g_document7, _id flag is 0.
     * Create projection to display name, other _info and existing field with space.
     * @tc.expected: step2. Return GRD_INVALID_ARGS.
     */
    projectionInfo = "{\"name\": true, \"other_Info\":true, \" item \":true}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step3. Create filter to match g_document7, _id flag is 0.
     * Create projection to display name, other _info and existing field with different case.
     * @tc.expected: step3. Match the g_document7 and display name, other_Info.
     */
    projectionInfo = "{\"name\": true, \"other_Info\":true, \"ITEM\": true}";
    query = { filter, projectionInfo };
    resultSet = nullptr;
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest028
  * @tc.desc: Test projection with invalid field in Array,_id field equals to 1.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest028, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter to match g_document7, _id flag is 0.
     * Create projection to display name, non existing field in array.
     * @tc.expected: step1. Match the g_document7 and display name, other_info.
     */
    const char *filter = "{\"_id\" : \"7\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"name\": true, \"other_Info.non_exist_field\":true}";
    const char *targetDocument = "{\"name\": \"doc7\"}";
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);

    /**
     * @tc.steps: step2. Create filter to match g_document7, _id flag is 0.
     * Create projection to display name, other _info and existing field with space.
     * @tc.expected: step2. Return GRD_INVALID_ARGS.
     */
    projectionInfo = "{\"name\": true, \"other_Info\":{\"non_exist_field\":true}}";
    query = { filter, projectionInfo };
    resultSet = nullptr;
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);

    /**
     * @tc.steps: step3. Create filter to match g_document7, _id flag is 0.
     * Create projection to display name, non existing field in array with index format.
     * @tc.expected: step3. Match the g_document7 and display name, other_Info.
     */
    projectionInfo = "{\"name\": true, \"other_Info.0\": true}";
    query = { filter, projectionInfo };
    resultSet = nullptr;
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_ARGS);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentFindApiTest029
  * @tc.desc: Test projection with path conflict._id field equals to 0.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest029, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter to match g_document4, _id flag is 0.
     * Create projection to display conflict path.
     * @tc.expected: step1. Return GRD_INVALID_ARGS.
     */
    const char *filter = "{\"_id\" : \"4\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"personInfo\": true, \"personInfo.grade\": true}";
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentFindApiTest030
  * @tc.desc: Test _id flag and field.None exist field.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest030, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter to match g_document7, _id flag is 0.
     * @tc.expected: step1. Match the g_document7 and return empty json.
     */
    const char *filter = "{\"_id\" : \"7\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"non_exist_field\":true}";
    int flag = 0;
    const char *targetDocument = "{}";
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);

    /**
     * @tc.steps: step2. Create filter to match g_document7, _id flag is 1.
     * @tc.expected: step2. Match g_document7, and return a json with _id.
     */
    resultSet = nullptr;
    flag = 1;
    targetDocument = "{\"_id\": \"7\"}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    value = NULL;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest031
  * @tc.desc: Test _id flag and field.Exist field with 1 value.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest031, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter to match g_document7, _id flag is 0.
     * @tc.expected: step1. Match the g_document7 and return json with name, item.
     */
    const char *filter = "{\"_id\" : \"7\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"name\":true, \"item\":true}";
    int flag = 0;
    const char *targetDocument = "{\"name\":\"doc7\", \"item\":\"fruit\"}";
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);

    /**
     * @tc.steps: step2. Create filter to match g_document7, _id flag is 1.
     * @tc.expected: step2. Match g_document7, and return a json with _id.
     */
    resultSet = nullptr;
    flag = 1;
    projectionInfo = "{\"name\": 1, \"item\": 1}";
    targetDocument = "{\"_id\":\"7\", \"name\":\"doc7\", \"item\":\"fruit\"}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    value = NULL;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest032
  * @tc.desc: Test _id flag and field.Exist field with 1 value.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest032, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter to match g_document7, _id flag is 0.
     * @tc.expected: step1. Match the g_document7 and return json with name, item.
     */
    const char *filter = "{\"_id\" : \"7\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"name\":true, \"item\":true}";
    int flag = 0;
    const char *targetDocument = "{\"name\":\"doc7\", \"item\":\"fruit\"}";
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
    /**
     * @tc.steps: step2. Create filter to match g_document7, _id flag is 1.
     * @tc.expected: step2. Match g_document7, and return a json with _id.
     */
    resultSet = nullptr;
    flag = 1;
    projectionInfo = "{\"name\": 1, \"item\": 1}";
    targetDocument = "{\"_id\":\"7\", \"name\":\"doc7\", \"item\":\"fruit\"}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    value = NULL;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
    /**
     * @tc.steps: step3. Create filter to match g_document7, _id flag is 1.Projection value is not 0.
     * @tc.expected: step3. Match g_document7, and return a json with name, item and _id.
     */
    resultSet = nullptr;
    flag = 1;
    projectionInfo = "{\"name\": 10, \"item\": 10}";
    targetDocument = "{\"_id\":\"7\", \"name\":\"doc7\", \"item\":\"fruit\"}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    value = NULL;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest033
  * @tc.desc: Test _id flag and field.Exist field with 0 value.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest033, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter to match g_document7, _id flag is 0.
     * @tc.expected: step1. Match the g_document7 and return json with name, item and _id
     */
    const char *filter = "{\"_id\" : \"7\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"name\":false, \"item\":false}";
    int flag = 0;
    const char *targetDocument = "{\"other_Info\":[{\"school\":\"BX\", \"age\" : 15}, {\"school\":\"C\", \"age\" : "
                                 "35}]}";
    ;
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
    /**
     * @tc.steps: step2. Create filter to match g_document7, _id flag is 1.
     * @tc.expected: step2. Match g_document7, and return a json without name and item.
     */
    resultSet = nullptr;
    flag = 1;
    projectionInfo = "{\"name\": 0, \"item\": 0}";
    targetDocument = "{\"_id\": \"7\", \"other_Info\":[{\"school\":\"BX\", \"age\" : 15}, {\"school\":\"C\", \"age\" "
                     ": 35}]}";
    ;
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    value = NULL;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest034
  * @tc.desc: Test projection with nonexist field in nested structure.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest034, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter to match g_document4, _id flag is 0.
     * @tc.expected: step1. Match the g_document4 and return json without name
     */
    const char *filter = "{\"_id\" : \"4\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"name\": 1, \"personInfo.grade1\": 1, \
            \"personInfo.shool1\": 1, \"personInfo.age1\": 1}";
    int flag = 0;
    const char *targetDocument = "{\"name\":\"doc4\"}";
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);

    /**
     * @tc.steps: step2. Create filter to match g_document4, _id flag is 0, display part of fields in nested structure.
     * @tc.expected: step2. Match the g_document4 and return json without name
     */
    projectionInfo = "{\"name\": false, \"personInfo.grade1\": false, \
            \"personInfo.shool1\": false, \"personInfo.age1\": false}";
    const char *targetDocument2 = "{\"item\":\"paper\",\"personInfo\":{\"grade\" : 1, \"school\":\"A\", \"age\" : "
                                  "18}}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument2);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);

    /**
     * @tc.steps: step3. Create filter to match g_document4, _id flag is 0, display part of fields in nested structure.
     * @tc.expected: step3. Match the g_document4 and return json with name, personInfo.school and personInfo.age.
     */
    projectionInfo = "{\"name\": 1, \"personInfo.school\": 1, \"personInfo.age\": 1}";
    const char *targetDocument3 = "{\"name\":\"doc4\", \"personInfo\": {\"school\":\"A\", \"age\" : 18}}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument3);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);

    /**
     * @tc.steps: step4. Create filter to match g_document4, _id flag is 0, display part of fields in nested structure.
     * @tc.expected: step4. Match the g_document4 and return json with name, personInfo.school
     */
    projectionInfo = "{\"name\": 1, \"personInfo.school\": 1, \"personInfo.age1\": 1}";
    const char *targetDocument4 = "{\"name\":\"doc4\", \"personInfo\": {\"school\":\"A\"}}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument4);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest035
  * @tc.desc: test filter with id string filter
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest035, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter with _id and get the record according to filter condition.
     * @tc.expected: step1. succeed to get the record, the matching record is g_document17
     */
    const char *filter = "{\"_id\" : \"17\"}";
    GRD_ResultSet *resultSet = nullptr;
    int flag = 0;
    Query query = { filter, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, GRD_DOC_ID_DISPLAY, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, g_document17);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    /**
     * @tc.steps: step2. Invoke GRD_Next to get the next matching value. Release resultSet.
     * @tc.expected: step2. Cannot get next record, return GRD_NO_DATA.
     */
    EXPECT_EQ(GRD_Next(resultSet), GRD_NO_DATA);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_NOT_AVAILABLE);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest036
  * @tc.desc: Test with invalid collectionName.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest036, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Test with invalid collectionName.
     * @tc.expected: step1. Return GRD_INVALID_ARGS.
     */
    const char *filter = "{\"_id\" : \"17\"}";
    GRD_ResultSet *resultSet = nullptr;
    int flag = 0;
    Query query = { filter, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, "", query, 0, &resultSet), GRD_INVALID_ARGS);
    EXPECT_EQ(GRD_FindDoc(g_db, NULL, query, 0, &resultSet), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentFindApiTest037
  * @tc.desc: Test filed with different value.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest037, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Test filed with different value.some are 1, other are 0.
     * @tc.expected: step1. Return GRD_INVALID_ARGS.
     */
    const char *filter = "{\"_id\" : \"4\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"name\":1, \"personInfo\":0, \"item\":1}";
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step2. Test filed with different value.some are 2, other are 0.
     * @tc.expected: step2. Return GRD_INVALID_ARGS.
     */
    projectionInfo = "{\"name\":2, \"personInfo\":0, \"item\":2}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step3. Test filed with different value.some are 0, other are true.
     * @tc.expected: step3. Return GRD_INVALID_ARGS.
     */
    projectionInfo = "{\"name\":true, \"personInfo\":0, \"item\":true}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step4. Test filed with different value.some are 0, other are "".
     * @tc.expected: step4. Return GRD_INVALID_ARGS.
     */
    projectionInfo = "{\"name\":\"\", \"personInfo\":0, \"item\":\"\"}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step5. Test filed with different value.some are 1, other are false.
     * @tc.expected: step5. Return GRD_INVALID_ARGS.
     */
    projectionInfo = "{\"name\":false, \"personInfo\":1, \"item\":false";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_FORMAT);

    /**
     * @tc.steps: step6. Test filed with different value.some are -1.123, other are false.
     * @tc.expected: step6. Return GRD_INVALID_ARGS.
     */
    projectionInfo = "{\"name\":false, \"personInfo\":-1.123, \"item\":false";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_FORMAT);

    /**
     * @tc.steps: step7. Test filed with different value.some are true, other are false.
     * @tc.expected: step7. Return GRD_INVALID_ARGS.
     */
    projectionInfo = "{\"name\":false, \"personInfo\":true, \"item\":false";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_FORMAT);
}

/**
  * @tc.name: DocumentFindApiTest038
  * @tc.desc: Test field with false value.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest038, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Test field with different false value. Some are false, other are 0. flag is 0.
     * @tc.expected: step1. Match the g_document6 and return empty json.
     */
    const char *filter = "{\"_id\" : \"6\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"name\":false, \"personInfo\": 0, \"item\":0}";
    int flag = 0;
    const char *targetDocument = "{}";
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);

    /**
     * @tc.steps: step2. Test field with different false value.Some are false, others are 0. flag is 1.
     * @tc.expected: step2. Match g_document6, Return json with _id.
     */
    targetDocument = "{\"_id\": \"6\"}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest039
  * @tc.desc: Test field with true value.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest039, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Test field with different true value. Some are true, other are 1. flag is 0.
     * @tc.expected: step1. Match the g_document18 and return json with name, item, personInfo.age and color.
     */
    const char *filter = "{\"_id\" : \"18\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"name\":true, \"personInfo.age\": \"\", \"item\":1, \"color\":10, \"nonExist\" : "
                                 "-100}";
    const char *targetDocument = "{\"name\":\"doc18\", \"item\":\"mobile phone\", \"personInfo\":\
        {\"age\":66}, \"color\":\"blue\"}";
    int flag = 0;
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);

    /**
     * @tc.steps: step2. Test field with different true value.Some are false, others are 0. flag is 1.
     * @tc.expected: step2. Match g_document18, Return json with name, item, personInfo.age, color and _id.
     */
    targetDocument = "{\"_id\" : \"18\", \"name\":\"doc18\",\"item\" : \"mobile phone\",\"personInfo\":\
        {\"age\":66}, \"color\":\"blue\"}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest040
  * @tc.desc: Test field with invalid value.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest040, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Test field with invalid value.Value is array.
     * @tc.expected: step1. Match the g_document18 and return GRD_INVALID_ARGS.
     */
    const char *filter = "{\"_id\" : \"18\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"personInfo\":[true, 1]}";
    int flag = 1;
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step2. Test field with invalid value.Value is null.
     * @tc.expected: step2. Match the g_document18 and return GRD_INVALID_ARGS.
     */
    projectionInfo = "{\"personInfo\":null}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step3. Test field with invalid value.Value is invalid string.
     * @tc.expected: step3. Match the g_document18 and return GRD_INVALID_ARGS.
     */
    projectionInfo = "{\"personInfo\":\"invalid string.\"}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentFindApiTest042
  * @tc.desc: Test field with no existed uppercase filter
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest042, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Test field with no existed uppercase filter.
     * @tc.expected: step1. not match any item.
     */
    const char *filter = "{\"_iD\" : \"18\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"Name\":true, \"personInfo.age\": \"\", \"item\":1, \"COLOR\":10, \"nonExist\" : "
                                 "-100}";
    int flag = 0;
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_NO_DATA);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
    /**
     * @tc.steps: step2. Test field with upper projection.
     * @tc.expected: step2. Match g_document18, Return json with item, personInfo.age, color and _id.
     */
    const char *filter1 = "{\"_id\" : \"18\"}";
    const char *targetDocument = "{\"_id\" : \"18\", \"item\" : \"mobile phone\",\"personInfo\":\
        {\"age\":66}}";
    query = { filter1, projectionInfo };
    char *value = nullptr;
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_NO_DATA);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest044
  * @tc.desc: Test field with uppercase projection
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest044, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Test with false uppercase projection
     * @tc.expected: step1. Match g_document18, Return json with item, personInfo.age and _id.
     */
    const char *filter = "{\"_id\" : \"18\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"Name\":0, \"personInfo.age\": false, \"personInfo.SCHOOL\": false, \"item\":\
        false, \"COLOR\":false, \"nonExist\" : false}";
    const char *targetDocument = "{\"_id\" : \"18\", \"name\":\"doc18\", \"personInfo\":\
        {\"school\":\"DD\"}, \"color\":\"blue\"}";
    int flag = 0;
    Query query = { filter, projectionInfo };
    char *value = nullptr;
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_NO_DATA);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest045
  * @tc.desc: Test field with too long collectionName
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest045, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Test with false uppercase projection
     * @tc.expected: step1. Match g_document18, Return json with item, personInfo.age and _id.
     */
    const char *filter = "{\"_id\" : \"18\"}";
    GRD_ResultSet *resultSet = nullptr;
    int flag = 0;
    Query query = { filter, "{}" };
    string collectionName1(MAX_COLLECTION_NAME, 'a');
    ASSERT_EQ(GRD_CreateCollection(g_db, collectionName1.c_str(), "", 0), GRD_OK);
    EXPECT_EQ(GRD_FindDoc(g_db, collectionName1.c_str(), query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
    ASSERT_EQ(GRD_DropCollection(g_db, collectionName1.c_str(), 0), GRD_OK);

    string collectionName2(MAX_COLLECTION_NAME + 1, 'a');
    EXPECT_EQ(GRD_FindDoc(g_db, collectionName2.c_str(), query, 1, &resultSet), GRD_OVER_LIMIT);
    EXPECT_EQ(GRD_FindDoc(g_db, "", query, 1, &resultSet), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentFindApiTest052
  * @tc.desc: Test field when id string len is large than max
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest052, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Test with false uppercase projection
     * @tc.expected: step1. Match g_document18, Return json with item, personInfo.age and _id.
     */
    const char *filter = "{\"_id\" : \"18\"}";
    GRD_ResultSet *resultSet = nullptr;
    int flag = 0;
    Query query = { filter, "{}" };
    string collectionName1(MAX_COLLECTION_NAME, 'a');
    ASSERT_EQ(GRD_CreateCollection(g_db, collectionName1.c_str(), "", 0), GRD_OK);
    EXPECT_EQ(GRD_FindDoc(g_db, collectionName1.c_str(), query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
    ASSERT_EQ(GRD_DropCollection(g_db, collectionName1.c_str(), 0), GRD_OK);

    string collectionName2(MAX_COLLECTION_NAME + 1, 'a');
    EXPECT_EQ(GRD_FindDoc(g_db, collectionName2.c_str(), query, 1, &resultSet), GRD_OVER_LIMIT);
    EXPECT_EQ(GRD_FindDoc(g_db, "", query, 1, &resultSet), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentFindApiTest053
  * @tc.desc: Test with invalid flags
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest053, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Test with invalid flags which is 3.
     * @tc.expected: step1. Return GRD_INVALID_ARGS.
     */
    const char *filter = "{\"_id\" : \"18\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{}";
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 3, &resultSet), GRD_INVALID_ARGS);

    /**
     * @tc.steps:step1.parameter flags is int_max
     * @tc.expected:step1.GRD_INVALID_ARGS
    */
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, INT_MAX, &resultSet), GRD_INVALID_ARGS);

    /**
     * @tc.steps:step1.parameter flags is INT_MIN
     * @tc.expected:step1.GRD_INVALID_ARGS
    */
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, INT_MIN, &resultSet), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentFindApiTest054
  * @tc.desc: Test with null g_db and resultSet, filter.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest054, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Test with null g_db.
     * @tc.expected: step1. Return GRD_INVALID_ARGS.
     */
    const char *filter = "{\"_id\" : \"18\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{}";
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(nullptr, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step2. Test with null resultSet.
     * @tc.expected: step2. Return GRD_INVALID_ARGS.
     */
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, nullptr), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step1. Test with query that has two nullptr data.
     * @tc.expected: step1. Return GRD_INVALID_ARGS.
     */
    query = { nullptr, nullptr };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentFindApiTest055
  * @tc.desc: Find doc, but filter' _id value lens is larger than MAX_ID_LENS
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest055, TestSize.Level1)
{
    /**
     * @tc.steps:step1.Find doc, but filter' _id value lens is larger than MAX_ID_LENS
     * @tc.expected:step1.GRD_OVER_LIMIT.
     */
    string document1 = "{\"_id\" : ";
    string document2 = "\"";
    string document4 = "\"";
    string document5 = "}";
    string document_midlle(MAX_ID_LENS + 1, 'k');
    string filter = document1 + document2 + document_midlle + document4 + document5;
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{}";
    Query query = { filter.c_str(), projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_OVER_LIMIT);

    /**
     * @tc.steps:step1.Find doc, filter' _id value lens is equal as MAX_ID_LENS
     * @tc.expected:step1.GRD_OK.
     */
    string document_midlle2(MAX_ID_LENS, 'k');
    filter = document1 + document2 + document_midlle2 + document4 + document5;
    query = { filter.c_str(), projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest056
  * @tc.desc: Test findDoc with no _id.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest056, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter with _id and get the record according to filter condition.
     * @tc.expected: step1. Succeed to get the record, the matching record is g_document6.
     */
    const char *filter = "{\"personInfo\" : {\"school\":\"B\"}}";
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = NULL;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, g_document6);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    /**
     * @tc.steps: step2. Invoke GRD_Next to get the next matching value. Release resultSet.
     * @tc.expected: step2. Cannot get next record, return GRD_NO_DATA.
     */
    // EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_NOT_AVAILABLE);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentFindApiTest056
  * @tc.desc: Test findDoc with no _id.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest057, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter with _id and get the record according to filter condition.
     * @tc.expected: step1. Succeed to get the record, the matching record is g_document6.
     */
    const char *filter = "{\"personInfo\" : {\"school\":\"B\"}}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projection = "{\"version\": 1}";
    Query query = { filter, projection };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = NULL;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    //CompareValue(value, g_document6);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    /**
     * @tc.steps: step2. Invoke GRD_Next to get the next matching value. Release resultSet.
     * @tc.expected: step2. Cannot get next record, return GRD_NO_DATA.
     */
    // EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_NOT_AVAILABLE);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}
/**
  * @tc.name: DocumentFindApiTest058
  * @tc.desc: Test findDoc with no _id.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentFindApiTest, DocumentFindApiTest058, TestSize.Level1) {}

HWTEST_F(DocumentFindApiTest, DocumentFindApiTest059, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter with _id and get the record according to filter condition.
     * @tc.expected: step1. Succeed to get the record, the matching record is g_document6.
     */
    const char *filter = "{\"personInfo\" : {\"school\":\"B\"}}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projection = R"({"a00001":1, "a00001":1})";
    Query query = { filter, projection };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_INVALID_FORMAT);
}