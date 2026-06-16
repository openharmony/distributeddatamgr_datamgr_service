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

#include "common_event_data.h"
#include "error/general_error.h"
#include "want.h"

#include "battery_state_monitor_impl.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;

namespace OHOS::Test {
namespace {
constexpr const char *BATTERY_CHANGED_EVENT = "usual.event.BATTERY_CHANGED";
constexpr const char *BATTERY_CAPACITY_LEVEL_EVENT = "usual.event.BATTERY_CAPACITY_LEVEL_UPDATE";
constexpr const char *BATTERY_LOW_EVENT = "usual.event.BATTERY_LOW";
constexpr const char *BATTERY_OKAY_EVENT = "usual.event.BATTERY_OKAY";

EventFwk::CommonEventData MakeBatteryEvent(const std::string &action)
{
    AAFwk::Want want;
    want.SetAction(action);
    return EventFwk::CommonEventData(want);
}

EventFwk::CommonEventData MakeBatteryLevelEvent(const std::string &action, const std::string &key, int32_t level)
{
    AAFwk::Want want;
    want.SetAction(action);
    want.SetParam(key, level);
    return EventFwk::CommonEventData(want);
}
} // namespace

class BatteryStateMonitorTest : public testing::Test {
public:
    void SetUp() override
    {
        monitor_.UnsubscribeBatteryEvent();
        monitor_.observers_.clear();
        monitor_.snapshot_ = BatteryStateMonitor::Snapshot {};
        monitor_.started_ = false;
        monitor_.subscribing_ = false;
    }

    BatteryStateMonitorImpl monitor_;
};

/**
 * @tc.name: Subscribe_InvalidArgs_ReturnsError001
 * @tc.desc: Verify Subscribe rejects empty observer names and null callbacks.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, Subscribe_InvalidArgs_ReturnsError001, TestSize.Level1)
{
    EXPECT_EQ(monitor_.Subscribe("", [](const BatteryStateMonitor::Snapshot &) {}), E_INVALID_ARGS);
    EXPECT_EQ(monitor_.Subscribe("observer", nullptr), E_INVALID_ARGS);
}

/**
 * @tc.name: Subscribe_ValidObserver_ReceivesSnapshotAndCanBeReplaced002
 * @tc.desc: Verify a valid observer receives the cached snapshot and same-name subscribe replaces the observer.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, Subscribe_ValidObserver_ReceivesSnapshotAndCanBeReplaced002, TestSize.Level1)
{
    monitor_.started_ = true;
    monitor_.snapshot_.batteryLevel = 2;
    int32_t firstCount = 0;
    int32_t secondCount = 0;
    BatteryStateMonitor::Snapshot received;

    ASSERT_EQ(monitor_.Subscribe("observer", [&firstCount](const BatteryStateMonitor::Snapshot &) {
        ++firstCount;
    }), E_OK);
    ASSERT_EQ(monitor_.Subscribe("observer", [&secondCount, &received](const BatteryStateMonitor::Snapshot &snapshot) {
        ++secondCount;
        received = snapshot;
    }), E_OK);

    EXPECT_EQ(firstCount, 1);
    EXPECT_EQ(secondCount, 1);
    EXPECT_EQ(received.batteryLevel, 2);
    EXPECT_TRUE(monitor_.started_);

    monitor_.OnBatteryEvent(MakeBatteryEvent(BATTERY_LOW_EVENT));

    EXPECT_EQ(firstCount, 1);
    EXPECT_EQ(secondCount, 2);
    EXPECT_EQ(received.batteryLevel, 4);
}

/**
 * @tc.name: Unsubscribe_ExistingAndMissingObserver_ReturnsExpectedStatus003
 * @tc.desc: Verify Unsubscribe removes existing observers and reports missing names.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, Unsubscribe_ExistingAndMissingObserver_ReturnsExpectedStatus003, TestSize.Level1)
{
    monitor_.started_ = true;
    ASSERT_EQ(monitor_.Subscribe("observer", [](const BatteryStateMonitor::Snapshot &) {}), E_OK);
    EXPECT_TRUE(monitor_.started_);

    EXPECT_EQ(monitor_.Unsubscribe("observer"), E_OK);
    EXPECT_FALSE(monitor_.started_);
    EXPECT_EQ(monitor_.Unsubscribe("observer"), E_ERROR);
}

/**
 * @tc.name: GetSnapshot_ReturnsCachedSnapshot004
 * @tc.desc: Verify GetSnapshot returns the cached battery state.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, GetSnapshot_ReturnsCachedSnapshot004, TestSize.Level1)
{
    monitor_.snapshot_.batteryLevel = 5;

    auto snapshot = monitor_.GetSnapshot();

    EXPECT_EQ(snapshot.batteryLevel, 5);
}

/**
 * @tc.name: Unsubscribe_LastObserver_StopsReceivingEvents005
 * @tc.desc: Verify removing the last observer stops battery event updates.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, Unsubscribe_LastObserver_StopsReceivingEvents005, TestSize.Level1)
{
    monitor_.started_ = true;
    int32_t callbackCount = 0;
    ASSERT_EQ(monitor_.Subscribe("observer", [&callbackCount](const BatteryStateMonitor::Snapshot &) {
        ++callbackCount;
    }), E_OK);
    ASSERT_EQ(monitor_.Unsubscribe("observer"), E_OK);

    monitor_.OnBatteryEvent(MakeBatteryEvent(BATTERY_LOW_EVENT));

    EXPECT_EQ(callbackCount, 1);
    EXPECT_EQ(monitor_.GetSnapshot().batteryLevel, 0);
    EXPECT_FALSE(monitor_.started_);
}

/**
 * @tc.name: OnBatteryEvent_LowAndOkay_UpdateSnapshot006
 * @tc.desc: Verify BATTERY_LOW and BATTERY_OKAY events map to expected battery levels.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, OnBatteryEvent_LowAndOkay_UpdateSnapshot006, TestSize.Level1)
{
    monitor_.started_ = true;
    int32_t callbackCount = 0;
    BatteryStateMonitor::Snapshot received;
    ASSERT_EQ(monitor_.Subscribe("observer", [&callbackCount, &received](
        const BatteryStateMonitor::Snapshot &snapshot) {
        ++callbackCount;
        received = snapshot;
    }), E_OK);

    monitor_.OnBatteryEvent(MakeBatteryEvent(BATTERY_LOW_EVENT));
    EXPECT_EQ(callbackCount, 2);
    EXPECT_EQ(received.batteryLevel, 4);
    EXPECT_EQ(monitor_.GetSnapshot().batteryLevel, 4);

    monitor_.OnBatteryEvent(MakeBatteryEvent(BATTERY_OKAY_EVENT));
    EXPECT_EQ(callbackCount, 3);
    EXPECT_EQ(received.batteryLevel, 3);
    EXPECT_EQ(monitor_.GetSnapshot().batteryLevel, 3);
}

/**
 * @tc.name: OnBatteryEvent_LevelEvents_ParseKnownKeysAndClamp007
 * @tc.desc: Verify capacity events parse known keys and clamp battery levels to the valid range.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, OnBatteryEvent_LevelEvents_ParseKnownKeysAndClamp007, TestSize.Level1)
{
    monitor_.started_ = true;
    int32_t callbackCount = 0;
    BatteryStateMonitor::Snapshot received;
    ASSERT_EQ(monitor_.Subscribe("observer", [&callbackCount, &received](
        const BatteryStateMonitor::Snapshot &snapshot) {
        ++callbackCount;
        received = snapshot;
    }), E_OK);

    monitor_.OnBatteryEvent(MakeBatteryLevelEvent(BATTERY_CHANGED_EVENT, "batteryCapacityLevel", 6));
    EXPECT_EQ(callbackCount, 2);
    EXPECT_EQ(received.batteryLevel, 6);

    monitor_.OnBatteryEvent(MakeBatteryLevelEvent(BATTERY_CAPACITY_LEVEL_EVENT, "capacityLevel", 9));
    EXPECT_EQ(callbackCount, 3);
    EXPECT_EQ(received.batteryLevel, 7);

    monitor_.OnBatteryEvent(MakeBatteryLevelEvent(BATTERY_CHANGED_EVENT, "batteryLevel", -2));
    EXPECT_EQ(callbackCount, 4);
    EXPECT_EQ(received.batteryLevel, 0);

    monitor_.OnBatteryEvent(MakeBatteryLevelEvent(BATTERY_CHANGED_EVENT, "level", 5));
    EXPECT_EQ(callbackCount, 5);
    EXPECT_EQ(received.batteryLevel, 5);
}

/**
 * @tc.name: OnBatteryEvent_UnknownActionOrDuplicateLevel_DoesNotNotify008
 * @tc.desc: Verify unknown events and repeated levels do not notify observers.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, OnBatteryEvent_UnknownActionOrDuplicateLevel_DoesNotNotify008, TestSize.Level1)
{
    monitor_.started_ = true;
    monitor_.snapshot_.batteryLevel = 4;
    int32_t callbackCount = 0;
    ASSERT_EQ(monitor_.Subscribe("observer", [&callbackCount](const BatteryStateMonitor::Snapshot &) {
        ++callbackCount;
    }), E_OK);

    monitor_.OnBatteryEvent(MakeBatteryEvent("usual.event.UNKNOWN"));
    EXPECT_EQ(callbackCount, 1);
    EXPECT_EQ(monitor_.GetSnapshot().batteryLevel, 4);

    monitor_.OnBatteryEvent(MakeBatteryEvent(BATTERY_LOW_EVENT));
    EXPECT_EQ(callbackCount, 1);
    EXPECT_EQ(monitor_.GetSnapshot().batteryLevel, 4);
}
} // namespace OHOS::Test
