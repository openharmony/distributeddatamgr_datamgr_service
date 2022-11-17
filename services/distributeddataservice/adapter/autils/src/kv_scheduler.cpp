/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "kv_scheduler.h"


namespace OHOS::DistributedKv {
KvScheduler::KvScheduler(size_t capacity)
{
    capacity_ = capacity;
    isRunning_ = true;
    thread_ = std::make_unique<std::thread>([this]() {
        pthread_setname_np(pthread_self(), "KvScheduler");
        this->Loop();
    });
}

KvScheduler::~KvScheduler()
{
    isRunning_ = false;
    At(std::chrono::system_clock::now(), [](){});
    thread_->join();
}

SchedulerTask KvScheduler::At(const std::chrono::system_clock::time_point &time, std::function<void()> task)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (kvTasks_.size() >= capacity_) {
        return kvTasks_.end();
    }

    auto it = kvTasks_.insert({time, task});
    if (it == kvTasks_.begin()) {
        condition_.notify_one();
    }
    return it;
}

SchedulerTask KvScheduler::Reset(SchedulerTask task, const std::chrono::system_clock::time_point &time,
    const std::chrono::system_clock::duration &interval)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (kvTasks_.begin()->first > time) {
        return {};
    }

    auto current = std::chrono::system_clock::now();
    if (current >= time) {
        return {};
    }

    auto it = kvTasks_.insert({ current + interval, std::move(task->second) });
    if (it == kvTasks_.begin()) {
        condition_.notify_one();
    }
    kvTasks_.erase(task);
    return it;
}

void KvScheduler::Clean()
{
    std::unique_lock<std::mutex> lock(mutex_);
    kvTasks_.clear();
}

void KvScheduler::Every(const std::chrono::system_clock::duration interval, std::function<void()> task)
{
    std::function<void()> waitFunc = [this, interval, task]() {
        task();
        this->Every(interval, task);
    };
    At(std::chrono::system_clock::now() + interval, waitFunc);
}

void KvScheduler::Every(std::chrono::system_clock::duration delay, std::chrono::system_clock::duration interval,
                        std::function<void()> func)
{
    std::function<void()> waitFunc = [this, interval, func]() {
        func();
        this->Every(interval, func);
    };
    At(std::chrono::system_clock::now() + delay, waitFunc);
}

void KvScheduler::Every(int times, std::chrono::system_clock::duration delay,
    std::chrono::system_clock::duration interval, std::function<void()> func)
{
    std::function<void()> waitFunc = [this, times, interval, func]() {
        func();
        int count = times;
        count--;
        if (times > 1) {
            this->Every(count, interval, interval, func);
        }
    };

    At(std::chrono::system_clock::now() + delay, waitFunc);
}

void KvScheduler::Remove(const std::map<std::chrono::system_clock::time_point, std::function<void()>>::iterator &task)
{
    std::unique_lock<std::mutex> lock(mutex_);
    auto it = kvTasks_.find(task->first);
    if (it != kvTasks_.end()) {
        kvTasks_.erase(it);
        condition_.notify_one();
    }
}

void KvScheduler::Loop()
{
    while (isRunning_) {
        std::function<void()> exec;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this] { return !kvTasks_.empty(); });
            if (kvTasks_.begin()->first > std::chrono::system_clock::now()) {
                condition_.wait_until(lock, kvTasks_.begin()->first);
                continue;
            }
            exec = kvTasks_.begin()->second;
            kvTasks_.erase(kvTasks_.begin());
        }

        if (exec) {
            exec();
        }
    }
}
} // namespace OHOS::DistributedKv
