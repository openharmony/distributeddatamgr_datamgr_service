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

#include "calc_kvdb_sync_data_size.h"
#include "directory_manager.h"
#include "store_cache.h"
namespace OHOS::DistributedKv {
using DBManager = DistributedDB::KvStoreDelegateManager;
using DBStore = DistributedDB::KvStoreNbDelegate;
uint32_t CalcKvSyncDataSize::CalcSyncDataSize(const StoreMetaData &data, DBStatus &status)
{
    DBStore *dbStore = nullptr;
    DBManager manager(data.appId, data.user, data.instanceId);
    manager.SetKvStoreConfig({DirectoryManager::GetInstance().GetStorePath(data)});
    StoreCache cache;
    manager.GetKvStore(data.storeId, cache.GetDBOption(data, cache.GetDBPassword(data)),
                       [&status, &dbStore](auto dbStatus, auto *tmpStore) {
                           status = dbStatus;
                           dbStore = tmpStore;
                       });

    if (status != DistributedDB::DBStatus::OK || dbStore == nullptr) {
        return 0;
    }

    uint32_t dataSize = 0;
    dbStore->CalculateSyncDataSize(data.deviceId, dataSize);
    manager.CloseKvStore(dbStore);
    return dataSize;
}
}