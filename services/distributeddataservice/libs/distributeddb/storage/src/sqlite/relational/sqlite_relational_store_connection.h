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
#ifndef SQLITE_RELATIONAL_STORE_CONNECTION_H
#define SQLITE_RELATIONAL_STORE_CONNECTION_H
#ifdef RELATIONAL_STORE

#include <atomic>
#include <string>
#include "macro_utils.h"
#include "relational_store_connection.h"
#include "sqlite_single_ver_relational_storage_executor.h"
#include "sqlite_relational_store.h"

namespace DistributedDB {
class SQLiteRelationalStoreConnection : public RelationalStoreConnection {
public:
    explicit SQLiteRelationalStoreConnection(SQLiteRelationalStore *store);

    ~SQLiteRelationalStoreConnection() override = default;

    DISABLE_COPY_ASSIGN_MOVE(SQLiteRelationalStoreConnection);

    // Close and release the connection.
    int Close() override;
    int TriggerAutoSync() override;
    int SyncToDevice(SyncInfo &info) override;
    std::string GetIdentifier() override;
    int CreateDistributedTable(const std::string &tableName) override;
    int RegisterLifeCycleCallback(const DatabaseLifeCycleNotifier &notifier) override;

    int RemoveDeviceData(const std::string &device) override;
    int RemoveDeviceData(const std::string &device, const std::string &tableName) override;
    void RegisterObserverAction(const RelationalObserverAction &action) override;
    int RemoteQuery(const std::string &device, const RemoteCondition &condition, uint64_t timeout,
        std::shared_ptr<ResultSet> &result) override;

protected:

    int Pragma(int cmd, void *parameter) override;
private:

    SQLiteSingleVerRelationalStorageExecutor *GetExecutor(bool isWrite, int &errCode) const;
    void ReleaseExecutor(SQLiteSingleVerRelationalStorageExecutor *&executor) const;
    int StartTransaction();
    // Commit the transaction
    int Commit();

    // Roll back the transaction
    int RollBack();

    SQLiteSingleVerRelationalStorageExecutor *writeHandle_ = nullptr;
    mutable std::mutex transactionMutex_; // used for transaction
};
} // namespace DistributedDB
#endif
#endif // SQLITE_RELATIONAL_STORE_CONNECTION_H