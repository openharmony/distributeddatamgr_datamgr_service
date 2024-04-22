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

#include "cloud/sync_event.h"

namespace OHOS::DistributedData {
SyncEvent::EventInfo::EventInfo(int32_t mode, int32_t wait, bool retry, std::shared_ptr<GenQuery> query, GenAsync async)
    : retry_(retry), mode_(mode), wait_(wait), query_(std::move(query)), asyncDetail_(std::move(async))
{
}
SyncEvent::EventInfo::EventInfo(const SyncParam &syncParam, bool retry, std::shared_ptr<GenQuery> query, GenAsync async)
    : retry_(retry), mode_(syncParam.mode), wait_(syncParam.wait), query_(std::move(query)),
      asyncDetail_(std::move(async)), isCompensation_(syncParam.isCompensation)
{
}

SyncEvent::EventInfo::EventInfo(SyncEvent::EventInfo &&info) noexcept
{
    operator=(std::move(info));
}

SyncEvent::EventInfo &SyncEvent::EventInfo::operator=(SyncEvent::EventInfo &&info) noexcept
{
    if (this == &info) {
        return *this;
    }
    retry_ = info.retry_;
    mode_ = info.mode_;
    wait_ = info.wait_;
    query_ = std::move(info.query_);
    asyncDetail_ = std::move(info.asyncDetail_);
    isCompensation_ = info.isCompensation_;
    return *this;
}

SyncEvent::SyncEvent(StoreInfo storeInfo, EventInfo info)
    : CloudEvent(CLOUD_SYNC, std::move(storeInfo)), info_(std::move(info))
{
}

SyncEvent::SyncEvent(int32_t evtId, StoreInfo storeInfo, EventInfo info)
    : CloudEvent(evtId, std::move(storeInfo)), info_(std::move(info))
{
}

int32_t SyncEvent::GetMode() const
{
    return info_.mode_;
}

int32_t SyncEvent::GetWait() const
{
    return info_.wait_;
}

bool SyncEvent::AutoRetry() const
{
    return info_.retry_;
}

std::shared_ptr<GenQuery> SyncEvent::GetQuery() const
{
    return info_.query_;
}

GenAsync SyncEvent::GetAsyncDetail() const
{
    return info_.asyncDetail_;
}

bool SyncEvent::IsCompensation() const
{
    return info_.isCompensation_;
}
} // namespace OHOS::DistributedData