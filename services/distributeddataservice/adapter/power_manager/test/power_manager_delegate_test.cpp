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

#define LOG_TAG "PowerManagerDelegateTest"
#include <gtest/gtest.h>
#include <memory>

#include "power_manager_impl.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;

namespace OHOS::Test {
// Mock Observer for testing
class MockPowerObserver : public PowerManger::Observer {
public:
    PowerManger::Observer::PowerEvent lastEvent = PowerManger::Observer::PowerEvent::BUTT;
    int notifyCount = 0;

    void OnChang(PowerEvent event) override
    {
        lastEvent = event;
        notifyCount++;
    }

    void Reset()
    {
        lastEvent = PowerManger::Observer::PowerEvent::BUTT;
        notifyCount = 0;
    }
};

class PowerManagerDelegateTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}

    static void TearDownTestCase(void) {}

    void SetUp() {}

    void TearDown() {}

    // Helper to create observer
    std::shared_ptr<MockPowerObserver> CreateObserver()
    {
        return std::make_shared<MockPowerObserver>();
    }

    // Helper to get manager instance
    PowerManger* GetManager()
    {
        return PowerManger::GetInstance();
    }
};

/**
 * @tc.name: Subscribe_NullObserver_ReturnsError
 * @tc.desc: test Subscribe with null observer returns error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PowerManagerDelegateTest, Subscribe_NullObserver_ReturnsError, TestSize.Level1)
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
 * @tc.author:
 */
HWTEST_F(PowerManagerDelegateTest, Subscribe_ValidObserver_ReturnsSuccess, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();

    int32_t result = manager->Subscribe(observer);
    ASSERT_EQ(result, 0);

    // Clean up
    manager->Unsubscribe(observer);
}

/**
 * @tc.name: Subscribe_DuplicateObserver_ReturnsError
 * @tc.desc: test Subscribe with duplicate observer returns error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PowerManagerDelegateTest, Subscribe_DuplicateObserver_ReturnsError, TestSize.Level1)
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
 * @tc.author:
 */
HWTEST_F(PowerManagerDelegateTest, Subscribe_MultipleObservers_AllAdded, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer1 = CreateObserver();
    auto observer2 = CreateObserver();
    auto observer3 = CreateObserver();

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
 * @tc.author:
 */
HWTEST_F(PowerManagerDelegateTest, Unsubscribe_NullObserver_ReturnsError, TestSize.Level1)
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
 * @tc.author:
 */
HWTEST_F(PowerManagerDelegateTest, Unsubscribe_ValidObserver_ReturnsSuccess, TestSize.Level1)
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
 * @tc.author:
 */
HWTEST_F(PowerManagerDelegateTest, Unsubscribe_NonExistentObserver_ReturnsError, TestSize.Level1)
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
 * @tc.author:
 */
HWTEST_F(PowerManagerDelegateTest, Unsubscribe_MiddleObserver_RemovesCorrectly, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer1 = CreateObserver();
    auto observer2 = CreateObserver();
    auto observer3 = CreateObserver();

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
 * @tc.author:
 */
HWTEST_F(PowerManagerDelegateTest, IsCharging_InitialState_ReturnsFalse, TestSize.Level1)
{
    auto *manager = GetManager();
    ASSERT_FALSE(manager->IsCharging());
}

/**
 * @tc.name: SubscribeUnsubscribe_ResubscribeWorks
 * @tc.desc: test subscribe-unsubscribe-resubscribe sequence works
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PowerManagerDelegateTest, SubscribeUnsubscribe_ResubscribeWorks, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();

    ASSERT_EQ(manager->Subscribe(observer), 0);

    ASSERT_EQ(manager->Unsubscribe(observer), 0);

    ASSERT_EQ(manager->Subscribe(observer), 0);

    // Clean up
    manager->Unsubscribe(observer);
}

} // namespace OHOS::Test
