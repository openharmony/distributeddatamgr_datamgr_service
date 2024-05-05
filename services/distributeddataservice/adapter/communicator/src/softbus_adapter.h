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
#include "app_device_change_listener.h"
#include "block_data.h"
#include "socket.h"
#include "softbus_bus_center.h"
#include "softbus_client.h"
namespace OHOS {
namespace AppDistributedKv {
class SoftBusAdapter : public AppDistributedKv::AppDeviceChangeListener {
public:
    SoftBusAdapter();
    ~SoftBusAdapter();
    static std::shared_ptr<SoftBusAdapter> GetInstance();

    struct ServerSocketInfo {
        std::string name;      /**< Peer socket name */
        std::string networkId; /**< Peer network ID */
        std::string pkgName;   /**< Peer package name */
    };

    // add DataChangeListener to watch data change;
    Status StartWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo);

    // stop DataChangeListener to watch data change;
    Status StopWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo);

    // Send data to other device, function will be called back after sent to notify send result.
    Status SendData(const PipeInfo &pipeInfo, const DeviceId &deviceId, const DataInfo &dataInfo, uint32_t length,
        const MessageInfo &info);

    bool IsSameStartedOnPeer(const struct PipeInfo &pipeInfo, const struct DeviceId &peer);

    void SetMessageTransFlag(const PipeInfo &pipeInfo, bool flag);

    int CreateSessionServerAdapter(const std::string &sessionName);

    int RemoveSessionServerAdapter(const std::string &sessionName) const;

    void NotifyDataListeners(const uint8_t *data, int size, const std::string &deviceId, const PipeInfo &pipeInfo);

    std::string OnClientShutdown(int32_t socket);

    void OnBind(int32_t socket, PeerSocketInfo info);

    void OnServerShutdown(int32_t socket);

    bool GetPeerSocketInfo(int32_t socket, ServerSocketInfo &info);

    Status Broadcast(const PipeInfo &pipeInfo, const LevelInfo &levelInfo);
    void OnBroadcast(const DeviceId &device, const LevelInfo &levelInfo);
    int32_t ListenBroadcastMsg(const PipeInfo &pipeInfo,
        std::function<void(const std::string &, const LevelInfo &)> listener);

    uint32_t GetMtuSize(const DeviceId &deviceId);
    uint32_t GetTimeout(const DeviceId &deviceId);

    void OnDeviceChanged(const AppDistributedKv::DeviceInfo &info,
        const AppDistributedKv::DeviceChangeType &type) const override;

private:
    using Time = std::chrono::steady_clock::time_point;
    using Duration = std::chrono::steady_clock::duration;
    using Task = ExecutorPool::Task;
    std::string DelConnect(int32_t socket);
    void StartCloseSessionTask(const std::string &deviceId);
    Task GetCloseSessionTask();
    static constexpr const char *PKG_NAME = "distributeddata-default";
    static constexpr Time INVALID_NEXT = std::chrono::steady_clock::time_point::max();
    static constexpr uint32_t QOS_COUNT = 3;
    static constexpr QosTV Qos[QOS_COUNT] = { { .qos = QOS_TYPE_MIN_BW, .value = 64 * 1024 },
        { .qos = QOS_TYPE_MAX_LATENCY, .value = 15000 }, { .qos = QOS_TYPE_MIN_LATENCY, .value = 1600 } };
    static std::shared_ptr<SoftBusAdapter> instance_;
    ConcurrentMap<std::string, const AppDataChangeListener *> dataChangeListeners_{};
    ConcurrentMap<std::string, std::vector<std::shared_ptr<SoftBusClient>>> connects_;
    bool flag_ = true; // only for br flag
    std::function<void(const std::string &, const LevelInfo &)> onBroadcast_;
    std::mutex taskMutex_;
    ExecutorPool::TaskId taskId_ = ExecutorPool::INVALID_TASK_ID;
    Time next_ = INVALID_NEXT;

    ConcurrentMap<int32_t, ServerSocketInfo> peerSocketInfos_;
    ISocketListener clientListener_{};
    ISocketListener serverListener_{};
    int32_t socket_ = 0;
};
} // namespace AppDistributedKv
} // namespace OHOS
#endif /* DISTRIBUTEDDATAFWK_SRC_SOFTBUS_ADAPTER_H */