/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef OMIT_JSON
#include <gtest/gtest.h>
#include <cmath>

#include "db_errno.h"
#include "distributeddb_tools_unit_test.h"
#include "log_print.h"
#include "relational_schema_object.h"
#include "schema_utils.h"

using namespace std;
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
    const std::string NORMAL_SCHEMA = R""({
            "SCHEMA_VERSION": "2.0",
            "SCHEMA_TYPE": "RELATIVE",
            "TABLES": [{
                "NAME": "FIRST",
                "DEFINE": {
                    "field_name1": {
                        "COLUMN_ID":1,
                        "TYPE": "STRING",
                        "NOT_NULL": true,
                        "DEFAULT": "abcd"
                    },
                    "field_name2": {
                        "COLUMN_ID":2,
                        "TYPE": "MYINT(21)",
                        "NOT_NULL": false,
                        "DEFAULT": "222"
                    },
                    "field_name3": {
                        "COLUMN_ID":3,
                        "TYPE": "INTGER",
                        "NOT_NULL": false,
                        "DEFAULT": "1"
                    }
                },
                "AUTOINCREMENT": true,
                "UNIQUE": ["field_name1", ["field_name2", "field_name3"]],
                "PRIMARY_KEY": "field_name1",
                "INDEX": {
                    "index_name1": ["field_name1", "field_name2"],
                    "index_name2": ["field_name3"]
                }
            }, {
                "NAME": "SECOND",
                "DEFINE": {
                    "key": {
                        "COLUMN_ID":1,
                        "TYPE": "BLOB",
                        "NOT_NULL": true
                    },
                    "value": {
                        "COLUMN_ID":2,
                        "TYPE": "BLOB",
                        "NOT_NULL": false
                    }
                },
                "PRIMARY_KEY": "field_name1"
            }]
        })"";

    const std::string INVALID_SCHEMA = R""({
            "SCHEMA_VERSION": "2.0",
            "SCHEMA_TYPE": "RELATIVE",
            "TABLES": [{
                "NAME": "FIRST",
                "DEFINE": {
                    "field_name1": {
                        "COLUMN_ID":1,
                        "TYPE": "STRING",
                        "NOT_NULL": true,
                        "DEFAULT": "abcd"
                    },"field_name2": {
                        "COLUMN_ID":2,
                        "TYPE": "MYINT(21)",
                        "NOT_NULL": false,
                        "DEFAULT": "222"
                    }
                },
                "PRIMARY_KEY": "field_name1"
            }]
        })"";

    const std::string INVALID_JSON_STRING = R""({
            "SCHEMA_VERSION": "2.0",
            "SCHEMA_TYPE": "RELATIVE",
            "TABLES": [{
                "NAME": "FIRST",
                "DEFINE": {
                    "field_name1": {)"";

    const std::string SCHEMA_VERSION_STR_1 = R"("SCHEMA_VERSION": "1.0",)";
    const std::string SCHEMA_VERSION_STR_2 = R"("SCHEMA_VERSION": "2.0",)";
    const std::string SCHEMA_VERSION_STR_INVALID = R"("SCHEMA_VERSION": "awd3",)";
    const std::string SCHEMA_TYPE_STR_NONE = R"("SCHEMA_TYPE": "NONE",)";
    const std::string SCHEMA_TYPE_STR_JSON = R"("SCHEMA_TYPE": "JSON",)";
    const std::string SCHEMA_TYPE_STR_FLATBUFFER = R"("SCHEMA_TYPE": "FLATBUFFER",)";
    const std::string SCHEMA_TYPE_STR_RELATIVE = R"("SCHEMA_TYPE": "RELATIVE",)";
    const std::string SCHEMA_TYPE_STR_INVALID = R"("SCHEMA_TYPE": "adewaaSAD",)";

    const std::string SCHEMA_TABLE_STR = R""("TABLES": [{
            "NAME": "FIRST",
            "DEFINE": {
                "field_name1": {
                    "COLUMN_ID":1,
                    "TYPE": "STRING",
                    "NOT_NULL": true,
                    "DEFAULT": "abcd"
                },"field_name2": {
                    "COLUMN_ID":2,
                    "TYPE": "MYINT(21)",
                    "NOT_NULL": false,
                    "DEFAULT": "222"
                }
            },
            "PRIMARY_KEY": "field_name1"
        }])"";

    const std::string TABLE_DEFINE_STR = R""({
        "NAME": "FIRST",
        "DEFINE": {
            "field_name1": {
                "COLUMN_ID":1,
                "TYPE": "STRING",
                "NOT_NULL": true,
                "DEFAULT": "abcd"
            },"field_name2": {
                "COLUMN_ID":2,
                "TYPE": "MYINT(21)",
                "NOT_NULL": false,
                "DEFAULT": "222"
            }
        },
        "PRIMARY_KEY": "field_name1"
    })"";

    const std::string TABLE_DEFINE_STR_NAME = R""("NAME": "FIRST",)"";
    const std::string TABLE_DEFINE_STR_NAME_INVALID = R"("NAME": 123,)";
    const std::string TABLE_DEFINE_STR_FIELDS = R""("DEFINE": {
            "field_name1": {
                "COLUMN_ID":1,
                "TYPE": "STRING",
                "NOT_NULL": true,
                "DEFAULT": "abcd"
            },"field_name2": {
                "COLUMN_ID":2,
                "TYPE": "MYINT(21)",
                "NOT_NULL": false,
                "DEFAULT": "222"
            }
        },)"";
    const std::string TABLE_DEFINE_STR_FIELDS_EMPTY = R""("DEFINE": {},)"";
    const std::string TABLE_DEFINE_STR_FIELDS_NOTYPE = R""("DEFINE": {
            "field_name1": {
                "COLUMN_ID":1,
                "NOT_NULL": true,
                "DEFAULT": "abcd"
            }},)"";
    const std::string TABLE_DEFINE_STR_KEY = R""("PRIMARY_KEY": "field_name1")"";
    const std::string TABLE_DEFINE_STR_KEY_INVALID = R""("PRIMARY_KEY": false)"";

}

class DistributedDBRelationalSchemaObjectTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() override ;
    void TearDown() override {};
};

void DistributedDBRelationalSchemaObjectTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

/**
 * @tc.name: RelationalSchemaParseTest001
 * @tc.desc: Test relational schema parse from json string
 * @tc.type: FUNC
 * @tc.require: AR000GK58I
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaParseTest001, TestSize.Level1)
{
    const std::string schemaStr = NORMAL_SCHEMA;
    RelationalSchemaObject schemaObj;
    int errCode = schemaObj.ParseFromSchemaString(schemaStr);
    EXPECT_EQ(errCode, E_OK);

    RelationalSchemaObject schemaObj2;
    schemaObj2.ParseFromSchemaString(schemaObj.ToSchemaString());
    EXPECT_EQ(errCode, E_OK);
}

/**
 * @tc.name: RelationalSchemaParseTest002
 * @tc.desc: Test relational schema parse from invalid json string
 * @tc.type: FUNC
 * @tc.require: AR000GK58I
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaParseTest002, TestSize.Level1)
{
    RelationalSchemaObject schemaObj;

    std::string schemaStr01(SCHEMA_STRING_SIZE_LIMIT + 1, 's');
    int errCode = schemaObj.ParseFromSchemaString(schemaStr01);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);

    errCode = schemaObj.ParseFromSchemaString(INVALID_JSON_STRING);
    EXPECT_EQ(errCode, -E_JSON_PARSE_FAIL);

    std::string noVersion = "{" + SCHEMA_TYPE_STR_RELATIVE + SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(noVersion);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidVersion1 = "{" + SCHEMA_VERSION_STR_1  + SCHEMA_TYPE_STR_RELATIVE + SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(invalidVersion1);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidVersion2 = "{" + SCHEMA_VERSION_STR_INVALID  + SCHEMA_TYPE_STR_RELATIVE + SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(invalidVersion2);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string noType = "{" + SCHEMA_VERSION_STR_2 + SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(noType);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidType1 = "{" + SCHEMA_VERSION_STR_2 + SCHEMA_TYPE_STR_NONE + SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(invalidType1);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidType2 = "{" + SCHEMA_VERSION_STR_2 + SCHEMA_TYPE_STR_JSON + SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(invalidType2);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidType3 = "{" + SCHEMA_VERSION_STR_2 + SCHEMA_TYPE_STR_FLATBUFFER + SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(invalidType3);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidType4 = "{" + SCHEMA_VERSION_STR_2 + SCHEMA_TYPE_STR_INVALID + SCHEMA_TABLE_STR + "}";
    errCode = schemaObj.ParseFromSchemaString(invalidType4);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string noTable = "{" + SCHEMA_VERSION_STR_2 +
        SCHEMA_TYPE_STR_RELATIVE.substr(0, SCHEMA_TYPE_STR_RELATIVE.length() - 1) + "}";
    errCode = schemaObj.ParseFromSchemaString(noTable);
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);
}

namespace {
std::string GenerateFromTableStr(const std::string tableStr)
{
    return R""({
        "SCHEMA_VERSION": "2.0",
        "SCHEMA_TYPE": "RELATIVE",
        "TABLES": )"" + tableStr + "}";
}
}

/**
 * @tc.name: RelationalSchemaParseTest003
 * @tc.desc: Test relational schema parse from invalid json string
 * @tc.type: FUNC
 * @tc.require: AR000GK58I
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaParseTest003, TestSize.Level1)
{
    RelationalSchemaObject schemaObj;
    int errCode = E_OK;

    // std::string emptyTableStr = GenerateFromTableStr("[]");
    // int errCode = schemaObj.ParseFromSchemaString(emptyTableStr);
    // EXPECT_EQ(errCode, -E_INVALID_ARGS);

    errCode = schemaObj.ParseFromSchemaString(GenerateFromTableStr(TABLE_DEFINE_STR));
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    // errCode = schemaObj.ParseFromSchemaString(GenerateFromTableStr("[" + TABLE_DEFINE_STR + "]"));
    // EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidTableStr01 = "{" + TABLE_DEFINE_STR_FIELDS + TABLE_DEFINE_STR_KEY + "}";
    errCode = schemaObj.ParseFromSchemaString(GenerateFromTableStr("[" + invalidTableStr01 + "]"));
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidTableStr02 = "{" + TABLE_DEFINE_STR_NAME_INVALID + TABLE_DEFINE_STR_FIELDS +
        TABLE_DEFINE_STR_KEY + "}";
    errCode = schemaObj.ParseFromSchemaString(GenerateFromTableStr("[" + invalidTableStr02 + "]"));
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    // std::string invalidTableStr03 = "{" + TABLE_DEFINE_STR_NAME + TABLE_DEFINE_STR_FIELDS_EMPTY +
    //     TABLE_DEFINE_STR_KEY + "}";
    // errCode = schemaObj.ParseFromSchemaString(GenerateFromTableStr("[" + invalidTableStr03 + "]"));
    // EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidTableStr04 = "{" + TABLE_DEFINE_STR_NAME + TABLE_DEFINE_STR_FIELDS_NOTYPE +
        TABLE_DEFINE_STR_KEY + "}";
    errCode = schemaObj.ParseFromSchemaString(GenerateFromTableStr("[" + invalidTableStr04 + "]"));
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);

    std::string invalidTableStr05 = "{" + TABLE_DEFINE_STR_NAME + TABLE_DEFINE_STR_FIELDS +
        TABLE_DEFINE_STR_KEY_INVALID + "}";
    errCode = schemaObj.ParseFromSchemaString(GenerateFromTableStr("[" + invalidTableStr05 + "]"));
    EXPECT_EQ(errCode, -E_SCHEMA_PARSE_FAIL);
}

/**
 * @tc.name: RelationalSchemaCompareTest001
 * @tc.desc: Test relational schema negotiate with same schema string
 * @tc.type: FUNC
 * @tc.require: AR000GK58I
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaCompareTest001, TestSize.Level1)
{
    RelationalSchemaObject schemaObj;
    int errCode = schemaObj.ParseFromSchemaString(NORMAL_SCHEMA);
    EXPECT_EQ(errCode, E_OK);

    RelationalSyncOpinion opinion = RelationalSchemaObject::MakeLocalSyncOpinion(schemaObj, NORMAL_SCHEMA,
        static_cast<uint8_t>(SchemaType::RELATIVE));
    EXPECT_EQ(opinion.GetTableOpinion("FIRST").permitSync, true);
    EXPECT_EQ(opinion.GetTableOpinion("FIRST").checkOnReceive, false);
    EXPECT_EQ(opinion.GetTableOpinion("FIRST").requirePeerConvert, false);
}

/**
 * @tc.name: RelationalSchemaOpinionTest001
 * @tc.desc: Test relational schema sync opinion
 * @tc.type: FUNC
 * @tc.require: AR000GK58I
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaOpinionTest001, TestSize.Level1)
{
    const uint32_t softWareVersion = 106;
    RelationalSyncOpinion opinion;
    opinion.AddSyncOpinion("table_1", SyncOpinion {true, false, false});
    opinion.AddSyncOpinion("table_2", SyncOpinion {false, true, false});
    opinion.AddSyncOpinion("table_3", SyncOpinion {false, false, true});

    uint32_t len = opinion.CalculateParcelLen(softWareVersion);
    std::vector<uint8_t> buff(len, 0);
    Parcel writeParcel(buff.data(), len);
    int errCode = opinion.SerializeData(writeParcel, softWareVersion);
    EXPECT_EQ(errCode, E_OK);

    Parcel readParcel(buff.data(), len);
    RelationalSyncOpinion opinionRecv;
    errCode = RelationalSyncOpinion::DeserializeData(readParcel, opinionRecv);
    EXPECT_EQ(errCode, E_OK);

    EXPECT_EQ(opinion.GetOpinions().size(), opinionRecv.GetOpinions().size());
    for (const auto &it : opinion.GetOpinions()) {
        SyncOpinion tableOpinionRecv = opinionRecv.GetTableOpinion(it.first);
        EXPECT_EQ(it.second.permitSync, tableOpinionRecv.permitSync);
        EXPECT_EQ(it.second.requirePeerConvert, tableOpinionRecv.requirePeerConvert);
    }
}

/**
 * @tc.name: RelationalSchemaNegotiateTest001
 * @tc.desc: Test relational schema negotiate
 * @tc.type: FUNC
 * @tc.require: AR000GK58I
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBRelationalSchemaObjectTest, RelationalSchemaNegotiateTest001, TestSize.Level1)
{
    RelationalSyncOpinion localOpinion;
    localOpinion.AddSyncOpinion("table_1", SyncOpinion {true, false, false});
    localOpinion.AddSyncOpinion("table_2", SyncOpinion {false, true, false});
    localOpinion.AddSyncOpinion("table_3", SyncOpinion {false, false, true});

    RelationalSyncOpinion remoteOpinion;
    remoteOpinion.AddSyncOpinion("table_2", SyncOpinion {true, false, false});
    remoteOpinion.AddSyncOpinion("table_3", SyncOpinion {false, true, false});
    remoteOpinion.AddSyncOpinion("table_4", SyncOpinion {false, false, true});
    RelationalSyncStrategy strategy = RelationalSchemaObject::ConcludeSyncStrategy(localOpinion, remoteOpinion);

    EXPECT_EQ(strategy.GetStrategies().size(), 2);
    EXPECT_EQ(strategy.GetTableStrategy("table_2").permitSync, true);
    EXPECT_EQ(strategy.GetTableStrategy("table_3").permitSync, false);
}
#endif