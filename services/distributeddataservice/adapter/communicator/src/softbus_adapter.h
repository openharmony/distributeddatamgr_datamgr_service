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

#ifndef DISTRIBUTEDDATAFWK_SRC_SOFTBUS_ADAPTER_H
#define DISTRIBUTEDDATAFWK_SRC_SOFTBUS_ADAPTER_H
#include <concurrent_map.h>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <tuple>
#include <vector>

#include "app_data_change_listener.h"
#include "block_data.h"
#include "task_scheduler.h"
#include "platform_specific.h"
#include "session.h"
#include "softbus_bus_center.h"
namespace OHOS {
namespace AppDistributedKv {
class SoftBusAdapter {
public:
    SoftBusAdapter();
    ~SoftBusAdapter();
    static std::shared_ptr<SoftBusAdapter> GetInstance();

    static std::string ToBeAnonymous(const std::string &name);

    // add DataChangeListener to watch data change;
    Status StartWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo);

    // stop DataChangeListener to watch data change;
    Status StopWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo);

    // Send data to other device, function will be called back after sent to notify send result.
    Status SendData(const PipeInfo &pipeInfo, const DeviceId &deviceId, const uint8_t *data, int size,
        const MessageInfo &info);

    bool IsSameStartedOnPeer(const struct PipeInfo &pipeInfo, const struct DeviceId &peer);

    void SetMessageTransFlag(const PipeInfo &pipeInfo, bool flag);

    int CreateSessionServerAdapter(const std::string &sessionName);

    int RemoveSessionServerAdapter(const std::string &sessionName) const;

    std::string DeleteSession(int32_t connId);

    void NotifyDataListeners(const uint8_t *data, int size, const std::string &deviceId, const PipeInfo &pipeInfo);

    int32_t GetSessionStatus(int32_t connId);

    void OnSessionOpen(int32_t connId, int32_t status);

    void OnSessionClose(int32_t connId);

    int32_t Broadcast(const PipeInfo &pipeInfo, uint16_t mask);
    void OnBroadcast(const DeviceId &device, uint16_t mask);
    int32_t ListenBroadcastMsg(const PipeInfo &pipeInfo, std::function<void(const std::string &, uint16_t)> listener);

    uint32_t GetMtuSize(const DeviceId &deviceId);
private:

    struct ConnectInfo {
        int32_t connId = INVALID_CONNECT_ID;
        uint32_t idleCount = 0;
        bool hasReconnect = false;
        std::string deviceId;
        uint32_t mtu = DEFAULT_MTU_SIZE;
    };
    std::shared_ptr<BlockData<int32_t>> GetSemaphore(int32_t connId);
    uint32_t GetConnectMtuSize(int32_t connId);
    Status GetConnect(const PipeInfo &pipeInfo, const DeviceId &deviceId, int32_t dataSize, int32_t &connId);
    Status OpenConnect(const PipeInfo &pipeInfo, const DeviceId &deviceId, const std::vector <LinkType> &linkTypes,
        int32_t &connId);
    void InitSessionAttribute(const std::vector <LinkType> &linkTypes, SessionAttribute &attr);
    std::function<void()> CloseIdleConnect();
    std::shared_ptr<std::recursive_mutex> GetMutex(const PipeInfo &pipeInfo, const DeviceId &deviceId);

    static std::shared_ptr<SoftBusAdapter> instance_;
    ConcurrentMap<std::string, const AppDataChangeListener *> dataChangeListeners_{};
    ConcurrentMap<std::string, ConnectInfo> connects_{};
    bool flag_ = true; // only for br flag
    ISessionListener sessionListener_{};
    std::mutex statusMutex_{};
    std::map<int32_t, std::shared_ptr<BlockData<int32_t>>> sessionsStatus_;
    std::function<void(const std::string &, uint16_t)> onBroadcast_;
    TaskScheduler scheduler_ { 1 };
    bool schedulerRunning_ = true;
    static constexpr uint32_t CONNECT_IDLE_CLOSE_COUNT = 10;
    static constexpr int32_t INVALID_CONNECT_ID = -1;
    static constexpr uint32_t DEFAULT_MTU_SIZE = 4096;
    std::mutex mutex_ {};
    std::map<std::string, std::shared_ptr<std::recursive_mutex>> mutexes_;
};
} // namespace AppDistributedKv
} // namespace OHOS
#endif /* DISTRIBUTEDDATAFWK_SRC_SOFTBUS_ADAPTER_H */
