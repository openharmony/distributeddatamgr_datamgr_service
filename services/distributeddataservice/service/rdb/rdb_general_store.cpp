/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "cloud_service.h"
#include "cloud/asset_loader.h"
#include "cloud/cloud_db.h"
#include "cloud/schema_meta.h"
#include "crypto_manager.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "rdb_cursor.h"
#include "rdb_helper.h"
#include "rdb_query.h"
#include "relational_store_manager.h"
#include "utils/anonymous.h"
#include "utils/constant.h"
#include "value_proxy.h"
#include "device_manager_adapter.h"
#include "rdb_result_set_impl.h"
namespace OHOS::DistributedRdb {
using namespace DistributedData;
using namespace DistributedDB;
using namespace NativeRdb;
using namespace  CloudData;
using DBField = DistributedDB::Field;
using DBTable = DistributedDB::TableSchema;
using DBSchema = DistributedDB::DataBaseSchema;
using ClearMode = DistributedDB::ClearMode;
using DBStatus = DistributedDB::DBStatus;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;

RdbGeneralStore::RdbGeneralStore(const StoreMetaData &meta) : manager_(meta.appId, meta.user, meta.instanceId)
{
    observer_.storeId_ = meta.storeId;
    RelationalStoreDelegate::Option option;
    if (meta.isEncrypt) {
        std::string key = meta.GetSecretKey();
        SecretKeyMetaData secretKeyMeta;
        MetaDataManager::GetInstance().LoadMeta(key, secretKeyMeta, true);
        std::vector<uint8_t> decryptKey;
        CryptoManager::GetInstance().Decrypt(secretKeyMeta.sKey, decryptKey);
        if (option.passwd.SetValue(decryptKey.data(), decryptKey.size()) != CipherPassword::OK) {
            std::fill(decryptKey.begin(), decryptKey.end(), 0);
        }
        std::fill(decryptKey.begin(), decryptKey.end(), 0);
        option.isEncryptedDb = meta.isEncrypt;
        option.iterateTimes = ITERATE_TIMES;
        option.cipher = CipherType::AES_256_GCM;
    }
    option.observer = &observer_;
    manager_.OpenStore(meta.dataDir, meta.storeId, option, delegate_);
}

RdbGeneralStore::~RdbGeneralStore()
{
    manager_.CloseStore(delegate_);
    delegate_ = nullptr;
    bindInfo_.loader_ = nullptr;
    bindInfo_.db_->Close();
    bindInfo_.db_ = nullptr;
    rdbCloud_ = nullptr;
    rdbLoader_ = nullptr;
}

int32_t RdbGeneralStore::Bind(const Database &database, BindInfo bindInfo)
{
    if (bindInfo.db_ == nullptr || bindInfo.loader_ == nullptr) {
        return GeneralError::E_INVALID_ARGS;
    }

    if (isBound_.exchange(true)) {
        return GeneralError::E_OK;
    }

    bindInfo_ = std::move(bindInfo);
    rdbCloud_ = std::make_shared<RdbCloud>(bindInfo_.db_);
    rdbLoader_ = std::make_shared<RdbAssetLoader>(bindInfo_.loader_);
    DBSchema schema;
    schema.tables.resize(database.tables.size());
    for (size_t i = 0; i < database.tables.size(); i++) {
        const Table &table = database.tables[i];
        DBTable &dbTable = schema.tables[i];
        dbTable.name = table.name;
        for (auto &field : table.fields) {
            DBField dbField;
            dbField.colName = field.colName;
            dbField.type = field.type;
            dbField.primary = field.primary;
            dbField.nullable = field.nullable;
            dbTable.fields.push_back(std::move(dbField));
        }
    }
    std::unique_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("database:%{public}s already closed!", Anonymous::Change(database.name).c_str());
        return GeneralError::E_ALREADY_CLOSED;
    }
    delegate_->SetCloudDB(rdbCloud_);
    delegate_->SetIAssetLoader(rdbLoader_);
    delegate_->SetCloudDbSchema(std::move(schema));
    return GeneralError::E_OK;
}

bool RdbGeneralStore::IsBound()
{
    return isBound_;
}

int32_t RdbGeneralStore::Close()
{
    std::unique_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        return 0;
    }
    auto status = manager_.CloseStore(delegate_);
    if (status != DBStatus::OK) {
        return status;
    }
    delegate_ = nullptr;
    bindInfo_.loader_ = nullptr;
    bindInfo_.db_->Close();
    bindInfo_.db_ = nullptr;
    rdbCloud_ = nullptr;
    rdbLoader_ = nullptr;
    return 0;
}

int32_t RdbGeneralStore::Execute(const std::vector<std::string> &tables, const std::string &sql, const Values &bindArgs)
{
    if (sql == SET_DISTRIBUTED_TABLE) {
        ZLOGD("Execute SET_DISTRIBUTED_TABLE");
        if (bindArgs.empty()) {
            ZLOGE("bindArgs is empty");
            return GeneralError::E_INVALID_ARGS;
        }
        auto *type = std::get_if<int64_t>(&bindArgs[0]);
        if (type == nullptr || *type < DistributedDB::TableSyncType::DEVICE_COOPERATION ||
            *type > DistributedDB::TableSyncType::CLOUD_COOPERATION) {
            ZLOGE("type is invalid, type:%{public}s", type == nullptr ? "nullptr" : std::to_string(*type).c_str());
            return GeneralError::E_INVALID_ARGS;
        }
        return SetDistributedTables(tables, static_cast<DistributedDB::TableSyncType>(*type));
    }
    return GeneralError::E_OK;
}

int32_t RdbGeneralStore::BatchInsert(const std::string &table, VBuckets &&values)
{
    return 0;
}

int32_t RdbGeneralStore::BatchUpdate(const std::string &table, const std::string &sql, VBuckets &&values)
{
    return 0;
}

int32_t RdbGeneralStore::Delete(const std::string &table, const std::string &sql, Values &&args)
{
    return 0;
}

int32_t RdbGeneralStore::RemoveDeviceData()
{
    auto status = delegate_->RemoveDeviceData();
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("RemoveDeviceData failed, status is %{public}d.", status);
        return GeneralError::E_ERROR;
    }
    return GeneralError::E_OK;
}

std::shared_ptr<Cursor> RdbGeneralStore::Query(const std::string &table, const std::string &sql, Values &&args,
    const std::string &device)
{
    std::shared_ptr<RdbCursor> cursor = nullptr;
    if (!device.empty()) {
        std::shared_ptr<DistributedDB::ResultSet> dbResultSet;
        std::vector<std::string> bindArgs;
        if (!Convert(std::move(args), bindArgs)) {
            ZLOGE("Convert to bindArgs failed");
            return {};
        }
        DistributedDB::DBStatus status =
            delegate_->RemoteQuery(device, { sql, bindArgs }, REMOTE_QUERY_TIME_OUT, dbResultSet);
        if (status != DistributedDB::DBStatus::OK) {
            ZLOGE("DistributedDB remote query failed, status is  %{public}d.", status);
            return {};
        }
        cursor = std::make_shared<RdbCursor>(dbResultSet);
    }
    return cursor;
}

std::shared_ptr<Cursor> RdbGeneralStore::Query(const std::string &table, GenQuery &query, const std::string &device)
{
    return std::shared_ptr<Cursor>();
}

int32_t RdbGeneralStore::Sync(const Devices &devices, int32_t mode, GenQuery &query, DetailAsync async, int32_t wait)
{
    DistributedDB::Query dbQuery;
    RdbQuery *rdbQuery = nullptr;
    auto ret = query.QueryInterface(rdbQuery);
    if (ret != GeneralError::E_OK || rdbQuery == nullptr) {
        dbQuery.FromTable(query.GetTables());
    } else {
        dbQuery = rdbQuery->GetQuery();
    }
    auto dbMode = DistributedDB::SyncMode(mode);
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("store already closed! devices count:%{public}zu, the 1st:%{public}s, mode:%{public}d, "
              "wait:%{public}d",
            devices.size(), devices.empty() ? "null" : Anonymous::Change(*devices.begin()).c_str(), mode, wait);
        return GeneralError::E_ALREADY_CLOSED;
    }
    auto status = (mode < NEARBY_END)
                  ? delegate_->Sync(devices, dbMode, dbQuery, GetDBBriefCB(std::move(async)), wait != 0 )
                  : (mode > NEARBY_END && mode < CLOUD_END)
                  ? delegate_->Sync(devices, dbMode, dbQuery, GetDBProcessCB(std::move(async)), wait)
                  : DistributedDB::INVALID_ARGS;
    return status == DistributedDB::OK ? GeneralError::E_OK : GeneralError::E_ERROR;
}

int32_t RdbGeneralStore::Clean(const std::vector<std::string> &devices, int32_t mode, const std::string &tableName)
{
    if (mode < 0 || mode > CloudService::CLEAR_CLOUD_BUTT) {
        return GeneralError::E_INVALID_ARGS;
    }
    int32_t dbMode;
    DBStatus status;
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (delegate_ == nullptr) {
        ZLOGE("store already closed! devices count:%{public}zu, the 1st:%{public}s, mode:%{public}d, "
              "tableName:%{public}s",
            devices.size(), devices.empty() ? "null" : Anonymous::Change(*devices.begin()).c_str(), mode,
            Anonymous::Change(tableName).c_str());
        return GeneralError::E_ALREADY_CLOSED;
    }
    switch (mode) {
        case CloudService::CLEAR_CLOUD_INFO:
            dbMode = CleanMode::CLOUD_INFO;
            status = delegate_->RemoveDeviceData("", static_cast<ClearMode>(dbMode));
            break;
        case CloudService::CLEAR_CLOUD_DATA_AND_INFO:
            dbMode = CleanMode::CLOUD_DATA;
            status = delegate_->RemoveDeviceData("", static_cast<ClearMode>(dbMode));
            break;
        default:
            if (devices.empty()) {
                status = delegate_->RemoveDeviceData();
                break;
            }

            for (auto device : devices) {
                status = delegate_->RemoveDeviceData(device, tableName);
            }
            break;
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
    return [async = std::move(async)](const std::map<std::string, std::vector<TableStatus>> &result) {
        DistributedData::GenDetails details;
        for (auto &[key, tables] : result) {
            auto &value = details[key];
            value.progress = FINISHED;
            value.progress = GeneralError::E_OK;
            for (auto &table : tables) {
                if (table.status != DBStatus::OK) {
                    value.code = GeneralError::E_ERROR;
                }
            }
        }
        async(details);
    };
}

RdbGeneralStore::DBProcessCB RdbGeneralStore::GetDBProcessCB(DetailAsync async)
{
    if (!async) {
        return [](auto &) {};
    }

    return [async = std::move(async)](const std::map<std::string, SyncProcess> &processes) {
        DistributedData::GenDetails details;
        for (auto &[id, process] : processes) {
            auto &detail = details[id];
            detail.progress = process.process;
            detail.code = process.errCode == DBStatus::OK ? GeneralError::E_OK : GeneralError::E_ERROR;
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
            }
        }
        async(details);
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

int32_t RdbGeneralStore::SetDistributedTables(const std::vector<std::string> &tables, int32_t type)
{
    for (const auto &table : tables) {
        ZLOGD("tableName:%{public}s, type:%{public}d", Anonymous::Change(table).c_str(), type);
        auto dBStatus = delegate_->CreateDistributedTable(table, static_cast<DistributedDB::TableSyncType>(type));
        if (dBStatus != DistributedDB::DBStatus::OK) {
            ZLOGE("create distributed table failed, table:%{public}s, err:%{public}d",
                Anonymous::Change(table).c_str(), dBStatus);
            return GeneralError::E_ERROR;
        }
    }
    return GeneralError::E_OK;
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
    watcher_->OnChange(genOrigin, {}, {});
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
    genOrigin.origin = (origin == DBOrigin::ORIGIN_LOCAL)   ? GenOrigin::ORIGIN_LOCAL
                       : (origin == DBOrigin::ORIGIN_CLOUD) ? GenOrigin::ORIGIN_CLOUD
                                                            : GenOrigin::ORIGIN_NEARBY;
    genOrigin.dataType = data.type == DistributedDB::ASSET ? GenOrigin::ASSET_DATA : GenOrigin::BASIC_DATA;
    genOrigin.id.push_back(originalId);
    genOrigin.store = storeId_;
    Watcher::PRIFields fields;
    Watcher::ChangeInfo changeInfo;
    for (uint32_t i = 0; i < DistributedDB::OP_BUTT; ++i) {
        auto &info = changeInfo[data.tableName][i];
        for (auto &priData : data.primaryData[i]) {
            Watcher::PRIValue value;
            Convert(std::move(*(priData.begin())), value);
            info.push_back(std::move(value));
        }
    }
    if (!data.field.empty()) {
        fields[std::move(data.tableName)] = std::move(*(data.field.begin()));
    }
    watcher_->OnChange(genOrigin, fields, std::move(changeInfo));
}
} // namespace OHOS::DistributedRdb
