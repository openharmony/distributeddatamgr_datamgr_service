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

#ifndef PROCESS_COMMUNICATOR_IMPL_H
#define PROCESS_COMMUNICATOR_IMPL_H

#include <mutex>

#include "communication_provider.h"
#include "iprocess_communicator.h"
#include "route_head_handler.h"

namespace OHOS {
namespace AppDistributedKv {
class API_EXPORT ProcessCommunicatorImpl : public DistributedDB::IProcessCommunicator, public AppDataChangeListener,
                                           public AppDeviceChangeListener {

public:
    using DBStatus = DistributedDB::DBStatus;
    using OnDeviceChange = DistributedDB::OnDeviceChange;
    using OnDataReceive = DistributedDB::OnDataReceive;
    using DeviceInfos = DistributedDB::DeviceInfos;
    using RouteHeadHandlerCreator =
        std::function<std::shared_ptr<DistributedData::RouteHeadHandler>(const DistributedDB::ExtendInfo &info)>;

    API_EXPORT ProcessCommunicatorImpl();
    API_EXPORT explicit ProcessCommunicatorImpl(RouteHeadHandlerCreator handlerCreator);
    API_EXPORT ~ProcessCommunicatorImpl() override;

    DBStatus Start(const std::string &processLabel) override;
    DBStatus Stop() override;

    DBStatus RegOnDeviceChange(const OnDeviceChange &callback) override;
    DBStatus RegOnDataReceive(const OnDataReceive &callback) override;

    DBStatus SendData(const DeviceInfos &dstDevInfo, const uint8_t *data, uint32_t length) override;
    uint32_t GetMtuSize() override;
    uint32_t GetMtuSize(const DeviceInfos &devInfo) override;
    DeviceInfos GetLocalDeviceInfos() override;
    std::vector<DeviceInfos> GetRemoteOnlineDeviceInfosList() override;
    bool IsSameProcessLabelStartedOnPeerDevice(const DeviceInfos &peerDevInfo) override;
    void OnDeviceChanged(const DeviceInfo &info, const DeviceChangeType &type) const override;

    API_EXPORT std::shared_ptr<DistributedDB::ExtendHeaderHandle> GetExtendHeaderHandle(
        const DistributedDB::ExtendInfo &info) override;
    API_EXPORT DBStatus CheckAndGetDataHeadInfo(
        const uint8_t *data, uint32_t dataLen, uint32_t &headLen, std::vector<std::string> &users) override;

private:
    void OnMessage(const DeviceInfo &info, const uint8_t *ptr, const int size,
                   const PipeInfo &pipeInfo) const override;
    static DeviceType GetDeviceType(int32_t deviceType);

    std::string thisProcessLabel_;
    OnDeviceChange onDeviceChangeHandler_;
    OnDataReceive onDataReceiveHandler_;
    RouteHeadHandlerCreator routeHeadHandlerCreator_; // route header handler creator

    mutable std::mutex onDeviceChangeMutex_;
    mutable std::mutex onDataReceiveMutex_;

    static constexpr uint32_t MTU_SIZE = 4194304; // the max transmission unit size(4M - 80B)
    static constexpr uint32_t MTU_SIZE_WATCH = 81920; // the max transmission unit size(80K)
};
}  // namespace AppDistributedKv
}  // namespace OHOS
#endif // PROCESS_COMMUNICATOR_IMPL_H
