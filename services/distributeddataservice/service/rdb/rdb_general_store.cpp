/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "RdbGeneralStore"

#include "rdb_general_store.h"

#include <chrono>
#include <cinttypes>

#include "cache_cursor.h"
#include "changeevent/remote_change_event.h"
#include "cloud/asset_loader.h"
#include "cloud/cloud_db.h"
#include "cloud/cloud_store_types.h"
#include "cloud/schema_meta.h"
#include "cloud_service.h"
#include "commonevent/data_sync_event.h"
#include "communicator/device_manager_adapter.h"
#include "crypto_manager.h"
#include "dfx_types.h"
#include "device_manager_adapter.h"
#include "eventcenter/event_center.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "rdb_cursor.h"
#include "rdb_helper.h"
#include "rdb_query.h"
#include "rdb_result_set_impl.h"
#include "relational_store_manager.h"
#include "reporter.h"
#include "snapshot/bind_event.h"
#include "utils/anonymous.h"
#include "value_proxy.h"
namespace OHOS::DistributedRdb {
using namespace DistributedData;
using namespace DistributedDB;
using namespace NativeRdb;
using namespace CloudData;
using namespace std::chrono;
using namespace DistributedDataDfx;
using DBField = DistributedDB::Field;
using DBTable = DistributedDB::TableSchema;
using DBSchema = DistributedDB::DataBaseSchema;
using ClearMode = DistributedDB::ClearMode;
using DBStatus = DistributedDB::DBStatus;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;

constexpr const char *INSERT = "INSERT INTO ";
constexpr const char *REPLACE = "REPLACE INTO ";
constexpr const char *VALUES = " VALUES ";
constexpr const char *LOGOUT_DELETE_FLAG = "DELETE#ALL_CLOUDDATA";
constexpr const LockAction LOCK_ACTION =
    static_cast<LockAction>(static_cast<uint32_t>(LockAction::INSERT) | static_cast<uint32_t>(LockAction::UPDATE) |
                            static_cast<uint32_t>(LockAction::DELETE) | static_cast<uint32_t>(LockAction::DOWNLOAD));
constexpr uint32_t CLOUD_SYNC_FLAG = 1;
constexpr uint32_t SEARCHABLE_FLAG = 2;
constexpr uint32_t LOCK_TIMEOUT = 3600; // second
static constexpr const char *FT_OPEN_STORE = "OPEN_STORE";
static constexpr const char *FT_CLOUD_SYNC = "CLOUD_SYNC";

static DBSchema GetDBSchema(const Database &database)
{
    DBSchema schema;
    schema.tables.resize(database.tables.size());
    for (size_t i = 0; i < database.tables.size(); i++) {
        const Table &table = database.tables[i];
        DBTable &dbTable = schema.tables[i];
        dbTable.name = table.name;
        dbTable.sharedTableName = table.sharedTableName;
        for (auto &field : table.fields) {
            DBField dbField;
            dbField.colName = field.colName;
            dbField.type = field.type;
            dbField.primary = field.primary;
            dbField.nullable = field.nullable;
            dbTable.fields.push_back(std::move(dbField));
        }
    }
    return schema;
}

static bool IsExistence(const std::string &col, const std::vector<std::string> &cols)
{
    for (auto &column : cols) {
        if (col == column) {
            return true;
        }
    }
    return false;
}

static DistributedSchema GetGaussDistributedSchema(const Database &database)
{
    DistributedSchema distributedSchema;
    distributedSchema.version = database.version;
    distributedSchema.tables.resize(database.tables.size());
    for (size_t i = 0; i < database.tables.size(); i++) {
        const Table &table = database.tables[i];
        DistributedTable &dbTable = distributedSchema.tables[i];
        dbTable.tableName = table.name;
        for (auto &field : table.fields) {
            DistributedField dbField;
            dbField.colName = field.colName;
            dbField.isP2pSync = IsExistence(field.colName, table.deviceSyncFields);
            dbTable.fields.push_back(std::move(dbField));
        }
    }
    return distributedSchema;
}

static std::pair<bool, Database> GetDistributedSchema(const StoreMetaData &meta)
{
    std::pair<bool, Database> tableData;
    Database database;
    database.bundleName = meta.bundleName;
    database.name = meta.storeId;
    database.user = meta.user;
    database.deviceId = meta.deviceId;
    tableData.first = MetaDataManager::GetInstance().LoadMeta(database.GetKey(), database, true);
    tableData.second = database;
    return tableData;
}

void RdbGeneralStore::InitStoreInfo(const StoreMetaData &meta)
{
    storeInfo_.tokenId = meta.tokenId;
    storeInfo_.bundleName = meta.bundleName;
    storeInfo_.storeName = meta.storeId;
    storeInfo_.instanceId = meta.instanceId;
    storeInfo_.user = std::stoi(meta.user);
    storeInfo_.deviceId = DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
}

RelationalStoreDelegate::Option GetOption(const StoreMetaData &meta)
{
    RelationalStoreDelegate::Option option;
    option.syncDualTupleMode = true;
    if (GetDistributedSchema(meta).first) {
        option.tableMode = DistributedTableMode::COLLABORATION;
    }
    return option;
}

RdbGeneralStore::RdbGeneralStore(const StoreMetaData &meta)
    : manager_(meta.appId, meta.user, meta.instanceId), tasks_(std::make_shared<ConcurrentMap<SyncId, FinishTask>>())
{
    observer_.storeId_ = meta.storeId;
    observer_.meta_ = meta;
    RelationalStoreDelegate::Option option = GetOption(meta);
    option.observer = &observer_;
    if (meta.isEncrypt) {
        std::string key = meta.GetSecretKey();
        SecretKeyMetaData secretKeyMeta;
        MetaDataManager::GetInstance().LoadMeta(key, secretKeyMeta, true);
        std::vector<uint8_t> decryptKey;
        CryptoManager::GetInstance().Decrypt(secretKeyMeta.sKey, decryptKey);
        option.passwd.SetValue(decryptKey.data(), decryptKey.size());
        std::fill(decryptKey.begin(), decryptKey.end(), 0);
        option.isEncryptedDb = meta.isEncrypt;
        option.cipher = CipherType::AES_256_GCM;
        for (uint32_t i = 0; i < ITERS_COUNT; ++i) {
            option.iterateTimes = ITERS[i];
            auto ret = manager_.OpenStore(meta.dataDir, meta.storeId, option, delegate_);
            if (ret == DBStatus::OK && delegate_ != nullptr) {
                break;
            }
            manager_.CloseStore(delegate_);
            delegate_ = nullptr;
        }
    } else {
        auto ret = manager_.OpenStore(meta.dataDir, meta.storeId, option, delegate_);
        if (ret != DBStatus::OK || delegate_ == nullptr) {
            manager_.CloseStore(delegate_);
            delegate_ = nullptr;
        }
    }
    InitStoreInfo(meta);
    if (meta.isSearchable) {
        syncNotifyFlag_ |= SEARCHABLE_FLAG;
    }
    if (delegate_ != nullptr && meta.isManualClean) {
        PragmaData data = static_cast<PragmaData>(const_cast<void *>(static_cast<const void *>(&meta.isManualClean)));
        delegate_->Pragma(PragmaCmd::LOGIC_DELETE_SYNC_DATA, data);
    }
    std::pair<bool, Database> tableData = GetDistributedSchema(meta);
    if (delegate_ != nullptr && tableData.first) {
        delegate_->SetDistributedSchema(GetGaussDistributedSchema(tableData.second));
    }
    ZLOGI("Get rdb store, tokenId:%{public}u, bundleName:%{public}s, storeName:%{public}s, user:%{public}s,"
          "isEncrypt:%{public}d, isManualClean:%{public}d, isSearchable:%{public}d",
          meta.tokenId, meta.bundleName.c_str(), Anonymous::Change(meta.storeId).c_str(), meta.user.c_str(),
          meta.isEncrypt, meta.isManualClean, meta.isSearchable);
}

RdbGeneralStore::~RdbGeneralStore()
{
    manager_.CloseStore(delegate_);
    delegate_ = nullptr;
    bindInfo_.loader_ = nullptr;
    if (bindInfo_.db_ != nullptr) {
        bindInfo_.db_->Close();
    }
    bindInfo_.db_ = nullptr;
    rdbCloud_ = nullptr;
    rdbLoader_ = nullptr;
    RemoveTasks();
    tasks_ = nullptr;
    executor_ = nullptr;
}

int32_t RdbGeneralStore::BindSnapshots(std::shared_ptr<std::map<std::string, std::shared_ptr<Snapshot>>> bindAssets)
{
    if (snapshots_.bindAssets == nullptr) {
        snapshots_.bindAssets = bindAssets;
    }
    return GenErr::E_OK;
}

int32_t RdbGeneralStore::Bind(Database &database, const std::map<uint32_t, BindInfo> &bindInfos,
    const CloudConfig &config)
{
    if (bindInfos.empty()) {
        return GeneralError::E_OK;
    }
    auto bindInfo = bindInfos.begin()->second;
    if (bindInfo.db_ == nullptr || bindInfo.loader_ == nullptr) {
        return GeneralError::E_INVALID_ARGS;
    }

    if (isBound_.exchange(true)) {
        return GeneralError::E_OK;
    }

    BindEvent::BindEventInfo eventInfo;
    eventInfo.tokenId = storeInfo_.tokenId;
    eventInfo.bundleName = storeInfo_.bundleName;
    eventInfo.storeName = storeInfo_.storeName;
    eventInfo.user = storeInfo_.user;
    eventInfo.instanceId = storeInfo_.instanceId;

    auto evt = std::make_unique<BindEvent>(BindEvent::BIND_SNAPSHOT, std::move(eventInfo));
    EventCenter::GetInstance().PostEvent(std::move(evt));
    bindInfo_ = std::move(bindInfo);
    {
        std::unique_lock<decltype(rdbCloudMutex_)> lock(rdbCloudMutex_);
        rdbCloud_ = std::make_shared<RdbCloud>(bindInfo_.db_, &snapshots_);
        rdbLoader_ = std::make_shared<RdbAssetLoader>(bindInfo_.loader_, &snapshots_);
    }

    DistributedDB::CloudSyncConfig dbConfig;
    dbConfig.maxUploadCount = config.maxNumber;
    dbConfig.maxUploadSize = config.maxSize;
    dbConfig.maxRetryConflictTimes = config.maxRetryConflictTimes;
    DBSchema schema = GetDBSchema(database);
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("database:%{public}s already closed!", Anonymous::Change(database.name).c_str());
        return GeneralError::E_ALREADY_CLOSED;
    }
    delegate_->SetCloudDB(rdbCloud_);
    delegate_->SetIAssetLoader(rdbLoader_);
    delegate_->SetCloudDbSchema(std::move(schema));
    delegate_->SetCloudSyncConfig(dbConfig);

    syncNotifyFlag_ |= CLOUD_SYNC_FLAG;
    return GeneralError::E_OK;
}

bool RdbGeneralStore::IsBound(uint32_t user)
{
    return isBound_;
}

int32_t RdbGeneralStore::Close(bool isForce)
{
    {
        std::unique_lock<decltype(rwMutex_)> lock(rwMutex_, std::chrono::seconds(isForce ? LOCK_TIMEOUT : 0));
        if (!lock) {
            return GeneralError::E_BUSY;
        }

        if (delegate_ == nullptr) {
            return GeneralError::E_OK;
        }
        auto [dbStatus, downloadCount] = delegate_->GetDownloadingAssetsCount();
        if (!isForce && (delegate_->GetCloudSyncTaskCount() > 0 || downloadCount > 0)) {
            return GeneralError::E_BUSY;
        }
        auto status = manager_.CloseStore(delegate_);
        if (status != DBStatus::OK) {
            return status;
        }
        delegate_ = nullptr;
    }
    RemoveTasks();
    bindInfo_.loader_ = nullptr;
    if (bindInfo_.db_ != nullptr) {
        bindInfo_.db_->Close();
    }
    bindInfo_.db_ = nullptr;
    {
        std::unique_lock<decltype(rdbCloudMutex_)> lock(rdbCloudMutex_);
        rdbCloud_ = nullptr;
        rdbLoader_ = nullptr;
    }
    return GeneralError::E_OK;
}

int32_t RdbGeneralStore::Execute(const std::string &table, const std::string &sql)
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("Database already closed! database:%{public}s, table:%{public}s, sql:%{public}s",
            Anonymous::Change(storeInfo_.storeName).c_str(), Anonymous::Change(table).c_str(),
            Anonymous::Change(sql).c_str());
        return GeneralError::E_ERROR;
    }
    std::vector<DistributedDB::VBucket> changedData;
    auto status = delegate_->ExecuteSql({ sql, {}, false }, changedData);
    if (status != DBStatus::OK) {
        ZLOGE("Execute failed! ret:%{public}d, sql:%{public}s, data size:%{public}zu", status,
              Anonymous::Change(sql).c_str(), changedData.size());
        if (status == DBStatus::BUSY) {
            return GeneralError::E_BUSY;
        }
        return GeneralError::E_ERROR;
    }
    return GeneralError::E_OK;
}

size_t RdbGeneralStore::SqlConcatenate(VBucket &value, std::string &strColumnSql, std::string &strRowValueSql)
{
    strColumnSql += " (";
    strRowValueSql += " (";
    auto columnSize = value.size();
    for (auto column = value.begin(); column != value.end(); ++column) {
        strRowValueSql += " ?,";
        strColumnSql += " " + column->first + ",";
    }
    if (columnSize != 0) {
        strColumnSql.pop_back();
        strColumnSql += ")";
        strRowValueSql.pop_back();
        strRowValueSql += ")";
    }
    return columnSize;
}

int32_t RdbGeneralStore::Insert(const std::string &table, VBuckets &&values)
{
    if (table.empty() || values.size() == 0) {
        ZLOGE("Insert:table maybe empty:%{public}d,value size is:%{public}zu", table.empty(), values.size());
        return GeneralError::E_INVALID_ARGS;
    }

    std::string strColumnSql;
    std::string strRowValueSql;
    auto value = values.front();
    size_t columnSize = SqlConcatenate(value, strColumnSql, strRowValueSql);
    if (columnSize == 0) {
        ZLOGE("Insert: columnSize error, can't be 0!");
        return GeneralError::E_INVALID_ARGS;
    }

    Values args;
    std::string strValueSql;
    for (auto &rowData : values) {
        if (rowData.size() != columnSize) {
            ZLOGE("Insert: VBucket size error, Each VBucket in values must be of the same length!");
            return GeneralError::E_INVALID_ARGS;
        }
        for (auto column = rowData.begin(); column != rowData.end(); ++column) {
            args.push_back(std::move(column->second));
        }
        strValueSql += strRowValueSql + ",";
    }
    strValueSql.pop_back();
    std::string sql = INSERT + table + strColumnSql + VALUES + strValueSql;

    std::vector<DistributedDB::VBucket> changedData;
    std::vector<DistributedDB::Type> bindArgs = ValueProxy::Convert(std::move(args));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("Database already closed! database:%{public}s, table:%{public}s",
            Anonymous::Change(storeInfo_.storeName).c_str(), Anonymous::Change(table).c_str());
        return GeneralError::E_ERROR;
    }
    auto status = delegate_->ExecuteSql({ sql, std::move(bindArgs), false }, changedData);
    if (status != DBStatus::OK) {
        if (IsPrintLog(status)) {
            auto time =
                static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
                ZLOGE("Failed! ret:%{public}d, sql:%{public}s, data size:%{public}zu times %{public}" PRIu64 ".",
                status, Anonymous::Change(sql).c_str(), changedData.size(), time);
        }
        return GeneralError::E_ERROR;
    }
    return GeneralError::E_OK;
}

bool RdbGeneralStore::IsPrintLog(DBStatus status)
{
    bool isPrint = false;
    if (status == lastError_) {
        if (++lastErrCnt_ % PRINT_ERROR_CNT == 0) {
            isPrint = true;
        }
    } else {
        isPrint = true;
        lastErrCnt_ = 0;
        lastError_ = status;
    }
    return isPrint;
}

/**
 * This function does not support batch updates in rdb, it only supports single data updates.
 */
int32_t RdbGeneralStore::Update(const std::string &table, const std::string &setSql, Values &&values,
    const std::string &whereSql, Values &&conditions)
{
    if (table.empty()) {
        ZLOGE("Update: table can't be empty!");
        return GeneralError::E_INVALID_ARGS;
    }
    if (setSql.empty() || values.size() == 0) {
        ZLOGE("Update: setSql and values can't be empty!");
        return GeneralError::E_INVALID_ARGS;
    }
    if (whereSql.empty() || conditions.size() == 0) {
        ZLOGE("Update: whereSql and conditions can't be empty!");
        return GeneralError::E_INVALID_ARGS;
    }

    std::string sqlIn = " UPDATE " + table + " SET " + setSql + " WHERE " + whereSql;
    Values args;
    for (auto &value : values) {
        args.push_back(std::move(value));
    }
    for (auto &condition : conditions) {
        args.push_back(std::move(condition));
    }

    std::vector<DistributedDB::VBucket> changedData;
    std::vector<DistributedDB::Type> bindArgs = ValueProxy::Convert(std::move(args));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("Database already closed! database:%{public}s, table:%{public}s",
            Anonymous::Change(storeInfo_.storeName).c_str(), Anonymous::Change(table).c_str());
        return GeneralError::E_ERROR;
    }
    auto status = delegate_->ExecuteSql({ sqlIn, std::move(bindArgs), false }, changedData);
    if (status != DBStatus::OK) {
        ZLOGE("Failed! ret:%{public}d, sql:%{public}s, data size:%{public}zu", status, Anonymous::Change(sqlIn).c_str(),
              changedData.size());
        return GeneralError::E_ERROR;
    }
    return GeneralError::E_OK;
}

int32_t RdbGeneralStore::Replace(const std::string &table, VBucket &&value)
{
    if (table.empty() || value.size() == 0) {
        return GeneralError::E_INVALID_ARGS;
    }
    std::string columnSql;
    std::string valueSql;
    SqlConcatenate(value, columnSql, valueSql);
    std::string sql = REPLACE + table + columnSql + VALUES + valueSql;

    Values args;
    for (auto &[k, v] : value) {
        args.emplace_back(std::move(v));
    }
    std::vector<DistributedDB::VBucket> changedData;
    std::vector<DistributedDB::Type> bindArgs = ValueProxy::Convert(std::move(args));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("Database already closed! database:%{public}s, table:%{public}s",
            Anonymous::Change(storeInfo_.storeName).c_str(), Anonymous::Change(table).c_str());
        return GeneralError::E_ERROR;
    }
    auto status = delegate_->ExecuteSql({ sql, std::move(bindArgs) }, changedData);
    if (status != DBStatus::OK) {
        ZLOGE("Replace failed! ret:%{public}d, table:%{public}s, sql:%{public}s, fields:%{public}s",
            status, Anonymous::Change(table).c_str(), Anonymous::Change(sql).c_str(), columnSql.c_str());
        if (status == DBStatus::BUSY) {
            return GeneralError::E_BUSY;
        }
        return GeneralError::E_ERROR;
    }
    return GeneralError::E_OK;
}

int32_t RdbGeneralStore::Delete(const std::string &table, const std::string &sql, Values &&args)
{
    return 0;
}

std::pair<int32_t, std::shared_ptr<Cursor>> RdbGeneralStore::Query(__attribute__((unused))const std::string &table,
    const std::string &sql, Values &&args)
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("Database already closed! database:%{public}s", Anonymous::Change(storeInfo_.storeName).c_str());
        return { GeneralError::E_ALREADY_CLOSED, nullptr };
    }
    auto [errCode, records] = QuerySql(sql, std::move(args));
    return { errCode, std::make_shared<CacheCursor>(std::move(records)) };
}

std::pair<int32_t, std::shared_ptr<Cursor>> RdbGeneralStore::Query(const std::string &table, GenQuery &query)
{
    RdbQuery *rdbQuery = nullptr;
    auto ret = query.QueryInterface(rdbQuery);
    if (ret != GeneralError::E_OK || rdbQuery == nullptr) {
        ZLOGE("not RdbQuery!");
        return { GeneralError::E_INVALID_ARGS, nullptr };
    }
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("Database already closed! database:%{public}s, table:%{public}s",
            Anonymous::Change(storeInfo_.storeName).c_str(), Anonymous::Change(table).c_str());
        return { GeneralError::E_ALREADY_CLOSED, nullptr };
    }
    if (rdbQuery->IsRemoteQuery()) {
        if (rdbQuery->GetDevices().size() != 1) {
            ZLOGE("RemoteQuery: devices size error! size:%{public}zu", rdbQuery->GetDevices().size());
            return { GeneralError::E_ERROR, nullptr };
        }
        auto cursor = RemoteQuery(*rdbQuery->GetDevices().begin(), rdbQuery->GetRemoteCondition());
        return { cursor != nullptr ? GeneralError::E_OK : GeneralError::E_ERROR, cursor};
    }
    return { GeneralError::E_ERROR, nullptr };
}

int32_t RdbGeneralStore::MergeMigratedData(const std::string &tableName, VBuckets &&values)
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("Database already closed! database:%{public}s, table:%{public}s",
            Anonymous::Change(storeInfo_.storeName).c_str(), Anonymous::Change(tableName).c_str());
        return GeneralError::E_ERROR;
    }

    auto status = delegate_->UpsertData(tableName, ValueProxy::Convert(std::move(values)));
    return status == DistributedDB::OK ? GeneralError::E_OK : GeneralError::E_ERROR;
}

int32_t RdbGeneralStore::CleanTrackerData(const std::string &tableName, int64_t cursor)
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("Database already closed! database:%{public}s, table:%{public}s",
              Anonymous::Change(storeInfo_.storeName).c_str(), Anonymous::Change(tableName).c_str());
        return GeneralError::E_ERROR;
    }

    auto status = delegate_->CleanTrackerData(tableName, cursor);
    return status == DistributedDB::OK ? GeneralError::E_OK : GeneralError::E_ERROR;
}

std::pair<int32_t, int32_t> RdbGeneralStore::DoCloudSync(const Devices &devices, const DistributedDB::Query &dbQuery,
    const SyncParam &syncParam, bool isPriority, DetailAsync async)
{
    auto syncMode = GeneralStore::GetSyncMode(syncParam.mode);
    auto highMode = GetHighMode(static_cast<uint32_t>(syncParam.mode));
    SyncId syncId = ++syncTaskId_;
    auto callback = GetDBProcessCB(std::move(async), syncMode, syncId, highMode);
    if (executor_ != nullptr && tasks_ != nullptr) {
        auto id = executor_->Schedule(std::chrono::minutes(INTERVAL), GetFinishTask(syncId));
        tasks_->Insert(syncId, { id, callback });
    }
    CloudSyncOption option;
    option.devices = devices;
    option.mode = DistributedDB::SyncMode(syncMode);
    option.query = dbQuery;
    option.waitTime = syncParam.wait;
    option.priorityTask = isPriority || highMode == MANUAL_SYNC_MODE;
    option.priorityLevel = GetPriorityLevel(highMode);
    option.compensatedSyncOnly = syncParam.isCompensation;
    option.merge = highMode == AUTO_SYNC_MODE;
    option.lockAction = LOCK_ACTION;
    option.prepareTraceId = syncParam.prepareTraceId;
    option.asyncDownloadAssets = syncParam.asyncDownloadAsset;
    auto dbStatus = DistributedDB::INVALID_ARGS;
    dbStatus = delegate_->Sync(option, tasks_ != nullptr ? GetCB(syncId) : callback, syncId);
    if (dbStatus == DBStatus::OK || tasks_ == nullptr) {
        return { ConvertStatus(dbStatus), dbStatus };
    }
    Report(FT_CLOUD_SYNC, static_cast<int32_t>(Fault::CSF_GS_CLOUD_SYNC),
        "Cloud sync ret=" + std::to_string(static_cast<int32_t>(dbStatus)));
    tasks_->ComputeIfPresent(syncId, [executor = executor_](SyncId syncId, const FinishTask &task) {
        if (executor != nullptr) {
            executor->Remove(task.taskId);
        }
        return false;
    });
    return { ConvertStatus(dbStatus), dbStatus };
}

std::pair<int32_t, int32_t> RdbGeneralStore::Sync(const Devices &devices, GenQuery &query, DetailAsync async,
    const SyncParam &syncParam)
{
    DistributedDB::Query dbQuery;
    RdbQuery *rdbQuery = nullptr;
    bool isPriority = false;
    auto ret = query.QueryInterface(rdbQuery);
    if (ret != GeneralError::E_OK || rdbQuery == nullptr) {
        dbQuery.FromTable(GetIntersection(query.GetTables(), GetTables()));
    } else {
        dbQuery = rdbQuery->GetQuery();
        isPriority = rdbQuery->IsPriority();
    }
    auto syncMode = GeneralStore::GetSyncMode(syncParam.mode);
    auto dbMode = DistributedDB::SyncMode(syncMode);
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("store already closed! devices count:%{public}zu, the 1st:%{public}s, mode:%{public}d, "
              "wait:%{public}d", devices.size(),
              devices.empty() ? "null" : Anonymous::Change(*devices.begin()).c_str(), syncParam.mode, syncParam.wait);
        return { GeneralError::E_ALREADY_CLOSED, DBStatus::OK };
    }
    auto dbStatus = DistributedDB::INVALID_ARGS;
    if (syncMode < NEARBY_END) {
        dbStatus = delegate_->Sync(devices, dbMode, dbQuery, GetDBBriefCB(std::move(async)), syncParam.wait != 0);
    } else if (syncMode > NEARBY_END && syncMode < CLOUD_END) {
        return DoCloudSync(devices, dbQuery, syncParam, isPriority, async);
    }
    return { ConvertStatus(dbStatus), dbStatus };
}

std::pair<int32_t, std::shared_ptr<Cursor>> RdbGeneralStore::PreSharing(GenQuery &query)
{
    RdbQuery *rdbQuery = nullptr;
    auto ret = query.QueryInterface(rdbQuery);
    if (ret != GeneralError::E_OK || rdbQuery == nullptr) {
        ZLOGE("not RdbQuery!");
        return { GeneralError::E_INVALID_ARGS, nullptr };
    }
    auto tables = rdbQuery->GetTables();
    auto statement = rdbQuery->GetStatement();
    if (statement.empty() || tables.empty()) {
        ZLOGE("statement size:%{public}zu, tables size:%{public}zu", statement.size(), tables.size());
        return { GeneralError::E_INVALID_ARGS, nullptr };
    }
    std::string sql = BuildSql(*tables.begin(), statement, rdbQuery->GetColumns());
    VBuckets values;
    {
        std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
        if (delegate_ == nullptr) {
            ZLOGE("Database already closed! database:%{public}s", Anonymous::Change(storeInfo_.storeName).c_str());
            return { GeneralError::E_ALREADY_CLOSED, nullptr };
        }
        auto [errCode, ret] = QuerySql(sql, rdbQuery->GetBindArgs());
        values = std::move(ret);
    }
    auto rdbCloud = GetRdbCloud();
    if (rdbCloud == nullptr || values.empty()) {
        ZLOGW("rdbCloud is %{public}s, values size:%{public}zu", rdbCloud == nullptr ? "nullptr" : "not nullptr",
            values.size());
        return { GeneralError::E_CLOUD_DISABLED, nullptr };
    }
    VBuckets extends = ExtractExtend(values);
    rdbCloud->PreSharing(*tables.begin(), extends);
    for (auto value = values.begin(), extend = extends.begin(); value != values.end() && extend != extends.end();
         ++value, ++extend) {
        value->insert_or_assign(DistributedRdb::Field::SHARING_RESOURCE_FIELD, (*extend)[SchemaMeta::SHARING_RESOURCE]);
        value->erase(CLOUD_GID);
    }
    return { GeneralError::E_OK, std::make_shared<CacheCursor>(std::move(values)) };
}

VBuckets RdbGeneralStore::ExtractExtend(VBuckets &values) const
{
    VBuckets extends(values.size());
    for (auto value = values.begin(), extend = extends.begin(); value != values.end() && extend != extends.end();
         ++value, ++extend) {
        auto it = value->find(CLOUD_GID);
        if (it == value->end()) {
            continue;
        }
        auto gid = std::get_if<std::string>(&(it->second));
        if (gid == nullptr || gid->empty()) {
            continue;
        }
        extend->insert_or_assign(SchemaMeta::GID_FIELD, std::move(*gid));
    }
    return extends;
}

std::string RdbGeneralStore::BuildSql(
    const std::string &table, const std::string &statement, const std::vector<std::string> &columns) const
{
    std::string sql = "select ";
    sql.append(CLOUD_GID);
    std::string sqlNode = "select rowid";
    for (auto &column : columns) {
        sql.append(", ").append(column);
        sqlNode.append(", ").append(column);
    }
    sqlNode.append(" from ").append(table).append(statement);
    auto logTable = RelationalStoreManager::GetDistributedLogTableName(table);
    sql.append(" from ").append(logTable).append(", (").append(sqlNode);
    sql.append(") where ").append(DATE_KEY).append(" = rowid");
    return sql;
}

int32_t RdbGeneralStore::Clean(const std::vector<std::string> &devices, int32_t mode, const std::string &tableName)
{
    if (mode < 0 || mode > CLEAN_MODE_BUTT) {
        return GeneralError::E_INVALID_ARGS;
    }
    DBStatus status = DistributedDB::DB_ERROR;
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("store already closed! devices count:%{public}zu, the 1st:%{public}s, mode:%{public}d, "
              "tableName:%{public}s",
            devices.size(), devices.empty() ? "null" : Anonymous::Change(*devices.begin()).c_str(), mode,
            Anonymous::Change(tableName).c_str());
        return GeneralError::E_ALREADY_CLOSED;
    }
    switch (mode) {
        case CLOUD_INFO:
            status = delegate_->RemoveDeviceData("", static_cast<ClearMode>(CLOUD_INFO));
            if (status == DistributedDB::OK) {
                status = delegate_->RemoveDeviceData("", ClearMode::CLEAR_SHARED_TABLE);
                break;
            }
            (void)delegate_->RemoveDeviceData("", ClearMode::CLEAR_SHARED_TABLE);
            break;
        case CLOUD_DATA:
            status = delegate_->RemoveDeviceData("", static_cast<ClearMode>(CLOUD_DATA));
            if (status == DistributedDB::OK) {
                status = delegate_->RemoveDeviceData("", ClearMode::CLEAR_SHARED_TABLE);
                break;
            }
            (void)delegate_->RemoveDeviceData("", ClearMode::CLEAR_SHARED_TABLE);
            break;
        case NEARBY_DATA:
            if (devices.empty()) {
                status = delegate_->RemoveDeviceData();
                break;
            }
            for (auto device : devices) {
                status = delegate_->RemoveDeviceData(device, tableName);
            }
            break;
        default:
            return GeneralError::E_ERROR;
    }
    return status == DistributedDB::OK ? GeneralError::E_OK : GeneralError::E_ERROR;
}

int32_t RdbGeneralStore::Watch(int32_t origin, Watcher &watcher)
{
    if (origin != Watcher::Origin::ORIGIN_ALL || observer_.watcher_ != nullptr) {
        return GeneralError::E_INVALID_ARGS;
    }

    observer_.watcher_ = &watcher;
    return GeneralError::E_OK;
}

int32_t RdbGeneralStore::Unwatch(int32_t origin, Watcher &watcher)
{
    if (origin != Watcher::Origin::ORIGIN_ALL || observer_.watcher_ != &watcher) {
        return GeneralError::E_INVALID_ARGS;
    }

    observer_.watcher_ = nullptr;
    return GeneralError::E_OK;
}

RdbGeneralStore::DBBriefCB RdbGeneralStore::GetDBBriefCB(DetailAsync async)
{
    if (!async) {
        return [](auto &) {};
    }
    return [async = std::move(async)](
        const std::map<std::string, std::vector<TableStatus>> &result) {
        DistributedData::GenDetails details;
        for (auto &[key, tables] : result) {
            auto &value = details[key];
            value.progress = FINISHED;
            value.code = GeneralError::E_OK;
            for (auto &table : tables) {
                if (table.status != DBStatus::OK) {
                    value.code = GeneralError::E_ERROR;
                }
            }
        }
        async(details);
    };
}

RdbGeneralStore::DBProcessCB RdbGeneralStore::GetDBProcessCB(DetailAsync async, uint32_t syncMode, SyncId syncId,
    uint32_t highMode)
{
    std::shared_lock<std::shared_mutex> lock(asyncMutex_);
    return [async, autoAsync = async_, highMode, storeInfo = storeInfo_, flag = syncNotifyFlag_, syncMode, syncId,
        rdbCloud = GetRdbCloud()](const std::map<std::string, SyncProcess> &processes) {
        DistributedData::GenDetails details;
        for (auto &[id, process] : processes) {
            bool isDownload = false;
            auto &detail = details[id];
            detail.progress = process.process;
            detail.code = ConvertStatus(process.errCode);
            detail.dbCode = process.errCode;
            uint32_t totalCount = 0;
            for (auto [key, value] : process.tableProcess) {
                auto &table = detail.details[key];
                table.upload.total = value.upLoadInfo.total;
                table.upload.success = value.upLoadInfo.successCount;
                table.upload.failed = value.upLoadInfo.failCount;
                table.upload.untreated = table.upload.total - table.upload.success - table.upload.failed;
                totalCount += table.upload.total;
                isDownload = table.download.total > 0;
                table.download.total = value.downLoadInfo.total;
                table.download.success = value.downLoadInfo.successCount;
                table.download.failed = value.downLoadInfo.failCount;
                table.download.untreated = table.download.total - table.download.success - table.download.failed;
                detail.changeCount = (process.process == FINISHED)
                                        ? value.downLoadInfo.insertCount + value.downLoadInfo.updateCount +
                                              value.downLoadInfo.deleteCount
                                        : 0;
                totalCount += table.download.total;
            }
            if (process.process == FINISHED) {
                RdbGeneralStore::OnSyncFinish(storeInfo, flag, syncMode, syncId);
            } else {
                RdbGeneralStore::OnSyncStart(storeInfo, flag, syncMode, syncId, totalCount);
            }

            if (isDownload && (process.process == FINISHED || process.process == PROCESSING) && rdbCloud != nullptr &&
                (rdbCloud->GetLockFlag() & RdbCloud::FLAG::APPLICATION)) {
                rdbCloud->LockCloudDB(RdbCloud::FLAG::APPLICATION);
            }
        }
        if (async) {
            async(details);
        }

        if (highMode == AUTO_SYNC_MODE && autoAsync
            && (details.empty() || details.begin()->second.code != E_SYNC_TASK_MERGED)) {
            autoAsync(details);
        }
    };
}

int32_t RdbGeneralStore::Release()
{
    auto ref = 1;
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        if (ref_ == 0) {
            return 0;
        }
        ref = --ref_;
    }
    ZLOGD("ref:%{public}d", ref);
    if (ref == 0) {
        delete this;
    }
    return ref;
}

int32_t RdbGeneralStore::AddRef()
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (ref_ == 0) {
        return 0;
    }
    return ++ref_;
}

void RdbGeneralStore::Report(const std::string &faultType, int32_t errCode, const std::string &appendix)
{
    ArkDataFaultMsg msg = { .faultType = faultType,
        .bundleName = storeInfo_.bundleName,
        .moduleName = ModuleName::RDB_STORE,
        .storeName = storeInfo_.storeName,
        .errorType = errCode + GeneralStore::CLOUD_ERR_OFFSET,
        .appendixMsg = appendix };
    Reporter::GetInstance()->CloudSyncFault()->Report(msg);
}

int32_t RdbGeneralStore::SetDistributedTables(const std::vector<std::string> &tables, int32_t type,
    const std::vector<Reference> &references)
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("Database already closed! database:%{public}s, tables size:%{public}zu, type:%{public}d",
            Anonymous::Change(storeInfo_.storeName).c_str(), tables.size(), type);
        return GeneralError::E_ALREADY_CLOSED;
    }
    for (const auto &table : tables) {
        ZLOGD("tableName:%{public}s, type:%{public}d", Anonymous::Change(table).c_str(), type);
        auto dBStatus = delegate_->CreateDistributedTable(table, static_cast<DistributedDB::TableSyncType>(type));
        if (dBStatus != DistributedDB::DBStatus::OK) {
            ZLOGE("create distributed table failed, table:%{public}s, err:%{public}d",
                Anonymous::Change(table).c_str(), dBStatus);
            Report(FT_OPEN_STORE, static_cast<int32_t>(Fault::CSF_GS_CREATE_DISTRIBUTED_TABLE),
                "SetDistributedTables ret=" + std::to_string(static_cast<int32_t>(dBStatus)));
            return GeneralError::E_ERROR;
        }
    }
    std::vector<DistributedDB::TableReferenceProperty> properties;
    for (const auto &reference : references) {
        properties.push_back({ reference.sourceTable, reference.targetTable, reference.refFields });
    }
    auto status = delegate_->SetReference(properties);
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("distributed table set reference failed, err:%{public}d", status);
        return GeneralError::E_ERROR;
    }
    return GeneralError::E_OK;
}

int32_t RdbGeneralStore::SetTrackerTable(const std::string &tableName, const std::set<std::string> &trackerColNames,
    const std::set<std::string> &extendColNames, bool isForceUpgrade)
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("database already closed! database:%{public}s, tables name:%{public}s",
            Anonymous::Change(storeInfo_.storeName).c_str(), Anonymous::Change(tableName).c_str());
        return GeneralError::E_ALREADY_CLOSED;
    }
    auto status = delegate_->SetTrackerTable({ tableName, extendColNames, trackerColNames, isForceUpgrade });
    if (status == DBStatus::WITH_INVENTORY_DATA) {
        ZLOGI("Set tracker table with inventory data, database:%{public}s, tables name:%{public}s",
            Anonymous::Change(storeInfo_.storeName).c_str(), Anonymous::Change(tableName).c_str());
        return GeneralError::E_WITH_INVENTORY_DATA;
    }
    if (status != DBStatus::OK) {
        ZLOGE("Set tracker table failed! ret:%{public}d, database:%{public}s, tables name:%{public}s",
            status, Anonymous::Change(storeInfo_.storeName).c_str(), Anonymous::Change(tableName).c_str());
        return GeneralError::E_ERROR;
    }
    return GeneralError::E_OK;
}

std::shared_ptr<Cursor> RdbGeneralStore::RemoteQuery(const std::string &device,
    const DistributedDB::RemoteCondition &remoteCondition)
{
    std::shared_ptr<DistributedDB::ResultSet> dbResultSet;
    DistributedDB::DBStatus status =
        delegate_->RemoteQuery(device, remoteCondition, REMOTE_QUERY_TIME_OUT, dbResultSet);
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("DistributedDB remote query failed, device:%{public}s, status is  %{public}d.",
            Anonymous::Change(device).c_str(), status);
        return nullptr;
    }
    return std::make_shared<RdbCursor>(dbResultSet);
}

RdbGeneralStore::GenErr RdbGeneralStore::ConvertStatus(DistributedDB::DBStatus status)
{
    switch (status) {
        case DBStatus::OK:
            return GenErr::E_OK;
        case DBStatus::CLOUD_NETWORK_ERROR:
            return GenErr::E_NETWORK_ERROR;
        case DBStatus::CLOUD_LOCK_ERROR:
            return GenErr::E_LOCKED_BY_OTHERS;
        case DBStatus::CLOUD_FULL_RECORDS:
            return GenErr::E_RECODE_LIMIT_EXCEEDED;
        case DBStatus::CLOUD_ASSET_SPACE_INSUFFICIENT:
            return GenErr::E_NO_SPACE_FOR_ASSET;
        case DBStatus::BUSY:
            return GenErr::E_BUSY;
        case DBStatus::CLOUD_SYNC_TASK_MERGED:
            return GenErr::E_SYNC_TASK_MERGED;
        case DBStatus::CLOUD_DISABLED:
            return GeneralError::E_CLOUD_DISABLED;
        default:
            ZLOGI("status:0x%{public}x", status);
            break;
    }
    return GenErr::E_ERROR;
}

bool RdbGeneralStore::IsValid()
{
    return delegate_ != nullptr;
}

int32_t RdbGeneralStore::RegisterDetailProgressObserver(GeneralStore::DetailAsync async)
{
    std::unique_lock<std::shared_mutex> lock(asyncMutex_);
    async_ = std::move(async);
    return GenErr::E_OK;
}

int32_t RdbGeneralStore::UnregisterDetailProgressObserver()
{
    std::unique_lock<std::shared_mutex> lock(asyncMutex_);
    async_ = nullptr;
    return GenErr::E_OK;
}

std::pair<int32_t, VBuckets> RdbGeneralStore::QuerySql(const std::string &sql, Values &&args)
{
    std::vector<DistributedDB::VBucket> changedData;
    std::vector<DistributedDB::Type> bindArgs = ValueProxy::Convert(std::move(args));
    auto status = delegate_->ExecuteSql({ sql, std::move(bindArgs), true }, changedData);
    if (status != DBStatus::OK) {
        ZLOGE("Query failed! ret:%{public}d, sql:%{public}s, data size:%{public}zu", status,
            Anonymous::Change(sql).c_str(), changedData.size());
        if (status == DBStatus::BUSY) {
            return { GenErr::E_BUSY, {} };
        }
        return { GenErr::E_ERROR, {} };
    }
    return { GenErr::E_OK, ValueProxy::Convert(std::move(changedData)) };
}

void RdbGeneralStore::OnSyncStart(const StoreInfo &storeInfo, uint32_t flag, uint32_t syncMode, uint32_t traceId,
    uint32_t syncCount)
{
    uint32_t requiredFlag = (CLOUD_SYNC_FLAG | SEARCHABLE_FLAG);
    if (requiredFlag != (requiredFlag & flag)) {
        return;
    }
    StoreInfo info = storeInfo;
    auto evt = std::make_unique<DataSyncEvent>(std::move(info), syncMode, DataSyncEvent::DataSyncStatus::START,
        traceId, syncCount);
    EventCenter::GetInstance().PostEvent(std::move(evt));
}

void RdbGeneralStore::OnSyncFinish(const StoreInfo &storeInfo, uint32_t flag, uint32_t syncMode, uint32_t traceId)
{
    uint32_t requiredFlag = (CLOUD_SYNC_FLAG | SEARCHABLE_FLAG);
    if (requiredFlag != (requiredFlag & flag)) {
        return;
    }
    StoreInfo info = storeInfo;
    auto evt = std::make_unique<DataSyncEvent>(std::move(info), syncMode, DataSyncEvent::DataSyncStatus::FINISH,
        traceId);
    EventCenter::GetInstance().PostEvent(std::move(evt));
}

std::set<std::string> RdbGeneralStore::GetTables()
{
    std::set<std::string> tables;
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("Database already closed! database:%{public}s", Anonymous::Change(storeInfo_.storeName).c_str());
        return tables;
    }
    auto [errCode, res] = QuerySql(QUERY_TABLES_SQL, {});
    if (errCode != GenErr::E_OK) {
        return tables;
    }
    for (auto &table : res) {
        auto it = table.find("name");
        if (it == table.end() || TYPE_INDEX<std::string> != it->second.index()) {
            ZLOGW("error res! database:%{public}s", Anonymous::Change(storeInfo_.storeName).c_str());
            continue;
        }
        tables.emplace(std::move(*std::get_if<std::string>(&(it->second))));
    }
    return tables;
}

std::vector<std::string> RdbGeneralStore::GetIntersection(std::vector<std::string> &&syncTables,
    const std::set<std::string> &localTables)
{
    std::vector<std::string> res;
    for (auto &it : syncTables) {
        if (localTables.count(it) &&
            localTables.count(RelationalStoreManager::GetDistributedLogTableName(it))) {
            res.push_back(std::move(it));
        }
    }
    return res;
}

void RdbGeneralStore::ObserverProxy::PostDataChange(const StoreMetaData &meta,
    const std::vector<std::string> &tables, ChangeType type)
{
    RemoteChangeEvent::DataInfo info;
    info.userId = meta.user;
    info.storeId = meta.storeId;
    info.deviceId = meta.deviceId;
    info.bundleName = meta.bundleName;
    info.tables = tables;
    info.changeType = type;
    auto evt = std::make_unique<RemoteChangeEvent>(RemoteChangeEvent::DATA_CHANGE, std::move(info));
    EventCenter::GetInstance().PostEvent(std::move(evt));
}

void RdbGeneralStore::ObserverProxy::OnChange(const DBChangedIF &data)
{
    if (!HasWatcher()) {
        return;
    }
    std::string device = data.GetDataChangeDevice();
    auto networkId = DmAdapter::GetInstance().ToNetworkID(device);
    ZLOGD("store:%{public}s data change from :%{public}s", Anonymous::Change(storeId_).c_str(),
        Anonymous::Change(device).c_str());
    GenOrigin genOrigin;
    genOrigin.origin = GenOrigin::ORIGIN_NEARBY;
    genOrigin.dataType = GenOrigin::BASIC_DATA;
    DistributedDB::StoreProperty property;
    data.GetStoreProperty(property);
    genOrigin.id.push_back(networkId);
    genOrigin.store = storeId_;
    GeneralWatcher::ChangeInfo changeInfo{};
    watcher_->OnChange(genOrigin, {}, std::move(changeInfo));
    return;
}

void RdbGeneralStore::ObserverProxy::OnChange(DBOrigin origin, const std::string &originalId, DBChangedData &&data)
{
    if (!HasWatcher()) {
        return;
    }
    ZLOGD("store:%{public}s table:%{public}s data change from :%{public}s", Anonymous::Change(storeId_).c_str(),
        Anonymous::Change(data.tableName).c_str(), Anonymous::Change(originalId).c_str());
    GenOrigin genOrigin;
    genOrigin.origin = (origin == DBOrigin::ORIGIN_LOCAL)
                           ? GenOrigin::ORIGIN_LOCAL
                           : (origin == DBOrigin::ORIGIN_CLOUD) ? GenOrigin::ORIGIN_CLOUD : GenOrigin::ORIGIN_NEARBY;
    genOrigin.dataType = data.type == DistributedDB::ASSET ? GenOrigin::ASSET_DATA : GenOrigin::BASIC_DATA;
    genOrigin.id.push_back(originalId);
    genOrigin.store = storeId_;
    Watcher::PRIFields fields;
    Watcher::ChangeInfo changeInfo;
    bool notifyFlag = false;
    for (uint32_t i = 0; i < DistributedDB::OP_BUTT; ++i) {
        auto &info = changeInfo[data.tableName][i];
        for (auto &priData : data.primaryData[i]) {
            Watcher::PRIValue value;
            Convert(std::move(*(priData.begin())), value);
            if (notifyFlag || origin != DBOrigin::ORIGIN_CLOUD || i != DistributedDB::OP_DELETE) {
                info.push_back(std::move(value));
                continue;
            }
            auto deleteKey = std::get_if<std::string>(&value);
            if (deleteKey != nullptr && (*deleteKey == LOGOUT_DELETE_FLAG)) {
                // notify to start app
                notifyFlag = true;
            }
            info.push_back(std::move(value));
        }
    }
    if (notifyFlag) {
        ZLOGI("post data change for cleaning cloud data");
        PostDataChange(meta_, {}, CLOUD_DATA_CLEAN);
    }
    if (!data.field.empty()) {
        fields[std::move(data.tableName)] = std::move(*(data.field.begin()));
    }
    watcher_->OnChange(genOrigin, fields, std::move(changeInfo));
}

std::pair<int32_t, uint32_t> RdbGeneralStore::LockCloudDB()
{
    auto rdbCloud = GetRdbCloud();
    if (rdbCloud == nullptr) {
        return { GeneralError::E_ERROR, 0 };
    }
    return rdbCloud->LockCloudDB(RdbCloud::FLAG::APPLICATION);
}

int32_t RdbGeneralStore::UnLockCloudDB()
{
    auto rdbCloud = GetRdbCloud();
    if (rdbCloud == nullptr) {
        return GeneralError::E_ERROR;
    }
    return rdbCloud->UnLockCloudDB(RdbCloud::FLAG::APPLICATION);
}

std::shared_ptr<RdbCloud> RdbGeneralStore::GetRdbCloud() const
{
    std::shared_lock<decltype(rdbCloudMutex_)> lock(rdbCloudMutex_);
    return rdbCloud_;
}

bool RdbGeneralStore::IsFinished(SyncId syncId) const
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("database already closed! database:%{public}s", Anonymous::Change(storeInfo_.storeName).c_str());
        return true;
    }
    return delegate_->GetCloudTaskStatus(syncId).process == DistributedDB::FINISHED;
}

Executor::Task RdbGeneralStore::GetFinishTask(SyncId syncId)
{
    return [this, executor = executor_, task = tasks_, syncId]() {
        auto [exist, finishTask] = task->Find(syncId);
        if (!exist || finishTask.cb == nullptr) {
            task->Erase(syncId);
            return;
        }
        if (!IsFinished(syncId)) {
            task->ComputeIfPresent(syncId, [executor = executor_, this](SyncId syncId, FinishTask &task) {
                task.taskId = executor->Schedule(std::chrono::minutes(INTERVAL), GetFinishTask(syncId));
                return true;
            });
            return;
        }
        DBProcessCB cb;
        task->ComputeIfPresent(syncId, [&cb, executor = executor_](SyncId syncId, const FinishTask &task) {
            cb = task.cb;
            return false;
        });
        if (cb != nullptr) {
            ZLOGW("database:%{public}s syncId:%{public}" PRIu64 " miss finished. ",
                  Anonymous::Change(storeInfo_.storeName).c_str(), syncId);
            std::map<std::string, SyncProcess> result;
            result.insert({ "", { DistributedDB::FINISHED, DBStatus::DB_ERROR } });
            cb(result);
        }
    };
}

void RdbGeneralStore::SetExecutor(std::shared_ptr<Executor> executor)
{
    if (executor_ == nullptr) {
        executor_ = executor;
    }
}

void RdbGeneralStore::RemoveTasks()
{
    if (tasks_ == nullptr) {
        return;
    }
    std::list<DBProcessCB> cbs;
    std::list<TaskId> taskIds;
    tasks_->EraseIf([&cbs, &taskIds, store = storeInfo_.storeName](SyncId syncId, const FinishTask &task) {
        if (task.cb != nullptr) {
            ZLOGW("database:%{public}s syncId:%{public}" PRIu64 " miss finished. ", Anonymous::Change(store).c_str(),
                  syncId);
        }
        cbs.push_back(std::move(task.cb));
        taskIds.push_back(task.taskId);
        return true;
    });
    auto func = [](const std::list<DBProcessCB> &cbs) {
        std::map<std::string, SyncProcess> result;
        result.insert({ "", { DistributedDB::FINISHED, DBStatus::DB_ERROR } });
        for (auto &cb : cbs) {
            if (cb != nullptr) {
                cb(result);
            }
        }
    };
    if (executor_ != nullptr) {
        for (auto taskId: taskIds) {
            executor_->Remove(taskId, true);
        }
        executor_->Execute([cbs, func]() {
            func(cbs);
        });
    } else {
        func(cbs);
    }
}

RdbGeneralStore::DBProcessCB RdbGeneralStore::GetCB(SyncId syncId)
{
    return [task = tasks_, executor = executor_, syncId](const std::map<std::string, SyncProcess> &progress) {
        if (task == nullptr) {
            return;
        }
        DBProcessCB cb;
        task->ComputeIfPresent(syncId, [&cb, &progress, executor](SyncId syncId, FinishTask &finishTask) {
            cb = finishTask.cb;
            bool isFinished = !progress.empty() && progress.begin()->second.process == DistributedDB::FINISHED;
            if (isFinished) {
                finishTask.cb = nullptr;
            }
            return true;
        });
        if (cb != nullptr) {
            cb(progress);
        }
        return;
    };
}
} // namespace OHOS::DistributedRdb