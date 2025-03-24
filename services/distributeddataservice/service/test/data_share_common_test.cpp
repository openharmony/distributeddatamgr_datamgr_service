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
#define LOG_TAG "DataShareCommonTest"

#include <gtest/gtest.h>
#include <unistd.h>
#include "data_share_profile_config.h"
#include "div_strategy.h"
#include "log_print.h"
#include "strategy.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DataShare;
class DataShareCommonTest : public testing::Test {
public:
    static constexpr int64_t USER_TEST = 100;
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: DivStrategy001
* @tc.desc: test DivStrategy function when three parameters are all nullptr
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareCommonTest, DivStrategy001, TestSize.Level1)
{
    ZLOGI("DataShareCommonTest DivStrategy001 start");
    std::shared_ptr<Strategy> check = nullptr;
    std::shared_ptr<Strategy> trueAction = nullptr;
    std::shared_ptr<Strategy> falseAction = nullptr;
    DivStrategy divStrategy(check, trueAction, falseAction);
    auto context = std::make_shared<Context>();
    bool result = divStrategy.operator()(context);
    EXPECT_EQ(result, false);
    ZLOGI("DataShareCommonTest DivStrategy001 end");
}

/**
* @tc.name: DivStrategy002
* @tc.desc: test DivStrategy function when trueAction and falseAction are nullptr
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareCommonTest, DivStrategy002, TestSize.Level1)
{
    ZLOGI("DataShareCommonTest DivStrategy002 start");
    std::shared_ptr<Strategy> check = std::make_shared<Strategy>();
    std::shared_ptr<Strategy> trueAction = nullptr;
    std::shared_ptr<Strategy> falseAction = nullptr;
    DivStrategy divStrategy(check, trueAction, falseAction);
    auto context = std::make_shared<Context>();
    bool result = divStrategy.operator()(context);
    EXPECT_EQ(result, false);
    ZLOGI("DataShareCommonTest DivStrategy002 end");
}

/**
* @tc.name: DivStrategy003
* @tc.desc: test DivStrategy function when only falseAction is nullptr
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareCommonTest, DivStrategy003, TestSize.Level1)
{
    ZLOGI("DataShareCommonTest DivStrategy003 start");
    std::shared_ptr<Strategy> check = std::make_shared<Strategy>();
    std::shared_ptr<Strategy> trueAction = std::make_shared<Strategy>();
    std::shared_ptr<Strategy> falseAction = nullptr;
    DivStrategy divStrategy(check, trueAction, falseAction);
    auto context = std::make_shared<Context>();
    bool result = divStrategy.operator()(context);
    EXPECT_EQ(result, false);
    ZLOGI("DataShareCommonTest DivStrategy003 end");
}
} // namespace OHOS::Test