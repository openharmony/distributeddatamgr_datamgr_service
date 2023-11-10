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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_COMMON_EVENT_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_COMMON_EVENT_H

#include "cloud/cloud_event.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class API_EXPORT DataChangeEvent : public CloudEvent {
public:
    struct TableChangeProperties {
        bool isTrackedDataChange = false;
    };
    using TableProperties = std::map<std::string, TableChangeProperties>;
    struct EventInfo {
        TableProperties tableProperties;
    };

    DataChangeEvent(StoreInfo storeInfo, EventInfo evtInfo)
        : CloudEvent(DATA_CHANGE, std::move(storeInfo)), info_(std::move(evtInfo))
    {
    }

    ~DataChangeEvent() override = default;

    TableProperties GetTableProperties() const
    {
        return info_.tableProperties;
    }
private:
    EventInfo info_;
};
} // OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_COMMON_EVENT_H