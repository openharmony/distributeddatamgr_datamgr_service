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
#define LOG_TAG "SysEventSubscriber"
#include "sys_event_subscriber.h"

#include "common_event_manager.h"
#include "common_event_support.h"
#include "log_print.h"

namespace OHOS::DataShare {
SysEventSubscriber::SysEventSubscriber(const EventFwk::CommonEventSubscribeInfo& info)
    : CommonEventSubscriber(info)
{
    callbacks_ = { { EventFwk::CommonEventSupport::COMMON_EVENT_BUNDLE_SCAN_FINISHED,
        &SysEventSubscriber::OnBMSReady } };
}

void SysEventSubscriber::OnReceiveEvent(const EventFwk::CommonEventData& event)
{
    EventFwk::Want want = event.GetWant();
    std::string action = want.GetAction();
    auto it = callbacks_.find(action);
    if (it != callbacks_.end()) {
        (this->*(it->second))();
    }
}

void SysEventSubscriber::OnBMSReady()
{
    NotifyDataShareReady();
}

void SysEventSubscriber::NotifyDataShareReady()
{
    AAFwk::Want want;
    want.SetAction("usual.event.DATA_SHARE_READY");
    EventFwk::CommonEventData CommonEventData { want };
    EventFwk::CommonEventPublishInfo publishInfo;
    publishInfo.SetSticky(true);
    if (!EventFwk::CommonEventManager::PublishCommonEvent(CommonEventData, publishInfo)) {
        ZLOGE("Notify dataShare ready failed.");
        return;
    }
    ZLOGI("Notify dataShare ready succeed.");
}
}
