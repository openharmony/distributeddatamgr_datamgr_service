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

#include "syncer_proxy.h"

#include "syncer_factory.h"
#include "db_errno.h"
#include "log_print.h"

namespace DistributedDB {
SyncerProxy::SyncerProxy()
    : syncer_(nullptr)
{
}

int SyncerProxy::Initialize(ISyncInterface *syncInterface, bool isNeedActive)
{
    if (syncInterface == nullptr) {
        return -E_INVALID_ARGS;
    }

    int interfaceType = syncInterface->GetInterfaceType();
    {
        std::lock_guard<std::mutex> lock(syncerLock_);
        if (syncer_ == nullptr) {
            syncer_ = SyncerFactory::GetSyncer(interfaceType);
        }
    }
    if (syncer_ == nullptr) {
        LOGF("syncer create failed! invalid interface type %d", interfaceType);
        return -E_OUT_OF_MEMORY;
    }

    return syncer_->Initialize(syncInterface, isNeedActive);
}

int SyncerProxy::Close(bool isClosedOperation)
{
    if (syncer_ == nullptr) {
        return -E_NOT_INIT;
    }
    return syncer_->Close(isClosedOperation);
}

int SyncerProxy::Sync(const std::vector<std::string> &devices, int mode,
    const std::function<void(const std::map<std::string, int> &)> &onComplete,
    const std::function<void(void)> &onFinalize, bool wait)
{
    if (syncer_ == nullptr) {
        return -E_NOT_INIT;
    }
    return syncer_->Sync(devices, mode, onComplete, onFinalize, wait);
}

int SyncerProxy::Sync(const SyncParma &parma)
{
    if (syncer_ == nullptr) {
        return -E_NOT_INIT;
    }
    return syncer_->Sync(parma);
}

int SyncerProxy::RemoveSyncOperation(int syncId)
{
    if (syncer_ == nullptr) {
        return -E_NOT_INIT;
    }
    return syncer_->RemoveSyncOperation(syncId);
}

int SyncerProxy::StopSync()
{
    if (syncer_ == nullptr) {
        return -E_NOT_INIT;
    }
    return syncer_->StopSync();
}

uint64_t SyncerProxy::GetTimestamp()
{
    if (syncer_ == nullptr) {
        return SyncerFactory::GetSyncer(ISyncInterface::SYNC_SVD)->GetTimestamp();
    }
    return syncer_->GetTimestamp();
}

void SyncerProxy::EnableAutoSync(bool enable)
{
    if (syncer_ == nullptr) {
        return;
    }
    syncer_->EnableAutoSync(enable);
}

int SyncerProxy::EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash)
{
    return syncer_->EraseDeviceWaterMark(deviceId, isNeedHash, "");
}

int SyncerProxy::EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash,
    const std::string &tableName)
{
    if (syncer_ == nullptr) {
        LOGE("[SyncerProxy] Syncer no init, unknown rule to erase waterMark!");
        return -E_NOT_INIT;
    }
    return syncer_->EraseDeviceWaterMark(deviceId, isNeedHash, tableName);
}

void SyncerProxy::LocalDataChanged(int notifyEvent)
{
    if (syncer_ == nullptr) {
        return;
    }
    syncer_->LocalDataChanged(notifyEvent);
}

int SyncerProxy::GetQueuedSyncSize(int *queuedSyncSize) const
{
    if (syncer_ == nullptr) {
        return -E_NOT_INIT;
    }
    return syncer_->GetQueuedSyncSize(queuedSyncSize);
}

int SyncerProxy::SetQueuedSyncLimit(const int *queuedSyncLimit)
{
    if (syncer_ == nullptr) {
        return -E_NOT_INIT;
    }
    return syncer_->SetQueuedSyncLimit(queuedSyncLimit);
}

int SyncerProxy::GetQueuedSyncLimit(int *queuedSyncLimit) const
{
    if (syncer_ == nullptr) {
        return -E_NOT_INIT;
    }
    return syncer_->GetQueuedSyncLimit(queuedSyncLimit);
}

int SyncerProxy::DisableManualSync(void)
{
    if (syncer_ == nullptr) {
        return -E_NOT_INIT;
    }
    return syncer_->DisableManualSync();
}

int SyncerProxy::EnableManualSync(void)
{
    if (syncer_ == nullptr) {
        return -E_NOT_INIT;
    }
    return syncer_->EnableManualSync();
}

int SyncerProxy::GetLocalIdentity(std::string &outTarget) const
{
    if (syncer_ == nullptr) {
        return -E_NOT_INIT;
    }
    return syncer_->GetLocalIdentity(outTarget);
}

int SyncerProxy::SetStaleDataWipePolicy(WipePolicy policy)
{
    if (syncer_ == nullptr) {
        return -E_NOT_INIT;
    }
    return syncer_->SetStaleDataWipePolicy(policy);
}

int SyncerProxy::SetSyncRetry(bool isRetry)
{
    if (syncer_ == nullptr) {
        return -E_NOT_INIT;
    }
    return syncer_->SetSyncRetry(isRetry);
}

int SyncerProxy::SetEqualIdentifier(const std::string &identifier, const std::vector<std::string> &targets)
{
    if (syncer_ == nullptr) {
        return -E_NOT_INIT;
    }
    return syncer_->SetEqualIdentifier(identifier, targets);
}
} // namespace DistributedDB
