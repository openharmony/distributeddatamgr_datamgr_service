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

#define LOG_TAG "LRUBucketTest"

#include "lru_bucket.h"
#include "gtest/gtest.h"

using namespace testing::ext;
template<typename _Key, typename _Tp> using LRUBucket = OHOS::LRUBucket<_Key, _Tp>;

class LRUBucketTest : public testing::Test {
public:
    struct TestValue {
        std::string id;
        std::string name;
        std::string testCase;
    };
    static constexpr size_t TEST_CAPACITY = 10;

    static void SetUpTestCase(void) {}

    static void TearDownTestCase(void) {}

protected:
    void SetUp()
    {
        bucket_.ResetCapacity(0);
        bucket_.ResetCapacity(TEST_CAPACITY);
        for (int i = 0; i < TEST_CAPACITY; ++i) {
            std::string key = std::string("test_") + std::to_string(i);
            TestValue value = {key, key, "case"};
            bucket_.Set(key, value);
        }
    }

    void TearDown() {}

    LRUBucket<std::string, TestValue> bucket_{TEST_CAPACITY};
};

/**
* @tc.name: insert
* @tc.desc: Set the value to the lru bucket, whose capacity is more than one.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(LRUBucketTest, insert, TestSize.Level0)
{
    bucket_.Set("test_10", {"test_10", "test_10", "case"});
    TestValue value;
    ASSERT_TRUE(!bucket_.Get("test_0", value));
    ASSERT_TRUE(bucket_.Get("test_6", value));
    ASSERT_TRUE(bucket_.ResetCapacity(1));
    ASSERT_TRUE(bucket_.Capacity() == 1);
    ASSERT_TRUE(bucket_.Size() <= 1);
    ASSERT_TRUE(bucket_.Get("test_6", value));
}

/**
* @tc.name: cap_one_insert
* @tc.desc: Set the value to the lru bucket, whose capacity is one.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(LRUBucketTest, cap_one_insert, TestSize.Level0)
{
    bucket_.ResetCapacity(1);
    for (int i = 0; i <= TEST_CAPACITY; ++i) {
        std::string key = std::string("test_") + std::to_string(i);
        TestValue value = {key, key, "find"};
        bucket_.Set(key, value);
    }
    TestValue value;
    ASSERT_TRUE(!bucket_.Get("test_0", value));
    ASSERT_TRUE(bucket_.Get("test_10", value));
}

/**
* @tc.name: cap_zero_insert
* @tc.desc: Set the value to the lru bucket, whose capacity is zero.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(LRUBucketTest, cap_zero_insert, TestSize.Level0)
{
    bucket_.ResetCapacity(0);
    for (int i = 0; i <= TEST_CAPACITY; ++i) {
        std::string key = std::string("test_") + std::to_string(i);
        TestValue value = {key, key, "find"};
        bucket_.Set(key, value);
    }
    TestValue value;
    ASSERT_TRUE(!bucket_.Get("test_10", value));
}

/**
* @tc.name: find_head
* @tc.desc: find the head element from the lru bucket.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(LRUBucketTest, find_head, TestSize.Level0)
{
    TestValue value;
    ASSERT_TRUE(bucket_.ResetCapacity(1));
    ASSERT_TRUE(bucket_.Capacity() == 1);
    ASSERT_TRUE(bucket_.Size() <= 1);
    ASSERT_TRUE(bucket_.Get("test_9", value));
}

/**
* @tc.name: find_tail
* @tc.desc: find the tail element, then the element will move to head.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(LRUBucketTest, find_tail, TestSize.Level0)
{
    TestValue value;
    ASSERT_TRUE(bucket_.Get("test_0", value));
    ASSERT_TRUE(bucket_.ResetCapacity(1));
    ASSERT_TRUE(bucket_.Capacity() == 1);
    ASSERT_TRUE(bucket_.Size() <= 1);
    ASSERT_TRUE(bucket_.Get("test_0", value));
}

/**
* @tc.name: find_mid
* @tc.desc: find the mid element, then the element will move to head.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(LRUBucketTest, find_mid, TestSize.Level0)
{
    TestValue value;
    ASSERT_TRUE(bucket_.Get("test_5", value));
    ASSERT_TRUE(bucket_.ResetCapacity(1));
    ASSERT_TRUE(bucket_.Capacity() == 1);
    ASSERT_TRUE(bucket_.Size() <= 1);
    ASSERT_TRUE(bucket_.Get("test_5", value));
}

/**
* @tc.name: find_and_insert
* @tc.desc: find the tail element, then the element will move to head.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(LRUBucketTest, find_and_insert, TestSize.Level0)
{
    TestValue value;
    if (!bucket_.Get("MyTest", value)) {
        bucket_.Set("MyTest", {"MyTest", "MyTest", "case"});
    }
    ASSERT_TRUE(bucket_.Get("MyTest", value));

    if (!bucket_.Get("test_0", value)) {
        bucket_.Set("test_0", {"test_0", "test_0", "case"});
    }
    ASSERT_TRUE(bucket_.Get("test_0", value));
    ASSERT_TRUE(bucket_.Get("test_5", value));
    ASSERT_TRUE(bucket_.Get("test_4", value));
    ASSERT_TRUE(!bucket_.Get("test_1", value));
    ASSERT_TRUE(bucket_.Get("test_2", value));
}

/**
* @tc.name: del_head
* @tc.desc: delete the head element, then the next element will move to head.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(LRUBucketTest, del_head, TestSize.Level0)
{
    TestValue value;
    ASSERT_TRUE(bucket_.Delete("test_9"));
    ASSERT_TRUE(!bucket_.Get("test_9", value));
    ASSERT_TRUE(bucket_.ResetCapacity(1));
    ASSERT_TRUE(bucket_.Capacity() == 1);
    ASSERT_TRUE(bucket_.Size() <= 1);
    ASSERT_TRUE(bucket_.Get("test_8", value));
}

/**
* @tc.name: del_head
* @tc.desc: delete the tail element, then the lru chain keep valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(LRUBucketTest, del_tail, TestSize.Level0)
{
    TestValue value;
    ASSERT_TRUE(bucket_.Delete("test_0"));
    ASSERT_TRUE(!bucket_.Get("test_0", value));
    ASSERT_TRUE(bucket_.Get("test_4", value));
    ASSERT_TRUE(bucket_.ResetCapacity(1));
    ASSERT_TRUE(bucket_.Capacity() == 1);
    ASSERT_TRUE(bucket_.Size() <= 1);
    ASSERT_TRUE(bucket_.Get("test_4", value));
}

/**
* @tc.name: del_mid
* @tc.desc: delete the mid element, then the lru chain keep valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(LRUBucketTest, del_mid, TestSize.Level0)
{
    TestValue value;
    ASSERT_TRUE(bucket_.Delete("test_5"));
    ASSERT_TRUE(!bucket_.Get("test_5", value));
    ASSERT_TRUE(bucket_.Get("test_4", value));
    ASSERT_TRUE(bucket_.Get("test_6", value));
    ASSERT_TRUE(bucket_.ResetCapacity(2));
    ASSERT_TRUE(bucket_.Capacity() == 2);
    ASSERT_TRUE(bucket_.Size() <= 2);
    ASSERT_TRUE(bucket_.Get("test_4", value));
    ASSERT_TRUE(bucket_.Get("test_6", value));
}

/**
* @tc.name: del_mid
* @tc.desc: the lru bucket has only one element, then delete it.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(LRUBucketTest, cap_one_del, TestSize.Level0)
{
    TestValue value;
    bucket_.ResetCapacity(1);
    ASSERT_TRUE(bucket_.Delete("test_9"));
    ASSERT_TRUE(!bucket_.Get("test_9", value));
}

/**
* @tc.name: del_mid
* @tc.desc: the lru bucket has no element.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(LRUBucketTest, cap_zero_del, TestSize.Level0)
{
    TestValue value;
    bucket_.ResetCapacity(0);
    ASSERT_TRUE(!bucket_.Delete("test_9"));
    ASSERT_TRUE(!bucket_.Get("test_9", value));
}

/**
* @tc.name: update_one
* @tc.desc: update the value and the lru chain won't change.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(LRUBucketTest, update_one, TestSize.Level0)
{
    TestValue value;
    ASSERT_TRUE(bucket_.Update("test_4", {"test_4", "test_4", "update"}));
    ASSERT_TRUE(bucket_.Get("test_4", value));
    ASSERT_TRUE(value.testCase == "update");
    ASSERT_TRUE(bucket_.Update("test_9", {"test_9", "test_9", "update"}));
    ASSERT_TRUE(bucket_.ResetCapacity(1));
    ASSERT_TRUE(bucket_.Capacity() == 1);
    ASSERT_TRUE(bucket_.Size() <= 1);
    ASSERT_TRUE(bucket_.Get("test_4", value));
    ASSERT_TRUE(!bucket_.Get("test_9", value));
}

/**
* @tc.name: update_several
* @tc.desc: update several values and the lru chain won't change.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(LRUBucketTest, update_several, TestSize.Level0)
{
    TestValue value;
    std::map<std::string, TestValue> values = {{"test_2", {"test_2", "test_2", "update"}},
                                               {"test_3", {"test_3", "test_3", "update"}},
                                               {"test_6", {"test_6", "test_6", "update"}}};
    ASSERT_TRUE(bucket_.Update(values));
    ASSERT_TRUE(bucket_.ResetCapacity(3));
    ASSERT_TRUE(bucket_.Capacity() == 3);
    ASSERT_TRUE(bucket_.Size() <= 3);
    ASSERT_TRUE(!bucket_.Get("test_2", value));
    ASSERT_TRUE(!bucket_.Get("test_3", value));
    ASSERT_TRUE(!bucket_.Get("test_6", value));
    ASSERT_TRUE(bucket_.Get("test_9", value));
    ASSERT_TRUE(bucket_.Get("test_8", value));
    ASSERT_TRUE(bucket_.Get("test_7", value));
}