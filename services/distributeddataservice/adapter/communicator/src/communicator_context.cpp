/*
* Copyright (c) 2023 Huawei Device Co., Ltd.
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

#define LOG_TAG "CommunicatorContext"
#include "communicator_context.h"
#include "log_print.h"
#include "kvstore_utils.h"

namespace OHOS::DistributedData {
using KvUtils = OHOS::DistributedKv::KvStoreUtils;
using Status = OHOS::DistributedKv::Status;

CommunicatorContext &CommunicatorContext::GetInstance()
{
    static CommunicatorContext context;
    return context;
}

void CommunicatorContext::SetThreadPool(std::shared_ptr<ExecutorPool> executors)
{
    executors_ = executors;
}

std::shared_ptr<ExecutorPool> CommunicatorContext::GetThreadPool()
{
    return executors_;
}

Status CommunicatorContext::RegSessionListener(const DevChangeListener *observer)
{
    if (observer == nullptr) {
        ZLOGE("observer is nullptr");
        return Status::INVALID_ARGUMENT;
    }
    if (!observers_.Insert(observer, observer)) {
        ZLOGE("insert observer fail");
        return Status::ERROR;
    }
    return Status::SUCCESS;
}

void CommunicatorContext::SetSessionListener(const OnCloseAble &closeAbleCallback)
{
    std::lock_guard<decltype(sessionMutex_)> sessionLockGard(sessionMutex_);
    closeListener_ = closeAbleCallback;
}

Status CommunicatorContext::UnRegSessionListener(const DevChangeListener *observer)
{
    if (observer == nullptr) {
        ZLOGE("observer is nullptr");
        return Status::INVALID_ARGUMENT;
    }
    if (!observers_.Erase(observer)) {
        ZLOGE("erase observer fail");
        return Status::ERROR;
    }
    return Status::SUCCESS;
}

void CommunicatorContext::NotifySessionReady(const std::string &deviceId)
{
    if (deviceId.empty()) {
        ZLOGE("deviceId empty");
        return;
    }
    devices_.Insert(deviceId, deviceId);
    std::vector<const DevChangeListener *> observers;
    observers_.ForEach([&observers](const auto &key, auto &value) {
        observers.emplace_back(value);
        return false;
    });
    ZLOGI("Notify session begin, deviceId:%{public}s, observer count:%{public}zu",
        KvUtils::ToBeAnonymous(deviceId).c_str(), observers.size());
    DeviceInfo devInfo;
    devInfo.uuid = deviceId;
    for (const auto &observer : observers) {
        if (observer != nullptr) {
            observer->OnSessionReady(devInfo);
        }
    }
    std::lock_guard<decltype(sessionMutex_)> sessionLockGard(sessionMutex_);
    if (closeListener_) {
        closeListener_(deviceId);
    }
}

void CommunicatorContext::NotifySessionClose(const std::string &deviceId)
{
    if (deviceId.empty()) {
        ZLOGE("deviceId empty");
        return;
    }
    devices_.Erase(deviceId);
}

bool CommunicatorContext::IsSessionReady(const std::string &deviceId)
{
    if (deviceId.empty()) {
        ZLOGE("deviceId empty");
        return false;
    }
    return devices_.Contains(deviceId);
}
} // namespace OHOS::DistributedData