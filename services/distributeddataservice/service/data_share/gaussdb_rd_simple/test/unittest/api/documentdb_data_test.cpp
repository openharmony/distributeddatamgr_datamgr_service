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
#include "grd_document/grd_document_api.h"
#include "log_print.h"
#include "sqlite_utils.h"
#include "cJSON.h"

using namespace DocumentDB;
using namespace testing::ext;
using namespace DocumentDBUnitTest;

namespace {
std::string g_path = "./document.db";
GRD_DB *g_db = nullptr;
const char *g_coll = "student";
} // namespace

class DocumentDBDataTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DocumentDBDataTest::SetUpTestCase(void) {}

void DocumentDBDataTest::TearDownTestCase(void) {}

void DocumentDBDataTest::SetUp(void)
{
    EXPECT_EQ(GRD_DBOpen(g_path.c_str(), nullptr, GRD_DB_OPEN_CREATE, &g_db), GRD_OK);
    EXPECT_NE(g_db, nullptr);

    EXPECT_EQ(GRD_CreateCollection(g_db, g_coll, "", 0), GRD_OK);
}

void DocumentDBDataTest::TearDown(void)
{
    if (g_db != nullptr) {
        EXPECT_EQ(GRD_DBClose(g_db, GRD_DB_CLOSE), GRD_OK);
        g_db = nullptr;
    }
    DocumentDBTestUtils::RemoveTestDbFiles(g_path);
}

/**
 * @tc.name: UpsertDataTest001
 * @tc.desc: Test upsert data into collection
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBDataTest, UpsertDataTest001, TestSize.Level0)
{
    std::string document = R""({"name":"Tmono","age":18,"addr":{"city":"shanghai","postal":200001}})"";
    EXPECT_EQ(GRD_UpsertDoc(g_db, g_coll, R""({"_id":"1234"})"", document.c_str(), GRD_DOC_REPLACE), 1);

    std::string update = R""({"CC":"AAAA"})"";
    EXPECT_EQ(GRD_UpdateDoc(g_db, g_coll, R""({"_id":"1234"})"", update.c_str(), 0), 1);

    std::string append = R""({"addr.city":"DDDD"})"";
    EXPECT_EQ(GRD_UpsertDoc(g_db, g_coll, R""({"_id":"1234"})"", append.c_str(), GRD_DOC_APPEND), 1);
}

/**
 * @tc.name: UpsertDataTest002
 * @tc.desc: Test upsert data with db is nullptr
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBDataTest, UpsertDataTest002, TestSize.Level0)
{
    std::string document = R""({"name":"Tmono","age":18,"addr":{"city":"shanghai","postal":200001}})"";
    EXPECT_EQ(GRD_UpsertDoc(nullptr, g_coll, "1234", document.c_str(), GRD_DOC_REPLACE), GRD_INVALID_ARGS);
}

/**
 * @tc.name: UpsertDataTest003
 * @tc.desc: Test upsert data with invalid collection name
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBDataTest, UpsertDataTest003, TestSize.Level0)
{
    std::string document = R""({"name":"Tmono","age":18,"addr":{"city":"shanghai","postal":200001}})"";
    std::vector<std::pair<const char *, int>> invalidName = {
        { nullptr, GRD_INVALID_ARGS },
        { "", GRD_INVALID_ARGS },
        { "GRD_123", GRD_INVALID_FORMAT },
        { "grd_123", GRD_INVALID_FORMAT },
        { "GM_SYS_123", GRD_INVALID_FORMAT },
        { "gm_sys_123", GRD_INVALID_FORMAT },
    };
    for (auto it : invalidName) {
        GLOGD("UpsertDataTest003: upsert data with collectionname: %s", it.first);
        EXPECT_EQ(GRD_UpsertDoc(g_db, it.first, "1234", document.c_str(), GRD_DOC_REPLACE), it.second);
    }
}

HWTEST_F(DocumentDBDataTest, UpsertDataTest004, TestSize.Level0) {}

HWTEST_F(DocumentDBDataTest, UpsertDataTest005, TestSize.Level0) {}

/**
 * @tc.name: UpsertDataTest006
 * @tc.desc: Test upsert data with invalid flags
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBDataTest, UpsertDataTest006, TestSize.Level0)
{
    std::string filter = R""({"_id":"1234"})"";
    std::string document = R""({"name":"Tmono","age":18,"addr":{"city":"shanghai","postal":200001}})"";

    for (auto flags : std::vector<unsigned int>{ 2, 4, 8, 64, 1024, UINT32_MAX }) {
        EXPECT_EQ(GRD_UpsertDoc(g_db, g_coll, filter.c_str(), document.c_str(), flags), GRD_INVALID_ARGS);
    }
}

/**
 * @tc.name: UpsertDataTest007
 * @tc.desc: Test upsert data with collection not create
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBDataTest, UpsertDataTest007, TestSize.Level0)
{
    std::string filter = R""({"_id":"1234"})"";
    std::string val = R""({"name":"Tmono","age":18,"addr":{"city":"shanghai","postal":200001}})"";
    EXPECT_EQ(GRD_UpsertDoc(g_db, "collection_not_exists", filter.c_str(), val.c_str(), GRD_DOC_REPLACE), GRD_INVALID_ARGS);
}

/**
 * @tc.name: UpsertDataTest008
 * @tc.desc: Test upsert data with different document in append
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBDataTest, UpsertDataTest008, TestSize.Level0)
{
    std::string filter = R""({"_id":"1234"})"";
    std::string document = R""({"name":"Tmn","age":18,"addr":{"city":"shanghai","postal":200001}})"";
    EXPECT_EQ(GRD_UpsertDoc(g_db, g_coll, filter.c_str(), document.c_str(), GRD_DOC_REPLACE), 1);

    std::string updateDoc = R""({"name":"Xue","case":2,"age":28,"addr":{"city":"shenzhen","postal":518000}})"";
    EXPECT_EQ(GRD_UpsertDoc(g_db, g_coll, filter.c_str(), updateDoc.c_str(), GRD_DOC_APPEND), 1);
}

HWTEST_F(DocumentDBDataTest, UpsertDataTest009, TestSize.Level0)
{
    std::string filter = R""({"_id":"1234", "aaa" : "bbb"})"";
    std::string document = R""({"name":"Tmn","age":18,"addr":{"city":"shanghai","postal":200001}})"";
    EXPECT_EQ(GRD_UpsertDoc(g_db, g_coll, filter.c_str(), document.c_str(), GRD_DOC_APPEND), 1);
    std::string filter2 = R""({"_id":"1234", "aaa" : "ccc"})"";
    std::string updateDoc = R""({"name":"Xue","case":2,"age":28,"addr":{"city":"shenzhen","postal":518000}})"";
    EXPECT_EQ(GRD_UpsertDoc(g_db, g_coll, filter.c_str(), document.c_str(), GRD_DOC_APPEND), GRD_DATA_CONFLICT);
}

HWTEST_F(DocumentDBDataTest, UpsertDataTest010, TestSize.Level0)
{
    std::string filter = R""({"_id":"abcde"})"";
    std::string document = R"({"field1": ")" + string(1024 * 1024 + 1, 'a') + "\"}";
    EXPECT_EQ(GRD_UpsertDoc(g_db, g_coll, filter.c_str(), document.c_str(), GRD_DOC_REPLACE), GRD_OVER_LIMIT);
}

/**
 * @tc.name: UpdateDataTest001
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBDataTest, UpdateDataTest001, TestSize.Level0)
{
    std::string filter = R""({"_id":"1234"})"";
    std::string updateDoc = R""({"name":"Xue"})"";
    EXPECT_EQ(GRD_UpdateDoc(g_db, g_coll, filter.c_str(), updateDoc.c_str(), 0), GRD_OK);
}

/**
 * @tc.name: UpdateDataTest002
 * @tc.desc: Test update data with db is nullptr
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBDataTest, UpdateDataTest002, TestSize.Level0)
{
    std::string filter = R""({"_id":"1234"})"";
    std::string document = R""({"name":"Tmono","age":18,"addr":{"city":"shanghai","postal":200001}})"";
    EXPECT_EQ(GRD_UpdateDoc(nullptr, g_coll, filter.c_str(), document.c_str(), 0), GRD_INVALID_ARGS);
}

/**
 * @tc.name: UpdateDataTest003
 * @tc.desc: Test update data with invalid collection name
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBDataTest, UpdateDataTest003, TestSize.Level0)
{
    std::string filter = R""({"_id":"1234"})"";
    std::string document = R""({"name":"Tmono","age":18,"addr":{"city":"shanghai","postal":200001}})"";
    std::vector<std::pair<const char *, int>> invalidName = {
        { nullptr, GRD_INVALID_ARGS },
        { "", GRD_INVALID_ARGS },
        { "GRD_123", GRD_INVALID_FORMAT },
        { "grd_123", GRD_INVALID_FORMAT },
        { "GM_SYS_123", GRD_INVALID_FORMAT },
        { "gm_sys_123", GRD_INVALID_FORMAT },
    };
    for (auto it : invalidName) {
        GLOGD("UpdateDataTest003: update data with collectionname: %s", it.first);
        EXPECT_EQ(GRD_UpdateDoc(g_db, it.first, filter.c_str(), document.c_str(), 0), it.second);
    }
}

/**
 * @tc.name: UpdateDataTest004
 * @tc.desc: Test update data with invalid filter
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBDataTest, UpdateDataTest004, TestSize.Level0) {}

/**
 * @tc.name: UpdateDataTest005
 * @tc.desc: Test update data with invalid doc
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBDataTest, UpdateDataTest005, TestSize.Level0) {}

/**
 * @tc.name: UpdateDataTest006
 * @tc.desc: Test update data with invalid flag
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: mazhao
 */
HWTEST_F(DocumentDBDataTest, UpdateDataTest006, TestSize.Level0)
{
    std::string filter = R""({"_id":"1234"})"";
    std::string document = R""({"name":"Tmono","age":18,"addr":{"city":"shanghai","postal":200001}})"";
    std::vector<unsigned int> invalidFlags = { 2, 4, 8, 1024, UINT32_MAX };
    for (auto flag : invalidFlags) {
        GLOGD("UpdateDataTest006: update data with flag: %u", flag);
        EXPECT_EQ(GRD_UpdateDoc(g_db, g_coll, filter.c_str(), document.c_str(), flag), GRD_INVALID_ARGS);
    }
}

HWTEST_F(DocumentDBDataTest, UpdateDataTest007, TestSize.Level0)
{
    std::string filter = R""({"_id":"1234"})"";
    std::string document = R""({"_id":"1234", "field1":{"c_field":{"cc_field":{"ccc_field":1}}}, "field2" : 2})"";

    EXPECT_EQ(GRD_InsertDoc(g_db, g_coll, document.c_str(), 0), GRD_OK);

    std::string updata = R""({"field1":1, "FIELD1":[1, true, 1.23456789, "hello world!", null]})"";
    EXPECT_EQ(GRD_UpdateDoc(g_db, g_coll, filter.c_str(), updata.c_str(), 0), 1);

    GRD_ResultSet *resultSet = nullptr;
    const char *projection = "{}";
    Query query = { filter.c_str(), projection };
    EXPECT_EQ(GRD_FindDoc(g_db, g_coll, query, 1, &resultSet), GRD_OK);
    EXPECT_EQ(GRD_Next(resultSet), GRD_OK);
    char *value = NULL;
    EXPECT_EQ(GRD_GetValue(resultSet, &value), GRD_OK);
    string valueStr = value;
    string repectStr = R""({"_id":"1234","field1":1,"field2":2,"FIELD1":[1,true,1.23456789,"hello world!",null]})"";
    EXPECT_EQ((valueStr == repectStr), 1);
    EXPECT_EQ(GRD_FreeValue(value), GRD_OK);
    EXPECT_EQ(GRD_FreeResultSet(resultSet), GRD_OK);
}

HWTEST_F(DocumentDBDataTest, UpdateDataTest008, TestSize.Level0)
{
    std::string filter = R""({"_id":"1234"})"";
    std::string updata = R""({"field1":1, "FIELD1":[1, true, 1.23456789, "hello world!", null]})"";
    EXPECT_EQ(GRD_UpdateDoc(g_db, "grd_aa", filter.c_str(), updata.c_str(), 0), GRD_INVALID_FORMAT);
    EXPECT_EQ(GRD_UpdateDoc(g_db, "gRd_aa", filter.c_str(), updata.c_str(), 0), GRD_INVALID_FORMAT);

}
