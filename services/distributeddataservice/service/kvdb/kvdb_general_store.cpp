/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "KVDBGeneralStore"
#include "kvdb_general_store.h"

#include <endian.h>
#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "cloud/cloud_sync_finished_event.h"
#include "cloud/schema_meta.h"
#include "crypto_manager.h"
#include "device_matrix.h"
#include "directory/directory_manager.h"
#include "eventcenter/event_center.h"
#include "kvdb_query.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "query_helper.h"
#include "rdb_cloud.h"
#include "snapshot/bind_event.h"
#include "types.h"
#include "user_delegate.h"
#include "utils/anonymous.h"
#include "water_version_manager.h"
#include "device_manager_adapter.h"
#include "utils/anonymous.h"

namespace OHOS::DistributedKv {
using namespace DistributedData;
using namespace DistributedDB;
using DBField = DistributedDB::Field;
using DBTable = DistributedDB::TableSchema;
using DBSchema = DistributedDB::DataBaseSchema;
using ClearMode = DistributedDB::ClearMode;
using DMAdapter = DistributedData::DeviceManagerAdapter;
using DBInterceptedData = DistributedDB::InterceptedData;
constexpr int UUID_WIDTH = 4;
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
KVDBGeneralStore::DBPassword KVDBGeneralStore::GetDBPassword(const StoreMetaData &data)
{
    DBPassword dbPassword;
    if (!data.isEncrypt) {
        return dbPassword;
    }

    SecretKeyMetaData secretKey;
    secretKey.storeType = data.storeType;
    auto storeKey = data.GetSecretKey();
    MetaDataManager::GetInstance().LoadMeta(storeKey, secretKey, true);
    std::vector<uint8_t> password;
    CryptoManager::GetInstance().Decrypt(secretKey.sKey, password);
    dbPassword.SetValue(password.data(), password.size());
    password.assign(password.size(), 0);
    return dbPassword;
}

KVDBGeneralStore::DBSecurity KVDBGeneralStore::GetDBSecurity(int32_t secLevel)
{
    if (secLevel < SecurityLevel::NO_LABEL || secLevel > SecurityLevel::S4) {
        return { DistributedDB::NOT_SET, DistributedDB::ECE };
    }
    if (secLevel == SecurityLevel::S3) {
        return { DistributedDB::S3, DistributedDB::SECE };
    }
    if (secLevel == SecurityLevel::S4) {
        return { DistributedDB::S4, DistributedDB::ECE };
    }
    return { secLevel, DistributedDB::ECE };
}

KVDBGeneralStore::DBOption KVDBGeneralStore::GetDBOption(const StoreMetaData &data, const DBPassword &password)
{
    DBOption dbOption;
    dbOption.createIfNecessary = false;
    dbOption.isMemoryDb = false;
    dbOption.isEncryptedDb = data.isEncrypt;
    dbOption.isNeedCompressOnSync = data.isNeedCompress;
    if (data.isEncrypt) {
        dbOption.cipher = DistributedDB::CipherType::AES_256_GCM;
        dbOption.passwd = password;
    }
    StoreMetaDataLocal local;
    MetaDataManager::GetInstance().LoadMeta(data.GetKeyLocal(), local, true);
    if (local.isPublic || data.storeType == KvStoreType::DEVICE_COLLABORATION) {
        dbOption.conflictResolvePolicy = DistributedDB::DEVICE_COLLABORATION;
    } else if (data.storeType == KvStoreType::SINGLE_VERSION) {
        dbOption.conflictResolvePolicy = DistributedDB::LAST_WIN;
    }
    if (data.appId == Bootstrap::GetInstance().GetProcessLabel()) {
        dbOption.compressionRate = META_COMPRESS_RATE;
        dbOption.conflictResolvePolicy = DistributedDB::LAST_WIN;
    } else {
        dbOption.syncDualTupleMode = true; // tuple of (appid+storeid)
    }
    dbOption.schema = data.schema;
    dbOption.createDirByStoreIdOnly = true;
    dbOption.secOption = GetDBSecurity(data.securityLevel);
    return dbOption;
}

KVDBGeneralStore::KVDBGeneralStore(const StoreMetaData &meta)
    : manager_(meta.appId, meta.appId == Bootstrap::GetInstance().GetProcessLabel() ? defaultAccountId : meta.user,
          meta.instanceId)
{
    observer_.storeId_ = meta.storeId;
    StoreMetaDataLocal local;
    MetaDataManager::GetInstance().LoadMeta(meta.GetKeyLocal(), local, true);
    isPublic_ = local.isPublic;
    DBStatus status = DBStatus::NOT_FOUND;
    manager_.SetKvStoreConfig({ meta.dataDir });
    std::unique_lock<decltype(rwMutex_)> lock(rwMutex_);
    manager_.GetKvStore(
        meta.storeId, GetDBOption(meta, GetDBPassword(meta)), [&status, this](auto dbStatus, auto *tmpStore) {
            status = dbStatus;
            delegate_ = tmpStore;
        });
    if (delegate_ == nullptr || status != DBStatus::OK) {
        ZLOGE("GetKvStore failed. delegate is null?[%{public}d], status = %{public}d", delegate_ == nullptr, status);
        manager_.CloseKvStore(delegate_);
        return;
    }
    SetDBPushDataInterceptor(meta.storeType);
    SetDBReceiveDataInterceptor(meta.storeType);
    delegate_->RegisterObserver({}, DistributedDB::OBSERVER_CHANGES_FOREIGN, &observer_);
    delegate_->RegisterObserver({}, DistributedDB::OBSERVER_CHANGES_CLOUD, &observer_);
    InitWaterVersion(meta);
    if (DeviceMatrix::GetInstance().IsDynamic(meta) || DeviceMatrix::GetInstance().IsStatics(meta)) {
        delegate_->SetRemotePushFinishedNotify([meta](const DistributedDB::RemotePushNotifyInfo &info) {
            DeviceMatrix::GetInstance().OnExchanged(info.deviceId, meta, DeviceMatrix::ChangeType::CHANGE_REMOTE);
        });
    }
    if (meta.isAutoSync) {
        bool param = true;
        auto data = static_cast<DistributedDB::PragmaData>(&param);
        delegate_->Pragma(DistributedDB::SET_SYNC_RETRY, data);
    }
    storeInfo_.tokenId = meta.tokenId;
    storeInfo_.bundleName = meta.bundleName;
    storeInfo_.storeName = meta.storeId;
    storeInfo_.instanceId = meta.instanceId;
    storeInfo_.user = std::stoi(meta.user);
    enableCloud_ = meta.enableCloud;
}

KVDBGeneralStore::~KVDBGeneralStore()
{
    {
        std::unique_lock<decltype(rwMutex_)> lock(rwMutex_);
        if (delegate_ != nullptr) {
            delegate_->UnRegisterObserver(&observer_);
        }
        manager_.CloseKvStore(delegate_);
        delegate_ = nullptr;
    }
    for (auto &bindInfo_ : bindInfos_) {
        if (bindInfo_.db_ != nullptr) {
            bindInfo_.db_->Close();
        }
    }
    bindInfos_.clear();
    dbClouds_.clear();
}

int32_t KVDBGeneralStore::BindSnapshots(std::shared_ptr<std::map<std::string, std::shared_ptr<Snapshot>>> bindAssets)
{
    return GenErr::E_NOT_SUPPORT;
}

int32_t KVDBGeneralStore::Bind(Database &database, const std::map<uint32_t, BindInfo> &bindInfos,
    const CloudConfig &config)
{
    if (bindInfos.empty()) {
        ZLOGW("No cloudDB!");
        return GeneralError::E_OK;
    }

    std::map<std::string, DataBaseSchema> schemas;
    auto dbSchema = GetDBSchema(database);
    for (auto &[userId, bindInfo] : bindInfos) {
        if (bindInfo.db_ == nullptr) {
            return GeneralError::E_INVALID_ARGS;
        }

        if (isBound_.exchange(true)) {
            return GeneralError::E_OK;
        }

        dbClouds_.insert({ std::to_string(userId), std::make_shared<DistributedRdb::RdbCloud>(bindInfo.db_, nullptr) });
        bindInfos_.insert(std::move(bindInfo));
        schemas.insert({ std::to_string(userId), dbSchema });
    }
    DistributedDB::CloudSyncConfig dbConfig;
    dbConfig.maxUploadCount = config.maxNumber;
    dbConfig.maxUploadSize = config.maxSize;
    dbConfig.maxRetryConflictTimes = config.maxRetryConflictTimes;
    std::unique_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        return GeneralError::E_ALREADY_CLOSED;
    }
    delegate_->SetCloudDB(dbClouds_);
    delegate_->SetCloudDbSchema(std::move(schemas));
    delegate_->SetCloudSyncConfig(dbConfig);
    return GeneralError::E_OK;
}

bool KVDBGeneralStore::IsBound()
{
    return isBound_;
}

int32_t KVDBGeneralStore::Close()
{
    std::unique_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        return GeneralError::E_OK;
    }
    int32_t count = delegate_->GetTaskCount();
    if (count > 0) {
        return GeneralError::E_BUSY;
    }
    if (delegate_ != nullptr) {
        delegate_->UnRegisterObserver(&observer_);
    }
    auto status = manager_.CloseKvStore(delegate_);
    if (status != DBStatus::OK) {
        return status;
    }
    delegate_ = nullptr;
    return GeneralError::E_OK;
}

int32_t KVDBGeneralStore::Execute(const std::string &table, const std::string &sql)
{
    return GeneralError::E_NOT_SUPPORT;
}

int32_t KVDBGeneralStore::Insert(const std::string &table, VBuckets &&values)
{
    return GeneralError::E_NOT_SUPPORT;
}

int32_t KVDBGeneralStore::Update(const std::string &table, const std::string &setSql, Values &&values,
    const std::string &whereSql, Values &&conditions)
{
    return GeneralError::E_NOT_SUPPORT;
}

int32_t KVDBGeneralStore::Delete(const std::string &table, const std::string &sql, Values &&args)
{
    return GeneralError::E_NOT_SUPPORT;
}

int32_t KVDBGeneralStore::Replace(const std::string &table, VBucket &&value)
{
    return GeneralError::E_NOT_SUPPORT;
}

std::shared_ptr<Cursor> KVDBGeneralStore::Query(
    __attribute__((unused)) const std::string &table, const std::string &sql, Values &&args)
{
    return nullptr;
}

std::shared_ptr<Cursor> KVDBGeneralStore::Query(const std::string &table, GenQuery &query)
{
    return nullptr;
}

int32_t KVDBGeneralStore::MergeMigratedData(const std::string &tableName, VBuckets &&values)
{
    return GeneralError::E_NOT_SUPPORT;
}

KVDBGeneralStore::DBSyncCallback KVDBGeneralStore::GetDBSyncCompleteCB(DetailAsync async)
{
    if (!async) {
        return [](auto &) {};
    }
    return [async = std::move(async)](const std::map<std::string, DBStatus> &status) {
        GenDetails details;
        for (auto &[key, dbStatus] : status) {
            auto &value = details[key];
            value.progress = FINISHED;
            value.code = GeneralError::E_OK;
            if (dbStatus != DBStatus::OK) {
                value.code = dbStatus;
            }
        }
        async(details);
    };
}

DBStatus KVDBGeneralStore::CloudSync(
    const Devices &devices, DistributedDB::SyncMode cloudSyncMode, DetailAsync async, int64_t wait)
{
    DistributedDB::CloudSyncOption syncOption;
    syncOption.devices = devices;
    syncOption.mode = cloudSyncMode;
    syncOption.waitTime = wait;
    syncOption.lockAction = DistributedDB::LockAction::NONE;
    if (storeInfo_.user == 0) {
        std::vector<int32_t> users;
        AccountDelegate::GetInstance()->QueryUsers(users);
        syncOption.users.push_back(std::to_string(users[0]));
    } else {
        syncOption.users.push_back(std::to_string(storeInfo_.user));
    }
    return delegate_->Sync(syncOption, GetDBProcessCB(async));
}

int32_t KVDBGeneralStore::Sync(const Devices &devices, GenQuery &query, DetailAsync async, SyncParam &syncParm)
{
    auto syncMode = GeneralStore::GetSyncMode(syncParm.mode);
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("store already closed! devices count:%{public}zu, the 1st:%{public}s, mode:%{public}d", devices.size(),
            devices.empty() ? "null" : Anonymous::Change(*devices.begin()).c_str(), syncParm.mode);
        return GeneralError::E_ALREADY_CLOSED;
    }
    DBStatus dbStatus;
    auto dbMode = DistributedDB::SyncMode(syncMode);
    if (syncMode > NEARBY_END && syncMode < CLOUD_END) {
        if (!enableCloud_) {
            return GeneralError::E_NOT_SUPPORT;
        }
        dbStatus = CloudSync(devices, dbMode, async, syncParm.wait);
    } else {
        if (devices.empty()) {
            ZLOGE("Devices is empty! mode:%{public}d", syncParm.mode);
            return GeneralError::E_INVALID_ARGS;
        }
        KVDBQuery *kvQuery = nullptr;
        auto ret = query.QueryInterface(kvQuery);
        DistributedDB::Query dbQuery;
        if (ret == GeneralError::E_OK && kvQuery != nullptr && kvQuery->IsValidQuery()) {
            dbQuery = kvQuery->GetDBQuery();
        } else {
            return GeneralError::E_INVALID_ARGS;
        }
        if (syncMode == NEARBY_SUBSCRIBE_REMOTE) {
            dbStatus = delegate_->SubscribeRemoteQuery(devices, GetDBSyncCompleteCB(std::move(async)), dbQuery, false);
        } else if (syncMode == NEARBY_UNSUBSCRIBE_REMOTE) {
            dbStatus =
                delegate_->UnSubscribeRemoteQuery(devices, GetDBSyncCompleteCB(std::move(async)), dbQuery, false);
        } else if (syncMode < NEARBY_END) {
            if (kvQuery->IsEmpty()) {
                dbStatus = delegate_->Sync(devices, dbMode, GetDBSyncCompleteCB(std::move(async)), false);
            } else {
                dbStatus = delegate_->Sync(devices, dbMode, GetDBSyncCompleteCB(std::move(async)), dbQuery, false);
            }
        } else {
            ZLOGE("Err sync mode! sync mode:%{public}d", syncMode);
            dbStatus = DistributedDB::INVALID_ARGS;
        }
    }
    return ConvertStatus(dbStatus);
}

void KVDBGeneralStore::SetEqualIdentifier(const std::string &appId, const std::string &storeId)
{
    if (delegate_ == nullptr) {
        ZLOGE("store already closed! appId:%{public}s storeId:%{public}s", appId.c_str(),
            Anonymous::Change(storeId).c_str());
        return;
    }
    std::vector<std::string> sameAccountDevs {};
    std::vector<std::string> defaultAccountDevs {};
    auto uuids = DMAdapter::ToUUID(DMAdapter::GetInstance().GetRemoteDevices());
    GetIdentifierParams(sameAccountDevs, uuids, IDENTICAL_ACCOUNT);
    GetIdentifierParams(defaultAccountDevs, uuids, NO_ACCOUNT);
    if (!sameAccountDevs.empty()) {
        auto accountId = AccountDelegate::GetInstance()->GetUnencryptedAccountId();
        auto syncIdentifier = KvManager::GetKvStoreIdentifier(accountId, appId, storeId);
        ZLOGI("same account set compatible identifier store:%{public}s, user:%{public}s, device:%{public}.10s",
            Anonymous::Change(storeId).c_str(), Anonymous::Change(accountId).c_str(),
            DistributedData::Serializable::Marshall(sameAccountDevs).c_str());
        delegate_->SetEqualIdentifier(syncIdentifier, sameAccountDevs);
    }
    if (!defaultAccountDevs.empty()) {
        auto syncIdentifier = KvManager::GetKvStoreIdentifier(defaultAccountId, appId, storeId);
        ZLOGI("no account set compatible identifier store:%{public}s, device:%{public}.10s",
            Anonymous::Change(storeId).c_str(),
            DistributedData::Serializable::Marshall(defaultAccountDevs).c_str());
        delegate_->SetEqualIdentifier(syncIdentifier, defaultAccountDevs);
    }
}

void KVDBGeneralStore::GetIdentifierParams(std::vector<std::string> &devices,
    const std::vector<std::string> &uuids, int32_t authType)
{
    for (const auto &devId : uuids) {
        if (DMAdapter::GetInstance().IsOHOSType(devId)) {
            continue;
        }
        if (DMAdapter::GetInstance().GetAuthType(devId) != authType) {
            continue;
        }
        devices.push_back(devId);
    }
    ZLOGI("devices size: %{public}zu", devices.size());
}

std::shared_ptr<Cursor> KVDBGeneralStore::PreSharing(GenQuery &query)
{
    return nullptr;
}

int32_t KVDBGeneralStore::Clean(const std::vector<std::string> &devices, int32_t mode, const std::string &tableName)
{
    if (mode < 0 || mode > CLEAN_MODE_BUTT) {
        return GeneralError::E_INVALID_ARGS;
    }
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("store already closed! devices count:%{public}zu, the 1st:%{public}s, mode:%{public}d", devices.size(),
            devices.empty() ? "null" : Anonymous::Change(*devices.begin()).c_str(), mode);
        return GeneralError::E_ALREADY_CLOSED;
    }
    DBStatus status = OK;
    switch (mode) {
        case CLOUD_INFO:
            status = delegate_->RemoveDeviceData(
                "", isPublic_ ? static_cast<ClearMode>(CLOUD_DATA) : static_cast<ClearMode>(CLOUD_INFO));
            break;
        case CLOUD_DATA:
            status = delegate_->RemoveDeviceData("", static_cast<ClearMode>(CLOUD_DATA));
            break;
        case NEARBY_DATA:
            if (devices.empty()) {
                status = delegate_->RemoveDeviceData();
                break;
            }
            for (auto device : devices) {
                status = delegate_->RemoveDeviceData(device);
            }
            break;
        default:
            return GeneralError::E_ERROR;
    }
    return status == DistributedDB::OK ? GeneralError::E_OK : GeneralError::E_ERROR;
}

int32_t KVDBGeneralStore::Watch(int32_t origin, Watcher &watcher)
{
    if (origin != Watcher::Origin::ORIGIN_ALL || observer_.watcher_ != nullptr) {
        return GeneralError::E_INVALID_ARGS;
    }

    observer_.watcher_ = &watcher;
    return GeneralError::E_OK;
}

int32_t KVDBGeneralStore::Unwatch(int32_t origin, Watcher &watcher)
{
    if (origin != Watcher::Origin::ORIGIN_ALL || observer_.watcher_ != &watcher) {
        return GeneralError::E_INVALID_ARGS;
    }

    observer_.watcher_ = nullptr;
    return GeneralError::E_OK;
}

int32_t KVDBGeneralStore::Release()
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

int32_t KVDBGeneralStore::AddRef()
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (ref_ == 0) {
        return 0;
    }
    return ++ref_;
}

int32_t KVDBGeneralStore::SetDistributedTables(
    const std::vector<std::string> &tables, int32_t type, const std::vector<Reference> &references)
{
    return GeneralError::E_OK;
}

int32_t KVDBGeneralStore::SetTrackerTable(
    const std::string &tableName, const std::set<std::string> &trackerColNames, const std::string &extendColName)
{
    return GeneralError::E_OK;
}

KVDBGeneralStore::GenErr KVDBGeneralStore::ConvertStatus(DistributedDB::DBStatus status)
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
        case DBStatus::CLOUD_SYNC_TASK_MERGED:
            return GenErr::E_SYNC_TASK_MERGED;
        default:
            ZLOGI("status:0x%{public}x", status);
            break;
    }
    return GenErr::E_ERROR;
}

bool KVDBGeneralStore::IsValid()
{
    std::unique_lock<decltype(rwMutex_)> lock(rwMutex_);
    return delegate_ != nullptr;
}

int32_t KVDBGeneralStore::RegisterDetailProgressObserver(GeneralStore::DetailAsync async)
{
    return GenErr::E_OK;
}

int32_t KVDBGeneralStore::UnregisterDetailProgressObserver()
{
    return GenErr::E_OK;
}

std::vector<std::string> KVDBGeneralStore::GetWaterVersion(const std::string &deviceId)
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("store already closed! deviceId:%{public}s", Anonymous::Change(deviceId).c_str());
        return {};
    }
    auto [status, versions] = delegate_->GetCloudVersion(deviceId);
    if (status != DBStatus::OK || versions.empty()) {
        return {};
    }
    std::vector<std::string> res;
    for (auto &[_, version] : versions) {
        res.push_back(std::move(version));
    }
    return res;
}

void KVDBGeneralStore::InitWaterVersion(const StoreMetaData &meta)
{
    CheckerManager::StoreInfo info = { atoi(meta.user.c_str()), meta.tokenId, meta.bundleName, meta.storeId };
    bool isDynamic = CheckerManager::GetInstance().IsDynamic(info);
    bool isStatic = CheckerManager::GetInstance().IsStatic(info);
    if (!isDynamic && !isStatic) {
        return;
    }
    delegate_->SetGenCloudVersionCallback([info](auto &originVersion) {
        return WaterVersionManager::GetInstance().GetWaterVersion(info.bundleName, info.storeId);
    });
    callback_ = [meta]() {
        auto event = std::make_unique<CloudSyncFinishedEvent>(CloudEvent::CLOUD_SYNC_FINISHED, meta);
        EventCenter::GetInstance().PostEvent(std::move(event));
    };
}

void KVDBGeneralStore::ObserverProxy::OnChange(DBOrigin origin, const std::string &originalId, DBChangeData &&data)
{
    if (!HasWatcher()) {
        return;
    }
    GenOrigin genOrigin;
    genOrigin.origin = (origin == DBOrigin::ORIGIN_CLOUD) ? GenOrigin::ORIGIN_CLOUD : GenOrigin::ORIGIN_NEARBY;
    genOrigin.id.push_back(originalId);
    genOrigin.store = storeId_;
    Watcher::ChangeInfo changeInfo;
    for (uint32_t i = 0; i < DistributedDB::OP_BUTT; ++i) {
        auto &info = changeInfo[storeId_][i];
        for (auto &priData : data.primaryData[i]) {
            Watcher::PRIValue value;
            Convert(std::move(*(priData.begin())), value);
            info.push_back(std::move(value));
        }
    }
    watcher_->OnChange(genOrigin, {}, std::move(changeInfo));
}

void KVDBGeneralStore::ObserverProxy::OnChange(const DistributedDB::KvStoreChangedData &data)
{
    if (!HasWatcher()) {
        return;
    }
    const auto &inserts = data.GetEntriesInserted();
    const auto &deletes = data.GetEntriesDeleted();
    const auto &updates = data.GetEntriesUpdated();
    Watcher::ChangeData changeData;
    ConvertChangeData(inserts, changeData[storeId_][DistributedDB::OP_INSERT]);
    ConvertChangeData(deletes, changeData[storeId_][DistributedDB::OP_DELETE]);
    ConvertChangeData(updates, changeData[storeId_][DistributedDB::OP_UPDATE]);
    GenOrigin genOrigin;
    genOrigin.origin = GenOrigin::ORIGIN_NEARBY;
    genOrigin.store = storeId_;

    watcher_->OnChange(genOrigin, {}, std::move(changeData));
}

void KVDBGeneralStore::ObserverProxy::ConvertChangeData(const std::list<DBEntry> &entries, std::vector<Values> &values)
{
    for (auto &entry : entries) {
        auto value = std::vector<Value>{ entry.key, entry.value };
        values.push_back(value);
    }
}

KVDBGeneralStore::DBProcessCB KVDBGeneralStore::GetDBProcessCB(DetailAsync async)
{
    return [async, callback = callback_](const std::map<std::string, SyncProcess> &processes) {
        if (!async && !callback) {
            return;
        }
        DistributedData::GenDetails details;
        for (auto &[id, process] : processes) {
            auto &detail = details[id];
            detail.progress = process.process;
            detail.code = ConvertStatus(process.errCode);
            detail.dbCode = DB_ERR_OFFSET + process.errCode;
            for (auto [key, value] : process.tableProcess) {
                auto &table = detail.details[key];
                table.upload.total = value.upLoadInfo.total;
                table.upload.success = value.upLoadInfo.successCount;
                table.upload.failed = value.upLoadInfo.failCount;
                table.upload.untreated = table.upload.total - table.upload.success - table.upload.failed;
                table.download.total = value.downLoadInfo.total;
                table.download.success = value.downLoadInfo.successCount;
                table.download.failed = value.downLoadInfo.failCount;
                table.download.untreated = table.download.total - table.download.success - table.download.failed;
                detail.dataChange = detail.dataChange ||
                                    (process.process == FINISHED &&
                                        (value.downLoadInfo.insertCount > 0 || value.downLoadInfo.updateCount > 0 ||
                                            value.downLoadInfo.deleteCount > 0));
            }
        }
        if (async) {
            async(details);
        }
        if (!details.empty() && details.begin()->second.dataChange && callback) {
            callback();
        }
    };
}

void KVDBGeneralStore::SetDBPushDataInterceptor(int32_t storeType)
{
    delegate_->SetPushDataInterceptor(
        [this, storeType](DBInterceptedData &data, const std::string &sourceID, const std::string &targetID) {
            int errCode = DBStatus::OK;
            if (storeType != KvStoreType::DEVICE_COLLABORATION || DMAdapter::GetInstance().IsOHOSType(targetID)) {
                return errCode;
            }
            if (targetID.empty()) {
                ZLOGE("targetID empty");
                return static_cast<int>(DBStatus::DB_ERROR);
            }
            auto entries = data.GetEntries();
            for (size_t i = 0; i < entries.size(); i++) {
                if (entries[i].key.empty()) {
                    continue;
                }
                auto oriKey = entries[i].key;
                auto newKey = GetNewKey(oriKey, sourceID);
                errCode = data.ModifyKey(i, newKey);
                if (errCode != DBStatus::OK) {
                    ZLOGE("ModifyKey err: %{public}d", errCode);
                    break;
                }
            }
            return errCode;
        }
    );
}

void KVDBGeneralStore::SetDBReceiveDataInterceptor(int32_t storeType)
{
    delegate_->SetReceiveDataInterceptor(
        [this, storeType](DBInterceptedData &data, const std::string &sourceID, const std::string &targetID) {
            int errCode = DBStatus::OK;
            if (storeType != KvStoreType::DEVICE_COLLABORATION || DMAdapter::GetInstance().IsOHOSType(sourceID)) {
                return errCode;
            }
            if (sourceID.empty()) {
                ZLOGE("targetID empty");
                return static_cast<int>(DBStatus::DB_ERROR);
            }
            auto entries = data.GetEntries();
            for (size_t i = 0; i < entries.size(); i++) {
                if (entries[i].key.empty()) {
                    continue;
                }

                auto networkId = DMAdapter::GetInstance().ToNetworkID(sourceID);
                auto encyptedUuid = DMAdapter::GetInstance().GetEncryptedUuidByNetworkId(networkId);
                if (encyptedUuid.empty()) {
                    ZLOGE("get encyptedUuid failed");
                    continue;
                }

                auto oriKey = entries[i].key;
                auto newKey = GetNewKey(oriKey, encyptedUuid);
                errCode = data.ModifyKey(i, newKey);
                if (errCode != DBStatus::OK) {
                    ZLOGE("ModifyKey err: %{public}d", errCode);
                    break;
                }
            }
            return errCode;
        }
    );
}

std::vector<uint8_t> KVDBGeneralStore::GetNewKey(std::vector<uint8_t> &key, const std::string &uuid)
{
    uint32_t remoteLen = *(reinterpret_cast<uint32_t *>(&(*(key.end() - sizeof(uint32_t)))));
    remoteLen = le32toh(remoteLen);
    uint32_t uuidLen = uuid.size();

    std::vector<uint8_t> out;
    std::vector<uint8_t> oriKey(key.begin() + remoteLen, key.end() - UUID_WIDTH);
    out.insert(out.end(), uuid.begin(), uuid.end());
    out.insert(out.end(), oriKey.begin(), oriKey.end());
    uuidLen = htole32(uuidLen);
    uint8_t *buf = reinterpret_cast<uint8_t *>(&uuidLen);
    out.insert(out.end(), buf, buf + sizeof(uuidLen));
    return out;
}

void KVDBGeneralStore::SetConfig(const GeneralStore::StoreConfig &storeConfig)
{
    enableCloud_ = storeConfig.enableCloud_;
}
} // namespace OHOS::DistributedKv