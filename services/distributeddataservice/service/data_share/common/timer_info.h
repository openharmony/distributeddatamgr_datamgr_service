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

#ifndef DATASHARESERVICE_TIMER_INFO_H
#define DATASHARESERVICE_TIMER_INFO_H

#include <functional>
#include "itimer_info.h"
#include "time_service_client.h"

namespace OHOS::DataShare {
using namespace OHOS::MiscServices;
class TimerInfo : public ITimerInfo {
public:
    TimerInfo();
    virtual ~TimerInfo();
    void OnTrigger() override;
    void SetType(const int &type) override;
    void SetRepeat(bool repeat) override;
    void SetInterval(const uint64_t &interval) override;
    void SetWantAgent(std::shared_ptr<AbilityRuntime::WantAgent::WantAgent> wantAgent) override;
    void SetCallbackInfo(const std::function<void()> &callBack);
private:
    std::function<void()> callBack_ = nullptr;
};

TimerInfo::TimerInfo()
{
}

TimerInfo::~TimerInfo()
{
}

void TimerInfo::OnTrigger()
{
    if (callBack_ != nullptr) {
        callBack_();
    }
}

void TimerInfo::SetCallbackInfo(const std::function<void()> &callBack)
{
    callBack_ = callBack;
}

void TimerInfo::SetType(const int &inputType)
{
    type = inputType;
}

void TimerInfo::SetRepeat(bool inputRepeat)
{
    repeat = inputRepeat;
}

void TimerInfo::SetInterval(const uint64_t &inputInterval)
{
    interval = inputInterval;
}

void TimerInfo::SetWantAgent(std::shared_ptr<AbilityRuntime::WantAgent::WantAgent> inputWantAgent)
{
    wantAgent = inputWantAgent;
}
}
#endif // DATASHARESERVICE_TIMER_INFO_H