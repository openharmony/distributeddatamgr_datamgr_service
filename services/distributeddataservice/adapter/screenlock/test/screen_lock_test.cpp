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

#include "screenlock/screen_lock.h"

#include <gtest/gtest.h>
namespace {
using namespace OHOS::DistributedData;
using namespace testing::ext;
class ScreenLockObserver : public ScreenManager::Observer {
public:
    void OnScreenUnlocked(int32_t user) override
    {
    }

    std::string GetName() override
    {
        return name_;
    }

    void SetName(const std::string &name)
    {
        name_ = name;
    }

private:
    std::string name_ = "screenTestObserver";
};

class ScreenLockTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
    }
    static void TearDownTestCase(void)
    {
    }
    void SetUp()
    {
    }
    void TearDown()
    {
    }
};

/**
* @tc.name: Subscribe001
* @tc.desc: invalid param
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(ScreenLockTest, Subscribe001, TestSize.Level0)
{
    auto screenLock = std::make_shared<ScreenLock>();
    screenLock->Subscribe(nullptr);
    EXPECT_TRUE(screenLock->observerMap_.Empty());
    auto observer = std::make_shared<ScreenLockObserver>();
    observer->SetName("");
    screenLock->Subscribe(observer);
    EXPECT_TRUE(screenLock->observerMap_.Empty());
}

/**
* @tc.name: Subscribe002
* @tc.desc: valid param
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(ScreenLockTest, Subscribe002, TestSize.Level0)
{
    auto screenLock = std::make_shared<ScreenLock>();
    auto observer = std::make_shared<ScreenLockObserver>();
    screenLock->Subscribe(observer);
    EXPECT_TRUE(screenLock->observerMap_.Contains(observer->GetName()));
    screenLock->Subscribe(observer);
    EXPECT_EQ(screenLock->observerMap_.Size(), 1);
}

/**
* @tc.name: Unsubscribe001
* @tc.desc: test normal unsubscribe
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(ScreenLockTest, Unsubscribe001, TestSize.Level0)
{
    auto screenLock = std::make_shared<ScreenLock>();
    auto observer = std::make_shared<ScreenLockObserver>();
    screenLock->Subscribe(observer);
    EXPECT_EQ(screenLock->observerMap_.Size(), 1);
    screenLock->Unsubscribe(observer);
    EXPECT_TRUE(screenLock->observerMap_.Empty());
}

/**
* @tc.name: SubscribeScreenEvent001
* @tc.desc: subscribe ScreenEvent
* @tc.type: FUNC
* @tc.author:
*/
HWTEST_F(ScreenLockTest, SubscribeScreenEvent001, TestSize.Level0)
{
    auto executor = std::make_shared<OHOS::ExecutorPool>(12, 5);
    auto screenLock = std::make_shared<ScreenLock>();
    screenLock->BindExecutor(executor);
    ASSERT_NE(screenLock->executors_, nullptr);
    EXPECT_EQ(screenLock->eventSubscriber_, nullptr);
    screenLock->SubscribeScreenEvent();
    EXPECT_NE(screenLock->eventSubscriber_, nullptr);
    screenLock->UnsubscribeScreenEvent();
}
} // namespace