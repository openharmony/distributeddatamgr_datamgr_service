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
    std::shared_ptr<BatteryStateEventSubscriber> GetSubscriberLocked();
    void UnsubscribeBatteryEvent();
    void OnBatteryEvent(const EventFwk::CommonEventData &event);
    bool UpdateBatteryLevel(int32_t level, Snapshot &snapshot);
    void Notify(const Snapshot &snapshot);

    mutable std::mutex mutex_;
    Snapshot snapshot_;
    std::map<std::string, Observer> observers_;
    std::shared_ptr<BatteryStateEventSubscriber> batterySubscriber_;
    bool started_ = false;
    bool subscribing_ = false;
    std::condition_variable condition_;
};
} // namespace OHOS::DistributedData

#endif // OHOS_DISTRIBUTED_DATA_SERVICES_ADAPTER_BATTERY_STATE_MONITOR_IMPL_H
