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
#define LOG_TAG "FlowControlManager"
#include "flow_control_manager/flow_control_manager.h"

#include <list>

#include "log_print.h"

namespace OHOS::DistributedData {
std::shared_ptr<FlowControlManager> FlowControlManager::Create(std::shared_ptr<ExecutorPool> pool,
    std::shared_ptr<Strategy> strategy)
{
    return std::shared_ptr<FlowControlManager>(new FlowControlManager(std::move(pool), std::move(strategy)));
}

FlowControlManager::FlowControlManager(std::shared_ptr<ExecutorPool> pool, std::shared_ptr<Strategy> strategy)
    : pool_(std::move(pool)), strategy_(std::move(strategy))
{
}

FlowControlManager::~FlowControlManager()
{
    ExecutorPool::TaskId taskId = ExecutorPool::INVALID_TASK_ID;
    {
        decltype(tasks_) tasks;
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        isRunning_ = false;
        taskId = taskId_;
        taskId_ = ExecutorPool::INVALID_TASK_ID;
        tasks = std::move(tasks_);
    }
    if (pool_ != nullptr && taskId != ExecutorPool::INVALID_TASK_ID) {
        pool_->Remove(taskId, false);
    }
}

void FlowControlManager::Execute(Task task, uint32_t type)
{
    TaskInfo info;
    info.type = type;
    Execute(std::move(task), std::move(info));
}

void FlowControlManager::Execute(Task task, TaskInfo info)
{
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        if (!isRunning_ || pool_ == nullptr) {
            return;
        }
    }
    Tp executeTime = std::chrono::steady_clock::now();
    if (strategy_ != nullptr) {
        executeTime = strategy_->GetExecuteTime(task, info);
    }
    auto id = GenTaskId();
    InnerTask innerTask{ std::move(task), std::move(info), executeTime, id };
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        if (!isRunning_ || pool_ == nullptr) {
            return;
        }
        tasks_.push(std::move(innerTask));
        // If the added task is not the first task or not the earliest task, return directly
        if (tasks_.top().id != id) {
            return;
        }
    }
    Schedule();
}

void FlowControlManager::ExecuteTask()
{
    std::list<Task> tasks;
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        if (!isRunning_ || pool_ == nullptr) {
            return;
        }
        Tp now = std::chrono::steady_clock::now();
        while (!tasks_.empty()) {
            const InnerTask &task = tasks_.top();
            if (task.task == nullptr || !isRunning_) {
                tasks_.pop();
                continue;
            }
            if (task.time <= now) {
                tasks.push_back(task.task);
                tasks_.pop();
                continue;
            }
            break;
        }
        taskId_ = ExecutorPool::INVALID_TASK_ID;
    }
    if (!tasks.empty()) {
        pool_->Execute([executeTasks = std::move(tasks)]() {
            for (auto &task : executeTasks) {
                task();
            }
        });
    }
}

void FlowControlManager::Schedule()
{
    auto taskId = ExecutorPool::INVALID_TASK_ID;
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        if (!isRunning_ || pool_ == nullptr) {
            return;
        }
        if (tasks_.empty() && taskId_ != ExecutorPool::INVALID_TASK_ID) {
            taskId = taskId_;
            taskId_ = ExecutorPool::INVALID_TASK_ID;
        }
    }
    if (taskId != ExecutorPool::INVALID_TASK_ID && pool_ != nullptr) {
        pool_->Remove(taskId, true);
        return;
    }
    auto weakThis = weak_from_this();
    if (weakThis.expired()) {
        ZLOGE("FlowControlManager must be managed by std::shared_ptr");
        return;
    }
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        if (tasks_.empty() || !isRunning_) {
            return;
        }
        const InnerTask &task = tasks_.top();
        Tp now = std::chrono::steady_clock::now();
        auto duration = task.time < now ? std::chrono::steady_clock::duration(0) : task.time - now;
        // If there is a task running, execute according to the earliest time
        if (taskId_ != ExecutorPool::INVALID_TASK_ID) {
            pool_->Reset(taskId_, duration);
            return;
        }
        taskId_ = pool_->Schedule(duration, [weakThis]() {
            auto self = weakThis.lock();
            if (self == nullptr) {
                return;
            }
            self->ExecuteTask();
            self->Schedule();
        });
    }
}

void FlowControlManager::Remove(uint32_t type)
{
    Remove([type](const TaskInfo &info) {
        return info.type == type;
    });
}

void FlowControlManager::Remove(Filter filter)
{
    {
        decltype(tasks_) tasks;
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        tasks = std::move(tasks_);
        while (!tasks.empty()) {
            if (!filter || filter(tasks.top().info)) {
                tasks.pop();
                continue;
            }
            tasks_.push(std::move(tasks.top()));
            tasks.pop();
        }
    }
    Schedule();
}

} // namespace OHOS::DistributedData
