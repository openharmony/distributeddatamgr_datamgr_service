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
#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_RDB_FLOW_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_RDB_FLOW_MANAGER_H

#include <functional>
#include <string>

#include "executor_pool.h"
#include "flow_control_manager/flow_control_manager.h"
namespace OHOS {
namespace DistributedRdb {
class RdbFlowControlManager {
public:
    enum TaskType {
        TASK_TYPE_SYNC = 0,
    };
    using Task = DistributedData::FlowControlManager::Task;
    using TaskInfo = DistributedData::FlowControlManager::TaskInfo;
    using Strategy = DistributedData::FlowControlManager::Strategy;
    using Tp = DistributedData::FlowControlManager::Tp;

    RdbFlowControlManager() = default;
    // execute once
    void Init(std::shared_ptr<ExecutorPool> pool, std::shared_ptr<Strategy> strategy);
    int32_t Execute(Task task, TaskInfo info);
    int32_t Remove(const std::string &label);

private:
    std::shared_ptr<DistributedData::FlowControlManager> manager_;
};

class RdbFlowControlStrategy : public RdbFlowControlManager::Strategy {
public:
    RdbFlowControlStrategy();
    RdbFlowControlStrategy(uint32_t appLimit, uint32_t deviceLimit, uint32_t duration);
    RdbFlowControlManager::Tp GetExecuteTime(
        RdbFlowControlManager::Task task, const RdbFlowControlManager::TaskInfo &info) override;
private:
    static constexpr uint32_t APP_LIMIT = 5;
    static constexpr uint32_t DEVICE_LIMIT = 20;
    static constexpr uint32_t DURATION = 60 * 1000; // 1min
    uint32_t appLimit_ = APP_LIMIT;
    uint32_t deviceLimit_ = DEVICE_LIMIT;
    uint32_t duration_ = DURATION;
    std::chrono::steady_clock::time_point GetDeviceSyncExecuteTime(const std::string &label);
    std::mutex mutex_;
    std::queue<std::chrono::steady_clock::time_point> requestDeviceQueue_;
    std::map<std::string, std::queue<std::chrono::steady_clock::time_point>> requestAppQueue_;
};
} // namespace DistributedRdb
} // namespace OHOS
#endif //OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_RDB_FLOW_MANAGER_H
