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

#define LOG_TAG "MemoryPressureMonitorImpl"
#include "memory_pressure_monitor_impl.h"

#include <new>
#include <utility>
#include <vector>

#include "error/general_error.h"
#include "log_print.h"

#if defined(DATAMGR_MEMORY_MANAGER_PART_ENABLED)
#include "mem_mgr_client.h"
#endif

namespace OHOS::DistributedData {
#if defined(DATAMGR_MEMORY_MANAGER_PART_ENABLED)
MemoryPressureMonitorImpl::MemoryPressureSubscriber::MemoryPressureSubscriber(MemoryPressureMonitorImpl &monitor)
    : monitor_(monitor)
{
}

void MemoryPressureMonitorImpl::MemoryPressureSubscriber::OnConnected()
{
    ZLOGI("memory manager connected");
}

void MemoryPressureMonitorImpl::MemoryPressureSubscriber::OnDisconnected()
{
    ZLOGW("memory manager disconnected; do not resubscribe");
}

void MemoryPressureMonitorImpl::MemoryPressureSubscriber::OnTrim(OHOS::Memory::SystemMemoryLevel level)
{
    monitor_.OnTrim(static_cast<int32_t>(level));
}

void MemoryPressureMonitorImpl::MemoryPressureSubscriber::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    (void)object;
    ZLOGW("memory manager died; do not resubscribe");
}
#endif

__attribute__((used)) static bool g_isInit = MemoryPressureMonitorImpl::Register();

bool MemoryPressureMonitorImpl::Register()
{
    static MemoryPressureMonitorImpl instance;
    MemoryPressureMonitor::RegisterInstance(&instance);
    return true;
}

int32_t MemoryPressureMonitorImpl::Subscribe(const std::string &name, Observer observer)
{
    if (name.empty() || observer == nullptr) {
        return E_INVALID_ARGS;
    }
#if defined(DATAMGR_MEMORY_MANAGER_PART_ENABLED)
    std::shared_ptr<MemoryPressureSubscriber> subscriber;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this]() {
            return !subscribing_;
        });
        observers_[name] = std::move(observer);
        if (started_) {
            return E_OK;
        }
        if (subscriber_ == nullptr) {
            subscriber_ = std::make_shared<MemoryPressureSubscriber>(*this);
        }
        subscriber = subscriber_;
        subscribing_ = true;
    }
    int32_t status = OHOS::Memory::MemMgrClient::GetInstance().SubscribeAppState(*subscriber);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        started_ = status == 0;
        subscribing_ = false;
        if (!started_) {
            observers_.erase(name);
        }
    }
    condition_.notify_all();
    if (status != 0) {
        ZLOGW("subscribe memory pressure failed, status:%{public}d; do not retry", status);
        return E_ERROR;
    }
#else
    {
        std::lock_guard<std::mutex> lock(mutex_);
        observers_[name] = std::move(observer);
        started_ = true;
    }
    ZLOGW("memory manager capability is disabled");
#endif
    return E_OK;
}

int32_t MemoryPressureMonitorImpl::Unsubscribe(const std::string &name)
{
#if defined(DATAMGR_MEMORY_MANAGER_PART_ENABLED)
    std::shared_ptr<MemoryPressureSubscriber> subscriber;
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
#if defined(DATAMGR_MEMORY_MANAGER_PART_ENABLED)
        subscriber = subscriber_;
#endif
    }
#if defined(DATAMGR_MEMORY_MANAGER_PART_ENABLED)
    int32_t status = OHOS::Memory::MemMgrClient::GetInstance().UnsubscribeAppState(*subscriber);
    if (status != 0) {
        ZLOGW("unsubscribe memory pressure failed, status:%{public}d", status);
        return E_ERROR;
    }
#endif
    return E_OK;
}

void MemoryPressureMonitorImpl::OnTrim(int32_t level)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!started_ && !subscribing_) {
            return;
        }
    }
    Notify(level);
}

void MemoryPressureMonitorImpl::Notify(int32_t level)
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
            observer(level);
        }
    }
}
} // namespace OHOS::DistributedData
