/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef MOCK_AUTO_LAUNCH_H
#define MOCK_AUTO_LAUNCH_H

#include <gmock/gmock.h>
#include "auto_launch.h"

namespace DistributedDB {
class MockAutoLaunch : public AutoLaunch {
public:

    void SetAutoLaunchItem(const std::string &identify, AutoLaunchItem &item)
    {
        std::lock_guard<std::mutex> autoLock(extLock_);
        extItemMap_[identify] = item;
    }

    void CallExtConnectionLifeCycleCallbackTask(const std::string &identifier)
    {
        AutoLaunch::ExtConnectionLifeCycleCallbackTask(identifier);
    }

    MOCK_METHOD1(TryCloseConnection, void(AutoLaunchItem &));
};
} // namespace DistributedDB
#endif  // #define MOCK_AUTO_LAUNCH_H