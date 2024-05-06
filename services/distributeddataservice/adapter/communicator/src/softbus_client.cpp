/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#define LOG_TAG "SoftBusClient"
#include "softbus_client.h"

#include "communicator_context.h"
#include "device_manager_adapter.h"
#include "inner_socket.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "softbus_error_code.h"

namespace OHOS::AppDistributedKv {
using namespace OHOS::DistributedKv;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Context = DistributedData::CommunicatorContext;
SoftBusClient::SoftBusClient(const PipeInfo& pipeInfo, const DeviceId& deviceId, uint32_t type)
    : type_(type), pipe_(pipeInfo), device_(deviceId)
{
    mtu_ = DEFAULT_MTU_SIZE;
}

SoftBusClient::~SoftBusClient()
{
    if (socket_ > 0) {
        Shutdown(socket_);
    }
}

bool SoftBusClient::operator==(int32_t socket) const
{
    return socket_ == socket;
}

bool SoftBusClient::operator==(const std::string &deviceId) const
{
    return device_.deviceId == deviceId;
}

uint32_t SoftBusClient::GetMtuSize() const
{
    ZLOGD("get mtu size socket:%{public}d mtu:%{public}d", socket_, mtu_);
    return mtu_;
}

uint32_t SoftBusClient::GetTimeout() const
{
    return DEFAULT_TIMEOUT;
}

Status SoftBusClient::SendData(const DataInfo &dataInfo, const ISocketListener *listener)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto result = OpenConnect(listener);
    if (result != Status::SUCCESS) {
        return result;
    }
    ZLOGD("send data socket:%{public}d, data size:%{public}u.", socket_, dataInfo.length);
    int32_t ret = SendBytes(socket_, dataInfo.data, dataInfo.length);
    if (ret != SOFTBUS_OK) {
        expireTime_ = std::chrono::steady_clock::now();
        ZLOGE("send data to socket%{public}d failed, ret:%{public}d.", socket_, ret);
        return Status::ERROR;
    }
    expireTime_ = CalcExpireTime();
    return Status::SUCCESS;
}

Status SoftBusClient::OpenConnect(const ISocketListener *listener)
{
    if (bindState_ == 0) {
        return Status ::SUCCESS;
    }
    if (isOpening_.exchange(true)) {
        return Status::RATE_LIMIT;
    }
    if (bindState_ == 0) {
        return Status ::SUCCESS;
    }
    SocketInfo socketInfo;
    std::string peerName = pipe_.pipeId;
    socketInfo.peerName = const_cast<char *>(peerName.c_str());
    std::string networkId = DmAdapter::GetInstance().ToNetworkID(device_.deviceId);
    socketInfo.peerNetworkId = const_cast<char *>(networkId.c_str());
    std::string clientName = (pipe_.pipeId + "_client_" + socketInfo.peerNetworkId).substr(0, 64);
    socketInfo.name = const_cast<char *>(clientName.c_str());
    std::string pkgName = "ohos.distributeddata";
    socketInfo.pkgName = pkgName.data();
    socketInfo.dataType = DATA_TYPE_BYTES;
    int32_t clientSocket = Socket(socketInfo);
    if (clientSocket <= 0) {
        isOpening_.store(false);
        ZLOGE("Create the client Socket:%{public}d failed, peerName:%{public}s", clientSocket, socketInfo.peerName);
        return Status::NETWORK_ERROR;
    }
    auto task = [type = type_, clientSocket, listener, client = shared_from_this()]() {
        if (client == nullptr) {
            ZLOGE("OpenSessionByAsync client is nullptr.");
            return;
        }
        ZLOGI("Bind Start, socket:%{public}d type:%{public}u", clientSocket, type);
        auto status = client->Open(clientSocket, QOS_INFOS[type % QOS_BUTT], listener);
        if (status == Status::SUCCESS) {
            Context::GetInstance().NotifySessionReady(client->device_.deviceId);
        }
        client->isOpening_.store(false);
    };
    Context::GetInstance().GetThreadPool()->Execute(task);
    return Status::RATE_LIMIT;
}

Status SoftBusClient::Open(int32_t socket, const QosTV qos[], const ISocketListener *listener)
{
    int32_t status = ::Bind(socket, qos, QOS_COUNT, listener);
    ZLOGI("Bind %{public}s,session:%{public}s,socketId:%{public}d",
        KvStoreUtils::ToBeAnonymous(device_.deviceId).c_str(), pipe_.pipeId.c_str(), socket);

    if (status != 0) {
        ZLOGE("[Bind] device:%{public}s socket failed, session:%{public}s,result:%{public}d",
            KvStoreUtils::ToBeAnonymous(device_.deviceId).c_str(), pipe_.pipeId.c_str(), status);
        ::Shutdown(socket);
        return Status::NETWORK_ERROR;
    }
    UpdateExpireTime();
    uint32_t mtu = 0;
    std::tie(status, mtu) = GetMtu(socket);
    if (status != SOFTBUS_OK) {
        ZLOGE("GetMtu failed, session:%{public}s, socket:%{public}d", pipe_.pipeId.c_str(), socket_);
        return Status::NETWORK_ERROR;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        socket_ = socket;
        mtu_ = mtu;
        bindState_ = status;
    }
    ZLOGI("open %{public}s, session:%{public}s success, socket:%{public}d",
        KvStoreUtils::ToBeAnonymous(device_.deviceId).c_str(), pipe_.pipeId.c_str(), socket_);
    return Status::SUCCESS;
}

SoftBusClient::Time SoftBusClient::GetExpireTime() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return expireTime_;
}

int32_t SoftBusClient::GetSocket() const
{
    return socket_;
}

void SoftBusClient::UpdateExpireTime()
{
    auto expireTime = CalcExpireTime();
    std::lock_guard<std::mutex> lock(mutex_);
    if (expireTime > expireTime_) {
        expireTime_ = expireTime;
    }
}

std::pair<int32_t, uint32_t> SoftBusClient::GetMtu(int32_t socket)
{
    uint32_t mtu = 0;
    auto ret = ::GetMtuSize(socket, &mtu);
    return { ret, mtu };
}

uint32_t SoftBusClient::GetQoSType() const
{
    return type_ % QOS_COUNT;
}

SoftBusClient::Time SoftBusClient::CalcExpireTime() const
{
    auto delay = type_ == QOS_BR ? BR_CLOSE_DELAY : HML_CLOSE_DELAY;
    return std::chrono::steady_clock::now() + delay;
}
} // namespace OHOS::AppDistributedKv