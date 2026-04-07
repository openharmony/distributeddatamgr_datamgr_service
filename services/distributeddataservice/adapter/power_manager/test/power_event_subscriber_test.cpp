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

#define LOG_TAG "PowerEventSubscriberTest"
#include <gtest/gtest.h>
#include <memory>

#include "common_event_data.h"
#include "common_event_support.h"
#include "power_manager_impl.h"
#include "want.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::EventFwk;

namespace OHOS::Test {
class PowerEventSubscriberTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}

    static void TearDownTestCase(void) {}

    void SetUp() {}

    void TearDown() {}

    // Helper to create subscriber
    std::shared_ptr<PowerEventSubscriber> CreateSubscriber()
    {
        MatchingSkills matchingSkills;
        matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_CHARGING);
        matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_DISCHARGING);
        CommonEventSubscribeInfo info(matchingSkills);
        return std::make_shared<PowerEventSubscriber>(info);
    }

    // Helper to create CommonEventData
    CommonEventData CreateEventData(const std::string &action)
    {
        Want want;
        want.SetAction(action);
        return CommonEventData(want);
    }
};


/**
 * @tc.name: OnReceiveEvent_MultipleEvents_AllCallbacksInvoked
 * @tc.desc: test OnReceiveEvent with multiple events
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PowerEventSubscriberTest, OnReceiveEvent_MultipleEvents_AllCallbacksInvoked, TestSize.Level1)
{
    auto subscriber = CreateSubscriber();

    int callCount = 0;
    PowerManger::Observer::PowerEvent lastEvent = PowerManger::Observer::PowerEvent::BUTT;

    auto callback = [&callCount, &lastEvent](PowerManger::Observer::PowerEvent event) {
        callCount++;
        lastEvent = event;
    };

    subscriber->SetEventCallback(callback);

    // First event
    auto chargingData = CreateEventData(CommonEventSupport::COMMON_EVENT_CHARGING);
    subscriber->OnReceiveEvent(chargingData);
    ASSERT_EQ(callCount, 1);
    ASSERT_EQ(lastEvent, PowerManger::Observer::PowerEvent::CHARGING);

    // Second event
    auto dischargingData = CreateEventData(CommonEventSupport::COMMON_EVENT_DISCHARGING);
    subscriber->OnReceiveEvent(dischargingData);
    ASSERT_EQ(callCount, 2);
    ASSERT_EQ(lastEvent, PowerManger::Observer::PowerEvent::DIS_CHARGING);
}
} // namespace OHOS::Test
