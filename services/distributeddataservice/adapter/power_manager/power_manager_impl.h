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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_ADAPTER_POWER_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_ADAPTER_POWER_MANAGER_H

#include "common_event_manager.h"
#include "common_event_subscriber.h"
#include "common_event_support.h"
#include "executor_pool.h"
#include "power_manager/power_manager.h"
#include "visibility.h"

#include <list>
#include <mutex>

namespace OHOS::DistributedData {
using namespace OHOS::EventFwk;
using PowerEventCallback = std::function<void(PowerManger::Observer::PowerEvent)>;

class PowerEventSubscriber final : public CommonEventSubscriber {
public:
    ~PowerEventSubscriber() {}
    explicit PowerEventSubscriber(const CommonEventSubscribeInfo &info);
    void SetEventCallback(PowerEventCallback callback);
    void OnReceiveEvent(const CommonEventData &event) override;

private:
    PowerEventCallback eventCallback_{};
};

class PowerManagerImpl : public PowerManger {
public:
    static bool Register();
    API_EXPORT int32_t Subscribe(std::shared_ptr<Observer> observer) override;
    API_EXPORT int32_t Unsubscribe(std::shared_ptr<Observer> observer) override;
    API_EXPORT bool IsCharging() override;
    API_EXPORT void SubscribePowerEvent() override;
    API_EXPORT void UnsubscribePowerEvent() override;
    ~PowerManagerImpl();
    PowerManagerImpl();
private:
    void NotifyPowerChanged(Observer::PowerEvent event);

    std::shared_ptr<PowerEventSubscriber> GetSubscriber();
    void SetSubscriber(std::shared_ptr<PowerEventSubscriber> subscriber);

    class Delegate {
    public:
        int32_t Add(std::shared_ptr<Observer> observer);
        int32_t Remove(std::shared_ptr<Observer> observer);
        std::list<std::weak_ptr<Observer>> GetObs();
        void SetCharging(bool isCharging);
        bool IsCharging();
    private:
        std::mutex mutex_;
        std::list<std::weak_ptr<Observer>> observers_;
        bool isCharging_ = false;
    };
    std::shared_ptr<Delegate> delegate_;
    std::shared_ptr<PowerEventSubscriber> eventSubscriber_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_ADAPTER_POWER_MANAGER_H
