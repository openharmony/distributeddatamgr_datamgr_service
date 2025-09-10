/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "rdb_types_utils.h"

#include <gtest/gtest.h>
using namespace testing::ext;
using namespace testing;
using namespace OHOS::DistributedRdb;
namespace OHOS::Test {
class RdbTypesUtilsTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};
void RdbTypesUtilsTest::SetUpTestCase(void) {};
void RdbTypesUtilsTest::TearDownTestCase(void) {};
void RdbTypesUtilsTest::SetUp() {};
void RdbTypesUtilsTest::TearDown() {};
/**
* @tc.name: GetSearchableTables_001
* @tc.desc: get searchable tables from RdbChangedData.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(RdbTypesUtilsTest, GetSearchableTables_001, TestSize.Level0)
{
    RdbChangedData changedData;
    changedData.tableData = {
        { "table1", { .isTrackedDataChange = true } },
        { "table2", { .isP2pSyncDataChange = true } },
        { "table3", { .isP2pSyncDataChange = true } },
        { "table4", { .isTrackedDataChange = true } },
    };
    auto searchableTables = RdbTypesUtils::GetSearchableTables(changedData);
    std::vector<std::string> tagTables = { "table1", "table4" };
    EXPECT_EQ(searchableTables, tagTables);
}

/**
* @tc.name: GetP2PTables_001
* @tc.desc: get p2p sync tables from RdbChangedData.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(RdbTypesUtilsTest, GetP2PTables_001, TestSize.Level0)
{
    RdbChangedData changedData;
    changedData.tableData = {
        { "table1", { .isTrackedDataChange = true } },
        { "table2", { .isP2pSyncDataChange = true } },
        { "table3", { .isP2pSyncDataChange = true } },
        { "table4", { .isTrackedDataChange = true } },
    };
    auto p2pTables = RdbTypesUtils::GetP2PTables(changedData);
    std::vector<std::string> tagTables = { "table2", "table3" };
    EXPECT_EQ(p2pTables, tagTables);
}

/**
* @tc.name: Convert_001
* @tc.desc: convert DistributedRdb Reference to DistributedData::Reference.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(RdbTypesUtilsTest, Convert_001, TestSize.Level0)
{
    std::vector<Reference> references = {
        { "srcTable", "tagTable", { {"uuid", "srcId"} } },
        { "srcTable1", "tagTable1", { {"uuid", "srcId"} } },
        { "srcTable2", "tagTable2", { {"uuid", "srcId"} } },
    };
    auto relations = RdbTypesUtils::Convert(references);
    std::vector<DistributedData::Reference> tagRelations = {
        { "srcTable", "tagTable", { { "uuid", "srcId" } } },
        { "srcTable1", "tagTable1", { { "uuid", "srcId" } } },
        { "srcTable2", "tagTable2", { { "uuid", "srcId" } } },
    };
    EXPECT_EQ(relations.size(), tagRelations.size());
    for (int i = 0; i < tagRelations.size(); ++i) {
        EXPECT_EQ(relations[i].sourceTable, tagRelations[i].sourceTable);
        EXPECT_EQ(relations[i].targetTable, tagRelations[i].targetTable);
        EXPECT_EQ(relations[i].refFields, tagRelations[i].refFields);
    }
}
}