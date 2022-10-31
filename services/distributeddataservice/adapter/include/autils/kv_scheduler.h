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

#ifndef KV_SCHEDULER_H
#define KV_SCHEDULER_H

#include <condition_variable>
#include <mutex>
#include <thread>
#include <functional>
#include <chrono>
#include <map>
#include "visibility.h"

namespace OHOS::DistributedKv {
using SchedulerTask =  std::map<std::chrono::system_clock::time_point, std::function<void()>>::iterator;

class KvScheduler {
public:
    KVSTORE_API KvScheduler(size_t capacity = std::numeric_limits<size_t>::max());
    KVSTORE_API ~KvScheduler();
    // execute task at specific time
    KVSTORE_API SchedulerTask At(const std::chrono::system_clock::time_point &time, std::function<void()> task);

    KVSTORE_API SchedulerTask Reset(SchedulerTask task, const std::chrono::system_clock::time_point &time,
                                    const std::chrono::system_clock::duration &interval);
    KVSTORE_API void Clean();

    // execute task periodically with duration
    KVSTORE_API void Every(const std::chrono::system_clock::duration interval, std::function<void()> task);
    // remove task in SchedulerTask
    KVSTORE_API void Remove(const SchedulerTask &task);
    // execute task periodically with duration after delay
    KVSTORE_API void Every(std::chrono::system_clock::duration delay,
               std::chrono::system_clock::duration interval, std::function<void()> func);
    // execute task for some times periodically with duration after delay
    KVSTORE_API void Every(int times, std::chrono::system_clock::duration delay,
               std::chrono::system_clock::duration interval, std::function<void()> func);
    KVSTORE_API void Now(std::function<void()> task);

private:
    void Loop();
    volatile bool isRunning_;
    size_t capacity_;
    std::multimap<std::chrono::system_clock::time_point, std::function<void()>> kvTasks_;
    std::mutex mutex_;
    std::unique_ptr<std::thread> thread_;
    std::condition_variable condition_;
};
} // namespace OHOS::DistributedKv
#endif // KV_SCHEDULER_H
