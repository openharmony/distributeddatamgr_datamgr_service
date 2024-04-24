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
#define LOG_TAG "LifeCycleManager"

#include "lifecycle_manager.h"

#include <algorithm>
#include <cinttypes>

#include "log_print.h"

namespace OHOS {
namespace UDMF {
using CleanAfterGet = LifeCyclePolicy;
std::unordered_map<std::string, std::shared_ptr<LifeCyclePolicy>> LifeCycleManager::intentionPolicy_ = {
    { UD_INTENTION_MAP.at(UD_INTENTION_DRAG), std::make_shared<CleanAfterGet>() },
};

LifeCycleManager &LifeCycleManager::GetInstance()
{
    static LifeCycleManager instance;
    return instance;
}

Status LifeCycleManager::OnGot(const UnifiedKey &key)
{
    auto findPolicy = intentionPolicy_.find(key.intention);
    if (findPolicy == intentionPolicy_.end()) {
        ZLOGE("Invalid intention, intention: %{public}s.", key.intention.c_str());
        return E_INVALID_PARAMETERS;
    }
    auto policy = findPolicy->second;
    ExecutorPool::TaskId taskId = executors_->Execute([=] {
        policy->OnGot(key);
    });
    if (taskId == ExecutorPool::INVALID_TASK_ID) {
        ZLOGE("OnGot task execute failed.");
        return E_ERROR;
    }
    return E_OK;
}

Status LifeCycleManager::OnStart()
{
    Status status = E_OK;
    std::string errorInfo;
    for (auto &[intention, lifeCyclePolicy] : intentionPolicy_) {
        if (lifeCyclePolicy == nullptr) {
            continue;
        }
        Status delStatus = lifeCyclePolicy->OnStart(intention);
        if (delStatus != E_OK) {
            status = delStatus;
            errorInfo += intention + " ";
        }
    }
    if (status != E_OK) {
        ZLOGW("fail, status = %{public}d, intention = [%{public}s].", status, errorInfo.c_str());
    }
    return status;
}

Status LifeCycleManager::StartLifeCycleTimer()
{
    if (executors_ == nullptr) {
        ZLOGE("Executors_ is nullptr.");
        return E_ERROR;
    }
    ExecutorPool::TaskId taskId = executors_->Schedule(GetTask(), LifeCyclePolicy::INTERVAL);
    if (taskId == ExecutorPool::INVALID_TASK_ID) {
        ZLOGE("ExecutorPool Schedule failed.");
        return E_ERROR;
    }
    ZLOGI("ScheduleTask start, TaskId: %{public}" PRIu64 ".", taskId);
    return E_OK;
}

ExecutorPool::Task LifeCycleManager::GetTask()
{
    return [this] {
        Status status = E_OK;
        std::string errorInfo;
        for (auto &[intention, lifeCyclePolicy] : intentionPolicy_) {
            if (lifeCyclePolicy == nullptr) {
                continue;
            }
            Status delStatus = lifeCyclePolicy->OnTimeout(intention);
            if (delStatus != E_OK) {
                status = delStatus;
                errorInfo += intention + " ";
            }
        }
        if (status != E_OK) {
            ZLOGW("fail, status = %{public}d, intention = [%{public}s].", status, errorInfo.c_str());
        }
        return status;
    };
}

void LifeCycleManager::SetThreadPool(std::shared_ptr<ExecutorPool> executors)
{
    executors_ = executors;
}
} // namespace UDMF
} // namespace OHOS
