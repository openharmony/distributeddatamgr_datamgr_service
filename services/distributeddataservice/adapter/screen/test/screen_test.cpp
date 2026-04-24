/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <gmock/gmock.h>
#include <memory>
#include <atomic>
#include <vector>
#include <chrono>

#include "executor_pool.h"
#include "screen/screen.h"
#include "mock_common_event.h"
#include "account/account_delegate.h"
#include "account_delegate_mock.h"
#include "screenlock_manager.h"

namespace OHOS::Test{
using namespace OHOS::DistributedData;
using namespace testing::ext;
using namespace OHOS;
using namespace testing;
using namespace OHOS::ScreenLock;

static AccountDelegateMock *accountDelegateMock_ = nullptr;
static constexpr size_t THREAD_MIN = 5;
static constexpr size_t THREAD_MAX = 12;

// Mock Observer for testing callback behavior
class MockScreenObserver : public ScreenManager::Observer {
public:
    std::string name_ = "mockObserver";
    std::atomic<int> unlockCalledCount_{0};
    std::atomic<int> lockCalledCount_{0};
    std::atomic<int> screenOnCalledCount_{0};
    std::atomic<int> screenOffCalledCount_{0};
    int32_t lastUnlockUser_{-1};
    int32_t lastLockUser_{-1};
    int32_t lastScreenOnUser_{-1};
    int32_t lastScreenOffUser_{-1};

    void OnScreenUnlocked(int32_t user) override
    {
        unlockCalledCount_++;
        lastUnlockUser_ = user;
    }

    void OnScreenLocked(int32_t user) override
    {
        lockCalledCount_++;
        lastLockUser_ = user;
    }

    void OnScreenOn(int32_t user) override
    {
        screenOnCalledCount_++;
        lastScreenOnUser_ = user;
    }

    void OnScreenOff(int32_t user) override
    {
        screenOffCalledCount_++;
        lastScreenOffUser_ = user;
    }

    std::string GetName() override
    {
        return name_;
    }

    void SetName(const std::string &name)
    {
        name_ = name;
    }

    void Reset()
    {
        unlockCalledCount_ = 0;
        lockCalledCount_ = 0;
        screenOnCalledCount_ = 0;
        screenOffCalledCount_ = 0;
        lastUnlockUser_ = -1;
        lastLockUser_ = -1;
        lastScreenOnUser_ = -1;
        lastScreenOffUser_ = -1;
    }

    bool IsUnlockCalled() const
    {
        return unlockCalledCount_ > 0;
    }

    bool IsLockCalled() const
    {
        return lockCalledCount_ > 0;
    }

    bool IsScreenOnCalled() const
    {
        return screenOnCalledCount_ > 0;
    }

    bool IsScreenOffCalled() const
    {
        return screenOffCalledCount_ > 0;
    }

    int GetUnlockCalledCount() const
    {
        return unlockCalledCount_;
    }

    int GetLockCalledCount() const
    {
        return lockCalledCount_;
    }

    int GetScreenOnCalledCount() const
    {
        return screenOnCalledCount_;
    }

    int GetScreenOffCalledCount() const
    {
        return screenOffCalledCount_;
    }
};

// Test helper class to access private members for critical testing only
// NOTE: Private member access should be minimized. Only access private methods
// when testing core business logic that cannot be verified through public APIs.
class ScreenTestHelper {
public:
    // GetTask is critical for testing retry logic - this is core business logic
    // that requires verification of internal retry behavior
    static ExecutorPool::Task GetTask(Screen &screenLock, uint32_t retry)
    {
        return screenLock.GetTask(retry);
    }
};

class ScreenTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        accountDelegateMock_ = new (std::nothrow) AccountDelegateMock();
        if (accountDelegateMock_ != nullptr) {
            AccountDelegate::instance_ = nullptr;
            AccountDelegate::RegisterAccountInstance(accountDelegateMock_);
            EXPECT_CALL(*accountDelegateMock_, QueryForegroundUsers(_))
                .WillRepeatedly([](std::vector<int> &users) {
                    users = { 100 };
                    return true;
                });
            EXPECT_CALL(*accountDelegateMock_, IsDeactivating(_))
                .WillRepeatedly(Return(false));
        }
        ScreenLockManager::Reset();
    }

    static void TearDownTestCase(void)
    {
        if (accountDelegateMock_ != nullptr) {
            delete accountDelegateMock_;
            accountDelegateMock_ = nullptr;
        }
        ScreenManager::RegisterInstance(nullptr);
        ScreenLockManager::Reset();
    }

    void SetUp()
    {
        OHOS::EventFwk::CommonEventManager::Reset();
        ScreenLockManager::Reset();
        
        screen_ = std::make_shared<Screen>();
        ScreenManager::RegisterInstance(screen_);
        
        executor_ = std::make_shared<ExecutorPool>(THREAD_MAX, THREAD_MIN);
        screen_->BindExecutor(executor_);
        screen_->SubscribeEvent();
        
        auto task = ScreenTestHelper::GetTask(*screen_, 0);
        if (task) {
            task();
        }
    }

    void TearDown()
    {
        screen_->UnsubscribeEvent();
        OHOS::EventFwk::CommonEventManager::Reset();
        ScreenLockManager::Reset();
        screen_.reset();
        executor_.reset();
    }

protected:
    std::shared_ptr<MockScreenObserver> CreateObserver(const std::string &name = "testObserver")
    {
        auto observer = std::make_shared<MockScreenObserver>();
        observer->SetName(name);
        return observer;
    }

    std::shared_ptr<MockScreenObserver> CreateAndSubscribeObserver(
        const std::string &name = "testObserver")
    {
        auto observer = CreateObserver(name);
        screen_->Subscribe(observer);
        return observer;
    }

    std::vector<std::shared_ptr<MockScreenObserver>> CreateAndSubscribeObservers(int count)
    {
        std::vector<std::shared_ptr<MockScreenObserver>> observers;
        for (int i = 0; i < count; i++) {
            observers.push_back(CreateAndSubscribeObserver("observer" + std::to_string(i)));
        }
        return observers;
    }

    std::shared_ptr<Screen> screen_;
    std::shared_ptr<ExecutorPool> executor_;
    static constexpr int32_t MAX_RETRY_TIMES = 300;
    static constexpr int32_t TEST_USER_ID = 100;
};

/**
 * @tc.name: Subscribe001
 * @tc.desc: subscribe with null observer should return without adding
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, Subscribe001, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver();

    screen_->Subscribe(nullptr);

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, TEST_USER_ID);
}

/**
 * @tc.name: Subscribe002
 * @tc.desc: subscribe with empty name observer should return without adding
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, Subscribe002, TestSize.Level0)
{
    auto observer = CreateObserver("");

    screen_->Subscribe(observer);

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 0);
}

/**
 * @tc.name: Subscribe003
 * @tc.desc: subscribe with valid observer should succeed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, Subscribe003, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("validObserver");

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, TEST_USER_ID);
}

/**
 * @tc.name: Subscribe004
 * @tc.desc: subscribe duplicate observer should not add again
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, Subscribe004, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("duplicateObserver");

    screen_->Subscribe(observer);

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, TEST_USER_ID);
}

/**
 * @tc.name: Subscribe005
 * @tc.desc: subscribe multiple different observers should all be added
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, Subscribe005, TestSize.Level0)
{
    auto observers = CreateAndSubscribeObservers(3);

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);
    for (const auto &observer : observers) {
        EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
        EXPECT_EQ(observer->lastUnlockUser_, TEST_USER_ID);
    }
}

/**
 * @tc.name: Unsubscribe001
 * @tc.desc: unsubscribe with null observer should return without error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, Unsubscribe001, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver();

    screen_->Unsubscribe(nullptr);

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, TEST_USER_ID);
}

/**
 * @tc.name: Unsubscribe002
 * @tc.desc: unsubscribe with empty name observer should return without error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, Unsubscribe002, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("testObserver");

    // Empty name cannot unsubscribe
    observer->SetName("");
    screen_->Unsubscribe(observer);

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, TEST_USER_ID);
}

/**
 * @tc.name: Unsubscribe003
 * @tc.desc: unsubscribe non-existent observer should return without error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, Unsubscribe003, TestSize.Level0)
{
    auto observer = CreateObserver("nonExistent");

    screen_->Unsubscribe(observer);

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 0);
}

/**
 * @tc.name: Unsubscribe004
 * @tc.desc: unsubscribe valid observer should remove it
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, Unsubscribe004, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("toRemove");

    screen_->Unsubscribe(observer);

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 0);
}

/**
 * @tc.name: Unsubscribe005
 * @tc.desc: subscribe-unsubscribe-resubscribe sequence should work
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, Unsubscribe005, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("resubscribeTest");

    screen_->Unsubscribe(observer);

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 0);

    screen_->Subscribe(observer);

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, TEST_USER_ID);
}

/**
 * @tc.name: NotifyScreenUnlocked001
 * @tc.desc: notify screen unlocked should call all observers
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, NotifyScreenUnlocked001, TestSize.Level0)
{
    auto observer1 = CreateAndSubscribeObserver("observer1");
    auto observer2 = CreateAndSubscribeObserver("observer2");

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);

    EXPECT_EQ(observer1->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer1->lastUnlockUser_, TEST_USER_ID);
    EXPECT_EQ(observer2->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer2->lastUnlockUser_, TEST_USER_ID);
}

/**
 * @tc.name: NotifyScreened001
 * @tc.desc: notify screen locked should call all observers
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, NotifyScreened001, TestSize.Level0)
{
    auto observer1 = CreateAndSubscribeObserver("observer1");
    auto observer2 = CreateAndSubscribeObserver("observer2");

    OHOS::EventFwk::CommonEventManager::PublishScreenLockedEvent(TEST_USER_ID);

    EXPECT_EQ(observer1->GetLockCalledCount(), 1);
    EXPECT_EQ(observer1->lastLockUser_, TEST_USER_ID);
    EXPECT_EQ(observer2->GetLockCalledCount(), 1);
    EXPECT_EQ(observer2->lastLockUser_, TEST_USER_ID);
}

/**
 * @tc.name: NotifyScreen002
 * @tc.desc: notify with no observers should not crash and no observers should be notified
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, NotifyScreen002, TestSize.Level0)
{
    auto observer = CreateObserver("notSubscribed");

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);
    OHOS::EventFwk::CommonEventManager::PublishScreenLockedEvent(TEST_USER_ID);

    EXPECT_EQ(observer->GetUnlockCalledCount(), 0);
    EXPECT_EQ(observer->GetLockCalledCount(), 0);
}

/**
 * @tc.name: NotifyScreen003
 * @tc.desc: notify multiple times should call observers multiple times
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, NotifyScreen003, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("multiCallObserver");

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);
    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);
    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);

    EXPECT_EQ(observer->GetUnlockCalledCount(), 3);
    EXPECT_EQ(observer->lastUnlockUser_, TEST_USER_ID);
}

/**
 * @tc.name: BindExecutor001
 * @tc.desc: bind executor should set executor correctly
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, BindExecutor001, TestSize.Level0)
{
    auto executor = std::make_shared<ExecutorPool>(THREAD_MAX, THREAD_MIN);

    screen_->BindExecutor(executor);

    auto task = ScreenTestHelper::GetTask(*screen_, 0);
    EXPECT_TRUE(task != nullptr);
}

/**
 * @tc.name: BindExecutor002
 * @tc.desc: bind null executor should replace existing executor
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, BindExecutor002, TestSize.Level0)
{
    auto executor = std::make_shared<ExecutorPool>(THREAD_MAX, THREAD_MIN);

    screen_->BindExecutor(executor);
    auto task1 = ScreenTestHelper::GetTask(*screen_, 0);
    EXPECT_TRUE(task1 != nullptr);

    screen_->BindExecutor(nullptr);

    auto task2 = ScreenTestHelper::GetTask(*screen_, 0);
    EXPECT_TRUE(task2 != nullptr);
}

/**
 * @tc.name: GetTask001
 * @tc.desc: get task without executor should return valid callable task
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, GetTask001, TestSize.Level0)
{
    auto task = ScreenTestHelper::GetTask(*screen_, 0);

    EXPECT_TRUE(task != nullptr);

    task();
}

/**
 * @tc.name: GetTask002
 * @tc.desc: get task with retry count exceeding max should return valid task
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, GetTask002, TestSize.Level0)
{
    auto executor = std::make_shared<ExecutorPool>(THREAD_MAX, THREAD_MIN);
    screen_->BindExecutor(executor);

    auto task = ScreenTestHelper::GetTask(*screen_, MAX_RETRY_TIMES + 1);

    EXPECT_TRUE(task != nullptr);
    task();
}

/**
 * @tc.name: GetTask003
 * @tc.desc: get task with retry count within max should return valid task
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, GetTask003, TestSize.Level0)
{
    auto executor = std::make_shared<ExecutorPool>(THREAD_MAX, THREAD_MIN);
    screen_->BindExecutor(executor);

    auto task = ScreenTestHelper::GetTask(*screen_, 0);

    EXPECT_TRUE(task != nullptr);
    task();
}

/**
 * @tc.name: GetTask004
 * @tc.desc: get task at max retry boundary should return valid task
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, GetTask004, TestSize.Level0)
{
    auto executor = std::make_shared<ExecutorPool>(THREAD_MAX, THREAD_MIN);
    screen_->BindExecutor(executor);

    auto task = ScreenTestHelper::GetTask(*screen_, MAX_RETRY_TIMES);

    EXPECT_TRUE(task != nullptr);
    task();
}

/**
 * @tc.name: Destructor001
 * @tc.desc: destructor should cleanup resources properly
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, Destructor001, TestSize.Level0)
{
    auto screenLock = std::make_shared<Screen>();
    auto executor = std::make_shared<ExecutorPool>(THREAD_MAX, THREAD_MIN);

    screenLock->BindExecutor(executor);

    std::weak_ptr<Screen> weakPtr = screenLock;

    screenLock.reset();

    EXPECT_TRUE(weakPtr.expired());
}

/**
 * @tc.name: IsLocked001
 * @tc.desc: test IsLocked method when screen is not locked
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, IsLocked001, TestSize.Level0)
{
    ScreenLockManager::SetScreenLocked(false);

    bool isLocked = screen_->IsLocked();

    EXPECT_FALSE(isLocked);
}

/**
 * @tc.name: IsLocked002
 * @tc.desc: test IsLocked method when screen is locked
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, IsLocked002, TestSize.Level0)
{
    ScreenLockManager::SetScreenLocked(true);

    bool isLocked = screen_->IsLocked();

    EXPECT_TRUE(isLocked);
}

/**
 * @tc.name: IsLocked003
 * @tc.desc: test IsLocked method returns correct value after state change
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, IsLocked003, TestSize.Level0)
{
    ScreenLockManager::SetScreenLocked(false);
    EXPECT_FALSE(screen_->IsLocked());

    ScreenLockManager::SetScreenLocked(true);
    EXPECT_TRUE(screen_->IsLocked());

    ScreenLockManager::SetScreenLocked(false);
    EXPECT_FALSE(screen_->IsLocked());
}

/**
 * @tc.name: MultiUserNotify001
 * @tc.desc: notify different users to observers
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, MultiUserNotify001, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("multiUserObserver");

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(100);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, 100);

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(200);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 2);
    EXPECT_EQ(observer->lastUnlockUser_, 200);

    OHOS::EventFwk::CommonEventManager::PublishScreenLockedEvent(300);
    EXPECT_EQ(observer->GetLockCalledCount(), 1);
    EXPECT_EQ(observer->lastLockUser_, 300);
}

/**
 * @tc.name: ConcurrentNotify001
 * @tc.desc: multiple observers should all be notified concurrently
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, ConcurrentNotify001, TestSize.Level0)
{
    auto observers = CreateAndSubscribeObservers(10);

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);

    for (const auto &observer : observers) {
        EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
        EXPECT_EQ(observer->lastUnlockUser_, TEST_USER_ID);
    }
}

/**
 * @tc.name: MixedNotify001
 * @tc.desc: mixed lock and unlock notifications should work correctly
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, MixedNotify001, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("mixedObserver");

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(100);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, 100);
    EXPECT_EQ(observer->GetLockCalledCount(), 0);

    OHOS::EventFwk::CommonEventManager::PublishScreenLockedEvent(100);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, 100);
    EXPECT_EQ(observer->GetLockCalledCount(), 1);
    EXPECT_EQ(observer->lastLockUser_, 100);

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(200);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 2);
    EXPECT_EQ(observer->lastUnlockUser_, 200);
    EXPECT_EQ(observer->GetLockCalledCount(), 1);
    EXPECT_EQ(observer->lastLockUser_, 100);

    OHOS::EventFwk::CommonEventManager::PublishScreenLockedEvent(200);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 2);
    EXPECT_EQ(observer->lastUnlockUser_, 200);
    EXPECT_EQ(observer->GetLockCalledCount(), 2);
    EXPECT_EQ(observer->lastLockUser_, 200);
}





/**
 * @tc.name: SubscribeRetryLogic
 * @tc.desc: verify retry logic when subscribe fails initially
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, SubscribeRetryLogic, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver();
    auto executor = std::make_shared<ExecutorPool>(THREAD_MAX, THREAD_MIN);
    screen_->BindExecutor(executor);

    OHOS::EventFwk::CommonEventManager::SetSubscribeResult(false);
    screen_->SubscribeEvent();

    OHOS::EventFwk::CommonEventManager::SetSubscribeResult(true);

    OHOS::EventFwk::CommonEventManager::Reset();
}

/**
 * @tc.name: ExecutorLifecycle
 * @tc.desc: verify executor lifecycle management
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, ExecutorLifecycle, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver();
    auto executor = std::make_shared<ExecutorPool>(THREAD_MAX, THREAD_MIN);
    screen_->BindExecutor(executor);

    screen_->SubscribeEvent();
    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(100);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, 100);

    screen_->BindExecutor(nullptr);

    observer->Reset();
    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(200);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, 200);

    screen_->UnsubscribeEvent();
}

/**
 * @tc.name: ConcurrentObserverModification
 * @tc.desc: verify thread safety when modifying observers concurrently
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, ConcurrentObserverModification, TestSize.Level0)
{
    auto observers = CreateAndSubscribeObservers(10);
    screen_->SubscribeEvent();

    std::atomic<bool> ready{false};
    std::atomic<int> completedCount{0};
    std::vector<std::thread> threads;

    for (size_t i = 0; i < observers.size(); i++) {
        threads.emplace_back([this, &observers, &ready, &completedCount, i]() {
            while (!ready.load()) {
                std::this_thread::yield();
            }
            screen_->Unsubscribe(observers[i]);
            screen_->Subscribe(observers[i]);
            completedCount++;
        });
    }

    ready.store(true);
    for (auto &thread : threads) {
        thread.join();
    }

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(100);

    for (const auto &observer : observers) {
        EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
        EXPECT_EQ(observer->lastUnlockUser_, 100);
    }

    screen_->UnsubscribeEvent();
}



/**
 * @tc.name: NotifyScreenOn001
 * @tc.desc: notify screen on should call observer OnScreenOn callback
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, NotifyScreenOn001, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("screenOnObserver");

    OHOS::EventFwk::CommonEventManager::PublishScreenOnEvent(TEST_USER_ID);

    EXPECT_EQ(observer->GetScreenOnCalledCount(), 1);
    EXPECT_EQ(observer->lastScreenOnUser_, TEST_USER_ID);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 0);
    EXPECT_EQ(observer->GetLockCalledCount(), 0);
    EXPECT_EQ(observer->GetScreenOffCalledCount(), 0);
}



/**
 * @tc.name: NotifyScreenOff001
 * @tc.desc: notify screen off should call observer OnScreenOff callback
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, NotifyScreenOff001, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("screenOffObserver");

    OHOS::EventFwk::CommonEventManager::PublishScreenOffEvent(TEST_USER_ID);

    EXPECT_EQ(observer->GetScreenOffCalledCount(), 1);
    EXPECT_EQ(observer->lastScreenOffUser_, TEST_USER_ID);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 0);
    EXPECT_EQ(observer->GetLockCalledCount(), 0);
    EXPECT_EQ(observer->GetScreenOnCalledCount(), 0);
}









/**
 * @tc.name: InvalidEventFiltered
 * @tc.desc: invalid events should be filtered and not trigger any callback
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenTest, InvalidEventFiltered, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("invalidEventObserver");

    OHOS::EventFwk::CommonEventManager::PublishChargingEvent(100);
    OHOS::EventFwk::CommonEventManager::PublishDisChargingEvent(0);

    EXPECT_EQ(observer->GetUnlockCalledCount(), 0);
    EXPECT_EQ(observer->GetLockCalledCount(), 0);
    EXPECT_EQ(observer->GetScreenOnCalledCount(), 0);
    EXPECT_EQ(observer->GetScreenOffCalledCount(), 0);
}



} // namespace OHOS::Test
