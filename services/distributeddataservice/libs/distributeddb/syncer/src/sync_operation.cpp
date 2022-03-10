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

#include "sync_operation.h"
#include "db_errno.h"
#include "log_print.h"
#include "performance_analysis.h"

namespace DistributedDB {
SyncOperation::SyncOperation(uint32_t syncId, const std::vector<std::string> &devices,
    int mode, const UserCallback &userCallback, bool isBlockSync)
    : devices_(devices),
      syncId_(syncId),
      mode_(mode),
      userCallback_(userCallback),
      isBlockSync_(isBlockSync),
      isAutoSync_(false),
      isFinished_(false),
      semaphore_(nullptr),
      query_(QuerySyncObject()),
      isQuerySync_(false),
      isAutoSubscribe_(false)
{
}

SyncOperation::~SyncOperation()
{
    LOGD("SyncOperation::~SyncOperation()");
    Finalize();
}

int SyncOperation::Initialize()
{
    LOGD("[SyncOperation] Init SyncOperation id:%d.", syncId_);
    AutoLock lockGuard(this);
    for (const std::string &deviceId : devices_) {
        statuses_.insert(std::pair<std::string, int>(deviceId, OP_WAITING));
    }

    if (mode_ == AUTO_PUSH) {
        mode_ = PUSH;
        isAutoSync_ = true;
    } else if (mode_ == AUTO_PULL) {
        mode_ = PULL;
        isAutoSync_ = true;
    } else if (mode_ == AUTO_SUBSCRIBE_QUERY) {
        mode_ = SUBSCRIBE_QUERY;
        isAutoSubscribe_ = true;
    }
    if (isBlockSync_) {
        semaphore_ = std::make_unique<SemaphoreUtils>(0);
    }

    return E_OK;
}

void SyncOperation::SetOnSyncFinalize(const OnSyncFinalize &callback)
{
    onFinalize_ = callback;
}

void SyncOperation::SetOnSyncFinished(const OnSyncFinished &callback)
{
    onFinished_ = callback;
}

void SyncOperation::SetStatus(const std::string &deviceId, int status)
{
    LOGD("[SyncOperation] SetStatus dev %s{private} status %d", deviceId.c_str(), status);
    AutoLock lockGuard(this);
    if (IsKilled()) {
        LOGE("[SyncOperation] SetStatus failed, the SyncOperation has been killed!");
        return;
    }
    if (isFinished_) {
        LOGI("[SyncOperation] SetStatus already finished");
        return;
    }

    auto iter = statuses_.find(deviceId);
    if (iter != statuses_.end()) {
        if (iter->second >= OP_FINISHED_ALL) {
            return;
        }
        iter->second = status;
        return;
    }
}

void SyncOperation::SetUnfinishedDevStatus(int status)
{
    LOGD("[SyncOperation] SetUnfinishedDevStatus status %d", status);
    AutoLock lockGuard(this);
    if (IsKilled()) {
        LOGE("[SyncOperation] SetUnfinishedDevStatus failed, the SyncOperation has been killed!");
        return;
    }
    if (isFinished_) {
        LOGI("[SyncOperation] SetUnfinishedDevStatus already finished");
        return;
    }
    for (auto &item : statuses_) {
        if (item.second >= OP_FINISHED_ALL) {
            continue;
        }
        item.second = status;
    }
}

int SyncOperation::GetStatus(const std::string &deviceId) const
{
    AutoLock lockGuard(this);
    auto iter = statuses_.find(deviceId);
    if (iter != statuses_.end()) {
        return iter->second;
    }
    return -E_INVALID_ARGS;
}

uint32_t SyncOperation::GetSyncId() const
{
    return syncId_;
}

int SyncOperation::GetMode() const
{
    return mode_;
}

void SyncOperation::Finished()
{
    std::map<std::string, int> tmpStatus;
    {
        AutoLock lockGuard(this);
        if (IsKilled() || isFinished_) {
            return;
        }
        isFinished_ = true;
        tmpStatus = statuses_;
    }
    PerformanceAnalysis *performance = PerformanceAnalysis::GetInstance();
    if (performance != nullptr) {
        performance->StepTimeRecordEnd(PT_TEST_RECORDS::RECORD_ACK_RECV_TO_USER_CALL_BACK);
    }
    if (userCallback_) {
        LOGI("[SyncOperation] Sync %d finished call onComplete.", syncId_);
        if (IsBlockSync()) {
            userCallback_(tmpStatus);
        } else {
            RefObject::IncObjRef(this);
            int errCode = RuntimeContext::GetInstance()->ScheduleQueuedTask(identifier_, [this, tmpStatus] {
                userCallback_(tmpStatus);
                RefObject::DecObjRef(this);
            });
            if (errCode != E_OK) {
                LOGE("[Finished] SyncOperation Finished userCallback_ retCode:%d", errCode);
                RefObject::DecObjRef(this);
            }
        }
    }
    if (onFinished_) {
        LOGD("[SyncOperation] Sync %d finished call onFinished.", syncId_);
        onFinished_(syncId_);
    }
}

const std::vector<std::string> &SyncOperation::GetDevices() const
{
    return devices_;
}

void SyncOperation::WaitIfNeed()
{
    if (isBlockSync_ && (semaphore_ != nullptr)) {
        LOGD("[SyncOperation] Wait.");
        semaphore_->WaitSemaphore();
    }
}

void SyncOperation::NotifyIfNeed()
{
    if (isBlockSync_ && (semaphore_ != nullptr)) {
        LOGD("[SyncOperation] Notify.");
        semaphore_->SendSemaphore();
    }
}

bool SyncOperation::IsAutoSync() const
{
    return isAutoSync_;
}

bool SyncOperation::IsBlockSync() const
{
    return isBlockSync_;
}

bool SyncOperation::IsAutoControlCmd() const
{
    return isAutoSubscribe_;
}

bool SyncOperation::CheckIsAllFinished() const
{
    AutoLock lockGuard(this);
    for (const auto &iter : statuses_) {
        if (iter.second < OP_FINISHED_ALL) {
            return false;
        }
    }
    return true;
}

void SyncOperation::Finalize()
{
    if ((syncId_ > 0) && onFinalize_) {
        LOGD("[SyncOperation] Callback SyncOperation onFinalize.");
        onFinalize_();
    }
}

void SyncOperation::SetQuery(const QuerySyncObject &query)
{
    query_ = query;
    isQuerySync_ = true;
    if (mode_ != SyncModeType::SUBSCRIBE_QUERY && mode_ != SyncModeType::UNSUBSCRIBE_QUERY) {
        mode_ += QUERY_SYNC_MODE_BASE;
    }
}

QuerySyncObject SyncOperation::GetQuery() const
{
    return query_;
}

bool SyncOperation::IsQuerySync() const
{
    return isQuerySync_;
}

void SyncOperation::SetIdentifier(const std::vector<uint8_t> &identifier)
{
    identifier_.assign(identifier.begin(), identifier.end());
}

SyncType SyncOperation::GetSyncType(int mode)
{
    static const std::map<int, SyncType> syncTypeMap = {
        {SyncModeType::PUSH, SyncType::MANUAL_FULL_SYNC_TYPE},
        {SyncModeType::PULL, SyncType::MANUAL_FULL_SYNC_TYPE},
        {SyncModeType::PUSH_AND_PULL, SyncType::MANUAL_FULL_SYNC_TYPE},
        {SyncModeType::RESPONSE_PULL, SyncType::MANUAL_FULL_SYNC_TYPE},
        {SyncModeType::AUTO_PULL, SyncType::AUTO_SYNC_TYPE},
        {SyncModeType::AUTO_PUSH, SyncType::AUTO_SYNC_TYPE},
        {SyncModeType::QUERY_PUSH, SyncType::QUERY_SYNC_TYPE},
        {SyncModeType::QUERY_PULL, SyncType::QUERY_SYNC_TYPE},
        {SyncModeType::QUERY_PUSH_PULL, SyncType::QUERY_SYNC_TYPE},
    };
    auto iter = syncTypeMap.find(mode);
    if (iter != syncTypeMap.end()) {
        return iter->second;
    }
    return SyncType::INVALID_SYNC_TYPE;
}

int SyncOperation::TransferSyncMode(int mode)
{
    // AUTO_PUSH and AUTO_PULL mode is used before sync, RESPONSE_PULL is regarded as push or query push mode.
    // so for the three mode, it is no need to transferred.
    if (mode >= SyncModeType::QUERY_PUSH && mode <= SyncModeType::QUERY_PUSH_PULL) {
        return (mode - QUERY_SYNC_MODE_BASE);
    }
    return mode;
}

std::string SyncOperation::GetQueryId() const
{
    return query_.GetIdentify();
}

const std::map<int, DBStatus> &SyncOperation::DBStatusTransMap()
{
    static const std::map<int, DBStatus> transMap = {
        { static_cast<int>(OP_FINISHED_ALL),                  OK },
        { static_cast<int>(OP_TIMEOUT),                       TIME_OUT },
        { static_cast<int>(OP_PERMISSION_CHECK_FAILED),       PERMISSION_CHECK_FORBID_SYNC },
        { static_cast<int>(OP_COMM_ABNORMAL),                 COMM_FAILURE },
        { static_cast<int>(OP_SECURITY_OPTION_CHECK_FAILURE), SECURITY_OPTION_CHECK_ERROR },
        { static_cast<int>(OP_EKEYREVOKED_FAILURE),           EKEYREVOKED_ERROR },
        { static_cast<int>(OP_SCHEMA_INCOMPATIBLE),           SCHEMA_MISMATCH },
        { static_cast<int>(OP_BUSY_FAILURE),                  BUSY },
        { static_cast<int>(OP_QUERY_FORMAT_FAILURE),          INVALID_QUERY_FORMAT },
        { static_cast<int>(OP_QUERY_FIELD_FAILURE),           INVALID_QUERY_FIELD },
        { static_cast<int>(OP_NOT_SUPPORT),                   NOT_SUPPORT },
        { static_cast<int>(OP_INTERCEPT_DATA_FAIL),           INTERCEPT_DATA_FAIL },
        { static_cast<int>(OP_MAX_LIMITS),                    OVER_MAX_LIMITS },
        { static_cast<int>(OP_SCHEMA_CHANGED),                DISTRIBUTED_SCHEMA_CHANGED },
        { static_cast<int>(OP_INVALID_ARGS),                  INVALID_ARGS },
        { static_cast<int>(OP_USER_CHANGED),                  USER_CHANGED},
    };
    return transMap;
}
DEFINE_OBJECT_TAG_FACILITIES(SyncOperation)
} // namespace DistributedDB