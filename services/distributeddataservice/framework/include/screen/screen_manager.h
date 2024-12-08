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
#ifndef DISTRIBUTEDDATAMGR_FRAMEWORK_SCREEN_LOCK_H
#define DISTRIBUTEDDATAMGR_FRAMEWORK_SCREEN_LOCK_H

#include <memory>
#include <mutex>

#include "executor_pool.h"
#include "visibility.h"

namespace OHOS {
namespace DistributedData {
class ScreenManager {
public:
    class Observer {
    public:
        virtual ~Observer() = default;
        virtual void OnScreenUnlocked(int32_t user) = 0;
        virtual std::string GetName() = 0;
    };
    API_EXPORT static std::shared_ptr<ScreenManager> GetInstance();
    API_EXPORT static bool RegisterInstance(std::shared_ptr<ScreenManager> instance);
    ScreenManager() = default;
    virtual ~ScreenManager() = default;
    virtual bool IsLocked();
    virtual void Subscribe(std::shared_ptr<Observer> observer);
    virtual void Unsubscribe(std::shared_ptr<Observer> observer);
    virtual void BindExecutor(std::shared_ptr<ExecutorPool> executors);
    virtual void SubscribeScreenEvent();
    virtual void UnsubscribeScreenEvent();

private:
    static std::mutex mutex_;
    static std::shared_ptr<ScreenManager> instance_;
};
} // namespace DistributedData
} // namespace OHOS
#endif //DISTRIBUTEDDATAMGR_FRAMEWORK_SCREEN_LOCK_H