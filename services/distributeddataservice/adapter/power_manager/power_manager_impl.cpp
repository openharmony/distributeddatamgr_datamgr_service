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

#define LOG_TAG "PowerManagerImpl"
#include "power_manager_impl.h"

#include "log_print.h"

namespace OHOS::DistributedData {
__attribute__((used)) static bool g_isInit = PowerManagerImpl::Register();

bool PowerManagerImpl::Register()
{
    static PowerManagerImpl instance;
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&]() { PowerManger::RegisterInstance(&instance); });
    return true;
}

PowerEventSubscriber::PowerEventSubscriber(const CommonEventSubscribeInfo &info) : CommonEventSubscriber(info)
{
}

void PowerEventSubscriber::SetEventCallback(PowerEventCallback callback)
{
    eventCallback_ = callback;
}

void PowerEventSubscriber::OnReceiveEvent(const CommonEventData &event)
{
    const auto want = event.GetWant();
    const auto action = want.GetAction();
    ZLOGI("Want Action is %{public}s", action.c_str());
    if (eventCallback_ == nullptr) {
        return;
    }
    if (action == CommonEventSupport::COMMON_EVENT_CHARGING) {
        eventCallback_(PowerManger::Observer::PowerEvent::CHARGING);
    } else if (action == CommonEventSupport::COMMON_EVENT_DISCHARGING) {
        eventCallback_(PowerManger::Observer::PowerEvent::DIS_CHARGING);
    }
}

// Delegate implementation
int32_t PowerManagerImpl::Delegate::Add(std::shared_ptr<Observer> observer)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = observers_.begin();
    while (it != observers_.end()) {
        auto obs = it->lock();
        if (obs == nullptr) {
            it = observers_.erase(it);
            continue;
        }
        if (obs.get() == observer.get()) {
            ZLOGD("observer already subscribed");
            return -1;
        }
        ++it;
    }
    observers_.push_back(observer);
    return 0;
}

int32_t PowerManagerImpl::Delegate::Remove(std::shared_ptr<Observer> observer)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = observers_.begin();
    while (it != observers_.end()) {
        auto obs = it->lock();
        if (obs == nullptr) {
            it = observers_.erase(it);
            continue;
        }
        if (obs.get() == observer.get()) {
            observers_.erase(it);
            return 0;
        }
        ++it;
    }
    ZLOGW("observer not found");
    return -1;
}

std::list<std::weak_ptr<PowerManger::Observer>> PowerManagerImpl::Delegate::GetObs()
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = observers_.begin();
    while (it != observers_.end()) {
        if (it->expired()) {
            it = observers_.erase(it);
        } else {
            ++it;
        }
    }
    return observers_;
}

void PowerManagerImpl::Delegate::SetCharging(bool charging)
{
    isCharging_ = charging;
}

bool PowerManagerImpl::Delegate::IsCharging()
{
    return isCharging_;
}

PowerManagerImpl::PowerManagerImpl() : delegate_(std::make_shared<Delegate>()) {}

int32_t PowerManagerImpl::Subscribe(std::shared_ptr<Observer> observer)
{
    if (observer == nullptr) {
        return -1;
    }
    return delegate_->Add(observer);
}

int32_t PowerManagerImpl::Unsubscribe(std::shared_ptr<Observer> observer)
{
    if (observer == nullptr) {
        return -1;
    }
    return delegate_->Remove(observer);
}

std::shared_ptr<PowerEventSubscriber> PowerManagerImpl::GetSubscriber()
{
    if (eventSubscriber_ != nullptr) {
        return eventSubscriber_;
    }
    MatchingSkills matchingSkills;
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_CHARGING);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_DISCHARGING);
    CommonEventSubscribeInfo info(matchingSkills);
    eventSubscriber_ = std::make_shared<PowerEventSubscriber>(info);
    eventSubscriber_->SetEventCallback([this](Observer::PowerEvent event) {
        NotifyPowerChanged(event);
    });
    return eventSubscriber_;
}

void PowerManagerImpl::SetSubscriber(std::shared_ptr<PowerEventSubscriber> subscriber)
{
    eventSubscriber_ = subscriber;
}

void PowerManagerImpl::SubscribePowerEvent()
{
    auto result = CommonEventManager::SubscribeCommonEvent(GetSubscriber());
    ZLOGI("register power subscriber: %{public}d.", result);
}

void PowerManagerImpl::UnsubscribePowerEvent()
{
    auto res = CommonEventManager::UnSubscribeCommonEvent(eventSubscriber_);
    ZLOGW("unregister power event res:%d", res);
}

void PowerManagerImpl::NotifyPowerChanged(Observer::PowerEvent event)
{
    if (event == Observer::PowerEvent::CHARGING) {
        delegate_->SetCharging(true);
    } else if (event == Observer::PowerEvent::DIS_CHARGING) {
        delegate_->SetCharging(false);
    } else {
        return;
    }
    auto observers = delegate_->GetObs();
    for (const auto &weakObs : observers) {
        auto obs = weakObs.lock();
        if (obs != nullptr) {
            obs->OnChang(event);
        }
    }
}

bool PowerManagerImpl::IsCharging()
{
    return delegate_->IsCharging();
}

PowerManagerImpl::~PowerManagerImpl() {}
} // namespace OHOS::DistributedData
