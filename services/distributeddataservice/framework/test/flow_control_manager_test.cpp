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

#include "flow_control_manager/flow_control_manager.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class FlowControlManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class ExecuteWithDelayStrategyImpl : public FlowControlManager::Strategy {
public:
    FlowControlManager::Tp GetExecuteTime(FlowControlManager::Task task,
        const FlowControlManager::TaskInfo &info) override
    {
        if (!flag_.exchange(true)) {
            return std::chrono::steady_clock::now();
        }
        // Delay subsequent tasks by 10 seconds
        return std::chrono::steady_clock::now() + std::chrono::seconds(10);
    }

private:
    std::atomic_bool flag_ = false;
};
/**
* @tc.name: FlowControlManager_ExecuteWithDelayStrategy_Test
* @tc.desc: Test that tasks are executed with delay strategy applied
* @tc.type: FUNC
* @tc.step: 1. Create a mock strategy implementation that delays tasks after the first one
* @tc.step: 2. Initialize FlowControlManager with the mock strategy and an executor pool
* @tc.step: 3. Submit multiple tasks to the FlowControlManager
* @tc.step: 4. Verify that only the first task is executed immediately due to the delay strategy
* @tc.step: 5. Clean up resources by removing the FlowControlManager
* @tc.expected: Only one task should be executed within the first second due to the delay strategy
*/
HWTEST_F(FlowControlManagerTest, FlowControlManager_ExecuteWithDelayStrategy_Test, TestSize.Level1)
{
    // Create executor pool with 2 initial threads and 3 max threads
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    FlowControlManager flowControlManager(pool, std::make_shared<ExecuteWithDelayStrategyImpl>());
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };
    // Submit 10 tasks
    for (int32_t i = 0; i < 10; i++) {
        flowControlManager.Execute(task);
    }
    // Wait for 1 second - only first task should execute
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(flag->load(), 1);
    flowControlManager.Remove();
}

class ExecuteWithIntervalStrategyImpl : public FlowControlManager::Strategy {
public:
    FlowControlManager::Tp GetExecuteTime(FlowControlManager::Task task,
        const FlowControlManager::TaskInfo &info) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        if (lastExecuteTime_ == FlowControlManager::Tp()) {
            lastExecuteTime_ = now;
            return lastExecuteTime_;
        }
        // Increase interval by 500ms for each task
        auto earliestExecutionTime = lastExecuteTime_ + std::chrono::milliseconds(500 * count_++);
        lastExecuteTime_ = earliestExecutionTime > now ? earliestExecutionTime : now;
        return lastExecuteTime_;
    }

private:
    std::mutex mutex_;
    int32_t count_ = 1;
    FlowControlManager::Tp lastExecuteTime_ = FlowControlManager::Tp();
};
/**
* @tc.name: FlowControlManager_ExecuteWithIntervalStrategy_Test
* @tc.desc: Test that tasks are executed with interval strategy applied
* @tc.type: FUNC
* @tc.step: 1. Create a mock strategy implementation that applies increasing intervals between task executions
* @tc.step: 2. Initialize FlowControlManager with the mock strategy and an executor pool
* @tc.step: 3. Submit multiple tasks to the FlowControlManager
* @tc.step: 4. Verify that tasks are executed according to the interval strategy over time
* @tc.step: 5. Clean up resources by removing the FlowControlManager
* @tc.expected: Tasks should be executed at increasing intervals as defined by the strategy
*/
HWTEST_F(FlowControlManagerTest, FlowControlManager_ExecuteWithIntervalStrategy_Test, TestSize.Level1)
{
    // Create executor pool with 2 initial threads and 3 max threads
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    FlowControlManager flowControlManager(pool, std::make_shared<ExecuteWithIntervalStrategyImpl>());
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };
    // Execution times: [0ms, 500ms, 1500ms, 3000ms, 5000ms]
    for (int32_t i = 0; i < 5; i++) {
        flowControlManager.Execute(task);
    }
    // After 1 second, 2 tasks should have executed (0ms and 500ms)
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(flag->load(), 2);
    // After additional 1.5 seconds (total 2.5s), 3 tasks should have executed (0ms, 500ms, 1500ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    EXPECT_EQ(flag->load(), 3);
    flowControlManager.Remove();
}

/**
* @tc.name: FlowControlManager_ExecuteWithoutStrategy_Test
* @tc.desc: Test that tasks are executed immediately when no strategy is provided
* @tc.type: FUNC
* @tc.step: 1. Initialize FlowControlManager with null strategy and an executor pool
* @tc.step: 2. Submit multiple tasks to the FlowControlManager
* @tc.step: 3. Verify that all tasks are executed immediately without any delay
* @tc.step: 4. Clean up resources by removing the FlowControlManager
* @tc.expected: All tasks should be executed immediately since no strategy is applied
* @tc.require:
*/
HWTEST_F(FlowControlManagerTest, FlowControlManager_ExecuteWithoutStrategy_Test, TestSize.Level1)
{
    // Create executor pool with 2 initial threads and 3 max threads
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    FlowControlManager flowControlManager(pool, nullptr); // No strategy
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };

    const int taskCount = 5;
    for (int32_t i = 0; i < taskCount; i++) {
        flowControlManager.Execute(task);
    }

    // Wait briefly for all tasks to execute immediately
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(flag->load(), taskCount); // All tasks should be executed immediately
    flowControlManager.Remove();
}

class PeakLimitStrategy : public FlowControlManager::Strategy {
public:
    // Constructor with default limit of 2 tasks per second
    explicit PeakLimitStrategy(size_t maxTasksPerSecond = 2) : maxTasksPerSecond_(maxTasksPerSecond) {}

    FlowControlManager::Tp GetExecuteTime(FlowControlManager::Task task,
        const FlowControlManager::TaskInfo &info) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        // Calculate time point one second ago
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
    std::deque<FlowControlManager::Tp> executionTimes_;
    size_t maxTasksPerSecond_; // Maximum tasks allowed per second
};
/**
* @tc.name: FlowControlManager_PeakLimitStrategy_Test
* @tc.desc: Test peak limit strategy that restricts number of tasks executed per time unit
* @tc.type: FUNC
* @tc.step: 1. Create a peak limit strategy implementation that limits task execution rate
* @tc.step: 2. Initialize FlowControlManager with the peak limit strategy and an executor pool
* @tc.step: 3. Submit multiple tasks rapidly to test the rate limiting
* @tc.step: 4. Verify that only allowed number of tasks are executed within a time window
* @tc.step: 5. Check that excess tasks are postponed according to the strategy
* @tc.step: 6. Confirm all tasks eventually execute after sufficient time
* @tc.step: 7. Clean up resources by removing the FlowControlManager
* @tc.expected: Task execution should respect the defined rate limit and all tasks should eventually complete
* @tc.require:
*/
HWTEST_F(FlowControlManagerTest, FlowControlManager_PeakLimitStrategy_Test, TestSize.Level1)
{
    // Create executor pool with 2 initial threads and 3 max threads
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    // Initialize with peak limit strategy allowing max 2 tasks per second
    FlowControlManager flowControlManager(pool, std::make_shared<PeakLimitStrategy>(2));
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };

    // Submit 5 tasks quickly
    for (int32_t i = 0; i < 5; i++) {
        flowControlManager.Execute(task, i);
    }

    // In first half second, only 2 tasks should be executed
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(flag->load(), 2);

    // Shortly after 1.1 seconds, additional tasks may be executed based on sliding window
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    // With sliding window approach, we might have executed more than 2 but less than 5 tasks
    EXPECT_GE(flag->load(), 4);

    // After sufficient time (about 3 seconds total), all tasks should be executed
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    EXPECT_EQ(flag->load(), 5);

    flowControlManager.Remove();
}

class PriorityStrategy : public FlowControlManager::Strategy {
public:
    FlowControlManager::Tp GetExecuteTime(FlowControlManager::Task task,
        const FlowControlManager::TaskInfo &info) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();

        // Type 0: High priority, execute immediately
        // Type 1: Medium priority, delay 100ms to execute
        // Type 2: Low priority, delay 500ms to execute
        // All Medium tasks delayed by 100ms, all Low tasks delayed by 500ms
        return now + std::chrono::milliseconds(info.type == 0 ? 0 : info.type == 1 ? 100 : 500);
    }

private:
    std::mutex mutex_;
};
/**
* @tc.name: FlowControlManager_PriorityStrategy_Test
* @tc.desc: Test priority strategy for different types of tasks
* @tc.type: FUNC
* @tc.step: 1. Create a priority-based strategy implementation that schedules tasks based on their type
* @tc.step: 2. Initialize FlowControlManager with the priority strategy and an executor pool
* @tc.step: 3. Submit tasks with different priority levels (high, medium, low)
* @tc.step: 4. Verify that high priority tasks execute first, followed by medium and low priority ones
* @tc.step: 5. Clean up resources by removing the FlowControlManager
* @tc.expected: Tasks should execute in order of their priority level regardless of submission order
* @tc.require:
*/
HWTEST_F(FlowControlManagerTest, FlowControlManager_PriorityStrategy_Test, TestSize.Level1)
{
    // Create executor pool with 2 initial threads and 3 max threads
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    FlowControlManager flowControlManager(pool, std::make_shared<PriorityStrategy>());

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
    flowControlManager.Execute(lowPriorityTask, 2);    // Execute last (500ms delay)
    flowControlManager.Execute(highPriorityTask, 0);   // Execute immediately
    flowControlManager.Execute(mediumPriorityTask, 1); // Execute with delay (100ms delay)

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

    flowControlManager.Remove();
}

/**
* @tc.name: FlowControlManager_CancelScheduledTask_Test
* @tc.desc: Test cancellation of scheduled tasks during execution
* @tc.type: FUNC
* @tc.step: 1. Create a delay strategy that postpones all task executions
* @tc.step: 2. Initialize FlowControlManager with the delay strategy and an executor pool
* @tc.step: 3. Submit multiple tasks to the FlowControlManager
* @tc.step: 4. Call Remove() before the scheduled execution time
* @tc.step: 5. Verify that none of the previously scheduled tasks execute
* @tc.step: 6. Submit new tasks and confirm they execute normally
* @tc.step: 7. Clean up resources by removing the FlowControlManager
* @tc.expected: Scheduled tasks should be cancelled upon calling Remove() and new tasks should function normally
* @tc.require:
*/
HWTEST_F(FlowControlManagerTest, FlowControlManager_CancelScheduledTask_Test, TestSize.Level1)
{
    class DelayStrategy : public FlowControlManager::Strategy {
    public:
        FlowControlManager::Tp GetExecuteTime(FlowControlManager::Task task,
            const FlowControlManager::TaskInfo &info) override
        {
            // All tasks delayed by 1 second
            return std::chrono::steady_clock::now() + std::chrono::seconds(1);
        }
    };

    // Create executor pool with 2 initial threads and 3 max threads
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    FlowControlManager flowControlManager(pool, std::make_shared<DelayStrategy>());

    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };

    // Submit 5 tasks
    for (int32_t i = 0; i < 5; i++) {
        flowControlManager.Execute(task);
    }

    // Wait for some time (500ms) but not enough for tasks to execute (1000ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(flag->load(), 0); // Tasks not yet executed

    // Remove all tasks
    flowControlManager.Remove();

    // Even after waiting long enough (1000ms), tasks should not execute
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(flag->load(), 0); // Tasks successfully cancelled

    // Submit new task to verify FlowControlManager is still functional
    flowControlManager.Execute(task);
    // Wait for new task to execute (1100ms > 1000ms delay)
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    EXPECT_EQ(flag->load(), 1); // New task executes normally

    flowControlManager.Remove();
}

/**
* @tc.name: FlowControlManager_ConcurrentTaskSubmission_Test
* @tc.desc: Test concurrent submission of large number of tasks
* @tc.type: FUNC
* @tc.step: 1. Create a concurrent strategy that spaces out task executions
* @tc.step: 2. Initialize FlowControlManager with the concurrent strategy and an enlarged executor pool
* @tc.step: 3. Launch multiple threads to submit tasks concurrently
* @tc.step: 4. Wait for all submission threads to complete
* @tc.step: 5. Verify that all submitted tasks eventually execute correctly
* @tc.step: 6. Clean up resources by removing the FlowControlManager
* @tc.expected: All concurrently submitted tasks should be processed and executed properly
* @tc.require:
*/
HWTEST_F(FlowControlManagerTest, FlowControlManager_ConcurrentTaskSubmission_Test, TestSize.Level1)
{
    class ConcurrentStrategy : public FlowControlManager::Strategy {
    public:
        FlowControlManager::Tp GetExecuteTime(FlowControlManager::Task task,
            const FlowControlManager::TaskInfo &info) override
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
        FlowControlManager::Tp lastExecuteTime_;
    };

    // Create enlarged executor pool with 3 initial threads and 5 max threads
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    FlowControlManager flowControlManager(pool, std::make_shared<ConcurrentStrategy>());

    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };

    const int taskCount = 100;
    std::vector<std::thread> threads;

    // Multi-threads concurrent task submission using 10 threads
    for (int t = 0; t < 10; t++) {
        threads.emplace_back([&]() {
            // Each thread submits 10 tasks (100 total / 10 threads)
            for (int i = 0; i < taskCount / 10; i++) {
                flowControlManager.Execute(task);
            }
        });
    }

    // Wait for all submission threads to complete
    for (auto &thread : threads) {
        thread.join();
    }

    // Wait for all tasks to complete execution (1500ms > 100 tasks * 10ms = 1000ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    EXPECT_EQ(flag->load(), taskCount);

    flowControlManager.Remove();
}

class PriorityPreemptionStrategy : public FlowControlManager::Strategy {
public:
    FlowControlManager::Tp GetExecuteTime(FlowControlManager::Task task,
        const FlowControlManager::TaskInfo &info) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();

        // Higher type value means lower priority, thus longer delay
        // Delay is 500ms multiplied by task type value
        return now + std::chrono::milliseconds(500 * info.type);
    }

private:
    std::mutex mutex_;
};
/**
* @tc.name: FlowControlManager_PriorityPreemption_Test
* @tc.desc: Test priority preemption where high priority tasks execute before low priority despite submission order
* @tc.type: FUNC
* @tc.step: 1. Create a priority strategy that delays low priority tasks but not high priority ones
* @tc.step: 2. Initialize FlowControlManager with the priority strategy and an executor pool
* @tc.step: 3. Submit a low priority task first, then a high priority task shortly after
* @tc.step: 4. Verify that the high priority task executes before the low priority task
* @tc.step: 5. Clean up resources by removing the FlowControlManager
* @tc.expected: High priority task should execute first even though it was submitted after the low priority task
*/
HWTEST_F(FlowControlManagerTest, FlowControlManager_PriorityPreemption_Test, TestSize.Level1)
{
    // Create executor pool with 2 initial threads and 3 max threads
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    FlowControlManager flowControlManager(pool, std::make_shared<PriorityPreemptionStrategy>());

    auto highPriorityFlag = std::make_shared<std::atomic_uint32_t>(0);
    auto lowPriorityFlag = std::make_shared<std::atomic_uint32_t>(0);

    auto highPriorityTask = [highPriorityFlag]() mutable {
        (*highPriorityFlag)++;
    };
    auto lowPriorityTask = [lowPriorityFlag]() mutable {
        (*lowPriorityFlag)++;
    };

    // Submit low priority task first (type 3, will be delayed 1500ms)
    flowControlManager.Execute(lowPriorityTask, 3);

    // Submit high priority task after short delay (100ms) but before low priority task would execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    flowControlManager.Execute(highPriorityTask, 1);

    // Check immediately - no tasks should have executed yet
    EXPECT_EQ(highPriorityFlag->load(), 0);
    EXPECT_EQ(lowPriorityFlag->load(), 0);

    // After 1000ms, only high priority task should have executed
    // (low priority still has 500ms remaining from its 1500ms total delay)
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(highPriorityFlag->load(), 1);
    EXPECT_EQ(lowPriorityFlag->load(), 0);

    // After another 1000ms (total 2100ms), low priority task should also have executed
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(highPriorityFlag->load(), 1);
    EXPECT_EQ(lowPriorityFlag->load(), 1);

    flowControlManager.Remove();
}

class PriorityCancellationStrategy : public FlowControlManager::Strategy {
public:
    FlowControlManager::Tp GetExecuteTime(FlowControlManager::Task task,
        const FlowControlManager::TaskInfo &info) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();

        std::chrono::milliseconds delay(0);
        switch (info.type) {
            case 0: // Type 0: High priority, execute with 100ms delay
                delay = std::chrono::milliseconds(100);
                break;
            case 1: // Type 1: Medium priority, execute with 500ms delay
                delay = std::chrono::milliseconds(500);
                break;
            case 2: // Type 2: Low priority, execute with 300ms delay
                delay = std::chrono::milliseconds(300);
                break;
            default: // Default - 1000ms delay
                delay = std::chrono::milliseconds(1000);
        }
        return now + delay;
    }

private:
    std::mutex mutex_;
};
/**
* @tc.name: FlowControlManager_CancelSpecificPriorityTask_Test
* @tc.desc: Test cancellation of specific priority tasks does not affect other priority tasks execution
* @tc.type: FUNC
* @tc.step: 1. Create a strategy that assigns different delays based on task priority
* @tc.step: 2. Initialize FlowControlManager with the strategy and an executor pool
* @tc.step: 3. Submit tasks with different priorities (priority 0, 1, and 2)
* @tc.step: 4. Cancel tasks with priority 1 before they execute
* @tc.step: 5. Verify that priority 0 and 2 tasks still execute normally
* @tc.step: 6. Confirm that priority 1 tasks are not executed after cancellation
* @tc.step: 7. Clean up resources by removing the FlowControlManager
* @tc.expected: Priority 1 tasks should be cancelled and not executed,
* while priority 0 and 2 tasks should execute normally
*/
HWTEST_F(FlowControlManagerTest, FlowControlManager_CancelSpecificPriorityTask_Test, TestSize.Level1)
{
    // Create executor pool with 2 initial threads and 3 max threads
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    FlowControlManager flowControlManager(pool, std::make_shared<PriorityCancellationStrategy>());

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
    flowControlManager.Execute(priority0Task, 0); // Should execute after 100ms
    flowControlManager.Execute(priority1Task, 1); // Should execute after 500ms - will be cancelled
    flowControlManager.Execute(priority2Task, 2); // Should execute after 300ms

    // Cancel priority 1 tasks before they execute (after 50ms, before 500ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    flowControlManager.Remove(1); // Remove all tasks with priority 1

    // After 100ms total, priority 0 task should execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(priority0Flag->load(), 1);
    EXPECT_EQ(priority1Flag->load(), 0);
    EXPECT_EQ(priority2Flag->load(), 0);

    // After 300ms total (200ms more), priority 2 task should execute
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(priority0Flag->load(), 1);
    EXPECT_EQ(priority1Flag->load(), 0);
    EXPECT_EQ(priority2Flag->load(), 1);

    // After 500ms total (200ms more), priority 1 task should still not execute because it was cancelled
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(priority0Flag->load(), 1);
    EXPECT_EQ(priority1Flag->load(), 0); // Still 0 because it was cancelled
    EXPECT_EQ(priority2Flag->load(), 1);

    // Submit a new priority 1 task to verify that the flow manager is still functional for that priority
    flowControlManager.Execute(priority1Task, 1);
    // Wait for it to execute (550ms > 500ms delay)
    std::this_thread::sleep_for(std::chrono::milliseconds(550));
    EXPECT_EQ(priority0Flag->load(), 1);
    EXPECT_EQ(priority1Flag->load(), 1); // This should now be 1 as it's a new task
    EXPECT_EQ(priority2Flag->load(), 1);

    flowControlManager.Remove();
}

class DelayStrategy : public FlowControlManager::Strategy {
public:
    FlowControlManager::Tp GetExecuteTime(FlowControlManager::Task task,
        const FlowControlManager::TaskInfo &info) override
    {
        // All tasks delayed by 500ms
        return std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    }
};
/**
* @tc.name: FlowControlManager_ExecuteAfterDestruction_Test
* @tc.desc: Test that tasks are not executed after FlowControlManager has been destroyed
* @tc.type: FUNC
* @tc.step: 1. Create a delay strategy that postpones all task executions
* @tc.step: 2. Initialize FlowControlManager with the delay strategy and an executor pool
* @tc.step: 3. Submit multiple tasks to the FlowControlManager
* @tc.step: 4. Destroy the FlowControlManager instance before tasks execute
* @tc.step: 5. Verify that none of the scheduled tasks execute after destruction
* @tc.step: 6. Confirm that attempt to submit new tasks after destruction are handled gracefully
* @tc.expected: No tasks should execute after FlowControlManager destruction and new submissions should be safe
*/
HWTEST_F(FlowControlManagerTest, FlowControlManager_ExecuteAfterDestruction_Test, TestSize.Level1)
{
    // Create executor pool with 2 initial threads and 3 max threads
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    auto flowControlManager = std::make_unique<FlowControlManager>(pool, std::make_shared<DelayStrategy>());

    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };

    // Submit 5 tasks
    for (int32_t i = 0; i < 5; i++) {
        flowControlManager->Execute(task);
    }

    // Wait for some time (100ms) but not enough for tasks to execute (500ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(flag->load(), 0); // Tasks not yet executed

    // Destroy the FlowControlManager
    flowControlManager.reset();

    // Even after waiting long enough (1000ms), tasks should not execute because manager was destroyed
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(flag->load(), 0); // Tasks successfully cancelled due to manager destruction

    // Note: Cannot test submitting new tasks as the object has been destroyed
    // The test confirms that destruction properly cancels pending tasks
}

class LabelBasedStrategy : public FlowControlManager::Strategy {
public:
    FlowControlManager::Tp GetExecuteTime(FlowControlManager::Task task,
        const FlowControlManager::TaskInfo &info) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();

        // Based on label, apply different flow control
        if (info.label == "HIGH_PRIORITY") {
            // High priority tasks execute immediately
            return now;
        } else if (info.label == "MEDIUM_PRIORITY") {
            // Medium priority tasks have 200ms delay
            return now + std::chrono::milliseconds(200);
        } else if (info.label == "LOW_PRIORITY") {
            // Low priority tasks have 500ms delay
            return now + std::chrono::milliseconds(500);
        }
        // Default: execute immediately
        return now;
    }

private:
    std::mutex mutex_;
};

/**
* @tc.name: FlowControlManager_LabelBasedFlowControl_Test
* @tc.desc: Test flow control based on task labels
* @tc.type: FUNC
* @tc.step: 1. Create a label-based strategy that applies different delays based on task labels
* @tc.step: 2. Initialize FlowControlManager with the label strategy and an executor pool
* @tc.step: 3. Submit tasks with different labels (HIGH_PRIORITY, MEDIUM_PRIORITY, LOW_PRIORITY)
* @tc.step: 4. Verify that tasks are executed according to their label-based delays
* @tc.step: 5. Clean up resources by removing the FlowControlManager
* @tc.expected: Tasks with different labels should be executed with corresponding delays
*/
HWTEST_F(FlowControlManagerTest, FlowControlManager_LabelBasedFlowControl_Test, TestSize.Level1)
{
    // Create executor pool with 2 initial threads and 3 max threads
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    FlowControlManager flowControlManager(pool, std::make_shared<LabelBasedStrategy>());

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

    // Submit tasks with different labels
    FlowControlManager::TaskInfo highInfo;
    highInfo.label = "HIGH_PRIORITY";
    flowControlManager.Execute(highPriorityTask, highInfo);

    FlowControlManager::TaskInfo mediumInfo;
    mediumInfo.label = "MEDIUM_PRIORITY";
    flowControlManager.Execute(mediumPriorityTask, mediumInfo);

    FlowControlManager::TaskInfo lowInfo;
    lowInfo.label = "LOW_PRIORITY";
    flowControlManager.Execute(lowPriorityTask, lowInfo);

    // Immediately check - only high priority task should be executed
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(highPriorityFlag->load(), 1);
    EXPECT_EQ(mediumPriorityFlag->load(), 0);
    EXPECT_EQ(lowPriorityFlag->load(), 0);

    // After 250ms, medium priority task should be executed
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(highPriorityFlag->load(), 1);
    EXPECT_EQ(mediumPriorityFlag->load(), 1);
    EXPECT_EQ(lowPriorityFlag->load(), 0);

    // After 500ms, low priority task should be executed
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(highPriorityFlag->load(), 1);
    EXPECT_EQ(mediumPriorityFlag->load(), 1);
    EXPECT_EQ(lowPriorityFlag->load(), 1);

    flowControlManager.Remove();
}

class RemoveByTypeRangeFilterStrategy : public FlowControlManager::Strategy {
public:
    FlowControlManager::Tp GetExecuteTime(FlowControlManager::Task task,
        const FlowControlManager::TaskInfo &info) override
    {
        // All tasks delayed by 500ms
        return std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    }
};
/**
* @tc.name: FlowControlManager_RemoveByTypeRangeFilter_Test
* @tc.desc: Test removing tasks using a custom filter based on task type ranges
* @tc.type: FUNC
* @tc.step: 1. Create tasks with different types spanning a range of values
* @tc.step: 2. Submit all tasks to FlowControlManager with a delay strategy
* @tc.step: 3. Use a custom filter to remove tasks within a specific type range
* @tc.step: 4. Verify only tasks outside the filtered range execute
* @tc.step: 5. Confirm tasks within the filtered range are cancelled
* @tc.expected: Only tasks with types outside the specified range should execute
*/
HWTEST_F(FlowControlManagerTest, FlowControlManager_RemoveByTypeRangeFilter_Test, TestSize.Level1)
{
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    FlowControlManager flowControlManager(pool, std::make_shared<RemoveByTypeRangeFilterStrategy>());

    std::vector<std::shared_ptr<std::atomic_uint32_t>> flags(10);
    for (int i = 0; i < 10; i++) {
        flags[i] = std::make_shared<std::atomic_uint32_t>(0);
    }

    // Submit tasks with types 0-9
    for (int i = 0; i < 10; i++) {
        auto task = [flag = flags[i]]() mutable {
            (*flag)++;
        };
        flowControlManager.Execute(task, i);
    }

    // Remove tasks with types between 3 and 6 (inclusive)
    flowControlManager.Remove([](const FlowControlManager::TaskInfo &info) {
        return info.type >= 3 && info.type <= 6;
    });

    // Wait for remaining tasks to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // Check that only tasks with types 0-2 and 7-9 executed
    for (int i = 0; i < 10; i++) {
        if (i >= 3 && i <= 6) {
            EXPECT_EQ(flags[i]->load(), 0) << "Task with type " << i << " should not have executed";
        } else {
            EXPECT_EQ(flags[i]->load(), 1) << "Task with type " << i << " should have executed";
        }
    }

    flowControlManager.Remove();
}

class RemoveByComplexFilterStrategy : public FlowControlManager::Strategy {
public:
    FlowControlManager::Tp GetExecuteTime(FlowControlManager::Task task,
        const FlowControlManager::TaskInfo &info) override
    {
        return std::chrono::steady_clock::now() + std::chrono::milliseconds(300);
    }
};
/**
* @tc.name: FlowControlManager_RemoveByComplexFilter_Test
* @tc.desc: Test removing tasks using a complex custom filter with multiple conditions
* @tc.type: FUNC
* @tc.step: 1. Create tasks with various combinations of type and label properties
* @tc.step: 2. Submit all tasks to FlowControlManager with a delay strategy
* @tc.step: 3. Use a complex filter to remove tasks matching multiple criteria
* @tc.step: 4. Verify only tasks not matching all filter conditions execute
* @tc.step: 5. Confirm tasks matching all filter conditions are cancelled
* @tc.expected: Only tasks that don't satisfy all filter conditions should execute
*/
HWTEST_F(FlowControlManagerTest, FlowControlManager_RemoveByComplexFilter_Test, TestSize.Level1)
{
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    FlowControlManager flowControlManager(pool, std::make_shared<RemoveByComplexFilterStrategy>());

    auto flag1 = std::make_shared<std::atomic_uint32_t>(0); // type=1, label="A"
    auto flag2 = std::make_shared<std::atomic_uint32_t>(0); // type=1, label="B"
    auto flag3 = std::make_shared<std::atomic_uint32_t>(0); // type=2, label="A"
    auto flag4 = std::make_shared<std::atomic_uint32_t>(0); // type=2, label="B"

    auto task1 = [flag1]() mutable {
        (*flag1)++;
    };
    auto task2 = [flag2]() mutable {
        (*flag2)++;
    };
    auto task3 = [flag3]() mutable {
        (*flag3)++;
    };
    auto task4 = [flag4]() mutable {
        (*flag4)++;
    };

    // Submit tasks with different type/label combinations
    FlowControlManager::TaskInfo info1(1);
    info1.label = "A";
    flowControlManager.Execute(task1, info1);

    FlowControlManager::TaskInfo info2(1);
    info2.label = "B";
    flowControlManager.Execute(task2, info2);

    FlowControlManager::TaskInfo info3(2);
    info3.label = "A";
    flowControlManager.Execute(task3, info3);

    FlowControlManager::TaskInfo info4(2);
    info4.label = "B";
    flowControlManager.Execute(task4, info4);

    // Remove tasks that are type 1 AND label "A" (only task1)
    flowControlManager.Remove([](const FlowControlManager::TaskInfo &info) {
        return info.type == 1 && info.label == "A";
    });

    // Wait for remaining tasks to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(400));

    // Check results
    EXPECT_EQ(flag1->load(), 0); // Removed
    EXPECT_EQ(flag2->load(), 1); // Executed
    EXPECT_EQ(flag3->load(), 1); // Executed
    EXPECT_EQ(flag4->load(), 1); // Executed

    flowControlManager.Remove();
}

class RemoveWithEmptyFilterStrategy : public FlowControlManager::Strategy {
public:
    FlowControlManager::Tp GetExecuteTime(FlowControlManager::Task task,
        const FlowControlManager::TaskInfo &info) override
    {
        // All tasks delayed by 200ms
        return std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
    }
};
/**
* @tc.name: FlowControlManager_RemoveWithEmptyFilter_Test
* @tc.desc: Test removing tasks with an empty filter that matches nothing
* @tc.type: FUNC
* @tc.step: 1. Create and submit several tasks to FlowControlManager
* @tc.step: 2. Use an empty filter that returns false for all tasks
* @tc.step: 3. Verify all tasks still execute normally
* @tc.step: 4. Test with a filter that matches everything
* @tc.step: 5. Verify all tasks are cancelled when matching everything
* @tc.expected: Empty filter should not remove any tasks; universal filter should remove all
*/
HWTEST_F(FlowControlManagerTest, FlowControlManager_RemoveWithEmptyFilter_Test, TestSize.Level1)
{
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    FlowControlManager flowControlManager(pool, std::make_shared<RemoveWithEmptyFilterStrategy>());

    auto flag1 = std::make_shared<std::atomic_uint32_t>(0);
    auto flag2 = std::make_shared<std::atomic_uint32_t>(0);
    auto flag3 = std::make_shared<std::atomic_uint32_t>(0);

    auto task1 = [flag1]() mutable {
        (*flag1)++;
    };
    auto task2 = [flag2]() mutable {
        (*flag2)++;
    };
    auto task3 = [flag3]() mutable {
        (*flag3)++;
    };

    flowControlManager.Execute(task1, 1);
    flowControlManager.Execute(task2, 2);
    flowControlManager.Execute(task3, 3);

    // Use empty filter (matches nothing)
    flowControlManager.Remove([](const FlowControlManager::TaskInfo &info) {
        return false; // Never match
    });

    // Wait for all tasks to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // All tasks should execute since nothing was removed
    EXPECT_EQ(flag1->load(), 1);
    EXPECT_EQ(flag2->load(), 1);
    EXPECT_EQ(flag3->load(), 1);

    // Reset flags
    flag1->store(0);
    flag2->store(0);
    flag3->store(0);

    // Submit new tasks
    flowControlManager.Execute(task1, 4);
    flowControlManager.Execute(task2, 5);
    flowControlManager.Execute(task3, 6);

    // Use universal filter (matches everything)
    flowControlManager.Remove([](const FlowControlManager::TaskInfo &info) {
        return true; // Match everything
    });

    // Wait to see if tasks execute
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // No tasks should execute since all were removed
    EXPECT_EQ(flag1->load(), 0);
    EXPECT_EQ(flag2->load(), 0);
    EXPECT_EQ(flag3->load(), 0);

    flowControlManager.Remove();
}

class NullPoolOnInitStrategy : public FlowControlManager::Strategy {
public:
    FlowControlManager::Tp GetExecuteTime(FlowControlManager::Task task,
        const FlowControlManager::TaskInfo &info) override
    {
        return std::chrono::steady_clock::now();
    }
};

/**
* @tc.name: FlowControlManager_NullPoolOnInit_Test
* @tc.desc: Test behavior when FlowControlManager is initialized with null executor pool
* @tc.type: FUNC
* @tc.step: 1. Initialize FlowControlManager with null executor pool and a valid strategy
* @tc.step: 2. Submit multiple tasks to the FlowControlManager
* @tc.step: 3. Verify that no tasks are executed due to null executor pool
* @tc.step: 4. Clean up resources by removing the FlowControlManager
* @tc.expected: No tasks should be executed since the executor pool is null
*/
HWTEST_F(FlowControlManagerTest, FlowControlManager_NullPoolOnInit_Test, TestSize.Level1)
{
    // Create null executor pool
    std::shared_ptr<ExecutorPool> pool = nullptr;
    FlowControlManager flowControlManager(pool, std::make_shared<NullPoolOnInitStrategy>()); // Null pool
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };
    // submit 5 tasks
    const int taskCount = 5;
    for (int32_t i = 0; i < taskCount; i++) {
        flowControlManager.Execute(task);
    }

    // Wait briefly to ensure no tasks execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(flag->load(), 0); // No tasks should be executed due to null pool
    flowControlManager.Remove();
}
} // namespace OHOS::Test