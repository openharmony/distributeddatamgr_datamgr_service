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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_COMMON_EVENT_SET_SEARCHABLE_EVENT_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_COMMON_EVENT_SET_SEARCHABLE_EVENT_H

#include "cloud/cloud_event.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class API_EXPORT SetSearchableEvent : public CloudEvent {
public:
    struct EventInfo {
        bool isSearchable = false;
        bool isRebuild = false;
    };

    SetSearchableEvent(StoreInfo storeInfo, EventInfo evtInfo, int32_t evtId = SET_SEARCHABLE)
        : CloudEvent(evtId, std::move(storeInfo)), info_(std::move(evtInfo))
    {
    }

    ~SetSearchableEvent() override = default;

    bool GetIsSearchabl() const
    {
        return info_.isSearchable;
    }

    bool GetIsRebuild() const
    {
        return info_.isRebuild;
    }
private:
    EventInfo info_;
};
} // OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_COMMON_EVENT_SET_SEARCHABLE_EVENT_H