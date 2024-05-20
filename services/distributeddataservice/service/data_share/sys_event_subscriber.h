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
#ifndef DATASHARESERVICE_SYS_EVENT_SUBSCRIBER_H
#define DATASHARESERVICE_SYS_EVENT_SUBSCRIBER_H

#include "common_event_subscriber.h"

#include "data_share_service_impl.h"

namespace OHOS::DataShare {
class SysEventSubscriber : public EventFwk::CommonEventSubscriber {
public:
    using SysEventCallback = void (SysEventSubscriber::*)();
    explicit SysEventSubscriber(const EventFwk::CommonEventSubscribeInfo &info);
    ~SysEventSubscriber() {}
    void OnReceiveEvent(const EventFwk::CommonEventData& event) override;

private:
    void OnBMSReady();
    void NotifyDataShareReady();
    std::map<std::string, SysEventCallback> callbacks_;
};
}
#endif