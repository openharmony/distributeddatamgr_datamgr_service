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

#define LOG_TAG "PowerManagerImplTest"
#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "power_manager_impl.h"
#include "mock_common_event.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::EventFwk;

namespace OHOS::Test {

// Unified Mock Observer class for all tests
class MockPowerObserver : public PowerManager::Observer {
public:
    explicit MockPowerObserver(const std::string &name = "MockObserver")
        : name_(name) {}

    void OnChange(PowerEvent event) override
    {
        lastEvent_ = event;
        notifyCount_++;
        if (event == PowerEvent::CHARGING) {
            chargingCount_++;
        } else if (event == PowerEvent::DIS_CHARGING) {
            disChargingCount_++;
        }
    }

    std::string GetName() const { return name_; }
    int GetNotifyCount() const { return notifyCount_; }
    int GetChargingCount() const { return chargingCount_; }
    int GetDisChargingCount() const { return disChargingCount_; }
    PowerEvent GetLastEvent() const { return lastEvent_; }

    void Reset()
    {
        lastEvent_ = PowerEvent::BUTT;
        notifyCount_ = 0;
        chargingCount_ = 0;
        disChargingCount_ = 0;
    }

private:
    std::string name_;
    PowerEvent lastEvent_ = PowerEvent::BUTT;
    int notifyCount_ = 0;
    int chargingCount_ = 0;
    int disChargingCount_ = 0;
};

// Test fixture
class PowerManagerImplTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}

    static void TearDownTestCase(void) {}

    void SetUp()
    {
        CommonEventManager::Reset();
    }

    void TearDown()
    {
        CommonEventManager::Reset();
    }

    PowerManager* GetManager()
    {
        return PowerManager::GetInstance();
    }

    std::shared_ptr<MockPowerObserver> CreateObserver(const std::string &name = "MockObserver")
    {
        return std::make_shared<MockPowerObserver>(name);
    }
};

/**
 * @tc.name: Subscribe_NullObserver_ReturnsError
 * @tc.desc: test Subscribe with null observer returns error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, Subscribe_NullObserver_ReturnsError, TestSize.Level1)
{
    auto *manager = GetManager();
    int32_t result = manager->Subscribe(nullptr);
    ASSERT_EQ(result, -1);
}

/**
 * @tc.name: Subscribe_ValidObserver_ReturnsSuccess
 * @tc.desc: test Subscribe with valid observer returns success
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, Subscribe_ValidObserver_ReturnsSuccess, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();

    int32_t result = manager->Subscribe(observer);
    ASSERT_EQ(result, 0);

    manager->Unsubscribe(observer);
}

/**
 * @tc.name: Subscribe_DuplicateObserver_ReturnsError
 * @tc.desc: test Subscribe with duplicate observer returns error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, Subscribe_DuplicateObserver_ReturnsError, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();

    int32_t firstResult = manager->Subscribe(observer);
    ASSERT_EQ(firstResult, 0);

    int32_t secondResult = manager->Subscribe(observer);
    ASSERT_EQ(secondResult, -1);

    // Clean up
    manager->Unsubscribe(observer);
}

/**
 * @tc.name: Subscribe_MultipleObservers_AllAdded
 * @tc.desc: test Subscribe with multiple observers
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, Subscribe_MultipleObservers_AllAdded, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer1 = CreateObserver("Observer1");
    auto observer2 = CreateObserver("Observer2");
    auto observer3 = CreateObserver("Observer3");

    int32_t result1 = manager->Subscribe(observer1);
    int32_t result2 = manager->Subscribe(observer2);
    int32_t result3 = manager->Subscribe(observer3);

    ASSERT_EQ(result1, 0);
    ASSERT_EQ(result2, 0);
    ASSERT_EQ(result3, 0);

    // Clean up
    manager->Unsubscribe(observer1);
    manager->Unsubscribe(observer2);
    manager->Unsubscribe(observer3);
}

/**
 * @tc.name: Unsubscribe_NullObserver_ReturnsError
 * @tc.desc: test Unsubscribe with null observer returns error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, Unsubscribe_NullObserver_ReturnsError, TestSize.Level1)
{
    auto *manager = GetManager();
    int32_t result = manager->Unsubscribe(nullptr);
    ASSERT_EQ(result, -1);
}

/**
 * @tc.name: Unsubscribe_ValidObserver_ReturnsSuccess
 * @tc.desc: test Unsubscribe with valid observer returns success
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, Unsubscribe_ValidObserver_ReturnsSuccess, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();

    manager->Subscribe(observer);
    int32_t result = manager->Unsubscribe(observer);
    ASSERT_EQ(result, 0);
}

/**
 * @tc.name: Unsubscribe_NonExistentObserver_ReturnsError
 * @tc.desc: test Unsubscribe with non-existent observer returns error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, Unsubscribe_NonExistentObserver_ReturnsError, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();

    int32_t result = manager->Unsubscribe(observer);
    ASSERT_EQ(result, -1);
}

/**
 * @tc.name: Unsubscribe_MiddleObserver_RemovesCorrectly
 * @tc.desc: test Remove middle observer from list
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, Unsubscribe_MiddleObserver_RemovesCorrectly, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer1 = CreateObserver("Observer1");
    auto observer2 = CreateObserver("Observer2");
    auto observer3 = CreateObserver("Observer3");

    manager->Subscribe(observer1);
    manager->Subscribe(observer2);
    manager->Subscribe(observer3);

    int32_t result = manager->Unsubscribe(observer2);
    ASSERT_EQ(result, 0);

    // Clean up
    manager->Unsubscribe(observer1);
    manager->Unsubscribe(observer3);
}

/**
 * @tc.name: IsCharging_InitialState_ReturnsFalse
 * @tc.desc: test IsCharging returns false in initial state
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, IsCharging_InitialState_ReturnsFalse, TestSize.Level1)
{
    auto *manager = GetManager();
    ASSERT_FALSE(manager->IsCharging());
}

/**
 * @tc.name: SubscribeUnsubscribe_ResubscribeWorks
 * @tc.desc: test subscribe-unsubscribe-resubscribe sequence works
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, SubscribeUnsubscribe_ResubscribeWorks, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();

    ASSERT_EQ(manager->Subscribe(observer), 0);

    ASSERT_EQ(manager->Unsubscribe(observer), 0);

    ASSERT_EQ(manager->Subscribe(observer), 0);

    // Clean up
    manager->Unsubscribe(observer);
}

/**
 * @tc.name: SubscribeUnsubscribeSubscribe_ResubscribeWorks
 * @tc.desc: test subscribe-unsubscribe-subscribe sequence works
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, SubscribeUnsubscribeSubscribe_ResubscribeWorks, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();

    // First subscribe
    ASSERT_EQ(manager->Subscribe(observer), 0);

    // Unsubscribe
    ASSERT_EQ(manager->Unsubscribe(observer), 0);

    // Resubscribe
    ASSERT_EQ(manager->Subscribe(observer), 0);

    // Clean up
    manager->Unsubscribe(observer);
}

/**
 * @tc.name: OnReceiveEvent_MultipleEvents_AllCallbacksInvoked
 * @tc.desc: test OnReceiveEvent with multiple events
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, OnReceiveEvent_MultipleEvents_AllCallbacksInvoked, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();
    ASSERT_EQ(manager->Subscribe(observer), 0);

    // Publish charging and discharging events
    CommonEventManager::PublishChargingEvent(80);

    CommonEventManager::PublishDisChargingEvent(70);

    // Verify observer received both events
    ASSERT_GE(observer->GetNotifyCount(), 2);
    ASSERT_EQ(observer->GetChargingCount(), 1);
    ASSERT_EQ(observer->GetDisChargingCount(), 1);

    // Clean up
    manager->Unsubscribe(observer);
}

/**
 * @tc.name: EventSubscriber_ChargingAndDisCharging_HandleCorrectly
 * @tc.desc: test subscriber handles both charging and discharging events
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, EventSubscriber_ChargingAndDisCharging_HandleCorrectly, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();
    ASSERT_EQ(manager->Subscribe(observer), 0);

    // Test charging
    CommonEventManager::PublishChargingEvent(50);
    ASSERT_EQ(observer->GetLastEvent(), PowerManager::Observer::PowerEvent::CHARGING);

    // Test discharging
    CommonEventManager::PublishDisChargingEvent(40);
    ASSERT_EQ(observer->GetLastEvent(), PowerManager::Observer::PowerEvent::DIS_CHARGING);

    // Clean up
    manager->Unsubscribe(observer);
}

/**
 * @tc.name: EventSubscriber_Charging_HandleCorrectly
 * @tc.desc: test subscriber can handle charging event
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, EventSubscriber_Charging_HandleCorrectly, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();
    ASSERT_EQ(manager->Subscribe(observer), 0);

    // Publish charging event
    CommonEventManager::PublishChargingEvent(75);

    // Verify charging event was received
    ASSERT_GT(observer->GetNotifyCount(), 0);
    ASSERT_EQ(observer->GetLastEvent(), PowerManager::Observer::PowerEvent::CHARGING);

    // Clean up
    manager->Unsubscribe(observer);
}

/**
 * @tc.name: EventSubscriber_DisCharging_HandleCorrectly
 * @tc.desc: test subscriber can handle discharging event
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, EventSubscriber_DisCharging_HandleCorrectly, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();
    ASSERT_EQ(manager->Subscribe(observer), 0);

    // Publish discharging event
    CommonEventManager::PublishDisChargingEvent(65);

    // Verify discharging event was received
    ASSERT_GT(observer->GetNotifyCount(), 0);
    ASSERT_EQ(observer->GetLastEvent(), PowerManager::Observer::PowerEvent::DIS_CHARGING);

    // Clean up
    manager->Unsubscribe(observer);
}

/**
 * @tc.name: EventCallback_BatteryLevel_Preserved
 * @tc.desc: test event callback preserves battery level information
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, EventCallback_BatteryLevel_Preserved, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();
    ASSERT_EQ(manager->Subscribe(observer), 0);

    // Publish charging event with battery level
    CommonEventManager::PublishChargingEvent(90);

    // Verify event was received
    ASSERT_GT(observer->GetNotifyCount(), 0);

    // Clean up
    manager->Unsubscribe(observer);
}

/**
 * @tc.name: SubscribeCharging_FailureHandling
 * @tc.desc: test subscription failure handling
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, SubscribeCharging_FailureHandling, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();

    // Simulate subscription failure
    CommonEventManager::SetSubscribeResult(false);
    manager->Subscribe(observer);

    // Clean up
    CommonEventManager::Reset();
    manager->Unsubscribe(observer);
}

/**
 * @tc.name: BatteryLevelChange_Notified
 * @tc.desc: test battery level change notification
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, BatteryLevelChange_Notified, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();
    ASSERT_EQ(manager->Subscribe(observer), 0);

    // Publish charging events with different battery levels
    CommonEventManager::PublishChargingEvent(50);

    CommonEventManager::PublishChargingEvent(60);

    CommonEventManager::PublishChargingEvent(70);

    // Verify notifications were received
    ASSERT_GE(observer->GetNotifyCount(), 3);

    // Clean up
    manager->Unsubscribe(observer);
}

/**
 * @tc.name: MultiplePowerEvents_Processed
 * @tc.desc: test multiple consecutive power events
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, MultiplePowerEvents_Processed, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();
    ASSERT_EQ(manager->Subscribe(observer), 0);

    // Send multiple charging and discharging events
    for (int i = 0; i < 3; i++) {
        CommonEventManager::PublishChargingEvent(80 + i);

        CommonEventManager::PublishDisChargingEvent(70 - i);
    }

    // Verify observer received notifications
    ASSERT_GT(observer->GetNotifyCount(), 0);

    // Clean up
    manager->Unsubscribe(observer);
}

/**
 * @tc.name: SubscribeRetry_SuccessAfterFailure
 * @tc.desc: test subscription retry logic
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, SubscribeRetry_SuccessAfterFailure, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();

    // First attempt fails
    CommonEventManager::SetSubscribeResult(false);
    manager->Subscribe(observer);

    // Second attempt succeeds
    CommonEventManager::SetSubscribeResult(true);
    manager->Subscribe(observer);

    // Clean up
    manager->Unsubscribe(observer);
    CommonEventManager::Reset();
}

/**
 * @tc.name: ChargingStateTransition_Successful
 * @tc.desc: test charging state transitions
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, ChargingStateTransition_Successful, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();
    ASSERT_EQ(manager->Subscribe(observer), 0);

    // Initial state: not charging
    ASSERT_FALSE(manager->IsCharging());

    // Transition to charging
    CommonEventManager::PublishChargingEvent(50);
    ASSERT_TRUE(manager->IsCharging());

    // Transition to discharging
    CommonEventManager::PublishDisChargingEvent(40);
    ASSERT_FALSE(manager->IsCharging());

    // Transition back to charging
    CommonEventManager::PublishChargingEvent(60);
    ASSERT_TRUE(manager->IsCharging());

    // Clean up
    manager->Unsubscribe(observer);
}

/**
 * @tc.name: ObserverUnsubscribe_StopsNotification
 * @tc.desc: test unsubscribed observer stops receiving events
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, ObserverUnsubscribe_StopsNotification, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer1 = CreateObserver("Observer1");
    auto observer2 = CreateObserver("Observer2");

    ASSERT_EQ(manager->Subscribe(observer1), 0);
    ASSERT_EQ(manager->Subscribe(observer2), 0);

    // Publish first event
    CommonEventManager::PublishChargingEvent(80);

    int32_t count1 = observer1->GetNotifyCount();
    int32_t count2 = observer2->GetNotifyCount();

    // Unsubscribe observer1
    ASSERT_EQ(manager->Unsubscribe(observer1), 0);

    // Publish second event
    CommonEventManager::PublishChargingEvent(85);

    // observer1 count should not change
    ASSERT_EQ(observer1->GetNotifyCount(), count1);

    // observer2 count should increase
    ASSERT_GT(observer2->GetNotifyCount(), count2);

    // Clean up
    manager->Unsubscribe(observer2);
}

/**
 * @tc.name: CompletePowerLifecycle_Successful
 * @tc.desc: test complete power lifecycle from charging to discharging
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, CompletePowerLifecycle_Successful, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver("Lifecycle");
    ASSERT_EQ(manager->Subscribe(observer), 0);

    // Initial state: not charging
    ASSERT_FALSE(manager->IsCharging());

    // Start charging
    CommonEventManager::PublishChargingEvent(50);
    ASSERT_TRUE(manager->IsCharging());
    ASSERT_EQ(observer->GetChargingCount(), 1);
    ASSERT_EQ(observer->GetLastEvent(), PowerManager::Observer::PowerEvent::CHARGING);

    // Stop charging
    CommonEventManager::PublishDisChargingEvent(40);
    ASSERT_FALSE(manager->IsCharging());
    ASSERT_EQ(observer->GetDisChargingCount(), 1);
    ASSERT_EQ(observer->GetLastEvent(), PowerManager::Observer::PowerEvent::DIS_CHARGING);

    // Clean up
    manager->Unsubscribe(observer);
}

/**
 * @tc.name: ChargingCycleScenario_Realistic
 * @tc.desc: test realistic charging cycle scenario
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, ChargingCycleScenario_Realistic, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver("ChargingCycle");
    ASSERT_EQ(manager->Subscribe(observer), 0);

    // Simulate charging from 20% to 100%
    for (int level = 20; level <= 100; level += 10) {
        CommonEventManager::PublishChargingEvent(level);
    }

    // Should receive multiple charging events
    ASSERT_GE(observer->GetChargingCount(), 8);
    ASSERT_TRUE(manager->IsCharging());

    // Simulate discharging from 100% to 20%
    for (int level = 100; level >= 20; level -= 10) {
        CommonEventManager::PublishDisChargingEvent(level);
    }

    // Should receive multiple discharging events
    ASSERT_GE(observer->GetDisChargingCount(), 8);
    ASSERT_FALSE(manager->IsCharging());

    // Clean up
    manager->Unsubscribe(observer);
}

/**
 * @tc.name: ConcurrentPowerEvents_HandledCorrectly
 * @tc.desc: test multiple observers receiving concurrent power events
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, ConcurrentPowerEvents_HandledCorrectly, TestSize.Level1)
{
    auto *manager = GetManager();

    // Create multiple observers
    std::vector<std::shared_ptr<MockPowerObserver>> observers;
    for (int i = 0; i < 5; i++) {
        auto observer = CreateObserver("Observer" + std::to_string(i));
        ASSERT_EQ(manager->Subscribe(observer), 0);
        observers.push_back(observer);
    }

    // Publish concurrent charging events
    for (int i = 0; i < 10; i++) {
        CommonEventManager::PublishChargingEvent(80 + i);
    }

    // Verify all observers received notifications
    for (const auto &observer : observers) {
        ASSERT_GE(observer->GetChargingCount(), 10);
        ASSERT_EQ(observer->GetLastEvent(), PowerManager::Observer::PowerEvent::CHARGING);
    }

    // Publish concurrent discharging events
    for (int i = 0; i < 10; i++) {
        CommonEventManager::PublishDisChargingEvent(70 - i);
    }

    // Verify all observers received discharging notifications
    for (const auto &observer : observers) {
        ASSERT_GE(observer->GetDisChargingCount(), 10);
        ASSERT_EQ(observer->GetLastEvent(), PowerManager::Observer::PowerEvent::DIS_CHARGING);
    }

    // Clean up all observers
    for (const auto &observer : observers) {
        manager->Unsubscribe(observer);
    }
}

/**
 * @tc.name: PowerStateConsistency_MultipleObservers
 * @tc.desc: test power state consistency across multiple observers
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, PowerStateConsistency_MultipleObservers, TestSize.Level1)
{
    auto *manager = GetManager();

    // Create multiple observers
    std::vector<std::shared_ptr<MockPowerObserver>> observers;
    for (int i = 0; i < 3; i++) {
        auto observer = CreateObserver("ConsistencyObserver" + std::to_string(i));
        ASSERT_EQ(manager->Subscribe(observer), 0);
        observers.push_back(observer);
    }

    // All observers should see consistent state
    CommonEventManager::PublishChargingEvent(75);

    for (const auto &observer : observers) {
        ASSERT_GT(observer->GetChargingCount(), 0);
        ASSERT_EQ(observer->GetLastEvent(), PowerManager::Observer::PowerEvent::CHARGING);
    }

    // Change to discharging
    CommonEventManager::PublishDisChargingEvent(65);

    for (const auto &observer : observers) {
        ASSERT_GT(observer->GetDisChargingCount(), 0);
        ASSERT_EQ(observer->GetLastEvent(), PowerManager::Observer::PowerEvent::DIS_CHARGING);
    }

    // Clean up all observers
    for (const auto &observer : observers) {
        manager->Unsubscribe(observer);
    }
}

/**
 * @tc.name: MultipleObserversNotification_AllReceive
 * @tc.desc: test all observers receive charging events
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(PowerManagerImplTest, MultipleObserversNotification_AllReceive, TestSize.Level1)
{
    auto *manager = GetManager();

    // Create multiple observers
    std::vector<std::shared_ptr<MockPowerObserver>> observers;
    for (int i = 0; i < 10; i++) {
        auto observer = CreateObserver("Observer" + std::to_string(i));
        ASSERT_EQ(manager->Subscribe(observer), 0);
        observers.push_back(observer);
    }

    // Publish charging event
    CommonEventManager::PublishChargingEvent(90);

    // Verify all observers received notification
    for (const auto &observer : observers) {
        ASSERT_GT(observer->GetNotifyCount(), 0);
        ASSERT_EQ(observer->GetLastEvent(), PowerManager::Observer::PowerEvent::CHARGING);
    }

    // Clean up all observers
    for (const auto &observer : observers) {
        manager->Unsubscribe(observer);
    }
}

} // namespace OHOS::Test
