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
#define LOG_TAG "RdbQueryTest"

#include "rdb_query.h"

#include "gtest/gtest.h"
#include "log_print.h"
#include "utils/anonymous.h"
#include "value_proxy.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedRdb;

namespace OHOS::Test {
namespace DistributedRDBTest {
class RdbQueryTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: RdbQueryTest001
* @tc.desc: RdbQuery function empty test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbQueryTest, RdbQueryTest001, TestSize.Level1)
{
    RdbQuery rdbQuery;
    uint64_t tid = RdbQuery::TYPE_ID;
    bool result = rdbQuery.IsEqual(tid);
    EXPECT_TRUE(result);
    std::vector<std::string> tables = rdbQuery.GetTables();
    EXPECT_TRUE(tables.empty());
    std::string devices = "devices1";
    std::string sql = "SELECT * FROM table";
    Values args;
    rdbQuery.MakeRemoteQuery(devices, sql, std::move(args));
    EXPECT_TRUE(rdbQuery.IsRemoteQuery());
    EXPECT_EQ(rdbQuery.GetDevices().size(), 1);
    EXPECT_EQ(rdbQuery.GetDevices()[0], devices);
    DistributedRdb::PredicatesMemo predicates;
    rdbQuery.MakeQuery(predicates);
    rdbQuery.MakeCloudQuery(predicates);
    EXPECT_EQ(predicates.tables_.size(), 0);
    EXPECT_TRUE(predicates.tables_.empty());
    EXPECT_EQ(predicates.operations_.size(), 0);
}

/**
* @tc.name: RdbQueryTest002
* @tc.desc: RdbQuery function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbQueryTest, RdbQueryTest002, TestSize.Level1)
{
    RdbQuery rdbQuery;
    std::string devices = "devices1";
    std::string sql = "SELECT * FROM table";
    Values args;
    rdbQuery.MakeRemoteQuery(devices, sql, std::move(args));
    DistributedRdb::PredicatesMemo predicates;
    predicates.tables_.push_back("table1");
    predicates.tables_.push_back("table2");
    predicates.devices_.push_back("device1");
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::EQUAL_TO, "name", "John Doe");
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::GREATER_THAN, "age", "30");
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::OPERATOR_MAX, "too", "99");
    rdbQuery.MakeQuery(predicates);
    rdbQuery.MakeCloudQuery(predicates);
    EXPECT_EQ(predicates.tables_.size(), 2);
    EXPECT_TRUE(!predicates.tables_.empty());
    EXPECT_EQ(predicates.operations_.size(), 3);
}

/**
* @tc.name: RdbQueryTest003
* @tc.desc: RdbQuery function operation empty test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbQueryTest, RdbQueryTest003, TestSize.Level1)
{
    RdbQuery rdbQuery;
    std::string devices = "devices1";
    std::string sql = "SELECT * FROM table";
    Values args;
    rdbQuery.MakeRemoteQuery(devices, sql, std::move(args));
    DistributedRdb::PredicatesMemo predicates;
    predicates.tables_.push_back("table1");
    predicates.tables_.push_back("table2");
    predicates.devices_.push_back("device1");
    std::vector<std::string> values;
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::EQUAL_TO, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::NOT_EQUAL_TO, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::ORDER_BY, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::CONTAIN, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::BEGIN_WITH, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::END_WITH, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::LIKE, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::GLOB, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::BETWEEN, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::NOT_BETWEEN, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::GREATER_THAN, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::GREATER_THAN_OR_EQUAL, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::LESS_THAN, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::LESS_THAN_OR_EQUAL, "test", values);
    rdbQuery.MakeQuery(predicates);
    EXPECT_TRUE(values.empty());
    EXPECT_TRUE(values.size() != 2);
}

/**
* @tc.name: RdbQueryTest004
* @tc.desc: RdbQuery function operation test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbQueryTest, RdbQueryTest004, TestSize.Level1)
{
    RdbQuery rdbQuery;
    std::string devices = "devices1";
    std::string sql = "SELECT * FROM table";
    Values args;
    rdbQuery.MakeRemoteQuery(devices, sql, std::move(args));
    DistributedRdb::PredicatesMemo predicates;
    predicates.tables_.push_back("table1");
    predicates.tables_.push_back("table2");
    predicates.devices_.push_back("device1");
    std::vector<std::string> values = { "value1", "value2" };
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::EQUAL_TO, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::NOT_EQUAL_TO, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::AND, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::OR, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::ORDER_BY, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::LIMIT, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::BEGIN_GROUP, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::END_GROUP, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::IN, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::NOT_IN, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::CONTAIN, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::BEGIN_WITH, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::END_WITH, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::IS_NULL, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::IS_NOT_NULL, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::LIKE, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::GLOB, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::BETWEEN, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::NOT_BETWEEN, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::GREATER_THAN, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::GREATER_THAN_OR_EQUAL, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::LESS_THAN, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::LESS_THAN_OR_EQUAL, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::DISTINCT, "test", values);
    predicates.AddOperation(DistributedRdb::RdbPredicateOperator::INDEXED_BY, "test", values);
    rdbQuery.MakeQuery(predicates);
    EXPECT_TRUE(!values.empty());
    EXPECT_FALSE(values.size() != 2);
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test