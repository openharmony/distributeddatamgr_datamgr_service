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
#include "utils/constant.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::DistributedData;

class ConstantTest : public testing::Test {
};

/**
* @tc.name: EqualTest
* @tc.desc: Equal.
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(ConstantTest, EqualTest, TestSize.Level1)
{
    ASSERT_TRUE(Constant::Equal(true, true));
    ASSERT_TRUE(Constant::Equal(false, false));
    ASSERT_FALSE(Constant::Equal(true, false));
    ASSERT_FALSE(Constant::Equal(false, true));
}

/**
* @tc.name: NotEqualTest
* @tc.desc: NotEqual.
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(ConstantTest, NotEqualTest, TestSize.Level1)
{
    ASSERT_TRUE(Constant::NotEqual(true, false));
    ASSERT_TRUE(Constant::NotEqual(false, true));
    ASSERT_FALSE(Constant::NotEqual(true, true));
    ASSERT_FALSE(Constant::NotEqual(false, false));
}

/**
* @tc.name: SplitTest
* @tc.desc: SplitEmptyStringTest.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ConstantTest, SplitEmptyStringTest, TestSize.Level1)
{
    auto tokens = Constant::Split("", ",");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0], "");
}

/**
* @tc.name: SplitTest
* @tc.desc: SplitStringOneDelimTest.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ConstantTest, SplitStringOneDelimTest, TestSize.Level1)
{
    auto tokens = Constant::Split("abc_123", "_");
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0], "abc");
    EXPECT_EQ(tokens[1], "123");
}

/**
* @tc.name: SplitTest
* @tc.desc: SplitStringOneDelimAtBeginTest.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ConstantTest, SplitStringOneDelimAtBeginTest, TestSize.Level1)
{
    auto tokens = Constant::Split("#abc123", "#");
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0], "");
    EXPECT_EQ(tokens[1], "abc123");
}

/**
* @tc.name: SplitTest
* @tc.desc: SplitStringOneDelimAtEndTest.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ConstantTest, SplitStringOneDelimAtEndTest, TestSize.Level1)
{
    auto tokens = Constant::Split("abc123!", "!");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0], "abc123");
}

/**
* @tc.name: SplitTest
* @tc.desc: SplitStringMutipleDelimTest.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ConstantTest, SplitStringMutipleDelimTest, TestSize.Level1)
{
    auto tokens = Constant::Split("abc!123!", "!");
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0], "abc");
    EXPECT_EQ(tokens[1], "123");
}

/**
* @tc.name: SplitTest
* @tc.desc: SplitStringNoDelimTest.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ConstantTest, SplitStringNoDelimTest, TestSize.Level1)
{
    auto tokens = Constant::Split("abc123", "^");
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0], "abc123");
}

/**
* @tc.name: SplitTest
* @tc.desc: SplitStringContinuousDelimTest.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ConstantTest, SplitStringContinuousDelimTest, TestSize.Level1)
{
    auto tokens = Constant::Split("abc$$$123", "$");
    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0], "abc");
    EXPECT_EQ(tokens[1], "");
    EXPECT_EQ(tokens[2], "");
    EXPECT_EQ(tokens[3], "123");
}

/**
* @tc.name: SplitTest
* @tc.desc: SplitStringLongDelimTest.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ConstantTest, SplitStringLongDelimTest, TestSize.Level1)
{
    auto tokens = Constant::Split("abc&&&123", "&&");
    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0], "abc");
    EXPECT_EQ(tokens[1], "&123");
}
