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
    bool result = Constant::Equal(true, true);
    ASSERT_TRUE(result);

    result = Constant::Equal(true, false);
    ASSERT_FALSE(result);

    result = OHOS::DistributedData::Constant::Equal(false, false);
    ASSERT_TRUE(result);
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
    bool result = Constant::NotEqual(true, true);
    ASSERT_FALSE(result);

    result = Constant::NotEqual(true, false);
    ASSERT_TRUE(result);

    result = Constant::NotEqual(false, false);
    ASSERT_FALSE(result);
}
