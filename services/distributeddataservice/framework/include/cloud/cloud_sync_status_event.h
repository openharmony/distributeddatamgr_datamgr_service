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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_SYNC_STATUS_EVENT_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_SYNC_STATUS_EVENT_H

#include <string>
#include "cloud_event.h"

namespace OHOS::DistributedData {
class API_EXPORT CloudSyncStatusEvent : public CloudEvent {
public:
    enum SyncStatus : int32_t {
        SATAR,
        FINISH,
    };

    CloudSyncStatusEvent(StoreInfo storeInfo, SyncStatus status)
        : CloudEvent(CLOUD_SYNC_STATUS, std::move(storeInfo)), status_(std::move(status))
    {
    }

    ~CloudSyncStatusEvent() override = default;

    SyncStatus GetCloudSyncStatus() const
    {
        return status_ ;
    }
private:
    SyncStatus status_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_SYNC_STATUS_EVENT_H
