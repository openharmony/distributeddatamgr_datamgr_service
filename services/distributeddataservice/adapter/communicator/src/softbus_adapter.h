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

    void InsertSession(const std::string &sessionName, int32_t connId);

    std::string DeleteSession(int32_t connId);

    void NotifyDataListeners(const uint8_t *data, int size, const std::string &deviceId, const PipeInfo &pipeInfo);

    int32_t GetSessionStatus(int32_t connId);

    void OnSessionOpen(int32_t connId, int32_t status);

    void OnSessionClose(int32_t connId);

    int32_t Broadcast(const PipeInfo &pipeInfo, uint16_t mask);
    void OnBroadcast(const DeviceId &device, uint16_t mask);
    int32_t ListenBroadcastMsg(const PipeInfo &pipeInfo, std::function<void(const std::string &, uint16_t)> listener);

private:
    std::shared_ptr<BlockData<int32_t>> GetSemaphore(int32_t connId);
    Status GetConnect(const PipeInfo &pipeInfo, const DeviceId &deviceId, int32_t dataSize, int32_t &connId);
    Status OpenConnect(const PipeInfo &pipeInfo, const DeviceId &deviceId, int32_t dataSize, int32_t &connId);
    void InitSessionAttribute(const PipeInfo &pipeInfo, const DeviceId &deviceId, int32_t dataSize,
        SessionAttribute &attr);
    static std::shared_ptr<SoftBusAdapter> instance_;
    ConcurrentMap<std::string, const AppDataChangeListener *> dataChangeListeners_{};
    ConcurrentMap<std::string, int32_t> connects_{};
    bool flag_ = true; // only for br flag
    ISessionListener sessionListener_{};
    std::mutex statusMutex_{};
    std::map<int32_t, std::shared_ptr<BlockData<int32_t>>> sessionsStatus_;
    std::function<void(const std::string &, uint16_t)> onBroadcast_;
};
} // namespace AppDistributedKv
} // namespace OHOS
#endif /* DISTRIBUTEDDATAFWK_SRC_SOFTBUS_ADAPTER_H */
