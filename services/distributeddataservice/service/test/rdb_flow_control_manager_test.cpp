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

#include "rdb_flow_control_manager.h"

#include "gtest/gtest.h"

using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
namespace DistributedRDBTest {

class RdbFlowControlManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp() {}
    void TearDown(){};
};

/**
 * @tc.name: RdbFlowControlManager_ExecuteWhenNoDelayStrategy_Test
 * @tc.desc: Test the task execution function of RdbFlowControlManager under no delay strategy
 * @tc.type: FUNC
 * @tc.require:
 * @tc.step: 1. Create an executor pool with 2 initial threads and 3 maximum threads
 * @tc.step: 2. Initialize RdbFlowControlManager and set sync flow control strategy with app limit 2, device limit 100,
 * delay 500ms
 * @tc.step: 3. Submit 5 tasks to the flow control manager
 * @tc.step: 4. Wait for 200ms, check that only 2 tasks executed (limited by app limit)
 * @tc.step: 5. Wait for additional 500ms, check that 4 tasks executed
 * @tc.step: 6. Wait for additional 500ms, check that all 5 tasks executed
 * @tc.expected: Tasks are executed according to the flow control limits
 */
HWTEST_F(RdbFlowControlManagerTest, RdbFlowControlManager_ExecuteWhenNoDelayStrategy_Test, TestSize.Level1)
{
    // Create executor pool with 2 initial threads and 3 max threads
    auto pool = std::make_shared<ExecutorPool>(2, 3);
    // App limit: 2, Device limit: 100, Delay duration: 500ms
    RdbFlowControlManager flowControlManager(2, 100, 500);
    flowControlManager.Init(pool);
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };
    const std::string label = "syncTask";
    // Submit 5 tasks
    for (int32_t i = 0; i < 5; i++) {
        flowControlManager.Execute(task, { 0, label });
    }
    // Wait for 200 millisecond - only 2 task should execute (limited by app limit)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(flag->load(), 2);
    // Wait for 700 millisecond - only 4 task should execute
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(flag->load(), 4);
    // Wait for 1200 millisecond - all task should execute
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(flag->load(), 5);
}

/**
 * @tc.name: RdbSyncFlowControlStrategy_DeviceLimitTriggerDelay_Test
 * @tc.desc: When device limit is reached, next task should be delayed by duration
 * @tc.type: FUNC
 * @tc.require:
 * @tc.step: 1. Create an executor pool with 1 initial thread and 1 maximum thread
 * @tc.step: 2. Initialize RdbFlowControlManager with app limit 5, device limit 1, delay 500ms
 * @tc.step: 3. Submit 2 tasks rapidly
 * @tc.step: 4. Wait for 100ms, check that only 1 task executed
 * @tc.step: 5. Wait for additional 500ms, check that both tasks executed
 * @tc.expected: Second task is delayed by 500ms due to device limit
 */
HWTEST_F(RdbFlowControlManagerTest, RdbSyncFlowControlStrategy_DeviceLimitTriggerDelay_Test, TestSize.Level1)
{
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    // App limit: 5, Device limit: 1 (will trigger delay), Delay duration: 500ms
    RdbFlowControlManager flowControlManager(5, 1, 500);
    flowControlManager.Init(pool);
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };

    flowControlManager.Execute(task, { 0, "syncTask" });
    flowControlManager.Execute(task, { 0, "syncTask" });

    // First task executes immediately
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(flag->load(), 1);

    // Second task delayed 500ms
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(flag->load(), 2);
}

/**
 * @tc.name: RdbSyncFlowControlStrategy_AppLimitTriggerDelay_Test
 * @tc.desc: When app limit is reached, next task should be delayed by duration
 * @tc.type: FUNC
 * @tc.require:
 * @tc.step: 1. Create an executor pool with 1 initial thread and 1 maximum thread
 * @tc.step: 2. Initialize RdbFlowControlManager with app limit 1, device limit 5, delay 500ms
 * @tc.step: 3. Submit 2 tasks rapidly
 * @tc.step: 4. Wait for 100ms, check that only 1 task executed
 * @tc.step: 5. Wait for additional 500ms, check that both tasks executed
 * @tc.expected: Second task is delayed by 500ms due to app limit
 */
HWTEST_F(RdbFlowControlManagerTest, RdbSyncFlowControlStrategy_AppLimitTriggerDelay_Test, TestSize.Level1)
{
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    // App limit: 1 (will trigger delay), Device limit: 5, Delay duration: 500ms
    RdbFlowControlManager flowControlManager(1, 5, 500);
    flowControlManager.Init(pool);
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };

    flowControlManager.Execute(task, { 0, "syncTask" });
    flowControlManager.Execute(task, { 0, "syncTask" });

    // First task executes immediately
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(flag->load(), 1);

    // Second task delayed 500ms
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(flag->load(), 2);
}

/**
 * @tc.name: RdbSyncFlowControlStrategy_BothLimitsReached_Test
 * @tc.desc: When both limits are reached, longer delay should be chosen
 * @tc.type: FUNC
 * @tc.require:
 * @tc.step: 1. Create an executor pool with 1 initial thread and 1 maximum thread
 * @tc.step: 2. Initialize RdbFlowControlManager with app limit 1, device limit 1, delay 500ms
 * @tc.step: 3. Submit 2 tasks rapidly
 * @tc.step: 4. Wait for 100ms, check that only 1 task executed
 * @tc.step: 5. Wait for additional 500ms, check that both tasks executed
 * @tc.expected: Second task is delayed by 500ms due to both limits being reached
 */
HWTEST_F(RdbFlowControlManagerTest, RdbSyncFlowControlStrategy_BothLimitsReached_Test, TestSize.Level1)
{
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    // App limit: 1, Device limit: 1, Delay duration: 500ms
    RdbFlowControlManager flowControlManager(1, 1, 500);
    flowControlManager.Init(pool);
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };

    flowControlManager.Execute(task, { 0, "syncTask" });
    flowControlManager.Execute(task, { 0, "syncTask" });

    // First task executes immediately
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(flag->load(), 1);

    // Second task delayed 500ms
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(flag->load(), 2);
}

/**
 * @tc.name: RdbSyncFlowControlStrategy_DifferentLabelsIndependent_Test
 * @tc.desc: Tasks with different labels should have independent limits
 * @tc.type: FUNC
 * @tc.require:
 * @tc.step: 1. Create an executor pool with 2 initial threads and 2 maximum threads
 * @tc.step: 2. Initialize RdbFlowControlManager with app limit 1, device limit 2, delay 500ms
 * @tc.step: 3. Submit one task each for two different labels
 * @tc.step: 4. Wait for 100ms, check that both tasks executed (independent limits)
 * @tc.expected: Tasks with different labels execute independently
 */
HWTEST_F(RdbFlowControlManagerTest, RdbSyncFlowControlStrategy_DifferentLabelsIndependent_Test, TestSize.Level1)
{
    auto pool = std::make_shared<ExecutorPool>(2, 2);
    // App limit: 1, Device limit: 2, Delay duration: 500ms
    RdbFlowControlManager flowControlManager(1, 2, 500);
    flowControlManager.Init(pool);
    auto flag1 = std::make_shared<std::atomic_uint32_t>(0);
    auto flag2 = std::make_shared<std::atomic_uint32_t>(0);
    auto task1 = [flag1]() mutable {
        (*flag1)++;
    };
    auto task2 = [flag2]() mutable {
        (*flag2)++;
    };

    flowControlManager.Execute(task1, { 0, "syncTask1" }); // First label, no delay
    flowControlManager.Execute(task2, { 0, "syncTask2" }); // Second label, no delay

    // Both tasks should execute immediately since they have different labels
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(flag1->load(), 1);
    EXPECT_EQ(flag2->load(), 1);
}

/**
 * @tc.name: RdbFlowControlManager_ExecuteWithManyTasks_Test
 * @tc.desc: Test executing large number of tasks simultaneously
 * @tc.type: FUNC
 * @tc.require:
 * @tc.step: 1. Create RdbFlowControlManager with limited thread pool (2 initial, 2 maximum)
 * @tc.step: 2. Configure flow control with app limit 10, device limit 10, delay 50ms
 * @tc.step: 3. Submit 100 tasks rapidly
 * @tc.step: 4. Wait for 1000ms for all tasks to complete
 * @tc.step: 5. Verify all 100 tasks are executed
 * @tc.expected: All 100 tasks should be executed without loss
 */
HWTEST_F(RdbFlowControlManagerTest, RdbFlowControlManager_ExecuteWithManyTasks_Test, TestSize.Level1)
{
    auto pool = std::make_shared<ExecutorPool>(2, 2);
    // App limit: 10, Device limit: 10, Delay duration: 50ms
    RdbFlowControlManager flowControlManager(10, 10, 50);
    flowControlManager.Init(pool);
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };

    // Submit 100 tasks
    const int taskCount = 100;
    for (int i = 0; i < taskCount; i++) {
        flowControlManager.Execute(task, { 0, "bulkTask" });
    }
    // Wait not enough time for all tasks to complete (230ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(230));
    EXPECT_LE(flag->load(), taskCount/2);

    // Wait enough time for all tasks to complete (500ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(flag->load(), taskCount);
}

/**
 * @tc.name: RdbFlowControlManager_ExecuteWithZeroDelayStrategy_Test
 * @tc.desc: Test task execution when delay duration is zero
 * @tc.type: FUNC
 * @tc.require:
 * @tc.step: 1. Create RdbFlowControlManager with single thread pool (1 initial, 1 maximum)
 * @tc.step: 2. Configure flow control with app limit 1, device limit 1, delay 0ms
 * @tc.step: 3. Submit 2 tasks rapidly
 * @tc.step: 4. Wait for 10ms
 * @tc.step: 5. Verify both tasks executed immediately
 * @tc.expected: Tasks should execute immediately even when limits are exceeded due to zero delay
 */
HWTEST_F(RdbFlowControlManagerTest, RdbFlowControlManager_ExecuteWithZeroDelayStrategy_Test, TestSize.Level1)
{
    auto pool = std::make_shared<ExecutorPool>(1, 1);
    // App limit: 1, Device limit: 1, Delay duration: 0ms (no actual delay)
    RdbFlowControlManager flowControlManager(1, 1, 0);
    flowControlManager.Init(pool);
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };

    flowControlManager.Execute(task, { 0, "zeroDelayTask" });
    flowControlManager.Execute(task, { 0, "zeroDelayTask" });

    // Both tasks should execute immediately due to zero delay duration
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_EQ(flag->load(), 2);
}

/**
 * @tc.name: RdbFlowControlManager_ExecuteMultiLabelHighFrequency_Test
 * @tc.desc: Test multi-label task execution under high frequency scenario with device and app limits
 * @tc.type: FUNC
 * @tc.require:
 * @tc.step: 1. Create RdbFlowControlManager with thread pool (3 initial, 3 maximum)
 * @tc.step: 2. Configure flow control with app limit 2, device limit 3, delay 100ms
 * @tc.step: 3. Submit multiple tasks with different labels rapidly
 * @tc.step: 4. Check execution count at different time points to verify limits
 * @tc.step: 5. Wait for sufficient time and verify all tasks executed
 * @tc.expected: Different labels should be controlled independently but respect overall device limit
 */
HWTEST_F(RdbFlowControlManagerTest, RdbFlowControlManager_ExecuteMultiLabelHighFrequency_Test, TestSize.Level1)
{
    auto pool = std::make_shared<ExecutorPool>(3, 3);
    RdbFlowControlManager flowControlManager(2, 3, 100);
    flowControlManager.Init(pool);

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

    // Submit 6 tasks rapidly (2 for each label)
    for (int i = 0; i < 2; i++) {
        flowControlManager.Execute(task1, { 0, "label1" });
        flowControlManager.Execute(task2, { 0, "label2" });
        flowControlManager.Execute(task3, { 0, "label3" });
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // At this point, 3 tasks (one from each label) should execute immediately due to device limit
    // The second task of each label should be delayed
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(flag1->load(), 1);
    EXPECT_EQ(flag2->load(), 1);
    EXPECT_EQ(flag3->load(), 1);

    // After 100ms, remaining tasks should start executing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(flag1->load(), 2);
    EXPECT_EQ(flag2->load(), 2);
    EXPECT_EQ(flag3->load(), 2);
}

/**
 * @tc.name: RdbFlowControlManager_ExecuteMultiLabelWithOneDominant_Test
 * @tc.desc: Test multi-label task execution where one label dominates and others are affected by device limit
 * @tc.type: FUNC
 * @tc.require:
 * @tc.step: 1. Create RdbFlowControlManager with thread pool (2 initial, 2 maximum)
 * @tc.step: 2. Configure flow control with app limit 5, device limit 2, delay 200ms
 * @tc.step: 3. Submit multiple tasks where one label has more tasks than others
 * @tc.step: 4. Check execution count at different time points to verify limits
 * @tc.step: 5. Wait for sufficient time and verify all tasks executed
 * @tc.expected: Dominant label tasks should be delayed after device limit is reached, allowing other labels to execute
 */
HWTEST_F(RdbFlowControlManagerTest, RdbFlowControlManager_ExecuteMultiLabelWithOneDominant_Test, TestSize.Level1)
{
    auto pool = std::make_shared<ExecutorPool>(2, 2);
    // App limit: 5, Device limit: 2, Delay duration: 200ms
    RdbFlowControlManager flowControlManager(5, 2, 200);
    flowControlManager.Init(pool);

    auto dominantFlag = std::make_shared<std::atomic_uint32_t>(0);
    auto secondaryFlag1 = std::make_shared<std::atomic_uint32_t>(0);
    auto secondaryFlag2 = std::make_shared<std::atomic_uint32_t>(0);

    auto dominantTask = [dominantFlag]() mutable {
        (*dominantFlag)++;
    };
    auto secondaryTask1 = [secondaryFlag1]() mutable {
        (*secondaryFlag1)++;
    };
    auto secondaryTask2 = [secondaryFlag2]() mutable {
        (*secondaryFlag2)++;
    };

    // Submit tasks: 4 dominant tasks, 1 each for secondary labels
    // This exceeds device limit of 2, so some tasks will be delayed
    flowControlManager.Execute(dominantTask, { 0, "dominant" });
    flowControlManager.Execute(dominantTask, { 0, "dominant" });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    flowControlManager.Execute(secondaryTask1, { 0, "secondary1" });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    flowControlManager.Execute(dominantTask, { 0, "dominant" });
    flowControlManager.Execute(dominantTask, { 0, "dominant" });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    flowControlManager.Execute(secondaryTask2, { 0, "secondary2" });

    // Initially, only 2 tasks should execute (device limit)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(dominantFlag->load(), 2);
    EXPECT_EQ(secondaryFlag1->load(), 0);
    EXPECT_EQ(secondaryFlag2->load(), 0);

    // After 200ms, next batch should execute
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(dominantFlag->load(), 3);
    EXPECT_EQ(secondaryFlag1->load(), 1);
    EXPECT_EQ(secondaryFlag2->load(), 0);

    // After another 200ms, all tasks should execute
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(dominantFlag->load(), 4);
    EXPECT_EQ(secondaryFlag1->load(), 1);
    EXPECT_EQ(secondaryFlag2->load(), 1);
}

/**
 * @tc.name: RdbFlowControlManager_ExecuteMultiLabelWithOneLimited_Test
 * @tc.desc: Test multi-label task execution where one label is frequently limited by app limit while another isn't
 * @tc.type: FUNC
 * @tc.require:
 * @tc.step: 1. Create RdbFlowControlManager with thread pool (3 initial, 3 maximum)
 * @tc.step: 2. Configure flow control with app limit 2, device limit 5, delay 100ms
 * @tc.step: 3. Submit frequent tasks for label1 which should hit app limit, and infrequent tasks for label2
 * @tc.step: 4. Check execution count at different time points to verify label1 is limited while label2 is not
 * @tc.step: 5. Wait for sufficient time and verify all tasks executed
 * @tc.expected: label1 tasks should be delayed by app limit while label2 tasks execute immediately
 */
HWTEST_F(RdbFlowControlManagerTest, RdbFlowControlManager_ExecuteMultiLabelWithOneLimited_Test, TestSize.Level1)
{
    auto pool = std::make_shared<ExecutorPool>(3, 3);
    // App limit: 2, Device limit: 5, Delay duration: 100ms
    RdbFlowControlManager flowControlManager(2, 5, 100);
    flowControlManager.Init(pool);

    auto flag1 = std::make_shared<std::atomic_uint32_t>(0);
    auto flag2 = std::make_shared<std::atomic_uint32_t>(0);

    auto task1 = [flag1]() mutable {
        (*flag1)++;
    };
    auto task2 = [flag2]() mutable {
        (*flag2)++;
    };

    // Submit 4 tasks for label1 rapidly (should hit app limit of 2)
    for (int i = 0; i < 10; i++) {
        flowControlManager.Execute(task1, { 0, "label1" });
    }

    // Submit 1 task for label2 (should not be limited)
    flowControlManager.Execute(task2, { 0, "label2" });

    // After short wait, 2 label1 tasks and 1 label2 task should execute
    // label1 is limited by app limit (2), label2 has no limit
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(flag1->load(), 2);  // Limited by app limit
    EXPECT_EQ(flag2->load(), 1);  // No limit for this label
    flowControlManager.Remove("label1");
}

/**
 * @tc.name: RdbFlowControlManager_ExecuteMultiLabelRandomLoad_Test
 * @tc.desc: Test multi-label task execution with random load simulation
 * @tc.type: FUNC
 * @tc.require:
 * @tc.step: 1. Create RdbFlowControlManager with thread pool (5 initial, 5 maximum)
 * @tc.step: 2. Configure flow control with app limit 2, device limit 10, delay 100ms
 * @tc.step: 3. Simulate random task generation for 3 labels over 5 seconds using separate threads for each label
 * @tc.step: 4. Check execution counts periodically to verify flow control
 * @tc.step: 5. Wait for sufficient time and verify all tasks executed
 * @tc.expected: Each label should be limited to 2 tasks per second, total not exceeding 10 per second
 */
HWTEST_F(RdbFlowControlManagerTest, RdbFlowControlManager_ExecuteMultiLabelRandomLoad_Test, TestSize.Level1)
{
    auto pool = std::make_shared<ExecutorPool>(5, 5);
    // App limit: 2 (per label), Device limit: 10 (total), Delay duration: 100ms
    RdbFlowControlManager flowControlManager(2, 10, 1000);
    flowControlManager.Init(pool);

    const int NUM_LABELS = 3;
    const int TEST_DURATION_SECONDS = 5;
    const int TASKS_PER_LABEL = 15;

    std::vector<std::shared_ptr<std::atomic_uint32_t>> flags;
    std::vector<std::string> labels = {"random_label1", "random_label2", "random_label3"};
    std::vector<std::function<void()>> tasks;

    // Initialize flags and tasks for each label
    for (int i = 0; i < NUM_LABELS; i++) {
        flags.push_back(std::make_shared<std::atomic_uint32_t>(0));
        tasks.push_back([flag = flags[i]]() mutable {
            (*flag)++;
        });
    }

    // Create threads for each label to simulate concurrent task generation
    std::vector<std::thread> threads;
    auto startTime = std::chrono::steady_clock::now();

    for (int labelIdx = 0; labelIdx < NUM_LABELS; labelIdx++) {
        threads.emplace_back([&, labelIdx]() {
            srand(static_cast<unsigned int>(time(nullptr)) + labelIdx); // Different seed for each thread
            for (int i = 0; i < TASKS_PER_LABEL; i++) {
                flowControlManager.Execute(tasks[labelIdx], { 0, labels[labelIdx] });

                // Random small delays to simulate real-world task arrival pattern
                int randomDelay = rand() % 100; // 0-99 ms
                std::this_thread::sleep_for(std::chrono::milliseconds(randomDelay));
            }
        });
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    // Additional wait time to allow all tasks to execute based on flow control rules
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime).count();
    if (elapsed < TEST_DURATION_SECONDS * 1000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(TEST_DURATION_SECONDS * 1000 - elapsed - 500));
    }

    // Verify final counts are within expected ranges considering app limit (2/sec) and device limit (10/sec)
    for (int labelIdx = 0; labelIdx < NUM_LABELS; labelIdx++) {
        // Each label should have at most 2 tasks/sec * 5 sec = 10 tasks executed
        EXPECT_LE(flags[labelIdx]->load(), 10);
        // But also ensure reasonable minimum execution
        EXPECT_GE(flags[labelIdx]->load(), 1);
    }

    // Total executed tasks should not exceed device limit (10/sec * 5 sec = 50 tasks)
    int totalExecutedTasks = 0;
    for (const auto& flag : flags) {
        totalExecutedTasks += flag->load();
    }
    EXPECT_LE(totalExecutedTasks, 50);
}

/**
 * @tc.name: RdbFlowControlManager_InitWithNullPool_Test
 * @tc.desc: Test RdbFlowControlManager behavior when initialized with null executor pool
 * @tc.type: FUNC
 * @tc.require:
 * @tc.step: 1. Initialize RdbFlowControlManager with null executor pool
 * @tc.step: 2. Submit several tasks to the flow control manager
 * @tc.step: 3. Wait for sufficient time
 * @tc.step: 4. Check that no tasks were executed
 * @tc.expected: No tasks should be executed when initialized with null pool
 */
HWTEST_F(RdbFlowControlManagerTest, RdbFlowControlManager_InitWithNullPool_Test, TestSize.Level1)
{
    // Initialize with null pool
    RdbFlowControlManager flowControlManager(2, 100, 500);
    flowControlManager.Init(nullptr);

    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };

    // Submit 5 tasks
    for (int32_t i = 0; i < 5; i++) {
        flowControlManager.Execute(task, { 0, "nullPoolTask" });
    }

    // Wait for sufficient time to allow execution if there was a pool
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // No tasks should execute because there's no executor pool
    EXPECT_EQ(flag->load(), 0);
}

} // namespace DistributedRDBTest
} // namespace OHOS::Test
