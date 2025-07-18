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

#define LOG_TAG "ScreenLock"
#include "screen_lock.h"

#include "account/account_delegate.h"
#include "log_print.h"
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

EventSubscriber::EventSubscriber(const CommonEventSubscribeInfo &info) : CommonEventSubscriber(info)
{
}

void EventSubscriber::OnReceiveEvent(const CommonEventData &event)
{
    const auto want = event.GetWant();
    const auto action = want.GetAction();
    if (action != CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED) {
        return;
    }
    ZLOGI("Want Action is %{public}s", action.c_str());
    auto user = want.GetIntParam(USER_ID, INVALID_USER);
    if (user == INVALID_USER) {
        std::vector<int32_t> users;
        AccountDelegate::GetInstance()->QueryForegroundUsers(users);
        if (users.empty()) {
            return;
        }
        user = users[0];
    }
    eventCallback_(user);
}

void EventSubscriber::SetEventCallback(EventCallback callback)
{
    eventCallback_ = callback;
}

void ScreenLock::Subscribe(std::shared_ptr<Observer> observer)
{
    if (observer == nullptr || observer->GetName().empty() || observerMap_.Contains(observer->GetName())) {
        return;
    }
    if (!observerMap_.Insert(observer->GetName(), observer)) {
        ZLOGE("fail, name is %{public}s", observer->GetName().c_str());
    }
}

void ScreenLock::Unsubscribe(std::shared_ptr<Observer> observer)
{
    if (observer == nullptr || observer->GetName().empty() || !observerMap_.Contains(observer->GetName())) {
        return;
    }
    if (!observerMap_.Erase(observer->GetName())) {
        ZLOGD("fail, name is %{public}s", observer->GetName().c_str());
    }
}

void ScreenLock::SubscribeScreenEvent()
{
    ZLOGI("Subscribe screen event listener start.");
    if (eventSubscriber_ == nullptr) {
        MatchingSkills matchingSkills;
        matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED);
        CommonEventSubscribeInfo info(matchingSkills);
        eventSubscriber_ = std::make_shared<EventSubscriber>(info);
        eventSubscriber_->SetEventCallback([this](int32_t user) {
            NotifyScreenUnlocked(user);
        });
    }
    if (executors_ != nullptr) {
        executors_->Execute(GetTask(0));
    }
}

void ScreenLock::UnsubscribeScreenEvent()
{
    auto res = CommonEventManager::UnSubscribeCommonEvent(eventSubscriber_);
    if (!res) {
        ZLOGW("unregister screen event fail res:%d", res);
    }
}

void ScreenLock::NotifyScreenUnlocked(int32_t user)
{
    observerMap_.ForEach([user](const auto &key, auto &val) {
        val->OnScreenUnlocked(user);
        return false;
    });
}

void ScreenLock::BindExecutor(std::shared_ptr<ExecutorPool> executors)
{
    executors_ = executors;
}

ExecutorPool::Task ScreenLock::GetTask(uint32_t retry)
{
    return [this, retry] {
        auto result = CommonEventManager::SubscribeCommonEvent(eventSubscriber_);
        if (result) {
            ZLOGI("success to register subscriber.");
            return;
        }
        ZLOGD("fail to register subscriber, error:%{public}d, time:%{public}d", result, retry);

        if (retry + 1 > MAX_RETRY_TIME) {
            ZLOGE("fail to register subscriber!");
            return;
        }
        if (executors_ != nullptr) {
            executors_->Schedule(std::chrono::seconds(RETRY_WAIT_TIME_S), GetTask(retry + 1));
        }
    };
}
} // namespace OHOS::DistributedData