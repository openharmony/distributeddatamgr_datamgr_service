/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#define LOG_TAG "ScreenManager"
#include "screen/screen_manager.h"

#include "log_print.h"

namespace OHOS::DistributedData {
std::mutex ScreenManager::mutex_;
std::shared_ptr<ScreenManager> ScreenManager::instance_ = nullptr;
std::shared_ptr<ScreenManager> ScreenManager::GetInstance()
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (instance_ != nullptr) {
        return instance_;
    }
    instance_ = std::make_shared<ScreenManager>();
    ZLOGW("no register, new instance!");
    return instance_;
}

bool ScreenManager::RegisterInstance(std::shared_ptr<ScreenManager> instance)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    instance_ = std::move(instance);
    return true;
}

// LCOV_EXCL_START
void ScreenManager::Subscribe(std::shared_ptr<Observer> observer)
{
    return;
}

void ScreenManager::Unsubscribe(std::shared_ptr<Observer> observer)
{
    return;
}

void ScreenManager::BindExecutor(std::shared_ptr<ExecutorPool> executors)
{
    return;
}

void ScreenManager::SubscribeScreenEvent()
{
    return;
}

void ScreenManager::UnsubscribeScreenEvent()
{
    return;
}

bool ScreenManager::IsLocked()
{
    return false;
}
// LCOV_EXCL_STOP
} // namespace OHOS::DistributedData