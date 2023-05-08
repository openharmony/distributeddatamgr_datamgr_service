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

#include "doc_errno.h"
#include "documentdb_test_utils.h"
#include "json_common.h"
#include "log_print.h"

using namespace DocumentDB;
using namespace testing::ext;
using namespace DocumentDBUnitTest;

namespace {
    class DocumentDBJsonCommonTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DocumentDBJsonCommonTest::SetUpTestCase(void) {}

void DocumentDBJsonCommonTest::TearDownTestCase(void) {}

void DocumentDBJsonCommonTest::SetUp(void) {}

void DocumentDBJsonCommonTest::TearDown(void) {}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest001, TestSize.Level0)
{
    std::string document = R""({"name":"Tmn","age":18,"addr":{"city":"shanghai","postal":200001}})"";
    std::string updateDoc =
        R""({"name":"Xue","case":{"field1":1,"field2":"string","field3":[1,2,3]},"age":28,"addr":{"city":"shenzhen","postal":518000}})"";

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);

    EXPECT_EQ(JsonCommon::Append(src, add, false), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({ "case", "field1" }, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemCase.GetItemValue().GetIntValue(), 1);

    JsonObject itemName = src.FindItem({ "name" }, errCode);
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
    EXPECT_EQ(JsonCommon::Append(src, add, false), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({ "grade" }, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemCase.GetItemValue().GetIntValue(), 99); // 99: grade

    JsonObject itemName = src.FindItem({ "name", "1" }, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemName.GetItemValue().GetStringValue(), "Neco");
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest003, TestSize.Level0)
{
    std::string document =
        R""({"name":["Tmn","BB","Alice"],"age":[1,2,3],"addr":[{"city":"shanghai","postal":200001},{"city":"wuhan","postal":430000}]})"";
    std::string updateDoc =
        R""({"name":["Xue","Neco","Lip"],"age":18,"addr":[{"city":"shanghai","postal":200001},{"city":"beijing","postal":100000}]})"";

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(JsonCommon::Append(src, add, false), E_OK);

    GLOGD("result: %s", src.Print().c_str());
    JsonObject itemCase = src.FindItem({ "addr", "1", "city" }, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemCase.GetItemValue().GetStringValue(), "beijing"); // 99: grade

    JsonObject itemName = src.FindItem({ "name", "1" }, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemName.GetItemValue().GetStringValue(), "Neco");
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest004, TestSize.Level0)
{
    std::string document = R""({"name":["Tmn","BB","Alice"]})"";
    std::string updateDoc = R""({"name.5":"GG"})"";
    ;

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(JsonCommon::Append(src, add, false), -E_NO_DATA);
    GLOGD("result: %s", src.Print().c_str());
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest005, TestSize.Level0)
{
    std::string document = R""({"name":["Tmn","BB","Alice"]})"";
    std::string updateDoc = R""({"name.2":"GG"})"";
    ;

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);

    EXPECT_EQ(JsonCommon::Append(src, add, false), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({ "name", "2" }, errCode);
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

    EXPECT_EQ(JsonCommon::Append(src, add, false), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({ "name", "midle.AA" }, errCode);
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

    EXPECT_EQ(JsonCommon::Append(src, add, false), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({ "name", "first", "0" }, errCode);
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

    EXPECT_EQ(JsonCommon::Append(src, add, false), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({ "name", "first", "0" }, errCode);
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

    EXPECT_EQ(JsonCommon::Append(src, add, false), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({ "name", "first" }, errCode);
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

    EXPECT_EQ(JsonCommon::Append(src, add, false), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({ "name", "first", "XX" }, errCode);
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

    EXPECT_EQ(JsonCommon::Append(src, add, false), -E_DATA_CONFLICT);
    GLOGD("result: %s", src.Print().c_str());
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest012, TestSize.Level0)
{
    std::string document = R""({"name":["Tmn","BB","Alice"]})"";
    std::string updateDoc = R""({"name.first":"GG"})"";
    ;

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(JsonCommon::Append(src, add, false), -E_DATA_CONFLICT);
    GLOGD("result: %s", src.Print().c_str());
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest013, TestSize.Level0)
{
    std::string document = R""({"name":["Tmn","BB","Alice"]})"";
    std::string updateDoc = R""({"name":{"first":"GG"}})"";
    ;

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(JsonCommon::Append(src, add, false), E_OK);
    GLOGD("result: %s", src.Print().c_str());
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest014, TestSize.Level0)
{
    std::string document = R""({"name":{"first":"Xue","second":"Lang"}})"";
    std::string updateDoc = R""({"name.0":"GG"})"";
    ;

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(JsonCommon::Append(src, add, false), -E_DATA_CONFLICT);
    GLOGD("result: %s", src.Print().c_str());
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectAppendTest015, TestSize.Level0)
{
    std::string document = R""({"name":{"first":"Xue","second":"Lang"}})"";
    std::string updateDoc = R""({"name.first":["GG","MM"]})"";
    ;

    int errCode = E_OK;
    JsonObject src = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject add = JsonObject::Parse(updateDoc, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(JsonCommon::Append(src, add, false), E_OK);
    GLOGD("result: %s", src.Print().c_str());

    JsonObject itemCase = src.FindItem({ "name", "first", "0" }, errCode);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(itemCase.GetItemValue().GetStringValue(), "GG");
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest001, TestSize.Level0)
{
    std::string document = R""({"name":{"first": {"job" : "it"}}, "t1" : {"second":"Lang"}})"";
    std::string filter = R""({"name":{"first": {"job" : "it"}}, "t1" : {"second":"Lang"}})"";
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);

    std::string document2 = R""({"name":{"first": {"job" : "it"}, "t1" : {"second":"Lang"}}})"";
    std::string filter2 = R""({"name":{"first": {"job" : "NoEqual"}}, "t1" : {"second":"Lang"}})"";
    int errCode2 = E_OK;
    JsonObject srcObj2 = JsonObject::Parse(document2, errCode2);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj2 = JsonObject::Parse(filter2, errCode2);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj2, filterObj2, errCode), false);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest002, TestSize.Level0)
{
    std::string document =
        R""({"item": [{"gender":"girl"}, "GG"], "instock": [{"warehouse":"A", "qty":5}, {"warehouse":"C", "qty":15}]})"";
    std::string filter = R""({"instock": {"warehouse":"A", "qty":5}})"";
    ;
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest003, TestSize.Level0)
{
    std::string document =
        R""({"item": [{"gender":"girl"}, "GG"], "instock": [{"warehouse":"A", "qty":5}, {"warehouse":"C", "qty":15}]})"";
    std::string filter = R""({"item": "GG"})"";
    ;
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), false);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest004, TestSize.Level0)
{
    std::string document =
        R""({"item": [{"gender":"girl"}, "GG"], "instock": [{"warehouse":"A", "qty":5}, {"warehouse":"C", "qty":15}]})"";
    std::string filter = R""({"item": "GG"})"";
    ;
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), false);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest005, TestSize.Level0)
{
    std::string document =
        R""({"item": ["GG", "AA"], "instock": [{"warehouse":"A", "qty":5}, {"warehouse":"C", "qty":15}]})"";
    std::string filter = R""({"item": ["GG", "AA"]})"";
    ;
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest006, TestSize.Level0)
{
    std::string document =
        R""({"item": ["GG", {"gender":"girl"}], "instock": [{"warehouse":"A", "qty":5}, {"warehouse":"C", "qty":15}]})"";
    std::string filter = R""({"item.0": "GG"})"";
    ;
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest007, TestSize.Level0)
{
    std::string document =
        R""({"item": ["GG", {"gender":"girl"}], "instock": [{"warehouse":"A", "qty":5}, {"warehouse":"C", "qty":15}]})"";
    std::string filter = R""({"item": ["GG", {"gender":"girl"}]})"";
    ;
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest008, TestSize.Level0)
{
    std::string document =
        R""({"item": ["GG", {"gender":"girl", "hobby" : "IT"}], "instock": [{"warehouse":"A", "qty":5}, {"warehouse":"C", "qty":15}]})"";
    std::string filter = R""({"item": {"gender":"girl"}})"";
    ;
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest009, TestSize.Level0)
{
    std::string document =
        R""({"item": ["GG", {"gender":"girl", "hobby" : "IT"}], "instock": [{"qty" : 16, "warehouse":"A"},
            {"warehouse":"C", "qty":15}]})"";
    std::string filter = R""({"instock.warehouse": "A"})"";
    ;
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest010, TestSize.Level0)
{
    std::string document = R""({"item": ["GG", {"gender":"girl", "hobby" : "IT"}], "instock": [{"warehouse":"A"},
            {"warehouse":"C", "qty":15}]})"";
    std::string filter = R""({"instock.warehouse": "C"})"";
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest011, TestSize.Level0)
{
    std::string document =
        R""({"item" : "journal", "instock" : [{"warehose" : "A", "qty" : 5}, {"warehose" : "C", "qty" : 15}]})"";
    std::string filter = R""({"instock" : {"warehose" : "A", "qty" : 5}})"";
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest012, TestSize.Level0)
{
    std::string document =
        R""({"item" : "journal", "instock" : [{"warehose" : "A", "qty" : 5}, {"warehose" : "C", "qty" : 15}]})"";
    std::string filter = R""({"instock" : {"warehose" : "A", "bad" : "2" ,"qty" : 5}})"";
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), false);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest013, TestSize.Level0)
{
    std::string document =
        R""({"item" : "journal", "instock" : [{"warehose" : "A", "qty" : 5}, {"warehose" : "C", "qty" : 15}]})"";
    std::string filter = R""({"instock.qty" : 15})"";
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest014, TestSize.Level0)
{
    std::string document =
        R""({"item" : "journal", "instock" : [{"warehose" : "A", "qty" : 5}, {"warehose" : "C", "qty" : 15}]})"";
    std::string filter = R""({"instock.1.qty" : 15})"";
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest015, TestSize.Level0)
{
    std::string document = R""({"item" : "journal", "qty" : 25, "tags" : ["blank", "red"], "dim_cm" : [14, 21]})"";
    std::string filter = R""({"tags" : ["blank", "red"]})"";
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest016, TestSize.Level0)
{
    std::string document = R""({"item" : "journal", "qty" : 25, "tags" : {"value" : null}, "dim_cm" : [14, 21]})"";
    std::string filter = R""({"tags" : {"value" : null}})"";
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest017, TestSize.Level0)
{
    std::string document = R""({"item" : "journal", "qty" : 25, "dim_cm" : [14, 21]})"";
    std::string filter = R""({"tags" : {"value" : null}})"";
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest018, TestSize.Level0)
{
    std::string document = "{\"_id\" : \"2\", \"name\":\"doc2\",\"item\": 1, \"personInfo\":\
    [1, \"my string\", {\"school\":\"AB\", \"age\" : 51}, true, {\"school\":\"CD\", \"age\" : 15}, false]}";
    std::string filter = R""({"_id" : "2"})"";
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest019, TestSize.Level0)
{
    const char *document = "{\"_id\" : \"1\", \"name\":\"doc1\",\"item\":\"journal\",\"personInfo\":\
    {\"school\":\"AB\", \"age\" : 51}}";
    const char *filter = "{\"personInfo.school\" : \"AB\"}";
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest020, TestSize.Level0)
{
    const char *document = "{\"_id\" : \"3\", \"name\":\"doc3\",\"item\":\"notebook\",\"personInfo\":\
    [{\"school\":\"C\", \"age\" : 5}]}";
    const char *filter = "{\"personInfo\" : [{\"school\":\"C\", \"age\" : 5}]}";
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest021, TestSize.Level0)
{
    const char *document = "{\"_id\" : \"15\", \"name\":\"doc15\",\"personInfo\":[{\"school\":\"C\", \"age\" : 5}]}";
    const char *filter = "{\"item\" : null, \"personInfo\" : [{\"school\":\"C\", \"age\" : 5}]}";
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest022, TestSize.Level0)
{
    string document = R"({"_id" : "001", "key1" : {"key2" : {"key3" : {"key4" : 123, "k42" : "v42"}, "k32" : "v32"}, "k22" : "v22"},
    "k12" : "v12"})";
    string document2 = R"({"_id" : "002", "key1" : {"key2" : {"key3" : {"key4" : 123, "k42" : "v42"}, "k32" : "v32"}}, "k12" : "v12"})";
    const char *filter = R"({"key1" : {"key2" : {"key3" : {"key4" : 123, "k42" : "v42"}, "k32" : "v32"}}})";
    int errCode = E_OK;
    JsonObject srcObj1 = JsonObject::Parse(document, errCode);
    JsonObject srcObj2 = JsonObject::Parse(document2, errCode);
    EXPECT_EQ(errCode, E_OK);
    JsonObject filterObj = JsonObject::Parse(filter, errCode);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj1, filterObj, errCode), false);
    EXPECT_EQ(JsonCommon::IsJsonNodeMatch(srcObj2, filterObj, errCode), true);
}

HWTEST_F(DocumentDBJsonCommonTest, JsonObjectisFilterCheckTest023, TestSize.Level0)
{
    std::string document = "{\"name\": 1, \"personInfo.school\": 1, \"personInfo.age\": 1}";
    int errCode = E_OK;
    JsonObject srcObj = JsonObject::Parse(document, errCode);
    EXPECT_EQ(errCode, E_OK);
    auto path = JsonCommon::ParsePath(srcObj, errCode);
    for (auto singlePath : path) {
        for (auto filedName : singlePath) {
            GLOGE("filedName is =========>%s", filedName.c_str());
        }
        GLOGE("///////////////////////////");
    }
}
}