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

#include "screen_lock.h"

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
        notifyUser_ = user;
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
    int32_t notifyUser_ = -1;
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
 * @tc.desc: eventSubscriber_ null/not null, executors_ null/not null
 * @tc.type: FUNC
 */
HWTEST_F(ScreenLockTest, SubscribeScreenEvent001, TestSize.Level0)
{
    EXPECT_EQ(screenLock_->eventSubscriber_, nullptr);
    EXPECT_EQ(screenLock_->executors_, nullptr);
    screenLock_->SubscribeScreenEvent();
    EXPECT_NE(screenLock_->eventSubscriber_, nullptr);

    auto executor = std::make_shared<OHOS::ExecutorPool>(1, 0);
    screenLock_->BindExecutor(executor);
    bool isSubscribe = false;
    EXPECT_CALL(*mock_, SubscribeCommonEvent(_)).WillRepeatedly([&isSubscribe](auto &) {
        isSubscribe = true;
        return true;
    });
    screenLock_->SubscribeScreenEvent();
    while (!isSubscribe) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    EXPECT_TRUE(isSubscribe);

    auto observer = std::make_shared<ScreenLockObserver>();
    screenLock_->Subscribe(observer);
    OHOS::AAFwk::Want want;
    want.SetAction(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED);
    want.SetParam("userId", 0);
    OHOS::EventFwk::CommonEventData data;
    data.SetWant(want);
    screenLock_->eventSubscriber_->OnReceiveEvent(data);
    EXPECT_EQ(observer->notifyUser_, 0);

    screenLock_->executors_ = nullptr;
}

/**
 * @tc.name: UnsubscribeScreenEvent001
 * @tc.desc: unsubscribe screen event success
 * @tc.type: FUNC
 */
HWTEST_F(ScreenLockTest, UnsubscribeScreenEvent001, TestSize.Level0)
{
    bool isUnsubscribe = false;
    EXPECT_CALL(*mock_, UnSubscribeCommonEvent(_)).WillOnce([&isUnsubscribe](auto &) {
        isUnsubscribe = true;
        return true;
    });
    screenLock_->UnsubscribeScreenEvent();
    EXPECT_TRUE(isUnsubscribe);
}

/**
 * @tc.name: UnsubscribeScreenEvent002
 * @tc.desc: unsubscribe screen event fail
 * @tc.type: FUNC
 */
HWTEST_F(ScreenLockTest, UnsubscribeScreenEvent002, TestSize.Level0)
{
    bool isUnsubscribe = false;
    EXPECT_CALL(*mock_, UnSubscribeCommonEvent(_)).WillOnce([&isUnsubscribe](auto &) {
        isUnsubscribe = true;
        return false;
    });
    screenLock_->UnsubscribeScreenEvent();
    EXPECT_TRUE(isUnsubscribe);
}

/**
 * @tc.name: GetTask001
 * @tc.desc: subscribe fail, executors_ null, no retry
 * @tc.type: FUNC
 */
HWTEST_F(ScreenLockTest, GetTask001, TestSize.Level0)
{
    bool isSubscribe = false;
    EXPECT_CALL(*mock_, SubscribeCommonEvent(_)).WillRepeatedly([&isSubscribe](auto &) {
        isSubscribe = true;
        return false;
    });
    EXPECT_EQ(screenLock_->executors_, nullptr);
    screenLock_->GetTask(0)();
    EXPECT_TRUE(isSubscribe);
}

/**
 * @tc.name: GetTask002
 * @tc.desc: subscribe fail twice, retry exceeded MAX_RETRY_TIMES, stop
 * @tc.type: FUNC
 */
HWTEST_F(ScreenLockTest, GetTask002, TestSize.Level0)
{
    uint32_t count = 0;
    EXPECT_CALL(*mock_, SubscribeCommonEvent(_)).WillRepeatedly([&count](auto &) {
        count++;
        return false;
    });
    auto executor = std::make_shared<OHOS::ExecutorPool>(1, 0);
    screenLock_->BindExecutor(executor);
    screenLock_->GetTask(ScreenLock::MAX_RETRY_TIMES - 1)();
    while (count < 2) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    EXPECT_EQ(count, 2);
    screenLock_->executors_ = nullptr;
}

} // namespace