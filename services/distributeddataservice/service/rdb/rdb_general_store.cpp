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

#include "crypto_manager.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "rdb_helper.h"
#include "rdb_query.h"
#include "rdb_syncer.h"
#include "relational_store_manager.h"
#include "store/general_watcher.h"
namespace OHOS::DistributedRdb {
using namespace DistributedData;
using namespace DistributedDB;
using namespace OHOS::NativeRdb;
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

int32_t RdbGeneralStore::Close()
{
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

int32_t RdbGeneralStore::Watch(int32_t origin, Watcher &watcher)
{
    watcher_ = &watcher;
    return GeneralError::E_NOT_SUPPORT;
}

int32_t RdbGeneralStore::Unwatch(int32_t origin, Watcher &watcher)
{
    return 0;
}

int32_t RdbGeneralStore::Sync(const Devices &devices, int32_t mode, GenQuery &query, Async async, int32_t wait)
{
    RdbQuery *rdbQuery = nullptr;
    auto ret = query.QueryInterface(rdbQuery);
    if (ret != GeneralError::E_OK || rdbQuery == nullptr) {
        return GeneralError::E_OK;
    }
    auto status = delegate_->Sync(
        devices, DistributedDB::SyncMode(mode), RdbSyncer::MakeQuery(rdbQuery->predicates_.GetDistributedPredicates()),
        [async](const std::map<std::string, std::vector<TableStatus>> &result) {
            std::map<std::string, std::map<std::string, int32_t>> detail;
            for (auto &[key, tables] : result) {
                auto value = detail[key];
                for (auto &table : tables) {
                    value[std::move(table.tableName)] = table.status;
                }
            }
            async(std::move(detail));
        },
        wait);
    return status == DistributedDB::OK ? GeneralError::E_OK : GeneralError::E_ERROR;
}

int32_t RdbGeneralStore::Bind(const SchemaMeta &schemaMeta, std::shared_ptr<CloudDB> cloudDb)
{
    cloudDb_ = std::move(cloudDb);
    return 0;
}
} // namespace OHOS::DistributedRdb
