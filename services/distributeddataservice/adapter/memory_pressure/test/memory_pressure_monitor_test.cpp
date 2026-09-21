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

#include <vector>

#include <gtest/gtest.h>

#include "error/general_error.h"
#include "memory_pressure_monitor_impl.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;

namespace OHOS::Test {
class MemoryPressureMonitorTest : public testing::Test {
};

/**
 * @tc.name: Subscribe_InvalidArguments_ReturnsError001
 * @tc.desc: Verify invalid subscriptions are rejected before accessing Memory Manager.
 * @tc.type: FUNC
 */
HWTEST_F(MemoryPressureMonitorTest, Subscribe_InvalidArguments_ReturnsError001, TestSize.Level1)
{
    MemoryPressureMonitorImpl monitor;

    EXPECT_EQ(monitor.Subscribe("", [](int32_t) {}), E_INVALID_ARGS);
    EXPECT_EQ(monitor.Subscribe("observer", nullptr), E_INVALID_ARGS);
}

/**
 * @tc.name: OnTrim_StartedMonitor_ForwardsEveryEvent002
 * @tc.desc: Verify the adapter forwards event values unchanged and leaves coalescing to the device guard.
 * @tc.type: FUNC
 */
HWTEST_F(MemoryPressureMonitorTest, OnTrim_StartedMonitor_ForwardsEveryEvent002, TestSize.Level1)
{
    MemoryPressureMonitorImpl monitor;
    std::vector<int32_t> levels;
    monitor.started_ = true;
    monitor.observers_["observer"] = [&levels](int32_t level) {
        levels.push_back(level);
    };

    monitor.OnTrim(2);
    monitor.OnTrim(2);
    monitor.OnTrim(4);

    EXPECT_EQ(levels, (std::vector<int32_t> { 2, 2, 4 }));
}

/**
 * @tc.name: OnTrim_StoppedMonitor_IgnoresLateEvent003
 * @tc.desc: Verify a callback received after local unsubscription is not forwarded.
 * @tc.type: FUNC
 */
HWTEST_F(MemoryPressureMonitorTest, OnTrim_StoppedMonitor_IgnoresLateEvent003, TestSize.Level1)
{
    MemoryPressureMonitorImpl monitor;
    int32_t callbackCount = 0;
    monitor.observers_["observer"] = [&callbackCount](int32_t) {
        ++callbackCount;
    };

    monitor.OnTrim(4);

    EXPECT_EQ(callbackCount, 0);
}

/**
 * @tc.name: Unsubscribe_MissingObserver_ReturnsError004
 * @tc.desc: Verify removing an unknown observer does not access Memory Manager.
 * @tc.type: FUNC
 */
HWTEST_F(MemoryPressureMonitorTest, Unsubscribe_MissingObserver_ReturnsError004, TestSize.Level1)
{
    MemoryPressureMonitorImpl monitor;

    EXPECT_EQ(monitor.Unsubscribe("missing"), E_ERROR);
}

/**
 * @tc.name: GetInstanceAndRegisterInstance_Registered_ReturnsStableInstance005
 * @tc.desc: Verify the getter returns the load-time registered instance and duplicate registration
 *           is rejected without clobbering it.
 * @tc.type: FUNC
 */
HWTEST_F(MemoryPressureMonitorTest, GetInstanceAndRegisterInstance_Registered_ReturnsStableInstance005,
    TestSize.Level1)
{
    MemoryPressureMonitor *instance = MemoryPressureMonitor::GetInstance();
    ASSERT_NE(instance, nullptr);

    MemoryPressureMonitorImpl other;
    EXPECT_FALSE(MemoryPressureMonitor::RegisterInstance(&other));
    EXPECT_EQ(MemoryPressureMonitor::GetInstance(), instance);
}

/**
 * @tc.name: Subscribe_AlreadyStarted_ReturnsOk006
 * @tc.desc: Verify a subscribe on an already started monitor records the observer and returns
 *           immediately without reaching Memory Manager.
 * @tc.type: FUNC
 */
HWTEST_F(MemoryPressureMonitorTest, Subscribe_AlreadyStarted_ReturnsOk006, TestSize.Level1)
{
    MemoryPressureMonitorImpl monitor;
    monitor.started_ = true;

    EXPECT_EQ(monitor.Subscribe("observer", [](int32_t) {}), E_OK);
    EXPECT_NE(monitor.observers_.find("observer"), monitor.observers_.end());
}

/**
 * @tc.name: Unsubscribe_ExistingObserver_NotLast_ReturnsOk007
 * @tc.desc: Verify removing an existing observer keeps the monitor started while others remain.
 * @tc.type: FUNC
 */
HWTEST_F(MemoryPressureMonitorTest, Unsubscribe_ExistingObserver_NotLast_ReturnsOk007, TestSize.Level1)
{
    MemoryPressureMonitorImpl monitor;
    monitor.observers_["first"] = [](int32_t) {};
    monitor.observers_["second"] = [](int32_t) {};
    monitor.started_ = true;

    EXPECT_EQ(monitor.Unsubscribe("first"), E_OK);
    EXPECT_EQ(monitor.observers_.size(), 1U);
    EXPECT_TRUE(monitor.started_);
}

/**
 * @tc.name: Unsubscribe_ExistingObserver_NotStarted_ReturnsOk008
 * @tc.desc: Verify removing the last observer on a stopped monitor returns without touching Memory Manager.
 * @tc.type: FUNC
 */
HWTEST_F(MemoryPressureMonitorTest, Unsubscribe_ExistingObserver_NotStarted_ReturnsOk008, TestSize.Level1)
{
    MemoryPressureMonitorImpl monitor;
    monitor.observers_["observer"] = [](int32_t) {};

    EXPECT_EQ(monitor.Unsubscribe("observer"), E_OK);
    EXPECT_TRUE(monitor.observers_.empty());
    EXPECT_FALSE(monitor.started_);
}

/**
 * @tc.name: OnTrim_NullObserver_SkipsInvocation009
 * @tc.desc: Verify a started monitor skips null observers while still invoking valid ones.
 * @tc.type: FUNC
 */
HWTEST_F(MemoryPressureMonitorTest, OnTrim_NullObserver_SkipsInvocation009, TestSize.Level1)
{
    MemoryPressureMonitorImpl monitor;
    int32_t count = 0;
    monitor.started_ = true;
    monitor.observers_["valid"] = [&count](int32_t) {
        ++count;
    };
    monitor.observers_["null"] = nullptr;

    monitor.OnTrim(3);

    EXPECT_EQ(count, 1);
}
} // namespace OHOS::Test
