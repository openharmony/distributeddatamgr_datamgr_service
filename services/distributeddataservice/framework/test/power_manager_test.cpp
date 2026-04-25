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

#define LOG_TAG "PowerManagerTest"
#include <gtest/gtest.h>

#include "power_manager/power_manager.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;

namespace OHOS::Test {
// Simple test implementation of PowerManager for testing registration mechanism
class TestPowerManager : public PowerManager {
public:
    int32_t Subscribe(std::shared_ptr<Observer> observer) override { return 0; }
    int32_t Unsubscribe(std::shared_ptr<Observer> observer) override { return 0; }
    bool IsCharging() override { return false; }
    void SubscribePowerEvent() override {}
    void UnsubscribePowerEvent() override {}
};

class PowerManagerTest : public testing::Test {
public:
    static TestPowerManager* testInstance_;

    static void SetUpTestCase(void)
    {
        // Perform first registration in SetUpTestCase
        testInstance_ = new TestPowerManager();
        bool result = PowerManager::RegisterInstance(testInstance_);
        ASSERT_TRUE(result) << "First registration should succeed";
    }

    static void TearDownTestCase(void) {}

    void SetUp() {}

    void TearDown() {}
};

TestPowerManager* PowerManagerTest::testInstance_ = nullptr;

/**
 * @tc.name: RegisterInstance_DuplicateCall_ReturnsFalse
 * @tc.desc: test RegisterInstance with duplicate registration returns false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PowerManagerTest, RegisterInstance_DuplicateCall_ReturnsFalse, TestSize.Level1)
{
    // After first registration in SetUpTestCase,
    // any additional registration should fail
    TestPowerManager anotherInstance;
    bool result = PowerManager::RegisterInstance(&anotherInstance);

    ASSERT_FALSE(result) << "Duplicate registration should return false";
}

/**
 * @tc.name: GetInstance_AfterRegistration_ReturnsValidInstance
 * @tc.desc: test GetInstance returns valid instance after registration
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PowerManagerTest, GetInstance_AfterRegistration_ReturnsValidInstance, TestSize.Level1)
{
    // After registration in SetUpTestCase,
    // GetInstance should return the registered instance
    auto *instance = PowerManager::GetInstance();

    ASSERT_NE(instance, nullptr) << "GetInstance should return valid instance after registration";
    ASSERT_EQ(instance, testInstance_) << "GetInstance should return the registered instance";
}

} // namespace OHOS::Test
