/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "socket.h"

#include "securec.h"
#include "softbus_error_code.h"

static constexpr uint32_t EXTRA_INFO_LENGTH = 100;
static constexpr const char *SOCKET_NAME = "socket_test";
static constexpr const char *PKG_NAME = "ddms_test";
static constexpr const char *REMOTE_NETWORK_ID = "remote_network_id";

int32_t socketId_ = DEFAULT_SOCKET;
ISocketListener *clientListener_ = nullptr;
ISocketListener *serverListener_ = nullptr;
SocketAccessInfo accessInfo_;

void ConfigSocketId(int32_t socketId)
{
    socketId_ = socketId;
}

int32_t Socket(SocketInfo info)
{
    (void)info;
    return socketId_;
}

int32_t SetAccessInfo(int32_t socket, SocketAccessInfo accessInfo)
{
    if (socket == INVALID_SET_SOCKET) {
        return SoftBusErrNo::SOFTBUS_ERROR;
    }
    accessInfo_.userId = accessInfo.userId;
    accessInfo_.localTokenId = accessInfo.localTokenId;
    accessInfo_.extraAccessInfo = new char[EXTRA_INFO_LENGTH];
    (void)strcpy_s(accessInfo_.extraAccessInfo, EXTRA_INFO_LENGTH, accessInfo.extraAccessInfo);
    return SoftBusErrNo::SOFTBUS_OK;
}

int32_t Bind(int32_t socket, const QosTV qos[], uint32_t qosCount, const ISocketListener *listener)
{
    (void)qos;
    (void)qosCount;
    if (socket <= INVALID_BIND_SOCKET) {
        return SoftBusErrNo::SOFTBUS_ERROR;
    }
    if (serverListener_ != nullptr) {
        PeerSocketInfo info;
        info.name = const_cast<char *>(SOCKET_NAME);
        info.pkgName = const_cast<char *>(PKG_NAME);
        info.networkId = const_cast<char *>(REMOTE_NETWORK_ID);
        SocketAccessInfo localInfo;
        if (!serverListener_->OnNegotiate2(socket, info, &accessInfo_, &localInfo)) {
            return SoftBusErrNo::SOFTBUS_ERROR;
        }
        serverListener_->OnBind(socket, info);
    }
    clientListener_ = const_cast<ISocketListener*>(listener);
    return SoftBusErrNo::SOFTBUS_OK;
}

void Shutdown(int32_t socket)
{
    if (socket <= INVALID_MTU_SOCKET) {
        return;
    }
    ShutdownReason reason = ShutdownReason::UN_KNOWN;
    if (clientListener_ != nullptr && clientListener_->OnShutdown != nullptr) {
        clientListener_->OnShutdown(socket, reason);
    }
    if (serverListener_ != nullptr && serverListener_->OnShutdown != nullptr) {
        serverListener_->OnShutdown(socket, reason);
    }
}

int32_t SendBytes(int32_t socket, const void *data, uint32_t len)
{
    if (socket <= INVALID_SEND_SOCKET) {
        return SoftBusErrNo::SOFTBUS_ERROR;
    }
    if (clientListener_ == nullptr) {
        return SoftBusErrNo::SOFTBUS_OK;
    }
    if (clientListener_->OnBytes != nullptr) {
        clientListener_->OnBytes(socket, data, len);
    }
    if (clientListener_->OnMessage != nullptr) {
        clientListener_->OnMessage(socket, data, len);
    }
    return SoftBusErrNo::SOFTBUS_OK;
}

int32_t Listen(int32_t socket, const QosTV qos[], uint32_t qosCount, const ISocketListener *listener)
{
    (void)qos;
    (void)qosCount;
    if (socket < VALID_SOCKET) {
        return SoftBusErrNo::SOFTBUS_ERROR;
    }
    serverListener_ = const_cast<ISocketListener*>(listener);
    return SoftBusErrNo::SOFTBUS_OK;
}