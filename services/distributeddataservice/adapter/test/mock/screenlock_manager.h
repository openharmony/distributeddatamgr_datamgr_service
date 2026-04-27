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

#ifndef MOCK_SCREENLOCK_MANAGER_H
#define MOCK_SCREENLOCK_MANAGER_H

#include <mutex>
#include <string>

namespace OHOS {
namespace ScreenLock {

class ScreenLockManager {
public:
    static ScreenLockManager* GetInstance();
    bool IsScreenLocked();
    
    static void SetScreenLocked(bool locked);
    static void Reset();

private:
    ScreenLockManager() = default;
    ~ScreenLockManager() = default;
    
    static std::mutex instanceLock_;
    static ScreenLockManager* instance_;
    static bool isScreenLocked_;
};

} // namespace ScreenLock
} // namespace OHOS

#endif // MOCK_SCREENLOCK_MANAGER_H