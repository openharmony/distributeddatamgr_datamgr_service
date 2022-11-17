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

#include "communication_provider_impl.h"

#include "communication_strategy.h"
#include "device_manager_adapter.h"
#include "log_print.h"

#undef LOG_TAG
#define LOG_TAG "CommunicationProviderImpl"

namespace OHOS {
namespace AppDistributedKv {
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
std::mutex CommunicationProviderImpl::mutex_;
CommunicationProviderImpl::CommunicationProviderImpl(AppPipeMgr &appPipeMgr) : appPipeMgr_(appPipeMgr)
{
}

CommunicationProviderImpl::~CommunicationProviderImpl()
{
    ZLOGD("destructor.");
}

Status CommunicationProviderImpl::Initialize()
{
    CommunicationStrategy::GetInstance().Init();
    DmAdapter::GetInstance().Init();
    return Status::SUCCESS;
}

Status CommunicationProviderImpl::StartWatchDeviceChange(const AppDeviceChangeListener *observer,
    const PipeInfo &pipeInfo)
{
    return DmAdapter::GetInstance().StartWatchDeviceChange(observer, pipeInfo);
}

Status CommunicationProviderImpl::StopWatchDeviceChange(const AppDeviceChangeListener *observer,
    const PipeInfo &pipeInfo)
{
    return DmAdapter::GetInstance().StopWatchDeviceChange(observer, pipeInfo);
}

DeviceInfo CommunicationProviderImpl::GetLocalDevice() const
{
    return DmAdapter::GetInstance().GetLocalDevice();
}

std::vector<DeviceInfo> CommunicationProviderImpl::GetRemoteDevices() const
{
    return DmAdapter::GetInstance().GetRemoteDevices();
}

DeviceInfo CommunicationProviderImpl::GetDeviceInfo(const std::string &networkId) const
{
    return DmAdapter::GetInstance().GetDeviceInfo(networkId);
}

Status CommunicationProviderImpl::StartWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo)
{
    return appPipeMgr_.StartWatchDataChange(observer, pipeInfo);
}

Status CommunicationProviderImpl::StopWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo)
{
    return appPipeMgr_.StopWatchDataChange(observer, pipeInfo);
}

Status CommunicationProviderImpl::SendData(const PipeInfo &pipeInfo, const DeviceId &deviceId, const uint8_t *ptr,
    int size, const MessageInfo &info)
{
    return appPipeMgr_.SendData(pipeInfo, deviceId, ptr, size, info);
}

Status CommunicationProviderImpl::Start(const PipeInfo &pipeInfo)
{
    return appPipeMgr_.Start(pipeInfo);
}

Status CommunicationProviderImpl::Stop(const PipeInfo &pipeInfo)
{
    return appPipeMgr_.Stop(pipeInfo);
}

bool CommunicationProviderImpl::IsSameStartedOnPeer(const PipeInfo &pipeInfo, const DeviceId &peer) const
{
    return appPipeMgr_.IsSameStartedOnPeer(pipeInfo, peer);
}

std::string CommunicationProviderImpl::GetUuidByNodeId(const std::string &nodeId) const
{
    return DmAdapter::GetInstance().GetUuidByNetworkId(nodeId);
}

DeviceInfo CommunicationProviderImpl::GetLocalBasicInfo() const
{
    return DmAdapter::GetInstance().GetLocalBasicInfo();
}

std::string CommunicationProviderImpl::ToNodeId(const std::string &id) const
{
    return DmAdapter::GetInstance().ToNetworkID(id);
}

std::string CommunicationProviderImpl::GetUdidByNodeId(const std::string &nodeId) const
{
    return DmAdapter::GetInstance().GetUdidByNetworkId(nodeId);
}

void CommunicationProviderImpl::SetMessageTransFlag(const PipeInfo &pipeInfo, bool flag)
{
    appPipeMgr_.SetMessageTransFlag(pipeInfo, flag);
}

int32_t CommunicationProviderImpl::Broadcast(const PipeInfo &pipeInfo, uint16_t mask)
{
    return SoftBusAdapter::GetInstance()->Broadcast(pipeInfo, mask);
}

int32_t CommunicationProviderImpl::ListenBroadcastMsg(const PipeInfo &pipeInfo,
    std::function<void(const std::string &, uint16_t)> listener)
{
    return SoftBusAdapter::GetInstance()->ListenBroadcastMsg(pipeInfo, std::move(listener));
}
} // namespace AppDistributedKv
} // namespace OHOS
