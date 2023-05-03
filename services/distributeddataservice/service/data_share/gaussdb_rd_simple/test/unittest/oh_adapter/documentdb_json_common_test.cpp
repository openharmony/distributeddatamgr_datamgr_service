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
#include <vector>

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
HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest001, TestSize.Level0)
{
    std::string document = R""({"name":"Tmn","age":18,"addr":{"city":"shanghai","postal":200001}})"";
    std::string updateDoc = R""({"name":"Xue","case":{"field1":1,"field2":"string","field3":[1,2,3]},"age":28,"addr":{"city":"shenzhen","postal":518000}})"";

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);

    EXPECT_EQ(JsonCommon::Append(src, add), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({"case", "field1"}, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemCase.GetItemValue().GetIntValue(), 1);

    JsonObject itemName = src.FindItem({"name"}, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemName.GetItemValue().GetStringValue(), "Xue");
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest002, TestSize.Level0)
{
    std::string document = R""({"name":"Tmn","case":2,"age":[1,2,3],"addr":{"city":"shanghai","postal":200001}})"";
    std::string updateDoc = R""({"name":["Xue","Neco","Lip"],"grade":99,"age":18,"addr":
        [{"city":"shanghai","postal":200001},{"city":"beijing","postal":100000}]})"";

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(JsonCommon::Append(src, add), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({"grade"}, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemCase.GetItemValue().GetIntValue(), 99); // 99: grade

    JsonObject itemName = src.FindItem({"name", "1"}, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemName.GetItemValue().GetStringValue(), "Neco");
}


HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest003, TestSize.Level0)
{
    std::string document = R""({"name":["Tmn","BB","Alice"],"age":[1,2,3],"addr":[{"city":"shanghai","postal":200001},{"city":"wuhan","postal":430000}]})"";
    std::string updateDoc = R""({"name":["Xue","Neco","Lip"],"age":18,"addr":[{"city":"shanghai","postal":200001},{"city":"beijing","postal":100000}]})"";

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(JsonCommon::Append(src, add), E_OK);

    GLOGD("result: %s", src.Print().c_str());
    JsonObject itemCase = src.FindItem({"addr", "1", "city"}, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemCase.GetItemValue().GetStringValue(), "beijing"); // 99: grade

    JsonObject itemName = src.FindItem({"name", "1"}, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemName.GetItemValue().GetStringValue(), "Neco");
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest004, TestSize.Level0)
{
    std::string document = R""({"name":["Tmn","BB","Alice"]})"";
    std::string updateDoc = R""({"name.5":"GG"})"";;

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(JsonCommon::Append(src, add), -E_DATA_CONFLICT);
    GLOGD("result: %s", src.Print().c_str());
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest005, TestSize.Level0)
{
    std::string document = R""({"name":["Tmn","BB","Alice"]})"";
    std::string updateDoc = R""({"name.2":"GG"})"";;

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);

    EXPECT_EQ(JsonCommon::Append(src, add), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({"name", "2"}, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemCase.GetItemValue().GetStringValue(), "GG");
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest006, TestSize.Level0)
{
    std::string document = R""({"name":{"first":"Tno","last":"moray"}})"";
    std::string updateDoc = R""({"name":{"midle.AA":"GG"}})"";

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);

    EXPECT_EQ(JsonCommon::Append(src, add), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({"name", "midle.AA"}, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemCase.GetItemValue().GetStringValue(), "GG");
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest007, TestSize.Level0)
{
    std::string document = R""({"name":{"first":["XX","CC"],"last":"moray"}})"";
    std::string updateDoc = R""({"name.first.0":"LL"})"";

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);

    EXPECT_EQ(JsonCommon::Append(src, add), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({"name", "first", "0"}, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemCase.GetItemValue().GetStringValue(), "LL");
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest008, TestSize.Level0)
{
    std::string document = R""({"name":{"first":"XX","last":"moray"}})"";
    std::string updateDoc = R""({"name":{"first":["XXX","BBB","CCC"]}})"";

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);

    EXPECT_EQ(JsonCommon::Append(src, add), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({"name", "first", "0"}, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemCase.GetItemValue().GetStringValue(), "XXX");
}


HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest009, TestSize.Level0)
{
    std::string document = R""({"name":{"first":["XXX","BBB","CCC"],"last":"moray"}})"";
    std::string updateDoc = R""({"name":{"first":"XX"}})"";

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);

    EXPECT_EQ(JsonCommon::Append(src, add), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({"name", "first"}, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemCase.GetItemValue().GetStringValue(), "XX");
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest010, TestSize.Level0)
{
    std::string document = R""({"name":{"first":["XXX","BBB","CCC"],"last":"moray"}})"";
    std::string updateDoc = R""({"name":{"first":{"XX":"AA"}}})"";

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);

    EXPECT_EQ(JsonCommon::Append(src, add), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({"name", "first", "XX"}, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemCase.GetItemValue().GetStringValue(), "AA");
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest011, TestSize.Level0)
{
    std::string document = R""({"name":{"first":["XXX","BBB","CCC"],"last":"moray"}})"";
    std::string updateDoc = R""({"name.last.AA.B":"Mnado"})"";

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);

    EXPECT_EQ(JsonCommon::Append(src, add), -E_DATA_CONFLICT);
    GLOGD("result: %s", src.Print().c_str());
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest012, TestSize.Level0)
{
    std::string document = R""({"name":["Tmn","BB","Alice"]})"";
    std::string updateDoc = R""({"name.first":"GG"})"";;

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(JsonCommon::Append(src, add), -E_DATA_CONFLICT);
    GLOGD("result: %s", src.Print().c_str());
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest013, TestSize.Level0)
{
    std::string document = R""({"name":["Tmn","BB","Alice"]})"";
    std::string updateDoc = R""({"name":{"first":"GG"}})"";;

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(JsonCommon::Append(src, add), E_OK);
    GLOGD("result: %s", src.Print().c_str());
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest014, TestSize.Level0)
{
    std::string document = R""({"name":{"first":"Xue","second":"Lang"}})"";
    std::string updateDoc = R""({"name.0":"GG"})"";;

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(JsonCommon::Append(src, add), -E_DATA_CONFLICT);
    GLOGD("result: %s", src.Print().c_str());
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest015, TestSize.Level0)
{
    std::string document = R""({"name":{"first":"Xue","second":"Lang"}})"";
    std::string updateDoc = R""({"name.first":["GG","MM"]})"";;

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(JsonCommon::Append(src, add), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({"name", "first", "0"}, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemCase.GetItemValue().GetStringValue(), "GG");
}