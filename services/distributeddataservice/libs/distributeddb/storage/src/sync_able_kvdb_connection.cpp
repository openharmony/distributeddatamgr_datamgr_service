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

#include "sync_able_kvdb_connection.h"

#include "log_print.h"
#include "db_errno.h"
#include "db_constant.h"
#include "kvdb_pragma.h"
#include "performance_analysis.h"
#include "runtime_context.h"
#include "sync_able_kvdb.h"

namespace DistributedDB {
SyncAbleKvDBConnection::SyncAbleKvDBConnection(SyncAbleKvDB *kvDB)
    : GenericKvDBConnection(kvDB),
      remotePushFinishedListener_(nullptr)
{
    OnKill([this]() {
        auto *db = GetDB<SyncAbleKvDB>();
        if (db == nullptr) {
            return;
        }
        // Drop the lock before we call RemoveSyncOperation().
        UnlockObj();
        db->StopSync(GetConnectionId());
        LockObj();
    });
}

SyncAbleKvDBConnection::~SyncAbleKvDBConnection()
{
    if (remotePushFinishedListener_ != nullptr) {
        remotePushFinishedListener_->Drop(true);
    }
    remotePushFinishedListener_ = nullptr;
}

void SyncAbleKvDBConnection::InitPragmaFunc()
{
    if (!pragmaFunc_.empty()) {
        return;
    }
    pragmaFunc_ = {
        {PRAGMA_SYNC_DEVICES, [this](void *parameter, int &errCode) {
            errCode = PragmaSyncAction(static_cast<PragmaSync *>(parameter)); }},
        {PRAGMA_AUTO_SYNC, [this](void *parameter, int &errCode) {
            errCode = EnableAutoSync(*(static_cast<bool *>(parameter))); }},
        {PRAGMA_PERFORMANCE_ANALYSIS_GET_REPORT, [](void *parameter, int &errCode) {
            *(static_cast<std::string *>(parameter)) = PerformanceAnalysis::GetInstance()->GetStatistics(); }},
        {PRAGMA_PERFORMANCE_ANALYSIS_OPEN, [](void *parameter, int &errCode) {
            PerformanceAnalysis::GetInstance()->OpenPerformanceAnalysis(); }},
        {PRAGMA_PERFORMANCE_ANALYSIS_CLOSE, [](void *parameter, int &errCode) {
            PerformanceAnalysis::GetInstance()->ClosePerformanceAnalysis(); }},
        {PRAGMA_PERFORMANCE_ANALYSIS_SET_REPORTFILENAME,  [](void *parameter, int &errCode) {
            PerformanceAnalysis::GetInstance()->SetFileNumber(*(static_cast<std::string *>(parameter))); }},
        {PRAGMA_GET_QUEUED_SYNC_SIZE, [this](void *parameter, int &errCode) {
            errCode = GetQueuedSyncSize(static_cast<int *>(parameter)); }},
        {PRAGMA_SET_QUEUED_SYNC_LIMIT, [this](void *parameter, int &errCode) {
            errCode = SetQueuedSyncLimit(static_cast<int *>(parameter)); }},
        {PRAGMA_GET_QUEUED_SYNC_LIMIT, [this](void *parameter, int &errCode) {
            errCode = GetQueuedSyncLimit(static_cast<int *>(parameter)); }},
        {PRAGMA_SET_WIPE_POLICY, [this](void *parameter, int &errCode) {
            errCode = SetStaleDataWipePolicy(static_cast<WipePolicy *>(parameter)); }},
        {PRAGMA_REMOTE_PUSH_FINISHED_NOTIFY, [this](void *parameter, int &errCode) {
            errCode = SetRemotePushFinishedNotify(static_cast<PragmaRemotePushNotify *>(parameter)); }},
        {PRAGMA_SET_SYNC_RETRY, [this](void *parameter, int &errCode) {
            errCode = SetSyncRetry(*(static_cast<bool *>(parameter))); }},
        {PRAGMA_ADD_EQUAL_IDENTIFIER, [this](void *parameter, int &errCode) {
            errCode = SetEqualIdentifier(static_cast<PragmaSetEqualIdentifier *>(parameter)); }},
        {PRAGMA_INTERCEPT_SYNC_DATA, [this](void *parameter, int &errCode) {
            errCode = SetPushDataInterceptor(*static_cast<PushDataInterceptor *>(parameter)); }},
        {PRAGMA_SUBSCRIBE_QUERY, [this](void *parameter, int &errCode) {
            errCode = PragmaSyncAction(static_cast<PragmaSync *>(parameter)); }},
    };
}

int SyncAbleKvDBConnection::Pragma(int cmd, void *parameter)
{
    int errCode = PragmaParamCheck(cmd, parameter);
    if (errCode != E_OK) {
        return -E_INVALID_ARGS;
    }

    InitPragmaFunc();
    auto iter = pragmaFunc_.find(cmd);
    if (iter != pragmaFunc_.end()) {
        iter->second(parameter, errCode);
        return errCode;
    }

    // Call Pragma() of super class.
    return GenericKvDBConnection::Pragma(cmd, parameter);
}

int SyncAbleKvDBConnection::PragmaParamCheck(int cmd, const void *parameter)
{
    switch (cmd) {
        case PRAGMA_AUTO_SYNC:
        case PRAGMA_PERFORMANCE_ANALYSIS_GET_REPORT:
        case PRAGMA_PERFORMANCE_ANALYSIS_SET_REPORTFILENAME:
            if (parameter == nullptr) {
                return -E_INVALID_ARGS;
            }
            return E_OK;
        default:
            return E_OK;
    }
}

int SyncAbleKvDBConnection::PragmaSyncAction(const PragmaSync *syncParameter)
{
    if (syncParameter == nullptr) {
        return -E_INVALID_ARGS;
    }
    SyncAbleKvDB *kvDB = GetDB<SyncAbleKvDB>();
    if (kvDB == nullptr) {
        return -E_INVALID_CONNECTION;
    }

    if (isExclusive_.load()) {
        return -E_BUSY;
    }
    {
        AutoLock lockGuard(this);
        if (IsKilled()) {
            // If this happens, users are using a closed connection.
            LOGE("Pragma sync on a closed connection.");
            return -E_STALE;
        }
        IncObjRef(this);
    }

    ISyncer::SyncParma syncParam;
    syncParam.devices = syncParameter->devices_;
    syncParam.mode = syncParameter->mode_;
    syncParam.wait = syncParameter->wait_;
    syncParam.isQuerySync = syncParameter->isQuerySync_;
    syncParam.syncQuery = syncParameter->query_;
    syncParam.onFinalize =  [this]() { DecObjRef(this); };
    syncParam.onComplete = std::bind(&SyncAbleKvDBConnection::OnSyncComplete, this, std::placeholders::_1,
        syncParameter->onComplete_, syncParameter->wait_);
    int errCode = kvDB->Sync(syncParam, GetConnectionId());
    if (errCode != E_OK) {
        DecObjRef(this);
    }
    return errCode;
}

int SyncAbleKvDBConnection::EnableAutoSync(bool enable)
{
    SyncAbleKvDB *kvDB = GetDB<SyncAbleKvDB>();
    if (kvDB == nullptr) {
        return -E_INVALID_CONNECTION;
    }
    kvDB->EnableAutoSync(enable);
    return E_OK;
}

void SyncAbleKvDBConnection::OnSyncComplete(const std::map<std::string, int> &statuses,
    const std::function<void(const std::map<std::string, int> &devicesMap)> &onComplete, bool wait)
{
    AutoLock lockGuard(this);
    if (!IsKilled() && onComplete) {
        // Drop the lock before invoking the callback.
        // Do pragma-sync again in the prev sync callback is supported.
        UnlockObj();
        // The connection may be closed after UnlockObj().
        // RACE: 'KillObj()' against 'onComplete()'.
        if (!IsKilled()) {
            onComplete(statuses);
        }
        LockObj();
    }
}

int SyncAbleKvDBConnection::GetQueuedSyncSize(int *queuedSyncSize) const
{
    if (queuedSyncSize == nullptr) {
        return -E_INVALID_ARGS;
    }
    SyncAbleKvDB *kvDB = GetDB<SyncAbleKvDB>();
    if (kvDB == nullptr) {
        return -E_INVALID_CONNECTION;
    }
    return kvDB->GetQueuedSyncSize(queuedSyncSize);
}

int SyncAbleKvDBConnection::SetQueuedSyncLimit(const int *queuedSyncLimit)
{
    if (queuedSyncLimit == nullptr) {
        return -E_INVALID_ARGS;
    }
    if ((*queuedSyncLimit > DBConstant::QUEUED_SYNC_LIMIT_MAX) ||
        (*queuedSyncLimit < DBConstant::QUEUED_SYNC_LIMIT_MIN)) {
        return -E_INVALID_ARGS;
    }
    SyncAbleKvDB *kvDB = GetDB<SyncAbleKvDB>();
    if (kvDB == nullptr) {
        return -E_INVALID_CONNECTION;
    }
    return kvDB->SetQueuedSyncLimit(queuedSyncLimit);
}

int SyncAbleKvDBConnection::GetQueuedSyncLimit(int *queuedSyncLimit) const
{
    if (queuedSyncLimit == nullptr) {
        return -E_INVALID_ARGS;
    }
    SyncAbleKvDB *kvDB = GetDB<SyncAbleKvDB>();
    if (kvDB == nullptr) {
        return -E_INVALID_CONNECTION;
    }
    return kvDB->GetQueuedSyncLimit(queuedSyncLimit);
}

int SyncAbleKvDBConnection::DisableManualSync(void)
{
    SyncAbleKvDB *kvDB = GetDB<SyncAbleKvDB>();
    if (kvDB == nullptr) {
        return -E_INVALID_CONNECTION;
    }
    return kvDB->DisableManualSync();
}

int SyncAbleKvDBConnection::EnableManualSync(void)
{
    SyncAbleKvDB *kvDB = GetDB<SyncAbleKvDB>();
    if (kvDB == nullptr) {
        return -E_INVALID_CONNECTION;
    }
    return kvDB->EnableManualSync();
}

int SyncAbleKvDBConnection::SetStaleDataWipePolicy(const WipePolicy *policy)
{
    if (policy == nullptr) {
        return -E_INVALID_ARGS;
    }
    SyncAbleKvDB *kvDB = GetDB<SyncAbleKvDB>();
    if (kvDB == nullptr) {
        return -E_INVALID_CONNECTION;
    }
    return kvDB->SetStaleDataWipePolicy(*policy);
}

int SyncAbleKvDBConnection::SetRemotePushFinishedNotify(PragmaRemotePushNotify *notifyParma)
{
    if (notifyParma == nullptr) {
        return -E_INVALID_ARGS;
    }

    SyncAbleKvDB *kvDB = GetDB<SyncAbleKvDB>();
    if (kvDB == nullptr) {
        return -E_INVALID_CONNECTION;
    }

    int errCode = E_OK;
    NotificationChain::Listener *tmpListener = nullptr;
    if (notifyParma->notifier_ != nullptr) {
        tmpListener = kvDB->AddRemotePushFinishedNotify(notifyParma->notifier_, errCode);
        if (tmpListener == nullptr) {
            return errCode;
        }
    }

    std::lock_guard<std::mutex> lock(remotePushFinishedListenerLock_);
    // Drop old listener and set the new listener
    if (remotePushFinishedListener_ != nullptr) {
        errCode = remotePushFinishedListener_->Drop();
        if (errCode != E_OK) {
            LOGE("[SyncAbleConnection] Drop Remote push finished listener failed %d", errCode);
            if (tmpListener != nullptr) {
                tmpListener->Drop();
            }
            return errCode;
        }
    }
    remotePushFinishedListener_ = tmpListener;
    return errCode;
}

int SyncAbleKvDBConnection::SetSyncRetry(bool isRetry)
{
    SyncAbleKvDB *kvDB = GetDB<SyncAbleKvDB>();
    if (kvDB == nullptr) {
        return -E_INVALID_CONNECTION;
    }
    return kvDB->SetSyncRetry(isRetry);
}

int SyncAbleKvDBConnection::SetEqualIdentifier(const PragmaSetEqualIdentifier *param)
{
    if (param == nullptr) {
        return -E_INVALID_ARGS;
    }

    SyncAbleKvDB *kvDB = GetDB<SyncAbleKvDB>();
    if (kvDB == nullptr) {
        return -E_INVALID_CONNECTION;
    }
    return kvDB->SetEqualIdentifier(param->identifier_, param->targets_);
}

int SyncAbleKvDBConnection::SetPushDataInterceptor(const PushDataInterceptor &interceptor)
{
    SyncAbleKvDB *kvDB = GetDB<SyncAbleKvDB>();
    if (kvDB == nullptr) {
        return -E_INVALID_CONNECTION;
    }
    kvDB->SetDataInterceptor(interceptor);
    return E_OK;
}
}
