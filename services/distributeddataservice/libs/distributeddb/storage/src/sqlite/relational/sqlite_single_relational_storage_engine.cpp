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
#ifdef RELATIONAL_STORE
#include "sqlite_single_relational_storage_engine.h"

#include "db_common.h"
#include "db_errno.h"
#include "sqlite_single_ver_relational_storage_executor.h"


namespace DistributedDB {
namespace {
    constexpr const char *RELATIONAL_SCHEMA_KEY = "relational_schema";
}

SQLiteSingleRelationalStorageEngine::SQLiteSingleRelationalStorageEngine() {};

SQLiteSingleRelationalStorageEngine::~SQLiteSingleRelationalStorageEngine() {};

StorageExecutor *SQLiteSingleRelationalStorageEngine::NewSQLiteStorageExecutor(sqlite3 *dbHandle, bool isWrite,
    bool isMemDb)
{
    return new (std::nothrow) SQLiteSingleVerRelationalStorageExecutor(dbHandle, isWrite);
}

int SQLiteSingleRelationalStorageEngine::Upgrade(sqlite3 *db)
{
    return SQLiteUtils::CreateRelationalMetaTable(db);
}

int SQLiteSingleRelationalStorageEngine::RegisterFunction(sqlite3 *db) const
{
    int errCode = SQLiteUtils::RegisterCalcHash(db);
    if (errCode != E_OK) {
        LOGE("[engine] register calculate hash failed!");
        return errCode;
    }

    errCode = SQLiteUtils::RegisterGetSysTime(db);
    if (errCode != E_OK) {
        LOGE("[engine] register get sys time failed!");
    }
    return E_OK;
}

int SQLiteSingleRelationalStorageEngine::CreateNewExecutor(bool isWrite, StorageExecutor *&handle)
{
    sqlite3 *db = nullptr;
    int errCode = SQLiteUtils::OpenDatabase(option_, db, false);
    if (errCode != E_OK) {
        return errCode;
    }
    do {
        errCode = Upgrade(db); // cerate meta_data table.
        if (errCode != E_OK) {
            break;
        }

        errCode = RegisterFunction(db);
        if (errCode != E_OK) {
            break;
        }

        handle = NewSQLiteStorageExecutor(db, isWrite, false);
        if (handle == nullptr) {
            LOGE("[Relational] New SQLiteStorageExecutor[%d] for the pool failed.", isWrite);
            errCode = -E_OUT_OF_MEMORY;
            break;
        }
        return E_OK;
    } while (false);

    (void)sqlite3_close_v2(db);
    db = nullptr;
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::ReleaseExecutor(SQLiteSingleVerRelationalStorageExecutor *&handle)
{
    if (handle == nullptr) { // TODO: check corrupted
        return E_OK;
    }
    StorageExecutor *databaseHandle = handle;
    Recycle(databaseHandle);
    handle = nullptr;
    return E_OK;
}

void SQLiteSingleRelationalStorageEngine::SetSchema(const RelationalSchemaObject &schema)
{
    std::lock_guard lock(schemaMutex_);
    schema_ = schema;
}

const RelationalSchemaObject &SQLiteSingleRelationalStorageEngine::GetSchemaRef() const
{
    std::lock_guard lock(schemaMutex_);
    return schema_;
}

int SQLiteSingleRelationalStorageEngine::CreateDistributedTable(const std::string &tableName)
{
    std::lock_guard lock(schemaMutex_);
    if (schema_.GetTable(tableName).GetTableName() == tableName) {
        LOGW("distributed table was already created.");
        // TODO: compare new schema
        return E_OK;
    }

    if (schema_.GetTables().size() >= DBConstant::MAX_DISTRIBUTED_TABLE_COUNT) {
        LOGE("The number of distributed tables is exceeds limit.");
        return -E_MAX_LIMITS;
    }

    LOGD("Create distributed table.");
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true, OperatePerm::NORMAL_PERM,
        errCode));
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        ReleaseExecutor(handle);
        return errCode;
    }

    TableInfo table;
    errCode = handle->CreateDistributedTable(tableName, table);
    if (errCode != E_OK) {
        LOGE("create distributed table failed. %d", errCode);
        (void)handle->Rollback();
        ReleaseExecutor(handle);
        return errCode;
    }

    errCode = handle->Commit();
    if (errCode == E_OK) {
        schema_.AddRelationalTable(table);
        const Key schemaKey(RELATIONAL_SCHEMA_KEY, RELATIONAL_SCHEMA_KEY + strlen(RELATIONAL_SCHEMA_KEY));
        Value schemaVal;
        DBCommon::StringToVector(schema_.ToSchemaString(), schemaVal);
        errCode = handle->PutKvData(schemaKey, schemaVal); // save schema to meta_data
    }
    ReleaseExecutor(handle);
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::CleanDistributedDeviceTable(std::vector<std::string> &missingTables)
{
    int errCode = E_OK;
    auto handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true, OperatePerm::NORMAL_PERM,
        errCode));
    if (handle == nullptr) {
        return errCode;
    }

    std::lock_guard lock(schemaMutex_);
    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        ReleaseExecutor(handle);
        return errCode;
    }

    errCode = handle->CkeckAndCleanDistributedTable(schema_.GetTableNames(), missingTables);
    if (errCode == E_OK) {
        errCode = handle->Commit();
        if (errCode == E_OK) {
            // Remove non-existent tables from the schema
            for (const auto &tableName : missingTables) {
                schema_.RemoveRelationalTable(tableName);
            }
        }
    } else {
        LOGE("Check distributed table failed. %d", errCode);
        (void)handle->Rollback();
    }

    ReleaseExecutor(handle);
    return errCode;
}
}
#endif