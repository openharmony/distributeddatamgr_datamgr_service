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

#include "query.h"
#include "types.h"

namespace DistributedDB {
class RelationalStoreDelegate {
public:
    DB_API virtual ~RelationalStoreDelegate() = default;

    struct Option {
        bool createIfNecessary = true;
    };

    DB_API virtual DBStatus Pragma(PragmaCmd cmd, PragmaData &paramData) = 0;

    DB_API virtual DBStatus RemoveDeviceData(const std::string &device) = 0;

    struct TableOption {
    };

    DB_API virtual DBStatus CreateDistributedTable(const std::string &tableName, const TableOption &option) = 0;

    DB_API virtual DBStatus Sync(const std::vector<std::string> &devices, SyncMode mode,
        SyncStatusCallback &onComplete, bool wait) = 0;

    DB_API virtual DBStatus Sync(const std::vector<std::string> &devices, SyncMode mode,
        SyncStatusCallback &onComplete, const Query &query, bool wait) = 0;

    DB_API virtual DBStatus RemoveDevicesData(const std::string &tableName, const std::string &device) = 0;
};
} // namespace DistributedDB
#endif // RELATIONAL_STORE_DELEGATE_H