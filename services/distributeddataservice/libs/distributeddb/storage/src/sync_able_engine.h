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

#include "ref_object.h"
#include "syncer_proxy.h"

namespace DistributedDB {
class SyncAbleEngine final {
public:
    explicit SyncAbleEngine(ISyncInterface *store);
    ~SyncAbleEngine();
    void TriggerSync(int notifyEvent);

    // Start a sync action.
    int Sync(const ISyncer::SyncParma &parm);

    void WakeUpSyncer();
    void Close();

    // Enable auto sync
    void EnableAutoSync(bool enable);

    int EnableManualSync(void);
    int DisableManualSync(void);

    // Stop a sync action in progress.
    void StopSync(int syncId);

    // Get The current virtual timestamp
    uint64_t GetTimeStamp();

    int EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash);

    int GetLocalIdentity(std::string &outTarget);

private:
    // Start syncer
    void StartSyncer();

    // Stop syncer
    void StopSyncer();

    SyncerProxy syncer_; // use for sync Interactive
    std::atomic<bool> started_;
    ISyncInterface *store_;
};
}  // namespace DistributedDB
#endif // SYNC_ABLE_ENGINE_H