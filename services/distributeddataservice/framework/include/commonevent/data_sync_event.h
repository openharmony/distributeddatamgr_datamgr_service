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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_COMMON_EVENT_DATA_SYNC_EVENT_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_COMMON_EVENT_DATA_SYNC_EVENT_H

#include "cloud/cloud_event.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class API_EXPORT DataSyncEvent : public CloudEvent {
public:
    enum DataSyncStatus : int32_t {
        START,
        FINISH,
    };

    DataSyncEvent(StoreInfo storeInfo, uint32_t syncMode, int32_t status, uint32_t traceId, uint32_t count = 0)
        : CloudEvent(DATA_SYNC, std::move(storeInfo)), syncMode_(syncMode), status_(std::move(status)),
        traceId_(traceId), count_(count)
    {
    }

    ~DataSyncEvent() override = default;

    uint32_t GetSyncMode() const
    {
        return syncMode_;
    }

    int32_t GetSyncStatus() const
    {
        return status_;
    }

    uint32_t GetTraceId() const
    {
        return traceId_;
    }

    uint32_t GetCount() const
    {
        return count_;
    }

private:
    uint32_t syncMode_;
    int32_t status_;
    uint32_t traceId_;
    uint32_t count_;
};
} // OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_COMMON_EVENT_DATA_SYNC_EVENT_H