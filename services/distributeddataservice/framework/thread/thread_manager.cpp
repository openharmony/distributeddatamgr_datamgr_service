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
#define LOG_TAG "ThreadManager"
#include "thread/thread_manager.h"

#include "log_print.h"
#include "types.h"
#include "unistd.h"
namespace OHOS::DistributedData {
ThreadManager::ThreadManager()
{
}

ThreadManager &ThreadManager::GetInstance()
{
    static ThreadManager instance;
    return instance;
}

void ThreadManager::Initialize(uint32_t minThreadNum, uint32_t maxThreadNum, uint32_t ipcThreadNum)
{
    minThreadNum_ = minThreadNum;
    maxThreadNum_ = maxThreadNum;
    ipcThreadNum_ = ipcThreadNum;
}

uint32_t ThreadManager::GetMinThreadNum()
{
    return minThreadNum_;
}

uint32_t ThreadManager::GetMaxThreadNum()
{
    return maxThreadNum_;
}

uint32_t ThreadManager::GetIpcThreadNum()
{
    return ipcThreadNum_;
}
} // namespace OHOS::DistributedData