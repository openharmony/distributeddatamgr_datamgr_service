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

#ifndef DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_SOFTBUS_CLIENT_H
#define DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_SOFTBUS_CLIENT_H

#include <atomic>
#include <map>
#include <mutex>

#include "commu_types.h"
#include "executor_pool.h"
#include "socket.h"
#include "softbus_bus_center.h"
namespace OHOS::AppDistributedKv {
class SoftBusClient : public std::enable_shared_from_this<SoftBusClient> {
public:
    enum QoSType {
        QOS_BR,
        QOS_HML,
        QOS_BUTT
    };
    SoftBusClient(const PipeInfo &pipeInfo, const DeviceId &deviceId, uint32_t type = QOS_HML);
    ~SoftBusClient();

    using Time = std::chrono::steady_clock::time_point;
    using Duration = std::chrono::steady_clock::duration;
    Status SendData(const DataInfo &dataInfo, const ISocketListener *listener);
    bool operator==(int32_t socket) const;
    bool operator==(const std::string &deviceId) const;
    uint32_t GetMtuSize() const;
    uint32_t GetTimeout() const;
    Time GetExpireTime() const;
    int32_t GetSocket() const;
    uint32_t GetQoSType() const;
    void UpdateExpireTime();

private:
    Status OpenConnect(const ISocketListener *listener);
    Status Open(int32_t socket, const QosTV qos[], const ISocketListener *listener);
    std::pair<int32_t, uint32_t> GetMtu(int32_t socket);
    Time CalcExpireTime() const;

    static constexpr int32_t INVALID_SOCKET_ID = -1;
    static constexpr uint32_t DEFAULT_TIMEOUT = 30 * 1000;
    static constexpr uint32_t DEFAULT_MTU_SIZE = 4096 * 1024u;
    static constexpr Duration BR_CLOSE_DELAY = std::chrono::seconds(5);
    static constexpr Duration HML_CLOSE_DELAY = std::chrono::seconds(3);
    static constexpr Duration MAX_DELAY = std::chrono::seconds(20);
    static constexpr uint32_t QOS_COUNT = 3;
    static constexpr QosTV QOS_INFOS[QOS_BUTT][QOS_COUNT] = {
        { // BR QOS
            QosTV{ .qos = QOS_TYPE_MIN_BW, .value = 0x5a5a5a5a },
            QosTV{ .qos = QOS_TYPE_MAX_LATENCY, .value = 15000 },
            QosTV{ .qos = QOS_TYPE_MIN_LATENCY, .value = 1600 }
        },
        { // HML QOS
            QosTV{ .qos = QOS_TYPE_MIN_BW, .value = 90 * 1024 * 1024 },
            QosTV{ .qos = QOS_TYPE_MAX_LATENCY, .value = 10000 },
            QosTV{ .qos = QOS_TYPE_MIN_LATENCY, .value = 2000 }
        }
    };
    std::atomic_bool isOpening_ = false;
    mutable std::mutex mutex_;
    uint32_t type_ = QOS_HML;
    PipeInfo pipe_;
    DeviceId device_;
    uint32_t mtu_;
    Time expireTime_ = std::chrono::steady_clock::now() + MAX_DELAY;

    int32_t socket_ = INVALID_SOCKET_ID;
    int32_t bindState_ = -1;
};
} // namespace OHOS::AppDistributedKv

#endif // DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_SOFTBUS_CLIENT_H