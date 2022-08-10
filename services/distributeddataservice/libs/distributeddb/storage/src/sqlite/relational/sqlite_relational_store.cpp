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
#include "db_constant.h"
#include "db_dump_helper.h"
#include "db_errno.h"
#include "log_print.h"
#include "db_types.h"
#include "sqlite_log_table_manager.h"
#include "sqlite_relational_store_connection.h"
#include "storage_engine_manager.h"

namespace DistributedDB {
namespace {
    constexpr const char *DISTRIBUTED_TABLE_MODE = "distributed_table_mode";
}

SQLiteRelationalStore::~SQLiteRelationalStore()
{
    delete sqliteStorageEngine_;
    sqliteStorageEngine_ = nullptr;
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

static void InitDataBaseOption(const RelationalDBProperties &properties, OpenDbProperties &option)
{
    option.uri = properties.GetStringProp(DBProperties::DATA_DIR, "");
    option.createIfNecessary = properties.GetBoolProp(DBProperties::CREATE_IF_NECESSARY, false);
    if (properties.IsEncrypted()) {
        option.cipherType = properties.GetCipherType();
        option.passwd = properties.GetPasswd();
        option.iterTimes = properties.GetIterTimes();
    }
}

int SQLiteRelationalStore::InitStorageEngine(const RelationalDBProperties &properties)
{
    OpenDbProperties option;
    InitDataBaseOption(properties, option);
    std::string identifier = properties.GetStringProp(DBProperties::IDENTIFIER_DATA, "");

    StorageEngineAttr poolSize = {1, 1, 0, 16}; // at most 1 write 16 read.
    int errCode = sqliteStorageEngine_->InitSQLiteStorageEngine(poolSize, option, identifier);
    if (errCode != E_OK) {
        LOGE("Init the sqlite storage engine failed:%d", errCode);
    }
    return errCode;
}

void SQLiteRelationalStore::ReleaseResources()
{
    if (sqliteStorageEngine_ != nullptr) {
        sqliteStorageEngine_->ClearEnginePasswd();
        (void)StorageEngineManager::ReleaseStorageEngine(sqliteStorageEngine_);
    }
    RefObject::DecObjRef(storageEngine_);
}

int SQLiteRelationalStore::CheckDBMode()
{
    int errCode = E_OK;
    auto *handle = GetHandle(false, errCode);
    if (handle == nullptr) {
        return errCode;
    }
    errCode = handle->CheckDBModeForRelational();
    if (errCode != E_OK) {
        LOGE("check relational DB mode failed. %d", errCode);
    }

    ReleaseHandle(handle);
    return errCode;
}

int SQLiteRelationalStore::GetSchemaFromMeta(RelationalSchemaObject &schema)
{
    Key schemaKey;
    DBCommon::StringToVector(DBConstant::RELATIONAL_SCHEMA_KEY, schemaKey);
    Value schemaVal;
    int errCode = storageEngine_->GetMetaData(schemaKey, schemaVal);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("Get relational schema from meta table failed. %d", errCode);
        return errCode;
    } else if (errCode == -E_NOT_FOUND || schemaVal.empty()) {
        LOGW("No relational schema info was found.");
        return -E_NOT_FOUND;
    }

    std::string schemaStr;
    DBCommon::VectorToString(schemaVal, schemaStr);
    errCode = schema.ParseFromSchemaString(schemaStr);
    if (errCode != E_OK) {
        LOGE("Parse schema string from meta table failed.");
        return errCode;
    }

    sqliteStorageEngine_->SetSchema(schema);
    return E_OK;
}

int SQLiteRelationalStore::CheckTableModeFromMeta(DistributedTableMode mode, bool isUnSet)
{
    const Key modeKey(DISTRIBUTED_TABLE_MODE, DISTRIBUTED_TABLE_MODE + strlen(DISTRIBUTED_TABLE_MODE));
    Value modeVal;
    int errCode = storageEngine_->GetMetaData(modeKey, modeVal);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("Get distributed table mode from meta table failed. errCode=%d", errCode);
        return errCode;
    }

    DistributedTableMode orgMode = DistributedTableMode::SPLIT_BY_DEVICE;
    if (!modeVal.empty()) {
        std::string value(modeVal.begin(), modeVal.end());
        orgMode = static_cast<DistributedTableMode>(strtoll(value.c_str(), nullptr, 10)); // 10: decimal
    } else if (isUnSet) {
        return E_OK; // First set table mode.
    }

    if (orgMode != mode) {
        LOGE("Check distributed table mode mismatch, orgMode=%d, openMode=%d", orgMode, mode);
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

int SQLiteRelationalStore::CheckProperties(RelationalDBProperties properties)
{
    RelationalSchemaObject schema;
    int errCode = GetSchemaFromMeta(schema);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("Get relational schema from meta failed. errcode=%d", errCode);
        return errCode;
    }
    properties.SetSchema(schema);

    // Empty schema means no distributed table has been used, we may set DB to any table mode
    // If there is a schema but no table mode, it is the 'SPLIT_BY_DEVICE' mode of old version
    bool isSchemaEmpty = (errCode == -E_NOT_FOUND);
    auto mode = static_cast<DistributedTableMode>(properties.GetIntProp(RelationalDBProperties::DISTRIBUTED_TABLE_MODE,
        DistributedTableMode::SPLIT_BY_DEVICE));
    errCode = CheckTableModeFromMeta(mode, isSchemaEmpty);
    if (errCode != E_OK) {
        LOGE("Get distributed table mode from meta failed. errcode=%d", errCode);
        return errCode;
    }

    errCode = SaveTableModeToMeta(mode);
    if (errCode != E_OK) {
        LOGE("Save table mode to meta failed. errCode=%d", errCode);
        return errCode;
    }

    return E_OK;
}

int SQLiteRelationalStore::SaveSchemaToMeta()
{
    Key schemaKey;
    DBCommon::StringToVector(DBConstant::RELATIONAL_SCHEMA_KEY, schemaKey);
    Value schemaVal;
    DBCommon::StringToVector(sqliteStorageEngine_->GetSchemaRef().ToSchemaString(), schemaVal);
    int errCode = storageEngine_->PutMetaData(schemaKey, schemaVal);
    if (errCode != E_OK) {
        LOGE("Save relational schema to meta table failed. %d", errCode);
    }
    return errCode;
}

int SQLiteRelationalStore::SaveTableModeToMeta(DistributedTableMode mode)
{
    const Key modeKey(DISTRIBUTED_TABLE_MODE, DISTRIBUTED_TABLE_MODE + strlen(DISTRIBUTED_TABLE_MODE));
    Value modeVal;
    DBCommon::StringToVector(std::to_string(mode), modeVal);
    int errCode = storageEngine_->PutMetaData(modeKey, modeVal);
    if (errCode != E_OK) {
        LOGE("Save relational schema to meta table failed. %d", errCode);
    }
    return errCode;
}

int SQLiteRelationalStore::SaveLogTableVersionToMeta()
{
    LOGD("save log table version to meta table, key: %s, val: %s", DBConstant::LOG_TABLE_VERSION_KEY.c_str(),
        DBConstant::LOG_TABLE_VERSION_CURRENT.c_str());
    const Key logVersionKey(DBConstant::LOG_TABLE_VERSION_KEY.begin(), DBConstant::LOG_TABLE_VERSION_KEY.end());
    Value logVersionVal(DBConstant::LOG_TABLE_VERSION_CURRENT.begin(), DBConstant::LOG_TABLE_VERSION_CURRENT.end());
    int errCode = storageEngine_->PutMetaData(logVersionKey, logVersionVal);
    if (errCode != E_OK) {
        LOGE("save log table version to meta table failed. %d", errCode);
    }
    return errCode;
}

int SQLiteRelationalStore::CleanDistributedDeviceTable()
{
    std::vector<std::string> missingTables;
    int errCode = sqliteStorageEngine_->CleanDistributedDeviceTable(missingTables);
    if (errCode != E_OK) {
        LOGE("Clean distributed device table failed. %d", errCode);
    }
    for (const auto &deviceTableName : missingTables) {
        std::string deviceHash;
        std::string tableName;
        DBCommon::GetDeviceFromName(deviceTableName, deviceHash, tableName);
        syncAbleEngine_->EraseDeviceWaterMark(deviceHash, false, tableName);
        if (errCode != E_OK) {
            LOGE("Erase water mark failed:%d", errCode);
            return errCode;
        }
    }
    return errCode;
}

int SQLiteRelationalStore::Open(const RelationalDBProperties &properties)
{
    std::lock_guard<std::mutex> lock(initalMutex_);
    if (isInitialized_) {
        LOGD("[RelationalStore][Open] relational db was already initialized.");
        return E_OK;
    }

    sqliteStorageEngine_ = new (std::nothrow) SQLiteSingleRelationalStorageEngine(properties);
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

        storageEngine_ = new (std::nothrow) RelationalSyncAbleStorage(sqliteStorageEngine_);
        if (storageEngine_ == nullptr) {
            LOGE("[RelationalStore][Open] Create syncable storage failed");
            errCode = -E_OUT_OF_MEMORY;
            break;
        }

        syncAbleEngine_ = std::make_unique<SyncAbleEngine>(storageEngine_);

        errCode = CheckDBMode();
        if (errCode != E_OK) {
            break;
        }

        errCode = CheckProperties(properties);
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

        isInitialized_ = true;
        return E_OK;
    } while (false);

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

int SQLiteRelationalStore::Sync(const ISyncer::SyncParma &syncParam, uint64_t connectionId)
{
    return syncAbleEngine_->Sync(syncParam, connectionId);
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
    syncAbleEngine_->Close();

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
    syncAbleEngine_->WakeUpSyncer();
}

int SQLiteRelationalStore::CreateDistributedTable(const std::string &tableName)
{
    auto mode = static_cast<DistributedTableMode>(sqliteStorageEngine_->GetProperties().GetIntProp(
        RelationalDBProperties::DISTRIBUTED_TABLE_MODE, DistributedTableMode::SPLIT_BY_DEVICE));

    std::string localIdentity; // collaboration mode need local identify
    if (mode == DistributedTableMode::COLLABORATION) {
        int errCode = syncAbleEngine_->GetLocalIdentity(localIdentity);
        if (errCode != E_OK || localIdentity.empty()) {
            LOGD("Get local identity failed, can not create.");
            return -E_NOT_SUPPORT;
        }
    }

    bool schemaChanged = false;
    int errCode = sqliteStorageEngine_->CreateDistributedTable(tableName, DBCommon::TransferStringToHex(localIdentity),
        schemaChanged);
    if (errCode != E_OK) {
        LOGE("Create distributed table failed. %d", errCode);
    }
    if (schemaChanged) {
        LOGD("Notify schema changed.");
        storageEngine_->NotifySchemaChanged();
    }
    return errCode;
}

int SQLiteRelationalStore::RemoveDeviceData(const std::string &device, const std::string &tableName)
{
    auto mode = static_cast<DistributedTableMode>(sqliteStorageEngine_->GetProperties().GetIntProp(
        RelationalDBProperties::DISTRIBUTED_TABLE_MODE, DistributedTableMode::SPLIT_BY_DEVICE));
    if (mode == DistributedTableMode::COLLABORATION) {
        LOGE("Not support remove device data in collaboration mode.");
        return -E_NOT_SUPPORT;
    }

    std::map<std::string, TableInfo> tables = sqliteStorageEngine_->GetSchemaRef().GetTables();
    if (!tableName.empty() && tables.find(tableName) == tables.end()) {
        LOGW("Remove device data with table name which is not a distributed table or not exist.");
        return E_OK;
    }

    int errCode = E_OK;
    auto *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        ReleaseHandle(handle);
        return errCode;
    }

    errCode = handle->DeleteDistributedDeviceTable(device, tableName);
    if (errCode != E_OK) {
        LOGE("delete device data failed. %d", errCode);
        (void)handle->Rollback();
        ReleaseHandle(handle);
        return errCode;
    }
    errCode = handle->Commit();
    ReleaseHandle(handle);
    storageEngine_->NotifySchemaChanged();
    return (errCode != E_OK) ? errCode : syncAbleEngine_->EraseDeviceWaterMark(device, true, tableName);
}

void SQLiteRelationalStore::RegisterObserverAction(const RelationalObserverAction &action)
{
    storageEngine_->RegisterObserverAction(action);
}

int SQLiteRelationalStore::StopLifeCycleTimer()
{
    auto runtimeCxt = RuntimeContext::GetInstance();
    if (runtimeCxt == nullptr) {
        return -E_INVALID_ARGS;
    }
    if (lifeTimerId_ != 0) {
        TimerId timerId = lifeTimerId_;
        lifeTimerId_ = 0;
        runtimeCxt->RemoveTimer(timerId, false);
    }
    return E_OK;
}

int SQLiteRelationalStore::StartLifeCycleTimer(const DatabaseLifeCycleNotifier &notifier)
{
    auto runtimeCxt = RuntimeContext::GetInstance();
    if (runtimeCxt == nullptr) {
        return -E_INVALID_ARGS;
    }
    RefObject::IncObjRef(this);
    TimerId timerId = 0;
    int errCode = runtimeCxt->SetTimer(DBConstant::DEF_LIFE_CYCLE_TIME,
        [this](TimerId id) -> int {
            std::lock_guard<std::mutex> lock(lifeCycleMutex_);
            if (lifeCycleNotifier_) {
                // normal identifier mode
                auto identifier = sqliteStorageEngine_->GetIdentifier();
                auto userId = sqliteStorageEngine_->GetProperties().GetStringProp(DBProperties::USER_ID, "");
                lifeCycleNotifier_(identifier, userId);
            }
            return 0;
        },
        [this]() {
            int ret = RuntimeContext::GetInstance()->ScheduleTask([this]() {
                RefObject::DecObjRef(this);
            });
            if (ret != E_OK) {
                LOGE("SQLiteSingleVerNaturalStore timer finalizer ScheduleTask, errCode %d", ret);
            }
        },
        timerId);
    if (errCode != E_OK) {
        lifeTimerId_ = 0;
        LOGE("SetTimer failed:%d", errCode);
        RefObject::DecObjRef(this);
        return errCode;
    }

    lifeCycleNotifier_ = notifier;
    lifeTimerId_ = timerId;
    return E_OK;
}

int SQLiteRelationalStore::RegisterLifeCycleCallback(const DatabaseLifeCycleNotifier &notifier)
{
    int errCode;
    {
        std::lock_guard<std::mutex> lock(lifeCycleMutex_);
        if (lifeTimerId_ != 0) {
            errCode = StopLifeCycleTimer();
            if (errCode != E_OK) {
                LOGE("Stop the life cycle timer failed:%d", errCode);
                return errCode;
            }
        }

        if (!notifier) {
            return E_OK;
        }
        errCode = StartLifeCycleTimer(notifier);
        if (errCode != E_OK) {
            LOGE("Register life cycle timer failed:%d", errCode);
            return errCode;
        }
    }
    auto listener = std::bind(&SQLiteRelationalStore::HeartBeat, this);
    storageEngine_->RegisterHeartBeatListener(listener);
    return errCode;
}

void SQLiteRelationalStore::HeartBeat()
{
    std::lock_guard<std::mutex> lock(lifeCycleMutex_);
    int errCode = ResetLifeCycleTimer();
    if (errCode != E_OK) {
        LOGE("Heart beat for life cycle failed:%d", errCode);
    }
}

int SQLiteRelationalStore::ResetLifeCycleTimer()
{
    if (lifeTimerId_ == 0) {
        return E_OK;
    }
    auto lifeNotifier = lifeCycleNotifier_;
    lifeCycleNotifier_ = nullptr;
    int errCode = StopLifeCycleTimer();
    if (errCode != E_OK) {
        LOGE("[Reset timer]Stop the life cycle timer failed:%d", errCode);
    }
    return StartLifeCycleTimer(lifeNotifier);
}

std::string SQLiteRelationalStore::GetStorePath() const
{
    return sqliteStorageEngine_->GetProperties().GetStringProp(DBProperties::DATA_DIR, "");
}

RelationalDBProperties SQLiteRelationalStore::GetProperties() const
{
    return sqliteStorageEngine_->GetProperties();
}

void SQLiteRelationalStore::StopSync(uint64_t connectionId)
{
    return syncAbleEngine_->StopSync(connectionId);
}

void SQLiteRelationalStore::Dump(int fd)
{
    std::string userId = "";
    std::string appId = "";
    std::string storeId = "";
    std::string label = "";
    if (sqliteStorageEngine_ != nullptr) {
        userId = sqliteStorageEngine_->GetProperties().GetStringProp(DBProperties::USER_ID, "");
        appId = sqliteStorageEngine_->GetProperties().GetStringProp(DBProperties::APP_ID, "");
        storeId = sqliteStorageEngine_->GetProperties().GetStringProp(DBProperties::STORE_ID, "");
        label = sqliteStorageEngine_->GetProperties().GetStringProp(DBProperties::IDENTIFIER_DATA, "");
    }
    label = DBCommon::TransferStringToHex(label);
    DBDumpHelper::Dump(fd, "\tdb userId = %s, appId = %s, storeId = %s, label = %s\n",
        userId.c_str(), appId.c_str(), storeId.c_str(), label.c_str());
    if (syncAbleEngine_ != nullptr) {
        syncAbleEngine_->Dump(fd);
    }
}

int SQLiteRelationalStore::RemoteQuery(const std::string &device, const RemoteCondition &condition, uint64_t timeout,
    uint64_t connectionId, std::shared_ptr<ResultSet> &result)
{
    if (sqliteStorageEngine_ == nullptr) {
        return -E_INVALID_DB;
    }

    if (!sqliteStorageEngine_->GetSchemaRef().IsSchemaValid()) {
        LOGW("not a distributed relational store.");
        return -E_NOT_SUPPORT;
    }
    const auto &properties = sqliteStorageEngine_->GetProperties();
    int tableMode = properties.GetIntProp(RelationalDBProperties::DISTRIBUTED_TABLE_MODE,
        DistributedTableMode::SPLIT_BY_DEVICE);
    if (tableMode != DistributedTableMode::SPLIT_BY_DEVICE) {
        LOGW("only support split mode.");
        return -E_NOT_SUPPORT;
    }
    return syncAbleEngine_->RemoteQuery(device, condition, timeout, connectionId, result);
}
}
#endif