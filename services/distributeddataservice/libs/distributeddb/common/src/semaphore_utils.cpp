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

#include "semaphore_utils.h"

#include <functional>

namespace DistributedDB {
using std::unique_lock;
using std::lock_guard;
using std::mutex;
using std::condition_variable;

SemaphoreUtils::SemaphoreUtils(int count)
    : count_(count)
{}

SemaphoreUtils::~SemaphoreUtils()
{}

bool SemaphoreUtils::WaitSemaphore(int waitSecond)
{
    unique_lock<mutex> lock(lockMutex_);
    bool result = cv_.wait_for(lock, std::chrono::seconds(waitSecond),
        std::bind(&SemaphoreUtils::CompareCount, this));
    if (result == true) {
        --count_;
    }
    return result;
}

void SemaphoreUtils::WaitSemaphore()
{
    unique_lock<mutex> lock(lockMutex_);
    cv_.wait(lock, std::bind(&SemaphoreUtils::CompareCount, this));
    --count_;
}

void SemaphoreUtils::SendSemaphore()
{
    lock_guard<std::mutex> lock(lockMutex_);
    count_++;
    cv_.notify_one();
}

bool SemaphoreUtils::CompareCount() const
{
    return count_ > 0;
}
}
