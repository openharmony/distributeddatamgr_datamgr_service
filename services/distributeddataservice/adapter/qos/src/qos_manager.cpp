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

#include "log_print.h"
#include "qos.h"

namespace OHOS {
namespace DistributedData {

thread_local bool g_threadQosSet = false;
constexpr auto STARTUP_PHASE_DURATION = std::chrono::minutes(3);

std::atomic<std::chrono::steady_clock::time_point> g_startTime { std::chrono::steady_clock::now() };
std::atomic<bool> g_startupPhaseEnded { false };

QosManager::SetQosFunc QosManager::setQosFunc_ = [](QOS::QosLevel level) { return QOS::SetThreadQos(level); };
QosManager::ResetQosFunc QosManager::resetQosFunc_ = []() { return QOS::ResetThreadQos(); };

void QosManager::SetQosFunctions(SetQosFunc setFunc, ResetQosFunc resetFunc)
{
    setQosFunc_ = setFunc ? setFunc : SetQosFunc {};
    resetQosFunc_ = resetFunc ? resetFunc : ResetQosFunc {};
}

void QosManager::SetStartTime(const std::chrono::steady_clock::time_point& time)
{
    g_startTime.store(time, std::memory_order_relaxed);
    g_startupPhaseEnded.store(false, std::memory_order_relaxed);
}

void QosManager::Reset()
{
    g_threadQosSet = false;
}

QosManager::QosManager(bool isDataShare)
    : isDataShare_(isDataShare)
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
    if (g_startupPhaseEnded.load(std::memory_order_relaxed)) {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    auto startTime = g_startTime.load(std::memory_order_relaxed);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);

    if (elapsed >= STARTUP_PHASE_DURATION) {
        g_startupPhaseEnded.store(true, std::memory_order_relaxed);
        return false;
    }

    return true;
}
} // namespace DistributedData
} // namespace OHOS
