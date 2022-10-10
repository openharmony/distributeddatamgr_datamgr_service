/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef APP_DEVICE_STATUS_CHANGE_LISTENER_H
#define APP_DEVICE_STATUS_CHANGE_LISTENER_H

#include "commu_types.h"
namespace OHOS {
namespace AppDistributedKv {
enum class ChangeLevelType {
    HIGH,
    LOW,
    MIN,
};
class AppDeviceChangeListener {
public:
    API_EXPORT virtual ~AppDeviceChangeListener() {};
    API_EXPORT virtual void OnDeviceChanged(const DeviceInfo &info, const DeviceChangeType &type) const = 0;
    API_EXPORT virtual ChangeLevelType GetChangeLevelType() const
    {
        return ChangeLevelType::HIGH;
    }
};
}  // namespace AppDistributedKv
}  // namespace OHOS

#endif  // APP_DEVICE_STATUS_CHANGE_LISTENER_H
