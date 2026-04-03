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
#include "mock/common_event_manager_mock.h"

namespace {
using namespace OHOS::DistributedData;
using namespace testing;
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
        mock_ = std::make_shared<CommonEventManagerMock>();
        screenLock_ = std::make_shared<ScreenLock>();
    }
    static void TearDownTestCase(void)
    {
        screenLock_ = nullptr;
        mock_ = nullptr;
    }
    void SetUp() {}
    void TearDown() {}

protected:
    static std::shared_ptr<CommonEventManagerMock> mock_;
    static std::shared_ptr<ScreenLock> screenLock_;
    static constexpr int maxWaitTime = 3;
};

std::shared_ptr<CommonEventManagerMock> ScreenLockTest::mock_;
std::shared_ptr<ScreenLock> ScreenLockTest::screenLock_;

/**
 * @tc.name: Subscribe001
 * @tc.desc: subscribe nullptr or empty name observer
 * @tc.type: FUNC
 */
HWTEST_F(ScreenLockTest, Subscribe001, TestSize.Level0)
{
    screenLock_->Subscribe(nullptr);
    EXPECT_TRUE(screenLock_->observerMap_.Empty());

    auto observer = std::make_shared<ScreenLockObserver>();
    observer->SetName("");
    screenLock_->Subscribe(observer);
    EXPECT_TRUE(screenLock_->observerMap_.Empty());
}

/**
* @tc.name: Subscribe002
* @tc.desc: valid param
* @tc.type: FUNC
*/
HWTEST_F(ScreenLockTest, Subscribe002, TestSize.Level0)
{
    auto observer = std::make_shared<ScreenLockObserver>();
    screenLock_->Subscribe(observer);
    EXPECT_TRUE(screenLock_->observerMap_.Contains(observer->GetName()));

    screenLock_->Subscribe(observer);
    EXPECT_EQ(screenLock_->observerMap_.Size(), 1);

    screenLock_->Unsubscribe(observer);
    EXPECT_TRUE(screenLock_->observerMap_.Empty());
}

/**
 * @tc.name: SubscribeScreenEvent001
 * @tc.desc: cover all branches: eventSubscriber_ null/not null, executors_ null/not null
 * @tc.type: FUNC
 */
HWTEST_F(ScreenLockTest, SubscribeScreenEvent001, TestSize.Level0)
{
    EXPECT_EQ(screenLock_->eventSubscriber_, nullptr);
    EXPECT_EQ(screenLock_->executors_, nullptr);
    EXPECT_CALL(*mock_, SubscribeCommonEvent(_)).Times(0);
    screenLock_->SubscribeScreenEvent();
    EXPECT_NE(screenLock_->eventSubscriber_, nullptr);

    auto executor = std::make_shared<OHOS::ExecutorPool>(1, 0);
    screenLock_->BindExecutor(executor);

    bool subscribed = false;
    EXPECT_CALL(*mock_, SubscribeCommonEvent(_)).WillRepeatedly([&subscribed](auto &) {
        subscribed = true;
        return true;
    });
    screenLock_->SubscribeScreenEvent();
    int elapsed = 0;
    while (!subscribed && elapsed < maxWaitTime) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        elapsed++;
    }
    ASSERT_TRUE(subscribed);
    screenLock_->executors_ = nullptr;
}

/**
 * @tc.name: UnsubscribeScreenEvent001
 * @tc.desc: unsubscribe screen event success
 * @tc.type: FUNC
 */
HWTEST_F(ScreenLockTest, UnsubscribeScreenEvent001, TestSize.Level0)
{
    screenLock_->SubscribeScreenEvent();
    bool called = false;
    EXPECT_CALL(*mock_, UnSubscribeCommonEvent(_)).WillOnce([&called](auto &) {
        called = true;
        return true;
    });
    screenLock_->UnsubscribeScreenEvent();
    ASSERT_TRUE(called);
}

/**
 * @tc.name: UnsubscribeScreenEvent002
 * @tc.desc: unsubscribe screen event fail
 * @tc.type: FUNC
 */
HWTEST_F(ScreenLockTest, UnsubscribeScreenEvent002, TestSize.Level0)
{
    screenLock_->SubscribeScreenEvent();
    bool called = false;
    EXPECT_CALL(*mock_, UnSubscribeCommonEvent(_)).WillOnce([&called](auto &) {
        called = true;
        return false;
    });
    screenLock_->UnsubscribeScreenEvent();
    ASSERT_TRUE(called);
}

/**
 * @tc.name: GetTask001
 * @tc.desc: subscribe fail, executors_ null, no retry
 * @tc.type: FUNC
 */
HWTEST_F(ScreenLockTest, GetTask001, TestSize.Level0)
{
    bool subscribed = false;
    EXPECT_CALL(*mock_, SubscribeCommonEvent(_)).WillRepeatedly([&subscribed](auto &) {
        subscribed = true;
        return false;
    });
    EXPECT_EQ(screenLock_->executors_, nullptr);
    screenLock_->GetTask(0)();
    ASSERT_TRUE(subscribed);
}

/**
 * @tc.name: GetTask002
 * @tc.desc: subscribe fail, retry exceeded, no retry
 * @tc.type: FUNC
 */
HWTEST_F(ScreenLockTest, GetTask002, TestSize.Level0)
{
    bool subscribed = false;
    EXPECT_CALL(*mock_, SubscribeCommonEvent(_)).WillRepeatedly([&subscribed](auto &) {
        subscribed = true;
        return false;
    });
    auto executor = std::make_shared<OHOS::ExecutorPool>(1, 0);
    screenLock_->BindExecutor(executor);
    screenLock_->GetTask(ScreenLock::MAX_RETRY_TIMES)();
    ASSERT_TRUE(subscribed);
    screenLock_->executors_ = nullptr;
}

/**
 * @tc.name: GetTask003
 * @tc.desc: subscribe fail, has executor, schedule retry
 * @tc.type: FUNC
 */
HWTEST_F(ScreenLockTest, GetTask003, TestSize.Level0)
{
    bool scheduled = false;
    bool retried = false;
    EXPECT_CALL(*mock_, SubscribeCommonEvent(_)).WillRepeatedly([&](auto &) {
        if (scheduled) {
            retried = true;
            return false;
        }
        scheduled = true;
        return false;
    });
    auto executor = std::make_shared<OHOS::ExecutorPool>(1, 0);
    screenLock_->BindExecutor(executor);
    screenLock_->GetTask(ScreenLock::MAX_RETRY_TIMES - 1)();
    int elapsed = 0;
    while (!retried && elapsed < maxWaitTime) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        elapsed++;
    }
    ASSERT_TRUE(retried);
    screenLock_->executors_ = nullptr;
}
} // namespace