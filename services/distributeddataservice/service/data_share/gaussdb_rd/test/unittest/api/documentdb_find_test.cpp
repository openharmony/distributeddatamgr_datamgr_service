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
#include <cstdio>
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
using namespace DocumentDBUnitTest;

namespace {
std::string g_path = "./document.db";
GRD_DB *g_db = nullptr;
constexpr const char *COLLECTION_NAME = "student";
const int MAX_COLLECTION_NAME = 511;

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
    [{\"school\":\"B\", \"teacher\" : \"mike\", \"age\" : 15},\
    {\"school\":\"C\", \"teacher\" : \"moon\", \"age\" : 20}]}";

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
static const char *document0631 = R""({"_id":"city_11_com.acts.ohos.data.datasharetestclient_100",
      "bundleName":"com.acts.ohos.data.datasharetestclient","key":"city","subscriberId":11,
      "timestamp":1509100700,"userId":100,"value":{"type":1,"value":"xian"},"version":0})"";
static const char *document0632 = R""({"_id":"datashareproxy://com.acts.ohos.data.datasharetest/appInfo_11_com.acts.oh
    os.data.datasharetest_100","bundleName":"com.acts.ohos.data.datasharetest","key":"datashareproxy://com.acts.ohos
    .data.datasharetest/appInfo","subscriberId":11,"timestamp":1509100700,"userId":100,
    "value":{"type":1,"value":"\"Qing\""},"version":0})"";
static const char *document0633 =
    R""({"_id":"empty_11_com.acts.ohos.data.datasharetestclient_100","bundleName":"com.acts.ohos.data.datasharetestcl
    ient","key":"empty","subscriberId":11,"timestamp":1509100700,"userId":100,"value":{"type":1,"value":"nobody sub"},
    "version":0})"";
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
    EXPECT_EQ(errCode, DocumentDB::E_OK);
    DocumentDB::JsonObject targetValueObj = DocumentDB::JsonObject::Parse(targetValue, errCode);
    EXPECT_EQ(errCode, DocumentDB::E_OK);
    EXPECT_EQ(valueObj.Print(), targetValueObj.Print());
}

class DocumentDBFindTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    void InsertDoc(const char *collectionName, const char *document);
};
void DocumentDBFindTest::SetUpTestCase(void)
{
    int status = GRD_DBOpen(g_path.c_str(), nullptr, GRD_DB_OPEN_CREATE, &g_db);
    EXPECT_EQ(status, GRD_OK);
    EXPECT_EQ(GRD_CreateCollection(g_db, COLLECTION_NAME, "", 0), GRD_OK);
    EXPECT_NE(g_db, nullptr);
}

void DocumentDBFindTest::TearDownTestCase(void)
{
    EXPECT_EQ(GRD_DBClose(g_db, 0), GRD_OK);
    DocumentDBTestUtils::RemoveTestDbFiles(g_path);
}

void DocumentDBFindTest::SetUp(void)
{
    EXPECT_EQ(GRD_DropCollection(g_db, COLLECTION_NAME, 0), GRD_OK);
    EXPECT_EQ(GRD_CreateCollection(g_db, COLLECTION_NAME, "", 0), GRD_OK);
    InsertData(g_db, "student");
}

void DocumentDBFindTest::TearDown(void) {}

/**
  * @tc.name: DocumentDBFindTest001
  * @tc.desc: Test Insert document db
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest001, TestSize.Level1)
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
    char *value = nullptr;
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
  * @tc.name: DocumentDBFindTest002
  * @tc.desc: Test filter with multiple fields and _id.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter with multiple and _id. and get the record according to filter condition.
     * @tc.expected: step1. Failed to get the record, the result is GRD_INVALID_ARGS,
     *     GRD_GetValue return GRD_NOT_AVAILABLE and GRD_Next return GRD_NO_DATA.
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
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentDBFindTest004
  * @tc.desc: test filter with string filter without _id.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter without _id and get the record according to filter condition.
     * @tc.expected: step1. Failed to get the record, the result is GRD_INVALID_ARGS,
     */
    const char *filter = "{\"name\":\"doc6\"}";
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);

    /**
     * @tc.steps: step2. Invoke GRD_Next to get the next matching value. Release resultSet.
     * @tc.expected: step2. GRD_GetValue return GRD_INVALID_ARGS and GRD_Next return GRD_INVALID_ARGS.
     */
    char *value = nullptr;
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentDBFindTest006
  * @tc.desc: test filter field with id which has different type of value.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest006, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter with _id which value is string
     * @tc.expected: step1. Failed to get the record, the result is GRD_INVALID_ARGS,
     */
    GRD_ResultSet *resultSet1 = nullptr;
    const char *filter1 = "{\"_id\" : \"valstring\"}";
    Query query1 = { filter1, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query1, 1, &resultSet1), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet1), GRD_OK);

    /**
     * @tc.steps: step2. Create filter with _id which value is number
     * @tc.expected: step2. Failed to get the record, the result is GRD_INVALID_ARGS,
     */
    GRD_ResultSet *resultSet2 = nullptr;
    const char *filter2 = "{\"_id\" : 1}";
    Query query2 = { filter2, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query2, 1, &resultSet2), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step3. Create filter with _id which value is array
     * @tc.expected: step3. Failed to get the record, the result is GRD_INVALID_ARGS,
     */
    GRD_ResultSet *resultSet3 = nullptr;
    const char *filter3 = "{\"_id\" : [\"2\", 1]}";
    Query query3 = { filter3, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query3, 1, &resultSet3), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step4. Create filter with _id which value is object
     * @tc.expected: step4. Failed to get the record, the result is GRD_INVALID_ARGS,
     */
    GRD_ResultSet *resultSet4 = nullptr;
    const char *filter4 = "{\"_id\" : {\"info_val\" : \"1\"}}";
    Query query4 = { filter4, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query4, 1, &resultSet4), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step5. Create filter with _id which value is bool
     * @tc.expected: step5. Failed to get the record, the result is GRD_INVALID_ARGS,
     */
    GRD_ResultSet *resultSet5 = nullptr;
    const char *filter5 = "{\"_id\" : true}";
    Query query5 = { filter5, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query5, 1, &resultSet5), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step6. Create filter with _id which value is null
     * @tc.expected: step6. Failed to get the record, the result is GRD_INVALID_ARGS,
     */
    GRD_ResultSet *resultSet6 = nullptr;
    const char *filter6 = "{\"_id\" : null}";
    Query query6 = { filter6, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query6, 1, &resultSet6), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentDBFindTest016
  * @tc.desc: Test filter with collection Name is invalid.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest016, TestSize.Level1)
{
    const char *colName1 = "grd_type";
    const char *colName2 = "GM_SYS_sysfff";
    GRD_ResultSet *resultSet = nullptr;
    const char *filter = "{\"_id\" : \"1\"}";
    Query query = { filter, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, colName1, query, 1, &resultSet), GRD_INVALID_FORMAT);
    EXPECT_EQ(GRD_FindDoc(g_db, colName2, query, 1, &resultSet), GRD_INVALID_FORMAT);
}

/**
  * @tc.name: DocumentDBFindTest019
  * @tc.desc: Test filter field with no result
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest019, TestSize.Level1)
{
    const char *filter = "{\"_id\" : \"100\"}";
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_NO_DATA);
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_NOT_AVAILABLE);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentDBFindTest023
  * @tc.desc: Test filter field with double find.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest023, TestSize.Level1)
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
    char *value = nullptr;
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
  * @tc.name: DocumentDBFindTest024
  * @tc.desc: Test filter field with multi collections
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest024, TestSize.Level1)
{
    const char *filter = "{\"_id\" : \"6\"}";
    GRD_ResultSet *resultSet = nullptr;
    GRD_ResultSet *resultSet2 = nullptr;
    Query query = { filter, "{}" };
    const char *collectionName = "DocumentDBFindTest024";
    EXPECT_EQ(GRD_CreateCollection(g_db, collectionName, "", 0), GRD_OK);
    InsertData(g_db, collectionName);
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_FindDoc(g_db, collectionName, query, 1, &resultSet2), GRD_OK);

    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, g_document6);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);

    EXPECT_EQ(GRD_Next(resultSet2), GRD_OK);
    char *value2 = nullptr;
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
  * @tc.name: DocumentDBFindTest025
  * @tc.desc: Test nested projection, with viewType equals to 1.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest025, TestSize.Level1)
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

/**
  * @tc.name: DocumentDBFindTest027
  * @tc.desc: Test projection with invalid field, _id field equals to 1.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest027, TestSize.Level1)
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
  * @tc.name: DocumentDBFindTest028
  * @tc.desc: Test projection with invalid field in Array,_id field equals to 1.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest028, TestSize.Level1)
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
  * @tc.name: DocumentDBFindTest029
  * @tc.desc: Test projection with path conflict._id field equals to 0.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest029, TestSize.Level1)
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
  * @tc.name: DocumentDBFindTest030
  * @tc.desc: Test _id flag and field.None exist field.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest030, TestSize.Level1)
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
    value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentDBFindTest031
  * @tc.desc: Test _id flag and field.Exist field with 1 value.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest031, TestSize.Level1)
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
    value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentDBFindTest032
  * @tc.desc: Test _id flag and field.Exist field with 1 value.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest032, TestSize.Level1)
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
    value = nullptr;
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
    value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentDBFindTest033
  * @tc.desc: Test _id flag and field.Exist field with 0 value.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest033, TestSize.Level1)
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
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, flag, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, targetDocument);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentDBFindTest034
  * @tc.desc: Test projection with nonexist field in nested structure.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest034, TestSize.Level1)
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
  * @tc.name: DocumentDBFindTest035
  * @tc.desc: test filter with id string filter
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest035, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create filter with _id and get the record according to filter condition.
     * @tc.expected: step1. succeed to get the record, the matching record is g_document17
     */
    const char *filter = "{\"_id\" : \"17\"}";
    GRD_ResultSet *resultSet = nullptr;
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
  * @tc.name: DocumentDBFindTest036
  * @tc.desc: Test with invalid collectionName.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest036, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Test with invalid collectionName.
     * @tc.expected: step1. Return GRD_INVALID_ARGS.
     */
    const char *filter = "{\"_id\" : \"17\"}";
    GRD_ResultSet *resultSet = nullptr;
    Query query = { filter, "{}" };
    EXPECT_EQ(GRD_FindDoc(g_db, "", query, 0, &resultSet), GRD_INVALID_ARGS);
    EXPECT_EQ(GRD_FindDoc(g_db, nullptr, query, 0, &resultSet), GRD_INVALID_ARGS);
}

/**
  * @tc.name: DocumentDBFindTest037
  * @tc.desc: Test field with different value.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest037, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Test field with different value.some are 1, other are 0.
     * @tc.expected: step1. Return GRD_INVALID_ARGS.
     */
    const char *filter = "{\"_id\" : \"4\"}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projectionInfo = "{\"name\":1, \"personInfo\":0, \"item\":1}";
    Query query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step2. Test field with different value.some are 2, other are 0.
     * @tc.expected: step2. Return GRD_INVALID_ARGS.
     */
    projectionInfo = "{\"name\":2, \"personInfo\":0, \"item\":2}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step3. Test field with different value.some are 0, other are true.
     * @tc.expected: step3. Return GRD_INVALID_ARGS.
     */
    projectionInfo = "{\"name\":true, \"personInfo\":0, \"item\":true}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step4. Test field with different value.some are 0, other are "".
     * @tc.expected: step4. Return GRD_INVALID_ARGS.
     */
    projectionInfo = "{\"name\":\"\", \"personInfo\":0, \"item\":\"\"}";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_ARGS);

    /**
     * @tc.steps: step5. Test field with different value.some are 1, other are false.
     * @tc.expected: step5. Return GRD_INVALID_ARGS.
     */
    projectionInfo = "{\"name\":false, \"personInfo\":1, \"item\":false";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_FORMAT);

    /**
     * @tc.steps: step6. Test field with different value.some are -1.123, other are false.
     * @tc.expected: step6. Return GRD_INVALID_ARGS.
     */
    projectionInfo = "{\"name\":false, \"personInfo\":-1.123, \"item\":false";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_FORMAT);

    /**
     * @tc.steps: step7. Test field with different value.some are true, other are false.
     * @tc.expected: step7. Return GRD_INVALID_ARGS.
     */
    projectionInfo = "{\"name\":false, \"personInfo\":true, \"item\":false";
    query = { filter, projectionInfo };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 0, &resultSet), GRD_INVALID_FORMAT);
}

/**
  * @tc.name: DocumentDBFindTest038
  * @tc.desc: Test field with false value.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest038, TestSize.Level1)
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
  * @tc.name: DocumentDBFindTest039
  * @tc.desc: Test field with true value.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest039, TestSize.Level1)
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
  * @tc.name: DocumentDBFindTest040
  * @tc.desc: Test field with invalid value.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest040, TestSize.Level1)
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
  * @tc.name: DocumentDBFindTest042
  * @tc.desc: Test field with no existed uppercase filter
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest042, TestSize.Level1)
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
  * @tc.name: DocumentDBFindTest044
  * @tc.desc: Test field with uppercase projection
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest044, TestSize.Level1)
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
  * @tc.name: DocumentDBFindTest045
  * @tc.desc: Test field with too long collectionName
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest045, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Test with false uppercase projection
     * @tc.expected: step1. Match g_document18, Return json with item, personInfo.age and _id.
     */
    const char *filter = "{\"_id\" : \"18\"}";
    GRD_ResultSet *resultSet = nullptr;
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
  * @tc.name: DocumentDBFindTest052
  * @tc.desc: Test field when id string len is large than max
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest052, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Test with false uppercase projection
     * @tc.expected: step1. Match g_document18, Return json with item, personInfo.age and _id.
     */
    const char *filter = "{\"_id\" : \"18\"}";
    GRD_ResultSet *resultSet = nullptr;
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
  * @tc.name: DocumentDBFindTest053
  * @tc.desc: Test with invalid flags
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest053, TestSize.Level1)
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
  * @tc.name: DocumentDBFindTest054
  * @tc.desc: Test with null g_db and resultSet, filter.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest054, TestSize.Level1)
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
  * @tc.name: DocumentDBFindTest055
  * @tc.desc: Find doc, but filter' _id value lens is larger than MAX_ID_LENS
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest055, TestSize.Level1)
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
  * @tc.name: DocumentDBFindTest056
  * @tc.desc: Test findDoc with no _id.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest056, TestSize.Level1)
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
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, g_document5);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    /**
     * @tc.steps: step2. Invoke GRD_Next to get the next matching value. Release resultSet.
     * @tc.expected: step2. Cannot get next record, return GRD_NO_DATA.
     */
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

/**
  * @tc.name: DocumentDBFindTest056
  * @tc.desc: Test findDoc with no _id.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest057, TestSize.Level1)
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
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    /**
     * @tc.steps: step2. Invoke GRD_Next to get the next matching value. Release resultSet.
     * @tc.expected: step2. Cannot get next record, return GRD_NO_DATA.
     */
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}
/**
  * @tc.name: DocumentDBFindTest058
  * @tc.desc: Test findDoc with no _id.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DocumentDBFindTest, DocumentDBFindTest058, TestSize.Level1) {}

HWTEST_F(DocumentDBFindTest, DocumentDBFindTest059, TestSize.Level1)
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

HWTEST_F(DocumentDBFindTest, DocumentDBFindTest060, TestSize.Level1)
{
    const char *filter = "{}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projection = R"({"abc123_.":1})";
    Query query = { filter, projection };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_INVALID_ARGS);
}

HWTEST_F(DocumentDBFindTest, DocumentDBFindTest061, TestSize.Level1)
{
    const char *document064 = "{\"_id\" : \"64\", \"a\":1, \"doc64\" : 2}";
    const char *document063 = "{\"_id\" : \"63\", \"a\":1, \"doc63\" : 2}";
    const char *document062 = "{\"_id\" : \"62\", \"a\":1, \"doc62\" : 2}";
    const char *document061 = "{\"_id\" : \"61\", \"a\":1, \"doc61\" : 2}";
    EXPECT_EQ(GRD_InsertDoc(g_db, COLLECTION_NAME, document064, 0), GRD_OK);
    EXPECT_EQ(GRD_InsertDoc(g_db, COLLECTION_NAME, document063, 0), GRD_OK);
    EXPECT_EQ(GRD_InsertDoc(g_db, COLLECTION_NAME, document062, 0), GRD_OK);
    EXPECT_EQ(GRD_InsertDoc(g_db, COLLECTION_NAME, document061, 0), GRD_OK);
    const char *filter = "{\"a\":1}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projection = R"({})";
    Query query = { filter, projection };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = nullptr;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, document061);

    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, document062);

    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, document063);

    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    CompareValue(value, document064);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

HWTEST_F(DocumentDBFindTest, DocumentDBFindTest062, TestSize.Level1)
{
    const char *filter = R"({"abc123_.":1})";
    GRD_ResultSet *resultSet = nullptr;
    const char *projection = R"({"abc123_":1})";
    Query query = { filter, projection };
    EXPECT_EQ(GRD_FindDoc(g_db, COLLECTION_NAME, query, 1, &resultSet), GRD_INVALID_ARGS);
}

HWTEST_F(DocumentDBFindTest, DocumentDBFindTest063, TestSize.Level1)
{
    char *colName1 = "data_";
    GRD_DB *test_db = nullptr;
    std::string path = "./dataShare.db";
    int status = GRD_DBOpen(path.c_str(), nullptr, GRD_DB_OPEN_CREATE, &test_db);
    EXPECT_EQ(status, GRD_OK);
    EXPECT_EQ(GRD_CreateCollection(test_db, colName1, "", 0), GRD_OK);

    EXPECT_EQ(GRD_InsertDoc(test_db, colName1, document0631, 0), GRD_OK);
    EXPECT_EQ(GRD_InsertDoc(test_db, colName1, document0632, 0), GRD_OK);
    EXPECT_EQ(GRD_InsertDoc(test_db, colName1, document0633, 0), GRD_OK);

    string document1 = "{\"_id\":\"key2_11_com.acts.ohos.data.datasharetestclient_100\"\
          ,\"bundleName\":\"com.acts.ohos.data.datasharetestclient\"\
          ,\"key\":\"key2\",\"subscriberId\":11,\"timestamp\":1509100700,"
          "\"userId\":100,\"value\":{\"type\":0,";
    string document2 = "\"value\":[";
    string document3 = "5,";
    string document4 = document3;
    for (int i = 0; i < 100000; i++) {
        document4 += document3;
    }
    document4.push_back('5');
    string document5 = "]}}";
    string document0635 = document1 + document2 + document4 + document5;
    EXPECT_EQ(GRD_InsertDoc(test_db, colName1, document0635.c_str(), 0), GRD_OK);
    EXPECT_EQ(status, GRD_OK);
    const char *filter = "{}";
    GRD_ResultSet *resultSet = nullptr;
    const char *projection = "{\"id_\":true, \"timestamp\":true, \"key\":true, \"bundleName\": true, "
                             "\"subscriberId\": true}";
    Query query = { filter, projection };
    EXPECT_EQ(GRD_FindDoc(test_db, colName1, query, 1, &resultSet), GRD_OK);
    char *value;
    while (GRD_Next(resultSet) == GRD_OK) {
        EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
        printf("value is ======>%s\n", value);
        GRD_FreeValue(value);
    }
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
    EXPECT_EQ(GRD_DBClose(test_db, 0), GRD_OK);
    DocumentDBTestUtils::RemoveTestDbFiles(path.c_str());
}
} // namespace
