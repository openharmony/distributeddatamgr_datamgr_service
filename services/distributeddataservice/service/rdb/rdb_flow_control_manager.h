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

#include "concurrent_map.h"
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

    RdbFlowControlManager(uint32_t appLimit, uint32_t globalLimit, uint32_t duration);
    // execute once
    void Init(std::shared_ptr<ExecutorPool> pool);
    int32_t Execute(Task task, TaskInfo taskInfo);
    int32_t Remove(const std::string &label);

private:
    std::shared_ptr<ExecutorPool> pool_;
    const uint32_t appLimit_ = 0;
    const uint32_t globalLimit_ = 0;
    const uint32_t duration_ = 0;
    std::shared_ptr<DistributedData::FlowControlManager> globalManager_;
    ConcurrentMap<std::string, std::shared_ptr<DistributedData::FlowControlManager>> managers_;
};

class RdbFlowControlStrategy : public RdbFlowControlManager::Strategy {
public:
    RdbFlowControlStrategy(uint32_t limit, uint32_t duration);
    RdbFlowControlManager::Tp GetExecuteTime(RdbFlowControlManager::Task task,
        const RdbFlowControlManager::TaskInfo &info) override;

private:
    using Tp = RdbFlowControlManager::Tp;
    std::mutex mutex_;
    std::deque<Tp> queue_;
    const uint32_t limit_ = 0;
    const uint32_t duration_;
};
} // namespace DistributedRdb
} // namespace OHOS
#endif //OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_RDB_FLOW_MANAGER_H
