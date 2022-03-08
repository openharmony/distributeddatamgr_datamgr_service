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

#ifndef I_SYNC_ENGINE_H
#define I_SYNC_ENGINE_H

#include <map>
#include <mutex>

#include "ikvdb_sync_interface.h"
#include "meta_data.h"
#include "ref_object.h"
#include "sync_operation.h"

namespace DistributedDB {
class ISyncEngine : public virtual RefObject {
public:
    // Do some init things
    virtual int Initialize(ISyncInterface *syncInterface, std::shared_ptr<Metadata> &metadata,
        const std::function<void(std::string)> &onRemoteDataChanged,
        const std::function<void(std::string)> &offlineChanged,
        const std::function<void(const InternalSyncParma &param)> &queryAutoSyncCallback) = 0;

    // Do some things, when db close.
    virtual int Close() = 0;

    // Alloc and Add sync SyncTarget
    // return E_OK if operator success.
    virtual int AddSyncOperation(SyncOperation *operation) = 0;

    // Clear the SyncTarget matched the syncId.
    virtual void RemoveSyncOperation(int syncId) = 0;

    // notify other devices data has changed
    virtual void BroadCastDataChanged() const = 0;

    // Get Online devices
    virtual void GetOnlineDevices(std::vector<std::string> &devices) const = 0;

    // Register the device connect callback, this function must be called after Engine initted
    virtual void RegConnectCallback() = 0;

    // Get the database identifier
    virtual std::string GetLabel() const = 0;

    // Set Manual Sync retry config
    virtual void SetSyncRetry(bool isRetry) = 0;

    // Set an equal identifier for this database, After this called, send msg to the target will use this identifier
    virtual int SetEqualIdentifier(const std::string &identifier, const std::vector<std::string> &targets) = 0;

    // Set record device equal identifier when called in import/rekey scene when restart syncer
    virtual void SetEqualIdentifier() = 0;

    virtual void SetEqualIdentifierMap(const std::string &identifier, const std::vector<std::string> &targets) = 0;

    // Add auto subscribe timer when start sync engine, used for auto subscribe failed subscribe task when db online
    virtual int StartAutoSubscribeTimer() = 0;

    // Stop auto subscribe timer when start sync engine
    virtual void StopAutoSubscribeTimer() = 0;

    // Check if number of subscriptions out of limit
    virtual int SubscribeLimitCheck(const std::vector<std::string> &devices, QuerySyncObject &query) const = 0;

    // Check if the Sync Engine is active, some times synchronization is not allowed
    virtual bool IsEngineActive() const = 0;

    virtual void SchemaChange() = 0;

protected:
    virtual ~ISyncEngine() {};
};
} // namespace DistributedDB

#endif // I_SYNC_ENGINE_H