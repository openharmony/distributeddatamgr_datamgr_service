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
#include "res_finalizer.h"
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
        errCode = SQLiteUtils::SetPersistWalMode(db);
        if (errCode != E_OK) {
            break;
        }

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
    if (handle == nullptr) {
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

namespace {
int SaveSchemaToMetaTable(SQLiteSingleVerRelationalStorageExecutor *handle, const RelationalSchemaObject &schema)
{
    const Key schemaKey(RELATIONAL_SCHEMA_KEY, RELATIONAL_SCHEMA_KEY + strlen(RELATIONAL_SCHEMA_KEY));
    Value schemaVal;
    DBCommon::StringToVector(schema.ToSchemaString(), schemaVal);
    int errCode = handle->PutKvData(schemaKey, schemaVal); // save schema to meta_data
    if (errCode != E_OK) {
        LOGE("Save schema to meta table failed. %d", errCode);
    }
    return errCode;
}
}

int SQLiteSingleRelationalStorageEngine::CreateDistributedTable(const std::string &tableName, bool &schemaChanged)
{
    std::lock_guard lock(schemaMutex_);
    RelationalSchemaObject tmpSchema = schema_;
    bool isUpgrade = false;
    if (tmpSchema.GetTable(tableName).GetTableName() == tableName) {
        LOGW("distributed table was already created.");
        isUpgrade = true;
        int errCode = UpgradeDistributedTable(tableName);
        if (errCode != E_OK) {
            LOGE("Upgrade distributed table failed. %d", errCode);
            return errCode;
        }
    }

    if (tmpSchema.GetTables().size() >= DBConstant::MAX_DISTRIBUTED_TABLE_COUNT) {
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
    ResFinalizer finalizer([&handle, this] { this->ReleaseExecutor(handle); });

    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        return errCode;
    }

    TableInfo table;
    errCode = handle->CreateDistributedTable(tableName, table, isUpgrade);
    if (errCode != E_OK) {
        LOGE("create distributed table failed. %d", errCode);
        (void)handle->Rollback();
        return errCode;
    }

    tmpSchema.AddRelationalTable(table);
    errCode = SaveSchemaToMetaTable(handle, tmpSchema);
    if (errCode != E_OK) {
        LOGE("Save schema to meta table for create distributed table failed. %d", errCode);
        (void)handle->Rollback();
        return errCode;
    }

    errCode = handle->Commit();
    if (errCode == E_OK) {
        schema_ = tmpSchema;
        schemaChanged = true;
    }
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::UpgradeDistributedTable(const std::string &tableName)
{
    LOGD("Upgrade distributed table.");
    RelationalSchemaObject tmpSchema = schema_;
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

    TableInfo newTable;
    errCode = handle->UpgradeDistributedTable(tmpSchema.GetTable(tableName), newTable);
    if (errCode != E_OK) {
        LOGE("Upgrade distributed table failed. %d", errCode);
        (void)handle->Rollback();
        ReleaseExecutor(handle);
        return errCode;
    }

    tmpSchema.AddRelationalTable(newTable);
    errCode = SaveSchemaToMetaTable(handle, tmpSchema);
        if (errCode != E_OK) {
        LOGE("Save schema to meta table for upgrade distributed table failed. %d", errCode);
        (void)handle->Rollback();
        ReleaseExecutor(handle);
        return errCode;
    }

    errCode = handle->Commit();
    if (errCode == E_OK) {
        schema_ = tmpSchema;
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

    errCode = handle->CheckAndCleanDistributedTable(schema_.GetTableNames(), missingTables);
    if (errCode == E_OK) {
        errCode = handle->Commit();
        if (errCode == E_OK) {
            // Remove non-existent tables from the schema
            for (const auto &tableName : missingTables) {
                schema_.RemoveRelationalTable(tableName);
            }
            SaveSchemaToMetaTable(handle, schema_); // save schema to meta_data
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