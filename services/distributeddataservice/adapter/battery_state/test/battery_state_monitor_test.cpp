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
        monitor_.stateVersion_ = 0;
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
 * @tc.desc: Verify capacity events parse known keys and clamp out-of-range levels to the nearest boundary.
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

/**
 * @tc.name: ApplyInitialLevel_EventArrivesDuringQuery_PreservesEventSnapshot009
 * @tc.desc: Verify a stale initial query cannot overwrite a newer CommonEvent sample.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, ApplyInitialLevel_EventArrivesDuringQuery_PreservesEventSnapshot009,
    TestSize.Level1)
{
    monitor_.started_ = true;
    BatteryStateMonitor::Snapshot snapshot;

    ASSERT_TRUE(monitor_.UpdateBatteryLevel(4, snapshot));
    EXPECT_FALSE(monitor_.ApplyInitialLevel(2, 0, snapshot));
    EXPECT_EQ(monitor_.GetSnapshot().batteryLevel, 4);
}

/**
 * @tc.name: ApplyInitialLevel_NoNewEvent_CachesQueriedSnapshot010
 * @tc.desc: Verify a valid initial capacity query populates the cache when no event races with it.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, ApplyInitialLevel_NoNewEvent_CachesQueriedSnapshot010, TestSize.Level1)
{
    monitor_.started_ = true;
    BatteryStateMonitor::Snapshot snapshot;

    ASSERT_TRUE(monitor_.ApplyInitialLevel(2, 0, snapshot));
    EXPECT_EQ(snapshot.batteryLevel, 2);
}

/**
 * @tc.name: GetInstanceAndRegisterInstance_Registered_ReturnsStableInstance011
 * @tc.desc: Verify the battery getter returns the load-time instance and duplicate registration is rejected.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, GetInstanceAndRegisterInstance_Registered_ReturnsStableInstance011, TestSize.Level1)
{
    BatteryStateMonitor *instance = BatteryStateMonitor::GetInstance();
    ASSERT_NE(instance, nullptr);

    BatteryStateMonitorImpl other;
    EXPECT_FALSE(BatteryStateMonitor::RegisterInstance(&other));
    EXPECT_EQ(BatteryStateMonitor::GetInstance(), instance);
}

/**
 * @tc.name: PrepareSubscription_FirstObserver_AllocatesSubscriber012
 * @tc.desc: Verify the first cold-start subscription allocates the event subscriber and marks the subscribing state.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, PrepareSubscription_FirstObserver_AllocatesSubscriber012, TestSize.Level1)
{
    BatteryStateMonitor::Snapshot snapshot;
    std::shared_ptr<BatteryStateEventSubscriber> subscriber;
    uint64_t stateVersion = 42;
    int32_t count = 0;
    BatteryStateMonitor::Observer observer = [&count](const BatteryStateMonitor::Snapshot &) {
        ++count;
    };

    monitor_.PrepareSubscription("observer", observer, subscriber, stateVersion, snapshot);

    EXPECT_NE(subscriber, nullptr);
    EXPECT_EQ(monitor_.batterySubscriber_, subscriber);
    EXPECT_TRUE(monitor_.subscribing_);
    EXPECT_EQ(stateVersion, monitor_.stateVersion_);
    EXPECT_EQ(snapshot.batteryLevel, monitor_.snapshot_.batteryLevel);
    EXPECT_EQ(monitor_.observers_.size(), 1U);
}

/**
 * @tc.name: GetSubscriberLocked_RepeatedCalls_ReusesExistingSubscriber013
 * @tc.desc: Verify the lazily created battery subscriber is reused on subsequent lookups.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, GetSubscriberLocked_RepeatedCalls_ReusesExistingSubscriber013, TestSize.Level1)
{
    auto first = monitor_.GetSubscriberLocked();
    ASSERT_NE(first, nullptr);

    auto second = monitor_.GetSubscriberLocked();

    EXPECT_EQ(first, second);
    EXPECT_EQ(monitor_.batterySubscriber_, first);
}

/**
 * @tc.name: ApplyInitialLevel_NotStarted_ReturnsFalse014
 * @tc.desc: Verify the initial capacity query is rejected before the monitor is started.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, ApplyInitialLevel_NotStarted_ReturnsFalse014, TestSize.Level1)
{
    BatteryStateMonitor::Snapshot snapshot;

    EXPECT_FALSE(monitor_.ApplyInitialLevel(3, 0, snapshot));
    EXPECT_EQ(snapshot.batteryLevel, 0);
}

/**
 * @tc.name: ApplyInitialLevel_OutOfRangeLevel_ClampsToBoundary015
 * @tc.desc: Verify an out-of-range initial capacity query is clamped to the nearest boundary.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, ApplyInitialLevel_OutOfRangeLevel_ClampsToBoundary015, TestSize.Level1)
{
    monitor_.started_ = true;
    BatteryStateMonitor::Snapshot snapshot;

    ASSERT_TRUE(monitor_.ApplyInitialLevel(9, 0, snapshot));
    EXPECT_EQ(snapshot.batteryLevel, 7);

    ASSERT_TRUE(monitor_.ApplyInitialLevel(-2, 0, snapshot));
    EXPECT_EQ(snapshot.batteryLevel, 0);
}

/**
 * @tc.name: UpdateBatteryLevel_OutOfRangeLevel_ClampsToBoundary016
 * @tc.desc: Verify an out-of-range event level is clamped before being stored.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, UpdateBatteryLevel_OutOfRangeLevel_ClampsToBoundary016, TestSize.Level1)
{
    monitor_.started_ = true;
    BatteryStateMonitor::Snapshot snapshot;

    ASSERT_TRUE(monitor_.UpdateBatteryLevel(9, snapshot));
    EXPECT_EQ(snapshot.batteryLevel, 7);

    ASSERT_TRUE(monitor_.UpdateBatteryLevel(-2, snapshot));
    EXPECT_EQ(snapshot.batteryLevel, 0);
}

/**
 * @tc.name: UpdateBatteryLevel_NotStartedAndNotSubscribing_ReturnsFalse017
 * @tc.desc: Verify a level update is ignored when the monitor is neither started nor subscribing.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, UpdateBatteryLevel_NotStartedAndNotSubscribing_ReturnsFalse017, TestSize.Level1)
{
    BatteryStateMonitor::Snapshot snapshot;

    EXPECT_FALSE(monitor_.UpdateBatteryLevel(5, snapshot));
    EXPECT_EQ(snapshot.batteryLevel, 0);
}

/**
 * @tc.name: UpdateBatteryLevel_NotFromEvent_SkipsVersionIncrement018
 * @tc.desc: Verify a non-event update does not advance the state version.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, UpdateBatteryLevel_NotFromEvent_SkipsVersionIncrement018, TestSize.Level1)
{
    monitor_.started_ = true;
    uint64_t version = monitor_.stateVersion_;
    BatteryStateMonitor::Snapshot snapshot;

    EXPECT_TRUE(monitor_.UpdateBatteryLevel(5, snapshot, false));
    EXPECT_EQ(snapshot.batteryLevel, 5);
    EXPECT_EQ(monitor_.stateVersion_, version);

    EXPECT_FALSE(monitor_.UpdateBatteryLevel(5, snapshot, false));
    EXPECT_EQ(monitor_.stateVersion_, version);
}

/**
 * @tc.name: UnsubscribeBatteryEvent_StartedWithoutSubscriber_ResetsStarted019
 * @tc.desc: Verify unsubscribing a started monitor without a live subscriber only clears the started state.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, UnsubscribeBatteryEvent_StartedWithoutSubscriber_ResetsStarted019, TestSize.Level1)
{
    monitor_.started_ = true;

    monitor_.UnsubscribeBatteryEvent();

    EXPECT_FALSE(monitor_.started_);
}

/**
 * @tc.name: OnBatteryEvent_ChangedWithoutLevelParam_KeepsSnapshot020
 * @tc.desc: Verify a changed event carrying no level param falls through to an invalid level without notifying.
 * @tc.type: FUNC
 */
HWTEST_F(BatteryStateMonitorTest, OnBatteryEvent_ChangedWithoutLevelParam_KeepsSnapshot020, TestSize.Level1)
{
    monitor_.started_ = true;
    monitor_.snapshot_.batteryLevel = 2;
    int32_t count = 0;
    ASSERT_EQ(monitor_.Subscribe("observer", [&count](const BatteryStateMonitor::Snapshot &) {
        ++count;
    }), E_OK);

    monitor_.OnBatteryEvent(MakeBatteryEvent(BATTERY_CHANGED_EVENT));

    EXPECT_EQ(count, 1);
    EXPECT_EQ(monitor_.GetSnapshot().batteryLevel, 2);
}
} // namespace OHOS::Test
