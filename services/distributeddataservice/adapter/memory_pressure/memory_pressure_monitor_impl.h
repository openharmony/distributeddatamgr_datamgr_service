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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_ADAPTER_MEMORY_PRESSURE_MONITOR_IMPL_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_ADAPTER_MEMORY_PRESSURE_MONITOR_IMPL_H

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "memory_pressure/memory_pressure_monitor.h"
#include "visibility.h"

#if defined(DATAMGR_MEMORY_MANAGER_PART_ENABLED)
#include "app_state_subscriber.h"
#endif

namespace OHOS::DistributedData {
class MemoryPressureMonitorImpl final : public MemoryPressureMonitor {
public:
    static bool Register();

    API_EXPORT int32_t Subscribe(const std::string &name, Observer observer) override;
    API_EXPORT int32_t Unsubscribe(const std::string &name) override;

private:
#if defined(DATAMGR_MEMORY_MANAGER_PART_ENABLED)
    class MemoryPressureSubscriber final : public OHOS::Memory::AppStateSubscriber {
    public:
        explicit MemoryPressureSubscriber(MemoryPressureMonitorImpl &monitor);
        void OnConnected() override;
        void OnDisconnected() override;
        void OnTrim(OHOS::Memory::SystemMemoryLevel level) override;
        void OnRemoteDied(const wptr<IRemoteObject> &object) override;

    private:
        MemoryPressureMonitorImpl &monitor_;
    };
#endif

    void OnTrim(int32_t level);
    void Notify(int32_t level);

    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::map<std::string, Observer> observers_;
    bool started_ = false;
    bool subscribing_ = false;
#if defined(DATAMGR_MEMORY_MANAGER_PART_ENABLED)
    std::shared_ptr<MemoryPressureSubscriber> subscriber_;
#endif
};
} // namespace OHOS::DistributedData

#endif // OHOS_DISTRIBUTED_DATA_SERVICES_ADAPTER_MEMORY_PRESSURE_MONITOR_IMPL_H
