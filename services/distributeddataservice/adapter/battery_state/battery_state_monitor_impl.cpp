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

#define LOG_TAG "BatteryStateMonitorImpl"
#include "battery_state_monitor_impl.h"

#include <initializer_list>
#include <utility>
#include <vector>

#include "common_event_manager.h"
#include "error/general_error.h"
#include "log_print.h"
#include "want.h"

namespace OHOS::DistributedData {
namespace {
constexpr const char *BATTERY_CHANGED_EVENT = "usual.event.BATTERY_CHANGED";
constexpr const char *BATTERY_CAPACITY_LEVEL_EVENT = "usual.event.BATTERY_CAPACITY_LEVEL_UPDATE";
constexpr const char *BATTERY_LOW_EVENT = "usual.event.BATTERY_LOW";
constexpr const char *BATTERY_OKAY_EVENT = "usual.event.BATTERY_OKAY";
constexpr int32_t INVALID_LEVEL = -1;
constexpr int32_t BATTERY_LEVEL_MIN = 0;
constexpr int32_t BATTERY_LEVEL_MAX = 7;
constexpr int32_t BATTERY_LEVEL_NORMAL = 3;
constexpr int32_t BATTERY_LEVEL_LOW = 4;

int32_t ClampLevel(int32_t value)
{
    if (value < BATTERY_LEVEL_MIN) {
        return BATTERY_LEVEL_MIN;
    }
    if (value > BATTERY_LEVEL_MAX) {
        return BATTERY_LEVEL_MAX;
    }
    return value;
}

int32_t GetIntParam(const AAFwk::Want &want, const std::initializer_list<const char *> &keys)
{
    for (const auto *key : keys) {
        int32_t value = want.GetIntParam(key, INVALID_LEVEL);
        if (value != INVALID_LEVEL) {
            return value;
        }
    }
    return INVALID_LEVEL;
}
} // namespace

class BatteryStateEventSubscriber final : public EventFwk::CommonEventSubscriber {
public:
    using Callback = std::function<void(const EventFwk::CommonEventData &)>;

    explicit BatteryStateEventSubscriber(const EventFwk::CommonEventSubscribeInfo &info)
        : CommonEventSubscriber(info)
    {
    }

    void SetCallback(Callback callback)
    {
        callback_ = std::move(callback);
    }

    void OnReceiveEvent(const EventFwk::CommonEventData &event) override
    {
        if (callback_ != nullptr) {
            callback_(event);
        }
    }

private:
    Callback callback_;
};

__attribute__((used)) static bool g_isInit = BatteryStateMonitorImpl::Register();

bool BatteryStateMonitorImpl::Register()
{
    static BatteryStateMonitorImpl instance;
    BatteryStateMonitor::RegisterInstance(&instance);
    return true;
}

int32_t BatteryStateMonitorImpl::Subscribe(const std::string &name, Observer observer)
{
    if (name.empty() || observer == nullptr) {
        return E_INVALID_ARGS;
    }
    Snapshot snapshot;
    std::shared_ptr<BatteryStateEventSubscriber> subscriber;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this]() {
            return !subscribing_;
        });
        observers_[name] = observer;
        snapshot = snapshot_;
        if (!started_) {
            subscriber = GetSubscriberLocked();
            subscribing_ = true;
        }
    }
    if (subscriber != nullptr) {
        bool result = EventFwk::CommonEventManager::SubscribeCommonEvent(subscriber);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            subscribing_ = false;
            started_ = result;
            if (!result) {
                observers_.erase(name);
            }
        }
        condition_.notify_all();
        if (!result) {
            ZLOGE("subscribe battery state event failed, name:%{public}s", name.c_str());
            return E_ERROR;
        }
    }
    observer(snapshot);
    return E_OK;
}

int32_t BatteryStateMonitorImpl::Unsubscribe(const std::string &name)
{
    std::shared_ptr<BatteryStateEventSubscriber> subscriber;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this]() {
            return !subscribing_;
        });
        if (observers_.erase(name) == 0) {
            ZLOGE("BatteryStateMonitor Unsubscribe failed: observer not found, name:%{public}s", name.c_str());
            return E_ERROR;
        }
        if (observers_.empty() && started_) {
            started_ = false;
            subscriber = batterySubscriber_;
        }
    }
    if (subscriber != nullptr) {
        bool result = EventFwk::CommonEventManager::UnSubscribeCommonEvent(subscriber);
        if (!result) {
            ZLOGE("unsubscribe battery state event failed");
        }
    }
    return E_OK;
}

BatteryStateMonitor::Snapshot BatteryStateMonitorImpl::GetSnapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_;
}

void BatteryStateMonitorImpl::UnsubscribeBatteryEvent()
{
    std::shared_ptr<BatteryStateEventSubscriber> subscriber;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!started_) {
            return;
        }
        started_ = false;
        subscriber = batterySubscriber_;
    }
    if (subscriber != nullptr) {
        bool result = EventFwk::CommonEventManager::UnSubscribeCommonEvent(subscriber);
        if (!result) {
            ZLOGE("unsubscribe battery state event failed");
        } else {
            ZLOGI("unsubscribe battery state event success");
        }
    }
}

std::shared_ptr<BatteryStateEventSubscriber> BatteryStateMonitorImpl::GetSubscriberLocked()
{
    if (batterySubscriber_ == nullptr) {
        EventFwk::MatchingSkills matchingSkills;
        matchingSkills.AddEvent(BATTERY_CHANGED_EVENT);
        matchingSkills.AddEvent(BATTERY_CAPACITY_LEVEL_EVENT);
        matchingSkills.AddEvent(BATTERY_LOW_EVENT);
        matchingSkills.AddEvent(BATTERY_OKAY_EVENT);
        EventFwk::CommonEventSubscribeInfo info(matchingSkills);
        batterySubscriber_ = std::make_shared<BatteryStateEventSubscriber>(info);
        batterySubscriber_->SetCallback([this](const EventFwk::CommonEventData &event) {
            OnBatteryEvent(event);
        });
    }
    return batterySubscriber_;
}

void BatteryStateMonitorImpl::OnBatteryEvent(const EventFwk::CommonEventData &event)
{
    const auto want = event.GetWant();
    const auto action = want.GetAction();
    int32_t level = INVALID_LEVEL;
    Snapshot snapshot;

    if (action == BATTERY_LOW_EVENT) {
        level = BATTERY_LEVEL_LOW;
    } else if (action == BATTERY_OKAY_EVENT) {
        level = BATTERY_LEVEL_NORMAL;
    } else if (action == BATTERY_CHANGED_EVENT || action == BATTERY_CAPACITY_LEVEL_EVENT) {
        level = GetIntParam(want, { "batteryCapacityLevel", "capacityLevel", "batteryLevel", "level" });
    }

    if (level != INVALID_LEVEL && UpdateBatteryLevel(level, snapshot)) {
        Notify(snapshot);
    }
}

bool BatteryStateMonitorImpl::UpdateBatteryLevel(int32_t level, Snapshot &snapshot)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!started_) {
        return false;
    }
    int32_t normalized = ClampLevel(level);
    if (snapshot_.batteryLevel == normalized) {
        return false;
    }
    snapshot_.batteryLevel = normalized;
    snapshot = snapshot_;
    return true;
}

/**
 * Observer is invoked synchronously. The initial snapshot is called on the
 * Subscribe caller thread, and later battery updates are called on the
 * CommonEvent callback thread. Observers must dispatch to the target runtime
 * thread themselves before touching JS/ArkTS objects.
 */
void BatteryStateMonitorImpl::Notify(const Snapshot &snapshot)
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
