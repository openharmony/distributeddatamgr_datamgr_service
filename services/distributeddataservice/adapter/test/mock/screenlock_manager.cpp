/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "screenlock_manager.h"

namespace OHOS {
namespace ScreenLock {

std::mutex ScreenLockManager::instanceLock_;
ScreenLockManager* ScreenLockManager::instance_ = nullptr;
bool ScreenLockManager::isScreenLocked_ = false;

ScreenLockManager* ScreenLockManager::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> autoLock(instanceLock_);
        if (instance_ == nullptr) {
            instance_ = new (std::nothrow) ScreenLockManager;
        }
    }
    return instance_;
}

bool ScreenLockManager::IsScreenLocked()
{
    return isScreenLocked_;
}

void ScreenLockManager::SetScreenLocked(bool locked)
{
    isScreenLocked_ = locked;
}

void ScreenLockManager::Reset()
{
    isScreenLocked_ = false;
}

} // namespace ScreenLock
} // namespace OHOS