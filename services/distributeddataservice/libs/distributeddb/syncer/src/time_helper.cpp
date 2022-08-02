/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "time_helper.h"

#include "db_errno.h"
#include "log_print.h"
#include "platform_specific.h"

namespace DistributedDB {
std::mutex TimeHelper::systemTimeLock_;
Timestamp TimeHelper::lastSystemTimeUs_ = 0;
Timestamp TimeHelper::currentIncCount_ = 0;
std::atomic<Timestamp> TimeHelper::lastMonotonicTime_ = 0;

Timestamp TimeHelper::GetSysCurrentTime()
{
    uint64_t curTime = 0;
    std::lock_guard<std::mutex> lock(systemTimeLock_);
    int errCode = OS::GetCurrentSysTimeInMicrosecond(curTime);
    if (errCode != E_OK) {
        return INVALID_TIMESTAMP;
    }

    // If GetSysCurrentTime in 1us, we need increase the currentIncCount_
    if (curTime == lastSystemTimeUs_) {
        // if the currentIncCount_ has been increased MAX_INC_COUNT, keep the currentIncCount_
        if (currentIncCount_ < MAX_INC_COUNT) {
            currentIncCount_++;
        }
    } else {
        lastSystemTimeUs_ = curTime;
        currentIncCount_ = 0;
    }
    return (curTime * TO_100_NS) + currentIncCount_; // Currently Timestamp is uint64_t
}

TimeHelper::TimeHelper()
    : storage_(nullptr),
      metadata_(nullptr)
{
}

TimeHelper::~TimeHelper()
{
    metadata_ = nullptr;
    storage_ = nullptr;
}

int TimeHelper::Initialize(const ISyncInterface *inStorage, std::shared_ptr<Metadata> &inMetadata)
{
    if ((inStorage == nullptr) || (inMetadata == nullptr)) {
        return -E_INVALID_ARGS;
    }
    metadata_ = inMetadata;
    storage_ = inStorage;
    Timestamp currentSysTime = GetSysCurrentTime();
    TimeOffset localTimeOffset = GetLocalTimeOffset();
    Timestamp maxItemTime = GetMaxDataItemTime();
    if (currentSysTime > MAX_VALID_TIME || maxItemTime > MAX_VALID_TIME) {
        return -E_INVALID_TIME;
    }
    Timestamp virtualSysTime = static_cast<Timestamp>(currentSysTime + localTimeOffset);
    if (virtualSysTime <= maxItemTime || virtualSysTime > BUFFER_VALID_TIME) {
        localTimeOffset = static_cast<TimeOffset>(maxItemTime - currentSysTime + MS_TO_100_NS); // 1ms
        int errCode = SaveLocalTimeOffset(localTimeOffset);
        if (errCode != E_OK) {
            LOGE("[TimeHelper] save local time offset failed,err=%d", errCode);
            return errCode;
        }
    }
    lastMonotonicTime_ = GetMonotonicTime();
    metadata_->SetLastLocalTime(currentSysTime + static_cast<Timestamp>(localTimeOffset));
    return E_OK;
}

Timestamp TimeHelper::GetTime()
{
    Timestamp currentSysTime = GetSysCurrentTime();
    TimeOffset localTimeOffset = GetLocalTimeOffset();
    Timestamp currentLocalTime = currentSysTime + localTimeOffset;
    Timestamp lastLocalTime = metadata_->GetLastLocalTime();
    Timestamp currentMonotonicTime = GetMonotonicTime();
    Timestamp deltaTime = 1UL;
    if (currentMonotonicTime != INVALID_TIMESTAMP && lastMonotonicTime_ != INVALID_TIMESTAMP) {
        deltaTime = currentMonotonicTime - lastMonotonicTime_;
    }
    lastMonotonicTime_ = currentMonotonicTime;
    if (currentLocalTime <= lastLocalTime || currentLocalTime > BUFFER_VALID_TIME) {
        lastLocalTime += deltaTime;
        currentLocalTime = lastLocalTime;
        metadata_->SetLastLocalTime(lastLocalTime);
    } else {
        metadata_->SetLastLocalTime(currentLocalTime);
    }
    return currentLocalTime;
}

Timestamp TimeHelper::GetMaxDataItemTime()
{
    Timestamp timestamp = 0;
    storage_->GetMaxTimestamp(timestamp);
    return timestamp;
}

TimeOffset TimeHelper::GetLocalTimeOffset() const
{
    return metadata_->GetLocalTimeOffset();
}

int TimeHelper::SaveLocalTimeOffset(TimeOffset offset)
{
    return metadata_->SaveLocalTimeOffset(offset);
}

void TimeHelper::SetSendConfig(const std::string &dstTarget, bool nonBlock, uint32_t timeout, SendConfig &sendConf)
{
    SetSendConfigParam(storage_->GetDbProperties(), dstTarget, false, SEND_TIME_OUT, sendConf);
}

Timestamp TimeHelper::GetMonotonicTime()
{
    Timestamp time = INVALID_TIMESTAMP;
    int errCode = OS::GetMonotonicRelativeTimeInMicrosecond(time);
    if (errCode != E_OK) {
        LOGE("GetMonotonicTime ERR! errCode = %d", errCode);
    }
    return time;
}
} // namespace DistributedDB
