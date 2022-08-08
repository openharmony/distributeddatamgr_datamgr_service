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
#ifndef RELATIONAL_STORE_MANAGER_H
#define RELATIONAL_STORE_MANAGER_H
#include <string>
#include <functional>
#include <mutex>

#include "auto_launch_export.h"
#include "relational_store_delegate.h"
#include "store_types.h"

namespace DistributedDB {
class RelationalStoreManager final {
public:
    // Only calculate the table name with device hash, no guarantee for the table exists
    DB_API static std::string GetDistributedTableName(const std::string &device, const std::string &tableName);

    DB_API RelationalStoreManager(const std::string &appId, const std::string &userId, int32_t instanceId = 0);
    DB_API ~RelationalStoreManager() = default;

    RelationalStoreManager(const RelationalStoreManager &) = delete;
    RelationalStoreManager(RelationalStoreManager &&) = delete;
    RelationalStoreManager &operator=(const RelationalStoreManager &) = delete;
    RelationalStoreManager &operator=(RelationalStoreManager &&) = delete;

    DB_API DBStatus OpenStore(const std::string &path, const std::string &storeId,
        const RelationalStoreDelegate::Option &option, RelationalStoreDelegate *&delegate);

    DB_API DBStatus CloseStore(RelationalStoreDelegate *store);

    DB_API static void SetAutoLaunchRequestCallback(const AutoLaunchRequestCallback &callback);

    DB_API static std::string GetRelationalStoreIdentifier(const std::string &userId, const std::string &appId,
        const std::string &storeId, bool syncDualTupleMode = false);

private:
    std::string appId_;
    std::string userId_;
    int32_t instanceId_;
};
} // namespace DistributedDB
#endif // RELATIONAL_STORE_MANAGER_H