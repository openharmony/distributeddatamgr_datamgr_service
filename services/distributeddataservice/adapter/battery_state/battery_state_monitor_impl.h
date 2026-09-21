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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_ADAPTER_BATTERY_STATE_MONITOR_IMPL_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_ADAPTER_BATTERY_STATE_MONITOR_IMPL_H

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "battery_state/battery_state_monitor.h"
#include "common_event_data.h"
#include "common_event_subscriber.h"
#include "visibility.h"

namespace OHOS::DistributedData {
class BatteryStateEventSubscriber;

class BatteryStateMonitorImpl final : public BatteryStateMonitor {
public:
    static bool Register();

    API_EXPORT int32_t Subscribe(const std::string &name, Observer observer) override;
    API_EXPORT int32_t Unsubscribe(const std::string &name) override;
    API_EXPORT Snapshot GetSnapshot() const override;

private:
    BatteryStateMonitorImpl();
    void PrepareSubscription(const std::string &name, const Observer &observer,
        std::shared_ptr<BatteryStateEventSubscriber> &subscriber, uint64_t &stateVersion, Snapshot &snapshot);
    int32_t CompleteSubscription(const std::string &name,
        const std::shared_ptr<BatteryStateEventSubscriber> &subscriber, uint64_t stateVersion, Snapshot &snapshot);
    std::shared_ptr<BatteryStateEventSubscriber> GetSubscriberLocked();
    void UnsubscribeBatteryEvent();
    void OnBatteryEvent(const EventFwk::CommonEventData &event);
    int32_t QueryCapacityLevel() const;
    bool ApplyInitialLevel(int32_t level, uint64_t stateVersion, Snapshot &snapshot);
    bool UpdateBatteryLevel(int32_t level, Snapshot &snapshot, bool fromEvent = true);
    void Notify(const Snapshot &snapshot);

    mutable std::mutex mutex_;
    Snapshot snapshot_;
    std::map<std::string, Observer> observers_;
    std::shared_ptr<BatteryStateEventSubscriber> batterySubscriber_;
    bool started_ = false;
    bool subscribing_ = false;
    uint64_t stateVersion_ = 0;
    std::condition_variable condition_;
};
} // namespace OHOS::DistributedData

#endif // OHOS_DISTRIBUTED_DATA_SERVICES_ADAPTER_BATTERY_STATE_MONITOR_IMPL_H
