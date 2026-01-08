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

#ifndef LDBPROJ_SCREEN_MANAGER_MOCK_H
#define LDBPROJ_SCREEN_MANAGER_MOCK_H

#include "screen/screen_manager.h"
#include <gmock/gmock.h>

namespace OHOS {
namespace DistributedData {
class ScreenManagerMock : public ScreenManager {
public:
    MOCK_METHOD(bool, IsLocked, (), (override));
    MOCK_METHOD(void, Subscribe, (std::shared_ptr<Observer> observer), (override));
    MOCK_METHOD(void, Unsubscribe, (std::shared_ptr<Observer> observer), (override));
    MOCK_METHOD(void, BindExecutor, (std::shared_ptr<ExecutorPool> executors), (override));
    MOCK_METHOD(void, SubscribeScreenEvent, (), (override));
    MOCK_METHOD(void, UnsubscribeScreenEvent, (), (override));
};
} // namespace DistributedData
} // namespace OHOS
#endif // LDBPROJ_SCREEN_MANAGER_MOCK_H