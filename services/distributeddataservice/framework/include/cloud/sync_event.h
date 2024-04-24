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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SYNC_EVENT_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SYNC_EVENT_H
#include "cloud/cloud_event.h"
#include "store/general_value.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class API_EXPORT SyncEvent : public CloudEvent {
public:
    class EventInfo {
    public:
        API_EXPORT EventInfo(int32_t mode, int32_t wait, bool retry, std::shared_ptr<GenQuery> query, GenAsync async);
        API_EXPORT EventInfo(const SyncParam &syncParam, bool retry, std::shared_ptr<GenQuery> query, GenAsync async);
        API_EXPORT EventInfo(EventInfo &&info) noexcept;
        EventInfo(const EventInfo &info) = default;
        API_EXPORT EventInfo &operator=(EventInfo &&info) noexcept;
        EventInfo &operator=(const EventInfo &info) = default;
    private:
        friend SyncEvent;
        bool retry_ = false;
        int32_t mode_ = -1;
        int32_t wait_ = 0;
        std::shared_ptr<GenQuery> query_;
        GenAsync asyncDetail_;
        bool isCompensation_ = false;
    };
    SyncEvent(StoreInfo storeInfo, EventInfo info);
    ~SyncEvent() override = default;
    int32_t GetMode() const;
    int32_t GetWait() const;
    bool AutoRetry() const;
    std::shared_ptr<GenQuery> GetQuery() const;
    GenAsync GetAsyncDetail() const;
    bool IsCompensation() const;

protected:
    SyncEvent(int32_t evtId, StoreInfo storeInfo, EventInfo info);

private:
    EventInfo info_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SYNC_EVENT_H
