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
#ifndef DISTRIBUTEDDATAMGR_ADAPTER_SCREEN_LOCK_H
#define DISTRIBUTEDDATAMGR_ADAPTER_SCREEN_LOCK_H

#include "common_event_manager.h"
#include "common_event_subscriber.h"
#include "common_event_support.h"
#include "concurrent_map.h"
#include "screen/screen_manager.h"

namespace OHOS {
namespace DistributedData {
using namespace OHOS::EventFwk;
using EventCallback = std::function<void(int32_t user)>;

class EventSubscriber final : public CommonEventSubscriber {
public:
    ~EventSubscriber() {}
    explicit EventSubscriber(const CommonEventSubscribeInfo &info);
    void SetEventCallback(EventCallback callback);
    void OnReceiveEvent(const CommonEventData &event) override;
private:
    static constexpr const char *USER_ID = "userId";
    static constexpr int32_t INVALID_USER = -1;
    EventCallback eventCallback_ {};
};

class ScreenLock : public ScreenManager {
public:
    bool IsLocked();
    void Subscribe(std::shared_ptr<Observer> observer) __attribute__((no_sanitize("cfi")));
    void Unsubscribe(std::shared_ptr<Observer> observer) __attribute__((no_sanitize("cfi")));
    void BindExecutor(std::shared_ptr<ExecutorPool> executors);
    void SubscribeScreenEvent();
    void UnsubscribeScreenEvent();
    ~ScreenLock();
private:
    static constexpr int32_t MAX_RETRY_TIMES = 300;
    static constexpr int32_t RETRY_WAIT_TIME_S = 1;
    void NotifyScreenUnlocked(int32_t user);
    ExecutorPool::Task GetTask(uint32_t retry);
    ConcurrentMap<std::string, std::shared_ptr<Observer>> observerMap_{};
    std::shared_ptr<ExecutorPool> executors_;
    std::shared_ptr<EventSubscriber> eventSubscriber_{};
};
} // namespace DistributedData
} // namespace OHOS
#endif //DISTRIBUTEDDATAMGR_ADAPTER_SCREEN_LOCK_H