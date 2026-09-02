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

#define LOG_TAG "ThermalStateMonitorImpl"
#include "thermal_state_monitor_impl.h"

#include <new>
#include <utility>
#include <vector>

#include "error/general_error.h"
#include "log_print.h"

#if defined(DATAMGR_THERMAL_PART_ENABLED)
#include "thermal_mgr_client.h"
#endif

namespace OHOS::DistributedData {
namespace {
constexpr int32_t THERMAL_LEVEL_MIN = 0;
constexpr int32_t THERMAL_LEVEL_MAX = 7;
constexpr int32_t THERMAL_QUERY_FAILED = -1;

int32_t ClampLevel(int32_t level)
{
    if (level < THERMAL_LEVEL_MIN) {
        return THERMAL_LEVEL_MIN;
    }
    if (level > THERMAL_LEVEL_MAX) {
        return THERMAL_LEVEL_MAX;
    }
    return level;
}
} // namespace

#if defined(DATAMGR_THERMAL_PART_ENABLED)
ThermalStateMonitorImpl::ThermalLevelCallback::ThermalLevelCallback(ThermalStateMonitorImpl &monitor)
    : monitor_(monitor)
{
}

bool ThermalStateMonitorImpl::ThermalLevelCallback::OnThermalLevelChanged(OHOS::PowerMgr::ThermalLevel level)
{
    monitor_.OnThermalLevelChanged(static_cast<int32_t>(level));
    return true;
}

#endif

__attribute__((used)) static bool g_isInit = ThermalStateMonitorImpl::Register();

bool ThermalStateMonitorImpl::Register()
{
    static ThermalStateMonitorImpl instance;
    ThermalStateMonitor::RegisterInstance(&instance);
    return true;
}

int32_t ThermalStateMonitorImpl::Subscribe(const std::string &name, Observer observer)
{
    if (name.empty() || observer == nullptr) {
        return E_INVALID_ARGS;
    }
    Snapshot snapshot;
#if defined(DATAMGR_THERMAL_PART_ENABLED)
    sptr<OHOS::PowerMgr::IThermalLevelCallback> callback;
    uint64_t querySequence = 0;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this]() {
            return !subscribing_;
        });
        observers_[name] = observer;
        if (started_) {
            snapshot = snapshot_;
        } else {
            if (callback_ == nullptr) {
                callback_ = new (std::nothrow) ThermalLevelCallback(*this);
            }
            if (callback_ == nullptr) {
                observers_.erase(name);
                return E_ERROR;
            }
            callback = callback_;
            subscribing_ = true;
            querySequence = updateSequence_;
        }
    }
    if (callback != nullptr) {
        auto &client = OHOS::PowerMgr::ThermalMgrClient::GetInstance();
        bool subscribed = client.SubscribeThermalLevelCallback(callback, true);
        auto level = client.GetThermalLevel();
        int32_t rawLevel = static_cast<int32_t>(level);
        int32_t initialLevel = THERMAL_LEVEL_MIN;
        bool querySucceeded = NormalizeInitialLevel(rawLevel, initialLevel);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            started_ = subscribed;
            subscribing_ = false;
            if (subscribed && querySucceeded && updateSequence_ == querySequence) {
                snapshot_.level = initialLevel;
                ++updateSequence_;
            }
            if (!subscribed) {
                observers_.erase(name);
            }
            snapshot = snapshot_;
        }
        condition_.notify_all();
        if (!subscribed) {
            ZLOGW("subscribe thermal level failed");
            return E_ERROR;
        }
    }
#else
    {
        std::lock_guard<std::mutex> lock(mutex_);
        observers_[name] = observer;
        started_ = true;
        snapshot = snapshot_;
    }
    ZLOGW("thermal capability is disabled");
#endif
    observer(snapshot);
    return E_OK;
}

int32_t ThermalStateMonitorImpl::Unsubscribe(const std::string &name)
{
#if defined(DATAMGR_THERMAL_PART_ENABLED)
    sptr<OHOS::PowerMgr::IThermalLevelCallback> callback;
#endif
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this]() {
            return !subscribing_;
        });
        if (observers_.erase(name) == 0) {
            return E_ERROR;
        }
        if (!observers_.empty() || !started_) {
            return E_OK;
        }
        started_ = false;
#if defined(DATAMGR_THERMAL_PART_ENABLED)
        callback = callback_;
#endif
    }
#if defined(DATAMGR_THERMAL_PART_ENABLED)
    if (callback != nullptr &&
        !OHOS::PowerMgr::ThermalMgrClient::GetInstance().UnSubscribeThermalLevelCallback(callback)) {
        ZLOGW("unsubscribe thermal level failed");
        return E_ERROR;
    }
#endif
    return E_OK;
}

ThermalStateMonitor::Snapshot ThermalStateMonitorImpl::GetSnapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_;
}

bool ThermalStateMonitorImpl::NormalizeInitialLevel(int32_t rawLevel, int32_t &level) const
{
    if (rawLevel == THERMAL_QUERY_FAILED) {
        ZLOGE("get initial thermal level failed, error:%{public}d", rawLevel);
        return false;
    }
    level = ClampLevel(rawLevel);
    if (rawLevel != level) {
        ZLOGW("clamp initial thermal level, raw:%{public}d, clamped:%{public}d", rawLevel, level);
    }
    return true;
}

void ThermalStateMonitorImpl::OnThermalLevelChanged(int32_t level)
{
    int32_t clampedLevel = ClampLevel(level);
    if (level != clampedLevel) {
        ZLOGW("clamp thermal callback, raw:%{public}d, clamped:%{public}d", level, clampedLevel);
    }
    Snapshot snapshot;
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!started_ && !subscribing_) {
            return;
        }
        changed = snapshot_.level != clampedLevel;
        snapshot_.level = clampedLevel;
        ++updateSequence_;
        snapshot = snapshot_;
    }
    if (changed) {
        Notify(snapshot);
    }
}

void ThermalStateMonitorImpl::Notify(const Snapshot &snapshot)
{
    std::vector<Observer> observers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto &[name, observer] : observers_) {
            (void)name;
            observers.push_back(observer);
        }
    }
    for (const auto &observer : observers) {
        if (observer != nullptr) {
            observer(snapshot);
        }
    }
}
} // namespace OHOS::DistributedData
