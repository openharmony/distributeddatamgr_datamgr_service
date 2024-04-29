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

#ifndef DISTRIBUTEDDATA_SRC_COMMUNICATION_PROVIDER_IMPL_H
#define DISTRIBUTEDDATA_SRC_COMMUNICATION_PROVIDER_IMPL_H

#include <set>

#include "app_pipe_mgr.h"
#include "communication_provider.h"

namespace OHOS {
namespace AppDistributedKv {
class CommunicationProviderImpl : public CommunicationProvider {
public:
    explicit CommunicationProviderImpl(AppPipeMgr &appPipeMgr);

    virtual ~CommunicationProviderImpl();

    // add DataChangeListener to watch data change;
    Status StartWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo) override;

    // stop watching data change;
    Status StopWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo) override;

    // Send data to other device, function will be called back after sent to notify send result.
    Status SendData(const PipeInfo &pipeInfo, const DeviceId &deviceId, const DataInfo &dataInfo,
        uint32_t totalLength, const MessageInfo &info) override;

    // start 1 server to listen data from other devices;
    Status Start(const PipeInfo &pipeInfo) override;

    // stop server
    Status Stop(const PipeInfo &pipeInfo) override;

    bool IsSameStartedOnPeer(const PipeInfo &pipeInfo, const DeviceId &peer) const override;
    void SetMessageTransFlag(const struct PipeInfo &pipeInfo, bool flag) override;

    Status Broadcast(const PipeInfo &pipeInfo, const LevelInfo &levelInfo) override;
    int32_t ListenBroadcastMsg(const PipeInfo &pipeInfo,
        std::function<void(const std::string &, const LevelInfo &)> listener) override;

protected:
    virtual Status Initialize();

    static std::mutex mutex_;

private:
    AppPipeMgr &appPipeMgr_;
};
} // namespace AppDistributedKv
} // namespace OHOS
#endif /* DISTRIBUTEDDATA_SRC_COMMUNICATION_PROVIDER_IMPL_H */
