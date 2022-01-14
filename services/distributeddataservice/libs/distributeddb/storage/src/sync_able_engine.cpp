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
#include "sync_able_engine.h"

#include "db_errno.h"
#include "log_print.h"
#include "parcel.h"
#include "runtime_context.h"

namespace DistributedDB {
SyncAbleEngine::SyncAbleEngine(ISyncInterface *store)
    : syncer_(),
      started_(false),
      store_(store)
{}

SyncAbleEngine::~SyncAbleEngine()
{}

// Start a sync action.
int SyncAbleEngine::Sync(const ISyncer::SyncParma &parm)
{
    if (!started_) {
        StartSyncer();
        if (!started_) {
            return -E_NOT_INIT;
        }
    }
    return syncer_.Sync(parm);
}

void SyncAbleEngine::WakeUpSyncer()
{
    StartSyncer();
}

void SyncAbleEngine::Close()
{
    StopSyncer();
}

void SyncAbleEngine::EnableAutoSync(bool enable)
{
    if (!started_) {
        StartSyncer();
    }
    return syncer_.EnableAutoSync(enable);
}

int SyncAbleEngine::EnableManualSync(void)
{
    return syncer_.EnableManualSync();
}

int SyncAbleEngine::DisableManualSync(void)
{
    return syncer_.DisableManualSync();
}

// Stop a sync action in progress.
void SyncAbleEngine::StopSync(int syncId)
{
    if (started_) {
        syncer_.RemoveSyncOperation(syncId);
    }
}

// Get The current virtual timestamp
uint64_t SyncAbleEngine::GetTimeStamp()
{
    if (!started_) {
        StartSyncer();
    }
    return syncer_.GetTimeStamp();
}

int SyncAbleEngine::EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash)
{
    if (!started_) {
        StartSyncer();
    }
    return syncer_.EraseDeviceWaterMark(deviceId, isNeedHash);
}

// Start syncer
void SyncAbleEngine::StartSyncer()
{
    if (store_ == nullptr) {
        LOGF("KvDB got null sync interface.");
        return;
    }

    int errCode = syncer_.Initialize(store_);
    if (errCode == E_OK) {
        started_ = true;
    } else {
        LOGE("KvDB start syncer failed, err:'%d'.", errCode);
    }
}

// Stop syncer
void SyncAbleEngine::StopSyncer()
{
    if (started_) {
        syncer_.Close();
        started_ = false;
    }
}

void SyncAbleEngine::TriggerSync(int notifyEvent)
{
    if (!started_) {
        StartSyncer();
    }
    if (started_) {
        int errCode = RuntimeContext::GetInstance()->ScheduleTask([this, notifyEvent] {
            syncer_.LocalDataChanged(notifyEvent);
        });
        if (errCode != E_OK) {
            LOGE("[TriggerSync] SyncAbleEngine TriggerSync LocalDataChanged retCode:%d", errCode);
        }
    }
}

int SyncAbleEngine::GetLocalIdentity(std::string &outTarget)
{
    if (!started_) {
        StartSyncer();
    }
    return syncer_.GetLocalIdentity(outTarget);
}
}
