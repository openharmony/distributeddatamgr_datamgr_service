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
#include "log_print.h"
#include "grd_base/grd_db_api.h"
#include "grd_base/grd_error.h"
#include "grd_document/grd_document_api.h"

using namespace DocumentDB;
using namespace testing::ext;
using namespace DocumentDBUnitTest;

class DocumentDBApiTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DocumentDBApiTest::SetUpTestCase(void)
{
}

void DocumentDBApiTest::TearDownTestCase(void)
{
}

void DocumentDBApiTest::SetUp(void)
{
}

void DocumentDBApiTest::TearDown(void)
{
}

/**
 * @tc.name: OpenDBTest001
 * @tc.desc: Test open document db
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBApiTest, OpenDBTest001, TestSize.Level0)
{
    std::string path = "./document.db";
    GRD_DB *db = nullptr;
    int status = GRD_DBOpen(path.c_str(), nullptr, GRD_DB_OPEN_CREATE, &db);
    EXPECT_EQ(status, GRD_OK);
    EXPECT_NE(db, nullptr);
    GLOGD("Open DB test 001: status: %d", status);

    EXPECT_EQ(GRD_CreateCollection(db, "student", "", 0), GRD_OK);

    EXPECT_EQ(GRD_UpSertDoc(db, "student", "10001", "{name:\"Tom\",age:23}", 0), GRD_OK);
    EXPECT_EQ(GRD_UpSertDoc(db, "student", "10001", "{name:\"Tom\",age:24}", 0), GRD_OK);

    EXPECT_EQ(GRD_DropCollection(db, "student", 0), GRD_OK);

    status = GRD_DBClose(db, 0);
    EXPECT_EQ(status, GRD_OK);
    db = nullptr;

    DocumentDBTestUtils::RemoveTestDbFiles(path);
}

/**
 * @tc.name: OpenDBPathTest001
 * @tc.desc: Test open document db with NULL path
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBApiTest, OpenDBPathTest001, TestSize.Level0)
{
    GRD_DB *db = nullptr;
    std::vector<const char *> invalidPath = {
        nullptr,
        "",
        "/a/b/c/"
    };
    for (auto path : invalidPath) {
        GLOGD("OpenDBPathTest001: open db with path: %s", path);
        int status = GRD_DBOpen(path, nullptr, GRD_DB_OPEN_CREATE, &db);
        EXPECT_EQ(status, GRD_INVALID_ARGS);
    }
}

/**
 * @tc.name: OpenDBPathTest002
 * @tc.desc: Test open document db with file no permission
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBApiTest, OpenDBPathTest002, TestSize.Level0)
{
    GRD_DB *db = nullptr;
    std::string pathNoPerm = "/root/document.db";
    int status = GRD_DBOpen(pathNoPerm.c_str(), nullptr, GRD_DB_OPEN_CREATE, &db);
    EXPECT_EQ(status, GRD_FAILED_FILE_OPERATION);
}

/**
 * @tc.name: OpenDBConfigTest001
 * @tc.desc: Test open document db with invalid config option
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBApiTest, OpenDBConfigTest001, TestSize.Level0)
{
    GRD_DB *db = nullptr;
    std::string path= "./document.db";
    const int MAX_JSON_LEN = 512 * 1024;
    std::string configStr = std::string(MAX_JSON_LEN + 1, 'a');
    int status = GRD_DBOpen(path.c_str(), configStr.c_str(), GRD_DB_OPEN_CREATE, &db);
    EXPECT_EQ(status, GRD_OVER_LIMIT);
}

/**
 * @tc.name: OpenDBConfigTest002
 * @tc.desc: Test open document db with invalid config format
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBApiTest, OpenDBConfigTest002, TestSize.Level0)
{
    GRD_DB *db = nullptr;
    std::string path= "./document.db";
    int status = GRD_DBOpen(path.c_str(), "{aa}", GRD_DB_OPEN_CREATE, &db);
    EXPECT_EQ(status, GRD_INVALID_JSON_FORMAT);
}

/**
 * @tc.name: OpenDBConfigMaxConnNumTest001
 * @tc.desc: Test open document db with invalid config item maxConnNum
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBApiTest, OpenDBConfigMaxConnNumTest001, TestSize.Level0)
{
    GRD_DB *db = nullptr;
    std::string path= "./document.db";

    std::vector<std::string> configList = {
        R""({"maxConnNum":0})"",
        R""({"maxConnNum":15})"",
        R""({"maxConnNum":1025})"",
        R""({"maxConnNum":1000000007})"",
        R""({"maxConnNum":"16"})"",
        R""({"maxConnNum":{"value":17}})"",
        R""({"maxConnNum":[16,17,18]})"",
    };
    for (const auto &config : configList) {
        GLOGD("OpenDBConfigMaxConnNumTest001: test with config:%s", config.c_str());
        int status = GRD_DBOpen(path.c_str(), config.c_str(), GRD_DB_OPEN_CREATE, &db);
        ASSERT_EQ(status, GRD_INVALID_CONFIG_VALUE);
    }
}

/**
 * @tc.name: OpenDBConfigMaxConnNumTest002
 * @tc.desc: Test open document db with valid item maxConnNum
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBApiTest, OpenDBConfigMaxConnNumTest002, TestSize.Level1)
{
    GRD_DB *db = nullptr;
    std::string path= "./document.db";

    for (int i = 16; i <= 1024; i++) {
        std::string config = "{\"maxConnNum\":" + std::to_string(i) + "}";
        int status = GRD_DBOpen(path.c_str(), config.c_str(), GRD_DB_OPEN_CREATE, &db);
        EXPECT_EQ(status, GRD_OK);
        ASSERT_NE(db, nullptr);

        status = GRD_DBClose(db, 0);
        EXPECT_EQ(status, GRD_OK);
        db = nullptr;

        DocumentDBTestUtils::RemoveTestDbFiles(path);
    }
}

/**
 * @tc.name: OpenDBConfigMaxConnNumTest003
 * @tc.desc: Test reopen document db with different maxConnNum
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBApiTest, OpenDBConfigMaxConnNumTest003, TestSize.Level1)
{
    GRD_DB *db = nullptr;
    std::string path= "./document.db";

    std::string config = R""({"maxConnNum":16})"";
    int status = GRD_DBOpen(path.c_str(), config.c_str(), GRD_DB_OPEN_CREATE, &db);
    EXPECT_EQ(status, GRD_OK);

    status = GRD_DBClose(db, 0);
    EXPECT_EQ(status, GRD_OK);
    db = nullptr;

    config = R""({"maxConnNum":17})"";
    status = GRD_DBOpen(path.c_str(), config.c_str(), GRD_DB_OPEN_CREATE, &db);
    EXPECT_EQ(status, GRD_INVALID_CONFIG_VALUE);

    // DocumentDBTestUtils::RemoveTestDbFiles(path);
}

/**
 * @tc.name: OpenDBConfigMaxConnNumTest004
 * @tc.desc: Test open document db over maxConnNum
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBApiTest, OpenDBConfigMaxConnNumTest004, TestSize.Level1)
{
    std::string path= "./document.db";

    int maxCnt = 16;
    std::string config = "{\"maxConnNum\":" + std::to_string(maxCnt) + "}";

    std::vector<GRD_DB *> dbList;
    while (maxCnt--) {
        GRD_DB *db = nullptr;
        int status = GRD_DBOpen(path.c_str(), config.c_str(), GRD_DB_OPEN_CREATE, &db);
        EXPECT_EQ(status, GRD_OK);
        dbList.push_back(db);
    }

    GRD_DB *db = nullptr;
    int status = GRD_DBOpen(path.c_str(), config.c_str(), GRD_DB_OPEN_CREATE, &db);
    EXPECT_EQ(status, GRD_OVER_LIMIT);
    EXPECT_EQ(db, nullptr);

    for (auto *it : dbList) {
        status = GRD_DBClose(it, 0);
        EXPECT_EQ(status, GRD_OK);
    }

    DocumentDBTestUtils::RemoveTestDbFiles(path);
}

/**
 * @tc.name: OpenDBConfigPageSizeTest001
 * @tc.desc: Test open document db with invalid config item pageSize
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBApiTest, OpenDBConfigPageSizeTest001, TestSize.Level0)
{
    GRD_DB *db = nullptr;
    std::string path= "./document.db";

    std::vector<std::string> configList = {
        R""({"pageSize":0})"",
        R""({"pageSize":5})"",
        R""({"pageSize":48})"",
        R""({"pageSize":1000000007})"",
        R""({"pageSize":"4"})"",
        R""({"pageSize":{"value":8}})"",
        R""({"pageSize":[16,32,64]})"",
    };
    for (const auto &config : configList) {
        GLOGD("OpenDBConfigPageSizeTest001: test with config:%s", config.c_str());
        int status = GRD_DBOpen(path.c_str(), config.c_str(), 0, &db);
        EXPECT_EQ(status, GRD_INVALID_CONFIG_VALUE);
    }
}

/**
 * @tc.name: OpenDBConfigPageSizeTest002
 * @tc.desc: Test open document db with valid config item pageSize
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBApiTest, OpenDBConfigPageSizeTest002, TestSize.Level0)
{
    GRD_DB *db = nullptr;
    std::string path= "./document.db";

    for (int size : {4, 8, 16, 32, 64}) {
        std::string config = "{\"pageSize\":" + std::to_string(size) + "}";
        int status = GRD_DBOpen(path.c_str(), config.c_str(), GRD_DB_OPEN_CREATE, &db);
        EXPECT_EQ(status, GRD_OK);

        status = GRD_DBClose(db, 0);
        EXPECT_EQ(status, GRD_OK);
        db = nullptr;

        DocumentDBTestUtils::RemoveTestDbFiles(path);
    }
}

/**
 * @tc.name: OpenDBConfigPageSizeTest003
 * @tc.desc: Test reopen document db with different pageSize
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBApiTest, OpenDBConfigPageSizeTest003, TestSize.Level1)
{
    GRD_DB *db = nullptr;
    std::string path= "./document.db";

    std::string config = R""({"pageSize":4})"";
    int status = GRD_DBOpen(path.c_str(), config.c_str(), GRD_DB_OPEN_CREATE, &db);
    EXPECT_EQ(status, GRD_OK);

    status = GRD_DBClose(db, 0);
    EXPECT_EQ(status, GRD_OK);
    db = nullptr;

    config = R""({"pageSize":8})"";
    status = GRD_DBOpen(path.c_str(), config.c_str(), GRD_DB_OPEN_CREATE, &db);
    EXPECT_EQ(status, GRD_INVALID_CONFIG_VALUE);

    DocumentDBTestUtils::RemoveTestDbFiles(path);
}

/**
 * @tc.name: OpenDBConfigRedoFlushTest001
 * @tc.desc: Test open document db with valid config item redoFlushByTrx
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBApiTest, OpenDBConfigRedoFlushTest001, TestSize.Level0)
{
    GRD_DB *db = nullptr;
    std::string path= "./document.db";

    for (int flush : {0, 1}) {
        std::string config = "{\"redoFlushByTrx\":" + std::to_string(flush) + "}";
        int status = GRD_DBOpen(path.c_str(), config.c_str(), GRD_DB_OPEN_CREATE, &db);
        EXPECT_EQ(status, GRD_OK);

        status = GRD_DBClose(db, 0);
        EXPECT_EQ(status, GRD_OK);
        db = nullptr;

        DocumentDBTestUtils::RemoveTestDbFiles(path);
    }
}

/**
 * @tc.name: OpenDBConfigXXXTest001
 * @tc.desc: Test open document db with invalid config item redoFlushByTrx
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBApiTest, OpenDBConfigRedoFlushTest002, TestSize.Level0)
{
    GRD_DB *db = nullptr;
    std::string path= "./document.db";

    std::string config = R""({"redoFlushByTrx":3})"";
    int status = GRD_DBOpen(path.c_str(), config.c_str(), GRD_DB_OPEN_CREATE, &db);
    EXPECT_EQ(status, GRD_INVALID_CONFIG_VALUE);
}

/**
 * @tc.name: OpenDBFlagTest001
 * @tc.desc: Test open document db with invalid flag
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBApiTest, OpenDBFlagTest001, TestSize.Level0)
{
    GRD_DB *db = nullptr;
    std::string path= "./document.db";
    for (unsigned int flag : {2, 4, 10, 1000000007}) {
        int status = GRD_DBOpen(path.c_str(), "", flag, &db);
        EXPECT_EQ(status, GRD_INVALID_ARGS);
    }
}

/**
 * @tc.name: OpenDBFlagTest002
 * @tc.desc: Test open document db with valid flag
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBApiTest, OpenDBFlagTest002, TestSize.Level0)
{
    GRD_DB *db = nullptr;
    std::string path= "./document.db";
    int status = GRD_DBOpen(path.c_str(), "", GRD_DB_OPEN_CREATE, &db);
    EXPECT_EQ(status, GRD_OK);

    status = GRD_DBClose(db, 0);
    EXPECT_EQ(status, GRD_OK);
    db = nullptr;

    status = GRD_DBOpen(path.c_str(), "", GRD_DB_OPEN_ONLY, &db);
    EXPECT_EQ(status, GRD_OK);

    status = GRD_DBClose(db, 0);
    EXPECT_EQ(status, GRD_OK);
    db = nullptr;

    DocumentDBTestUtils::RemoveTestDbFiles(path);
}