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
TimeStamp TimeHelper::lastSystemTimeUs_ = 0;
TimeStamp TimeHelper::currentIncCount_ = 0;

TimeStamp TimeHelper::GetSysCurrentTime()
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
    return (curTime * TO_100_NS) + currentIncCount_; // Currently TimeStamp is uint64_t
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
    TimeStamp currentSysTime = GetSysCurrentTime();
    TimeOffset localTimeOffset = GetLocalTimeOffset();
    TimeStamp maxItemTime = GetMaxDataItemTime();
    if (currentSysTime > MAX_VALID_TIME || localTimeOffset > MAX_VALID_TIME || maxItemTime > MAX_VALID_TIME) {
        return -E_INVALID_TIME;
    }
    if ((currentSysTime + localTimeOffset) <= maxItemTime) {
        localTimeOffset = static_cast<TimeOffset>(maxItemTime - currentSysTime + MS_TO_100_NS); // 1ms
        int errCode = SaveLocalTimeOffset(localTimeOffset);
        if (errCode != E_OK) {
            LOGE("[TimeHelper] save local time offset failed,err=%d", errCode);
            return errCode;
        }
    }
    metadata_->SetLastLocalTime(currentSysTime + static_cast<TimeStamp>(localTimeOffset));
    return E_OK;
}

TimeStamp TimeHelper::GetTime()
{
    TimeStamp currentSysTime = GetSysCurrentTime();
    TimeOffset localTimeOffset = GetLocalTimeOffset();
    TimeStamp currentLocalTime = currentSysTime + localTimeOffset;
    TimeStamp lastLocalTime = metadata_->GetLastLocalTime();
    if (currentLocalTime <= lastLocalTime || currentLocalTime > MAX_VALID_TIME) {
        lastLocalTime++;
        currentLocalTime = lastLocalTime;
        metadata_->SetLastLocalTime(lastLocalTime);
    } else {
        metadata_->SetLastLocalTime(currentLocalTime);
    }
    return currentLocalTime;
}

TimeStamp TimeHelper::GetMaxDataItemTime()
{
    TimeStamp timestamp = 0;
    storage_->GetMaxTimeStamp(timestamp);
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
    sendConf.nonBlock = nonBlock;
    sendConf.timeout = timeout;
    sendConf.isNeedExtendHead = storage_->GetDbProperties().GetBoolProp(KvDBProperties::SYNC_DUAL_TUPLE_MODE,
        false);
    sendConf.paramInfo.appId = storage_->GetDbProperties().GetStringProp(KvDBProperties::APP_ID, "");
    sendConf.paramInfo.userId = storage_->GetDbProperties().GetStringProp(KvDBProperties::USER_ID, "");
    sendConf.paramInfo.storeId = storage_->GetDbProperties().GetStringProp(KvDBProperties::STORE_ID, "");
    sendConf.paramInfo.dstTarget = dstTarget;
}
} // namespace DistributedDB
