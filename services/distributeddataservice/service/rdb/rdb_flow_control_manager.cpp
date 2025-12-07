/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#define LOG_TAG "rdb_flow_manager"
#include "rdb_flow_control_manager.h"

namespace OHOS::DistributedRdb {

void RdbFlowControlManager::Init(std::shared_ptr<ExecutorPool> pool, std::shared_ptr<Strategy> strategy)
{
    manager_ = std::make_shared<DistributedData::FlowControlManager>(std::move(pool), std::move(strategy));
}

int32_t RdbFlowControlManager::Execute(Task task, TaskInfo info)
{
    if (manager_ == nullptr) {
        return -1;
    }
    manager_->Execute(std::move(task), std::move(info));
    return 0;
}

int32_t RdbFlowControlManager::Remove(const std::string &label)
{
    if (manager_ == nullptr) {
        return 0;
    }
    manager_->Remove([&label](const TaskInfo &info) {
        return info.label == label;
    });
    return 0;
}

RdbFlowControlStrategy::RdbFlowControlStrategy() : RdbFlowControlStrategy(APP_LIMIT, DEVICE_LIMIT, DURATION) {}
RdbFlowControlStrategy::RdbFlowControlStrategy(uint32_t appLimit, uint32_t deviceLimit, uint32_t duration)
    : appLimit_(appLimit), deviceLimit_(deviceLimit), duration_(duration)
{
}

RdbFlowControlManager::Tp RdbFlowControlStrategy::GetExecuteTime(RdbFlowControlManager::Task task,
    const RdbFlowControlManager::TaskInfo &info)
{
    switch (info.type) {
        case RdbFlowControlManager::TASK_TYPE_SYNC:
            return GetDeviceSyncExecuteTime(info.label);
    }
    return std::chrono::steady_clock::now();
}

std::chrono::steady_clock::time_point RdbFlowControlStrategy::GetDeviceSyncExecuteTime(const std::string &label)
{
    auto executeTime = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);
    auto &queue = requestAppQueue_[label];
    if (queue.size() >= appLimit_) {
        executeTime = std::max(executeTime, queue.front() + std::chrono::milliseconds(duration_));
    }
    if (requestDeviceQueue_.size() >= deviceLimit_) {
        executeTime = std::max(executeTime, requestDeviceQueue_.front() + std::chrono::milliseconds(duration_));
    }
    if (executeTime > std::chrono::steady_clock::now()) {
        uint64_t delaytime =
            std::chrono::duration_cast<std::chrono::milliseconds>(executeTime - std::chrono::steady_clock::now())
                .count();
        ZLOGE("Sync delay %{public}lu ms! Bundelname:%{public}s.", delaytime, label.c_str());
        RdbHiViewAdapter::GetInstance().ReportRdbFault({DECIVE_SYNC,
            DEVICE_SYNC_LIMIT, label, "device sync delay " + std::to_string(delaytime) + " ms"});
    }
    queue.push(executeTime);
    requestDeviceQueue_.push(executeTime);
    while (requestDeviceQueue_.size() > deviceLimit_) {
        requestDeviceQueue_.pop();
    }
    while (queue.size() > appLimit_) {
        queue.pop();
    }
    return executeTime;
}
} // namespace OHOS::DistributedRdb