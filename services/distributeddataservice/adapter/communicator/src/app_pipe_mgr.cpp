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

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "AppPipeMgr"

#include "app_pipe_mgr.h"

#include "utils/anonymous.h"

namespace OHOS {
namespace AppDistributedKv {
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedData;
Status AppPipeMgr::StartWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo)
{
    ZLOGI("begin");
    if (observer == nullptr || pipeInfo.pipeId.empty()) {
        ZLOGE("argument invalid");
        return Status::INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(dataBusMapMutex_);
    auto it = dataBusMap_.find(pipeInfo.pipeId);
    if (it == dataBusMap_.end() || it->second == nullptr) {
        ZLOGE("pipeid not found");
        return Status::ERROR;
    }
    ZLOGI("end");
    return it->second->StartWatchDataChange(observer, pipeInfo);
}

// stop DataChangeListener to watch data change;
Status AppPipeMgr::StopWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo)
{
    ZLOGI("begin");
    if (observer == nullptr || pipeInfo.pipeId.empty()) {
        ZLOGE("argument invalid");
        return Status::INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(dataBusMapMutex_);
    auto it = dataBusMap_.find(pipeInfo.pipeId);
    if (it == dataBusMap_.end() || it->second == nullptr) {
        ZLOGE("pipeid not found");
        return Status::ERROR;
    }
    ZLOGI("end");
    return it->second->StopWatchDataChange(observer, pipeInfo);
}

// Send data to other device, function will be called back after sent to notify send result.
std::pair<Status, int32_t> AppPipeMgr::SendData(const PipeInfo &pipeInfo, const DeviceId &deviceId,
    const DataInfo &dataInfo,  uint32_t totalLength, const MessageInfo &info)
{
    if (pipeInfo.pipeId.empty() || deviceId.deviceId.empty() || dataInfo.length > DataBuffer::MAX_TRANSFER_SIZE ||
        dataInfo.length == 0 || dataInfo.data == nullptr) {
        ZLOGE("Input is invalid, maxSize:%{public}u, current size:%{public}u", DataBuffer::MAX_TRANSFER_SIZE,
            dataInfo.length);
        return std::make_pair(Status::ERROR, 0);
    }
    if (dataInfo.extraInfo.userId.empty() || dataInfo.extraInfo.appId.empty() || dataInfo.extraInfo.storeId.empty()) {
        ZLOGE("access info is empty, userId:%{public}s, appId:%{public}s, storeId:%{public}s",
            dataInfo.extraInfo.userId.c_str(), dataInfo.extraInfo.appId.c_str(),
            Anonymous::Change(dataInfo.extraInfo.storeId).c_str());
        return std::make_pair(Status::ERROR, 0);
    }
    ZLOGD("pipeInfo:%{public}s ,size:%{public}u, total length:%{public}u", pipeInfo.pipeId.c_str(),
        dataInfo.length, totalLength);
    std::shared_ptr<AppPipeHandler> appPipeHandler;
    {
        std::lock_guard<std::mutex> lock(dataBusMapMutex_);
        auto it = dataBusMap_.find(pipeInfo.pipeId);
        if (it == dataBusMap_.end() || it->second == nullptr) {
            ZLOGE("pipeInfo:%{public}s not found", pipeInfo.pipeId.c_str());
            return std::make_pair(Status::KEY_NOT_FOUND, 0);
        }
        appPipeHandler = it->second;
    }
    return appPipeHandler->SendData(pipeInfo, deviceId, dataInfo, totalLength, info);
}

// start server
Status AppPipeMgr::Start(const PipeInfo &pipeInfo)
{
    if (pipeInfo.pipeId.empty()) {
        ZLOGW("Start Failed, pipeInfo is empty.");
        return Status::INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(dataBusMapMutex_);
    auto it = dataBusMap_.find(pipeInfo.pipeId);
    if (it != dataBusMap_.end() && it->second != nullptr) {
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
    dataBusMap_.insert_or_assign(pipeInfo.pipeId, handler);
    return Status::SUCCESS;
}

// stop server
Status AppPipeMgr::Stop(const PipeInfo &pipeInfo)
{
    std::lock_guard<std::mutex> lock(dataBusMapMutex_);
    auto it = dataBusMap_.find(pipeInfo.pipeId);
    if (it == dataBusMap_.end() || it->second == nullptr) {
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

bool AppPipeMgr::IsSameStartedOnPeer(const struct PipeInfo &pipeInfo, const struct DeviceId &peer)
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
        if (it == dataBusMap_.end() || it->second == nullptr) {
            ZLOGE("pipeInfo:%s not found. Return false.", pipeInfo.pipeId.c_str());
            return false;
        }
        appPipeHandler = it->second;
    }
    return appPipeHandler->IsSameStartedOnPeer(pipeInfo, peer);
}

void AppPipeMgr::SetMessageTransFlag(const PipeInfo &pipeInfo, bool flag)
{
    if (pipeInfo.pipeId.empty()) {
        return;
    }
    std::shared_ptr<AppPipeHandler> appPipeHandler;
    {
        std::lock_guard<std::mutex> lock(dataBusMapMutex_);
        auto it = dataBusMap_.find(pipeInfo.pipeId);
        if (it == dataBusMap_.end() || it->second == nullptr) {
            ZLOGW("pipeInfo:%s not found", pipeInfo.pipeId.c_str());
            return;
        }
        appPipeHandler = it->second;
    }
    appPipeHandler->SetMessageTransFlag(pipeInfo, flag);
}

Status AppPipeMgr::ReuseConnect(const PipeInfo &pipeInfo, const DeviceId &deviceId, const ExtraDataInfo &extraInfo)
{
    if (pipeInfo.pipeId.empty() || deviceId.deviceId.empty()) {
        ZLOGE("Input is invalid, pipeId:%{public}s, deviceId:%{public}s", pipeInfo.pipeId.c_str(),
            Anonymous::Change(deviceId.deviceId).c_str());
        return Status::INVALID_ARGUMENT;
    }
    if (extraInfo.userId.empty() || extraInfo.bundleName.empty() || extraInfo.storeId.empty() ||
        extraInfo.tokenId == 0) {
        ZLOGE("access info is empty, userId:%{public}s, bundleName:%{public}s, storeId:%{public}s, tokenId:%{public}d",
            extraInfo.userId.c_str(), extraInfo.bundleName.c_str(), Anonymous::Change(extraInfo.storeId).c_str(),
            extraInfo.tokenId);
        return Status::ERROR;
    }
    ZLOGD("pipeInfo:%{public}s", pipeInfo.pipeId.c_str());
    std::shared_ptr<AppPipeHandler> appPipeHandler;
    {
        std::lock_guard<std::mutex> lock(dataBusMapMutex_);
        auto it = dataBusMap_.find(pipeInfo.pipeId);
        if (it == dataBusMap_.end() || it->second == nullptr) {
            ZLOGE("pipeInfo:%{public}s not found", pipeInfo.pipeId.c_str());
            return Status::KEY_NOT_FOUND;
        }
        appPipeHandler = it->second;
    }
    return appPipeHandler->ReuseConnect(pipeInfo, deviceId, extraInfo);
}
}  // namespace AppDistributedKv
}  // namespace OHOS