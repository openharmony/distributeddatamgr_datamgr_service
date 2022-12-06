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

#define LOG_TAG "processCommunication"

#include "log_print.h"
#include "softbus_adapter.h"
#include "process_communicator_impl.h"
namespace OHOS {
namespace AppDistributedKv {
using namespace DistributedDB;
ProcessCommunicatorImpl::ProcessCommunicatorImpl()
{
}

ProcessCommunicatorImpl::ProcessCommunicatorImpl(RouteHeadHandlerCreator handlerCreator)
    : routeHeadHandlerCreator_(std::move(handlerCreator))
{
}

ProcessCommunicatorImpl::~ProcessCommunicatorImpl()
{
    ZLOGE("destructor.");
}

DBStatus ProcessCommunicatorImpl::Start(const std::string &processLabel)
{
    ZLOGI("init commProvider");
    thisProcessLabel_ = processLabel;
    PipeInfo pi = {thisProcessLabel_, ""};
    Status errCode = CommunicationProvider::GetInstance().Start(pi);
    if (errCode != Status::SUCCESS) {
        ZLOGE("commProvider_ Start Fail.");
        return DBStatus::DB_ERROR;
    }
    return DBStatus::OK;
}

DBStatus ProcessCommunicatorImpl::Stop()
{
    PipeInfo pi = {thisProcessLabel_, ""};
    Status errCode = CommunicationProvider::GetInstance().Stop(pi);
    if (errCode != Status::SUCCESS) {
        ZLOGE("commProvider_ Stop Fail.");
        return DBStatus::DB_ERROR;
    }
    return DBStatus::OK;
}

DBStatus ProcessCommunicatorImpl::RegOnDeviceChange(const OnDeviceChange &callback)
{
    {
        std::lock_guard<std::mutex> onDeviceChangeLockGard(onDeviceChangeMutex_);
        onDeviceChangeHandler_ = callback;
    }

    PipeInfo pi = {thisProcessLabel_, ""};
    if (callback) {
        Status errCode = CommunicationProvider::GetInstance().StartWatchDeviceChange(this, pi);
        if (errCode != Status::SUCCESS) {
            ZLOGE("commProvider_ StartWatchDeviceChange Fail.");
            return DBStatus::DB_ERROR;
        }
    } else {
        Status errCode = CommunicationProvider::GetInstance().StopWatchDeviceChange(this, pi);
        if (errCode != Status::SUCCESS) {
            ZLOGE("commProvider_ StopWatchDeviceChange Fail.");
            return DBStatus::DB_ERROR;
        }
    }

    return DBStatus::OK;
}

DBStatus ProcessCommunicatorImpl::RegOnDataReceive(const OnDataReceive &callback)
{
    {
        std::lock_guard<std::mutex> onDataReceiveLockGard(onDataReceiveMutex_);
        onDataReceiveHandler_ = callback;
    }

    PipeInfo pi = {thisProcessLabel_, ""};
    if (callback) {
        Status errCode = CommunicationProvider::GetInstance().StartWatchDataChange(this, pi);
        if (errCode != Status::SUCCESS) {
            ZLOGE("commProvider_ StartWatchDataChange Fail.");
            return DBStatus::DB_ERROR;
        }
    } else {
        Status errCode = CommunicationProvider::GetInstance().StopWatchDataChange(this, pi);
        if (errCode != Status::SUCCESS) {
            ZLOGE("commProvider_ StopWatchDataChange Fail.");
            return DBStatus::DB_ERROR;
        }
    }
    return DBStatus::OK;
}

DBStatus ProcessCommunicatorImpl::SendData(const DeviceInfos &dstDevInfo, const uint8_t *data, uint32_t length)
{
    PipeInfo pi = {thisProcessLabel_, ""};
    DeviceId destination;
    destination.deviceId = dstDevInfo.identifier;
    Status errCode = CommunicationProvider::GetInstance().SendData(pi, destination, data, static_cast<int>(length));
    if (errCode != Status::SUCCESS) {
        ZLOGE("commProvider_ SendData Fail.");
        return DBStatus::DB_ERROR;
    }

    return DBStatus::OK;
}

DeviceType ProcessCommunicatorImpl::GetDeviceType(int32_t deviceType)
{
    switch (deviceType) {
        case 0x06D:  // SMART_WATCH
        case 0x0A1:  // SMART_WATCH
            return SMART_WATCH;
        case 0x085:  // KID_WATCH
        case 0x0A3:  // KID_WATCH
            return KID_WATCH;
        default:
            return OTHERS;
    }
}

uint32_t ProcessCommunicatorImpl::GetMtuSize()
{
    return MTU_SIZE;
}

uint32_t ProcessCommunicatorImpl::GetMtuSize(const DeviceInfos &devInfo)
{
    ZLOGI("GetMtuSize start");
    DeviceId deviceId = {
        .deviceId = devInfo.identifier
    };

    return SoftBusAdapter::GetInstance()->GetMtuSize(deviceId);
}

DeviceInfos ProcessCommunicatorImpl::GetLocalDeviceInfos()
{
    DeviceInfos localDevInfos;
    DeviceInfo devInfo = CommunicationProvider::GetInstance().GetLocalDevice();
    localDevInfos.identifier = devInfo.uuid;
    return localDevInfos;
}

std::vector<DeviceInfos> ProcessCommunicatorImpl::GetRemoteOnlineDeviceInfosList()
{
    std::vector<DeviceInfos> remoteDevInfos;
    std::vector<DeviceInfo> devInfoVec = CommunicationProvider::GetInstance().GetRemoteDevices();
    for (auto const &entry : devInfoVec) {
        DeviceInfos remoteDev;
        remoteDev.identifier = entry.uuid;
        remoteDevInfos.push_back(remoteDev);
    }
    return remoteDevInfos;
}

bool ProcessCommunicatorImpl::IsSameProcessLabelStartedOnPeerDevice(const DeviceInfos &peerDevInfo)
{
    PipeInfo pi = {thisProcessLabel_, ""};
    DeviceId di = {peerDevInfo.identifier};
    return CommunicationProvider::GetInstance().IsSameStartedOnPeer(pi, di);
}

void ProcessCommunicatorImpl::OnMessage(const DeviceInfo &info, const uint8_t *ptr, const int size,
                                        __attribute__((unused)) const PipeInfo &pipeInfo) const
{
    std::lock_guard<std::mutex> onDataReceiveLockGuard(onDataReceiveMutex_);
    if (onDataReceiveHandler_ == nullptr) {
        ZLOGE("onDataReceiveHandler_ invalid.");
        return;
    }
    DeviceInfos devInfo;
    devInfo.identifier = info.uuid;
    onDataReceiveHandler_(devInfo, ptr, static_cast<uint32_t>(size));
}

void ProcessCommunicatorImpl::OnDeviceChanged(const DeviceInfo &info, const DeviceChangeType &type) const
{
    if (type == DeviceChangeType::DEVICE_ONREADY) {
        return;
    }
    std::lock_guard<std::mutex> onDeviceChangeLockGuard(onDeviceChangeMutex_);
    if (onDeviceChangeHandler_ == nullptr) {
        ZLOGE("onDeviceChangeHandler_ invalid.");
        return;
    }
    DeviceInfos devInfo;
    devInfo.identifier = info.uuid;
    onDeviceChangeHandler_(devInfo, (type == DeviceChangeType::DEVICE_ONLINE));
}

std::shared_ptr<ExtendHeaderHandle> ProcessCommunicatorImpl::GetExtendHeaderHandle(const ExtendInfo &info)
{
    if (routeHeadHandlerCreator_ != nullptr) {
        return routeHeadHandlerCreator_(info);
    }
    return {};
}

DBStatus ProcessCommunicatorImpl::CheckAndGetDataHeadInfo(
    const uint8_t *data, uint32_t dataLen, uint32_t &headLen, std::vector<std::string> &users)
{
    ZLOGD("begin");
    if (routeHeadHandlerCreator_ == nullptr) {
        ZLOGE("header handler creator not registered");
        return DBStatus::DB_ERROR;
    }
    auto handler = routeHeadHandlerCreator_({});
    if (handler == nullptr) {
        ZLOGE("failed to get header handler");
        return DBStatus::DB_ERROR;
    }
    auto ret = handler->ParseHeadData(data, dataLen, headLen, users);
    if (!ret) {
        ZLOGD("illegal head format");
        return DBStatus::INVALID_FORMAT;
    }
    if (users.empty()) {
        ZLOGW("no valid user");
        return DBStatus::NO_PERMISSION;
    }
    ZLOGD("ok, result:%{public}zu, user:%{public}s", users.size(), users.front().c_str());
    return DBStatus::OK;
}
} // namespace AppDistributedKv
} // namespace OHOS
