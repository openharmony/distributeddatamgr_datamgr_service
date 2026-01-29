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
#include <gmock/gmock.h>
#include <chrono>
#include <thread>

#include "log_print.h"
#include "qos_manager.h"

namespace OHOS {
namespace DistributedData {
using namespace testing;
using namespace testing::ext;

class QosManagerTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
    }

    static void TearDownTestCase()
    {
        QosManager::SetQosFunctions(nullptr, nullptr);
    }

    void SetUp() override
    {
        QosManager::SetQosFunctions(nullptr, nullptr);
    }

    void TearDown() override
    {
        QosManager::SetQosFunctions(nullptr, nullptr);
    }

protected:
    void SetStartupPhase()
    {
        auto now = std::chrono::steady_clock::now();
        auto startTime = now - std::chrono::minutes(1);
        QosManager::SetStartTimeForTest(startTime);
    }

    void SetDynamicPhase()
    {
        auto now = std::chrono::steady_clock::now();
        auto startTime = now - std::chrono::minutes(4);
        QosManager::SetStartTimeForTest(startTime);
    }
};

/**
 * @tc.name: StartupPhase_DataShare_SetsQos_NoReset
 * @tc.desc: DataShare in startup phase sets QoS and does not reset
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, StartupPhase_DataShare_SetsQos_NoReset, TestSize.Level1)
{
    SetStartupPhase();

    int setCallCount = 0;
    int resetCallCount = 0;

    auto setFunc = [&setCallCount](QOS::QosLevel) -> int {
        setCallCount++;
        return 0;
    };

    auto resetFunc = [&resetCallCount]() -> int {
        resetCallCount++;
        return 0;
    };

    QosManager::SetQosFunctions(setFunc, resetFunc);

    {
        QosManager qos(true);
        EXPECT_EQ(setCallCount, 1);
    }

    EXPECT_EQ(setCallCount, 1);
    EXPECT_EQ(resetCallCount, 0);
}

/**
 * @tc.name: StartupPhase_OtherBusiness_SetsQos_NoReset
 * @tc.desc: Other businesses in startup phase set QoS and do not reset
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, StartupPhase_OtherBusiness_SetsQos_NoReset, TestSize.Level1)
{
    SetStartupPhase();

    int setCallCount = 0;

    auto setFunc = [&setCallCount](QOS::QosLevel) -> int {
        setCallCount++;
        return 0;
    };

    QosManager::SetQosFunctions(setFunc, nullptr);

    {
        QosManager qos(false);
    }

    EXPECT_EQ(setCallCount, 1);
}

/**
 * @tc.name: DynamicPhase_DataShare_SetsAndResetsQos
 * @tc.desc: DataShare in dynamic phase sets and resets QoS
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, DynamicPhase_DataShare_SetsAndResetsQos, TestSize.Level1)
{
    SetDynamicPhase();

    int setCallCount = 0;
    int resetCallCount = 0;

    auto setFunc = [&setCallCount](QOS::QosLevel) -> int {
        setCallCount++;
        return 0;
    };

    auto resetFunc = [&resetCallCount]() -> int {
        resetCallCount++;
        return 0;
    };

    QosManager::SetQosFunctions(setFunc, resetFunc);

    {
        QosManager qos(true);
        EXPECT_EQ(setCallCount, 1);
    }

    EXPECT_EQ(setCallCount, 1);
    EXPECT_EQ(resetCallCount, 1);
}

/**
 * @tc.name: DynamicPhase_OtherBusiness_DoesNothing
 * @tc.desc: Other businesses in dynamic phase do nothing
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, DynamicPhase_OtherBusiness_DoesNothing, TestSize.Level1)
{
    SetDynamicPhase();

    int setCallCount = 0;

    auto setFunc = [&setCallCount](QOS::QosLevel) -> int {
        setCallCount++;
        return 0;
    };

    QosManager::SetQosFunctions(setFunc, nullptr);

    {
        QosManager qos(false);
    }

    EXPECT_EQ(setCallCount, 0);
}

/**
 * @tc.name: NestedQosManager_SecondIsNoOp
 * @tc.desc: Nested QosManager objects don't set QoS twice
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, NestedQosManager_SecondIsNoOp, TestSize.Level1)
{
    SetStartupPhase();

    int setCallCount = 0;

    auto setFunc = [&setCallCount](QOS::QosLevel) -> int {
        setCallCount++;
        return 0;
    };

    auto resetFunc = []() -> int { return 0; };
    QosManager::SetQosFunctions(setFunc, resetFunc);

    {
        QosManager qos1(true);
        EXPECT_EQ(setCallCount, 1);

        QosManager qos2(true);
        EXPECT_EQ(setCallCount, 1);
    }

    EXPECT_EQ(setCallCount, 1);
}

/**
 * @tc.name: MultipleSequential_CanSetAgainAfterReset
 * @tc.desc: After reset, sequential QosManager can set QoS again
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, MultipleSequential_CanSetAgainAfterReset, TestSize.Level1)
{
    SetDynamicPhase();

    int setCallCount = 0;
    int resetCallCount = 0;

    auto setFunc = [&setCallCount](QOS::QosLevel) -> int {
        setCallCount++;
        return 0;
    };

    auto resetFunc = [&resetCallCount]() -> int {
        resetCallCount++;
        return 0;
    };

    QosManager::SetQosFunctions(setFunc, resetFunc);

    {
        QosManager qos1(true);
        EXPECT_EQ(setCallCount, 1);
    }

    {
        QosManager qos2(true);
        EXPECT_EQ(setCallCount, 2);
    }

    EXPECT_EQ(resetCallCount, 1);
}

/**
 * @tc.name: QosSetFailure_DoesNotSetFlag
 * @tc.desc: When QoS set fails, thread_local flag is not set
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, QosSetFailure_DoesNotSetFlag, TestSize.Level1)
{
    SetDynamicPhase();

    int setCallCount = 0;
    int resetCallCount = 0;

    auto setFunc = [&setCallCount](QOS::QosLevel) -> int {
        setCallCount++;
        return -1;
    };

    auto resetFunc = [&resetCallCount]() -> int {
        resetCallCount++;
        return 0;
    };

    QosManager::SetQosFunctions(setFunc, resetFunc);

    {
        QosManager qos(true);
        EXPECT_EQ(setCallCount, 1);
    }

    EXPECT_EQ(resetCallCount, 0);
}

/**
 * @tc.name: ThreadLocal_SeparateThreads
 * @tc.desc: Thread-local flag works correctly across threads
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, ThreadLocal_SeparateThreads, TestSize.Level1)
{
    int setCallCount = 0;
    std::mutex countMutex;

    auto setFunc = [&setCallCount, &countMutex](QOS::QosLevel) -> int {
        std::lock_guard<std::mutex> lock(countMutex);
        setCallCount++;
        return 0;
    };

    QosManager::SetQosFunctions(setFunc, nullptr);

    std::thread thread1([&setFunc]() {
        QosManager qos1(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });

    std::thread thread2([&setFunc]() {
        QosManager qos2(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });

    thread1.join();
    thread2.join();

    EXPECT_EQ(setCallCount, 2);
}

/**
 * @tc.name: ResetFailure_LogsError
 * @tc.desc: When QoS reset fails, error is logged but no crash
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, ResetFailure_LogsError, TestSize.Level1)
{
    SetDynamicPhase();

    int setCallCount = 0;
    int resetCallCount = 0;

    auto setFunc = [&setCallCount](QOS::QosLevel) -> int {
        setCallCount++;
        return 0;
    };

    auto resetFunc = [&resetCallCount]() -> int {
        resetCallCount++;
        return -1;
    };

    QosManager::SetQosFunctions(setFunc, resetFunc);

    {
        QosManager qos(true);
        EXPECT_EQ(setCallCount, 1);
    }

    EXPECT_EQ(resetCallCount, 1);
}

/**
 * @tc.name: StartupToDynamicTransition
 * @tc.desc: Test behavior when transitioning from startup to dynamic phase
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, StartupToDynamicTransition, TestSize.Level1)
{
    SetStartupPhase();

    int setCallCount = 0;
    int resetCallCount = 0;

    auto setFunc = [&setCallCount](QOS::QosLevel) -> int {
        setCallCount++;
        return 0;
    };

    auto resetFunc = [&resetCallCount]() -> int {
        resetCallCount++;
        return 0;
    };

    QosManager::SetQosFunctions(setFunc, resetFunc);

    {
        QosManager qos1(true);
        EXPECT_EQ(setCallCount, 1);
    }
    EXPECT_EQ(resetCallCount, 0);

    SetDynamicPhase();

    {
        QosManager qos2(true);
        EXPECT_EQ(setCallCount, 2);
    }

    EXPECT_EQ(resetCallCount, 1);
}

} // namespace DistributedData
} // namespace OHOS
