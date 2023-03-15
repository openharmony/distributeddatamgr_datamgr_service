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

#include "log_print.h"
#include "grd_base/grd_db_api.h"
#include "grd_base/grd_error.h"

using namespace DocumentDB;
using namespace testing::ext;

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
HWTEST_F(DocumentDBApiTest, OpenDBTest001, TestSize.Level1)
{
    std::string path = "./document.db";
    GRD_DB *db = nullptr;
    int status = GRD_DBOpen(path.c_str(), nullptr, 0, &db);
    EXPECT_EQ(status, GRD_OK);
    EXPECT_NE(db, nullptr);
    GLOGD("Open DB test 001: status: %d", status);
    status = GRD_DBClose(db, 0);
    EXPECT_EQ(status, GRD_OK);
    db = nullptr;
}