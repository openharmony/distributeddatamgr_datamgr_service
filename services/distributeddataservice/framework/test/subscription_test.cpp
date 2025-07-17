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

#include "cloud/subscription.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class SubscriptionTest : public testing::Test {
public:
    const std::map<std::string, std::string> testRelation = { { "testUserId", "testBundleName" } };
    const std::map<std::string, uint64_t> testExpiresTime = { { "1h", 3600 } };
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

/**
* @tc.name: RelationMarshal
* @tc.desc: relation marshal.
* @tc.type: FUNC
*/
HWTEST_F(SubscriptionTest, RelationMarshal, TestSize.Level1)
{
    Subscription::Relation relation;
    relation.id = "testId";
    relation.bundleName = "testBundleName";
    relation.relations = testRelation;
    Subscription::json node;
    relation.Marshal(node);
    ASSERT_EQ(node["id"], "testId");
    ASSERT_EQ(node["bundleName"], "testBundleName");
    ASSERT_EQ(node["relations"], testRelation);
}

/**
* @tc.name: RelationUnmarshal
* @tc.desc: relation unmarshal.
* @tc.type: FUNC
*/
HWTEST_F(SubscriptionTest, RelationUnmarshal, TestSize.Level1)
{
    Subscription::json node;
    node["id"] = "testId";
    node["bundleName"] = "testBundleName";
    node["relations"] = testRelation;
    Subscription::Relation relation;
    relation.Unmarshal(node);
    ASSERT_EQ(relation.id, "testId");
    ASSERT_EQ(relation.bundleName, "testBundleName");
    ASSERT_EQ(relation.relations, testRelation);
}

/**
* @tc.name: Marshal
* @tc.desc: marshal.
* @tc.type: FUNC
*/
HWTEST_F(SubscriptionTest, Marshal, TestSize.Level1)
{
    Subscription subscription;
    subscription.userId = 100;
    subscription.id = "testId";
    subscription.expiresTime = testExpiresTime;
    Subscription::json node;
    subscription.Marshal(node);
    ASSERT_EQ(node["userId"], 100);
    ASSERT_EQ(node["id"], "testId");
    ASSERT_EQ(node["expiresTime"], testExpiresTime);
}

/**
* @tc.name: Unmarshal
* @tc.desc: unmarshal.
* @tc.type: FUNC
*/
HWTEST_F(SubscriptionTest, Unmarshal, TestSize.Level1)
{
    Subscription::json node;
    node["userId"] = 100;
    node["id"] = "testId";
    node["expiresTime"] = testExpiresTime;
    Subscription subscription;
    subscription.Unmarshal(node);
    ASSERT_EQ(subscription.userId, 100);
    ASSERT_EQ(subscription.id, "testId");
    ASSERT_EQ(subscription.expiresTime, testExpiresTime);
}

/**
* @tc.name: GetKey
* @tc.desc: get key.
* @tc.type: FUNC
*/
HWTEST_F(SubscriptionTest, GetKey, TestSize.Level1)
{
    Subscription subscription;
    subscription.userId = 100;
    std::string key = subscription.GetKey();
    ASSERT_EQ(key, "CLOUD_SUBSCRIPTION###100");
}

/**
* @tc.name: GetRelationKey
* @tc.desc: get relation key.
* @tc.type: FUNC
*/
HWTEST_F(SubscriptionTest, GetRelationKey, TestSize.Level1)
{
    Subscription subscription;
    subscription.userId = 100;
    std::string relationKey = subscription.GetRelationKey("testBundleName");
    ASSERT_EQ(relationKey, "CLOUD_RELATION###100###testBundleName");
}

/**
* @tc.name: GetKey002
* @tc.desc: get key.
* @tc.type: FUNC
*/
HWTEST_F(SubscriptionTest, GetKey002, TestSize.Level1)
{
    std::string key = Subscription::GetKey(100);
    ASSERT_EQ(key, "CLOUD_SUBSCRIPTION###100");
}

/**
* @tc.name: GetRelationKey002
* @tc.desc: get relation key.
* @tc.type: FUNC
*/
HWTEST_F(SubscriptionTest, GetRelationKey002, TestSize.Level1)
{
    std::string relationKey = Subscription::GetRelationKey(100, "testBundleName");
    ASSERT_EQ(relationKey, "CLOUD_RELATION###100###testBundleName");
}

/**
* @tc.name: GetPrefix
* @tc.desc: get prefix.
* @tc.type: FUNC
*/
HWTEST_F(SubscriptionTest, GetPrefix, TestSize.Level1)
{
    std::initializer_list<std::string> fields = { "field1", "field2" };
    std::string prefix = Subscription::GetPrefix(fields);
    ASSERT_EQ(prefix, "CLOUD_SUBSCRIPTION###field1###field2");
}

/**
* @tc.name: GetMinExpireTime001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(SubscriptionTest, GetMinExpireTime001, TestSize.Level1)
{
    Subscription subscription;
    subscription.userId = 100;
    subscription.id = "testId";
    subscription.expiresTime = testExpiresTime;
    auto expire = subscription.GetMinExpireTime();
    EXPECT_EQ(expire, 3600);
}

/**
* @tc.name: GetMinExpireTime002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(SubscriptionTest, GetMinExpireTime002, TestSize.Level1)
{
    Subscription subscription;
    subscription.userId = 100;
    subscription.id = "testId";
    std::map<std::string, uint64_t> testExpiresTimes = { { } };
    subscription.expiresTime = testExpiresTimes;
    auto expire = subscription.GetMinExpireTime();
    EXPECT_EQ(expire, 0);
}
} // namespace OHOS::Test