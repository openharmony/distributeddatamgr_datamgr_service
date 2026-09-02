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

} // namespace OHOS::Test
