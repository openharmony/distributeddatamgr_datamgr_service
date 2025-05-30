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

#define LOG_TAG "ScreenManagerTest"
#include <gtest/gtest.h>


#include "nlohmann/json.hpp"
#include "utils/crypto.h"
#include "screen/screen_manager.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class ScreenManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

void ScreenManagerTest::SetUpTestCase()
{
    ScreenManager::GetInstance()->BindExecutor(nullptr);
}

/**
* @tc.name: IsLocked
* @tc.desc: test IsLocked function
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(ScreenManagerTest, IsLocked, TestSize.Level1)
{
    EXPECT_NO_FATAL_FAILURE(ScreenManager::GetInstance()->Subscribe(nullptr));
    EXPECT_NO_FATAL_FAILURE(ScreenManager::GetInstance()->Unsubscribe(nullptr));
    EXPECT_NO_FATAL_FAILURE(ScreenManager::GetInstance()->SubscribeScreenEvent());
    EXPECT_NO_FATAL_FAILURE(ScreenManager::GetInstance()->UnsubscribeScreenEvent());
    ASSERT_FALSE(ScreenManager::GetInstance()->IsLocked());
}
} // namespace OHOS::Test
