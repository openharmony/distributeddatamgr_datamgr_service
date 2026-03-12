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

#define LOG_TAG "QosManager"
#include "qos_manager.h"

#include <atomic>
#include <chrono>
#include <mutex>

#include "account/account_delegate.h"
#include "log_print.h"
#include "qos.h"

namespace OHOS {
namespace DistributedData {

thread_local bool g_threadQosSet = false;
constexpr auto STARTUP_PHASE_DURATION = std::chrono::minutes(3);
std::chrono::steady_clock::time_point g_startupEndTime = std::chrono::steady_clock::now() + STARTUP_PHASE_DURATION;
bool g_startupPhaseEnded = false;

class QosObserver : public AccountDelegate::Observer {
public:
    explicit QosObserver();
    void OnAccountChanged(const DistributedData::AccountEventInfo &eventInfo, int32_t timeout) override;
    std::string Name() override;
    LevelType GetLevel() override;
};

QosObserver::QosObserver()
{
}

void QosObserver::OnAccountChanged(const DistributedData::AccountEventInfo &eventInfo, int32_t timeout)
{
    if (eventInfo.status == AccountStatus::DEVICE_ACCOUNT_UNLOCKED ||
        eventInfo.status == AccountStatus::DEVICE_ACCOUNT_SWITCHED) {
        ZLOGI("StartupPhase restart");
        QosManager::ResetStartupEndTime(std::chrono::steady_clock::now() + STARTUP_PHASE_DURATION);
    }
}

std::string QosObserver::Name()
{
    return "QosObserver";
}

QosObserver::LevelType QosObserver::GetLevel()
{
    return LevelType::HIGH;
}

QosManager::SetQosFunc QosManager::setQosFunc_ = [](QOS::QosLevel level) { return QOS::SetThreadQos(level); };
QosManager::ResetQosFunc QosManager::resetQosFunc_ = []() { return QOS::ResetThreadQos(); };

std::mutex QosManager::mutex_;
void QosManager::SetQosFunctions(SetQosFunc setFunc, ResetQosFunc resetFunc)
{
    setQosFunc_ = setFunc ? setFunc : SetQosFunc{};
    resetQosFunc_ = resetFunc ? resetFunc : ResetQosFunc{};
}

void QosManager::ResetStartupEndTime(const std::chrono::steady_clock::time_point &startupEndTime)
{
    std::lock_guard<std::mutex> lock(mutex_);
    g_startupEndTime = startupEndTime;
    g_startupPhaseEnded = false;
}

void QosManager::Reset()
{
    g_threadQosSet = false;
}

QosManager::QosManager(bool isDataShare) : isDataShare_(isDataShare)
{
#ifndef IS_EMULATOR
    if (g_threadQosSet || !(isDataShare_ || IsInStartupPhase()) || setQosFunc_ == nullptr) {
        return;
    }

    int ret = setQosFunc_(QOS::QosLevel::QOS_USER_INTERACTIVE);
    if (ret == 0) {
        g_threadQosSet = true;
    } else {
        ZLOGE("Failed to set thread QoS to QOS_USER_INTERACTIVE, ret: %{public}d", ret);
    }
#endif
}

QosManager::~QosManager()
{
#ifndef IS_EMULATOR
    if (!g_threadQosSet || IsInStartupPhase()) {
        return;
    }
    if (resetQosFunc_ != nullptr) {
        int ret = resetQosFunc_();
        if (ret != 0) {
            ZLOGE("Failed to reset thread QoS, ret: %{public}d", ret);
        }
    }
    g_threadQosSet = false;
#endif
}

bool QosManager::IsInStartupPhase() const
{
    if (g_startupPhaseEnded) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (g_startupPhaseEnded) {
        return false;
    }
    auto now = std::chrono::steady_clock::now();
    if (now >= g_startupEndTime) {
        g_startupPhaseEnded = true;
        ZLOGI("StartupPhase end");
        return false;
    }

    return true;
}

void QosManager::QosInit()
{
    auto obs = std::make_shared<QosObserver>();
    AccountDelegate::GetInstance()->Subscribe(obs);
    ZLOGI("StartupPhase start");
}
} // namespace DistributedData
} // namespace OHOS
