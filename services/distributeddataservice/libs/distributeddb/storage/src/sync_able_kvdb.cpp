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

#include "sync_able_kvdb.h"

#include "db_errno.h"
#include "log_print.h"
#include "parcel.h"
#include "runtime_context.h"
#include "user_change_monitor.h"

namespace DistributedDB {
const EventType SyncAbleKvDB::REMOTE_PUSH_FINISHED = 1;

SyncAbleKvDB::SyncAbleKvDB()
    : started_(false),
      isSyncModuleActiveCheck_(false),
      isSyncNeedActive_(true),
      notifyChain_(nullptr),
      userChangeListerner_(nullptr)
{}

SyncAbleKvDB::~SyncAbleKvDB()
{
    if (notifyChain_ != nullptr) {
        (void)notifyChain_->UnRegisterEventType(REMOTE_PUSH_FINISHED);
        KillAndDecObjRef(notifyChain_);
        notifyChain_ = nullptr;
    }
    if (userChangeListerner_ != nullptr) {
        userChangeListerner_->Drop(true);
        userChangeListerner_ = nullptr;
    }
}

void SyncAbleKvDB::DelConnection(GenericKvDBConnection *connection)
{
    auto realConnection = static_cast<SyncAbleKvDBConnection *>(connection);
    if (realConnection != nullptr) {
        KillAndDecObjRef(realConnection);
        realConnection = nullptr;
    }
}

void SyncAbleKvDB::TriggerSync(int notifyEvent)
{
    if (!started_) {
        StartSyncer();
    }
    if (started_) {
        syncer_.LocalDataChanged(notifyEvent);
    }
}

void SyncAbleKvDB::CommitNotify(int notifyEvent, KvDBCommitNotifyFilterAbleData *data)
{
    SyncAbleKvDB::TriggerSync(notifyEvent);

    GenericKvDB::CommitNotify(notifyEvent, data);
}

void SyncAbleKvDB::Close()
{
    StopSyncer();
}

// Start a sync action.
int SyncAbleKvDB::Sync(const ISyncer::SyncParma &parma)
{
    if (!started_) {
        StartSyncer();
        if (!started_) {
            return -E_NOT_INIT;
        }
    }
    return syncer_.Sync(parma);
}

void SyncAbleKvDB::EnableAutoSync(bool enable)
{
    if (!started_) {
        StartSyncer();
    }
    return syncer_.EnableAutoSync(enable);
}

void SyncAbleKvDB::WakeUpSyncer()
{
    StartSyncer();
}

// Stop a sync action in progress.
void SyncAbleKvDB::StopSync()
{
    if (started_) {
        syncer_.StopSync();
    }
}

void SyncAbleKvDB::SetSyncModuleActive()
{
    if (isSyncModuleActiveCheck_) {
        return;
    }
    IKvDBSyncInterface *syncInterface = GetSyncInterface();
    if (syncInterface == nullptr) {
        LOGF("KvDB got null sync interface.");
        return;
    }
    bool isSyncDualTupleMode = syncInterface->GetDbProperties().GetBoolProp(KvDBProperties::SYNC_DUAL_TUPLE_MODE,
        false);
    if (!isSyncDualTupleMode) {
        isSyncNeedActive_ = true;
        isSyncModuleActiveCheck_ = true;
        return;
    }
    std::string userId = syncInterface->GetDbProperties().GetStringProp(KvDBProperties::USER_ID, "");
    std::string appId = syncInterface->GetDbProperties().GetStringProp(KvDBProperties::APP_ID, "");
    std::string storeId = syncInterface->GetDbProperties().GetStringProp(KvDBProperties::STORE_ID, "");
    isSyncNeedActive_ = RuntimeContext::GetInstance()->IsSyncerNeedActive(userId, appId, storeId);
    if (!isSyncNeedActive_) {
        LOGI("syncer no need to active");
    }
    isSyncModuleActiveCheck_ = true;
}

bool SyncAbleKvDB::GetSyncModuleActive()
{
    return isSyncNeedActive_;
}

void SyncAbleKvDB::ReSetSyncModuleActive()
{
    isSyncModuleActiveCheck_ = false;
    isSyncNeedActive_ = true;
}

// Start syncer
void SyncAbleKvDB::StartSyncer(bool isCheckSyncActive, bool isNeedActive)
{
    IKvDBSyncInterface *syncInterface = GetSyncInterface();
    if (syncInterface == nullptr) {
        LOGF("KvDB got null sync interface.");
        return;
    }
    std::unique_lock<std::mutex> lock(syncerOperateLock_);
    if (!isCheckSyncActive) {
        SetSyncModuleActive();
        isNeedActive = GetSyncModuleActive();
    }
    int errCode = syncer_.Initialize(syncInterface, isNeedActive);
    if (errCode == E_OK) {
        started_ = true;
    } else {
        LOGE("KvDB start syncer failed, err:'%d'.", errCode);
    }
    bool isSyncDualTupleMode = syncInterface->GetDbProperties().GetBoolProp(KvDBProperties::SYNC_DUAL_TUPLE_MODE,
        false);
    if (isSyncDualTupleMode && isCheckSyncActive && !isNeedActive && (userChangeListerner_ == nullptr)) {
        // active to non_active
        userChangeListerner_ = RuntimeContext::GetInstance()->RegisterUserChangedListerner(
            std::bind(&SyncAbleKvDB::ChangeUserListerner, this), UserChangeMonitor::USER_ACTIVE_TO_NON_ACTIVE_EVENT);
    } else if (isSyncDualTupleMode && (userChangeListerner_ == nullptr)) {
        EventType event = started_?
            UserChangeMonitor::USER_ACTIVE_EVENT : UserChangeMonitor::USER_NON_ACTIVE_EVENT;
        userChangeListerner_ = RuntimeContext::GetInstance()->RegisterUserChangedListerner(
            std::bind(&SyncAbleKvDB::UserChangeHandle, this), event);
    }
}

// Stop syncer
void SyncAbleKvDB::StopSyncer()
{
    std::unique_lock<std::mutex> lock(syncerOperateLock_);
    ReSetSyncModuleActive();
    syncer_.Close();
    if (started_) {
        started_ = false;
    }
    if (userChangeListerner_ != nullptr) {
        userChangeListerner_->Drop(true);
        userChangeListerner_ = nullptr;
    }
}

void SyncAbleKvDB::UserChangeHandle()
{
    bool isNeedChange;
    bool isNeedActive = true;
    {
        IKvDBSyncInterface *syncInterface = GetSyncInterface();
        if (syncInterface == nullptr) {
            LOGF("KvDB got null sync interface.");
            return;
        }
        std::unique_lock<std::mutex> lock(syncerOperateLock_);
        std::string userId = syncInterface->GetDbProperties().GetStringProp(KvDBProperties::USER_ID, "");
        std::string appId = syncInterface->GetDbProperties().GetStringProp(KvDBProperties::APP_ID, "");
        std::string storeId = syncInterface->GetDbProperties().GetStringProp(KvDBProperties::STORE_ID, "");
        isNeedActive = RuntimeContext::GetInstance()->IsSyncerNeedActive(userId, appId, storeId);
        isNeedChange = (isNeedActive != isSyncNeedActive_) ? true : false;
    }
    // non_active to active or active to non_active
    if (isNeedChange) {
        StopSyncer(); // will drop userChangeListerner;
        {
            std::unique_lock<std::mutex> lock(syncerOperateLock_);
            isSyncModuleActiveCheck_ = true;
            isSyncNeedActive_ = isNeedActive;
        }
        StartSyncer(true, isNeedActive);
    }
}

void SyncAbleKvDB::ChangeUserListerner()
{
    // only active to non_active call, put into USER_NON_ACTIVE_EVENT listerner from USER_ACTIVE_TO_NON_ACTIVE_EVENT
    if (userChangeListerner_ != nullptr) {
        userChangeListerner_->Drop(true);
        userChangeListerner_ = nullptr;
    }
    if (userChangeListerner_ == nullptr) {
        userChangeListerner_ = RuntimeContext::GetInstance()->RegisterUserChangedListerner(
            std::bind(&SyncAbleKvDB::UserChangeHandle, this), UserChangeMonitor::USER_NON_ACTIVE_EVENT);
    }
}

// Get The current virtual timestamp
uint64_t SyncAbleKvDB::GetTimeStamp()
{
    if (!started_ && !isSyncModuleActiveCheck_) {
        StartSyncer();
    }
    return syncer_.GetTimeStamp();
}

// Get the dataItem's append length
uint32_t SyncAbleKvDB::GetAppendedLen() const
{
    return Parcel::GetAppendedLen();
}

int SyncAbleKvDB::EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash)
{
    if (!started_) {
        StartSyncer();
    }
    return syncer_.EraseDeviceWaterMark(deviceId, isNeedHash);
}

int SyncAbleKvDB::GetQueuedSyncSize(int *queuedSyncSize) const
{
    return syncer_.GetQueuedSyncSize(queuedSyncSize);
}

int SyncAbleKvDB::SetQueuedSyncLimit(const int *queuedSyncLimit)
{
    return syncer_.SetQueuedSyncLimit(queuedSyncLimit);
}

int SyncAbleKvDB::GetQueuedSyncLimit(int *queuedSyncLimit) const
{
    return syncer_.GetQueuedSyncLimit(queuedSyncLimit);
}

int SyncAbleKvDB::DisableManualSync(void)
{
    return syncer_.DisableManualSync();
}

int SyncAbleKvDB::EnableManualSync(void)
{
    return syncer_.EnableManualSync();
}

int SyncAbleKvDB::GetLocalIdentity(std::string &outTarget)
{
    return syncer_.GetLocalIdentity(outTarget);
}

int SyncAbleKvDB::SetStaleDataWipePolicy(WipePolicy policy)
{
    return syncer_.SetStaleDataWipePolicy(policy);
}

int SyncAbleKvDB::RegisterEventType(EventType type)
{
    if (notifyChain_ == nullptr) {
        notifyChain_ = new (std::nothrow) NotificationChain;
        if (notifyChain_ == nullptr) {
            return -E_OUT_OF_MEMORY;
        }
    }

    int errCode = notifyChain_->RegisterEventType(type);
    if (errCode == -E_ALREADY_REGISTER) {
        return E_OK;
    }
    if (errCode != E_OK) {
        LOGE("[SyncAbleKvDB] Register event type %u failed! err %d", type, errCode);
        KillAndDecObjRef(notifyChain_);
        notifyChain_ = nullptr;
    }
    return errCode;
}

NotificationChain::Listener *SyncAbleKvDB::AddRemotePushFinishedNotify(const RemotePushFinishedNotifier &notifier,
    int &errCode)
{
    std::unique_lock<std::shared_mutex> lock(notifyChainLock_);
    errCode = RegisterEventType(REMOTE_PUSH_FINISHED);
    if (errCode != E_OK) {
        return nullptr;
    }

    auto listener = notifyChain_->RegisterListener(REMOTE_PUSH_FINISHED,
        [notifier](void *arg) {
            if (arg == nullptr) {
                LOGE("PragmaRemotePushNotify is null.");
                return;
            }
            notifier(*static_cast<RemotePushNotifyInfo *>(arg));
        }, nullptr, errCode);
    if (errCode != E_OK) {
        LOGE("[SyncAbleKvDB] Add remote push finished notifier failed! err %d", errCode);
    }
    return listener;
}

void SyncAbleKvDB::NotifyRemotePushFinishedInner(const std::string &targetId) const
{
    {
        std::shared_lock<std::shared_mutex> lock(notifyChainLock_);
        if (notifyChain_ == nullptr) {
            return;
        }
    }
    RemotePushNotifyInfo info;
    info.deviceId = targetId;
    notifyChain_->NotifyEvent(REMOTE_PUSH_FINISHED, static_cast<void *>(&info));
}

int SyncAbleKvDB::SetSyncRetry(bool isRetry)
{
    return syncer_.SetSyncRetry(isRetry);
}

int SyncAbleKvDB::SetEqualIdentifier(const std::string &identifier, const std::vector<std::string> &targets)
{
    return syncer_.SetEqualIdentifier(identifier, targets);
}
}
