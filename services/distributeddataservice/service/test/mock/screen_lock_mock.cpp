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
#include "screen_lock_mock.h"

namespace OHOS {
namespace DistributedData {
bool ScreenLockMock::IsLocked()
{
    return isLocked_;
}

void ScreenLockMock::Subscribe(std::shared_ptr<Observer> observer)
{
    return;
}

void ScreenLockMock::Unsubscribe(std::shared_ptr<Observer> observer)
{
    return;
}

void ScreenLockMock::BindExecutor(std::shared_ptr<ExecutorPool> executors)
{
    return;
}

void ScreenLockMock::SubscribeScreenEvent()
{
    return;
}

void ScreenLockMock::UnsubscribeScreenEvent()
{
    return;
}
} // namespace DistributedData
} // namespace OHOS