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

#define LOG_TAG "screen_lock"
#include "screen_lock.h"

#include "screenlock_manager.h"

namespace OHOS::DistributedData {
using namespace OHOS::ScreenLock;
__attribute__((used)) static bool g_init =
    ScreenManager::RegisterInstance(std::static_pointer_cast<ScreenManager>(std::make_shared<ScreenLock>()));
bool ScreenLock::IsLocked()
{
    auto manager = ScreenLockManager::GetInstance();
    if (manager == nullptr) {
        return false;
    }
    return manager->IsScreenLocked();
}
} // namespace OHOS::DistributedData