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
#ifndef RELATIONAL_STORE_CONNECTION_H
#define RELATIONAL_STORE_CONNECTION_H
#ifdef RELATIONAL_STORE

#include <atomic>
#include <string>

#include "db_types.h"
#include "macro_utils.h"
#include "ref_object.h"
#include "relational_store_delegate.h"

namespace DistributedDB {
class IRelationalStore;

class RelationalStoreConnection : public virtual RefObject {
public:
    struct SyncInfo {
        const std::vector<std::string> &devices;
        SyncMode mode = SYNC_MODE_PUSH_PULL;
        const SyncStatusCallback &onComplete;
        const Query &query;
        bool wait = true;
    };

    RelationalStoreConnection();

    explicit RelationalStoreConnection(IRelationalStore *store);

    virtual ~RelationalStoreConnection() = default;

    DISABLE_COPY_ASSIGN_MOVE(RelationalStoreConnection);

    // Close and release the connection.
    virtual int Close() = 0;
    virtual int TriggerAutoSync() = 0;
    virtual int SyncToDevice(SyncInfo &info) = 0;
    virtual std::string GetIdentifier() = 0;
    virtual int CreateDistributedTable(const std::string &tableName) = 0;
    virtual int RegisterLifeCycleCallback(const DatabaseLifeCycleNotifier &notifier) = 0;

protected:
    // Get the stashed 'KvDB_ pointer' without ref.
    template<typename DerivedDBType>
    DerivedDBType *GetDB() const
    {
        return static_cast<DerivedDBType *>(store_);
    }

    virtual int Pragma(int cmd, void *parameter);
    IRelationalStore *store_ = nullptr;
    std::atomic<bool> isExclusive_;
};
} // namespace DistributedDB
#endif
#endif // RELATIONAL_STORE_CONNECTION_H