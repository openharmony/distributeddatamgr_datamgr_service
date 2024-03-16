/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "StoreTest"

#include "access_token.h"
#include "gtest/gtest.h"
#include "log_print.h"
#include "rdb_query.h"
#include "rdb_types.h"
#include "store/general_store.h"
#include "store/general_value.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;

class GeneralValueTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};
class GeneralStoreTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
 * @tc.name: SetQueryNodesTest
 * @tc.desc: Set and query nodes.
 * @tc.type: FUNC
 * @tc.require: AR000F8N0
 */
HWTEST_F(GeneralValueTest, SetQueryNodesTest, TestSize.Level2)
{
    ZLOGI("GeneralValueTest SetQueryNodesTest begin.");
    std::string tableName = "test_tableName";
    QueryNode node;
    node.op = OHOS::DistributedData::QueryOperation::EQUAL_TO;
    node.fieldName =  "test_fieldName";
    node.fieldValue = {"aaa", "bbb", "ccc"};
    QueryNodes nodes{
        {node}
    };
    OHOS::DistributedRdb::RdbQuery query;
    query.SetQueryNodes(tableName, std::move(nodes));
    QueryNodes nodes1 =  query.GetQueryNodes("test_tableName");
    EXPECT_EQ(nodes1[0].fieldName, "test_fieldName");
    EXPECT_EQ(nodes1[0].op, OHOS::DistributedData::QueryOperation::EQUAL_TO);
    EXPECT_EQ(nodes1[0].fieldValue.size(), 3);
}

/**
* @tc.name: GetMixModeTest
* @tc.desc: get mix mode
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(GeneralStoreTest, GetMixModeTest, TestSize.Level2)
{
    ZLOGI("GeneralStoreTest GetMixModeTest begin.");
    auto mode1 = OHOS::DistributedRdb::TIME_FIRST;
    auto mode2 = GeneralStore::AUTO_SYNC_MODE;

    auto mixMode = GeneralStore::MixMode(mode1, mode2);
    EXPECT_EQ(mixMode, mode1 | mode2);

    auto syncMode = GeneralStore::GetSyncMode(mixMode);
    EXPECT_EQ(syncMode, OHOS::DistributedRdb::TIME_FIRST);

    auto highMode = GeneralStore::GetHighMode(mixMode);
    EXPECT_EQ(highMode, GeneralStore::AUTO_SYNC_MODE);
}
