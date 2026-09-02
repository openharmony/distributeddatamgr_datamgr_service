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

#define LOG_TAG "SystemLoadMonitorImpl"
#include "system_load_monitor_impl.h"

#include <new>
#include <utility>
#include <vector>

#include "error/general_error.h"
#include "log_print.h"

#if defined(DATAMGR_RESOURCE_SCHEDULE_PART_ENABLED)
#include "res_sched_client.h"
#endif

namespace OHOS::DistributedData {
namespace {
constexpr int32_t SYSTEM_LOAD_MIN = 0;
constexpr int32_t SYSTEM_LOAD_MAX = 7;
constexpr int32_t SYSTEM_LOAD_QUERY_FAILED = -2;

int32_t ClampLevel(int32_t level)
{
    if (level < SYSTEM_LOAD_MIN) {
        return SYSTEM_LOAD_MIN;
    }
    if (level > SYSTEM_LOAD_MAX) {
        return SYSTEM_LOAD_MAX;
    }
    return level;
}
} // namespace

#if defined(DATAMGR_RESOURCE_SCHEDULE_PART_ENABLED)
SystemLoadMonitorImpl::SystemLoadNotifier::SystemLoadNotifier(SystemLoadMonitorImpl &monitor) : monitor_(monitor)
{
}

void SystemLoadMonitorImpl::SystemLoadNotifier::OnSystemloadLevel(int32_t level)
{
    monitor_.OnSystemLoadChanged(level);
}
#endif

__attribute__((used)) static bool g_isInit = SystemLoadMonitorImpl::Register();

bool SystemLoadMonitorImpl::Register()
{
    static SystemLoadMonitorImpl instance;
    SystemLoadMonitor::RegisterInstance(&instance);
    return true;
}

int32_t SystemLoadMonitorImpl::Subscribe(const std::string &name, Observer observer)
{
    if (name.empty() || observer == nullptr) {
        return E_INVALID_ARGS;
    }
    Snapshot snapshot;
    SubscriptionContext context;
    int32_t status = PrepareSubscription(name, observer, snapshot, context);
    if (status != E_OK) {
        return status;
    }
#if defined(DATAMGR_RESOURCE_SCHEDULE_PART_ENABLED)
    if (context.notifier != nullptr) {
        CompleteSubscription(context, snapshot);
    }
#else
    ZLOGW("system load capability is disabled");
#endif
    observer(snapshot);
    return E_OK;
}

int32_t SystemLoadMonitorImpl::PrepareSubscription(
    const std::string &name, const Observer &observer, Snapshot &snapshot, SubscriptionContext &context)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this]() { return !subscribing_; });
        observers_[name] = observer;
#if defined(DATAMGR_RESOURCE_SCHEDULE_PART_ENABLED)
        if (started_) {
            snapshot = snapshot_;
        } else {
            if (notifier_ == nullptr) {
                notifier_ = new (std::nothrow) SystemLoadNotifier(*this);
            }
            if (notifier_ == nullptr) {
                observers_.erase(name);
                return E_ERROR;
            }
            context.notifier = notifier_;
            subscribing_ = true;
            context.updateSequence = updateSequence_;
        }
#else
        started_ = true;
        snapshot = snapshot_;
#endif
    }
    return E_OK;
}

#if defined(DATAMGR_RESOURCE_SCHEDULE_PART_ENABLED)
void SystemLoadMonitorImpl::CompleteSubscription(const SubscriptionContext &context, Snapshot &snapshot)
{
    auto &client = OHOS::ResourceSchedule::ResSchedClient::GetInstance();
    client.RegisterSystemloadNotifier(context.notifier);
    int32_t rawLevel = client.GetSystemloadLevel();
    int32_t initialLevel = SYSTEM_LOAD_MIN;
    bool querySucceeded = NormalizeInitialLevel(rawLevel, initialLevel);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        started_ = true;
        subscribing_ = false;
        if (querySucceeded && updateSequence_ == context.updateSequence) {
            snapshot_.level = initialLevel;
            ++updateSequence_;
        }
        snapshot = snapshot_;
    }
    condition_.notify_all();
}
#endif

int32_t SystemLoadMonitorImpl::Unsubscribe(const std::string &name)
{
#if defined(DATAMGR_RESOURCE_SCHEDULE_PART_ENABLED)
    sptr<OHOS::ResourceSchedule::ResSchedSystemloadNotifierClient> notifier;
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
#if defined(DATAMGR_RESOURCE_SCHEDULE_PART_ENABLED)
        notifier = notifier_;
#endif
    }
#if defined(DATAMGR_RESOURCE_SCHEDULE_PART_ENABLED)
    if (notifier != nullptr) {
        OHOS::ResourceSchedule::ResSchedClient::GetInstance().UnRegisterSystemloadNotifier(notifier);
    }
#endif
    return E_OK;
}

SystemLoadMonitor::Snapshot SystemLoadMonitorImpl::GetSnapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_;
}

bool SystemLoadMonitorImpl::NormalizeInitialLevel(int32_t rawLevel, int32_t &level) const
{
    if (rawLevel == SYSTEM_LOAD_QUERY_FAILED) {
        ZLOGE("get initial system load failed, error:%{public}d", rawLevel);
        return false;
    }
    level = ClampLevel(rawLevel);
    if (rawLevel != level) {
        ZLOGW("clamp initial system load, raw:%{public}d, clamped:%{public}d", rawLevel, level);
    }
    return true;
}

void SystemLoadMonitorImpl::OnSystemLoadChanged(int32_t level)
{
    int32_t clampedLevel = ClampLevel(level);
    if (level != clampedLevel) {
        ZLOGW("clamp system load callback, raw:%{public}d, clamped:%{public}d", level, clampedLevel);
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

void SystemLoadMonitorImpl::Notify(const Snapshot &snapshot)
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
