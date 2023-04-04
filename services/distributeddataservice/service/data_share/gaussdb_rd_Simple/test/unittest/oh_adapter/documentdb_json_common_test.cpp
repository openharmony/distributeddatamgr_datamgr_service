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
#include "json_common.h"
#include "log_print.h"

using namespace DocumentDB;
using namespace testing::ext;
using namespace DocumentDBUnitTest;

class DocumentDBJsonCommonTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DocumentDBJsonCommonTest::SetUpTestCase(void)
{
}

void DocumentDBJsonCommonTest::TearDownTestCase(void)
{
}

void DocumentDBJsonCommonTest::SetUp(void)
{
}

void DocumentDBJsonCommonTest::TearDown(void)
{
}

/**
 * @tc.name: OpenDBTest001
 * @tc.desc: Test open document db
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DocumentDBJsonCommonTest, JsonObjectTest001, TestSize.Level0)
{
    std::string document = R""({"name":"Tmn","age":18,"addr":{"city":"shanghai","postal":200001}})"";
    std::string updateDoc = R""({"name":"Xue","case":{"field1":1,"field2":"string","field3":[1,2,3]},"age":28,"addr":{"city":"shenzhen","postal":518000}})"";

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    GLOGD("Source result: %s", src.Print().c_str());
    GLOGD("Append result: %s", add.Print().c_str());

    GLOGD("----> isPathExist: %d.", src.IsFieldExists({"name"}));

    JsonCommon::Append(src, add);

    GLOGD("result: %s", src.Print().c_str());
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectTest002, TestSize.Level0)
{
    std::string document = R""({"name":"Tmn","case":2,"age":[1,2,3],"addr":{"city":"shanghai","postal":200001}})"";
    std::string updateDoc = R""({"name":["Xue","Neco","Lip"],"grade":99,"age":18,"addr":[{"city":"shanghai","postal":200001},{"city":"beijing","postal":100000}]})"";

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    GLOGD("Source result: %s", src.Print().c_str());
    GLOGD("Append result: %s", add.Print().c_str());

    GLOGD("----> isPathExist: %d.", src.IsFieldExists({"name"}));

    JsonCommon::Append(src, add);

    GLOGD("result: %s", src.Print().c_str());
}


HWTEST_F(DocumentDBJsonCommonTest, JsonObjectTest003, TestSize.Level0)
{
    std::string document = R""({"name":["Tmn","BB","Alice"],"age":[1,2,3],"addr":[{"city":"shanghai","postal":200001},{"city":"wuhan","postal":430000}]})"";
    std::string updateDoc = R""({"name":["Xue","Neco","Lip"],"age":18,"addr":[{"city":"shanghai","postal":200001},{"city":"beijing","postal":100000}]})"";

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    GLOGD("Source result: %s", src.Print().c_str());
    GLOGD("Append result: %s", add.Print().c_str());

    GLOGD("----> isPathExist: %d.", src.IsFieldExists({"name"}));

    JsonCommon::Append(src, add);

    GLOGD("result: %s", src.Print().c_str());
}


HWTEST_F(DocumentDBJsonCommonTest, JsonObjectTest004, TestSize.Level0)
{
    std::string document = R""({"name":["Tmn","BB","Alice"]})"";
    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);

    JsonObject child = src.FindItem({"name", "1"}, errCode);

    GLOGD("----> GetItemValue: %s.", child.GetItemValue().GetStringValue().c_str());

    GLOGD("----> GetItemFiled: %s.", child.GetItemFiled().c_str());
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectTest005, TestSize.Level0)
{
    std::string document = R""({"name":["Tmn","BB","Alice"]})"";
    std::string updateDoc = R""({"name.2":"GG"})"";;

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    GLOGD("Source result: %s", src.Print().c_str());
    GLOGD("Append result: %s", add.Print().c_str());

    GLOGD("----> isPathExist: %d.", src.IsFieldExists({"name"}));

    JsonCommon::Append(src, add);

    GLOGD("result: %s", src.Print().c_str());
}