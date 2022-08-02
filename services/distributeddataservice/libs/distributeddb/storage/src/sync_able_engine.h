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

#ifndef SYNC_ABLE_ENGINE_H
#define SYNC_ABLE_ENGINE_H

#include <memory.h>

#include "notification_chain.h"
#include "ref_object.h"
#include "syncer_proxy.h"

namespace DistributedDB {
class SyncAbleEngine final {
public:
    explicit SyncAbleEngine(ISyncInterface *store);
    ~SyncAbleEngine();
    void TriggerSync(int notifyEvent);

    // Start a sync action.
    int Sync(const ISyncer::SyncParma &parm, uint64_t connectionId);

    void WakeUpSyncer();
    void Close();

    // Enable auto sync
    void EnableAutoSync(bool enable);

    int EnableManualSync(void);
    int DisableManualSync(void);

    // Get The current virtual timestamp
    uint64_t GetTimestamp();

    int EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash, const std::string &tableName = "");

    int GetLocalIdentity(std::string &outTarget);

    // Stop a sync action in progress
    void StopSync(uint64_t connectionId);

    void Dump(int fd);

    int RemoteQuery(const std::string &device, const RemoteCondition &condition, uint64_t timeout,
        uint64_t connectionId, std::shared_ptr<ResultSet> &result);
private:
    // Start syncer
    int StartSyncer(bool isCheckSyncActive = false, bool isNeedActive = true);

    int StartSyncerWithNoLock(bool isCheckSyncActive, bool isNeedActive);

    // Stop syncer
    void StopSyncer(bool isClosedOperation = false);

    void StopSyncerWithNoLock(bool isClosedOperation = false);

    void SetSyncModuleActive();

    bool GetSyncModuleActive();

    void ReSetSyncModuleActive();

    void UserChangeHandle();

    void ChangeUserListerner();

    SyncerProxy syncer_; // use for sync Interactive
    std::atomic<bool> started_;
    std::atomic<bool> closed_;
    std::atomic<bool> isSyncModuleActiveCheck_;
    std::atomic<bool> isSyncNeedActive_;
    ISyncInterface *store_;

    mutable std::mutex syncerOperateLock_;
    NotificationChain::Listener *userChangeListerner_;
};
}  // namespace DistributedDB
#endif // SYNC_ABLE_ENGINE_H