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
#define LOG_TAG "StoreManager"
#include "store_manager.h"

#include "kvdb_service_client.h"
#include "security_manager.h"
#include "store_factory.h"
#include "log_print.h"
namespace OHOS::DistributedKv {
StoreManager &StoreManager::GetInstance()
{
    static StoreManager instance;
    return instance;
}

std::shared_ptr<SingleKvStore> StoreManager::GetKVStore(const AppId &appId, const StoreId &storeId,
    const Options &options, Status &status)
{
    ZLOGD("appId:%{public}s, storeId:%{public}s type:%{public}d area:%{public}d dir:%{public}s", appId.appId.c_str(),
        storeId.storeId.c_str(), options.kvStoreType, options.area, options.baseDir.c_str());
    if (!appId.IsValid() || !storeId.IsValid() || !options.IsValidType()) {
        status = INVALID_ARGUMENT;
        return nullptr;
    }

    auto service = KVDBServiceClient::GetInstance();
    if (service != nullptr) {
        status = service->BeforeCreate(appId, storeId, options);
    }

    if (status == STORE_META_CHANGED) {
        ZLOGE("appId:%{public}s, storeId:%{public}s type:%{public}d encrypt:%{public}d", appId.appId.c_str(),
            storeId.storeId.c_str(), options.kvStoreType, options.encrypt);
        return nullptr;
    }

    bool isCreate = false;
    auto kvStore = StoreFactory::GetInstance().GetOrOpenStore(appId, storeId, options, status, isCreate);
    if (isCreate) {
        auto password = SecurityManager::GetInstance().GetDBPassword(storeId, options.baseDir, options.encrypt);
        std::vector<uint8_t> pwd(password.GetData(), password.GetData() + password.GetSize());
        if (service != nullptr) {
            // delay notify
            service->AfterCreate(appId, storeId, options, pwd);
        }
        pwd.assign(pwd.size(), 0);
    }
    return kvStore;
}

Status StoreManager::CloseKVStore(const AppId &appId, const StoreId &storeId)
{
    ZLOGD("appId:%{public}s, storeId:%{public}s", appId.appId.c_str(), storeId.storeId.c_str());
    if (!appId.IsValid() || !storeId.IsValid()) {
        return INVALID_ARGUMENT;
    }

    return StoreFactory::GetInstance().Close(appId, storeId);
}

Status StoreManager::CloseKVStore(const AppId &appId, std::shared_ptr<SingleKvStore> &kvStore)
{
    if (!appId.IsValid() || kvStore == nullptr) {
        return INVALID_ARGUMENT;
    }

    StoreId storeId{ kvStore->GetStoreId() };
    kvStore = nullptr;
    return StoreFactory::GetInstance().Close(appId, storeId);
}

Status StoreManager::CloseAllKVStore(const AppId &appId)
{
    ZLOGD("appId:%{public}s", appId.appId.c_str());
    if (!appId.IsValid()) {
        return INVALID_ARGUMENT;
    }

    return StoreFactory::GetInstance().Close(appId, { "" }, true);
}

Status StoreManager::Delete(const AppId &appId, const StoreId &storeId, const std::string &path)
{
    ZLOGD("appId:%{public}s, storeId:%{public}s dir:%{public}s", appId.appId.c_str(), storeId.storeId.c_str(),
        path.c_str());
    if (!appId.IsValid() || !storeId.IsValid()) {
        return INVALID_ARGUMENT;
    }

    auto service = KVDBServiceClient::GetInstance();
    if (service != nullptr) {
        service->Delete(appId, storeId);
    }
    return StoreFactory::GetInstance().Delete(appId, storeId, path);
}
} // namespace OHOS::DistributedKv