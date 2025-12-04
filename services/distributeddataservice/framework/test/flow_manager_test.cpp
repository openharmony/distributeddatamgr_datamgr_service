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

#include "flow_manager/flow_manager.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class FlowManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: FlowManager_ExecuteWithDelayStrategy_Test
* @tc.desc: Test that tasks are executed with delay strategy applied
* @tc.type: FUNC
* @tc.step: 1. Create a mock strategy implementation that delays tasks after the first one
* @tc.step: 2. Initialize FlowManager with the mock strategy and an executor pool
* @tc.step: 3. Submit multiple tasks to the FlowManager
* @tc.step: 4. Verify that only the first task is executed immediately due to the delay strategy
* @tc.step: 5. Clean up resources by removing the FlowManager
* @tc.expected: Only one task should be executed within the first second due to the delay strategy
*/
HWTEST_F(FlowManagerTest, FlowManager_ExecuteWithDelayStrategy_Test, TestSize.Level1)
{
    class StrategyMockImpl : public FlowManager::Strategy {
    public:
        FlowManager::Tp GetExecuteTime(FlowManager::Task task, uint32_t type) override
        {
            if (!flag_.exchange(true)) {
                return std::chrono::steady_clock::now();
            }
            return std::chrono::steady_clock::now() + std::chrono::seconds(10);
        }

    private:
        std::atomic_bool flag_ = false;
    };
    auto pool = std::make_shared<ExecutorPool>(2, 3);
    FlowManager flowManager(pool, std::make_shared<StrategyMockImpl>());
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };
    for (int32_t i = 0; i < 10; i++) {
        flowManager.Execute(task);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(flag->load(), 1);
    flowManager.Remove();
}

/**
* @tc.name: FlowManager_ExecuteWithIntervalStrategy_Test
* @tc.desc: Test that tasks are executed with interval strategy applied
* @tc.type: FUNC
* @tc.step: 1. Create a mock strategy implementation that applies increasing intervals between task executions
* @tc.step: 2. Initialize FlowManager with the mock strategy and an executor pool
* @tc.step: 3. Submit multiple tasks to the FlowManager
* @tc.step: 4. Verify that tasks are executed according to the interval strategy over time
* @tc.step: 5. Clean up resources by removing the FlowManager
* @tc.expected: Tasks should be executed at increasing intervals as defined by the strategy
*/
HWTEST_F(FlowManagerTest, FlowManager_ExecuteWithIntervalStrategy_Test, TestSize.Level1)
{
    class StrategyMockImpl : public FlowManager::Strategy {
    public:
        FlowManager::Tp GetExecuteTime(FlowManager::Task task, uint32_t type) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto now = std::chrono::steady_clock::now();
            if (lastExecuteTime_ == FlowManager::Tp()) {
                lastExecuteTime_ = now;
                return lastExecuteTime_;
            }
            auto earliestExecutionTime = lastExecuteTime_ + std::chrono::milliseconds(500 * count_++);
            lastExecuteTime_ = earliestExecutionTime > now ? earliestExecutionTime : now;
            return lastExecuteTime_;
        }

    private:
        std::mutex mutex_;
        int32_t count_ = 1;
        FlowManager::Tp lastExecuteTime_ = FlowManager::Tp();
    };
    auto pool = std::make_shared<ExecutorPool>(2, 3);
    FlowManager flowManager(pool, std::make_shared<StrategyMockImpl>());
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };
    // [0, 500ms, 1500ms, 3000ms, 5000ms]
    for (int32_t i = 0; i < 5; i++) {
        flowManager.Execute(task);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(flag->load(), 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    EXPECT_EQ(flag->load(), 3);
    flowManager.Remove();
}

/**
* @tc.name: FlowManager_ExecuteWithoutStrategy_Test
* @tc.desc: Test that tasks are executed immediately when no strategy is provided
* @tc.type: FUNC
* @tc.step: 1. Initialize FlowManager with null strategy and an executor pool
* @tc.step: 2. Submit multiple tasks to the FlowManager
* @tc.step: 3. Verify that all tasks are executed immediately without any delay
* @tc.step: 4. Clean up resources by removing the FlowManager
* @tc.expected: All tasks should be executed immediately since no strategy is applied
* @tc.require:
*/
HWTEST_F(FlowManagerTest, FlowManager_ExecuteWithoutStrategy_Test, TestSize.Level1)
{
    auto pool = std::make_shared<ExecutorPool>(2, 3);
    FlowManager flowManager(pool, nullptr); // No strategy
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };

    const int taskCount = 5;
    for (int32_t i = 0; i < taskCount; i++) {
        flowManager.Execute(task);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(flag->load(), taskCount); // All tasks should be executed immediately
    flowManager.Remove();
}

/**
* @tc.name: FlowManager_PeakLimitStrategy_Test
* @tc.desc: Test peak limit strategy that restricts number of tasks executed per time unit
* @tc.type: FUNC
* @tc.step: 1. Create a peak limit strategy implementation that limits task execution rate
* @tc.step: 2. Initialize FlowManager with the peak limit strategy and an executor pool
* @tc.step: 3. Submit multiple tasks rapidly to test the rate limiting
* @tc.step: 4. Verify that only allowed number of tasks are executed within a time window
* @tc.step: 5. Check that excess tasks are postponed according to the strategy
* @tc.step: 6. Confirm all tasks eventually execute after sufficient time
* @tc.step: 7. Clean up resources by removing the FlowManager
* @tc.expected: Task execution should respect the defined rate limit and all tasks should eventually complete
* @tc.require:
*/
HWTEST_F(FlowManagerTest, FlowManager_PeakLimitStrategy_Test, TestSize.Level1)
{
    class PeakLimitStrategy : public FlowManager::Strategy {
    public:
        explicit PeakLimitStrategy(size_t maxTasksPerSecond = 2) : maxTasksPerSecond_(maxTasksPerSecond) {}

        FlowManager::Tp GetExecuteTime(FlowManager::Task task, uint32_t type) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto now = std::chrono::steady_clock::now();
            auto oneSecondLater = now - std::chrono::seconds(1);

            // Clear expired execution records (older than 1 second)
            while (!executionTimes_.empty() && executionTimes_.front() <= oneSecondLater) {
                executionTimes_.pop_front();
            }

            // If maximum tasks per second reached, postpone to next available slot
            if (executionTimes_.size() >= maxTasksPerSecond_) {
                // Find the earliest time we can schedule a new task
                auto earliestSlot = executionTimes_.front() + std::chrono::seconds(1);
                executionTimes_.push_back(earliestSlot);
                executionTimes_.pop_front();
                return earliestSlot;
            } else {
                executionTimes_.push_back(now);
                return now;
            }
        }

    private:
        std::mutex mutex_;
        std::deque<FlowManager::Tp> executionTimes_;
        size_t maxTasksPerSecond_;
    };

    auto pool = std::make_shared<ExecutorPool>(2, 3);
    FlowManager flowManager(pool, std::make_shared<PeakLimitStrategy>(2)); // Max 2 tasks per second
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };

    // Submit 5 tasks quickly
    for (int32_t i = 0; i < 5; i++) {
        flowManager.Execute(task, i);
    }

    // In first second, only 2 tasks should be executed
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(flag->load(), 2);

    // Shortly after 1 second, additional tasks may be executed based on sliding window
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    // With sliding window approach, we might have executed more than 2 but less than 5 tasks
    EXPECT_GE(flag->load(), 4);

    // After sufficient time, all tasks should be executed
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    EXPECT_EQ(flag->load(), 5);

    flowManager.Remove();
}

/**
* @tc.name: FlowManager_PriorityStrategy_Test
* @tc.desc: Test priority strategy for different types of tasks
* @tc.type: FUNC
* @tc.step: 1. Create a priority-based strategy implementation that schedules tasks based on their type
* @tc.step: 2. Initialize FlowManager with the priority strategy and an executor pool
* @tc.step: 3. Submit tasks with different priority levels (high, medium, low)
* @tc.step: 4. Verify that high priority tasks execute first, followed by medium and low priority ones
* @tc.step: 5. Clean up resources by removing the FlowManager
* @tc.expected: Tasks should execute in order of their priority level regardless of submission order
* @tc.require:
*/
HWTEST_F(FlowManagerTest, FlowManager_PriorityStrategy_Test, TestSize.Level1)
{
    class PriorityStrategy : public FlowManager::Strategy {
    public:
        FlowManager::Tp GetExecuteTime(FlowManager::Task task, uint32_t type) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto now = std::chrono::steady_clock::now();

            // Type 0: High priority, execute immediately
            // Type 1: Medium priority, delay 100ms to execute
            // Type 2: Low priority, delay 500ms to execute
            return now + std::chrono::milliseconds(type == 0 ? 0 : type == 1 ? 100 : 500);
        }

    private:
        std::mutex mutex_;
    };

    auto pool = std::make_shared<ExecutorPool>(2, 3);
    FlowManager flowManager(pool, std::make_shared<PriorityStrategy>());

    auto highPriorityFlag = std::make_shared<std::atomic_uint32_t>(0);
    auto mediumPriorityFlag = std::make_shared<std::atomic_uint32_t>(0);
    auto lowPriorityFlag = std::make_shared<std::atomic_uint32_t>(0);

    auto highPriorityTask = [highPriorityFlag]() mutable {
        (*highPriorityFlag)++;
    };
    auto mediumPriorityTask = [mediumPriorityFlag]() mutable {
        (*mediumPriorityFlag)++;
    };
    auto lowPriorityTask = [lowPriorityFlag]() mutable {
        (*lowPriorityFlag)++;
    };

    // Submit tasks with different priorities
    flowManager.Execute(lowPriorityTask, 2);    // Execute last
    flowManager.Execute(highPriorityTask, 0);   // Execute immediately
    flowManager.Execute(mediumPriorityTask, 1); // Execute with delay

    // Check immediately, only high priority task should be executed
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(highPriorityFlag->load(), 1);
    EXPECT_EQ(mediumPriorityFlag->load(), 0);
    EXPECT_EQ(lowPriorityFlag->load(), 0);

    // After 150ms, medium priority task should be executed
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_EQ(highPriorityFlag->load(), 1);
    EXPECT_EQ(mediumPriorityFlag->load(), 1);
    EXPECT_EQ(lowPriorityFlag->load(), 0);

    // After 500ms, low priority task should be executed
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(highPriorityFlag->load(), 1);
    EXPECT_EQ(mediumPriorityFlag->load(), 1);
    EXPECT_EQ(lowPriorityFlag->load(), 1);

    flowManager.Remove();
}

/**
* @tc.name: FlowManager_CancelScheduledTask_Test
* @tc.desc: Test cancellation of scheduled tasks during execution
* @tc.type: FUNC
* @tc.step: 1. Create a delay strategy that postpones all task executions
* @tc.step: 2. Initialize FlowManager with the delay strategy and an executor pool
* @tc.step: 3. Submit multiple tasks to the FlowManager
* @tc.step: 4. Call Remove() before the scheduled execution time
* @tc.step: 5. Verify that none of the previously scheduled tasks execute
* @tc.step: 6. Submit new tasks and confirm they execute normally
* @tc.step: 7. Clean up resources by removing the FlowManager
* @tc.expected: Scheduled tasks should be cancelled upon calling Remove() and new tasks should function normally
* @tc.require:
*/
HWTEST_F(FlowManagerTest, FlowManager_CancelScheduledTask_Test, TestSize.Level1)
{
    class DelayStrategy : public FlowManager::Strategy {
    public:
        FlowManager::Tp GetExecuteTime(FlowManager::Task task, uint32_t type) override
        {
            // All tasks delayed by 1 seconds
            return std::chrono::steady_clock::now() + std::chrono::seconds(1);
        }
    };

    auto pool = std::make_shared<ExecutorPool>(2, 3);
    FlowManager flowManager(pool, std::make_shared<DelayStrategy>());

    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };

    // Submit tasks
    for (int32_t i = 0; i < 5; i++) {
        flowManager.Execute(task);
    }

    // Wait for some time but not enough for tasks to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(flag->load(), 0); // Tasks not yet executed

    // Remove all tasks
    flowManager.Remove();

    // Even after waiting long enough, tasks should not execute
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(flag->load(), 0); // Tasks successfully cancelled

    // Submit new task to verify FlowManager is still functional
    flowManager.Execute(task);
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    EXPECT_EQ(flag->load(), 1); // New task executes normally

    flowManager.Remove();
}

/**
* @tc.name: FlowManager_ConcurrentTaskSubmission_Test
* @tc.desc: Test concurrent submission of large number of tasks
* @tc.type: FUNC
* @tc.step: 1. Create a concurrent strategy that spaces out task executions
* @tc.step: 2. Initialize FlowManager with the concurrent strategy and an enlarged executor pool
* @tc.step: 3. Launch multiple threads to submit tasks concurrently
* @tc.step: 4. Wait for all submission threads to complete
* @tc.step: 5. Verify that all submitted tasks eventually execute correctly
* @tc.step: 6. Clean up resources by removing the FlowManager
* @tc.expected: All concurrently submitted tasks should be processed and executed properly
* @tc.require:
*/
HWTEST_F(FlowManagerTest, FlowManager_ConcurrentTaskSubmission_Test, TestSize.Level1)
{
    class ConcurrentStrategy : public FlowManager::Strategy {
    public:
        FlowManager::Tp GetExecuteTime(FlowManager::Task task, uint32_t type) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto now = std::chrono::steady_clock::now();
            // Each task executed with 10ms interval to avoid instant concurrency
            auto executeTime = lastExecuteTime_ + std::chrono::milliseconds(10);
            lastExecuteTime_ = executeTime > now ? executeTime : now;
            return lastExecuteTime_;
        }

    private:
        std::mutex mutex_;
        FlowManager::Tp lastExecuteTime_;
    };

    auto pool = std::make_shared<ExecutorPool>(3, 5); // Enlarged thread pool
    FlowManager flowManager(pool, std::make_shared<ConcurrentStrategy>());

    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };

    const int taskCount = 100;
    std::vector<std::thread> threads;

    // Multi-threads concurrent task submission
    for (int t = 0; t < 10; t++) {
        threads.emplace_back([&]() {
            for (int i = 0; i < taskCount / 10; i++) {
                flowManager.Execute(task);
            }
        });
    }

    // Wait for all submission threads to complete
    for (auto &thread : threads) {
        thread.join();
    }

    // Wait for all tasks to complete execution
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    EXPECT_EQ(flag->load(), taskCount);

    flowManager.Remove();
}
/**
* @tc.name: FlowManager_PriorityPreemption_Test
* @tc.desc: Test priority preemption where high priority tasks execute before low priority despite submission order
* @tc.type: FUNC
* @tc.step: 1. Create a priority strategy that delays low priority tasks but not high priority ones
* @tc.step: 2. Initialize FlowManager with the priority strategy and an executor pool
* @tc.step: 3. Submit a low priority task first, then a high priority task shortly after
* @tc.step: 4. Verify that the high priority task executes before the low priority task
* @tc.step: 5. Clean up resources by removing the FlowManager
* @tc.expected: High priority task should execute first even though it was submitted after the low priority task
*/
HWTEST_F(FlowManagerTest, FlowManager_PriorityPreemption_Test, TestSize.Level1)
{
    class PriorityPreemptionStrategy : public FlowManager::Strategy {
    public:
        FlowManager::Tp GetExecuteTime(FlowManager::Task task, uint32_t type) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto now = std::chrono::steady_clock::now();

            // Higher type value means lower priority, thus longer delay
            return now + std::chrono::milliseconds(500 * type);
        }

    private:
        std::mutex mutex_;
    };

    auto pool = std::make_shared<ExecutorPool>(2, 3);
    FlowManager flowManager(pool, std::make_shared<PriorityPreemptionStrategy>());

    auto highPriorityFlag = std::make_shared<std::atomic_uint32_t>(0);
    auto lowPriorityFlag = std::make_shared<std::atomic_uint32_t>(0);

    auto highPriorityTask = [highPriorityFlag]() mutable {
        (*highPriorityFlag)++;
    };
    auto lowPriorityTask = [lowPriorityFlag]() mutable {
        (*lowPriorityFlag)++;
    };

    // Submit low priority task first (type 3, will be delayed 1500ms)
    flowManager.Execute(lowPriorityTask, 3);

    // Submit high priority task after short delay but before low priority task would execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    flowManager.Execute(highPriorityTask, 1);

    // Check immediately - no tasks should have executed yet
    EXPECT_EQ(highPriorityFlag->load(), 0);
    EXPECT_EQ(lowPriorityFlag->load(), 0);

    // After 500ms, only high priority task should have executed (low priority still has 1000ms remaining)
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(highPriorityFlag->load(), 1);
    EXPECT_EQ(lowPriorityFlag->load(), 0);

    // After another 1000ms, low priority task should also have executed
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(highPriorityFlag->load(), 1);
    EXPECT_EQ(lowPriorityFlag->load(), 1);

    flowManager.Remove();
}

class PriorityCancellationStrategy : public FlowManager::Strategy {
public:
    FlowManager::Tp GetExecuteTime(FlowManager::Task task, uint32_t type) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();

        // Type 0: High priority, execute with 100ms delay
        // Type 1: Medium priority, execute with 500ms delay
        // Type 2: Low priority, execute with 300ms delay
        std::chrono::milliseconds delay(0);
        switch (type) {
            case 0:
                delay = std::chrono::milliseconds(100);
                break;
            case 1:
                delay = std::chrono::milliseconds(500);
                break;
            case 2:
                delay = std::chrono::milliseconds(300);
                break;
            default:
                delay = std::chrono::milliseconds(1000);
        }
        return now + delay;
    }

private:
    std::mutex mutex_;
};
/**
* @tc.name: FlowManager_CancelSpecificPriorityTask_Test
* @tc.desc: Test cancellation of specific priority tasks does not affect other priority tasks execution
* @tc.type: FUNC
* @tc.step: 1. Create a strategy that assigns different delays based on task priority
* @tc.step: 2. Initialize FlowManager with the strategy and an executor pool
* @tc.step: 3. Submit tasks with different priorities (priority 0, 1, and 2)
* @tc.step: 4. Cancel tasks with priority 1 before they execute
* @tc.step: 5. Verify that priority 0 and 2 tasks still execute normally
* @tc.step: 6. Confirm that priority 1 tasks are not executed after cancellation
* @tc.step: 7. Clean up resources by removing the FlowManager
* @tc.expected: Priority 1 tasks should be cancelled and not executed, while priority 0 and 2 tasks should execute normally
*/
HWTEST_F(FlowManagerTest, FlowManager_CancelSpecificPriorityTask_Test, TestSize.Level1)
{
    auto pool = std::make_shared<ExecutorPool>(2, 3);
    FlowManager flowManager(pool, std::make_shared<PriorityCancellationStrategy>());

    auto priority0Flag = std::make_shared<std::atomic_uint32_t>(0);
    auto priority1Flag = std::make_shared<std::atomic_uint32_t>(0);
    auto priority2Flag = std::make_shared<std::atomic_uint32_t>(0);

    auto priority0Task = [priority0Flag]() mutable {
        (*priority0Flag)++;
    };
    auto priority1Task = [priority1Flag]() mutable {
        (*priority1Flag)++;
    };
    auto priority2Task = [priority2Flag]() mutable {
        (*priority2Flag)++;
    };

    // Submit tasks with different priorities
    flowManager.Execute(priority0Task, 0);  // Should execute after 100ms
    flowManager.Execute(priority1Task, 1);  // Should execute after 500ms - will be cancelled
    flowManager.Execute(priority2Task, 2);  // Should execute after 300ms

    // Cancel priority 1 tasks before they execute
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    flowManager.Remove(1);  // Remove all tasks with priority 1

    // After 100ms, priority 0 task should execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(priority0Flag->load(), 1);
    EXPECT_EQ(priority1Flag->load(), 0);
    EXPECT_EQ(priority2Flag->load(), 0);

    // After 300ms total, priority 2 task should execute
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(priority0Flag->load(), 1);
    EXPECT_EQ(priority1Flag->load(), 0);
    EXPECT_EQ(priority2Flag->load(), 1);

    // After 500ms total, priority 1 task should still not execute because it was cancelled
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(priority0Flag->load(), 1);
    EXPECT_EQ(priority1Flag->load(), 0);  // Still 0 because it was cancelled
    EXPECT_EQ(priority2Flag->load(), 1);

    // Submit a new priority 1 task to verify that the flow manager is still functional for that priority
    flowManager.Execute(priority1Task, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(550));  // Wait for it to execute
    EXPECT_EQ(priority0Flag->load(), 1);
    EXPECT_EQ(priority1Flag->load(), 1);  // This should now be 1 as it's a new task
    EXPECT_EQ(priority2Flag->load(), 1);

    flowManager.Remove();
}
} // namespace OHOS::Test