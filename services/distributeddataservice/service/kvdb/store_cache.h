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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_SERVICE_KVDB_STORE_CACHE_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_SERVICE_KVDB_STORE_CACHE_H
#include <kv_store_delegate_manager.h>

#include <chrono>
#include <memory>

#include "concurrent_map.h"
#include "kv_scheduler.h"
#include "kv_store_nb_delegate.h"
#include "metadata/store_meta_data.h"
namespace OHOS::DistributedKv {
class StoreCache {
public:
    using DBStatus = DistributedDB::DBStatus;
    using DBStore = DistributedDB::KvStoreNbDelegate;
    using StoreMetaData = OHOS::DistributedData::StoreMetaData;
    using Time = std::chrono::system_clock::time_point;
    struct DBStoreDelegate {
        DBStoreDelegate(DBStore *delegate);
        ~DBStoreDelegate();
        operator DBStore *() const;
        bool operator < (const Time &time) const;

    private:
        mutable Time time_;
        DBStore *delegate_;
    };
    std::shared_ptr<DBStore> GetStore(const StoreMetaData &data, DBStatus &status);

private:
    using DBManager = DistributedDB::KvStoreDelegateManager;
    using DBOption = DistributedDB::KvStoreNbDelegate::Option;
    using DBSecurity = DistributedDB::SecurityOption;
    void CollectGarbage();
    DBOption GetDBOption(const StoreMetaData &data) const;
    DBSecurity GetDBSecurity(int32_t secLevel) const;
    static constexpr int64_t INTERVAL = 1;
    static constexpr size_t TIME_TASK_NUM = 1;
    ConcurrentMap<uint32_t, std::map<std::string, DBStoreDelegate>> stores_;
    KvScheduler scheduler_{ TIME_TASK_NUM };
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_SERVICE_KVDB_STORE_CACHE_H
