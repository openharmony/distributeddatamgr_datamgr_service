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
#include "doc_errno.h"
#include "json_object.h"

using namespace DocumentDB;
using namespace testing::ext;
using namespace DocumentDBUnitTest;

class DocumentDBJsonObjectTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DocumentDBJsonObjectTest::SetUpTestCase(void)
{
}

void DocumentDBJsonObjectTest::TearDownTestCase(void)
{
}

void DocumentDBJsonObjectTest::SetUp(void)
{
}

void DocumentDBJsonObjectTest::TearDown(void)
{
}

/**
 * @tc.name: OpenDBTest001
 * @tc.desc: Test open document db
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBJsonObjectTest, JsonObjectTest001, TestSize.Level0)
{
    const std::string config = R""({"a":123,"b":{"c":234,"d":"12345"}})"";

    std::shared_ptr<JsonObject> conf;
    int ret = JsonObject::Parse(config, conf);
    EXPECT_EQ(ret, E_OK);

    ValueObject obj;
    conf->GetObjectByPath({"b", "c"}, obj);
    EXPECT_EQ(obj.valueType, ValueObject::ValueType::VALUE_NUMBER);
    EXPECT_EQ(obj.intValue, 234);
}