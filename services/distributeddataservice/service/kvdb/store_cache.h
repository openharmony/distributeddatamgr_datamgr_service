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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_STORE_CACHE_H
#define OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_STORE_CACHE_H
#include <kv_store_delegate_manager.h>
#include <chrono>
#include <memory>
#include <shared_mutex>
#include "concurrent_map.h"
#include "task_scheduler.h"
#include "kv_store_nb_delegate.h"
#include "metadata/store_meta_data.h"
#include "refbase.h"
#include "ikvstore_observer.h"

namespace OHOS::DistributedKv {
class StoreCache {
public:
    template<class T>
    struct Less {
    public:
        bool operator()(const sptr<T> &_x, const sptr<T> &_y) const
        {
            return _x.GetRefPtr() < _y.GetRefPtr();
        }
    };
    using DBStatus = DistributedDB::DBStatus;
    using DBStore = DistributedDB::KvStoreNbDelegate;
    using Store = std::shared_ptr<DBStore>;
    using DBManager = DistributedDB::KvStoreDelegateManager;
    using DBObserver = DistributedDB::KvStoreObserver;
    using DBChangeData = DistributedDB::KvStoreChangedData;
    using DBEntry = DistributedDB::Entry;
    using Observers = std::set<sptr<IKvStoreObserver>, Less<IKvStoreObserver>>;
    using StoreMetaData = OHOS::DistributedData::StoreMetaData;
    using Time = std::chrono::steady_clock::time_point;
    using DBOption = DistributedDB::KvStoreNbDelegate::Option;
    using DBSecurity = DistributedDB::SecurityOption;
    using DBPassword = DistributedDB::CipherPassword;

    struct DBStoreDelegate : public DBObserver {
        DBStoreDelegate(DBStore *delegate, std::shared_ptr<Observers> observers);
        ~DBStoreDelegate();
        operator std::shared_ptr<DBStore> ();
        bool operator<(const Time &time) const;
        bool Close(DBManager &manager);
        void OnChange(const DBChangeData &data) override;
        void SetObservers(std::shared_ptr<Observers> observers);

    private:
        std::vector<Entry> Convert(const std::list<DBEntry> &dbEntries);
        mutable Time time_;
        DBStore *delegate_ = nullptr;
        std::shared_ptr<Observers> observers_ = nullptr;
        std::shared_mutex mutex_;
    };

    Store GetStore(const StoreMetaData &data, std::shared_ptr<Observers> observers, DBStatus &status);
    void CloseStore(uint32_t tokenId, const std::string &storeId);
    void CloseExcept(const std::set<int32_t> &users);
    void SetObserver(uint32_t tokenId, const std::string &storeId, std::shared_ptr<Observers> observers);
    static DBOption GetDBOption(const StoreMetaData &data, const DBPassword &password);
    static DBSecurity GetDBSecurity(int32_t secLevel);
    static DBPassword GetDBPassword(const StoreMetaData &data);
private:
    void GarbageCollect();
    static constexpr int64_t INTERVAL = 1;
    static constexpr size_t TIME_TASK_NUM = 1;
    ConcurrentMap<uint32_t, std::map<std::string, DBStoreDelegate>> stores_;
    TaskScheduler scheduler_{ TIME_TASK_NUM, "store_cache" };
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_STORE_CACHE_H
