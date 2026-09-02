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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_ADAPTER_SYSTEM_LOAD_MONITOR_IMPL_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_ADAPTER_SYSTEM_LOAD_MONITOR_IMPL_H

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "system_load/system_load_monitor.h"
#include "visibility.h"

#if defined(DATAMGR_RESOURCE_SCHEDULE_PART_ENABLED)
#include "res_sched_systemload_notifier_client.h"
#endif

namespace OHOS::DistributedData {
class SystemLoadMonitorImpl final : public SystemLoadMonitor {
public:
    static bool Register();

    API_EXPORT int32_t Subscribe(const std::string &name, Observer observer) override;
    API_EXPORT int32_t Unsubscribe(const std::string &name) override;
    API_EXPORT Snapshot GetSnapshot() const override;

private:
#if defined(DATAMGR_RESOURCE_SCHEDULE_PART_ENABLED)
    class SystemLoadNotifier final : public OHOS::ResourceSchedule::ResSchedSystemloadNotifierClient {
    public:
        explicit SystemLoadNotifier(SystemLoadMonitorImpl &monitor);
        void OnSystemloadLevel(int32_t level) override;

    private:
        SystemLoadMonitorImpl &monitor_;
    };
#endif

    struct SubscriptionContext {
#if defined(DATAMGR_RESOURCE_SCHEDULE_PART_ENABLED)
        sptr<OHOS::ResourceSchedule::ResSchedSystemloadNotifierClient> notifier;
        uint64_t updateSequence = 0;
#endif
    };

    int32_t PrepareSubscription(
        const std::string &name, const Observer &observer, Snapshot &snapshot, SubscriptionContext &context);
#if defined(DATAMGR_RESOURCE_SCHEDULE_PART_ENABLED)
    void CompleteSubscription(const SubscriptionContext &context, Snapshot &snapshot);
#endif
    void OnSystemLoadChanged(int32_t level);
    bool NormalizeInitialLevel(int32_t rawLevel, int32_t &level) const;
    void Notify(const Snapshot &snapshot);

    mutable std::mutex mutex_;
    std::condition_variable condition_;
    Snapshot snapshot_;
    std::map<std::string, Observer> observers_;
    uint64_t updateSequence_ = 0;
    bool started_ = false;
    bool subscribing_ = false;
#if defined(DATAMGR_RESOURCE_SCHEDULE_PART_ENABLED)
    sptr<OHOS::ResourceSchedule::ResSchedSystemloadNotifierClient> notifier_;
#endif
};
} // namespace OHOS::DistributedData

#endif // OHOS_DISTRIBUTED_DATA_SERVICES_ADAPTER_SYSTEM_LOAD_MONITOR_IMPL_H
