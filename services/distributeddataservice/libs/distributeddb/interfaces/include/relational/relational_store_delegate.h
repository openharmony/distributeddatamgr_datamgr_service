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

#ifndef RELATIONAL_STORE_DELEGATE_H
#define RELATIONAL_STORE_DELEGATE_H

#include <map>
#include <memory>
#include "distributeddb/result_set.h"
#include "query.h"
#include "store_types.h"
#include "store_observer.h"

namespace DistributedDB {
class RelationalStoreDelegate {
public:
    DB_API virtual ~RelationalStoreDelegate() = default;

    struct Option {
        StoreObserver *observer = nullptr;
        // communicator label use dualTuple hash or not;
        bool syncDualTupleMode = false;
    };

    DB_API virtual DBStatus CreateDistributedTable(const std::string &tableName) = 0;

    DB_API virtual DBStatus Sync(const std::vector<std::string> &devices, SyncMode mode,
        const Query &query, const SyncStatusCallback &onComplete, bool wait) = 0;

    DB_API virtual DBStatus RemoveDeviceData(const std::string &device) = 0;

    DB_API virtual DBStatus RemoveDeviceData(const std::string &device, const std::string &tableName) = 0;

    // timeout is in ms.
    DB_API virtual DBStatus RemoteQuery(const std::string &device, const RemoteCondition &condition,
        uint64_t timeout, std::shared_ptr<ResultSet> &result) = 0;
};
} // namespace DistributedDB
#endif // RELATIONAL_STORE_DELEGATE_H