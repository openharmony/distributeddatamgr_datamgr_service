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

#include "task_manager.h"
namespace OHOS::DistributedData {
TaskManager::TaskManager(std::shared_ptr<ExecutorPool> executors) : executors_(executors)
{
}

TaskManager::~TaskManager()
{
    executors_ = nullptr;
}
TaskManager::TaskId TaskManager::Execute(const Task &task)
{
    return executors_->Execute(task);
}
TaskManager::TaskId TaskManager::Execute(const Task &task, Duration delay)
{
    return executors_->Schedule(delay, task);
}
TaskManager::TaskId TaskManager::Schedule(const Task &task, Duration interval)
{
    return executors_->Schedule(task, interval);
}
TaskManager::TaskId TaskManager::Schedule(const Task &task, Duration delay, Duration interval)
{
    return executors_->Schedule(task, delay, interval);
}
TaskManager::TaskId TaskManager::Schedule(const Task &task, Duration delay, Duration interval, uint64_t times)
{
    return executors_->Schedule(task, delay, interval, times);
}
bool TaskManager::Remove(const TaskId &taskId, bool wait)
{
    return executors_->Remove(taskId, wait);
}
TaskManager::TaskId TaskManager::Reset(const TaskId &taskId, Duration interval)
{
    return executors_->Reset(taskId, interval);
}
} // namespace OHOS::DistributedData
