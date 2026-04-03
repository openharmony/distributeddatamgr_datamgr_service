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

#ifndef MOCK_COMMON_EVENT_MANAGER_MOCK_H
#define MOCK_COMMON_EVENT_MANAGER_MOCK_H

#include <atomic>
#include <gmock/gmock.h>
#include <memory>

#include "common_event_manager.h"

namespace OHOS::EventFwk {

class CommonEventSubscriber;

class CommonEventManagerMock {
public:
    static CommonEventManagerMock *GetMock()
    {
        return mock.load();
    }

    CommonEventManagerMock();
    ~CommonEventManagerMock();

    MOCK_METHOD(bool, SubscribeCommonEvent, (const std::shared_ptr<CommonEventSubscriber> &));
    MOCK_METHOD(bool, UnSubscribeCommonEvent, (const std::shared_ptr<CommonEventSubscriber> &));

private:
    static inline std::atomic<CommonEventManagerMock *> mock = nullptr;
};

} // namespace OHOS::EventFwk

#endif // MOCK_COMMON_EVENT_MANAGER_MOCK_H
