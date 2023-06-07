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

#include "cloud/asset_loader.h"
#include "cloud/cloud_db.h"
#include "cloud/schema_meta.h"
#include "crypto_manager.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "rdb_helper.h"
#include "rdb_query.h"
#include "rdb_syncer.h"
#include "relational_store_manager.h"
namespace OHOS::DistributedRdb {
using namespace DistributedData;
using namespace DistributedDB;
using namespace NativeRdb;
using DBField = DistributedDB::Field;
using DBTable = DistributedDB::TableSchema;
using DBSchema = DistributedDB::DataBaseSchema;
class RdbOpenCallbackImpl : public RdbOpenCallback {
public:
    int OnCreate(RdbStore &rdbStore) override
    {
        return NativeRdb::E_OK;
    }
    int OnUpgrade(RdbStore &rdbStore, int oldVersion, int newVersion) override
    {
        return NativeRdb::E_OK;
    }
};

RdbGeneralStore::RdbGeneralStore(const StoreMetaData &meta) : manager_(meta.appId, meta.user, meta.instanceId)
{
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
    option.observer = nullptr;
    manager_.OpenStore(meta.dataDir, meta.storeId, option, delegate_);
    RdbStoreConfig config(meta.dataDir);
    config.SetCreateNecessary(false);
    RdbOpenCallbackImpl callback;
    int32_t errCode = NativeRdb::E_OK;
    store_ = RdbHelper::GetRdbStore(config, meta.version, callback, errCode);
    if (errCode != NativeRdb::E_OK) {
        ZLOGE("GetRdbStore failed, errCode is %{public}d, storeId is %{public}s", errCode, meta.storeId.c_str());
    }
}

RdbGeneralStore::~RdbGeneralStore()
{
    manager_.CloseStore(delegate_);
    delegate_ = nullptr;
    store_ = nullptr;
    bindInfo_.loader_ = nullptr;
    bindInfo_.db_->Close();
    bindInfo_.db_ = nullptr;
}

int32_t RdbGeneralStore::Bind(const Database &database, BindInfo bindInfo)
{
    bindInfo_ = std::move(bindInfo);
    // SetCloudDB
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
    // SetCloudDbSchema
    return GeneralError::E_NOT_SUPPORT;
}

int32_t RdbGeneralStore::Close()
{
    manager_.CloseStore(delegate_);
    delegate_ = nullptr;
    store_ = nullptr;
    bindInfo_.loader_ = nullptr;
    bindInfo_.db_->Close();
    bindInfo_.db_ = nullptr;
    return 0;
}

int32_t RdbGeneralStore::Execute(const std::string &table, const std::string &sql)
{
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

std::shared_ptr<Cursor> RdbGeneralStore::Query(const std::string &table, const std::string &sql, Values &&args)
{
    return std::shared_ptr<Cursor>();
}

std::shared_ptr<Cursor> RdbGeneralStore::Query(const std::string &table, GenQuery &query)
{
    return std::shared_ptr<Cursor>();
}

int32_t RdbGeneralStore::Sync(const Devices &devices, int32_t mode, GenQuery &query, DetailAsync async, int32_t wait)
{
    return GeneralError::E_NOT_SUPPORT;
}

int32_t RdbGeneralStore::Watch(int32_t origin, Watcher &watcher)
{
    return GeneralError::E_NOT_SUPPORT;
}

int32_t RdbGeneralStore::Unwatch(int32_t origin, Watcher &watcher)
{
    return GeneralError::E_NOT_SUPPORT;
}
} // namespace OHOS::DistributedRdb
