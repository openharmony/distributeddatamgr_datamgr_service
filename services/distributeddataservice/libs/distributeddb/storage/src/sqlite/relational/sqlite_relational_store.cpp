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
#include "sqlite_relational_store.h"

#include "db_common.h"
#include "db_errno.h"
#include "log_print.h"
#include "db_types.h"
#include "sqlite_relational_store_connection.h"
#include "storage_engine_manager.h"

namespace DistributedDB {
namespace {
    constexpr const char *RELATIONAL_SCHEMA_KEY = "relational_schema";
    constexpr const char *LOG_TABLE_VERSION_KEY = "log_table_versoin";
    constexpr const char *LOG_TABLE_VERSION_1 = "1.0";
}

SQLiteRelationalStore::~SQLiteRelationalStore()
{
    delete sqliteStorageEngine_;
}

// Called when a new connection created.
void SQLiteRelationalStore::IncreaseConnectionCounter()
{
    connectionCount_.fetch_add(1, std::memory_order_seq_cst);
    if (connectionCount_.load() > 0) {
        sqliteStorageEngine_->SetConnectionFlag(true);
    }
}

RelationalStoreConnection *SQLiteRelationalStore::GetDBConnection(int &errCode)
{
    std::lock_guard<std::mutex> lock(connectMutex_);
    RelationalStoreConnection* connection = new (std::nothrow) SQLiteRelationalStoreConnection(this);
    if (connection == nullptr) {
        errCode = -E_OUT_OF_MEMORY;
        return nullptr;
    }
    IncObjRef(this);
    IncreaseConnectionCounter();
    return connection;
}

static void InitDataBaseOption(const RelationalDBProperties &kvDBProp, OpenDbProperties &option)
{
    option.uri = kvDBProp.GetStringProp(KvDBProperties::DATA_DIR, "");
    option.createIfNecessary = kvDBProp.GetBoolProp(KvDBProperties::CREATE_IF_NECESSARY, false);
}

int SQLiteRelationalStore::InitStorageEngine(const RelationalDBProperties &kvDBProp)
{
    OpenDbProperties option;
    InitDataBaseOption(kvDBProp, option);

    StorageEngineAttr poolSize = {1, 1, 0, 16}; // at most 1 write 16 read.
    int errCode = sqliteStorageEngine_->InitSQLiteStorageEngine(poolSize, option);
    if (errCode != E_OK) {
        LOGE("Init the sqlite storage engine failed:%d", errCode);
    }
    return errCode;
}

void SQLiteRelationalStore::ReleaseResources()
{
    // TODO: Lock
    if (sqliteStorageEngine_ != nullptr) {
        sqliteStorageEngine_->ClearEnginePasswd();
        (void)StorageEngineManager::ReleaseStorageEngine(sqliteStorageEngine_);
    }
}

int SQLiteRelationalStore::GetSchemaFromMeta()
{
    const Key schemaKey(RELATIONAL_SCHEMA_KEY, RELATIONAL_SCHEMA_KEY + strlen(RELATIONAL_SCHEMA_KEY));
    Value schemaVal;
    int errCode = storageEngine_->GetMetaData(schemaKey, schemaVal);
    if (errCode != E_OK && errCode != -E_NOT_FOUND ) {
        LOGE("Get relationale schema from meta table failed. %d", errCode);
        return errCode;
    } else if (errCode == -E_NOT_FOUND) {
        LOGW("No relational schema info was found.");
        return E_OK;
    }

    std::string schemaStr;
    DBCommon::VectorToString(schemaVal, schemaStr);
    RelationalSchemaObject schema;
    errCode = schema.ParseFromSchemaString(schemaStr);
    if (errCode != E_OK) {
        LOGE("Parse schema string from mata table failed.");
        return errCode;
    }

    properties_.SetSchema(schema);
    return E_OK;
}

int SQLiteRelationalStore::SaveLogTableVersionToMeta()
{
    // TODO: save log table version into meta data
    LOGD("save log table version to meta table, key: %s, val: %s", LOG_TABLE_VERSION_KEY, LOG_TABLE_VERSION_1);
    return E_OK;
}

int SQLiteRelationalStore::CleanDistributedDeviceTable()
{
    // TODO: clean the device table which is no longer in schema
    RelationalSchemaObject schema = properties_.GetSchema();
    for (const auto &table : schema.GetTables()) {
        std::string tableName = table.first;
        LOGD("Get schema %s.", tableName.c_str());
    }

    int errCode = E_OK;
    auto *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }
    // TODO: Get device table names, and clean
    ReleaseHandle(handle);
    return E_OK;
}

int SQLiteRelationalStore::Open(const RelationalDBProperties &properties)
{
    sqliteStorageEngine_ = new (std::nothrow) SQLiteSingleRelationalStorageEngine();
    if (sqliteStorageEngine_ == nullptr) {
        LOGE("[RelationalStore][Open] Create storage engine failed");
        return -E_OUT_OF_MEMORY;
    }

    int errCode = E_OK;

    do {
        errCode = InitStorageEngine(properties);
        if (errCode != E_OK) {
            LOGE("[RelationalStore][Open] Init database context fail! errCode = [%d]", errCode);
            break;
        }
        storageEngine_ = new(std::nothrow) RelationalSyncAbleStorage(sqliteStorageEngine_);
        if (storageEngine_ == nullptr) {
            LOGE("[RelationalStore][Open] Create syncable storage failed"); // TODO:
            errCode = -E_OUT_OF_MEMORY;
            break;
        }

        properties_ = properties;
        errCode = GetSchemaFromMeta();
        if (errCode != E_OK) {
            break;
        }

        errCode = SaveLogTableVersionToMeta();
        if (errCode != E_OK) {
            break;
        }

        errCode = CleanDistributedDeviceTable();
        if (errCode != E_OK) {
            break;
        }

        syncEngine_ = std::make_shared<SyncAbleEngine>(storageEngine_);
        return E_OK;
    } while (false);

    // TODO: release resources.
    ReleaseResources();
    return errCode;
}

void SQLiteRelationalStore::OnClose(const std::function<void(void)> &notifier)
{
    AutoLock lockGuard(this);
    if (notifier) {
        closeNotifiers_.push_back(notifier);
    } else {
        LOGW("Register 'Close()' notifier failed, notifier is null.");
    }
}

SQLiteSingleVerRelationalStorageExecutor *SQLiteRelationalStore::GetHandle(bool isWrite, int &errCode) const
{
    if (sqliteStorageEngine_ == nullptr) {
        errCode = -E_INVALID_DB;
        return nullptr;
    }

    return static_cast<SQLiteSingleVerRelationalStorageExecutor *>(sqliteStorageEngine_->FindExecutor(isWrite,
        OperatePerm::NORMAL_PERM, errCode));
}
void SQLiteRelationalStore::ReleaseHandle(SQLiteSingleVerRelationalStorageExecutor *&handle) const
{
    if (handle == nullptr) {
        return;
    }

    if (sqliteStorageEngine_ != nullptr) {
        StorageExecutor *databaseHandle = handle;
        sqliteStorageEngine_->Recycle(databaseHandle);
        handle = nullptr;
    }
}

int SQLiteRelationalStore::Sync(const ISyncer::SyncParma &syncParam)
{
    return syncEngine_->Sync(syncParam);
}

// Called when a connection released.
void SQLiteRelationalStore::DecreaseConnectionCounter()
{
    int count = connectionCount_.fetch_sub(1, std::memory_order_seq_cst);
    if (count <= 0) {
        LOGF("Decrease db connection counter failed, count <= 0.");
        return;
    }
    if (count != 1) {
        return;
    }

    LockObj();
    auto notifiers = std::move(closeNotifiers_);
    UnlockObj();

    for (auto &notifier : notifiers) {
        if (notifier) {
            notifier();
        }
    }

    // Sync Close
    syncEngine_->Close();

    if (sqliteStorageEngine_ != nullptr) {
        delete sqliteStorageEngine_;
        sqliteStorageEngine_ = nullptr;
    }
    // close will dec sync ref of storageEngine_
    DecObjRef(storageEngine_);
}

void SQLiteRelationalStore::ReleaseDBConnection(RelationalStoreConnection *connection)
{
    if (connectionCount_.load() == 1) {
        sqliteStorageEngine_->SetConnectionFlag(false);
    }

    connectMutex_.lock();
    if (connection != nullptr) {
        KillAndDecObjRef(connection);
        DecreaseConnectionCounter();
        connectMutex_.unlock();
        KillAndDecObjRef(this);
    } else {
        connectMutex_.unlock();
    }
}

void SQLiteRelationalStore::WakeUpSyncer()
{
    syncEngine_->WakeUpSyncer();
}
}
#endif