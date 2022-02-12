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
#include "relational_store_instance.h"

#include <thread>
#include <algorithm>

#include "db_common.h"
#include "db_errno.h"
#include "sqlite_relational_store.h"
#include "log_print.h"

namespace DistributedDB {
RelationalStoreInstance *RelationalStoreInstance::instance_ = nullptr;
std::mutex RelationalStoreInstance::instanceLock_;

static std::mutex storeLock_;
static std::map<std::string, IRelationalStore *> dbs_;

RelationalStoreInstance::RelationalStoreInstance()
{}

RelationalStoreInstance *RelationalStoreInstance::GetInstance()
{
    std::lock_guard<std::mutex> lockGuard(instanceLock_);
    if (instance_ == nullptr) {
        instance_ = new (std::nothrow) RelationalStoreInstance();
        if (instance_ == nullptr) {
            LOGE("failed to new RelationalStoreManager!");
            return nullptr;
        }
    }
    return instance_;
}

int RelationalStoreInstance::CheckDatabaseFileStatus(const std::string &id)
{
    std::lock_guard<std::mutex> lockGuard(storeLock_);
    if (dbs_.count(id) != 0 && dbs_[id] != nullptr) {
        return -E_BUSY;
    }
    return E_OK;
}

static IRelationalStore *GetFromCache(const RelationalDBProperties &properties, int &errCode)
{
    errCode = E_OK;
    std::string identifier = properties.GetStringProp(RelationalDBProperties::IDENTIFIER_DATA, "");
    auto iter = dbs_.find(identifier);
    if (iter == dbs_.end()) {
        errCode = -E_NOT_FOUND;
        return nullptr;
    }

    auto *db = iter->second;
    if (db == nullptr) {
        LOGE("Store cache is nullptr, there may be a logic error");
        errCode = -E_INTERNAL_ERROR;
        return nullptr;
    }
    db->IncObjRef(db);
    return db;
}

// Save to IKvDB to the global map
void RelationalStoreInstance::RemoveKvDBFromCache(const RelationalDBProperties &properties)
{
    std::lock_guard<std::mutex> lockGuard(storeLock_);
    std::string identifier = properties.GetStringProp(RelationalDBProperties::IDENTIFIER_DATA, "");
    dbs_.erase(identifier);
}

void RelationalStoreInstance::SaveKvDBToCache(IRelationalStore *store, const RelationalDBProperties &properties)
{
    if (store == nullptr) {
        return;
    }

    {
        std::string identifier = properties.GetStringProp(RelationalDBProperties::IDENTIFIER_DATA, "");
        store->WakeUpSyncer();
        if (dbs_.count(identifier) == 0) {
            dbs_.insert(std::pair<std::string, IRelationalStore *>(identifier, store));
        }
    }
}

IRelationalStore *RelationalStoreInstance::OpenDatabase(const RelationalDBProperties &properties, int &errCode)
{
    auto db = new (std::nothrow) SQLiteRelationalStore();
    if (db == nullptr) {
        LOGE("Failed to get relational store! err:%d", errCode);
        return nullptr;
    }

    db->OnClose([this, properties]() {
        LOGI("Remove from the cache");
        this->RemoveKvDBFromCache(properties);
    });

    errCode = db->Open(properties);
    if (errCode != E_OK) {
        LOGE("Failed to open db! err:%d", errCode);
        RefObject::KillAndDecObjRef(db);
        return nullptr;
    }

    SaveKvDBToCache(db, properties);
    return db;
}

IRelationalStore *RelationalStoreInstance::GetDataBase(const RelationalDBProperties &properties, int &errCode)
{
    std::lock_guard<std::mutex> lockGuard(storeLock_);
    auto *db = GetFromCache(properties, errCode);
    if (db != nullptr) {
        LOGD("Get db from cache.");
        return db;
    }

    // file lock
    RelationalStoreInstance *manager = RelationalStoreInstance::GetInstance();
    if (manager == nullptr) {
        errCode = -E_OUT_OF_MEMORY;
        return nullptr;
    }

    db = manager->OpenDatabase(properties, errCode);
    if (errCode != E_OK) {
        LOGE("Create data base failed, errCode = [%d]", errCode);
    }
    return db;
}

RelationalStoreConnection *RelationalStoreInstance::GetDatabaseConnection(const RelationalDBProperties &properties,
    int &errCode)
{
    std::string identifier = properties.GetStringProp(KvDBProperties::IDENTIFIER_DATA, "");
    LOGD("Begin to get [%s] database connection.", STR_MASK(DBCommon::TransferStringToHex(identifier)));
    IRelationalStore *db = GetDataBase(properties, errCode);
    if (db == nullptr) {
        LOGE("Failed to open the db:%d", errCode);
        return nullptr;
    }

    std::string canonicalDir = properties.GetStringProp(KvDBProperties::DATA_DIR, "");
    if (canonicalDir.empty() || canonicalDir != db->GetStorePath()) {
        LOGE("Failed to check store path, the input path does not match with cached store.");
        errCode = -E_INVALID_ARGS;
        RefObject::DecObjRef(db);
        return nullptr;
    }

    auto connection = db->GetDBConnection(errCode);
    if (connection == nullptr) { // not kill db, Other operations like import may be used concurrently
        LOGE("Failed to get the db connect for delegate:%d", errCode);
    }

    RefObject::DecObjRef(db); // restore the reference increased by the cache.
    return connection;
}
} // namespace DistributedDB
#endif