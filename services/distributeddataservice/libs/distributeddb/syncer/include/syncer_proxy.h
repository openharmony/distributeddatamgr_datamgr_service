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

#ifndef SYNCER_PROXY_H
#define SYNCER_PROXY_H

#include <functional>
#include <mutex>
#include <map>
#include <memory>

#include "isyncer.h"

namespace DistributedDB {
class SyncerProxy : public ISyncer {
public:
    SyncerProxy();
    ~SyncerProxy() {};

    // Init the Syncer modules
    int Initialize(ISyncInterface *syncInterface, bool isNeedActive) override;

    // Close the syncer
    int Close(bool isClosedOperation) override;

    // Sync function.
    // param devices: The device id list.
    // param mode: Sync mode, see SyncMode.
    // param onComplete: The syncer finish callback. set by caller
    // param onFinalize: will be callback when this Sync Operation finalized.
    // return a Sync id. It will return a positive value if failed,
    int Sync(const std::vector<std::string> &devices, int mode,
        const std::function<void(const std::map<std::string, int> &)> &onComplete,
        const std::function<void(void)> &onFinalize, bool wait) override;

    // Sync function. use SyncParma to reduce parameter.
    int Sync(const SyncParma &param, uint64_t connectionId) override;

    // Remove the operation, with the given syncId, used to clean resource if sync finished or failed.
    int RemoveSyncOperation(int syncId) override;

    int StopSync(uint64_t connectionId) override;

    // Get The current virtual timestamp
    uint64_t GetTimestamp() override;

    // Enable auto sync function
    void EnableAutoSync(bool enable) override;

    // delete specified device's watermark
    int EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash) override;

    // delete specified device's and table's watermark
    int EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash,
        const std::string &tableName) override;

    // Local data changed callback
    void LocalDataChanged(int notifyEvent) override;

    // Get manual sync queue size
    int GetQueuedSyncSize(int *queuedSyncSize) const override;

    // Set manual sync queue limit
    int SetQueuedSyncLimit(const int *queuedSyncLimit) override;

    // Get manual sync queue limit
    int GetQueuedSyncLimit(int *queuedSyncLimit) const override;

    // Disable add new manual sync, for rekey
    int DisableManualSync(void) override;

    // Enable add new manual sync, for rekey
    int EnableManualSync(void) override;

    // Get local deviceId, is hashed
    int GetLocalIdentity(std::string &outTarget) const override;

    // Set stale data wipe policy
    int SetStaleDataWipePolicy(WipePolicy policy) override;

    // Set Manual Sync retry config
    int SetSyncRetry(bool isRetry) override;

    // Set an equal identifier for this database, After this called, send msg to the target will use this identifier
    int SetEqualIdentifier(const std::string &identifier, const std::vector<std::string> &targets) override;

    // Dump syncer info
    void Dump(int fd) override;

    // Dump syncer basic info
    SyncerBasicInfo DumpSyncerBasicInfo() override;

    int RemoteQuery(const std::string &device, const RemoteCondition &condition,
        uint64_t timeout, uint64_t connectionId, std::shared_ptr<ResultSet> &result) override;

private:
    std::mutex syncerLock_;
    std::shared_ptr<ISyncer> syncer_;
};
} // namespace DistributedDB

#endif  // SYNCER_PROXY_H
