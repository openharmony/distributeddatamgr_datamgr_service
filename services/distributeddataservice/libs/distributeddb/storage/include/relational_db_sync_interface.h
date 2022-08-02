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
#ifndef RELATIONAL_DB_SYNC_INTERFACE_H
#define RELATIONAL_DB_SYNC_INTERFACE_H
#ifdef RELATIONAL_STORE

#include "prepared_stmt.h"
#include "query_sync_object.h"
#include "relationaldb_properties.h"
#include "relational_row_data_set.h"
#include "relational_schema_object.h"
#include "relationaldb_properties.h"
#include "single_ver_kv_entry.h"
#include "sync_generic_interface.h"
#include "schema_negotiate.h"

namespace DistributedDB {
class RelationalDBSyncInterface : public SyncGenericInterface {
public:
    ~RelationalDBSyncInterface() override {};

    virtual RelationalSchemaObject GetSchemaInfo() const = 0;

    // Get batch meta data associated with the given key.
    virtual int GetBatchMetaData(const std::vector<Key> &keys, std::vector<Entry> &entries) const = 0;
    // Put batch meta data as a key-value entry vector
    virtual int PutBatchMetaData(std::vector<Entry> &entries) = 0;

    virtual std::vector<QuerySyncObject> GetTablesQuery() = 0;

    virtual int LocalDataChanged(int notifyEvent, std::vector<QuerySyncObject> &queryObj) = 0;

    virtual int CreateDistributedDeviceTable(const std::string &device, const RelationalSyncStrategy &syncStrategy) = 0;

    virtual int RegisterSchemaChangedCallback(const std::function<void()> &callback) = 0;

    using ISyncInterface::GetMaxTimestamp;
    virtual int GetMaxTimestamp(const std::string &tableName, Timestamp &timestamp) const = 0;

    // return OK, when query finished. token will be set NULL; when query unfinished. token must not be null.
    // return other errCode, some wrong, token will be set NULL.
    virtual int ExecuteQuery(const PreparedStmt &prepStmt, size_t packetSize, RelationalRowDataSet &data,
        ContinueToken &token) const = 0;

    virtual const RelationalDBProperties &GetRelationalDbProperties() const = 0;
};
}
#endif // RELATIONAL_STORE
#endif