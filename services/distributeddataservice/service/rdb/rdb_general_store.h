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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_GENERAL_STORE_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_GENERAL_STORE_H
#include <atomic>
#include <functional>
#include <shared_mutex>

#include "metadata/store_meta_data.h"
#include "rdb_asset_loader.h"
#include "rdb_cloud.h"
#include "rdb_store.h"
#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "store/general_store.h"
#include "store/general_value.h"
#include "snapshot/snapshot.h"
namespace OHOS::DistributedRdb {
class RdbGeneralStore : public DistributedData::GeneralStore {
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
    using RdbStore = OHOS::NativeRdb::RdbStore;
    using Reference = DistributedData::Reference;
    using Snapshot = DistributedData::Snapshot;
    using BindAssets = DistributedData::BindAssets;

    explicit RdbGeneralStore(const StoreMetaData &meta);
    ~RdbGeneralStore();
    int32_t Bind(Database &database, const std::map<uint32_t, BindInfo> &bindInfos) override;
    bool IsBound() override;
    bool IsValid();
    int32_t Execute(const std::string &table, const std::string &sql) override;
    int32_t SetDistributedTables(const std::vector<std::string> &tables, int32_t type,
	    const std::vector<Reference> &references) override;
    int32_t SetTrackerTable(const std::string& tableName, const std::set<std::string>& trackerColNames,
        const std::string& extendColName) override;
    int32_t Insert(const std::string &table, VBuckets &&values) override;
    int32_t Update(const std::string &table, const std::string &setSql, Values &&values, const std::string &whereSql,
        Values &&conditions) override;
    int32_t Replace(const std::string &table, VBucket &&value) override;
    int32_t Delete(const std::string &table, const std::string &sql, Values &&args) override;
    std::shared_ptr<Cursor> Query(const std::string &table, const std::string &sql, Values &&args) override;
    std::shared_ptr<Cursor> Query(const std::string &table, GenQuery &query) override;
    int32_t Sync(
        const Devices &devices, GenQuery &query, DetailAsync async, DistributedData::SyncParam &syncParam) override;
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
    int32_t MergeMigratedData(const std::string &tableName, VBuckets&& values) override;
    std::vector<std::string> GetWaterVersion(const std::string &deviceId) override;

private:
    RdbGeneralStore(const RdbGeneralStore& rdbGeneralStore);
    RdbGeneralStore& operator=(const RdbGeneralStore& rdbGeneralStore);
    using RdbDelegate = DistributedDB::RelationalStoreDelegate;
    using RdbManager = DistributedDB::RelationalStoreManager;
    using SyncProcess = DistributedDB::SyncProcess;
    using DBBriefCB = DistributedDB::SyncStatusCallback;
    using DBProcessCB = std::function<void(const std::map<std::string, SyncProcess> &processes)>;
    static GenErr ConvertStatus(DistributedDB::DBStatus status);
    static constexpr inline uint64_t REMOTE_QUERY_TIME_OUT = 30 * 1000;
    static constexpr const char* CLOUD_GID = "cloud_gid";
    static constexpr const char* DATE_KEY = "data_key";
    static constexpr uint32_t ITER_V0 = 10000;
    static constexpr uint32_t ITER_V1 = 5000;
    static constexpr uint32_t ITERS[] = {ITER_V0, ITER_V1};
    static constexpr uint32_t ITERS_COUNT = sizeof(ITERS) / sizeof(ITERS[0]);
    class ObserverProxy : public DistributedDB::StoreObserver {
    public:
        using DBChangedIF = DistributedDB::StoreChangedData;
        using DBChangedData = DistributedDB::ChangedData;
        using DBOrigin = DistributedDB::Origin;
        using GenOrigin = Watcher::Origin;
        void OnChange(const DistributedDB::StoreChangedData &data) override;
        void OnChange(DBOrigin origin, const std::string &originalId, DBChangedData &&data) override;
        bool HasWatcher() const
        {
            return watcher_ != nullptr;
        }
    private:
        friend RdbGeneralStore;
        Watcher *watcher_ = nullptr;
        std::string storeId_;
    };
    DBBriefCB GetDBBriefCB(DetailAsync async);
    DBProcessCB GetDBProcessCB(DetailAsync async, uint32_t highMode = AUTO_SYNC_MODE);
    std::shared_ptr<Cursor> RemoteQuery(const std::string &device,
        const DistributedDB::RemoteCondition &remoteCondition);
    std::string BuildSql(const std::string& table, const std::string& statement,
        const std::vector<std::string>& columns) const;
    VBuckets QuerySql(const std::string& sql, Values &&args);
    VBuckets ExtractExtend(VBuckets& values) const;
    size_t SqlConcatenate(VBucket &value, std::string &strColumnSql, std::string &strRowValueSql);
    bool IsPrintLog(DistributedDB::DBStatus status);
    
    ObserverProxy observer_;
    RdbManager manager_;
    RdbDelegate *delegate_ = nullptr;
    DetailAsync async_ = nullptr;
    std::shared_ptr<RdbCloud> rdbCloud_ {};
    std::shared_ptr<RdbAssetLoader> rdbLoader_ {};
    BindInfo bindInfo_;
    std::atomic<bool> isBound_ = false;
    std::mutex mutex_;
    int32_t ref_ = 1;
    mutable std::shared_mutex rwMutex_;

    BindAssets snapshots_;
    DistributedData::StoreInfo storeInfo_;

    DistributedDB::DBStatus lastError_ = DistributedDB::DBStatus::OK;
    static constexpr uint32_t PRINT_ERROR_CNT = 150;
    uint32_t lastErrCnt_ = 0;
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_GENERAL_STORE_H
