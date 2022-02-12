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
#include "single_ver_syncer.h"

#include "db_common.h"
#include "single_ver_sync_engine.h"

namespace DistributedDB {
void SingleVerSyncer::RemoteDataChanged(const std::string &device)
{
    LOGI("[SingleVerSyncer] device online dev %s", STR_MASK(device));
    // while remote db is online again, need to do abilitySync
    static_cast<SingleVerSyncEngine *>(syncEngine_)->SetIsNeedResetAbilitySync(device, true);
}

void SingleVerSyncer::RemoteDeviceOffline(const std::string &device)
{
    LOGI("[SingleVerRelationalSyncer] device offline dev %s", STR_MASK(device));
    std::string userId = syncInterface_->GetDbProperties().GetStringProp(KvDBProperties::USER_ID, "");
    std::string appId = syncInterface_->GetDbProperties().GetStringProp(KvDBProperties::APP_ID, "");
    std::string storeId = syncInterface_->GetDbProperties().GetStringProp(KvDBProperties::STORE_ID, "");
    RuntimeContext::GetInstance()->NotifyDatabaseStatusChange(userId, appId, storeId, device, false);
    RefObject::IncObjRef(syncEngine_);
    static_cast<SingleVerSyncEngine *>(syncEngine_)->OfflineHandleByDevice(device);
    RefObject::DecObjRef(syncEngine_);
}

int SingleVerSyncer::EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash,
    const std::string &tableName)
{
    if (metadata_ == nullptr) {
        return -E_NOT_INIT;
    }
    return metadata_->EraseDeviceWaterMark(deviceId, isNeedHash, tableName);
}

int SingleVerSyncer::SetStaleDataWipePolicy(WipePolicy policy)
{
    std::lock_guard<std::mutex> lock(syncerLock_);
    if (closing_) {
        LOGE("[Syncer] Syncer is closing, return!");
        return -E_BUSY;
    }
    if (syncEngine_ == nullptr) {
        return -E_NOT_INIT;
    }
    int errCode = E_OK;
    switch (policy) {
        case RETAIN_STALE_DATA:
            static_cast<SingleVerSyncEngine *>(syncEngine_)->EnableClearRemoteStaleData(false);
            break;
        case WIPE_STALE_DATA:
            static_cast<SingleVerSyncEngine *>(syncEngine_)->EnableClearRemoteStaleData(true);
            break;
        default:
            errCode = -E_NOT_SUPPORT;
            break;
    }
    return errCode;
}

ISyncEngine *SingleVerSyncer::CreateSyncEngine()
{
    return new (std::nothrow) SingleVerSyncEngine();
}
}
