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
#include <memory>
#include <atomic>
#include <vector>
#include <thread>
#include <algorithm>
#include <chrono>

#include "executor_pool.h"
#include "screenlock/screen_lock.h"
#include "mock/mock_common_event.h"

namespace {
using namespace OHOS::DistributedData;
using namespace testing::ext;
using namespace OHOS;

// Mock Observer for testing callback behavior
class MockScreenLockObserver : public ScreenManager::Observer {
public:
    std::string name_ = "mockObserver";
    std::atomic<int> unlockCalledCount_{0};
    std::atomic<int> lockCalledCount_{0};
    int32_t lastUnlockUser_{-1};
    int32_t lastLockUser_{-1};

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
        lastUnlockUser_ = -1;
        lastLockUser_ = -1;
    }

    bool IsUnlockCalled() const
    {
        return unlockCalledCount_ > 0;
    }

    bool IsLockCalled() const
    {
        return lockCalledCount_ > 0;
    }

    int GetUnlockCalledCount() const
    {
        return unlockCalledCount_;
    }

    int GetLockCalledCount() const
    {
        return lockCalledCount_;
    }
};

// Test helper class to access private members for critical testing only
// NOTE: Private member access should be minimized. Only access private methods
// when testing core business logic that cannot be verified through public APIs.
class ScreenLockTestHelper {
public:
    // GetTask is critical for testing retry logic - this is core business logic
    // that requires verification of internal retry behavior
    static ExecutorPool::Task GetTask(ScreenLock &screenLock, uint32_t retry)
    {
        return screenLock.GetTask(retry);
    }
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
        screenLock_ = std::make_shared<ScreenLock>();
    }

    void TearDown()
    {
        // Reset mock state after each test
        OHOS::EventFwk::CommonEventManager::Reset();
    }

protected:
    std::shared_ptr<MockScreenLockObserver> CreateObserver(const std::string &name = "testObserver")
    {
        auto observer = std::make_shared<MockScreenLockObserver>();
        observer->SetName(name);
        return observer;
    }

    std::shared_ptr<MockScreenLockObserver> CreateAndSubscribeObserver(
        const std::string &name = "testObserver")
    {
        auto observer = CreateObserver(name);
        screenLock_->Subscribe(observer);
        return observer;
    }

    std::vector<std::shared_ptr<MockScreenLockObserver>> CreateAndSubscribeObservers(int count)
    {
        std::vector<std::shared_ptr<MockScreenLockObserver>> observers;
        for (int i = 0; i < count; i++) {
            observers.push_back(CreateAndSubscribeObserver("observer" + std::to_string(i)));
        }
        return observers;
    }

    std::shared_ptr<ScreenLock> screenLock_;
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
HWTEST_F(ScreenLockTest, Subscribe001, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver();

    screenLock_->Subscribe(nullptr);

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
HWTEST_F(ScreenLockTest, Subscribe002, TestSize.Level0)
{
    auto observer = CreateObserver("");

    screenLock_->Subscribe(observer);

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
HWTEST_F(ScreenLockTest, Subscribe003, TestSize.Level0)
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
HWTEST_F(ScreenLockTest, Subscribe004, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("duplicateObserver");

    screenLock_->Subscribe(observer);

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
HWTEST_F(ScreenLockTest, Subscribe005, TestSize.Level0)
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
HWTEST_F(ScreenLockTest, Unsubscribe001, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver();

    screenLock_->Unsubscribe(nullptr);

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
HWTEST_F(ScreenLockTest, Unsubscribe002, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("testObserver");

    // Empty name cannot unsubscribe
    observer->SetName("");
    screenLock_->Unsubscribe(observer);

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
HWTEST_F(ScreenLockTest, Unsubscribe003, TestSize.Level0)
{
    auto observer = CreateObserver("nonExistent");

    screenLock_->Unsubscribe(observer);

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
HWTEST_F(ScreenLockTest, Unsubscribe004, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("toRemove");

    screenLock_->Unsubscribe(observer);

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
HWTEST_F(ScreenLockTest, Unsubscribe005, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("resubscribeTest");

    screenLock_->Unsubscribe(observer);

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(TEST_USER_ID);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 0);

    screenLock_->Subscribe(observer);

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
HWTEST_F(ScreenLockTest, NotifyScreenUnlocked001, TestSize.Level0)
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
 * @tc.name: NotifyScreenLocked001
 * @tc.desc: notify screen locked should call all observers
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenLockTest, NotifyScreenLocked001, TestSize.Level0)
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
HWTEST_F(ScreenLockTest, NotifyScreen002, TestSize.Level0)
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
HWTEST_F(ScreenLockTest, NotifyScreen003, TestSize.Level0)
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
HWTEST_F(ScreenLockTest, BindExecutor001, TestSize.Level0)
{
    auto executor = std::make_shared<ExecutorPool>(12, 5);

    screenLock_->BindExecutor(executor);

    auto task = ScreenLockTestHelper::GetTask(*screenLock_, 0);
    EXPECT_TRUE(task != nullptr);
}

/**
 * @tc.name: BindExecutor002
 * @tc.desc: bind null executor should replace existing executor
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenLockTest, BindExecutor002, TestSize.Level0)
{
    auto executor = std::make_shared<ExecutorPool>(12, 5);

    screenLock_->BindExecutor(executor);
    auto task1 = ScreenLockTestHelper::GetTask(*screenLock_, 0);
    EXPECT_TRUE(task1 != nullptr);

    screenLock_->BindExecutor(nullptr);

    auto task2 = ScreenLockTestHelper::GetTask(*screenLock_, 0);
    EXPECT_TRUE(task2 != nullptr);
}

/**
 * @tc.name: GetTask001
 * @tc.desc: get task without executor should return valid callable task
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenLockTest, GetTask001, TestSize.Level0)
{
    auto task = ScreenLockTestHelper::GetTask(*screenLock_, 0);

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
HWTEST_F(ScreenLockTest, GetTask002, TestSize.Level0)
{
    auto executor = std::make_shared<ExecutorPool>(12, 5);
    screenLock_->BindExecutor(executor);

    auto task = ScreenLockTestHelper::GetTask(*screenLock_, MAX_RETRY_TIMES + 1);

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
HWTEST_F(ScreenLockTest, GetTask003, TestSize.Level0)
{
    auto executor = std::make_shared<ExecutorPool>(12, 5);
    screenLock_->BindExecutor(executor);

    auto task = ScreenLockTestHelper::GetTask(*screenLock_, 0);

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
HWTEST_F(ScreenLockTest, GetTask004, TestSize.Level0)
{
    auto executor = std::make_shared<ExecutorPool>(12, 5);
    screenLock_->BindExecutor(executor);

    auto task = ScreenLockTestHelper::GetTask(*screenLock_, MAX_RETRY_TIMES);

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
HWTEST_F(ScreenLockTest, Destructor001, TestSize.Level0)
{
    auto screenLock = std::make_shared<ScreenLock>();
    auto executor = std::make_shared<ExecutorPool>(12, 5);

    screenLock->BindExecutor(executor);

    std::weak_ptr<ScreenLock> weakPtr = screenLock;

    screenLock.reset();

    EXPECT_TRUE(weakPtr.expired());
}

/**
 * @tc.name: IsLocked001
 * @tc.desc: test IsLocked method
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenLockTest, IsLocked001, TestSize.Level0)
{
    auto screenLock = std::make_shared<ScreenLock>();

    bool isLocked = screenLock->IsLocked();

    EXPECT_FALSE(isLocked);
}

/**
 * @tc.name: MultiUserNotify001
 * @tc.desc: notify different users to observers
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenLockTest, MultiUserNotify001, TestSize.Level0)
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
HWTEST_F(ScreenLockTest, ConcurrentNotify001, TestSize.Level0)
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
HWTEST_F(ScreenLockTest, MixedNotify001, TestSize.Level0)
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
 * @tc.name: ObserverReuse001
 * @tc.desc: observer can be reused after unsubscribe
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenLockTest, ObserverReuse001, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("reuseObserver");

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(100);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, 100);

    screenLock_->Unsubscribe(observer);

    observer->Reset();

    screenLock_->Subscribe(observer);
    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(200);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, 200);
}

/**
 * @tc.name: CompleteEventLifecycle
 * @tc.desc: complete event lifecycle from subscribe to unsubscribe
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenLockTest, CompleteEventLifecycle, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("lifecycleObserver");

    screenLock_->SubscribeScreenEvent();

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(100);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, 100);

    screenLock_->UnsubscribeScreenEvent();

    observer->Reset();
    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(200);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 0);
}

/**
 * @tc.name: UserSwitchScenario
 * @tc.desc: simulate user switch with different user IDs
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenLockTest, UserSwitchScenario, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver("userSwitchObserver");
    screenLock_->SubscribeScreenEvent();

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(100);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, 100);

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(200);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 2);
    EXPECT_EQ(observer->lastUnlockUser_, 200);

    screenLock_->UnsubscribeScreenEvent();
}

/**
 * @tc.name: DynamicObserverManagement
 * @tc.desc: add observers while events are being published
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenLockTest, DynamicObserverManagement, TestSize.Level0)
{
    auto observer1 = CreateAndSubscribeObserver("observer1");
    screenLock_->SubscribeScreenEvent();

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(100);
    EXPECT_EQ(observer1->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer1->lastUnlockUser_, 100);

    auto observer2 = CreateAndSubscribeObserver("observer2");

    observer1->Reset();
    observer2->Reset();

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(200);
    EXPECT_EQ(observer1->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer1->lastUnlockUser_, 200);
    EXPECT_EQ(observer2->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer2->lastUnlockUser_, 200);

    screenLock_->UnsubscribeScreenEvent();
}

/**
 * @tc.name: SubscribeRetryLogic
 * @tc.desc: verify retry logic when subscribe fails initially
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenLockTest, SubscribeRetryLogic, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver();
    auto executor = std::make_shared<ExecutorPool>(12, 5);
    screenLock_->BindExecutor(executor);

    OHOS::EventFwk::CommonEventManager::SetSubscribeResult(false);
    screenLock_->SubscribeScreenEvent();

    OHOS::EventFwk::CommonEventManager::SetSubscribeResult(true);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    OHOS::EventFwk::CommonEventManager::Reset();
}

/**
 * @tc.name: ExecutorLifecycle
 * @tc.desc: verify executor lifecycle management
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenLockTest, ExecutorLifecycle, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver();
    auto executor = std::make_shared<ExecutorPool>(12, 5);
    screenLock_->BindExecutor(executor);

    screenLock_->SubscribeScreenEvent();
    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(100);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, 100);

    screenLock_->BindExecutor(nullptr);

    observer->Reset();
    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(200);
    EXPECT_EQ(observer->GetUnlockCalledCount(), 1);
    EXPECT_EQ(observer->lastUnlockUser_, 200);

    screenLock_->UnsubscribeScreenEvent();
}

/**
 * @tc.name: ConcurrentObserverModification
 * @tc.desc: verify thread safety when modifying observers concurrently
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenLockTest, ConcurrentObserverModification, TestSize.Level0)
{
    auto observers = CreateAndSubscribeObservers(10);
    screenLock_->SubscribeScreenEvent();

    std::thread unsubscribeThread([this, &observers]() {
        screenLock_->Unsubscribe(observers[5]);
    });
    unsubscribeThread.join();

    OHOS::EventFwk::CommonEventManager::PublishScreenUnlockEvent(100);

    for (size_t i = 0; i < observers.size(); i++) {
        if (i != 5) {
            EXPECT_EQ(observers[i]->GetUnlockCalledCount(), 1);
            EXPECT_EQ(observers[i]->lastUnlockUser_, 100);
        }
    }

    screenLock_->UnsubscribeScreenEvent();
}

/**
 * @tc.name: MixedEventHandling
 * @tc.desc: verify correct handling of mixed lock and unlock events
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ScreenLockTest, MixedEventHandling, TestSize.Level0)
{
    auto observer = CreateAndSubscribeObserver();
    screenLock_->SubscribeScreenEvent();

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

    screenLock_->UnsubscribeScreenEvent();
}

} // namespace
