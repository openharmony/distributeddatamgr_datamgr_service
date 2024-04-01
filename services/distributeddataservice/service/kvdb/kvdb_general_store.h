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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_KVDB_GENERAL_STORE_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_KVDB_GENERAL_STORE_H

#include <atomic>
#include <functional>
#include <shared_mutex>

#include "kv_store_changed_data.h"
#include "kv_store_delegate_manager.h"
#include "kv_store_nb_delegate.h"
#include "kvstore_sync_callback.h"
#include "kvstore_sync_manager.h"
#include "metadata/store_meta_data.h"
#include "rdb_cloud.h"
#include "store/general_store.h"
#include "store/general_value.h"
#include "store_observer.h"

namespace OHOS::DistributedKv {
class KVDBGeneralStore : public DistributedData::GeneralStore {
public:
    using Cursor = DistributedData::Cursor;
    using GenQuery = DistributedData::GenQuery;
    using VBucket = DistributedData::VBucket;
    using VBuckets = DistributedData::VBuckets;
    using Value = DistributedData::Value;
    using Values = DistributedData::Values;
    using StoreMetaData = DistributedData::StoreMetaData;
    using Database = DistributedData::Database;
    using GenErr = DistributedData::GeneralError;
    using Reference = DistributedData::Reference;
    using SyncCallback = KvStoreSyncCallback;
    using SyncEnd = KvStoreSyncManager::SyncEnd;
    using DBStore = DistributedDB::KvStoreNbDelegate;
    using Store = std::shared_ptr<DBStore>;
    using DBStatus = DistributedDB::DBStatus;
    using DBOption = DistributedDB::KvStoreNbDelegate::Option;
    using DBSecurity = DistributedDB::SecurityOption;
    using DBPassword = DistributedDB::CipherPassword;
    using BindAssets = DistributedData::BindAssets;
    using DBMode = DistributedDB::SyncMode;

    explicit KVDBGeneralStore(const StoreMetaData &meta);
    ~KVDBGeneralStore();
    int32_t Bind(const std::map<std::string, std::pair<Database, BindInfo>> &cloudDBs) override;
    bool IsBound() override;
    bool IsValid();
    int32_t Execute(const std::string &table, const std::string &sql) override;
    int32_t SetDistributedTables(
        const std::vector<std::string> &tables, int32_t type, const std::vector<Reference> &references) override;
    int32_t SetTrackerTable(const std::string &tableName, const std::set<std::string> &trackerColNames,
        const std::string &extendColName) override;
    int32_t Insert(const std::string &table, VBuckets &&values) override;
    int32_t Update(const std::string &table, const std::string &setSql, Values &&values, const std::string &whereSql,
        Values &&conditions) override;
    int32_t Replace(const std::string &table, VBucket &&value) override;
    int32_t Delete(const std::string &table, const std::string &sql, Values &&args) override;
    std::shared_ptr<Cursor> Query(const std::string &table, const std::string &sql, Values &&args) override;
    std::shared_ptr<Cursor> Query(const std::string &table, GenQuery &query) override;
    int32_t Sync(const Devices &devices, int32_t mode, GenQuery &query, DetailAsync async, int32_t wait) override;
    std::shared_ptr<Cursor> PreSharing(GenQuery &query) override;
    int32_t Clean(const std::vector<std::string> &devices, int32_t mode, const std::string &tableName) override;
    int32_t Watch(int32_t origin, Watcher &watcher) override;
    int32_t Unwatch(int32_t origin, Watcher &watcher) override;
    int32_t RegisterDetailProgressObserver(DetailAsync async) override;
    int32_t UnregisterDetailProgressObserver() override;
    int32_t Close() override;
    int32_t AddRef() override;
    int32_t Release() override;
    int32_t BindSnapshots(
        std::shared_ptr<std::map<std::string, std::shared_ptr<DistributedData::Snapshot>>> bindAssets) override;
    int32_t MergeMigratedData(const std::string &tableName, VBuckets &&values) override;

    static DBPassword GetDBPassword(const StoreMetaData &data);
    static DBOption GetDBOption(const StoreMetaData &data, const DBPassword &password);
    static DBSecurity GetDBSecurity(int32_t secLevel);

private:
    KVDBGeneralStore(const KVDBGeneralStore &store);
    KVDBGeneralStore &operator=(const KVDBGeneralStore &store);
    using KvDelegate = DistributedDB::KvStoreNbDelegate;
    using KvManager = DistributedDB::KvStoreDelegateManager;
    using SyncProcess = DistributedDB::SyncProcess;
    using DBSyncCallback = std::function<void(const std::map<std::string, DBStatus> &status)>;
    using DBProcessCB = std::function<void(const std::map<std::string, SyncProcess> &processes)>;
    static GenErr ConvertStatus(DistributedDB::DBStatus status);
    DBSyncCallback GetDBSyncCompleteCB(DetailAsync async);
    class ObserverProxy : public DistributedDB::KvStoreObserver {
    public:
        using DBOrigin = DistributedDB::Origin;
        using GenOrigin = Watcher::Origin;
        ~ObserverProxy() = default;
        void OnChange(DistributedDB::Origin origin, const std::string &originalId, DistributedDB::ChangedData &&data);
        void OnChange(const DistributedDB::KvStoreChangedData &data) override;
        void ConvertChangeData(const std::list<DistributedDB::Entry> &entries, std::vector<Values> &values);
        bool HasWatcher() const
        {
            return watcher_ != nullptr;
        }

    private:
        friend KVDBGeneralStore;
        Watcher *watcher_ = nullptr;
        std::string storeId_;
    };

    ObserverProxy observer_;
    KvManager manager_;
    KvDelegate *delegate_ = nullptr;
    std::map<std::string, std::shared_ptr<DistributedDB::ICloudDb>> dbClouds_{};
    std::set<BindInfo> bindInfos_;
    std::atomic<bool> isBound_ = false;
    std::mutex mutex_;
    int32_t ref_ = 1;
    mutable std::shared_mutex rwMutex_;
    DistributedData::StoreInfo storeInfo_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_KVDB_GENERAL_STORE_H
