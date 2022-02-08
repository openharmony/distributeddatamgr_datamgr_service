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
#ifndef SQLITE_RELATIONAL_ENGINE_H
#define SQLITE_RELATIONAL_ENGINE_H
#ifdef RELATIONAL_STORE

#include "macro_utils.h"
#include "sqlite_storage_engine.h"
#include "sqlite_single_ver_relational_storage_executor.h"

namespace DistributedDB {
class SQLiteSingleRelationalStorageEngine : public SQLiteStorageEngine {
public:
    SQLiteSingleRelationalStorageEngine();
    ~SQLiteSingleRelationalStorageEngine() override;

    // Delete the copy and assign constructors
    DISABLE_COPY_ASSIGN_MOVE(SQLiteSingleRelationalStorageEngine);

    void SetSchema(const RelationalSchemaObject &schema);

    const RelationalSchemaObject &GetSchemaRef() const;

    int CreateDistributedTable(const std::string &tableName);

    int CleanDistributedDeviceTable(std::vector<std::string> &missingTables);

protected:
    StorageExecutor *NewSQLiteStorageExecutor(sqlite3 *db, bool isWrite, bool isMemDb) override;
    int Upgrade(sqlite3 *db) override;
    int CreateNewExecutor(bool isWrite, StorageExecutor *&handle) override;

private:
    // For executor.
    int ReleaseExecutor(SQLiteSingleVerRelationalStorageExecutor *&handle);

    int UpgradeDistributedTable(const std::string &tableName);

    RelationalSchemaObject schema_;
    mutable std::mutex schemaMutex_;
};
} // namespace DistributedDB
#endif
#endif // SQLITE_RELATIONAL_ENGINE_H