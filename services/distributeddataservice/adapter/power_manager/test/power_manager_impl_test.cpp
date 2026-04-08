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

#include "power_manager_impl.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;

namespace OHOS::Test {
// Mock Observer for testing
class TestPowerObserver : public PowerManager::Observer {
public:
    PowerManager::Observer::PowerEvent lastEvent = PowerManager::Observer::PowerEvent::BUTT;
    int notifyCount = 0;

    void OnChange(PowerEvent event) override
    {
        lastEvent = event;
        notifyCount++;
    }

    void Reset()
    {
        lastEvent = PowerManager::Observer::PowerEvent::BUTT;
        notifyCount = 0;
    }
};

class PowerManagerImplTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}

    static void TearDownTestCase(void) {}

    void SetUp() {}

    void TearDown() {}

    // Helper to get manager instance
    PowerManager* GetManager()
    {
        return PowerManager::GetInstance();
    }

    // Helper to create observer
    std::shared_ptr<TestPowerObserver> CreateObserver()
    {
        return std::make_shared<TestPowerObserver>();
    }
};

/**
 * @tc.name: Subscribe_NullObserver_ReturnsError
 * @tc.desc: test Subscribe with null observer returns error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
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
 * @tc.author:
 */
HWTEST_F(PowerManagerImplTest, Subscribe_ValidObserver_ReturnsSuccess, TestSize.Level1)
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
 * @tc.name: Unsubscribe_NullObserver_ReturnsError
 * @tc.desc: test Unsubscribe with null observer returns error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
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
 * @tc.author:
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
 * @tc.author:
 */
HWTEST_F(PowerManagerImplTest, Unsubscribe_NonExistentObserver_ReturnsError, TestSize.Level1)
{
    auto *manager = GetManager();
    auto observer = CreateObserver();

    int32_t result = manager->Unsubscribe(observer);
    ASSERT_EQ(result, -1);
}

/**
 * @tc.name: IsCharging_InitialState_ReturnsFalse
 * @tc.desc: test IsCharging returns false in initial state
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PowerManagerImplTest, IsCharging_InitialState_ReturnsFalse, TestSize.Level1)
{
    auto *manager = GetManager();
    ASSERT_FALSE(manager->IsCharging());
}

/**
 * @tc.name: SubscribeUnsubscribeSubscribe_ResubscribeWorks
 * @tc.desc: test subscribe-unsubscribe-subscribe sequence works
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
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

} // namespace OHOS::Test
