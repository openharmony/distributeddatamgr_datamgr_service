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

#include "error/general_error.h"
#include "thermal_state_monitor_impl.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;

namespace OHOS::Test {
class ThermalStateMonitorTest : public testing::Test {
};

/**
 * @tc.name: Subscribe_InvalidArguments_ReturnsError001
 * @tc.desc: Verify empty names and null callbacks are rejected before accessing Thermal Manager.
 * @tc.type: FUNC
 */
HWTEST_F(ThermalStateMonitorTest, Subscribe_InvalidArguments_ReturnsError001, TestSize.Level1)
{
    ThermalStateMonitorImpl monitor;

    EXPECT_EQ(monitor.Subscribe("", [](const ThermalStateMonitor::Snapshot &) {}), E_INVALID_ARGS);
    EXPECT_EQ(monitor.Subscribe("observer", nullptr), E_INVALID_ARGS);
}

/**
 * @tc.name: OnThermalLevelChanged_OutOfRangeLevels_ClampsToBoundaries002
 * @tc.desc: Verify out-of-range thermal values are clamped while duplicate levels are not notified.
 * @tc.type: FUNC
 */
HWTEST_F(ThermalStateMonitorTest, OnThermalLevelChanged_OutOfRangeLevels_ClampsToBoundaries002,
    TestSize.Level1)
{
    ThermalStateMonitorImpl monitor;
    int32_t callbackCount = 0;
    monitor.started_ = true;
    monitor.observers_["observer"] = [&callbackCount](const ThermalStateMonitor::Snapshot &) {
        ++callbackCount;
    };

    monitor.OnThermalLevelChanged(3);
    monitor.OnThermalLevelChanged(3);
    monitor.OnThermalLevelChanged(-1);
    monitor.OnThermalLevelChanged(8);

    auto snapshot = monitor.GetSnapshot();
    EXPECT_EQ(callbackCount, 3);
    EXPECT_EQ(snapshot.level, 7);
}

/**
 * @tc.name: Unsubscribe_MissingObserver_ReturnsError003
 * @tc.desc: Verify removing an unknown observer reports an error without touching Thermal Manager registration.
 * @tc.type: FUNC
 */
HWTEST_F(ThermalStateMonitorTest, Unsubscribe_MissingObserver_ReturnsError003, TestSize.Level1)
{
    ThermalStateMonitorImpl monitor;

    EXPECT_EQ(monitor.Unsubscribe("missing"), E_ERROR);
}

/**
 * @tc.name: NormalizeInitialLevel_QueryFailure_PreservesCool004
 * @tc.desc: Verify Thermal Manager query failure is not treated as a sampled COOL level.
 * @tc.type: FUNC
 */
HWTEST_F(ThermalStateMonitorTest, NormalizeInitialLevel_QueryFailure_PreservesCool004, TestSize.Level1)
{
    ThermalStateMonitorImpl monitor;
    int32_t level = 3;

    EXPECT_FALSE(monitor.NormalizeInitialLevel(-1, level));
    EXPECT_EQ(level, 3);
}

/**
 * @tc.name: Subscribe_StartedMonitorMultipleObservers_StopsAfterLastUnsubscribe005
 * @tc.desc: Verify observers share a started monitor and only the last unsubscribe stops its local lifecycle.
 * @tc.type: FUNC
 */
HWTEST_F(ThermalStateMonitorTest, Subscribe_StartedMonitorMultipleObservers_StopsAfterLastUnsubscribe005,
    TestSize.Level1)
{
    ThermalStateMonitorImpl monitor;
    monitor.started_ = true;
    monitor.snapshot_.level = 3;
    int32_t firstLevel = -1;
    int32_t secondLevel = -1;

    ASSERT_EQ(monitor.Subscribe("first", [&firstLevel](const auto &snapshot) {
        firstLevel = snapshot.level;
    }), E_OK);
    ASSERT_EQ(monitor.Subscribe("second", [&secondLevel](const auto &snapshot) {
        secondLevel = snapshot.level;
    }), E_OK);
    EXPECT_EQ(firstLevel, 3);
    EXPECT_EQ(secondLevel, 3);
    EXPECT_EQ(monitor.observers_.size(), 2U);

    EXPECT_EQ(monitor.Unsubscribe("first"), E_OK);
    EXPECT_TRUE(monitor.started_);
    EXPECT_EQ(monitor.observers_.size(), 1U);
    EXPECT_EQ(monitor.Unsubscribe("second"), E_OK);
    EXPECT_FALSE(monitor.started_);
    EXPECT_TRUE(monitor.observers_.empty());
}

/**
 * @tc.name: GetInstanceAndRegisterInstance_Registered_ReturnsStableInstance006
 * @tc.desc: Verify the getter returns the load-time registered instance and duplicate registration
 *           is rejected without clobbering it.
 * @tc.type: FUNC
 */
HWTEST_F(ThermalStateMonitorTest, GetInstanceAndRegisterInstance_Registered_ReturnsStableInstance006,
    TestSize.Level1)
{
    ThermalStateMonitor *instance = ThermalStateMonitor::GetInstance();
    ASSERT_NE(instance, nullptr);

    ThermalStateMonitorImpl other;
    EXPECT_FALSE(ThermalStateMonitor::RegisterInstance(&other));
    EXPECT_EQ(ThermalStateMonitor::GetInstance(), instance);
}

/**
 * @tc.name: NormalizeInitialLevel_ValidLevel_ReturnsTrue007
 * @tc.desc: Verify an in-range raw level is normalized to itself.
 * @tc.type: FUNC
 */
HWTEST_F(ThermalStateMonitorTest, NormalizeInitialLevel_ValidLevel_ReturnsTrue007, TestSize.Level1)
{
    ThermalStateMonitorImpl monitor;
    int32_t level = -1;

    ASSERT_TRUE(monitor.NormalizeInitialLevel(5, level));
    EXPECT_EQ(level, 5);
}

/**
 * @tc.name: NormalizeInitialLevel_OutOfRangeLevel_ClampsToBoundary008
 * @tc.desc: Verify an out-of-range raw level is clamped to the nearest boundary.
 * @tc.type: FUNC
 */
HWTEST_F(ThermalStateMonitorTest, NormalizeInitialLevel_OutOfRangeLevel_ClampsToBoundary008, TestSize.Level1)
{
    ThermalStateMonitorImpl monitor;
    int32_t level = -1;

    ASSERT_TRUE(monitor.NormalizeInitialLevel(9, level));
    EXPECT_EQ(level, 7);

    ASSERT_TRUE(monitor.NormalizeInitialLevel(-2, level));
    EXPECT_EQ(level, 0);
}

/**
 * @tc.name: OnThermalLevelChanged_NotStartedAndNotSubscribing_ReturnsEarly009
 * @tc.desc: Verify a level change is ignored when the monitor is neither started nor subscribing.
 * @tc.type: FUNC
 */
HWTEST_F(ThermalStateMonitorTest, OnThermalLevelChanged_NotStartedAndNotSubscribing_ReturnsEarly009, TestSize.Level1)
{
    ThermalStateMonitorImpl monitor;
    int32_t count = 0;
    monitor.observers_["observer"] = [&count](const ThermalStateMonitor::Snapshot &) {
        ++count;
    };

    monitor.OnThermalLevelChanged(3);

    EXPECT_EQ(count, 0);
    EXPECT_EQ(monitor.GetSnapshot().level, 0);
}

#if defined(DATAMGR_THERMAL_PART_ENABLED)
/**
 * @tc.name: ThermalLevelCallback_OnThermalLevelChanged_ForwardsToMonitor010
 * @tc.desc: Verify the callback stub forwards a thermal level change to the owning monitor.
 * @tc.type: FUNC
 */
HWTEST_F(ThermalStateMonitorTest, ThermalLevelCallback_OnThermalLevelChanged_ForwardsToMonitor010, TestSize.Level1)
{
    ThermalStateMonitorImpl monitor;
    ThermalStateMonitorImpl::ThermalLevelCallback callback(monitor);

    EXPECT_TRUE(callback.OnThermalLevelChanged(OHOS::PowerMgr::ThermalLevel::COOL));
}

/**
 * @tc.name: PrepareSubscription_FirstObserver_AllocatesCallbackAndSetsSubscribing011
 * @tc.desc: Verify the first cold-start subscription allocates a callback and marks the subscribing state.
 * @tc.type: FUNC
 */
HWTEST_F(ThermalStateMonitorTest, PrepareSubscription_FirstObserver_AllocatesCallbackAndSetsSubscribing011,
    TestSize.Level1)
{
    ThermalStateMonitorImpl monitor;
    ThermalStateMonitor::Snapshot snapshot;
    ThermalStateMonitorImpl::SubscriptionContext context;
    uint64_t sequence = monitor.updateSequence_;
    int32_t count = 0;
    ThermalStateMonitor::Observer observer = [&count](const ThermalStateMonitor::Snapshot &) {
        ++count;
    };

    int32_t status = monitor.PrepareSubscription("observer", observer, snapshot, context);

    EXPECT_EQ(status, E_OK);
    EXPECT_NE(context.callback, nullptr);
    EXPECT_NE(monitor.callback_, nullptr);
    EXPECT_EQ(monitor.callback_, context.callback);
    EXPECT_TRUE(monitor.subscribing_);
    EXPECT_EQ(context.updateSequence, sequence);
    EXPECT_EQ(monitor.observers_.size(), 1U);
}
#endif
} // namespace OHOS::Test
