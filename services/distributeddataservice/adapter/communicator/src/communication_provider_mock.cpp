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

#include "communication_provider.h"

namespace OHOS {
namespace AppDistributedKv {
class MockCommunicationProvider : public CommunicationProvider {
public:
    Status StartWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo) override
    {
        return Status::SUCCESS;
    }

    Status StopWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo) override
    {
        return Status::SUCCESS;
    }

    std::pair<Status, int32_t> SendData(const PipeInfo &pipeInfo, const DeviceId &deviceId,
        const DataInfo &dataInfo, uint32_t totalLength, const MessageInfo &info) override
    {
        return {Status::SUCCESS, 0};
    }

    Status Start(const PipeInfo &pipeInfo) override
    {
        return Status::SUCCESS;
    }

    Status Stop(const PipeInfo &pipeInfo) override
    {
        return Status::SUCCESS;
    }

    bool IsSameStartedOnPeer(const PipeInfo &pipeInfo, const DeviceId &peer) const override
    {
        return false;
    }

    void SetMessageTransFlag(const PipeInfo &pipeInfo, bool flag) override
    {
    }

    Status Broadcast(const PipeInfo &pipeInfo, const LevelInfo &levelInfo) override
    {
        return Status::SUCCESS;
    }

    int32_t ListenBroadcastMsg(const PipeInfo &pipeInfo,
        std::function<void(const std::string &, const LevelInfo &)> listener) override
    {
        return 0;
    }

    Status ReuseConnect(const PipeInfo &pipeInfo, const DeviceId &deviceId, const ExtraDataInfo &extraInfo) override
    {
        return Status::ERROR;
    }
};

CommunicationProvider &CommunicationProvider::GetInstance()
{
    static MockCommunicationProvider instance;
    return instance;
}

std::shared_ptr<CommunicationProvider> CommunicationProvider::MakeCommunicationProvider()
{
    return nullptr;
}
}  // namespace AppDistributedKv
}  // namespace OHOS