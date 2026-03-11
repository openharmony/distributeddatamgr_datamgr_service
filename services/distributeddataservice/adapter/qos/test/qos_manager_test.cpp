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
#include <vector>

#include "account/account_delegate.h"
#include "account_delegate_mock.h"
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
        testing::FLAGS_gmock_verbose = "error";
        mockAccountDelegate_ = std::make_shared<AccountDelegateMock>();
        AccountDelegate::RegisterAccountInstance(mockAccountDelegate_.get());
    }

    static void TearDownTestCase()
    {
        QosManager::SetQosFunctions(nullptr, nullptr);
        AccountDelegate::RegisterAccountInstance(nullptr);
        mockAccountDelegate_.reset();
    }

    void SetUp() override
    {
        QosManager::SetQosFunctions(nullptr, nullptr);
        QosManager::Reset();
        subscribedObserver_.reset();
        ON_CALL(*mockAccountDelegate_, Subscribe(_))
            .WillByDefault(WithArg<0>([this](std::shared_ptr<AccountDelegate::Observer> obs) {
                subscribedObserver_ = obs;
                return 0;
            }));
        ON_CALL(*mockAccountDelegate_, Unsubscribe(_)).WillByDefault(Return(0));
    }

    void TearDown() override
    {
        QosManager::SetQosFunctions(nullptr, nullptr);
        QosManager::Reset();
        subscribedObserver_.reset();
    }

protected:
    static std::shared_ptr<AccountDelegateMock> mockAccountDelegate_;
    std::shared_ptr<AccountDelegate::Observer> subscribedObserver_;

    void SetStartupPhase()
    {
        auto now = std::chrono::steady_clock::now();
        auto startTime = now - std::chrono::minutes(1);
        QosManager::SetStartTime(startTime);
    }

    void SetDynamicPhase()
    {
        auto now = std::chrono::steady_clock::now();
        auto startTime = now - std::chrono::minutes(4);
        QosManager::SetStartTime(startTime);
    }

    bool IsInStartupPhase()
    {
        auto setFunc = [](QOS::QosLevel) -> int {
            return 0;
        };
        QosManager::SetQosFunctions(setFunc, nullptr);
        QosManager qos(false);
        return qos.IsInStartupPhase();
    }

    void TriggerAccountEvent(AccountStatus status, const std::string &userId = "100")
    {
        AccountEventInfo eventInfo;
        eventInfo.status = status;
        eventInfo.userId = userId;
        if (subscribedObserver_) {
            subscribedObserver_->OnAccountChanged(eventInfo);
        }
    }
};

std::shared_ptr<AccountDelegateMock> QosManagerTest::mockAccountDelegate_ = nullptr;

/**
 * @tc.name: StartupPhase_DataShare_SetsQos_NoReset
 * @tc.desc: DataShare in startup phase sets QoS and does not reset
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, StartupPhase_DataShare_SetsQos_NoReset, TestSize.Level1)
{
    QosManager::Reset();
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
    QosManager::Reset();
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
    QosManager::Reset();
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
    QosManager::Reset();
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
    QosManager::Reset();
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
    QosManager::Reset();
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

    EXPECT_EQ(resetCallCount, 2);
}

/**
 * @tc.name: QosSetFailure_DoesNotSetFlag
 * @tc.desc: When QoS set fails, thread_local flag is not set
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, QosSetFailure_DoesNotSetFlag, TestSize.Level1)
{
    QosManager::Reset();
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
    QosManager::Reset();

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
    QosManager::Reset();
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
    QosManager::Reset();
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
        EXPECT_EQ(setCallCount, 1);
    }

    EXPECT_EQ(resetCallCount, 1);
}

/**
 * @tc.name: AccountUnlocked_RestartsStartupPhase
 * @tc.desc: Device account unlock event restarts startup phase
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, AccountUnlocked_RestartsStartupPhase, TestSize.Level1)
{
    QosManager::Reset();
    SetDynamicPhase();
    EXPECT_FALSE(IsInStartupPhase());
    QosManager::QosInit();
    TriggerAccountEvent(AccountStatus::DEVICE_ACCOUNT_UNLOCKED);
    EXPECT_TRUE(IsInStartupPhase());
}

/**
 * @tc.name: AccountSwitched_RestartsStartupPhase
 * @tc.desc: Device account switch event restarts startup phase
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, AccountSwitched_RestartsStartupPhase, TestSize.Level1)
{
    QosManager::Reset();
    SetDynamicPhase();
    EXPECT_FALSE(IsInStartupPhase());
    QosManager::QosInit();
    TriggerAccountEvent(AccountStatus::DEVICE_ACCOUNT_SWITCHED);
    EXPECT_TRUE(IsInStartupPhase());
}

/**
 * @tc.name: OtherAccountStatus_DoesNotRestartPhase
 * @tc.desc: Other account status events do not restart startup phase
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, OtherAccountStatus_DoesNotRestartPhase, TestSize.Level1)
{
    QosManager::Reset();
    SetDynamicPhase();
    EXPECT_FALSE(IsInStartupPhase());
    QosManager::QosInit();
    std::vector<AccountStatus> nonTriggeringStatuses = {
        AccountStatus::DEVICE_ACCOUNT_STOPPING,
        AccountStatus::DEVICE_ACCOUNT_STOPPED,
        AccountStatus::HARMONY_ACCOUNT_LOGOUT,
        AccountStatus::HARMONY_ACCOUNT_DELETE,
        AccountStatus::HARMONY_ACCOUNT_LOGIN,
        AccountStatus::DEVICE_ACCOUNT_DELETE,
    };

    for (auto status : nonTriggeringStatuses) {
        TriggerAccountEvent(status);
        EXPECT_FALSE(IsInStartupPhase());
    }
}

/**
 * @tc.name: MultipleAccountEvents_HandleCorrectly
 * @tc.desc: Multiple account events are handled correctly
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, MultipleAccountEvents_HandleCorrectly, TestSize.Level1)
{
    QosManager::Reset();
    SetDynamicPhase();
    EXPECT_FALSE(IsInStartupPhase());
    QosManager::QosInit();
    TriggerAccountEvent(AccountStatus::DEVICE_ACCOUNT_UNLOCKED);
    EXPECT_TRUE(IsInStartupPhase());

    TriggerAccountEvent(AccountStatus::DEVICE_ACCOUNT_SWITCHED);
    EXPECT_TRUE(IsInStartupPhase());

    TriggerAccountEvent(AccountStatus::DEVICE_ACCOUNT_STOPPING);
    EXPECT_TRUE(IsInStartupPhase());

    TriggerAccountEvent(AccountStatus::DEVICE_ACCOUNT_UNLOCKED);
    EXPECT_TRUE(IsInStartupPhase());
}

/**
 * @tc.name: QosInit_SubscribesObserver
 * @tc.desc: QosInit subscribes QosObserver to AccountDelegate
 * @tc.type: FUNC
 */
HWTEST_F(QosManagerTest, QosInit_SubscribesObserver, TestSize.Level1)
{
    QosManager::Reset();
    QosManager::QosInit();
    EXPECT_NE(subscribedObserver_, nullptr);
    std::string name = subscribedObserver_->Name();
    EXPECT_EQ(name, "QosObserver");

    AccountDelegate::Observer::LevelType level = subscribedObserver_->GetLevel();
    EXPECT_EQ(level, AccountDelegate::Observer::LevelType::HIGH);
}
} // namespace DistributedData
} // namespace OHOS
