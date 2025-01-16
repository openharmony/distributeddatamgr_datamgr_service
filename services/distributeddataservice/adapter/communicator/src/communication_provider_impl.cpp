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
#include "device_manager_adapter.h"
#include "log_print.h"
#include "kvstore_utils.h"
#include "reporter.h"

#undef LOG_TAG
#define LOG_TAG "CommunicationProviderImpl"

namespace OHOS {
namespace AppDistributedKv {
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using namespace OHOS::DistributedKv;
CommunicationProviderImpl::CommunicationProviderImpl()
{
}

CommunicationProviderImpl::~CommunicationProviderImpl()
{
    ZLOGD("destructor.");
}

Status CommunicationProviderImpl::StartWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo)
{
    ZLOGI("begin");
    if (observer == nullptr || pipeInfo.pipeId.empty()) {
        ZLOGE("argument invalid");
        return Status::INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(dataBusMapMutex_);
    auto it = dataBusMap_.find(pipeInfo.pipeId);
    if (it == dataBusMap_.end()) {
        ZLOGE("pipeid not found");
        return Status::ERROR;
    }
    ZLOGI("end");
    return it->second->StartWatchDataChange(observer, pipeInfo);
}

Status CommunicationProviderImpl::StopWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo)
{
    ZLOGI("begin");
    if (observer == nullptr || pipeInfo.pipeId.empty()) {
        ZLOGE("argument invalid");
        return Status::INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(dataBusMapMutex_);
    auto it = dataBusMap_.find(pipeInfo.pipeId);
    if (it == dataBusMap_.end()) {
        ZLOGE("pipeid not found");
        return Status::ERROR;
    }
    ZLOGI("end");
    return it->second->StopWatchDataChange(observer, pipeInfo);
}

std::pair<Status, int32_t> CommunicationProviderImpl::SendData(const PipeInfo &pipeInfo, const DeviceId &deviceId,
    const DataInfo &dataInfo, uint32_t totalLength, const MessageInfo &info)
{
    if (dataInfo.length > DataBuffer::MAX_TRANSFER_SIZE || dataInfo.length == 0 || dataInfo.data == nullptr ||
        pipeInfo.pipeId.empty() || deviceId.deviceId.empty()) {
        ZLOGW("Input is invalid, maxSize:%u, current size:%u", DataBuffer::MAX_TRANSFER_SIZE, dataInfo.length);
        return std::make_pair(Status::ERROR, 0);
    }
    ZLOGD("pipeInfo:%s ,size:%u, total length:%u", pipeInfo.pipeId.c_str(), dataInfo.length, totalLength);
    std::shared_ptr<AppPipeHandler> appPipeHandler;
    {
        std::lock_guard<std::mutex> lock(dataBusMapMutex_);
        auto it = dataBusMap_.find(pipeInfo.pipeId);
        if (it == dataBusMap_.end()) {
            ZLOGW("pipeInfo:%s not found", pipeInfo.pipeId.c_str());
            return std::make_pair(Status::KEY_NOT_FOUND, 0);
        }
        appPipeHandler = it->second;
    }
    return appPipeHandler->SendData(pipeInfo, deviceId, dataInfo, totalLength, info);
}

Status CommunicationProviderImpl::Start(const PipeInfo &pipeInfo)
{
    if (pipeInfo.pipeId.empty()) {
        ZLOGW("Start Failed, pipeInfo is empty.");
        return Status::INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(dataBusMapMutex_);
    auto it = dataBusMap_.find(pipeInfo.pipeId);
    if (it != dataBusMap_.end()) {
        ZLOGW("repeated start, pipeInfo:%{public}s.", pipeInfo.pipeId.c_str());
        return Status::SUCCESS;
    }
    ZLOGD("Start pipeInfo:%{public}s ", pipeInfo.pipeId.c_str());
    auto handler = std::make_shared<AppPipeHandler>(pipeInfo);
    int ret = handler->CreateSessionServer(pipeInfo.pipeId);
    if (ret != 0) {
        ZLOGW("Start pipeInfo:%{public}s, failed ret:%{public}d.", pipeInfo.pipeId.c_str(), ret);
        return Status::ILLEGAL_STATE;
    }

    dataBusMap_.insert(std::pair<std::string, std::shared_ptr<AppPipeHandler>>(pipeInfo.pipeId, handler));
    return Status::SUCCESS;
}

Status CommunicationProviderImpl::Stop(const PipeInfo &pipeInfo)
{
    std::lock_guard<std::mutex> lock(dataBusMapMutex_);
    auto it = dataBusMap_.find(pipeInfo.pipeId);
    if (it == dataBusMap_.end()) {
        ZLOGW("pipeInfo:%s not found", pipeInfo.pipeId.c_str());
        return Status::KEY_NOT_FOUND;
    }
    std::shared_ptr<AppPipeHandler> appPipeHandler = it->second;
    int ret = appPipeHandler->RemoveSessionServer(pipeInfo.pipeId);
    if (ret != 0) {
        ZLOGW("Stop pipeInfo:%s ret:%d.", pipeInfo.pipeId.c_str(), ret);
        return Status::ERROR;
    }
    dataBusMap_.erase(pipeInfo.pipeId);
    return Status::SUCCESS;
}

bool CommunicationProviderImpl::IsSameStartedOnPeer(const PipeInfo &pipeInfo, const DeviceId &peer)
{
    ZLOGI("start");
    if (pipeInfo.pipeId.empty() || peer.deviceId.empty()) {
        ZLOGE("pipeId or deviceId is empty. Return false.");
        return false;
    }
    ZLOGI("pipeInfo == [%s]", pipeInfo.pipeId.c_str());
    std::shared_ptr<AppPipeHandler> appPipeHandler;
    {
        std::lock_guard<std::mutex> lock(dataBusMapMutex_);
        auto it = dataBusMap_.find(pipeInfo.pipeId);
        if (it == dataBusMap_.end()) {
            ZLOGE("pipeInfo:%s not found. Return false.", pipeInfo.pipeId.c_str());
            return false;
        }
        appPipeHandler = it->second;
    }
    return appPipeHandler->IsSameStartedOnPeer(pipeInfo, peer);
}

void CommunicationProviderImpl::SetMessageTransFlag(const PipeInfo &pipeInfo, bool flag)
{
    if (pipeInfo.pipeId.empty()) {
        return;
    }
    std::shared_ptr<AppPipeHandler> appPipeHandler;
    {
        std::lock_guard<std::mutex> lock(dataBusMapMutex_);
        auto it = dataBusMap_.find(pipeInfo.pipeId);
        if (it == dataBusMap_.end()) {
            ZLOGW("pipeInfo:%s not found", pipeInfo.pipeId.c_str());
            return;
        }
        appPipeHandler = it->second;
    }
    appPipeHandler->SetMessageTransFlag(pipeInfo, flag);
}

Status CommunicationProviderImpl::Broadcast(const PipeInfo &pipeInfo, const LevelInfo &levelInfo)
{
    return SoftBusAdapter::GetInstance()->Broadcast(pipeInfo, levelInfo);
}

int32_t CommunicationProviderImpl::ListenBroadcastMsg(const PipeInfo &pipeInfo,
    std::function<void(const std::string &, const LevelInfo &)> listener)
{
    return SoftBusAdapter::GetInstance()->ListenBroadcastMsg(pipeInfo, std::move(listener));
}

Status CommunicationProviderImpl::ReuseConnect(const PipeInfo &pipeInfo, const DeviceId &deviceId)
{
    if (pipeInfo.pipeId.empty() || deviceId.deviceId.empty()) {
        ZLOGW("Input is invalid, pipeId:%{public}s, deviceId:%{public}s", pipeInfo.pipeId.c_str(),
            KvStoreUtils::ToBeAnonymous(deviceId.deviceId).c_str());
        return Status::INVALID_ARGUMENT;
    }
    ZLOGD("pipeInfo:%s", pipeInfo.pipeId.c_str());
    std::shared_ptr<AppPipeHandler> appPipeHandler;
    {
        std::lock_guard<std::mutex> lock(dataBusMapMutex_);
        auto it = dataBusMap_.find(pipeInfo.pipeId);
        if (it == dataBusMap_.end()) {
            ZLOGW("pipeInfo:%s not found", pipeInfo.pipeId.c_str());
            return Status::INVALID_ARGUMENT;
        }
        appPipeHandler = it->second;
    }
    return appPipeHandler->ReuseConnect(pipeInfo, deviceId);
}
} // namespace AppDistributedKv
} // namespace OHOS
