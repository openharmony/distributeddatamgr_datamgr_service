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

#include <map>
#include <mutex>

#include "commu_types.h"
#include "communication_strategy.h"
#include "executor_pool.h"
#include "session.h"
#include "softbus_bus_center.h"
namespace OHOS::AppDistributedKv {
class SoftBusClient {
public:
    SoftBusClient(const PipeInfo &pipeInfo, const DeviceId &deviceId,
        const std::function<int32_t(int32_t)> &getConnStatus);
    ~SoftBusClient();

    using Strategy = CommunicationStrategy::Strategy;
    Status Send(const DataInfo &dataInfo, uint32_t totalLength);
    bool operator==(int32_t connId) const;
    bool operator==(const std::string &deviceId) const;
    uint32_t GetMtuSize() const;
    void AfterStrategyUpdate(Strategy strategy);
private:
    enum class ConnectStatus : int32_t {
        CONNECT_OK,
        DISCONNECT,
    };

    using Time = std::chrono::steady_clock::time_point;
    Status OpenConnect(uint32_t totalLength);
    Status SwitchChannel(uint32_t totalLength);
    Status CreateChannel(uint32_t totalLength);
    Status Open(SessionAttribute attr);
    SessionAttribute GetSessionAttribute(bool isP2p);
    void RestoreDefaultValue();
    void UpdateMtuSize();
    void CloseP2pSessions();
    void UpdateP2pFinishTime(int32_t connId, uint32_t dataLength);

    static constexpr int32_t INVALID_CONNECT_ID = -1;
    static constexpr uint32_t WAIT_MAX_TIME = 10;
    static constexpr uint32_t DEFAULT_MTU_SIZE = 4096u;
    static constexpr uint32_t P2P_SIZE_THRESHOLD = 0x10000u; // 64KB
    static constexpr uint32_t P2P_TRANSFER_PER_MICROSECOND = 10; // 10 bytes per microsecond
    static constexpr float SWITCH_DELAY_FACTOR = 0.6f;
    static constexpr std::chrono::steady_clock::duration P2P_CLOSE_DELAY = std::chrono::seconds(2);
    int32_t connId_ = INVALID_CONNECT_ID;
    int32_t routeType_ = RouteType::INVALID_ROUTE_TYPE;
    Strategy strategy_ = Strategy::DEFAULT;
    ConnectStatus status_ = ConnectStatus::DISCONNECT;
    std::mutex mutex_;
    std::mutex taskMutex_;
    PipeInfo pipe_;
    DeviceId device_;
    uint32_t mtu_;
    ConcurrentMap<int32_t, Time> p2pFinishTime_;
    ExecutorPool::TaskId closeP2pTaskId_ = ExecutorPool::INVALID_TASK_ID;
    std::function<int32_t(int32_t)> getConnStatus_;
};
}


#endif // DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_SOFTBUS_CLIENT_H
