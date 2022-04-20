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

#include "sqlite_single_ver_natural_store.h"

#include <algorithm>
#include <thread>
#include <chrono>

#include "data_compression.h"
#include "db_common.h"
#include "db_constant.h"
#include "db_errno.h"
#include "generic_single_ver_kv_entry.h"
#include "intercepted_data_impl.h"
#include "kvdb_utils.h"
#include "log_print.h"
#include "platform_specific.h"
#include "schema_object.h"
#include "single_ver_database_oper.h"
#include "storage_engine_manager.h"
#include "sqlite_single_ver_natural_store_connection.h"
#include "value_hash_calc.h"

namespace DistributedDB {
namespace {
    constexpr int WAIT_DELEGATE_CALLBACK_TIME = 100;

    const std::string CREATE_DB_TIME = "createDBTime";

    // Called when get multiple dev data.
    // deviceID is the device which currently being getting. When getting one dev data, deviceID is "".
    // dataItems is the DataItems which already be get from DB sorted by timestamp.
    // token must not be null.
    void ProcessContinueToken(const DeviceID &deviceID, const std::vector<DataItem> &dataItems, int &errCode,
        SQLiteSingleVerContinueToken *&token)
    {
        if (errCode != -E_UNFINISHED) { // Error happened or get data finished. Token should be cleared.
            delete token;
            token = nullptr;
            return;
        }

        if (dataItems.empty()) {
            errCode = -E_INTERNAL_ERROR;
            LOGE("Get data unfinished but dataitems is empty.");
            delete token;
            token = nullptr;
            return;
        }

        Timestamp nextBeginTime = dataItems.back().timestamp + 1;
        if (nextBeginTime > INT64_MAX) {
            nextBeginTime = INT64_MAX;
        }
        token->SetNextBeginTime(deviceID, nextBeginTime);
        return;
    }

    // Called when get one dev data.
    void ProcessContinueToken(const std::vector<DataItem> &dataItems, int &errCode,
        SQLiteSingleVerContinueToken *&token)
    {
        ProcessContinueToken("", dataItems, errCode, token);
    }

    // Called when get query sync data.
    // dataItems is the DataItems which already be get from DB sorted by timestamp.
    // token must not be null.
    void ProcessContinueTokenForQuerySync(const std::vector<DataItem> &dataItems, int &errCode,
        SQLiteSingleVerContinueToken *&token)
    {
        if (errCode != -E_UNFINISHED) { // Error happened or get data finished. Token should be cleared.
            delete token;
            token = nullptr;
            return;
        }

        if (dataItems.empty()) {
            errCode = -E_INTERNAL_ERROR;
            LOGE("Get data unfinished but dataitems is empty.");
            delete token;
            token = nullptr;
            return;
        }

        Timestamp nextBeginTime = dataItems.back().timestamp + 1;
        if (nextBeginTime > INT64_MAX) {
            nextBeginTime = INT64_MAX;
        }
        bool getDeleteData = ((dataItems.back().flag & DataItem::DELETE_FLAG) != 0);
        if (getDeleteData) {
            token->FinishGetQueryData();
            token->SetDeletedNextBeginTime("", nextBeginTime);
        } else {
            token->SetNextBeginTime("", nextBeginTime);
        }
        return;
    }

    void UpdateSecProperties(KvDBProperties &properties, bool isReadOnly, const SchemaObject &savedSchemaObj,
        const SQLiteSingleVerStorageEngine *engine)
    {
        if (isReadOnly) {
            properties.SetSchema(savedSchemaObj);
            properties.SetBoolProp(KvDBProperties::FIRST_OPEN_IS_READ_ONLY, true);
        }
        // Update the security option from the storage engine for that
        // we will not update the security label and flag for the existed database.
        // So the security label and flag are from the existed database.
        if (engine == nullptr) {
            return;
        }
        properties.SetIntProp(KvDBProperties::SECURITY_LABEL, engine->GetSecurityOption().securityLabel);
        properties.SetIntProp(KvDBProperties::SECURITY_FLAG, engine->GetSecurityOption().securityFlag);
    }

    int GetKvEntriesByDataItems(std::vector<SingleVerKvEntry *> &entries, std::vector<DataItem> &dataItems)
    {
        int errCode = E_OK;
        for (auto &item : dataItems) {
            auto entry = new (std::nothrow) GenericSingleVerKvEntry();
            if (entry == nullptr) {
                errCode = -E_OUT_OF_MEMORY;
                LOGE("GetKvEntries failed, errCode:%d", errCode);
                SingleVerKvEntry::Release(entries);
                break;
            }
            entry->SetEntryData(std::move(item));
            entries.push_back(entry);
        }
        return errCode;
    }

    bool CanHoldDeletedData(const std::vector<DataItem> &dataItems, const DataSizeSpecInfo &dataSizeInfo,
        size_t appendLen)
    {
        bool reachThreshold = false;
        size_t blockSize = 0;
        for (size_t i = 0; !reachThreshold && i < dataItems.size(); i++) {
            blockSize += SQLiteSingleVerStorageExecutor::GetDataItemSerialSize(dataItems[i], appendLen);
            reachThreshold = (blockSize >= dataSizeInfo.blockSize * DBConstant::QUERY_SYNC_THRESHOLD);
        }
        return !reachThreshold;
    }
}

SQLiteSingleVerNaturalStore::SQLiteSingleVerNaturalStore()
    : currentMaxTimestamp_(0),
      storageEngine_(nullptr),
      notificationEventsRegistered_(false),
      notificationConflictEventsRegistered_(false),
      isInitialized_(false),
      isReadOnly_(false),
      lifeCycleNotifier_(nullptr),
      lifeTimerId_(0),
      autoLifeTime_(DBConstant::DEF_LIFE_CYCLE_TIME),
      createDBTime_(0),
      dataInterceptor_(nullptr),
      maxLogSize_(DBConstant::MAX_LOG_SIZE_DEFAULT)
{}

SQLiteSingleVerNaturalStore::~SQLiteSingleVerNaturalStore()
{
    ReleaseResources();
}

std::string SQLiteSingleVerNaturalStore::GetDatabasePath(const KvDBProperties &kvDBProp)
{
    std::string filePath = GetSubDirPath(kvDBProp) + "/" +
        DBConstant::MAINDB_DIR + "/" + DBConstant::SINGLE_VER_DATA_STORE + DBConstant::SQLITE_DB_EXTENSION;
    return filePath;
}

std::string SQLiteSingleVerNaturalStore::GetSubDirPath(const KvDBProperties &kvDBProp)
{
    std::string dataDir = kvDBProp.GetStringProp(KvDBProperties::DATA_DIR, "");
    std::string identifierDir = kvDBProp.GetStringProp(KvDBProperties::IDENTIFIER_DIR, "");
    std::string dirPath = dataDir + "/" + identifierDir + "/" + DBConstant::SINGLE_SUB_DIR;
    return dirPath;
}

int SQLiteSingleVerNaturalStore::SetUserVer(const KvDBProperties &kvDBProp, int version)
{
    OpenDbProperties properties;
    properties.uri = GetDatabasePath(kvDBProp);
    bool isEncryptedDb = kvDBProp.GetBoolProp(KvDBProperties::ENCRYPTED_MODE, false);
    if (isEncryptedDb) {
        kvDBProp.GetPassword(properties.cipherType, properties.passwd);
    }

    int errCode = SQLiteUtils::SetUserVer(properties, version);
    if (errCode != E_OK) {
        LOGE("Recover for open db failed in single version:%d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerNaturalStore::InitDatabaseContext(const KvDBProperties &kvDBProp, bool isNeedUpdateSecOpt)
{
    int errCode = InitStorageEngine(kvDBProp, isNeedUpdateSecOpt);
    if (errCode != E_OK) {
        return errCode;
    }
    InitCurrentMaxStamp();
    return errCode;
}

int SQLiteSingleVerNaturalStore::RegisterLifeCycleCallback(const DatabaseLifeCycleNotifier &notifier)
{
    std::lock_guard<std::mutex> lock(lifeCycleMutex_);
    int errCode;
    if (!notifier) {
        if (lifeTimerId_ == 0) {
            return E_OK;
        }
        errCode = StopLifeCycleTimer();
        if (errCode != E_OK) {
            LOGE("Stop the life cycle timer failed:%d", errCode);
        }
        return E_OK;
    }

    if (lifeTimerId_ != 0) {
        errCode = StopLifeCycleTimer();
        if (errCode != E_OK) {
            LOGE("Stop the life cycle timer failed:%d", errCode);
        }
    }
    errCode = StartLifeCycleTimer(notifier);
    if (errCode != E_OK) {
        LOGE("Register life cycle timer failed:%d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerNaturalStore::SetAutoLifeCycleTime(uint32_t time)
{
    std::lock_guard<std::mutex> lock(lifeCycleMutex_);
    if (lifeTimerId_ == 0) {
        autoLifeTime_ = time;
    } else {
        auto runtimeCxt = RuntimeContext::GetInstance();
        if (runtimeCxt == nullptr) {
            return -E_INVALID_ARGS;
        }
        LOGI("[SingleVer] Set life cycle to %u", time);
        int errCode = runtimeCxt->ModifyTimer(lifeTimerId_, time);
        if (errCode != E_OK) {
            return errCode;
        }
        autoLifeTime_ = time;
    }
    return E_OK;
}

int SQLiteSingleVerNaturalStore::GetSecurityOption(SecurityOption &option) const
{
    bool isMemDb = GetDbProperties().GetBoolProp(KvDBProperties::MEMORY_MODE, false);
    if (isMemDb) {
        LOGI("[GetSecurityOption] MemDb, no need to get security option");
        option = SecurityOption();
        return E_OK;
    }

    option.securityLabel = GetDbProperties().GetSecLabel();
    option.securityFlag = GetDbProperties().GetSecFlag();

    return E_OK;
}

namespace {
inline bool OriValueCanBeUse(int errCode)
{
    return (errCode == -E_VALUE_MATCH);
}

inline bool AmendValueShouldBeUse(int errCode)
{
    return (errCode == -E_VALUE_MATCH_AMENDED);
}

inline bool IsValueMismatched(int errCode)
{
    return (errCode == -E_VALUE_MISMATCH_FEILD_COUNT ||
            errCode == -E_VALUE_MISMATCH_FEILD_TYPE ||
            errCode == -E_VALUE_MISMATCH_CONSTRAINT);
}
}

int SQLiteSingleVerNaturalStore::CheckValueAndAmendIfNeed(ValueSource sourceType, const Value &oriValue,
    Value &amendValue, bool &useAmendValue) const
{
    // oriValue size may already be checked previously, but check here const little
    if (oriValue.size() > DBConstant::MAX_VALUE_SIZE) {
        return -E_INVALID_ARGS;
    }
    const SchemaObject &schemaObjRef = MyProp().GetSchemaConstRef();
    if (!schemaObjRef.IsSchemaValid()) {
        // Not a schema database, do not need to check more
        return E_OK;
    }
    if (schemaObjRef.GetSchemaType() == SchemaType::JSON) {
        ValueObject valueObj;
        int errCode = valueObj.Parse(oriValue.data(), oriValue.data() + oriValue.size(), schemaObjRef.GetSkipSize());
        if (errCode != E_OK) {
            return -E_INVALID_FORMAT;
        }
        errCode = schemaObjRef.CheckValueAndAmendIfNeed(sourceType, valueObj);
        if (OriValueCanBeUse(errCode)) {
            useAmendValue = false;
            return E_OK;
        }
        if (AmendValueShouldBeUse(errCode)) {
            std::string amended = valueObj.ToString();
            if (amended.size() > DBConstant::MAX_VALUE_SIZE) {
                LOGE("[SqlSinStore][CheckAmendValue] ValueSize=%zu exceed limit after amend.", amended.size());
                return -E_INVALID_FORMAT;
            }
            amendValue.clear();
            amendValue.assign(amended.begin(), amended.end());
            useAmendValue = true;
            return E_OK;
        }
        if (IsValueMismatched(errCode)) {
            return errCode;
        }
    } else {
        int errCode = schemaObjRef.VerifyValue(sourceType, oriValue);
        if (errCode == E_OK) {
            useAmendValue = false;
            return E_OK;
        }
    }
    // Any unexpected wrong
    return -E_INVALID_FORMAT;
}

int SQLiteSingleVerNaturalStore::ClearIncompleteDatabase(const KvDBProperties &kvDBPro) const
{
    std::string dbSubDir = SQLiteSingleVerNaturalStore::GetSubDirPath(kvDBPro);
    if (OS::CheckPathExistence(dbSubDir + DBConstant::PATH_POSTFIX_DB_INCOMPLETE)) {
        int errCode = DBCommon::RemoveAllFilesOfDirectory(dbSubDir);
        if (errCode != E_OK) {
            LOGE("Remove the incomplete database dir failed!");
            return -E_REMOVE_FILE;
        }
    }
    return E_OK;
}

int SQLiteSingleVerNaturalStore::CheckDatabaseRecovery(const KvDBProperties &kvDBProp)
{
    if (kvDBProp.GetBoolProp(KvDBProperties::MEMORY_MODE, false)) { // memory status not need recovery
        return E_OK;
    }
    std::unique_ptr<SingleVerDatabaseOper> operation = std::make_unique<SingleVerDatabaseOper>(this, nullptr);
    (void)operation->ClearExportedTempFiles(kvDBProp);
    int errCode = operation->RekeyRecover(kvDBProp);
    if (errCode != E_OK) {
        LOGE("Recover from rekey failed in single version:%d", errCode);
        return errCode;
    }

    errCode = operation->ClearImportTempFile(kvDBProp);
    if (errCode != E_OK) {
        LOGE("Clear imported temp db failed in single version:%d", errCode);
        return errCode;
    }

    // Currently, Design for the consistency of directory and file setting secOption
    errCode = ClearIncompleteDatabase(kvDBProp);
    if (errCode != E_OK) {
        LOGE("Clear incomplete database failed in single version:%d", errCode);
        return errCode;
    }
    const std::string dataDir = kvDBProp.GetStringProp(KvDBProperties::DATA_DIR, "");
    const std::string identifierDir = kvDBProp.GetStringProp(KvDBProperties::IDENTIFIER_DIR, "");
    bool isCreate = kvDBProp.GetBoolProp(KvDBProperties::CREATE_IF_NECESSARY, true);
    bool isMemoryDb = kvDBProp.GetBoolProp(KvDBProperties::MEMORY_MODE, false);
    if (!isMemoryDb) {
        errCode = DBCommon::CreateStoreDirectory(dataDir, identifierDir, DBConstant::SINGLE_SUB_DIR, isCreate);
        if (errCode != E_OK) {
            LOGE("Create single version natural store directory failed:%d", errCode);
        }
    }
    return errCode;
}

int SQLiteSingleVerNaturalStore::GetAndInitStorageEngine(const KvDBProperties &kvDBProp)
{
    int errCode = E_OK;
    storageEngine_ =
        static_cast<SQLiteSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(kvDBProp, errCode));
    if (storageEngine_ == nullptr) {
        return errCode;
    }

    if (storageEngine_->IsEngineCorrupted()) {
        LOGE("[SqlSinStore][GetAndInitStorageEngine] database engine is corrupted, not need continue to open!");
        return -E_INVALID_PASSWD_OR_CORRUPTED_DB;
    }

    errCode = InitDatabaseContext(kvDBProp);
    if (errCode != E_OK) {
        LOGE("[SqlSinStore][Open] Init database context fail! errCode = [%d]", errCode);
    }
    return errCode;
}

int SQLiteSingleVerNaturalStore::Open(const KvDBProperties &kvDBProp)
{
    std::lock_guard<std::mutex> lock(initialMutex_);
    if (isInitialized_) {
        return E_OK; // avoid the reopen operation.
    }

    int errCode = CheckDatabaseRecovery(kvDBProp);
    if (errCode != E_OK) {
        return errCode;
    }

    bool isReadOnly = false;
    SchemaObject savedSchemaObj;

    errCode = GetAndInitStorageEngine(kvDBProp);
    if (errCode != E_OK) {
        goto ERROR;
    }

    errCode = RegisterNotification();
    if (errCode != E_OK) {
        LOGE("Register notification failed:%d", errCode);
        goto ERROR;
    }

    errCode = RemoveAllSubscribe();
    if (errCode != E_OK) {
        LOGE("[SqlSinStore][Open] remove subscribe fail! errCode = [%d]", errCode);
        goto ERROR;
    }

    // Here, the dbfile is created or opened, and upgrade of table structure has done.
    // More, Upgrade of schema is also done in upgrader call in InitDatabaseContext, schema in dbfile updated if need.
    // If inputSchema is empty, upgrader do nothing of schema, isReadOnly will be true if dbfile contain schema before.
    // In this case, we should load the savedSchema for checking value from sync which not restricted by readOnly.
    // If inputSchema not empty, isReadOnly will not be true, we should do nothing more.
    errCode = DecideReadOnlyBaseOnSchema(kvDBProp, isReadOnly, savedSchemaObj);
    if (errCode != E_OK) {
        LOGE("[SqlSinStore][Open] DecideReadOnlyBaseOnSchema failed=%d", errCode);
        goto ERROR;
    }
    // Set KvDBProperties and set Schema
    MyProp() = kvDBProp;
    UpdateSecProperties(MyProp(), isReadOnly, savedSchemaObj, storageEngine_);

    StartSyncer();
    OnKill([this]() { ReleaseResources(); });

    errCode = SaveCreateDBTimeIfNotExisted();
    if (errCode != E_OK) {
        goto ERROR;
    }

    InitialLocalDataTimestamp();
    isInitialized_ = true;
    isReadOnly_ = isReadOnly;
    return E_OK;
ERROR:
    ReleaseResources();
    return errCode;
}

void SQLiteSingleVerNaturalStore::Close()
{
    ReleaseResources();
}

GenericKvDBConnection *SQLiteSingleVerNaturalStore::NewConnection(int &errCode)
{
    SQLiteSingleVerNaturalStoreConnection *connection = new (std::nothrow) SQLiteSingleVerNaturalStoreConnection(this);
    if (connection == nullptr) {
        errCode = -E_OUT_OF_MEMORY;
        return nullptr;
    }
    errCode = E_OK;
    return connection;
}

// Get interface type of this kvdb.
int SQLiteSingleVerNaturalStore::GetInterfaceType() const
{
    return SYNC_SVD;
}

// Get the interface ref-count, in order to access asynchronously.
void SQLiteSingleVerNaturalStore::IncRefCount()
{
    IncObjRef(this);
}

// Drop the interface ref-count.
void SQLiteSingleVerNaturalStore::DecRefCount()
{
    DecObjRef(this);
}

// Get the identifier of this kvdb.
std::vector<uint8_t> SQLiteSingleVerNaturalStore::GetIdentifier() const
{
    std::string identifier = MyProp().GetStringProp(KvDBProperties::IDENTIFIER_DATA, "");
    std::vector<uint8_t> identifierVect(identifier.begin(), identifier.end());
    return identifierVect;
}

std::vector<uint8_t> SQLiteSingleVerNaturalStore::GetDualTupleIdentifier() const
{
    std::string identifier = MyProp().GetStringProp(KvDBProperties::DUAL_TUPLE_IDENTIFIER_DATA, "");
    std::vector<uint8_t> identifierVect(identifier.begin(), identifier.end());
    return identifierVect;
}

// Get interface for syncer.
IKvDBSyncInterface *SQLiteSingleVerNaturalStore::GetSyncInterface()
{
    return this;
}

int SQLiteSingleVerNaturalStore::GetMetaData(const Key &key, Value &value) const
{
    if (storageEngine_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (key.size() > DBConstant::MAX_KEY_SIZE) {
        return -E_INVALID_ARGS;
    }

    int errCode = E_OK;
    auto handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    Timestamp timestamp;
    errCode = handle->GetKvData(SingleVerDataType::META_TYPE, key, value, timestamp);
    ReleaseHandle(handle);
    HeartBeatForLifeCycle();
    return errCode;
}

int SQLiteSingleVerNaturalStore::PutMetaData(const Key &key, const Value &value)
{
    int errCode = SQLiteSingleVerNaturalStore::CheckDataStatus(key, value, false);
    if (errCode != E_OK) {
        return errCode;
    }

    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->PutKvData(SingleVerDataType::META_TYPE, key, value, 0, nullptr); // meta doesn't need time.
    if (errCode != E_OK) {
        LOGE("Put kv data err:%d", errCode);
    }

    HeartBeatForLifeCycle();
    ReleaseHandle(handle);
    return errCode;
}

// Delete multiple meta data records in a transaction.
int SQLiteSingleVerNaturalStore::DeleteMetaData(const std::vector<Key> &keys)
{
    for (const auto &key : keys) {
        if (key.empty() || key.size() > DBConstant::MAX_KEY_SIZE) {
            return -E_INVALID_ARGS;
        }
    }
    int errCode = E_OK;
    auto handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    handle->StartTransaction(TransactType::IMMEDIATE);
    errCode = handle->DeleteMetaData(keys);
    if (errCode != E_OK) {
        handle->Rollback();
        LOGE("[SinStore] DeleteMetaData failed, errCode = %d", errCode);
    } else {
        handle->Commit();
    }

    ReleaseHandle(handle);
    HeartBeatForLifeCycle();
    return errCode;
}

int SQLiteSingleVerNaturalStore::GetAllMetaKeys(std::vector<Key> &keys) const
{
    if (storageEngine_ == nullptr) {
        return -E_INVALID_DB;
    }
    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->GetAllMetaKeys(keys);
    ReleaseHandle(handle);
    return errCode;
}

void SQLiteSingleVerNaturalStore::CommitAndReleaseNotifyData(SingleVerNaturalStoreCommitNotifyData *&committedData,
    bool isNeedCommit, int eventType)
{
    if (isNeedCommit) {
        if (committedData != nullptr) {
            if (!committedData->IsChangedDataEmpty()) {
                CommitNotify(eventType, committedData);
            }
            if (!committedData->IsConflictedDataEmpty()) {
                CommitNotify(SQLITE_GENERAL_CONFLICT_EVENT, committedData);
            }
        }
    }

    if (committedData != nullptr) {
        committedData->DecObjRef(committedData);
        committedData = nullptr;
    }
}

int SQLiteSingleVerNaturalStore::GetSyncData(Timestamp begin, Timestamp end, std::vector<SingleVerKvEntry *> &entries,
    ContinueToken &continueStmtToken, const DataSizeSpecInfo &dataSizeInfo) const
{
    int errCode = CheckReadDataControlled();
    if (errCode != E_OK) {
        LOGE("[GetSyncData] Existed cache database can not read data, errCode = [%d]!", errCode);
        return errCode;
    }

    std::vector<DataItem> dataItems;
    errCode = GetSyncData(begin, end, dataItems, continueStmtToken, dataSizeInfo);
    if (errCode != E_OK && errCode != -E_UNFINISHED) {
        LOGE("GetSyncData errCode:%d", errCode);
        goto ERROR;
    }

    for (auto &item : dataItems) {
        GenericSingleVerKvEntry *entry = new (std::nothrow) GenericSingleVerKvEntry();
        if (entry == nullptr) {
            errCode = -E_OUT_OF_MEMORY;
            LOGE("GetSyncData errCode:%d", errCode);
            goto ERROR;
        }
        entry->SetEntryData(std::move(item));
        entries.push_back(entry);
    }

ERROR:
    if (errCode != E_OK && errCode != -E_UNFINISHED) {
        SingleVerKvEntry::Release(entries);
    }
    HeartBeatForLifeCycle();
    return errCode;
}

int SQLiteSingleVerNaturalStore::GetSyncData(Timestamp begin, Timestamp end, std::vector<DataItem> &dataItems,
    ContinueToken &continueStmtToken, const DataSizeSpecInfo &dataSizeInfo) const
{
    if (begin >= end || dataSizeInfo.blockSize > DBConstant::MAX_SYNC_BLOCK_SIZE) {
        return -E_INVALID_ARGS;
    }

    auto token = new (std::nothrow) SQLiteSingleVerContinueToken(begin, end);
    if (token == nullptr) {
        LOGE("[SQLiteSingleVerNaturalStore][NewToken] Bad alloc.");
        return -E_OUT_OF_MEMORY;
    }

    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(false, errCode);
    if (handle == nullptr) {
        goto ERROR;
    }

    errCode = handle->GetSyncDataByTimestamp(dataItems, GetAppendedLen(), begin, end, dataSizeInfo);
    if (errCode == -E_FINISHED) {
        errCode = E_OK;
    }

ERROR:
    if (errCode != -E_UNFINISHED  && errCode != E_OK) {
        dataItems.clear();
    }
    ProcessContinueToken(dataItems, errCode, token);
    continueStmtToken = static_cast<ContinueToken>(token);

    ReleaseHandle(handle);
    return errCode;
}

int SQLiteSingleVerNaturalStore::GetSyncData(QueryObject &query, const SyncTimeRange &timeRange,
    const DataSizeSpecInfo &dataSizeInfo, ContinueToken &continueStmtToken,
    std::vector<SingleVerKvEntry *> &entries) const
{
    if (!timeRange.IsValid()) {
        return -E_INVALID_ARGS;
    }
    int errCode = CheckReadDataControlled();
    if (errCode != E_OK) {
        LOGE("[GetEntries] Existed cache prevents the reading from query sync[%d]!", errCode);
        return errCode;
    }

    query.SetSchema(GetSchemaObject());
    auto token = new (std::nothrow) SQLiteSingleVerContinueToken(timeRange, query);
    if (token == nullptr) {
        LOGE("[SingleVerNStore] Allocate continue token failed.");
        return -E_OUT_OF_MEMORY;
    }

    int innerCode;
    std::vector<DataItem> dataItems;
    errCode = GetSyncDataForQuerySync(dataItems, token, dataSizeInfo);
    if (errCode != E_OK && errCode != -E_UNFINISHED) { // The code need be sent to outside except new error happened.
        goto ERROR;
    }

    innerCode = GetKvEntriesByDataItems(entries, dataItems);
    if (innerCode != E_OK) {
        errCode = innerCode;
        delete token;
        token = nullptr;
    }

ERROR:
    continueStmtToken = static_cast<ContinueToken>(token);
    return errCode;
}

/**
 * Caller must ensure that parameter continueStmtToken is valid.
 * If error happened, token will be deleted here.
 */
int SQLiteSingleVerNaturalStore::GetSyncDataForQuerySync(std::vector<DataItem> &dataItems,
    SQLiteSingleVerContinueToken *&continueStmtToken, const DataSizeSpecInfo &dataSizeInfo) const
{
    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(false, errCode);
    if (handle == nullptr) {
        goto ERROR;
    }

    // Get query data.
    if (!continueStmtToken->IsGetQueryDataFinished()) {
        LOGD("[SingleVerNStore] Get query data between %" PRIu64 " and %" PRIu64 ".",
            continueStmtToken->GetQueryBeginTime(), continueStmtToken->GetQueryEndTime());
        errCode = handle->GetSyncDataWithQuery(continueStmtToken->GetQuery(), GetAppendedLen(), dataSizeInfo,
            std::make_pair(continueStmtToken->GetQueryBeginTime(), continueStmtToken->GetQueryEndTime()), dataItems);
    }

    // Get query data finished.
    if (errCode == E_OK || errCode == -E_FINISHED) {
        // Clear query timeRange.
        continueStmtToken->FinishGetQueryData();
        if (!continueStmtToken->IsGetDeletedDataFinished()) {
            errCode = -E_UNFINISHED;
            // Get delete time next.
            if (CanHoldDeletedData(dataItems, dataSizeInfo, GetAppendedLen())) {
                LOGD("[SingleVerNStore] Get deleted data between %" PRIu64 " and %" PRIu64 ".",
                    continueStmtToken->GetDeletedBeginTime(), continueStmtToken->GetDeletedEndTime());
                errCode = handle->GetDeletedSyncDataByTimestamp(dataItems, GetAppendedLen(),
                    continueStmtToken->GetDeletedBeginTime(), continueStmtToken->GetDeletedEndTime(), dataSizeInfo);
            }
        }
    }

    if (errCode == -E_FINISHED) {
        errCode = E_OK;
    }

ERROR:
    if (errCode != -E_UNFINISHED && errCode != E_OK) { // Error happened.
        dataItems.clear();
    }
    ProcessContinueTokenForQuerySync(dataItems, errCode, continueStmtToken);
    ReleaseHandle(handle);
    return errCode;
}

int SQLiteSingleVerNaturalStore::GetSyncDataNext(std::vector<SingleVerKvEntry *> &entries,
    ContinueToken &continueStmtToken, const DataSizeSpecInfo &dataSizeInfo) const
{
    int errCode = CheckReadDataControlled();
    if (errCode != E_OK) {
        LOGE("[GetSyncDataNext] Existed cache database can not read data, errCode = [%d]!", errCode);
        return errCode;
    }

    std::vector<DataItem> dataItems;
    auto token = static_cast<SQLiteSingleVerContinueToken *>(continueStmtToken);
    if (token->IsQuerySync()) {
        errCode = GetSyncDataForQuerySync(dataItems, token, dataSizeInfo);
        continueStmtToken = static_cast<ContinueToken>(token);
    } else {
        errCode = GetSyncDataNext(dataItems, continueStmtToken, dataSizeInfo);
    }

    if (errCode != E_OK && errCode != -E_UNFINISHED) {
        LOGE("GetSyncDataNext errCode:%d", errCode);
        return errCode;
    }

    int innerErrCode = GetKvEntriesByDataItems(entries, dataItems);
    if (innerErrCode != E_OK) {
        errCode = innerErrCode;
        ReleaseContinueToken(continueStmtToken);
    }
    return errCode;
}

int SQLiteSingleVerNaturalStore::GetSyncDataNext(std::vector<DataItem> &dataItems, ContinueToken &continueStmtToken,
    const DataSizeSpecInfo &dataSizeInfo) const
{
    if (dataSizeInfo.blockSize > DBConstant::MAX_SYNC_BLOCK_SIZE) {
        return -E_INVALID_ARGS;
    }

    auto token = static_cast<SQLiteSingleVerContinueToken *>(continueStmtToken);
    if (token == nullptr || !(token->CheckValid())) {
        LOGE("[SingleVerNaturalStore][GetSyncDataNext] invalid continue token.");
        return -E_INVALID_ARGS;
    }

    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(false, errCode);
    if (handle == nullptr) {
        ReleaseContinueToken(continueStmtToken);
        return errCode;
    }

    errCode = handle->GetSyncDataByTimestamp(dataItems, GetAppendedLen(), token->GetQueryBeginTime(),
        token->GetQueryEndTime(), dataSizeInfo);
    if (errCode == -E_FINISHED) {
        errCode = E_OK;
    }

    ProcessContinueToken(dataItems, errCode, token);
    continueStmtToken = static_cast<ContinueToken>(token);

    ReleaseHandle(handle);
    return errCode;
}

void SQLiteSingleVerNaturalStore::ReleaseContinueToken(ContinueToken &continueStmtToken) const
{
    auto token = static_cast<SQLiteSingleVerContinueToken *>(continueStmtToken);
    if (token == nullptr || !(token->CheckValid())) {
        LOGE("[SQLiteSingleVerNaturalStore][ReleaseContinueToken] Input is not a continue token.");
        return;
    }
    delete token;
    continueStmtToken = nullptr;
}

int SQLiteSingleVerNaturalStore::PutSyncDataWithQuery(const QueryObject &query,
    const std::vector<SingleVerKvEntry *> &entries, const std::string &deviceName)
{
    if (deviceName.length() > DBConstant::MAX_DEV_LENGTH) {
        LOGW("Device length is invalid for sync put");
        return -E_INVALID_ARGS;
    }
    HeartBeatForLifeCycle();
    DeviceInfo deviceInfo = {false, deviceName};
    if (deviceName.empty()) {
        deviceInfo.deviceName = "Unknown";
    }

    std::vector<DataItem> dataItems;
    for (const auto itemEntry : entries) {
        auto *entry = static_cast<GenericSingleVerKvEntry *>(itemEntry);
        if (entry != nullptr) {
            DataItem item;
            item.origDev = entry->GetOrigDevice();
            item.flag = entry->GetFlag();
            item.timestamp = entry->GetTimestamp();
            item.writeTimestamp = entry->GetWriteTimestamp();
            entry->GetKey(item.key);
            entry->GetValue(item.value);
            dataItems.push_back(item);
        }
    }

    int errCode = SaveSyncDataItems(query, dataItems, deviceInfo, true); // Current is true to check value content
    if (errCode != E_OK) {
        LOGE("PutSyncData failed:%d", errCode);
    }

    return errCode;
}

void SQLiteSingleVerNaturalStore::GetMaxTimestamp(Timestamp &stamp) const
{
    std::lock_guard<std::mutex> lock(maxTimestampMutex_);
    stamp = currentMaxTimestamp_;
}

int SQLiteSingleVerNaturalStore::SetMaxTimestamp(Timestamp timestamp)
{
    std::lock_guard<std::mutex> lock(maxTimestampMutex_);
    if (timestamp > currentMaxTimestamp_) {
        currentMaxTimestamp_ = timestamp;
    }
    return E_OK;
}

// In sync procedure, call this function
int SQLiteSingleVerNaturalStore::RemoveDeviceData(const std::string &deviceName, bool isNeedNotify)
{
    LOGI("[RemoveDeviceData] %s{private} rebuild, clear historydata", deviceName.c_str());
    return RemoveDeviceData(deviceName, isNeedNotify, true);
}

// In local procedure, call this function
int SQLiteSingleVerNaturalStore::RemoveDeviceData(const std::string &deviceName, bool isNeedNotify, bool isInSync)
{
    if (deviceName.empty() || deviceName.length() > DBConstant::MAX_DEV_LENGTH) {
        return -E_INVALID_ARGS;
    }
    if (!isInSync && !CheckWritePermission()) {
        return -E_NOT_PERMIT;
    }
    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        LOGE("[SingleVerNStore] RemoveDeviceData get handle failed:%d", errCode);
        return errCode;
    }
    uint64_t logFileSize = handle->GetLogFileSize();
    ReleaseHandle(handle);
    if (logFileSize > GetMaxLogSize()) {
        LOGW("[SingleVerNStore] RmDevData log size[%" PRIu64 "] over the limit", logFileSize);
        return -E_LOG_OVER_LIMITS;
    }

    // Call the syncer module to erase the water mark.
    errCode = EraseDeviceWaterMark(deviceName, true);
    if (errCode != E_OK) {
        LOGE("[SingleVerNStore] erase water mark failed:%d", errCode);
        return errCode;
    }

    if (IsExtendedCacheDBMode()) {
        errCode = RemoveDeviceDataInCacheMode(deviceName, isNeedNotify);
    } else {
        errCode = RemoveDeviceDataNormally(deviceName, isNeedNotify);
    }
    if (errCode != E_OK) {
        LOGE("[SingleVerNStore] RemoveDeviceData failed:%d", errCode);
    }

    return errCode;
}

int SQLiteSingleVerNaturalStore::RemoveDeviceDataInCacheMode(const std::string &deviceName, bool isNeedNotify)
{
    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        LOGE("[SingleVerNStore] RemoveDeviceData get handle failed:%d", errCode);
        return errCode;
    }
    uint64_t recordVersion = GetAndIncreaseCacheRecordVersion();
    LOGI("Remove device data in cache mode isNeedNotify:%d, recordVersion:%" PRIu64, isNeedNotify, recordVersion);
    errCode = handle->RemoveDeviceDataInCacheMode(deviceName, isNeedNotify, recordVersion);
    if (errCode != E_OK) {
        LOGE("[SingleVerNStore] RemoveDeviceDataInCacheMode failed:%d", errCode);
    }
    ReleaseHandle(handle);
    return errCode;
}

int SQLiteSingleVerNaturalStore::RemoveDeviceDataNormally(const std::string &deviceName, bool isNeedNotify)
{
    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        LOGE("[SingleVerNStore] RemoveDeviceData get handle failed:%d", errCode);
        return errCode;
    }

    std::vector<Entry> entries;
    if (isNeedNotify) {
        handle->GetAllSyncedEntries(deviceName, entries);
    }

    LOGI("Remove device data:%d", isNeedNotify);
    errCode = handle->RemoveDeviceData(deviceName);
    if (errCode == E_OK && isNeedNotify) {
        NotifyRemovedData(entries);
    }
    ReleaseHandle(handle);
    return errCode;
}

void SQLiteSingleVerNaturalStore::NotifyRemovedData(std::vector<Entry> &entries)
{
    if (entries.empty() || entries.size() > MAX_TOTAL_NOTIFY_ITEM_SIZE) {
        return;
    }

    size_t index = 0;
    size_t totalSize = 0;
    SingleVerNaturalStoreCommitNotifyData *notifyData = nullptr;
    while (index < entries.size()) {
        if (notifyData == nullptr) {
            notifyData = new (std::nothrow) SingleVerNaturalStoreCommitNotifyData;
            if (notifyData == nullptr) {
                LOGE("Failed to do commit sync removing because of OOM");
                break;
            }
        }

        // ignore the invalid key.
        if (entries[index].key.size() > DBConstant::MAX_KEY_SIZE ||
            entries[index].value.size() > DBConstant::MAX_VALUE_SIZE) {
            index++;
            continue;
        }

        if ((entries[index].key.size() + entries[index].value.size() + totalSize) > MAX_TOTAL_NOTIFY_DATA_SIZE) {
            CommitAndReleaseNotifyData(notifyData, true, SQLITE_GENERAL_NS_SYNC_EVENT);
            totalSize = 0;
            notifyData = nullptr;
            continue;
        }

        totalSize += (entries[index].key.size() + entries[index].value.size());
        notifyData->InsertCommittedData(std::move(entries[index]), DataType::DELETE, false);
        index++;
    }
    if (notifyData != nullptr) {
        CommitAndReleaseNotifyData(notifyData, true, SQLITE_GENERAL_NS_SYNC_EVENT);
    }
}

SQLiteSingleVerStorageExecutor *SQLiteSingleVerNaturalStore::GetHandle(bool isWrite, int &errCode,
    OperatePerm perm) const
{
    if (storageEngine_ == nullptr) {
        errCode = -E_INVALID_DB;
        return nullptr;
    }
    // Use for check database corrupted in Asynchronous task, like cache data migrate to main database
    if (storageEngine_->IsEngineCorrupted()) {
        CorruptNotify();
        errCode = -E_INVALID_PASSWD_OR_CORRUPTED_DB;
        LOGI("Handle is corrupted can not to get! errCode = [%d]", errCode);
        return nullptr;
    }
    return static_cast<SQLiteSingleVerStorageExecutor *>(storageEngine_->FindExecutor(isWrite, perm, errCode));
}

void SQLiteSingleVerNaturalStore::ReleaseHandle(SQLiteSingleVerStorageExecutor *&handle) const
{
    if (handle == nullptr) {
        return;
    }

    if (storageEngine_ != nullptr) {
        bool isCorrupted = handle->GetCorruptedStatus();
        StorageExecutor *databaseHandle = handle;
        storageEngine_->Recycle(databaseHandle);
        handle = nullptr;
        if (isCorrupted) {
            CorruptNotify();
        }
    }
}

int SQLiteSingleVerNaturalStore::RegisterNotification()
{
    static const std::vector<int> events {
        static_cast<int>(SQLITE_GENERAL_NS_LOCAL_PUT_EVENT),
        static_cast<int>(SQLITE_GENERAL_NS_PUT_EVENT),
        static_cast<int>(SQLITE_GENERAL_NS_SYNC_EVENT),
        static_cast<int>(SQLITE_GENERAL_CONFLICT_EVENT),
    };

    for (auto event = events.begin(); event != events.end(); ++event) {
        int errCode = RegisterNotificationEventType(*event);
        if (errCode == E_OK) {
            continue;
        }
        LOGE("Register single version event %d failed:%d!", *event, errCode);
        for (auto iter = events.begin(); iter != event; ++iter) {
            UnRegisterNotificationEventType(*iter);
        }
        return errCode;
    }

    notificationEventsRegistered_ = true;
    notificationConflictEventsRegistered_ = true;
    return E_OK;
}

void SQLiteSingleVerNaturalStore::ReleaseResources()
{
    SyncAbleKvDB::Close();
    if (notificationEventsRegistered_) {
        UnRegisterNotificationEventType(static_cast<EventType>(SQLITE_GENERAL_NS_SYNC_EVENT));
        UnRegisterNotificationEventType(static_cast<EventType>(SQLITE_GENERAL_NS_PUT_EVENT));
        UnRegisterNotificationEventType(static_cast<EventType>(SQLITE_GENERAL_NS_LOCAL_PUT_EVENT));
        notificationEventsRegistered_ = false;
    }

    if (notificationConflictEventsRegistered_) {
        UnRegisterNotificationEventType(static_cast<EventType>(SQLITE_GENERAL_CONFLICT_EVENT));
        notificationConflictEventsRegistered_ = false;
    }
    {
        std::lock_guard<std::mutex> lock(syncerMutex_);
        if (storageEngine_ != nullptr) {
            storageEngine_->ClearEnginePasswd();
            (void)StorageEngineManager::ReleaseStorageEngine(storageEngine_);
            storageEngine_ = nullptr;
        }
    }

    isInitialized_ = false;
}

void SQLiteSingleVerNaturalStore::InitCurrentMaxStamp()
{
    if (storageEngine_ == nullptr) {
        return;
    }
    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return;
    }

    handle->InitCurrentMaxStamp(currentMaxTimestamp_);
    LOGD("Init max timestamp:%" PRIu64, currentMaxTimestamp_);
    ReleaseHandle(handle);
}

void SQLiteSingleVerNaturalStore::InitConflictNotifiedFlag(SingleVerNaturalStoreCommitNotifyData *committedData)
{
    unsigned int conflictFlag = 0;
    if (GetRegisterFunctionCount(CONFLICT_SINGLE_VERSION_NS_FOREIGN_KEY_ONLY) != 0) {
        conflictFlag |= static_cast<unsigned>(SQLITE_GENERAL_NS_FOREIGN_KEY_ONLY);
    }
    if (GetRegisterFunctionCount(CONFLICT_SINGLE_VERSION_NS_FOREIGN_KEY_ORIG) != 0) {
        conflictFlag |= static_cast<unsigned>(SQLITE_GENERAL_NS_FOREIGN_KEY_ORIG);
    }
    if (GetRegisterFunctionCount(CONFLICT_SINGLE_VERSION_NS_NATIVE_ALL) != 0) {
        conflictFlag |= static_cast<unsigned>(SQLITE_GENERAL_NS_NATIVE_ALL);
    }
    committedData->SetConflictedNotifiedFlag(static_cast<int>(conflictFlag));
}

// Currently this function only suitable to be call from sync in insert_record_from_sync procedure
// Take attention if future coder attempt to call it in other situation procedure
int SQLiteSingleVerNaturalStore::SaveSyncDataItems(const QueryObject &query, std::vector<DataItem> &dataItems,
    const DeviceInfo &deviceInfo, bool checkValueContent)
{
    // Sync procedure does not care readOnly Flag
    if (storageEngine_ == nullptr) {
        return -E_INVALID_DB;
    }
    int errCode = E_OK;
    for (const auto &item : dataItems) {
        // Check only the key and value size
        errCode = CheckDataStatus(item.key, item.value, (item.flag & DataItem::DELETE_FLAG) != 0);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    if (checkValueContent) {
        CheckAmendValueContentForSyncProcedure(dataItems);
    }
    QueryObject queryInner = query;
    queryInner.SetSchema(GetSchemaObjectConstRef());
    if (IsExtendedCacheDBMode()) {
        errCode = SaveSyncDataToCacheDB(queryInner, dataItems, deviceInfo);
    } else {
        errCode = SaveSyncDataToMain(queryInner, dataItems, deviceInfo);
    }
    if (errCode != E_OK) {
        LOGE("[SingleVerNStore] SaveSyncDataItems failed:%d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerNaturalStore::SaveSyncDataToMain(const QueryObject &query, std::vector<DataItem> &dataItems,
    const DeviceInfo &deviceInfo)
{
    auto *committedData = new (std::nothrow) SingleVerNaturalStoreCommitNotifyData;
    if (committedData == nullptr) {
        LOGE("[SingleVerNStore] Failed to alloc single version notify data");
        return -E_OUT_OF_MEMORY;
    }
    InitConflictNotifiedFlag(committedData);
    Timestamp maxTimestamp = 0;
    bool isNeedCommit = false;
    int errCode = SaveSyncItems(query, dataItems, deviceInfo, maxTimestamp, committedData);
    if (errCode == E_OK) {
        isNeedCommit = true;
        (void)SetMaxTimestamp(maxTimestamp);
    }

    CommitAndReleaseNotifyData(committedData, isNeedCommit, SQLITE_GENERAL_NS_SYNC_EVENT);
    return errCode;
}

// Currently, this function only suitable to be call from sync in insert_record_from_sync procedure
// Take attention if future coder attempt to call it in other situation procedure
int SQLiteSingleVerNaturalStore::SaveSyncItems(const QueryObject &query, std::vector<DataItem> &dataItems,
    const DeviceInfo &deviceInfo, Timestamp &maxTimestamp, SingleVerNaturalStoreCommitNotifyData *commitData) const
{
    int errCode = E_OK;
    int innerCode = E_OK;
    LOGD("[SQLiteSingleVerNaturalStore::SaveSyncData] Get write handle.");
    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }
    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        ReleaseHandle(handle);
        return errCode;
    }
    bool isPermitForceWrite = !(GetDbProperties().GetBoolProp(KvDBProperties::SYNC_DUAL_TUPLE_MODE, false));
    errCode = handle->CheckDataWithQuery(query, dataItems, deviceInfo);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = handle->PrepareForSavingData(SingleVerDataType::SYNC_TYPE);
    if (errCode != E_OK) {
        goto END;
    }
    for (auto &item: dataItems) {
        if (item.neglect) { // Do not save this record if it is neglected
            continue;
        }
        errCode = handle->SaveSyncDataItem(item, deviceInfo, maxTimestamp, commitData, isPermitForceWrite);
        if (errCode != E_OK && errCode != -E_NOT_FOUND) {
            break;
        }
    }
    if (errCode == -E_NOT_FOUND) {
        errCode = E_OK;
    }
    innerCode = handle->ResetForSavingData(SingleVerDataType::SYNC_TYPE);
    if (innerCode != E_OK) {
        errCode = innerCode;
    }
END:
    if (errCode == E_OK) {
        errCode = handle->Commit();
    } else {
        (void)handle->Rollback(); // Keep the error code of the first scene
    }
    ReleaseHandle(handle);
    return errCode;
}

int SQLiteSingleVerNaturalStore::SaveSyncDataToCacheDB(const QueryObject &query, std::vector<DataItem> &dataItems,
    const DeviceInfo &deviceInfo)
{
    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    Timestamp maxTimestamp = 0;
    errCode = SaveSyncItemsInCacheMode(handle, query, dataItems, deviceInfo, maxTimestamp);
    if (errCode != E_OK) {
        LOGE("[SingleVerNStore] Failed to save sync data in cache mode, err : %d", errCode);
    } else {
        (void)SetMaxTimestamp(maxTimestamp);
    }
    ReleaseHandle(handle);
    return errCode;
}

Timestamp SQLiteSingleVerNaturalStore::GetCurrentTimestamp()
{
    return GetTimestamp();
}

int SQLiteSingleVerNaturalStore::InitStorageEngine(const KvDBProperties &kvDBProp, bool isNeedUpdateSecOpt)
{
    OpenDbProperties option;
    InitDataBaseOption(kvDBProp, option);

    bool isMemoryMode = kvDBProp.GetBoolProp(KvDBProperties::MEMORY_MODE, false);
    StorageEngineAttr poolSize = {1, 1, 1, 16}; // at most 1 write 16 read.
    if (isMemoryMode) {
        poolSize.minWriteNum = 1; // keep at least one connection.
    }

    storageEngine_->SetNotifiedCallback(
        [&](int eventType, KvDBCommitNotifyFilterAbleData *committedData) {
            if (eventType == SQLITE_GENERAL_FINISH_MIGRATE_EVENT) {
                return this->TriggerSync(eventType);
            }
            auto commitData = static_cast<SingleVerNaturalStoreCommitNotifyData *>(committedData);
            this->CommitAndReleaseNotifyData(commitData, true, eventType);
        }
    );

    std::string identifier = kvDBProp.GetStringProp(KvDBProperties::IDENTIFIER_DATA, "");
    storageEngine_->SetNeedUpdateSecOption(isNeedUpdateSecOpt);
    int errCode = storageEngine_->InitSQLiteStorageEngine(poolSize, option, identifier);
    if (errCode != E_OK) {
        LOGE("Init the sqlite storage engine failed:%d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerNaturalStore::Rekey(const CipherPassword &passwd)
{
    // Check the storage engine and try to disable the engine.
    if (storageEngine_ == nullptr) {
        return -E_INVALID_DB;
    }

    std::unique_ptr<SingleVerDatabaseOper> operation;

    // stop the syncer
    int errCode = storageEngine_->TryToDisable(false, OperatePerm::REKEY_MONOPOLIZE_PERM);
    if (errCode != E_OK) {
        return errCode;
    }
    LOGI("Stop the syncer for rekey");
    StopSyncer(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));  // wait for 5 ms
    errCode = storageEngine_->TryToDisable(true, OperatePerm::REKEY_MONOPOLIZE_PERM);
    if (errCode != E_OK) {
        LOGE("[Rekey] Failed to disable the database: %d", errCode);
        goto END;
    }

    if (storageEngine_->GetEngineState() != EngineState::MAINDB) {
        LOGE("Rekey is not supported while cache exists! state = [%d]", storageEngine_->GetEngineState());
        errCode = (storageEngine_->GetEngineState() == EngineState::CACHEDB) ? -E_NOT_SUPPORT : -E_BUSY;
        goto END;
    }

    operation = std::make_unique<SingleVerDatabaseOper>(this, storageEngine_);
    LOGI("Operation rekey");
    errCode = operation->Rekey(passwd);
END:
    // Only maindb state have existed handle, if rekey fail other state will create error cache db
    // Abort can forbid get new handle, requesting handle will return BUSY and nullptr handle
    if (errCode != -E_FORBID_CACHEDB) {
        storageEngine_->Enable(OperatePerm::REKEY_MONOPOLIZE_PERM);
    } else {
        storageEngine_->Abort(OperatePerm::REKEY_MONOPOLIZE_PERM);
        errCode = E_OK;
    }
    StartSyncer();
    return errCode;
}

int SQLiteSingleVerNaturalStore::Export(const std::string &filePath, const CipherPassword &passwd)
{
    if (storageEngine_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (MyProp().GetBoolProp(KvDBProperties::MEMORY_MODE, false)) {
        return -E_NOT_SUPPORT;
    }

    // Exclusively write resources
    std::string localDev;
    int errCode = GetLocalIdentity(localDev);
    if (errCode != E_OK) {
        LOGE("Get local dev id err:%d", errCode);
        localDev.resize(0);
    }

    // The write handle is applied to prevent writing data during the export process.
    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode, OperatePerm::NORMAL_PERM);
    if (handle == nullptr) {
        return errCode;
    }

    // forbid migrate by hold write handle not release
    if (storageEngine_->GetEngineState() != EngineState::MAINDB) {
        LOGE("Not support export when cacheDB existed! state = [%d]", storageEngine_->GetEngineState());
        errCode = (storageEngine_->GetEngineState() == EngineState::CACHEDB) ? -E_NOT_SUPPORT : -E_BUSY;
        ReleaseHandle(handle);
        return errCode;
    }

    std::unique_ptr<SingleVerDatabaseOper> operation = std::make_unique<SingleVerDatabaseOper>(this, storageEngine_);
    operation->SetLocalDevId(localDev);
    LOGI("Begin export the kv store");
    errCode = operation->Export(filePath, passwd);

    ReleaseHandle(handle);
    return errCode;
}

int SQLiteSingleVerNaturalStore::Import(const std::string &filePath, const CipherPassword &passwd)
{
    if (storageEngine_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (MyProp().GetBoolProp(KvDBProperties::MEMORY_MODE, false)) {
        return -E_NOT_SUPPORT;
    }

    std::string localDev;
    int errCode = GetLocalIdentity(localDev);
    if (errCode != E_OK) {
        LOGE("Failed to GetLocalIdentity!");
        localDev.resize(0);
    }

    // stop the syncer
    errCode = storageEngine_->TryToDisable(false, OperatePerm::IMPORT_MONOPOLIZE_PERM);
    if (errCode != E_OK) {
        return errCode;
    }
    StopSyncer(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(5)); // wait for 5 ms
    std::unique_ptr<SingleVerDatabaseOper> operation;

    errCode = storageEngine_->TryToDisable(true, OperatePerm::IMPORT_MONOPOLIZE_PERM);
    if (errCode != E_OK) {
        LOGE("[Import] Failed to disable the database: %d", errCode);
        goto END;
    }

    if (storageEngine_->GetEngineState() != EngineState::MAINDB) {
        LOGE("Not support import when cacheDB existed! state = [%d]", storageEngine_->GetEngineState());
        errCode = (storageEngine_->GetEngineState() == EngineState::CACHEDB) ? -E_NOT_SUPPORT : -E_BUSY;
        goto END;
    }

    operation = std::make_unique<SingleVerDatabaseOper>(this, storageEngine_);
    operation->SetLocalDevId(localDev);
    errCode = operation->Import(filePath, passwd);
    if (errCode != E_OK) {
        goto END;
    }

    // Save create db time.
    storageEngine_->Enable(OperatePerm::IMPORT_MONOPOLIZE_PERM);
    errCode = SaveCreateDBTime();

END:
    // restore the storage engine and the syncer.
    storageEngine_->Enable(OperatePerm::IMPORT_MONOPOLIZE_PERM);
    StartSyncer();
    return errCode;
}

bool SQLiteSingleVerNaturalStore::CheckWritePermission() const
{
    return !isReadOnly_;
}

SchemaObject SQLiteSingleVerNaturalStore::GetSchemaInfo() const
{
    return MyProp().GetSchemaConstRef();
}

SchemaObject SQLiteSingleVerNaturalStore::GetSchemaObject() const
{
    return MyProp().GetSchema();
}

const SchemaObject &SQLiteSingleVerNaturalStore::GetSchemaObjectConstRef() const
{
    return MyProp().GetSchemaConstRef();
}

bool SQLiteSingleVerNaturalStore::CheckCompatible(const std::string &schema, uint8_t type) const
{
    const SchemaObject &localSchema = MyProp().GetSchemaConstRef();
    if (!localSchema.IsSchemaValid() || schema.empty() || ReadSchemaType(type) == SchemaType::NONE) {
        // If at least one of local or remote is normal-kvdb, then allow sync
        LOGI("IsLocalSchemaDb=%d, IsRemoteSchemaDb=%d.", localSchema.IsSchemaValid(), !schema.empty());
        return true;
    }
    // Here both are schema-db, check their compatibility mutually
    SchemaObject remoteSchema;
    int errCode = remoteSchema.ParseFromSchemaString(schema);
    if (errCode != E_OK) {
        // Consider: if the parse errCode is SchemaVersionNotSupport, we can consider allow sync if schemaType equal.
        LOGE("Parse remote schema fail, errCode=%d.", errCode);
        return false;
    }
    // First, Compare remoteSchema based on localSchema
    errCode = localSchema.CompareAgainstSchemaObject(remoteSchema);
    if (errCode != -E_SCHEMA_UNEQUAL_INCOMPATIBLE) {
        LOGI("Remote(Maybe newer) compatible based on local, result=%d.", errCode);
        return true;
    }
    // Second, Compare localSchema based on remoteSchema
    errCode = remoteSchema.CompareAgainstSchemaObject(localSchema);
    if (errCode != -E_SCHEMA_UNEQUAL_INCOMPATIBLE) {
        LOGI("Local(Newer) compatible based on remote, result=%d.", errCode);
        return true;
    }
    LOGE("Local incompatible with remote mutually.");
    return false;
}

void SQLiteSingleVerNaturalStore::InitDataBaseOption(const KvDBProperties &kvDBProp, OpenDbProperties &option)
{
    std::string uri = GetDatabasePath(kvDBProp);
    bool isMemoryDb = kvDBProp.GetBoolProp(KvDBProperties::MEMORY_MODE, false);
    if (isMemoryDb) {
        std::string identifierDir = kvDBProp.GetStringProp(KvDBProperties::IDENTIFIER_DIR, "");
        uri = identifierDir + DBConstant::SQLITE_MEMDB_IDENTIFY;
        LOGD("Begin create memory natural store database");
    }
    std::string subDir = GetSubDirPath(kvDBProp);
    CipherType cipherType;
    CipherPassword passwd;
    kvDBProp.GetPassword(cipherType, passwd);
    std::string schemaStr = kvDBProp.GetSchema().ToSchemaString();

    bool isCreateNecessary = kvDBProp.GetBoolProp(KvDBProperties::CREATE_IF_NECESSARY, true);
    std::vector<std::string> createTableSqls;

    SecurityOption securityOpt;
    if (RuntimeContext::GetInstance()->IsProcessSystemApiAdapterValid()) {
        securityOpt.securityLabel = kvDBProp.GetSecLabel();
        securityOpt.securityFlag = kvDBProp.GetSecFlag();
    }

    option = {uri, isCreateNecessary, isMemoryDb, createTableSqls, cipherType, passwd, schemaStr, subDir, securityOpt};
    option.conflictReslovePolicy = kvDBProp.GetIntProp(KvDBProperties::CONFLICT_RESOLVE_POLICY, DEFAULT_LAST_WIN);
    option.createDirByStoreIdOnly = kvDBProp.GetBoolProp(KvDBProperties::CREATE_DIR_BY_STORE_ID_ONLY, false);
}

int SQLiteSingleVerNaturalStore::TransObserverTypeToRegisterFunctionType(
    int observerType, RegisterFuncType &type) const
{
    static constexpr TransPair transMap[] = {
        { static_cast<int>(SQLITE_GENERAL_NS_PUT_EVENT),       OBSERVER_SINGLE_VERSION_NS_PUT_EVENT },
        { static_cast<int>(SQLITE_GENERAL_NS_SYNC_EVENT),      OBSERVER_SINGLE_VERSION_NS_SYNC_EVENT },
        { static_cast<int>(SQLITE_GENERAL_NS_LOCAL_PUT_EVENT), OBSERVER_SINGLE_VERSION_NS_LOCAL_EVENT },
        { static_cast<int>(SQLITE_GENERAL_CONFLICT_EVENT),     OBSERVER_SINGLE_VERSION_NS_CONFLICT_EVENT },
    };
    auto funcType = GetFuncType(observerType, transMap, sizeof(transMap) / sizeof(TransPair));
    if (funcType == REGISTER_FUNC_TYPE_MAX) {
        return -E_NOT_SUPPORT;
    }
    type = funcType;
    return E_OK;
}

int SQLiteSingleVerNaturalStore::TransConflictTypeToRegisterFunctionType(
    int conflictType, RegisterFuncType &type) const
{
    static constexpr TransPair transMap[] = {
        { static_cast<int>(SQLITE_GENERAL_NS_FOREIGN_KEY_ONLY), CONFLICT_SINGLE_VERSION_NS_FOREIGN_KEY_ONLY },
        { static_cast<int>(SQLITE_GENERAL_NS_FOREIGN_KEY_ORIG), CONFLICT_SINGLE_VERSION_NS_FOREIGN_KEY_ORIG },
        { static_cast<int>(SQLITE_GENERAL_NS_NATIVE_ALL),       CONFLICT_SINGLE_VERSION_NS_NATIVE_ALL },
    };
    auto funcType = GetFuncType(conflictType, transMap, sizeof(transMap) / sizeof(TransPair));
    if (funcType == REGISTER_FUNC_TYPE_MAX) {
        return -E_NOT_SUPPORT;
    }
    type = funcType;
    return E_OK;
}

RegisterFuncType SQLiteSingleVerNaturalStore::GetFuncType(int index, const TransPair *transMap, int32_t len)
{
    int32_t head = 0;
    int32_t end = len - 1;
    while (head <= end) {
        int32_t mid = (head + end) / 2;
        if (transMap[mid].index < index) {
            head = mid + 1;
            continue;
        }
        if (transMap[mid].index > index) {
            end = mid - 1;
            continue;
        }
        return transMap[mid].funcType;
    }
    return REGISTER_FUNC_TYPE_MAX;
}

int SQLiteSingleVerNaturalStore::GetSchema(SchemaObject &schema) const
{
    int errCode = E_OK;
    auto handle = GetHandle(true, errCode); // Only open kvdb use, no competition for write handle
    if (handle == nullptr) {
        return errCode;
    }

    Timestamp timestamp;
    std::string schemaKey = DBConstant::SCHEMA_KEY;
    Key key(schemaKey.begin(), schemaKey.end());
    Value value;
    errCode = handle->GetKvData(SingleVerDataType::META_TYPE, key, value, timestamp);
    if (errCode == E_OK) {
        std::string schemaValue(value.begin(), value.end());
        errCode = schema.ParseFromSchemaString(schemaValue);
    } else {
        LOGI("[SqlSinStore] Get schema error:%d.", errCode);
    }
    ReleaseHandle(handle);
    return errCode;
}

int SQLiteSingleVerNaturalStore::DecideReadOnlyBaseOnSchema(const KvDBProperties &kvDBProp, bool &isReadOnly,
    SchemaObject &savedSchemaObj) const
{
    // Check whether it is a memory db
    if (kvDBProp.GetBoolProp(KvDBProperties::MEMORY_MODE, false)) {
        isReadOnly = false;
        return E_OK;
    }
    SchemaObject inputSchemaObj = kvDBProp.GetSchema();
    if (!inputSchemaObj.IsSchemaValid()) {
        int errCode = GetSchema(savedSchemaObj);
        if (errCode != E_OK && errCode != -E_NOT_FOUND) {
            LOGE("[SqlSinStore][DecideReadOnly] GetSchema fail=%d.", errCode);
            return errCode;
        }
        if (savedSchemaObj.IsSchemaValid()) {
            isReadOnly = true;
            return E_OK;
        }
    }
    // An valid schema will not lead to readonly
    isReadOnly = false;
    return E_OK;
}

void SQLiteSingleVerNaturalStore::InitialLocalDataTimestamp()
{
    Timestamp timestamp = GetCurrentTimestamp();

    int errCode = E_OK;
    auto handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return;
    }

    errCode = handle->UpdateLocalDataTimestamp(timestamp);
    if (errCode != E_OK) {
        LOGE("Update the timestamp for local data failed:%d", errCode);
    }
    ReleaseHandle(handle);
}

const KvDBProperties &SQLiteSingleVerNaturalStore::GetDbProperties() const
{
    return GetMyProperties();
}

int SQLiteSingleVerNaturalStore::RemoveKvDB(const KvDBProperties &properties)
{
    // To avoid leakage, the engine resources are forced to be released
    const std::string identifier = properties.GetStringProp(KvDBProperties::IDENTIFIER_DATA, "");
    (void)StorageEngineManager::ForceReleaseStorageEngine(identifier);

    // Only care the data directory and the db name.
    std::string storeOnlyDir;
    std::string storeDir;
    GenericKvDB::GetStoreDirectory(properties, KvDBProperties::SINGLE_VER_TYPE, storeDir, storeOnlyDir);

    const std::vector<std::pair<const std::string &, const std::string &>> dbDir {
        {DBConstant::MAINDB_DIR, DBConstant::SINGLE_VER_DATA_STORE},
        {DBConstant::METADB_DIR, DBConstant::SINGLE_VER_META_STORE},
        {DBConstant::CACHEDB_DIR, DBConstant::SINGLE_VER_CACHE_STORE}};

    bool isAllNotFound = true;
    for (const auto &item : dbDir) {
        std::string currentDir = storeDir + item.first + "/";
        std::string currentOnlyDir = storeOnlyDir + item.first + "/";
        int errCode = KvDBUtils::RemoveKvDB(currentDir, currentOnlyDir, item.second);
        if (errCode != -E_NOT_FOUND) {
            if (errCode != E_OK) {
                return errCode;
            }
            isAllNotFound = false;
        }
    };
    if (isAllNotFound) {
        return -E_NOT_FOUND;
    }

    int errCode = DBCommon::RemoveAllFilesOfDirectory(storeDir, true);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = DBCommon::RemoveAllFilesOfDirectory(storeOnlyDir, true);
    if (errCode != E_OK) {
        return errCode;
    }
    return errCode;
}

int SQLiteSingleVerNaturalStore::GetKvDBSize(const KvDBProperties &properties, uint64_t &size) const
{
    std::string storeOnlyIdentDir;
    std::string storeIdentDir;
    GenericKvDB::GetStoreDirectory(properties, KvDBProperties::SINGLE_VER_TYPE, storeIdentDir, storeOnlyIdentDir);
    const std::vector<std::pair<const std::string &, const std::string &>> dbDir {
        {DBConstant::MAINDB_DIR, DBConstant::SINGLE_VER_DATA_STORE},
        {DBConstant::METADB_DIR, DBConstant::SINGLE_VER_META_STORE},
        {DBConstant::CACHEDB_DIR, DBConstant::SINGLE_VER_CACHE_STORE}};
    int errCode = -E_NOT_FOUND;
    for (const auto &item : dbDir) {
        std::string storeDir = storeIdentDir + item.first;
        std::string storeOnlyDir = storeOnlyIdentDir + item.first;
        int err = KvDBUtils::GetKvDbSize(storeDir, storeOnlyDir, item.second, size);
        if (err != -E_NOT_FOUND && err != E_OK) {
            return err;
        }
        if (err == E_OK) {
            errCode = E_OK;
        }
    }
    return errCode;
}

KvDBProperties &SQLiteSingleVerNaturalStore::GetDbPropertyForUpdate()
{
    return MyProp();
}

void SQLiteSingleVerNaturalStore::HeartBeatForLifeCycle() const
{
    std::lock_guard<std::mutex> lock(lifeCycleMutex_);
    int errCode = ResetLifeCycleTimer();
    if (errCode != E_OK) {
        LOGE("Heart beat for life cycle failed:%d", errCode);
    }
}

int SQLiteSingleVerNaturalStore::StartLifeCycleTimer(const DatabaseLifeCycleNotifier &notifier) const
{
    auto runtimeCxt = RuntimeContext::GetInstance();
    if (runtimeCxt == nullptr) {
        return -E_INVALID_ARGS;
    }
    RefObject::IncObjRef(this);
    TimerId timerId = 0;
    int errCode = runtimeCxt->SetTimer(autoLifeTime_,
        [this](TimerId id) -> int {
            std::lock_guard<std::mutex> lock(lifeCycleMutex_);
            if (lifeCycleNotifier_) {
                std::string identifier;
                if (GetMyProperties().GetBoolProp(KvDBProperties::SYNC_DUAL_TUPLE_MODE, false)) {
                    identifier = GetMyProperties().GetStringProp(KvDBProperties::DUAL_TUPLE_IDENTIFIER_DATA, "");
                } else {
                    identifier = GetMyProperties().GetStringProp(KvDBProperties::IDENTIFIER_DATA, "");
                }
                auto userId = GetMyProperties().GetStringProp(DBProperties::USER_ID, "");
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

int SQLiteSingleVerNaturalStore::ResetLifeCycleTimer() const
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

int SQLiteSingleVerNaturalStore::StopLifeCycleTimer() const
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

bool SQLiteSingleVerNaturalStore::IsDataMigrating() const
{
    if (storageEngine_ == nullptr) {
        return false;
    }

    if (storageEngine_->IsMigrating()) {
        LOGD("Migrating now.");
        return true;
    }
    return false;
}

void SQLiteSingleVerNaturalStore::SetConnectionFlag(bool isExisted) const
{
    if (storageEngine_ != nullptr) {
        storageEngine_->SetConnectionFlag(isExisted);
    }
}

int SQLiteSingleVerNaturalStore::TriggerToMigrateData() const
{
    RefObject::IncObjRef(this);
    int errCode = RuntimeContext::GetInstance()->ScheduleTask(
        std::bind(&SQLiteSingleVerNaturalStore::AsyncDataMigration, this));
    if (errCode != E_OK) {
        RefObject::DecObjRef(this);
        LOGE("[SingleVerNStore] Trigger to migrate data failed : %d.", errCode);
    }
    return errCode;
}

bool SQLiteSingleVerNaturalStore::IsCacheDBMode() const
{
    if (storageEngine_ == nullptr) {
        LOGE("[SingleVerNStore] IsCacheDBMode storage engine is invalid.");
        return false;
    }
    EngineState engineState = storageEngine_->GetEngineState();
    return (engineState == CACHEDB);
}

bool SQLiteSingleVerNaturalStore::IsExtendedCacheDBMode() const
{
    if (storageEngine_ == nullptr) {
        LOGE("[SingleVerNStore] storage engine is invalid.");
        return false;
    }
    EngineState engineState = storageEngine_->GetEngineState();
    return (engineState == CACHEDB || engineState == MIGRATING || engineState == ATTACHING);
}

int SQLiteSingleVerNaturalStore::CheckReadDataControlled() const
{
    if (IsExtendedCacheDBMode()) {
        int err = IsCacheDBMode() ? -E_EKEYREVOKED : -E_BUSY;
        LOGE("Existed cache database can not read data, errCode = [%d]!", err);
        return err;
    }
    return E_OK;
}

void SQLiteSingleVerNaturalStore::IncreaseCacheRecordVersion() const
{
    if (storageEngine_ == nullptr) {
        LOGE("[SingleVerNStore] Increase cache version storage engine is invalid.");
        return;
    }
    storageEngine_->IncreaseCacheRecordVersion();
}

uint64_t SQLiteSingleVerNaturalStore::GetCacheRecordVersion() const
{
    if (storageEngine_ == nullptr) {
        LOGE("[SingleVerNStore] Get cache version storage engine is invalid.");
        return 0;
    }
    return storageEngine_->GetCacheRecordVersion();
}

uint64_t SQLiteSingleVerNaturalStore::GetAndIncreaseCacheRecordVersion() const
{
    if (storageEngine_ == nullptr) {
        LOGE("[SingleVerNStore] Get cache version storage engine is invalid.");
        return 0;
    }
    return storageEngine_->GetAndIncreaseCacheRecordVersion();
}

void SQLiteSingleVerNaturalStore::AsyncDataMigration() const
{
    // Delay a little time to ensure the completion of the delegate callback
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_DELEGATE_CALLBACK_TIME));
    bool isLocked = RuntimeContext::GetInstance()->IsAccessControlled();
    if (!isLocked) {
        LOGI("Begin to migrate cache data to manDb asynchronously!");
        (void)StorageEngineManager::ExecuteMigration(storageEngine_);
    }

    RefObject::DecObjRef(this);
}

void SQLiteSingleVerNaturalStore::CheckAmendValueContentForSyncProcedure(std::vector<DataItem> &dataItems) const
{
    const SchemaObject &schemaObjRef = MyProp().GetSchemaConstRef();
    if (!schemaObjRef.IsSchemaValid()) {
        // Not a schema database, do not need to check more
        return;
    }
    uint32_t deleteCount = 0;
    uint32_t amendCount = 0;
    uint32_t neglectCount = 0;
    for (auto &eachItem : dataItems) {
        if ((eachItem.flag & DataItem::DELETE_FLAG) == DataItem::DELETE_FLAG ||
            (eachItem.flag & DataItem::REMOTE_DEVICE_DATA_MISS_QUERY) == DataItem::REMOTE_DEVICE_DATA_MISS_QUERY) {
            // Delete record not concerned
            deleteCount++;
            continue;
        }
        bool useAmendValue = false;
        int errCode = CheckValueAndAmendIfNeed(ValueSource::FROM_SYNC, eachItem.value, eachItem.value, useAmendValue);
        if (errCode != E_OK) {
            eachItem.neglect = true;
            neglectCount++;
            continue;
        }
        if (useAmendValue) {
            amendCount++;
        }
    }
    LOGI("[SqlSinStore][CheckAmendForSync] OriCount=%zu, DeleteCount=%u, AmendCount=%u, NeglectCount=%u",
        dataItems.size(), deleteCount, amendCount, neglectCount);
}

int SQLiteSingleVerNaturalStore::SaveSyncItemsInCacheMode(SQLiteSingleVerStorageExecutor *handle,
    const QueryObject &query, std::vector<DataItem> &dataItems, const DeviceInfo &deviceInfo,
    Timestamp &maxTimestamp) const
{
    int errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        return errCode;
    }

    int innerCode;
    const uint64_t recordVersion = GetCacheRecordVersion();
    errCode = handle->PrepareForSavingCacheData(SingleVerDataType::SYNC_TYPE);
    if (errCode != E_OK) {
        goto END;
    }

    for (auto &item : dataItems) {
        errCode = handle->SaveSyncDataItemInCacheMode(item, deviceInfo, maxTimestamp, recordVersion, query);
        if (errCode != E_OK && errCode != -E_NOT_FOUND) {
            break;
        }
    }

    if (errCode == -E_NOT_FOUND) {
        errCode = E_OK;
    }

    innerCode = handle->ResetForSavingCacheData(SingleVerDataType::SYNC_TYPE);
    if (innerCode != E_OK) {
        errCode = innerCode;
    }
END:
    if (errCode == E_OK) {
        errCode = handle->Commit();
        storageEngine_->IncreaseCacheRecordVersion();
    } else {
        (void)handle->Rollback(); // Keep the error code of the first scene
    }
    return errCode;
}

void SQLiteSingleVerNaturalStore::NotifyRemotePushFinished(const std::string &targetId) const
{
    std::string identifier = DBCommon::VectorToHexString(GetIdentifier());
    LOGI("label:%s sourceTarget: %s{private} push finished", identifier.c_str(), targetId.c_str());
    NotifyRemotePushFinishedInner(targetId);
}

int SQLiteSingleVerNaturalStore::GetDatabaseCreateTimestamp(Timestamp &outTime) const
{
    // Found in memory.
    {
        std::lock_guard<std::mutex> autoLock(createDBTimeMutex_);
        if (createDBTime_ != 0) {
            outTime = createDBTime_;
            return E_OK;
        }
    }

    const Key key(CREATE_DB_TIME.begin(), CREATE_DB_TIME.end());
    Value value;
    int errCode = GetMetaData(key, value);
    if (errCode != E_OK) {
        LOGD("GetDatabaseCreateTimestamp failed, errCode = %d.", errCode);
        return errCode;
    }

    Timestamp createDBTime = 0;
    Parcel parcel(value.data(), value.size());
    (void)parcel.ReadUInt64(createDBTime);
    if (parcel.IsError()) {
        return -E_INVALID_ARGS;
    }
    outTime = createDBTime;
    std::lock_guard<std::mutex> autoLock(createDBTimeMutex_);
    createDBTime_ = createDBTime;
    return E_OK;
}

int SQLiteSingleVerNaturalStore::CheckIntegrity() const
{
    int errCode = E_OK;
    auto handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->CheckIntegrity();
    ReleaseHandle(handle);
    return errCode;
}

int SQLiteSingleVerNaturalStore::SaveCreateDBTime()
{
    Timestamp createDBTime = GetCurrentTimestamp();
    const Key key(CREATE_DB_TIME.begin(), CREATE_DB_TIME.end());
    Value value(Parcel::GetUInt64Len());
    Parcel parcel(value.data(), Parcel::GetUInt64Len());
    (void)parcel.WriteUInt64(createDBTime);
    if (parcel.IsError()) {
        LOGE("SaveCreateDBTime failed, something wrong in parcel.");
        return -E_PARSE_FAIL;
    }

    int errCode = PutMetaData(key, value);
    if (errCode != E_OK) {
        LOGE("SaveCreateDBTime failed, errCode = %d", errCode);
        return errCode;
    }

    // save in memory.
    std::lock_guard<std::mutex> autoLock(createDBTimeMutex_);
    createDBTime_ = createDBTime;
    return errCode;
}

int SQLiteSingleVerNaturalStore::SaveCreateDBTimeIfNotExisted()
{
    Timestamp createDBTime = 0;
    int errCode = GetDatabaseCreateTimestamp(createDBTime);
    if (errCode == -E_NOT_FOUND) {
        errCode = SaveCreateDBTime();
    }
    if (errCode != E_OK) {
        LOGE("SaveCreateDBTimeIfNotExisted failed, errCode=%d.", errCode);
    }
    return errCode;
}

int SQLiteSingleVerNaturalStore::DeleteMetaDataByPrefixKey(const Key &keyPrefix) const
{
    if (keyPrefix.empty() || keyPrefix.size() > DBConstant::MAX_KEY_SIZE) {
        return -E_INVALID_ARGS;
    }

    int errCode = E_OK;
    auto handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->DeleteMetaDataByPrefixKey(keyPrefix);
    if (errCode != E_OK) {
        LOGE("[SinStore] DeleteMetaData by prefix key failed, errCode = %d", errCode);
    }

    ReleaseHandle(handle);
    HeartBeatForLifeCycle();
    return errCode;
}

int SQLiteSingleVerNaturalStore::GetCompressionOption(bool &needCompressOnSync, uint8_t &compressionRate) const
{
    needCompressOnSync = GetDbProperties().GetBoolProp(KvDBProperties::COMPRESS_ON_SYNC, false);
    compressionRate = GetDbProperties().GetIntProp(KvDBProperties::COMPRESSION_RATE,
        DBConstant::DEFAULT_COMPTRESS_RATE);
    return E_OK;
}

int SQLiteSingleVerNaturalStore::GetCompressionAlgo(std::set<CompressAlgorithm> &algorithmSet) const
{
    algorithmSet.clear();
    DataCompression::GetCompressionAlgo(algorithmSet);
    return E_OK;
}

int SQLiteSingleVerNaturalStore::CheckAndInitQueryCondition(QueryObject &query) const
{
    const SchemaObject &localSchema = MyProp().GetSchemaConstRef();
    if (localSchema.GetSchemaType() != SchemaType::NONE && localSchema.GetSchemaType() != SchemaType::JSON) {
        // Flatbuffer schema is not support subscribe
        return -E_NOT_SUPPORT;
    }
    query.SetSchema(localSchema);

    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(false, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->CheckQueryObjectLegal(query);
    if (errCode != E_OK) {
        LOGE("Check query condition failed [%d]!", errCode);
    }
    ReleaseHandle(handle);
    return errCode;
}

void SQLiteSingleVerNaturalStore::SetDataInterceptor(const PushDataInterceptor &interceptor)
{
    std::unique_lock<std::shared_mutex> lock(dataInterceptorMutex_);
    dataInterceptor_ = interceptor;
}

int SQLiteSingleVerNaturalStore::InterceptData(std::vector<SingleVerKvEntry *> &entries, const std::string &sourceID,
    const std::string &targetID) const
{
    PushDataInterceptor interceptor = nullptr;
    {
        std::shared_lock<std::shared_mutex> lock(dataInterceptorMutex_);
        if (dataInterceptor_ == nullptr) {
            return E_OK;
        }
        interceptor = dataInterceptor_;
    }

    InterceptedDataImpl data(entries, [this](const Value &newValue) -> int {
            bool useAmendValue = false;
            Value amendValue = newValue;
            return this->CheckValueAndAmendIfNeed(ValueSource::FROM_LOCAL, newValue, amendValue, useAmendValue);
        }
    );

    int errCode = interceptor(data, sourceID, targetID);
    if (data.IsError()) {
        SingleVerKvEntry::Release(entries);
        LOGE("Intercept data failed:%d.", errCode);
        return -E_INTERCEPT_DATA_FAIL;
    }
    return E_OK;
}

int SQLiteSingleVerNaturalStore::AddSubscribe(const std::string &subscribeId, const QueryObject &query,
    bool needCacheSubscribe)
{
    const SchemaObject &localSchema = MyProp().GetSchemaConstRef();
    if (localSchema.GetSchemaType() != SchemaType::NONE && localSchema.GetSchemaType() != SchemaType::JSON) {
        // Flatbuffer schema is not support subscribe
        return -E_NOT_SUPPORT;
    }
    QueryObject queryInner = query;
    queryInner.SetSchema(localSchema);
    if (IsExtendedCacheDBMode() && needCacheSubscribe) { // cache auto subscribe when engine state is in CACHEDB mode
        LOGI("Cache subscribe query and return ok when in cacheDB.");
        storageEngine_->CacheSubscribe(subscribeId, queryInner);
        return E_OK;
    }

    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        ReleaseHandle(handle);
        return errCode;
    }

    errCode = handle->AddSubscribeTrigger(queryInner, subscribeId);
    if (errCode != E_OK) {
        LOGE("Add subscribe trigger failed: %d", errCode);
        (void)handle->Rollback();
    } else {
        errCode = handle->Commit();
    }
    ReleaseHandle(handle);
    return errCode;
}

int SQLiteSingleVerNaturalStore::RemoveSubscribe(const std::vector<std::string> &subscribeIds)
{
    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        ReleaseHandle(handle);
        return errCode;
    }
    errCode = handle->RemoveSubscribeTrigger(subscribeIds);
    if (errCode != E_OK) {
        LOGE("Remove subscribe trigger failed: %d", errCode);
        goto ERR;
    }
    errCode = handle->RemoveSubscribeTriggerWaterMark(subscribeIds);
    if (errCode != E_OK) {
        LOGE("Remove subscribe data water mark failed: %d", errCode);
    }
ERR:
    if (errCode == E_OK) {
        errCode = handle->Commit();
    } else {
        (void)handle->Rollback();
    }
    ReleaseHandle(handle);
    return errCode;
}

int SQLiteSingleVerNaturalStore::RemoveSubscribe(const std::string &subscribeId)
{
    return RemoveSubscribe(std::vector<std::string> {subscribeId});
}

int SQLiteSingleVerNaturalStore::SetMaxLogSize(uint64_t limit)
{
    LOGI("Set the max log size to %" PRIu64, limit);
    maxLogSize_.store(limit);
    return E_OK;
}
uint64_t SQLiteSingleVerNaturalStore::GetMaxLogSize() const
{
    return maxLogSize_.load();
}

int SQLiteSingleVerNaturalStore::RemoveAllSubscribe()
{
    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }
    std::vector<std::string> triggers;
    errCode = handle->GetTriggers(DBConstant::SUBSCRIBE_QUERY_PREFIX, triggers);
    if (errCode != E_OK) {
        LOGE("Get all subscribe triggers failed. %d", errCode);
        ReleaseHandle(handle);
        return errCode;
    }

    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        ReleaseHandle(handle);
        return errCode;
    }

    Key prefixKey;
    errCode = handle->RemoveTrigger(triggers);
    if (errCode != E_OK) {
        LOGE("remove all subscribe triggers failed. %d", errCode);
        goto END;
    }

    DBCommon::StringToVector(DBConstant::SUBSCRIBE_QUERY_PREFIX, prefixKey);
    errCode = handle->DeleteMetaDataByPrefixKey(prefixKey);
    if (errCode != E_OK) {
        LOGE("remove all subscribe water mark failed. %d", errCode);
    }
END:
    if (errCode == E_OK) {
        errCode = handle->Commit();
    } else {
        (void)handle->Rollback();
    }
    ReleaseHandle(handle);
    return errCode;
}

DEFINE_OBJECT_TAG_FACILITIES(SQLiteSingleVerNaturalStore)
}
