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

#ifndef DISTRIBUTEDDATA_COMMUNICATION_PROVIDER_H
#define DISTRIBUTEDDATA_COMMUNICATION_PROVIDER_H

#include <functional>
#include <memory>
#include <vector>

#include "app_data_change_listener.h"
#include "app_device_change_listener.h"
#include "idevice_query.h"
namespace OHOS {
namespace AppDistributedKv {
class CommunicationProvider {
public:
    // constructor
    API_EXPORT CommunicationProvider() {}

    // destructor
    API_EXPORT virtual ~CommunicationProvider() {}

    // user should use this method to get instance of CommunicationProvider;
    API_EXPORT static CommunicationProvider &GetInstance();

    API_EXPORT static std::shared_ptr<CommunicationProvider> MakeCommunicationProvider();
    // add DeviceChangeListener to watch device change
    virtual Status StartWatchDeviceChange(const AppDeviceChangeListener *observer, const PipeInfo &pipeInfo) = 0;

    // stop DeviceChangeListener to watch device change
    virtual Status StopWatchDeviceChange(const AppDeviceChangeListener *observer, const PipeInfo &pipeInfo) = 0;

    // add DataChangeListener to watch data change
    virtual Status StartWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo) = 0;

    // stop DataChangeListener to watch data change
    virtual Status StopWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo) = 0;

    // Send data to other device, function will be called back after sent to notify send result
    virtual Status SendData(const PipeInfo &pipeInfo, const DeviceId &deviceId, const uint8_t *ptr, int size,
        const MessageInfo &info = { MessageType::DEFAULT }) = 0;

    // Get online deviceList
    virtual std::vector<DeviceInfo> GetRemoteDevices() const = 0;

    // Get deviceInfo by id
    virtual DeviceInfo GetDeviceInfo(const std::string &networkId) const = 0;

    // Get local device information
    virtual DeviceInfo GetLocalDevice() const = 0;

    // start one server to listen data from other devices;
    virtual Status Start(const PipeInfo &pipeInfo) = 0;

    // stop server
    virtual Status Stop(const PipeInfo &pipeInfo) = 0;

    // check peer device pipeInfo Process
    virtual bool IsSameStartedOnPeer(const PipeInfo &pipeInfo, const DeviceId &peer) const = 0;

    virtual void SetDeviceQuery(std::shared_ptr<IDeviceQuery> deviceQuery) = 0;
    virtual std::string GetUuidByNodeId(const std::string &nodeId) const = 0;
    virtual std::string GetUdidByNodeId(const std::string &nodeId) const = 0;
    virtual DeviceInfo GetLocalBasicInfo() const = 0;
    virtual std::string ToNodeId(const std::string &id) const = 0;
    virtual void SetMessageTransFlag(const PipeInfo &pipeInfo, bool flag) = 0;

    virtual int32_t Broadcast(const PipeInfo &pipeInfo, uint16_t mask) = 0;
    virtual int32_t ListenBroadcastMsg(const PipeInfo &pipeInfo,
        std::function<void(const std::string &, uint16_t)> listener) = 0;
};
} // namespace AppDistributedKv
} // namespace OHOS
#endif // DISTRIBUTEDDATA_COMMUNICATION_PROVIDER_H
