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

#include <gtest/gtest.h>
#include "query_helper.h"

namespace OHOS::DistributedKv {
using namespace testing;
using namespace std;

class QueryHelperUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

/**
  * @tc.name: StringToDbQuery001
  * @tc.desc: in order to test HandleEqualTo.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery001, testing::ext::TestSize.Level0)
{
    bool isSuccess = false;
    string query = "^EQUAL INTEGER TYPE_INTEGER 0";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^EQUAL LONG TYPE_LONG 1l";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^EQUAL DOUBLE TYPE_DOUBLE 2.1";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^EQUAL BOOL TYPE_BOOLEAN 3";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^EQUAL STRING TYPE_STRING dafafg";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    query = "^EQUAL CHAR STRING 4";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery002
  * @tc.desc: in order to test HandleNotEqualTo.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery002, testing::ext::TestSize.Level0)
{
    bool isOk = false;
    string query = "^NOT_EQUAL INTEGER TYPE_INTEGER 0";
    (void)QueryHelper::StringToDbQuery(query, isOk);
    EXPECT_TRUE(isOk);

    isOk = false;
    query = "^NOT_EQUAL LONG TYPE_LONG 1l";
    (void)QueryHelper::StringToDbQuery(query, isOk);
    EXPECT_TRUE(isOk);

    isOk = false;
    query = "^NOT_EQUAL DOUBLE TYPE_DOUBLE 2.1";
    (void)QueryHelper::StringToDbQuery(query, isOk);
    EXPECT_TRUE(isOk);

    isOk = false;
    query = "^NOT_EQUAL BOOL TYPE_BOOLEAN 3";
    (void)QueryHelper::StringToDbQuery(query, isOk);
    EXPECT_TRUE(isOk);

    isOk = false;
    query = "^NOT_EQUAL STRING TYPE_STRING dafafg";
    (void)QueryHelper::StringToDbQuery(query, isOk);
    EXPECT_TRUE(isOk);

    query = "^NOT_EQUAL CHAR STRING 4";
    (void)QueryHelper::StringToDbQuery(query, isOk);
    EXPECT_FALSE(isOk);
}

/**
  * @tc.name: StringToDbQuery003
  * @tc.desc: in order to test HandleGreaterThan.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery003, testing::ext::TestSize.Level0)
{
    bool isSuccess = false;
    string query = "^GREATER INTEGER TYPE_INTEGER 2";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^GREATER LONG TYPE_LONG 3l";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^GREATER DOUBLE TYPE_DOUBLE 4.1";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^GREATER STRING TYPE_STRING gohkhkhk";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    query = "^GREATER BOOL STRING 5";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery004
  * @tc.desc: in order to test HandleGreaterThanOrEqualTo.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery004, testing::ext::TestSize.Level0)
{
    bool isSuccess = false;
    string query = "^GREATER_EQUAL INTEGER TYPE_INTEGER 0";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^GREATER_EQUAL LONG TYPE_LONG 2l";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^GREATER_EQUAL DOUBLE TYPE_DOUBLE 4.1";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^GREATER_EQUAL STRING TYPE_STRING lofhfgh";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    query = "^GREATER_EQUAL CHAR STRING 6";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery005
  * @tc.desc: in order to test HandleLessThanOrEqualTo.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery005, testing::ext::TestSize.Level0)
{
    bool isSuccess = false;
    string query = "^LESS_EQUAL INTEGER TYPE_INTEGER 1";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^LESS_EQUAL LONG TYPE_LONG 3l";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^LESS_EQUAL DOUBLE TYPE_DOUBLE 5.1";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^LESS_EQUAL STRING TYPE_STRING lofhfgh";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    query = "^LESS_EQUAL CHAR STRING a";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery006
  * @tc.desc: in order to test Handle's HandleIsNull.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery006, testing::ext::TestSize.Level0)
{
    bool isSuccess = true;
    string query = "^IS_NULL";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);

    isSuccess = false;
    query = "^IS_NULL field1";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery007
  * @tc.desc: in order to test Handle's HandleIn.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery007, testing::ext::TestSize.Level0)
{
    bool isSuccess = true;
    string query = "^IN";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
    isSuccess = true;
    query = "^IN INTEGER grade other ^START";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);

    query = "^IN INTEGER old ^START ^END";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
    isSuccess = false;
    query = "^IN INTEGER old ^START 95 ^END";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
    isSuccess = false;
    query = "^IN INTEGER grade ^START 123";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
    
    isSuccess = false;
    query = "^IN LONG salary ^START ^END";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
    isSuccess = false;
    query = "^IN LONG grade ^START 650 ^END";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
    isSuccess = false;
    query = "^IN LONG range ^START 123478";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
    
    isSuccess = false;
    query = "^IN DOUBLE 1234 ^START ^END";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
    isSuccess = false;
    query = "^IN DOUBLE salary ^START 129456 ^END";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
    isSuccess = false;
    query = "^IN DOUBLE salary ^START 16478";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery008
  * @tc.desc: in order to test Handle's HandleIn.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery008, testing::ext::TestSize.Level0)
{
    bool isSuccess = false;
    string query = "^IN STRING fieldname ^START ^END";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
    isSuccess = false;
    query = "^IN STRING fieldname ^START xadada ^END";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
    isSuccess = false;
    query = "^IN STRING fieldname ^START other";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    query = "^IN CHAR fieldname ^START other";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery009
  * @tc.desc: in order to test Handle's HandleNotIn.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery009, testing::ext::TestSize.Level0)
{
    bool isSuccess = true;
    string query = "^NOT_IN";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
    isSuccess = true;
    query = "^IN INTEGER 12 other ^START";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);

    isSuccess = false;
    query = "^NOT_IN INTEGER name ^START ^END";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
    isSuccess = false;
    query = "^NOT_IN LONG salary ^START 10000 ^END";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
    isSuccess = false;
    query = "^NOT_IN DOUBLE height ^START 175";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^NOT_IN STRING 14578 ^START fieldname";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    query = "^NOT_IN CHAR department ^START other";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery010
  * @tc.desc: in order to test Handle's HandleLike.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery010, testing::ext::TestSize.Level0)
{
    bool isSuccess = true;
    string query = "^LIKE";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);

    query = "^LIKE Name Mr*";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery011
  * @tc.desc: in order to test Handle's HandleNotLike.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery011, testing::ext::TestSize.Level0)
{
    bool isSuccess = true;
    string query = "^NOT_LIKE";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);

    query = "^NOT_LIKE Name Wan*";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery012
  * @tc.desc: in order to test Handle's HandleAnd.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery012, testing::ext::TestSize.Level0)
{
    bool isSuccess = false;
    string query = "^AND";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery013
  * @tc.desc: in order to test Handle's HandleOr.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery013, testing::ext::TestSize.Level0)
{
    bool isSuccess = false;
    string query = "^OR";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery014
  * @tc.desc: in order to test Handle's HandleOrderByAsc.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery014, testing::ext::TestSize.Level0)
{
    bool isSuccess = true;
    string query = "^ASC";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);

    query = "^ASC old";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery015
  * @tc.desc: in order to test Handle's HandleOrderByDesc.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery015, testing::ext::TestSize.Level0)
{
    bool isSuccess = true;
    string query = "^DESC";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);

    query = "^DESC grade";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery016
  * @tc.desc: in order to test Handle's HandleOrderByWriteTime.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery016, testing::ext::TestSize.Level0)
{
    bool isSuccess = true;
    string query = "^OrderByWriteTime";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);

    query = "^OrderByWriteTime salary";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery017
  * @tc.desc: in order to test Handle's HandleLimit.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery017, testing::ext::TestSize.Level0)
{
    bool isSuccess = true;
    string query = "^LIMIT";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);

    query = "^LIMIT 0 5";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery018
  * @tc.desc: in order to test Handle's HandleExtra.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery018, testing::ext::TestSize.Level0)
{
    bool isSuccess = false;
    string query = "^BEGIN_GROUP";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
    
    isSuccess = false;
    query = "^END_GROUP";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    query = "^KEY_PREFIX";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
    query = "^KEY_PREFIX _XXX";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    query = "^IS_NOT_NULL";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
    query = "^IS_NOT_NULL NAME";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    query = "^DEVICE_ID";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
    isSuccess = true;
    query = "^DEVICE_ID 4DAAAJFGKAGNGM ^KEY_PREFIX";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);

    query = "^DEVICE_ID FJGKGNMGJM4DAFA78";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    query = "^SUGGEST_INDEX";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
    query = "^SUGGEST_INDEX xadada";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
    
    query = "^IN_KEYS xxxx instart";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
}

/**
  * @tc.name: StringToDbQuery019
  * @tc.desc: in order to test HandleLessThan.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(QueryHelperUnitTest, StringToDbQuery019, testing::ext::TestSize.Level0)
{
    bool isSuccess = false;
    string query = "^LESS INTEGER TYPE_INTEGER 1";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^LESS LONG TYPE_LONG 2l";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^LESS DOUBLE TYPE_DOUBLE 3.1";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    isSuccess = false;
    query = "^LESS STRING TYPE_STRING bnnjjjh";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);

    query = "^LESS BOOL STRING 5";
    (void)QueryHelper::StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
}
}