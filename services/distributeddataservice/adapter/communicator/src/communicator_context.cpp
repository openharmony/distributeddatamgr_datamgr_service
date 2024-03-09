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
using KvStoreUtils = OHOS::DistributedKv::KvStoreUtils;

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

void CommunicatorContext::SetSessionListener(const OnSendAble &sendAbleCallback)
{
    std::lock_guard<std::mutex> sessionLockGard(sessionMutex_);
    sessionListener_ = sendAbleCallback;
}

void CommunicatorContext::NotifySessionChanged(const std::string &deviceId)
{
    ZLOGI("Notify session begin, deviceId:%{public}s", KvStoreUtils::ToBeAnonymous(deviceId).c_str());
    std::lock_guard<std::mutex> sessionLockGard(sessionMutex_);
    if (sessionListener_ == nullptr) {
        return;
    }
    DeviceInfos devInfo;
    devInfo.identifier = deviceId;
    sessionListener_(devInfo);
}
} // namespace OHOS::DistributedData