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
#include "communication_strategy.h"
#include "executor_pool.h"
#include "session.h"
#include "softbus_bus_center.h"
namespace OHOS::AppDistributedKv {
class SoftBusClient : public std::enable_shared_from_this<SoftBusClient> {
public:
    SoftBusClient(const PipeInfo &pipeInfo, const DeviceId &deviceId,
        const std::function<int32_t(int32_t)> &getConnStatus);
    ~SoftBusClient();

    using Strategy = CommunicationStrategy::Strategy;
    using Time = std::chrono::steady_clock::time_point;
    using Duration = std::chrono::steady_clock::duration;
    static constexpr Duration P2P_CLOSE_DELAY = std::chrono::seconds(3);
    Status Send(const DataInfo &dataInfo, uint32_t totalLength);
    bool operator==(int32_t connId) const;
    bool operator==(const std::string &deviceId) const;
    uint32_t GetMtuSize() const;
    uint32_t GetTimeout() const;
    Time GetExpireTime() const;
    int32_t GetConnId() const;
    int32_t GetRoutType() const;
    bool Support(uint32_t length) const;
    void UpdateExpireTime();
private:
    enum class ConnectStatus : int32_t {
        CONNECT_OK,
        DISCONNECT,
    };

    Status OpenConnect(uint32_t length);
    Status Open(SessionAttribute attr);
    SessionAttribute GetSessionAttribute(bool isP2p);
    void RestoreDefaultValue();
    std::pair<int32_t, uint32_t> GetMtu(int32_t id);
    std::pair<int32_t, int32_t> GetRouteType(int32_t id);
    Duration GetDelayTime(uint32_t dataLength);

    static constexpr int32_t INVALID_CONNECT_ID = -1;
    static constexpr uint32_t WIFI_TIMEOUT = 8 * 1000;
    static constexpr uint32_t BR_TIMEOUT = 15 * 1000;
    static constexpr uint32_t DEFAULT_TIMEOUT = 15 * 1000;
    static constexpr uint32_t WAIT_MAX_TIME = 10;
    static constexpr uint32_t DEFAULT_MTU_SIZE = 4096u;
    static constexpr uint32_t P2P_SIZE_THRESHOLD = 0x10000u; // 64KB
    static constexpr uint32_t P2P_TRANSFER_PER_MICROSECOND = 3; // 2^3 bytes per microsecond
    static constexpr float SWITCH_DELAY_FACTOR = 0.6f;
    static constexpr Duration SESSION_CLOSE_DELAY = std::chrono::seconds(150);
    static constexpr Duration SESSION_OPEN_DELAY = std::chrono::seconds(20);
    std::atomic_bool sessionFlag_ = false;
    int32_t connId_ = INVALID_CONNECT_ID;
    int32_t routeType_ = RouteType::INVALID_ROUTE_TYPE;
    int32_t expireType_ = RouteType::INVALID_ROUTE_TYPE;
    ConnectStatus status_ = ConnectStatus::DISCONNECT;
    mutable std::mutex mutex_;
    PipeInfo pipe_;
    DeviceId device_;
    uint32_t mtu_;
    std::function<int32_t(int32_t)> getConnStatus_;
    Time expireTime_ = std::chrono::steady_clock::now() + SESSION_OPEN_DELAY;
};
}

#endif // DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_SOFTBUS_CLIENT_H