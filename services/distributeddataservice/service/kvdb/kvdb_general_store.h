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
#include "store/general_store.h"
#include "store/general_value.h"
#include "store_observer.h"

namespace OHOS::DistributedKv {
using namespace DistributedData;
class KVDBGeneralStore : public DistributedData::GeneralStore {
public:
    using Value = DistributedData::Value;
    using GenErr = DistributedData::GeneralError;
    using DBStatus = DistributedDB::DBStatus;
    using DBOption = DistributedDB::KvStoreNbDelegate::Option;
    using DBSecurity = DistributedDB::SecurityOption;
    using DBPassword = DistributedDB::CipherPassword;

    explicit KVDBGeneralStore(const StoreMetaData &meta);
    ~KVDBGeneralStore();
    int32_t Bind(Database &database, const std::map<uint32_t, BindInfo> &bindInfos) override;
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
    int32_t Sync(const Devices &devices, GenQuery &query, DetailAsync async, SyncParam &syncParm) override;
    std::shared_ptr<Cursor> PreSharing(GenQuery &query) override;
    int32_t Clean(const std::vector<std::string> &devices, int32_t mode, const std::string &tableName) override;
    int32_t Watch(int32_t origin, Watcher &watcher) override;
    int32_t Unwatch(int32_t origin, Watcher &watcher) override;
    int32_t RegisterDetailProgressObserver(DetailAsync async) override;
    int32_t UnregisterDetailProgressObserver() override;
    int32_t Close() override;
    int32_t AddRef() override;
    int32_t Release() override;
    int32_t BindSnapshots(std::shared_ptr<std::map<std::string, std::shared_ptr<Snapshot>>> bindAssets) override;
    int32_t MergeMigratedData(const std::string &tableName, VBuckets &&values) override;
    std::vector<std::string> GetWaterVersion(const std::string &deviceId) override;

    static DBPassword GetDBPassword(const StoreMetaData &data);
    static DBOption GetDBOption(const StoreMetaData &data, const DBPassword &password);
    static DBSecurity GetDBSecurity(int32_t secLevel);

private:
    using KvDelegate = DistributedDB::KvStoreNbDelegate;
    using KvManager = DistributedDB::KvStoreDelegateManager;
    using SyncProcess = DistributedDB::SyncProcess;
    using DBSyncCallback = std::function<void(const std::map<std::string, DBStatus> &status)>;
    using DBProcessCB = std::function<void(const std::map<std::string, SyncProcess> &processes)>;
    static GenErr ConvertStatus(DBStatus status);
    DBSyncCallback GetDBSyncCompleteCB(DetailAsync async);
    DBProcessCB GetDBProcessCB(DetailAsync async);
    DBStatus CloudSync(
        const Devices &devices, DistributedDB::SyncMode &cloudSyncMode, DetailAsync async, int64_t wait);
    void InitWaterVersion(const StoreMetaData &meta);
    class ObserverProxy : public DistributedDB::KvStoreObserver {
    public:
        using DBOrigin = DistributedDB::Origin;
        using DBChangeData = DistributedDB::ChangedData;
        using DBEntry = DistributedDB::Entry;
        using GenOrigin = Watcher::Origin;
        ~ObserverProxy() = default;
        void OnChange(DBOrigin origin, const std::string &originalId, DBChangeData &&data) override;
        void OnChange(const DistributedDB::KvStoreChangedData &data) override;
        void ConvertChangeData(const std::list<DBEntry> &entries, std::vector<Values> &values);
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
    StoreInfo storeInfo_;
    std::function<void()> callback_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_KVDB_GENERAL_STORE_H
