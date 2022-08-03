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

#include "virtual_communicator.h"

#include "log_print.h"
#include "protocol_proto.h"
#include "single_ver_serialize_manager.h"
#include "sync_engine.h"
#include "virtual_communicator_aggregator.h"

namespace DistributedDB {
int VirtualCommunicator::RegOnMessageCallback(const OnMessageCallback &onMessage, const Finalizer &inOper)
{
    std::lock_guard<std::mutex> lock(onMessageLock_);
    onMessage_ = onMessage;
    return E_OK;
}

int VirtualCommunicator::RegOnConnectCallback(const OnConnectCallback &onConnect, const Finalizer &inOper)
{
    std::lock_guard<std::mutex> lock(onConnectLock_);
    onConnect_ = onConnect;
    return E_OK;
}

int VirtualCommunicator::RegOnSendableCallback(const std::function<void(void)> &onSendable, const Finalizer &inOper)
{
    return E_OK;
}

void VirtualCommunicator::Activate()
{
}

int VirtualCommunicator::SendMessage(const std::string &dstTarget, const Message *inMsg, const SendConfig &config)
{
    return SendMessage(dstTarget, inMsg, config, nullptr);
}

int VirtualCommunicator::SendMessage(const std::string &dstTarget, const Message *inMsg, const SendConfig &config,
    const OnSendEnd &onEnd)
{
    AutoLock lock(this);
    if (IsKilled()) {
        return -E_OBJ_IS_KILLED;
    }
    if (!isEnable_) {
        LOGD("[VirtualCommunicator] the VirtualCommunicator disabled!");
        return -E_PERIPHERAL_INTERFACE_FAIL;
    }
    if (dstTarget == deviceId_) {
        delete inMsg;
        inMsg = nullptr;
        return E_OK;
    }
    int errCode = E_OK;
    std::shared_ptr<ExtendHeaderHandle> extendHandle = nullptr;

    auto buffer = ProtocolProto::ToSerialBuffer(inMsg, errCode, extendHandle);
    if (errCode != E_OK && errCode != -E_NOT_REGISTER) {
        return errCode;
    }
    const Message *message;
    if (errCode == -E_NOT_REGISTER) {
        message = inMsg;
        errCode = E_OK;
    } else {
        message = ProtocolProto::ToMessage(buffer, errCode);
    }
    delete buffer;
    buffer = nullptr;
    if (errCode != E_OK) {
        return errCode;
    }
    communicatorAggregator_->DispatchMessage(deviceId_, dstTarget, message, onEnd);
    return E_OK;
}

int VirtualCommunicator::GetRemoteCommunicatorVersion(const std::string &deviceId, uint16_t &version) const
{
    version = UINT16_MAX;
    return E_OK;
}

void VirtualCommunicator::CallbackOnMessage(const std::string &srcTarget, Message *inMsg) const
{
    std::lock_guard<std::mutex> lock(onMessageLock_);
    if (isEnable_ && onMessage_ && (srcTarget != deviceId_)) {
        onMessage_(srcTarget, inMsg);
    } else {
        delete inMsg;
        inMsg = nullptr;
    }
}

void VirtualCommunicator::CallbackOnConnect(const std::string &target, bool isConnect) const
{
    {
        std::lock_guard<std::mutex> lock(devicesMapLock_);
        if (target != deviceId_) {
            onlineDevicesMap_[target] = isConnect;
        }
    }
    std::lock_guard<std::mutex> lock(onConnectLock_);
    if (isEnable_ && onConnect_) {
        onConnect_(target, isConnect);
    }
}

uint32_t VirtualCommunicator::GetCommunicatorMtuSize() const
{
    return 5 * 1024 * 1024; // 5 * 1024 * 1024B
}

uint32_t VirtualCommunicator::GetCommunicatorMtuSize(const std::string &target) const
{
    return GetCommunicatorMtuSize();
}

uint32_t VirtualCommunicator::GetTimeout() const
{
    return 5 * 1000; // 5 * 1000ms
}

uint32_t VirtualCommunicator::GetTimeout(const std::string &target) const
{
    return GetTimeout();
}

int VirtualCommunicator::GetLocalIdentity(std::string &outTarget) const
{
    outTarget = deviceId_;
    return E_OK;
}

int VirtualCommunicator::GeneralVirtualSyncId()
{
    std::lock_guard<std::mutex> lock(syncIdLock_);
    currentSyncId_++;
    return currentSyncId_;
}

void VirtualCommunicator::Disable()
{
    isEnable_ = false;
}

void VirtualCommunicator::Enable()
{
    isEnable_ = true;
}

void VirtualCommunicator::SetDeviceId(const std::string &deviceId)
{
    deviceId_ = deviceId;
}

std::string VirtualCommunicator::GetDeviceId() const
{
    return deviceId_;
}

bool VirtualCommunicator::IsEnabled() const
{
    return isEnable_;
}

bool VirtualCommunicator::IsDeviceOnline(const std::string &device) const
{
    bool res = true;
    {
        std::lock_guard<std::mutex> lock(devicesMapLock_);
        if (onlineDevicesMap_.find(device) != onlineDevicesMap_.end()) {
            res = onlineDevicesMap_[device];
        }
    }
    return res;
}

VirtualCommunicator::~VirtualCommunicator()
{
}

VirtualCommunicator::VirtualCommunicator(const std::string &deviceId,
    VirtualCommunicatorAggregator *communicatorAggregator)
    : deviceId_(deviceId), communicatorAggregator_(communicatorAggregator)
{
}
} // namespace DistributedDB