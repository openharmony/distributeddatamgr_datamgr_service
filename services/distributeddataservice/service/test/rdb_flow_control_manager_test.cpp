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
    RdbFlowControlManager flowControlManager;
    // App limit: 2, Device limit: 100, Delay duration: 500ms
    flowControlManager.Init(pool, std::make_shared<RdbFlowControlStrategy>(2, 100, 500));
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
    RdbFlowControlManager flowControlManager;
    // App limit: 5, Device limit: 1 (will trigger delay), Delay duration: 500ms
    flowControlManager.Init(pool, std::make_shared<RdbFlowControlStrategy>(5, 1, 500));
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
    RdbFlowControlManager flowControlManager;
    // App limit: 1 (will trigger delay), Device limit: 5, Delay duration: 500ms
    flowControlManager.Init(pool, std::make_shared<RdbFlowControlStrategy>(1, 5, 500));
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
    RdbFlowControlManager flowControlManager;
    // App limit: 1, Device limit: 1, Delay duration: 500ms
    flowControlManager.Init(pool, std::make_shared<RdbFlowControlStrategy>(1, 1, 500));
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
    RdbFlowControlManager flowControlManager;
    // App limit: 1, Device limit: 2, Delay duration: 500ms
    flowControlManager.Init(pool, std::make_shared<RdbFlowControlStrategy>(1, 2, 500));
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
    RdbFlowControlManager flowControlManager;
    // App limit: 10, Device limit: 10, Delay duration: 50ms
    flowControlManager.Init(pool, std::make_shared<RdbFlowControlStrategy>(10, 10, 50));
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };

    const int taskCount = 100;
    for (int i = 0; i < taskCount; i++) {
        flowControlManager.Execute(task, { 0, "bulkTask" });
    }

    // Wait enough time for all tasks to complete (1000ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
    RdbFlowControlManager flowControlManager;
    // App limit: 1, Device limit: 1, Delay duration: 0ms (no actual delay)
    flowControlManager.Init(pool, std::make_shared<RdbFlowControlStrategy>(1, 1, 0));
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
} // namespace DistributedRDBTest
} // namespace OHOS::Test
