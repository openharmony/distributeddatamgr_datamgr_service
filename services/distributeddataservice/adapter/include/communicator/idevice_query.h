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

#ifndef IDEVICE_QUERY_H
#define IDEVICE_QUERY_H

#include "commu_types.h"
namespace OHOS {
namespace AppDistributedKv {
class IDeviceQuery {
public:
    API_EXPORT virtual ~IDeviceQuery() {}

    // Get online deviceList
    virtual std::vector<DeviceInfo> GetRemoteDevices() const = 0;

    // Get local device information
    virtual DeviceInfo GetLocalDevice() const = 0;
    // Get deviceType by networkId
    virtual DeviceInfo GetDeviceInfo(const std::string &networkId) const = 0;
};
}  // namespace AppDistributedKv
}  // namespace OHOS
#endif // IDEVICE_QUERY_H
