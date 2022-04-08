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

#ifndef DISTRIBUTEDDATAFWK_SRC_DEVICE_HANDLER_H
#define DISTRIBUTEDDATAFWK_SRC_DEVICE_HANDLER_H
#include "softbus_adapter.h"

namespace OHOS {
namespace AppDistributedKv {
class AppDeviceHandler {
public:
    ~AppDeviceHandler();
    explicit AppDeviceHandler();
    void Init();

    // add DeviceChangeListener to watch device change;
    Status StartWatchDeviceChange(const AppDeviceChangeListener *observer, const PipeInfo &pipeInfo);
    // stop DeviceChangeListener to watch device change;
    Status StopWatchDeviceChange(const AppDeviceChangeListener *observer, const PipeInfo &pipeInfo);

    DeviceInfo GetLocalDevice();
    std::vector<DeviceInfo> GetRemoteDevices() const;
    DeviceInfo GetDeviceInfo(const std::string &networkId) const;

    std::string GetUuidByNodeId(const std::string &nodeId) const;
    std::string GetUdidByNodeId(const std::string &nodeId) const;
    // get local device node information;
    DeviceInfo GetLocalBasicInfo() const;
    // transfer nodeId or udid to uuid
    // input: id
    // output: uuid
    // return: transfer success or not
    std::string ToUUID(const std::string &id) const;
    // transfer uuid or udid to nodeId
    // input: id
    // output: nodeId
    // return: transfer success or not
    std::string ToNodeID(const std::string &nodeId, const std::string &defaultId) const;

    static std::string ToBeAnonymous(const std::string &name);

private:
    void UpdateRelationship(const DeviceInfo &deviceInfo, const DeviceChangeType &type);
    std::shared_ptr<SoftBusAdapter> softbusAdapter_ {};
};
}  // namespace AppDistributedKv
}  // namespace OHOS
#endif // DISTRIBUTEDDATAFWK_SRC_DEVICE_HANDLER_H
