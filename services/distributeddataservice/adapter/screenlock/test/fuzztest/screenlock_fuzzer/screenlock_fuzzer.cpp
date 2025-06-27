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

#include <fuzzer/FuzzedDataProvider.h>
#include "screenlock_fuzzer.h"
#include "screenlock/screen_lock.h"

using namespace OHOS::DistributedData;
namespace OHOS {
class ScreenLockObserver : public ScreenManager::Observer {
public:
    void OnScreenUnlocked(int32_t user) override
    {
    }

    std::string GetName() override
    {
        return name_;
    }

    void SetName(const std::string &name)
    {
        name_ = name;
    }

private:
    std::string name_ = "screenTestObserver";
};

void SubscribeFuzz(FuzzedDataProvider &provider)
{
    auto screenLock = std::make_shared<ScreenLock>();
    screenLock->Subscribe(nullptr);
    auto observer = std::make_shared<ScreenLockObserver>();
    std::string name = provider.ConsumeRandomLengthString();
    observer->SetName(name);
    screenLock->Subscribe(observer);
    screenLock->IsLocked();
}

void UnsubscribeFuzz(FuzzedDataProvider &provider)
{
    auto screenLock = std::make_shared<ScreenLock>();
    auto observer = std::make_shared<ScreenLockObserver>();
    std::string name = provider.ConsumeRandomLengthString();
    observer->SetName(name);
    screenLock->Subscribe(observer);
    screenLock->Unsubscribe(observer);
    screenLock->UnsubscribeScreenEvent();
}

void GetTaskFuzz(FuzzedDataProvider &provider)
{
    auto screenLock = std::make_shared<ScreenLock>();
    int retry = provider.ConsumeIntegralInRange<int>(1, 20);
    screenLock->GetTask(retry);
}

void NotifyScreenUnlockedFuzz(FuzzedDataProvider &provider)
{
    auto screenLock = std::make_shared<ScreenLock>();
    int user = provider.ConsumeIntegral<int>();
    screenLock->NotifyScreenUnlocked(user);
}

void OnReceiveEventFuzz(FuzzedDataProvider &provider)
{
    OHOS::EventFwk::MatchingSkills matchingSkills;
    std::string reserved = provider.ConsumeRandomLengthString();
    matchingSkills.AddEvent(reserved);
    OHOS::EventFwk::CommonEventSubscribeInfo info(matchingSkills);
    int priority = provider.ConsumeIntegral<int>();
    info.SetPriority(priority);
    info.SetPermission("ohos.permission.SUBSCRIBE_SCREEN_LOCK");
    auto subscriber = std::make_shared<EventSubscriber>(info);
    OHOS::AAFwk::Want want;
    want.SetAction(CommonEventSupport::COMMON_EVENT_HWID_LOGIN);
    CommonEventData commonEventData(want);
    CommonEventData event(want);
    subscriber->OnReceiveEvent(event);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::SubscribeFuzz(provider);
    OHOS::UnsubscribeFuzz(provider);
    OHOS::GetTaskFuzz(provider);
    OHOS::NotifyScreenUnlockedFuzz(provider);
    OHOS::OnReceiveEventFuzz(provider);
    return 0;
}