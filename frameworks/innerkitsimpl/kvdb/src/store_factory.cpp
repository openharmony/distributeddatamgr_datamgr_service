/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#define LOG_TAG "StoreFactory"
#include "store_factory.h"

#include "device_store_impl.h"
#include "log_print.h"
#include "security_manager.h"
#include "single_store_impl.h"
#include "store_util.h"
#include "system_api.h"
namespace OHOS::DistributedKv {
using namespace DistributedDB;
StoreFactory &StoreFactory::GetInstance()
{
    static StoreFactory instance;
    return instance;
}

StoreFactory::StoreFactory()
{
    (void)DBManager::SetProcessSystemAPIAdapter(std::make_shared<SystemApi>());
}

std::shared_ptr<SingleKvStore> StoreFactory::GetOrOpenStore(
    const AppId &appId, const StoreId &storeId, const Options &options, const std::string &path, Status &status)
{
    DBStatus dbStatus = DBStatus::OK;
    std::shared_ptr<SingleStoreImpl> kvStore;
    stores_.Compute(appId, [&](auto &, auto &stores) {
        if (stores.find(storeId) != stores.end()) {
            kvStore = stores[storeId];
            return !stores.empty();
        }
        auto dbManager = GetDBManager(path, appId);
        auto password = SecurityManager::GetInstance().GetDBPassword(appId, storeId, path);
        dbManager->GetKvStore(storeId, GetDBOption(options, password),
            [&dbManager, &kvStore, &stores, &appId, &dbStatus, &options](auto status, auto *store) {
                dbStatus = status;
                if (store == nullptr) {
                    ZLOGE("Create DBStore failed, status:%{public}d", status);
                    return;
                }
                auto release = [dbManager](auto *store) { dbManager->CloseKvStore(store); };
                auto dbStore = std::shared_ptr<DBStore>(store, release);
                if (options.kvStoreType == DEVICE_COLLABORATION) {
                    kvStore = std::make_shared<DeviceStoreImpl>(dbStore, appId, options);
                } else {
                    kvStore = std::make_shared<SingleStoreImpl>(dbStore, appId, options);
                }
                stores[dbStore->GetStoreId()] = kvStore;
            });
        return !stores.empty();
    });
    status = StoreUtil::ConvertStatus(dbStatus);
    return kvStore;
}

Status StoreFactory::Delete(const AppId &appId, const StoreId &storeId, const std::string &path)
{
    Close(appId, storeId);
    auto dbManager = GetDBManager(path, appId);
    auto status = dbManager->DeleteKvStore(storeId);
    SecurityManager::GetInstance().DelDBPassword(appId, storeId, path);
    return StoreUtil::ConvertStatus(status);
}

Status StoreFactory::Close(const AppId &appId, const StoreId &storeId)
{
    stores_.ComputeIfPresent(appId, [&storeId](auto &, auto &values) {
        for (auto it = values.begin(); it != values.end();) {
            if (!storeId.storeId.empty() && (it->first != storeId.storeId)) {
                ++it;
            } else {
                it->second->Close();
                it = values.erase(it);
            }
        }
        return !values.empty();
    });
    return SUCCESS;
}

bool StoreFactory::IsOpen(const AppId &appId, const StoreId &storeId)
{
    bool isExits = false;
    stores_.ComputeIfPresent(appId, [&storeId, &isExits](auto &, auto &values) {
        isExits = (values.find(storeId) != values.end());
        return !values.empty();
    });
    return isExits;
}

std::shared_ptr<StoreFactory::DBManager> StoreFactory::GetDBManager(const std::string &path, const AppId &appId)
{
    std::shared_ptr<DBManager> dbManager;
    dbManagers_.Compute(path, [&dbManager, &appId](const auto &path, std::shared_ptr<DBManager> &manager) {
        if (manager == nullptr) {
            manager = std::make_shared<DBManager>(appId.appId, "default");
            std::string fullPath = path + "/kvdb";
            StoreUtil::InitPath(fullPath);
            manager->SetKvStoreConfig({ fullPath });
        }
        dbManager = manager;
        return true;
    });
    return dbManager;
}

StoreFactory::DBOption StoreFactory::GetDBOption(const Options &options, const DBPassword &password) const
{
    DBOption dbOption;
    dbOption.syncDualTupleMode = true; // tuple of (appid+storeid)
    dbOption.createIfNecessary = true;
    dbOption.isMemoryDb = (!options.persistent);
    dbOption.isEncryptedDb = options.encrypt;
    if (options.encrypt) {
        dbOption.cipher = DistributedDB::CipherType::AES_256_GCM;
        dbOption.passwd = password;
    }

    if (options.kvStoreType == KvStoreType::SINGLE_VERSION) {
        dbOption.conflictResolvePolicy = DistributedDB::LAST_WIN;
    } else if (options.kvStoreType == KvStoreType::DEVICE_COLLABORATION) {
        dbOption.conflictResolvePolicy = DistributedDB::DEVICE_COLLABORATION;
    }

    dbOption.schema = options.schema;
    dbOption.createDirByStoreIdOnly = options.dataOwnership;
    dbOption.secOption = StoreUtil::GetDBSecurity(options.securityLevel);
    return dbOption;
}
} // namespace OHOS::DistributedKv