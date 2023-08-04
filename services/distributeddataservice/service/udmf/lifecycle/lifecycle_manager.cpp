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
std::shared_ptr<ExecutorPool> LifeCycleManager::executorPool_ = std::make_shared<ExecutorPool>(2, 1);

std::unordered_map<std::string, std::shared_ptr<LifeCyclePolicy>> LifeCycleManager::intentionPolicyMap_ = {
    { UD_INTENTION_MAP.at(UD_INTENTION_DRAG), std::make_shared<CleanAfterGet>() },
};

LifeCycleManager &LifeCycleManager::GetInstance()
{
    static LifeCycleManager instance;
    return instance;
}

Status LifeCycleManager::DeleteOnGet(const UnifiedKey &key)
{
    auto findPolicy = intentionPolicyMap_.find(key.intention);
    if (findPolicy == intentionPolicyMap_.end()) {
        ZLOGE("Invalid intention, intention: %{public}s.", key.intention.c_str());
        return E_INVALID_PARAMETERS;
    }
    auto policy = findPolicy->second;
    return policy->DeleteOnGet(key);
}

Status LifeCycleManager::DeleteOnStart()
{
    Status status = E_OK;
    std::shared_ptr<LifeCyclePolicy> LifeCyclePolicy;
    for (const auto &intentionPolicyPair : intentionPolicyMap_) {
        LifeCyclePolicy = GetPolicy(intentionPolicyPair.first);
        Status delStatus = LifeCyclePolicy->DeleteOnStart(intentionPolicyPair.first);
        if (delStatus != E_OK) {
            status = delStatus;
            ZLOGW("DeleteOnStart fail, intentionType = %{public}s, status = %{public}d.",
                  intentionPolicyPair.first.c_str(), delStatus);
        }
    }
    return status;
}

Status LifeCycleManager::DeleteOnSchedule()
{
    ExecutorPool::TaskId taskId =
        executorPool_->Schedule(&LifeCycleManager::DeleteOnTimeout, LifeCyclePolicy::INTERVAL);
    if (taskId == ExecutorPool::INVALID_TASK_ID) {
        ZLOGE("ExecutorPool Schedule failed.");
        return E_ERROR;
    }
    ZLOGI("ScheduleTask start, TaskId: %{public}" PRIu64 ".", taskId);
    return E_OK;
}

std::shared_ptr<LifeCyclePolicy> LifeCycleManager::GetPolicy(const std::string &intention)
{
    auto findPolicy = intentionPolicyMap_.find(intention);
    if (findPolicy == intentionPolicyMap_.end()) {
        return nullptr;
    }
    return findPolicy->second;
}

Status LifeCycleManager::DeleteOnTimeout()
{
    Status status = E_OK;
    std::shared_ptr<LifeCyclePolicy> LifeCyclePolicy;
    for (const auto &intentionPolicyPair : intentionPolicyMap_) {
        LifeCyclePolicy = LifeCycleManager::GetInstance().GetPolicy(intentionPolicyPair.first);
        Status delStatus = LifeCyclePolicy->DeleteOnTimeout(intentionPolicyPair.first);
        if (delStatus != E_OK) {
            status = delStatus;
            ZLOGW("DeleteOnTimeout fail, intentionType = %{public}s, status = %{public}d.",
                  intentionPolicyPair.first.c_str(), delStatus);
        }
    }
    return status;
}
} // namespace UDMF
} // namespace OHOS
