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
#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_FLOW_CONTROL_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_FLOW_CONTROL_MANAGER_H

#include <chrono>
#include <functional>
#include <priority_queue.h>
#include <string>

#include "executor_pool.h"
#include "visibility.h"
namespace OHOS {
namespace DistributedData {
class API_EXPORT FlowControlManager {
public:
    using Task = std::function<void()>;
    using Tp = std::chrono::steady_clock::time_point;
    class Strategy {
    public:
        virtual ~Strategy() = default;
        virtual Tp GetExecuteTime(Task task, uint32_t type) = 0;
    };

    FlowControlManager(std::shared_ptr<ExecutorPool> pool, std::shared_ptr<Strategy> strategy);
    ~FlowControlManager();
    void Execute(Task task, uint32_t type = 0);
    void Remove(uint32_t type = 0);

private:
    static constexpr uint32_t INVALID_INNER_TASK_ID = 0;
    struct InnerTask {
        InnerTask(Task task, uint32_t type, Tp time, uint64_t taskId) : task(task), type(type), time(time), id(taskId)
        {
        }
        Task task;
        uint32_t type;
        Tp time;
        uint64_t id;
        bool operator<(const InnerTask &other) const
        {
            return time > other.time;
        }
    };

    void Schedule();

    void ExecuteTask();

    uint64_t GenTaskId()
    {
        auto taskId = ++innerTaskId_;
        if (taskId == INVALID_INNER_TASK_ID) {
            taskId = ++innerTaskId_;
        }
        return taskId;
    }

    const std::shared_ptr<ExecutorPool> pool_;
    const std::shared_ptr<Strategy> strategy_;
    bool isDestroyed_ = false;
    std::mutex mutex_;
    std::priority_queue<InnerTask> tasks_;
    ExecutorPool::TaskId taskId_ = ExecutorPool::INVALID_TASK_ID;
    std::atomic_uint64_t innerTaskId_ = INVALID_INNER_TASK_ID;
};

} // namespace DistributedData
} // namespace OHOS
#endif //OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_FLOW_CONTROL_MANAGER_H
