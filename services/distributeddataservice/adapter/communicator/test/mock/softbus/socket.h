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

#ifndef MOCK_SOFTBUS_SESSION_SOCKET_H
#define MOCK_SOFTBUS_SESSION_SOCKET_H

#include <stdint.h>

enum TransDataType {
    DATA_TYPE_BYTES,
};

enum ShutdownReason {
    UN_KNOWN,
};

enum QoSEvent {
    QOS_SATISFIED,
    QOS_NOT_SATISFIED,
};

enum QosType {
    QOS_TYPE_MIN_BW,
    QOS_TYPE_MAX_LATENCY,
    QOS_TYPE_MIN_LATENCY,
    QOS_TYPE_REUSE_BE,
};

struct QosTV {
    QosType qos;
    int32_t value;
};

struct PeerSocketInfo {
    char *name;
    char *networkId;
    char *pkgName;
    TransDataType dataType = TransDataType::DATA_TYPE_BYTES;
};

struct SocketInfo {
    char *name;
    char *peerName;
    char *peerNetworkId;
    char *pkgName;
    TransDataType dataType;
};

struct SocketAccessInfo {
    int32_t userId;
    uint64_t localTokenId;
    char *businessAccountId;
    char *extraAccessInfo;
};

struct ISocketListener {
    void (*OnBind)(int32_t socket, PeerSocketInfo info);
    void (*OnShutdown)(int32_t socket, ShutdownReason reason);
    void (*OnBytes)(int32_t socket, const void *data, uint32_t dataLen);
    void (*OnMessage)(int32_t socket, const void *data, uint32_t dataLen);
    void (*OnQos)(int32_t socket, QoSEvent eventId, const QosTV *qos, uint32_t qosCount);
    bool (*OnNegotiate2)(int32_t socket, PeerSocketInfo info, SocketAccessInfo *peerInfo, SocketAccessInfo *localInfo);
};

static constexpr int32_t DEFAULT_SOCKET = -1;
static constexpr int32_t INVALID_SOCKET = 0;
static constexpr int32_t INVALID_SET_SOCKET = 1;
static constexpr int32_t INVALID_BIND_SOCKET = 2;
static constexpr int32_t INVALID_MTU_SOCKET = 3;
static constexpr int32_t INVALID_SEND_SOCKET = 4;
static constexpr int32_t VALID_SOCKET = 5;

void ConfigSocketId(int32_t socketId);

int32_t Socket(SocketInfo info);

int32_t Listen(int32_t socket, const QosTV qos[], uint32_t qosCount, const ISocketListener *listener);

int32_t SetAccessInfo(int32_t socket, SocketAccessInfo accessInfo);

int32_t Bind(int32_t socket, const QosTV qos[], uint32_t qosCount, const ISocketListener *listener);

void Shutdown(int32_t socket);

int32_t SendBytes(int32_t socket, const void *data, uint32_t len);
#endif // MOCK_SOFTBUS_SESSION_SOCKET_H