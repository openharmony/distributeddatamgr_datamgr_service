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

using namespace DocumentDB;
using namespace testing::ext;
using namespace DocumentDBUnitTest;

namespace {
std::string g_path = "./document.db";
GRD_DB *g_db = nullptr;

class DocumentDBCollectionTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DocumentDBCollectionTest::SetUpTestCase(void) {}

void DocumentDBCollectionTest::TearDownTestCase(void) {}

void DocumentDBCollectionTest::SetUp(void)
{
    EXPECT_EQ(GRD_DBOpen(g_path.c_str(), nullptr, GRD_DB_OPEN_CREATE, &g_db), GRD_OK);
    EXPECT_NE(g_db, nullptr);
}

void DocumentDBCollectionTest::TearDown(void)
{
    if (g_db != nullptr) {
        EXPECT_EQ(GRD_DBClose(g_db, GRD_DB_CLOSE), GRD_OK);
        g_db = nullptr;
    }
    DocumentDBTestUtils::RemoveTestDbFiles(g_path);
}

/**
 * @tc.name: CollectionTest001
 * @tc.desc: Test create collection with null db
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBCollectionTest, CollectionTest001, TestSize.Level0)
{
    EXPECT_EQ(GRD_CreateCollection(nullptr, "student", "", 0), GRD_INVALID_ARGS);
}

namespace {
const int MAX_COLLECTION_LEN = 512;
}

/**
 * @tc.name: CollectionTest002
 * @tc.desc: Test create/drop collection with invalid collection name
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBCollectionTest, CollectionTest002, TestSize.Level0)
{
    string overLenName(MAX_COLLECTION_LEN, 'a');
    EXPECT_EQ(GRD_CreateCollection(g_db, overLenName.c_str(), "", 0), GRD_OVER_LIMIT);
    EXPECT_EQ(GRD_DropCollection(g_db, overLenName.c_str(), 0), GRD_OVER_LIMIT);

    std::vector<const char *> invalidName = {
        nullptr,
        "",
    };

    for (auto *it : invalidName) {
        GLOGD("CollectionTest002: create collection with name: %s", it);
        EXPECT_EQ(GRD_CreateCollection(g_db, it, "", 0), GRD_INVALID_ARGS);
        EXPECT_EQ(GRD_DropCollection(g_db, it, 0), GRD_INVALID_ARGS);
    }

    std::vector<const char *> invalidNameFormat = { "GRD_123", "grd_123", "GM_SYS_123", "gm_sys_123" };

    for (auto *it : invalidNameFormat) {
        GLOGD("CollectionTest002: create collection with name: %s", it);
        EXPECT_EQ(GRD_CreateCollection(g_db, it, "", 0), GRD_INVALID_FORMAT);
        EXPECT_EQ(GRD_DropCollection(g_db, it, 0), GRD_INVALID_FORMAT);
    }
}

/**
 * @tc.name: CollectionTest003
 * @tc.desc: Test create/drop collection with valid collection name
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBCollectionTest, CollectionTest003, TestSize.Level0)
{
    string overLenName(MAX_COLLECTION_LEN - 1, 'a');
    std::vector<const char *> validName = { "123", "&^%@", "中文字符", "sqlite_master", "NULL", "SELECT", "CREATE",
        "student/", "student'", "student\"", "student[", "student]", "student%", "student&", "student_", "student(",
        "student)", overLenName.c_str() };

    for (auto *it : validName) {
        GLOGD("CollectionTest003: create collection with name: %s", it);
        EXPECT_EQ(GRD_CreateCollection(g_db, it, "", 0), GRD_OK);
        EXPECT_EQ(GRD_DropCollection(g_db, it, 0), GRD_OK);
    }
}

/**
 * @tc.name: CollectionTest004
 * @tc.desc: Test create collection with ignore flag
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBCollectionTest, CollectionTest004, TestSize.Level0)
{
    EXPECT_EQ(GRD_CreateCollection(g_db, "student", "", 0), GRD_OK);
    EXPECT_EQ(GRD_CreateCollection(g_db, "student", "", 0), GRD_OK);
    EXPECT_EQ(GRD_CreateCollection(g_db, "Student", "", CHK_EXIST_COLLECTION), GRD_DATA_CONFLICT);
}

/**
 * @tc.name: CollectionTest005
 * @tc.desc: Test create collection with invalid option
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBCollectionTest, CollectionTest005, TestSize.Level0)
{
    EXPECT_EQ(GRD_CreateCollection(g_db, "student", R""({aa})"", 0), GRD_INVALID_FORMAT);

    std::vector<const char *> invalidOption = {
        R""({"maxDoc":0})"",
        R""({"maxDoc":"123"})"",
        R""({"maxDoc":{"value":1024}})"",
        R""({"maxDoc":[1,2,4,8]})"",
        R""({"minDoc":1024})"",
    };

    for (auto opt : invalidOption) {
        GLOGD("CollectionTest005: create collection with option: %s", opt);
        EXPECT_EQ(GRD_CreateCollection(g_db, "student", opt, 0), GRD_INVALID_ARGS);
    }
}

/**
 * @tc.name: CollectionTest006
 * @tc.desc: Test create/drop collection with valid flag
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBCollectionTest, CollectionTest006, TestSize.Level0)
{
    EXPECT_EQ(GRD_CreateCollection(g_db, "student", R""({"maxDoc":1024})"", 0), GRD_OK);
    EXPECT_EQ(GRD_CreateCollection(g_db, "student", R""({"maxDoc":2048})"", 0), GRD_OK);
    EXPECT_EQ(GRD_CreateCollection(g_db, "student", R""({"maxDoc":2048})"", CHK_EXIST_COLLECTION), GRD_DATA_CONFLICT);

    EXPECT_EQ(GRD_DropCollection(g_db, "student", 0), GRD_OK);
    EXPECT_EQ(GRD_DropCollection(g_db, "student", 0), GRD_OK);
    EXPECT_EQ(GRD_DropCollection(g_db, "student", CHK_NON_EXIST_COLLECTION), GRD_INVALID_ARGS);

    // Create collection with different option return OK after drop collection
    EXPECT_EQ(GRD_CreateCollection(g_db, "student", R""({"maxDoc":2048})"", 0), GRD_OK);
}

/**
 * @tc.name: CollectionTest007
 * @tc.desc: Test create/drop collection with invalid flag
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBCollectionTest, CollectionTest007, TestSize.Level0)
{
    for (int flag : std::vector<unsigned int> { 2, 4, 8, 1024, UINT32_MAX }) {
        EXPECT_EQ(GRD_CreateCollection(g_db, "student", "", flag), GRD_INVALID_ARGS);
        EXPECT_EQ(GRD_DropCollection(g_db, "student", flag), GRD_INVALID_ARGS);
    }
}
} // namespace