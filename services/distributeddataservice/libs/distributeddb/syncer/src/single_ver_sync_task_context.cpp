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

#include "single_ver_sync_task_context.h"

#include <algorithm>
#include "db_common.h"
#include "db_errno.h"
#include "log_print.h"
#include "isyncer.h"
#include "single_ver_sync_state_machine.h"
#include "single_ver_sync_target.h"
#include "sync_types.h"

namespace DistributedDB {
SingleVerSyncTaskContext::SingleVerSyncTaskContext()
    : SyncTaskContext(),
      token_(nullptr),
      endMark_(0),
      needClearRemoteStaleData_(false)
{}

SingleVerSyncTaskContext::~SingleVerSyncTaskContext()
{
    token_ = nullptr;
    subManager_ = nullptr;
}

int SingleVerSyncTaskContext::Initialize(const std::string &deviceId,
    ISyncInterface *syncInterface, std::shared_ptr<Metadata> &metadata, ICommunicator *communicator)
{
    if (deviceId.empty() || syncInterface == nullptr || metadata == nullptr ||
        communicator == nullptr) {
        return -E_INVALID_ARGS;
    }
    stateMachine_ = new (std::nothrow) SingleVerSyncStateMachine;
    if (stateMachine_ == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    deviceId_ = deviceId;
    TimerAction timeOutCallback;
    int errCode = stateMachine_->Initialize(this, syncInterface, metadata, communicator);
    if (errCode != E_OK) {
        LOGE("[SingleVerSyncTaskContext] stateMachine Initialize failed, err %d.", errCode);
        goto ERROR_OUT;
    }

    timeHelper_ = std::make_unique<TimeHelper>();
    errCode = timeHelper_->Initialize(syncInterface, metadata);
    if (errCode != E_OK) {
        LOGE("[SingleVerSyncTaskContext] timeHelper Initialize failed, err %d.", errCode);
        goto ERROR_OUT;
    }
    timeOutCallback = std::bind(&SyncStateMachine::TimeoutCallback,
        static_cast<SingleVerSyncStateMachine *>(stateMachine_),
        std::placeholders::_1);
    SetTimeoutCallback(timeOutCallback);

    syncInterface_ = syncInterface;
    communicator_ = communicator;
    taskExecStatus_ = INIT;
    OnKill([this]() { this->KillWait(); });
    {
        std::lock_guard<std::mutex> lock(synTaskContextSetLock_);
        synTaskContextSet_.insert(this);
    }
    return errCode;

ERROR_OUT:
    delete stateMachine_;
    stateMachine_ = nullptr;
    return errCode;
}

int SingleVerSyncTaskContext::AddSyncOperation(SyncOperation *operation)
{
    if (operation == nullptr) {
        return -E_INVALID_ARGS;
    }

    // If auto sync, just update the end watermark
    if (operation->IsAutoSync()) {
        std::lock_guard<std::mutex> lock(targetQueueLock_);
        bool isQuerySync = operation->IsQuerySync();
        std::string queryId = operation->GetQueryId();
        auto iter = std::find_if(requestTargetQueue_.begin(), requestTargetQueue_.end(),
            [isQuerySync, queryId](const ISyncTarget *target) {
            if (target == nullptr) {
                return false;
            }
            if (isQuerySync) {
                SyncOperation *tmpOperation = nullptr;
                target->GetSyncOperation(tmpOperation);
                return (tmpOperation != nullptr && tmpOperation->GetQueryId() == queryId) && target->IsAutoSync();
            }
            return target->IsAutoSync();
        });
        if (iter != requestTargetQueue_.end()) {
            static_cast<SingleVerSyncTarget *>(*iter)->SetEndWaterMark(timeHelper_->GetTime());
            operation->SetStatus(deviceId_, SyncOperation::OP_FINISHED_ALL);
            return E_OK;
        }
    }

    auto *newTarget = new (std::nothrow) SingleVerSyncTarget;
    if (newTarget == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    newTarget->SetSyncOperation(operation);
    TimeStamp timstamp = timeHelper_->GetTime();
    newTarget->SetEndWaterMark(timstamp);
    newTarget->SetTaskType(ISyncTarget::REQUEST);
    AddSyncTarget(newTarget);
    return E_OK;
}

void SingleVerSyncTaskContext::SetEndMark(WaterMark endMark)
{
    endMark_ = endMark;
}

WaterMark SingleVerSyncTaskContext::GetEndMark() const
{
    return endMark_;
}

void SingleVerSyncTaskContext::GetContinueToken(ContinueToken &outToken) const
{
    outToken = token_;
}

void SingleVerSyncTaskContext::SetContinueToken(ContinueToken token)
{
    token_ = token;
    return;
}

void SingleVerSyncTaskContext::ReleaseContinueToken()
{
    if (token_ != nullptr) {
        static_cast<SingleVerKvDBSyncInterface *>(syncInterface_)->ReleaseContinueToken(token_);
        token_ = nullptr;
    }
}

int SingleVerSyncTaskContext::PopResponseTarget(SingleVerSyncTarget &target)
{
    std::lock_guard<std::mutex> lock(targetQueueLock_);
    LOGD("[SingleVerSyncTaskContext] GetFrontExtWaterMarak size = %d", responseTargetQueue_.size());
    if (!responseTargetQueue_.empty()) {
        ISyncTarget *tmpTarget = responseTargetQueue_.front();
        responseTargetQueue_.pop_front();
        target = *(static_cast<SingleVerSyncTarget *>(tmpTarget));
        delete tmpTarget;
        tmpTarget = nullptr;
        return E_OK;
    }
    return -E_LENGTH_ERROR;
}

int SingleVerSyncTaskContext::GetRspTargetQueueSize() const
{
    std::lock_guard<std::mutex> lock(targetQueueLock_);
    return responseTargetQueue_.size();
}

void SingleVerSyncTaskContext::SetResponseSessionId(uint32_t responseSessionId)
{
    responseSessionId_ = responseSessionId;
}

uint32_t SingleVerSyncTaskContext::GetResponseSessionId() const
{
    return responseSessionId_;
}

void SingleVerSyncTaskContext::CopyTargetData(const ISyncTarget *target, const TaskParam &taskParam)
{
    const SingleVerSyncTarget *targetTmp = static_cast<const SingleVerSyncTarget *>(target);
    SyncTaskContext::CopyTargetData(target, taskParam);
    mode_ = targetTmp->GetMode();
    endMark_ = targetTmp->GetEndWaterMark();
    if (mode_ == SyncModeType::RESPONSE_PULL) {
        responseSessionId_ = targetTmp->GetResponseSessionId();
    }
    query_ = targetTmp->GetQuery();
    isQuerySync_ = targetTmp->IsQuerySync();
}

void SingleVerSyncTaskContext::Clear()
{
    retryTime_ = 0;
    ClearSyncOperation();
    SyncTaskContext::Clear();
    SetMode(SyncModeType::INVALID_MODE);
    syncId_ = 0;
    isAutoSync_ = false;
    SetOperationStatus(SyncOperation::OP_WAITING);
    SetEndMark(0);
    SetResponseSessionId(0);
    query_ = QuerySyncObject();
    isQuerySync_ = false;
}

void SingleVerSyncTaskContext::Abort(int status)
{
    {
        std::lock_guard<std::mutex> lock(operationLock_);
        if (syncOperation_ != nullptr) {
            syncOperation_->SetStatus(deviceId_, status);
            if ((status >= SyncOperation::OP_FINISHED_ALL)) {
                UnlockObj();
                if (syncOperation_->CheckIsAllFinished()) {
                    syncOperation_->Finished();
                }
                LockObj();
            }
        }
    }
    StopFeedDogForSync(SyncDirectionFlag::SEND);
    StopFeedDogForSync(SyncDirectionFlag::RECEIVE);
    Clear();
}

void SingleVerSyncTaskContext::ClearAllSyncTask()
{
    // clear request queue sync task and responsequeue first.
    std::list<ISyncTarget *> targetQueue;
    {
        std::lock_guard<std::mutex> lock(targetQueueLock_);
        LOGI("[SingleVerSyncTaskContext] request taskcount=%d，responsecount=%d", requestTargetQueue_.size(),
            responseTargetQueue_.size());
        while (!requestTargetQueue_.empty()) {
            ISyncTarget *tmpTarget = nullptr;
            tmpTarget = requestTargetQueue_.front();
            requestTargetQueue_.pop_front();
            targetQueue.push_back(tmpTarget);
        }
        while (!responseTargetQueue_.empty()) {
            ISyncTarget *tmpTarget = nullptr;
            tmpTarget = responseTargetQueue_.front();
            responseTargetQueue_.pop_front();
            delete tmpTarget;
            tmpTarget = nullptr;
        }
    }
    while (!targetQueue.empty()) {
        ISyncTarget *target = nullptr;
        target = targetQueue.front();
        targetQueue.pop_front();
        SyncOperation *tmpOperation = nullptr;
        target->GetSyncOperation(tmpOperation);
        if (tmpOperation == nullptr) {
            LOGE("[ClearAllSyncTask] tmpOperation is nullptr");
            continue; // not exit this scene
        }
        LOGI("[SingleVerSyncTaskContext] killing syncId=%d,dev=%s", tmpOperation->GetSyncId(), STR_MASK(deviceId_));
        if (target->IsAutoSync()) {
            tmpOperation->SetStatus(deviceId_, SyncOperation::OP_FINISHED_ALL);
        } else {
            tmpOperation->SetStatus(deviceId_, SyncOperation::OP_COMM_ABNORMAL);
        }
        if (tmpOperation->CheckIsAllFinished()) {
            tmpOperation->Finished();
        }
        delete target;
        target = nullptr;
    }
    if (GetTaskExecStatus() == SyncTaskContext::RUNNING) {
        // clear syncing task.
        isCommNormal_ = false;
        stateMachine_->CommErrAbort();
    }
    // reset last push status for sync merge
    ResetLastPushTaskStatus();
}

void SingleVerSyncTaskContext::EnableClearRemoteStaleData(bool enable)
{
    needClearRemoteStaleData_ = enable;
}

bool SingleVerSyncTaskContext::IsNeedClearRemoteStaleData() const
{
    return needClearRemoteStaleData_;
}

bool SingleVerSyncTaskContext::StartFeedDogForSync(uint32_t time, SyncDirectionFlag flag)
{
    return stateMachine_->StartFeedDogForSync(time, flag);
}

void SingleVerSyncTaskContext::StopFeedDogForSync(SyncDirectionFlag flag)
{
    stateMachine_->StopFeedDogForSync(flag);
}

int SingleVerSyncTaskContext::HandleDataRequestRecv(const Message *msg)
{
    return static_cast<SingleVerSyncStateMachine *>(stateMachine_)->HandleDataRequestRecv(msg);
}

bool SingleVerSyncTaskContext::IsReceiveWaterMarkErr() const
{
    return isReceiveWaterMarkErr_;
}

void SingleVerSyncTaskContext::SetReceiveWaterMarkErr(bool isErr)
{
    isReceiveWaterMarkErr_ = isErr;
}

void SingleVerSyncTaskContext::SetRemoteSeccurityOption(SecurityOption secOption)
{
    remoteSecOption_ = secOption;
}

SecurityOption SingleVerSyncTaskContext::GetRemoteSeccurityOption() const
{
    return remoteSecOption_;
}

void SingleVerSyncTaskContext::SetReceivcPermitCheck(bool isChecked)
{
    isReceivcPermitChecked_ = isChecked;
}

bool SingleVerSyncTaskContext::GetReceivcPermitCheck() const
{
    return isReceivcPermitChecked_;
}

void SingleVerSyncTaskContext::SetSendPermitCheck(bool isChecked)
{
    isSendPermitChecked_ = isChecked;
}

bool SingleVerSyncTaskContext::GetSendPermitCheck() const
{
    return isSendPermitChecked_;
}

void SingleVerSyncTaskContext::SetSyncStrategy(SyncStrategy strategy)
{
    syncStrategy_.permitSync = strategy.permitSync;
    syncStrategy_.convertOnSend = strategy.convertOnSend;
    syncStrategy_.convertOnReceive = strategy.convertOnReceive;
    syncStrategy_.checkOnReceive = strategy.checkOnReceive;
}

SyncStrategy SingleVerSyncTaskContext::GetSyncStrategy() const
{
    return syncStrategy_;
}

void SingleVerSyncTaskContext::SetIsSchemaSync(bool isSchemaSync)
{
    isSchemaSync_ = isSchemaSync;
}

bool SingleVerSyncTaskContext::GetIsSchemaSync() const
{
    return isSchemaSync_;
}

bool SingleVerSyncTaskContext::IsSkipTimeoutError(int errCode) const
{
    if (errCode == -E_TIMEOUT && IsSyncTaskNeedRetry() && (GetRetryTime() < GetSyncRetryTimes())) {
        LOGE("[SingleVerSyncTaskContext] send message timeout error occurred");
        return true;
    } else {
        return false;
    }
}

bool SingleVerSyncTaskContext::FindResponseSyncTarget(uint32_t responseSessionId) const
{
    std::lock_guard<std::mutex> lock(targetQueueLock_);
    auto iter = std::find_if(responseTargetQueue_.begin(), responseTargetQueue_.end(),
        [responseSessionId](const ISyncTarget *target) {
            return target->GetResponseSessionId() == responseSessionId;
        });
    if (iter == responseTargetQueue_.end()) {
        return false;
    }
    return true;
}

void SingleVerSyncTaskContext::SetQuery(const QuerySyncObject &query)
{
    query_ = query;
}

const QuerySyncObject &SingleVerSyncTaskContext::GetQuery() const
{
    return query_;
}

void SingleVerSyncTaskContext::SetQuerySync(bool isQuerySync)
{
    isQuerySync_ = isQuerySync;
}

bool SingleVerSyncTaskContext::IsQuerySync() const
{
    return isQuerySync_;
}

std::set<CompressAlgorithm> SingleVerSyncTaskContext::GetRemoteCompressAlgo() const
{
    std::set<CompressAlgorithm> compressAlgoSet;
    for (const auto &algo : COMPRESSALGOMAP) {
        if (remoteDbAbility_.GetAbilityItem(algo.second) == SUPPORT_MARK) {
            compressAlgoSet.insert(static_cast<CompressAlgorithm>(algo.first));
        }
    }
    return compressAlgoSet;
}

std::string SingleVerSyncTaskContext::GetRemoteCompressAlgoStr() const
{
    static std::map<CompressAlgorithm, std::string> algoMap = {{CompressAlgorithm::ZLIB, "zlib"}};
    std::set<CompressAlgorithm> remoteCompressAlgoSet = GetRemoteCompressAlgo();
    if (remoteCompressAlgoSet.size() == 0) {
        return "none";
    }
    std::string currentAlgoStr;
    for (const auto &algo : remoteCompressAlgoSet) {
        auto iter = algoMap.find(algo);
        if (iter != algoMap.end()) {
            currentAlgoStr += algoMap[algo] + ",";
        }
    }
    return currentAlgoStr.substr(0, currentAlgoStr.length() - 1);
}

void SingleVerSyncTaskContext::SetDbAbility(DbAbility &remoteDbAbility)
{
    remoteDbAbility_ = remoteDbAbility;
    LOGI("[SingleVerSyncTaskContext] set dev=%s compressAlgo=%s, IsSupAllPredicateQuery=%u, IsSupSubscribeQuery=%u",
        STR_MASK(GetDeviceId()), GetRemoteCompressAlgoStr().c_str(), remoteDbAbility.GetAbilityItem(ALLPREDICATEQUERY),
        remoteDbAbility.GetAbilityItem(SUBSCRIBEQUERY));
}

CompressAlgorithm SingleVerSyncTaskContext::ChooseCompressAlgo() const
{
    std::set<CompressAlgorithm> remoteAlgo = GetRemoteCompressAlgo();
    if (remoteAlgo.size() == 0) {
        return CompressAlgorithm::NONE;
    }
    std::set<CompressAlgorithm> localAlgorithmSet;
    (void)(static_cast<SingleVerKvDBSyncInterface *>(syncInterface_))->GetCompressionAlgo(localAlgorithmSet);
    std::set<CompressAlgorithm> algoIntersection;
    set_intersection(remoteAlgo.begin(), remoteAlgo.end(), localAlgorithmSet.begin(), localAlgorithmSet.end(),
        inserter(algoIntersection, algoIntersection.begin()));
    if (algoIntersection.size() == 0) {
        return CompressAlgorithm::NONE;
    }
    return *(algoIntersection.begin());
}

const DbAbility& SingleVerSyncTaskContext::GetRemoteDbAbility() const
{
    return remoteDbAbility_;
}

void SingleVerSyncTaskContext::SetSubscribeManager(std::shared_ptr<SubscribeManager> &subManager)
{
    subManager_ = subManager;
}

std::shared_ptr<SubscribeManager> SingleVerSyncTaskContext::GetSubscribeManager() const
{
    return subManager_;
}
DEFINE_OBJECT_TAG_FACILITIES(SingleVerSyncTaskContext)

bool SingleVerSyncTaskContext::IsCurrentSyncTaskCanBeSkipped() const
{
    if (mode_ == SyncModeType::PUSH) {
        if (lastFullSyncTaskStatus_ != SyncOperation::OP_FINISHED_ALL) {
            return false;
        }
    } else if (mode_ == SyncModeType::QUERY_PUSH) {
        auto it = lastQuerySyncTaskStatusMap_.find(syncOperation_->GetQueryId());
        if (it == lastQuerySyncTaskStatusMap_.end()) {
            // no last query_push and push
            if (lastFullSyncTaskStatus_ != SyncOperation::OP_FINISHED_ALL) {
                LOGD("no prev query push or successful prev push");
                return false;
            }
        } else {
            if (it->second != SyncOperation::OP_FINISHED_ALL) {
                LOGD("last query push status = %d.", it->second);
                return false;
            }
        }
    } else {
        return false;
    }

    TimeStamp maxTimeStampInDb;
    syncInterface_->GetMaxTimeStamp(maxTimeStampInDb);
    uint64_t localWaterMark = 0;
    int errCode = GetCorrectedSendWaterMarkForCurrentTask(localWaterMark);
    if (errCode != E_OK) {
        LOGE("GetLocalWaterMark in state machine failed: %d", errCode);
        return false;
    }
    if (localWaterMark > maxTimeStampInDb) {
        LOGI("skip current push task, deviceId_ = %s, localWaterMark = %llu, maxTimeStampInDb = %llu",
            STR_MASK(deviceId_), localWaterMark, maxTimeStampInDb);
        return true;
    }
    return false;
}

void SingleVerSyncTaskContext::SaveLastPushTaskExecStatus(int finalStatus)
{
    if (IsTargetQueueEmpty()) {
        LOGD("sync que is empty, reset last push status");
        ResetLastPushTaskStatus();
        return;
    }
    if (mode_ == SyncModeType::PUSH || mode_ == SyncModeType::PUSH_AND_PULL || mode_ == SyncModeType::RESPONSE_PULL) {
        lastFullSyncTaskStatus_ = finalStatus;
    } else if (mode_ == SyncModeType::QUERY_PUSH || mode_ == SyncModeType::QUERY_PUSH_PULL) {
        lastQuerySyncTaskStatusMap_[syncOperation_->GetQueryId()] = finalStatus;
    }
}

int SingleVerSyncTaskContext::GetCorrectedSendWaterMarkForCurrentTask(uint64_t &waterMark) const
{
    if (syncOperation_->IsQuerySync()) {
        LOGD("Is QuerySync");
        int errCode = static_cast<SingleVerSyncStateMachine *>(stateMachine_)->GetSendQueryWaterMark(
            syncOperation_->GetQueryId(), deviceId_,
            lastFullSyncTaskStatus_ == SyncOperation::OP_FINISHED_ALL, waterMark);
        if (errCode != E_OK) {
            return errCode;
        }
    } else {
        LOGD("Not QuerySync");
        static_cast<SingleVerSyncStateMachine *>(stateMachine_)->GetLocalWaterMark(deviceId_, waterMark);
    }
    return E_OK;
}

void SingleVerSyncTaskContext::ResetLastPushTaskStatus()
{
    lastFullSyncTaskStatus_ = SyncOperation::OP_WAITING;
    lastQuerySyncTaskStatusMap_.clear();
}
} // namespace DistributedDB
