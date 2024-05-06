/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_EVENT_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_EVENT_H

#include <string>
#include "eventcenter/event.h"
#include "store/store_info.h"

namespace OHOS::DistributedData {
class API_EXPORT CloudEvent : public Event {
public:
    enum : int32_t {
        FEATURE_INIT = EVT_CLOUD,
        GET_SCHEMA,
        LOCAL_CHANGE,
        CLOUD_SYNC,
        DATA_CHANGE,
        CLOUD_SHARE,
        MAKE_QUERY,
        CLOUD_SYNC_FINISHED,
        CLOUD_BUTT
    };

    CloudEvent(int32_t evtId, StoreInfo storeInfo);
    ~CloudEvent() = default;
    const StoreInfo& GetStoreInfo() const;

private:
    StoreInfo storeInfo_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_EVENT_H
