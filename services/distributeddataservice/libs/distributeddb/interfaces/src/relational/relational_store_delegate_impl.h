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
#ifndef RELATIONAL_STORE_DELEGATE_IMPL_H
#define RELATIONAL_STORE_DELEGATE_IMPL_H
#ifdef RELATIONAL_STORE

#include "macro_utils.h"
#include "relational_store_connection.h"

namespace DistributedDB {
class RelationalStoreDelegateImpl final : public RelationalStoreDelegate {
public:
    RelationalStoreDelegateImpl() = default;
    ~RelationalStoreDelegateImpl() override;

    RelationalStoreDelegateImpl(RelationalStoreConnection *conn, const std::string &path);

    DISABLE_COPY_ASSIGN_MOVE(RelationalStoreDelegateImpl);

    DBStatus Sync(const std::vector<std::string> &devices, SyncMode mode,
        const Query &query, const SyncStatusCallback &onComplete, bool wait) override;

    DBStatus RemoveDeviceData(const std::string &device) override;

    DBStatus CreateDistributedTable(const std::string &tableName) override;

    DBStatus RemoveDeviceData(const std::string &device, const std::string &tableName) override;

    // For connection
    DBStatus Close();

    void SetReleaseFlag(bool flag);

    DBStatus RemoteQuery(const std::string &device, const RemoteCondition &condition, uint64_t timeout,
        std::shared_ptr<ResultSet> &result) override;

private:
    static void OnSyncComplete(const std::map<std::string, std::vector<TableStatus>> &devicesStatus,
        SyncStatusCallback &onComplete);

    RelationalStoreConnection *conn_ = nullptr;
    std::string storePath_;
    std::atomic<bool> releaseFlag_ = false;
};
} // namespace DistributedDB
#endif
#endif // RELATIONAL_STORE_DELEGATE_IMPL_H