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

#define LOG_TAG "rdb_flow_control_manager"
#include "rdb_flow_control_manager.h"

#include "rdb_hiview_adapter.h"
#include "log_print.h"

namespace OHOS::DistributedRdb {

void RdbFlowControlManager::Init(std::shared_ptr<ExecutorPool> pool)
{
    if (pool == nullptr || pool_ != nullptr) {
        return;
    }
    pool_ = pool;
    auto strategy = std::make_shared<RdbFlowControlStrategy>(globalLimit_, duration_);
    globalManager_ = std::make_shared<DistributedData::FlowControlManager>(std::move(pool), std::move(strategy));
}

int32_t RdbFlowControlManager::Execute(Task task, TaskInfo taskInfo)
{
    if (globalManager_ == nullptr || pool_ == nullptr) {
        return -1;
    }
    std::shared_ptr<DistributedData::FlowControlManager> manager;
    managers_.Compute(taskInfo.label,
        [this, &manager](const auto &key, std::shared_ptr<DistributedData::FlowControlManager> &value) {
            if (value == nullptr) {
                auto strategy = std::make_shared<RdbFlowControlStrategy>(appLimit_, duration_);
                value = std::make_shared<DistributedData::FlowControlManager>(pool_, std::move(strategy));
            }
            manager = value;
            return true;
        });
    if (manager == nullptr) {
        return -1;
    }
    manager->Execute(
        [this, task, taskInfo]() mutable { globalManager_->Execute(task, taskInfo); }, std::move(taskInfo));
    return 0;
}

int32_t RdbFlowControlManager::Remove(const std::string &label)
{
    if (globalManager_ == nullptr) {
        return 0;
    }
    globalManager_->Remove([&label](const TaskInfo &info) {
        return info.label == label;
    });
    managers_.ComputeIfPresent(label, [](const auto &key, auto &value) {
        if (value != nullptr) {
            value->Remove([&key](const TaskInfo &info) {
                return info.label == key;
            });
        }
        return false;
    });
    return 0;
}
RdbFlowControlManager::RdbFlowControlManager(uint32_t appLimit, uint32_t globalLimit, uint32_t duration)
    : appLimit_(appLimit), globalLimit_(globalLimit), duration_(duration)
{
}

RdbFlowControlStrategy::RdbFlowControlStrategy(uint32_t limit, uint32_t duration) : limit_(limit), duration_(duration)
{
}
RdbFlowControlManager::Tp RdbFlowControlStrategy::GetExecuteTime(RdbFlowControlManager::Task task,
    const RdbFlowControlManager::TaskInfo &info)
{
    auto executeTime = std::chrono::steady_clock::now();
    if (limit_ == 0) {
        return executeTime;
    }
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    while (!queue_.empty()) {
        if (queue_.size() > limit_ || executeTime > queue_.front() + std::chrono::milliseconds(duration_)) {
            queue_.pop_front();
        } else {
            break;
        }
    }
    if (queue_.size() >= limit_) {
        executeTime = std::max(executeTime, queue_.front() + std::chrono::milliseconds(duration_));
    }
    if (executeTime > std::chrono::steady_clock::now()) {
        int64_t delaytime =
            std::chrono::duration_cast<std::chrono::milliseconds>(executeTime - std::chrono::steady_clock::now())
                .count();
        ZLOGW("Sync delay %{public}" PRIu64 "ms! bundleName:%{public}s.", delaytime, info.label.c_str());
        RdbHiViewAdapter::GetInstance().ReportRdbFault({DEVICE_SYNC,
            DEVICE_SYNC_LIMIT, info.label, "device sync delay " + std::to_string(delaytime) + " ms"});
    }
    queue_.push_back(executeTime);
    return executeTime;
}
} // namespace OHOS::DistributedRdb