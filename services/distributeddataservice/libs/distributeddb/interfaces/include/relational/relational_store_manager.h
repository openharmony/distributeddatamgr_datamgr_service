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
#ifdef RELATIONAL_STORE
#include <string>
#include <functional>
#include <mutex>

#include "auto_launch_export.h"
#include "relational_store_delegate.h"
#include "irelational_store.h"
#include "kvdb_properties.h"
#include "types.h"

namespace DistributedDB {
class RelationalStoreManager final {
public:
    DB_API RelationalStoreManager(const std::string &appId, const std::string &userId);
    DB_API ~RelationalStoreManager() = default;

    RelationalStoreManager(const RelationalStoreManager &) = delete;
    RelationalStoreManager(RelationalStoreManager &&) = delete;
    RelationalStoreManager &operator=(const RelationalStoreManager &) = delete;
    RelationalStoreManager &operator=(RelationalStoreManager &&) = delete;

    // PRAGMA journal_mode=WAL
    // PRAGMA synchronous=FULL
    // PRAGMA synchronous=NORMAL
    DB_API void OpenStore(const std::string &path, const RelationalStoreDelegate::Option &option,
        const std::function<void(DBStatus, RelationalStoreDelegate *)> &callback);

    DB_API DBStatus CloseStore(RelationalStoreDelegate *store);

    DB_API DBStatus DeleteStore(const std::string &path);

private:
    std::string appId_;
    std::string userId_;
};
} // namespace DistributedDB
#endif // RELATIONAL_STORE
#endif // RELATIONAL_STORE_MANAGER_H