/*
* Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef DISTRIBUTED_DATA_TASK_MANAGER_H
#define DISTRIBUTED_DATA_TASK_MANAGER_H
#include "ithread_pool.h"
#include "executor_pool.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class TaskManager : public DistributedDB::IThreadPool {
public:
    using TaskId = uint64_t;
    using Task = std::function<void()>;
    using Duration = std::chrono::steady_clock::duration;
    TaskManager() = default;
    explicit TaskManager(std::shared_ptr<ExecutorPool> executors);
    ~TaskManager() override;
    TaskId Execute(const Task &task) override;
    TaskId Execute(const Task &task, Duration delay) override;
    TaskId Schedule(const Task &task, Duration interval) override;
    TaskId Schedule(const Task &task, Duration delay, Duration interval) override;
    TaskId Schedule(const Task &task, Duration delay, Duration interval, uint64_t times) override;
    bool Remove(const TaskId &taskId, bool wait) override;
    TaskId Reset(const TaskId &taskId, Duration interval) override;

private:
    std::shared_ptr<ExecutorPool> executors_ = nullptr;
};
} // namespace OHOS::DistributedData
#endif // DISTRIBUTED_DATA_TASK_MANAGER_H
