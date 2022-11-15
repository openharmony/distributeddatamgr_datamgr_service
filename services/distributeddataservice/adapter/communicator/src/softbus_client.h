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

#include "block_data.h"
#include "communication_strategy.h"
#include "session.h"
#include "softbus_bus_center.h"
namespace OHOS::AppDistributedKv {
class SoftBusClient {
public:
    SoftBusClient(const PipeInfo &pipeInfo, const DeviceId &deviceId);
    ~SoftBusClient();

    Status Send(const uint8_t *data, int size);
    bool operator==(int32_t connId);
    bool operator==(const DeviceId &deviceId);
    void OnConnected(int32_t status);
    uint32_t GetMtuSize() const;
private:
    enum class ConnectStatus : int32_t {
        CONNECT_OK,
        DISCONNECT,
    };
    using Strategy = CommunicationStrategy::Strategy;
    Status OpenConnect();
    Status Open(Strategy strategy);
    bool IsReconnect(Strategy strategy);
    void InitSessionAttribute(Strategy strategy, SessionAttribute &attr);
    void RestoreDefaultValue();
    void UpdateMtuSize();

    static constexpr int32_t INVALID_CONNECT_ID = -1;
    static constexpr uint32_t WAIT_MAX_TIME = 10;
    static constexpr int32_t DEFAULT_MTU_SIZE = 4096;
    int32_t connId_ = INVALID_CONNECT_ID;
    Strategy strategy_ = Strategy::DEFAULT;
    ConnectStatus status_ = ConnectStatus::DISCONNECT;
    std::mutex mutex_;
    std::shared_ptr <BlockData<int32_t>> block_;
    PipeInfo pipe_;
    DeviceId device_;
    uint32_t mtu_;
};
}


#endif // DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_SOFTBUS_CLIENT_H
