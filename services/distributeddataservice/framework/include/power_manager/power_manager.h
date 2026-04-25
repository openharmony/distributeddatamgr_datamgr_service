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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_POWER_MANAGER_POWER_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_POWER_MANAGER_POWER_MANAGER_H
#include "visibility.h"
#include <cstdint>
#include <memory>

namespace OHOS::DistributedData {
class PowerManager {
public:
    API_EXPORT static PowerManager *GetInstance();
    API_EXPORT static bool RegisterInstance(PowerManager *instance);

    class Observer {
    public:
        enum class PowerEvent {
            CHARGING,
            DIS_CHARGING,
            BUTT,
        };

        virtual ~Observer() = default;
        virtual void OnChange(PowerEvent event) = 0;
    };

    virtual int32_t Subscribe(std::shared_ptr<Observer> observer) = 0;
    virtual int32_t Unsubscribe(std::shared_ptr<Observer> observer) = 0;
    virtual bool IsCharging() = 0;
    virtual void SubscribePowerEvent() = 0;
    virtual void UnsubscribePowerEvent() = 0;

private:
    static PowerManager *instance_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_POWER_MANAGER_POWER_MANAGER_H