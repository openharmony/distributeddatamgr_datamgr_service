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

#include "flow_control_manager/flow_control_manager.h"

#include <list>

namespace OHOS::DistributedData {
FlowControlManager::FlowControlManager(std::shared_ptr<ExecutorPool> pool, std::shared_ptr<Strategy> strategy)
    : pool_(std::move(pool)), strategy_(std::move(strategy))
{
}

FlowControlManager::~FlowControlManager()
{
    isDestroyed_ = true;
    ExecutorPool::TaskId taskId = ExecutorPool::INVALID_TASK_ID;
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        taskId = taskId_;
        taskId_ = ExecutorPool::INVALID_TASK_ID;
        auto tasks = std::move(tasks_);
    }
    pool_->Remove(taskId, true);
}

void FlowControlManager::Execute(Task task, uint32_t type)
{
    TaskInfo info;
    info.type = type;
    Execute(std::move(task), std::move(info));
}

void FlowControlManager::Execute(Task task, TaskInfo info)
{
    Tp executeTime = std::chrono::steady_clock::now();
    if (strategy_ != nullptr) {
        executeTime = strategy_->GetExecuteTime(task, info);
    }
    auto id = GenTaskId();
    InnerTask innerTask{ std::move(task), std::move(info), executeTime, id };
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
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
    if (isDestroyed_) {
        return;
    }
    std::list<Task> tasks;
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    Tp now = std::chrono::steady_clock::now();
    while (!tasks_.empty()) {
        const InnerTask &task = tasks_.top();
        if (task.task == nullptr) {
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
    if (!tasks.empty()) {
        pool_->Execute([executeTasks = std::move(tasks)]() {
            for (auto &task : executeTasks) {
                task();
            }
        });
    }
    taskId_ = ExecutorPool::INVALID_TASK_ID;
}

void FlowControlManager::Schedule()
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (tasks_.empty()) {
        pool_->Remove(taskId_);
        taskId_ = ExecutorPool::INVALID_TASK_ID;
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
    taskId_ = pool_->Schedule(duration, [this]() {
        ExecuteTask();
        Schedule();
    });
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
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto tasks = std::move(tasks_);
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