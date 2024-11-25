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

#include "concurrent_map.h"
#include "metadata/store_meta_data.h"
#include "rdb_asset_loader.h"
#include "rdb_cloud.h"
#include "rdb_store.h"
#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "snapshot/snapshot.h"
#include "store/general_store.h"
#include "store/general_value.h"
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

    static void OnSyncStart(const DistributedData::StoreInfo &storeInfo, uint32_t flag, uint32_t syncMode,
        uint32_t traceId, uint32_t syncCount);
    static void OnSyncFinish(const DistributedData::StoreInfo &storeInfo, uint32_t flag, uint32_t syncMode,
        uint32_t traceId);
    void SetExecutor(std::shared_ptr<Executor> executor) override;
    int32_t Bind(Database &database, const std::map<uint32_t, BindInfo> &bindInfos,
        const CloudConfig &config) override;
    bool IsBound() override;
    bool IsValid();
    int32_t Execute(const std::string &table, const std::string &sql) override;
    int32_t SetDistributedTables(const std::vector<std::string> &tables, int32_t type,
	    const std::vector<Reference> &references) override;
    int32_t SetTrackerTable(const std::string& tableName, const std::set<std::string>& trackerColNames,
        const std::set<std::string> &extendColNames, bool isForceUpgrade = false) override;
    int32_t Insert(const std::string &table, VBuckets &&values) override;
    int32_t Update(const std::string &table, const std::string &setSql, Values &&values, const std::string &whereSql,
        Values &&conditions) override;
    int32_t Replace(const std::string &table, VBucket &&value) override;
    int32_t Delete(const std::string &table, const std::string &sql, Values &&args) override;
    std::pair<int32_t, std::shared_ptr<Cursor>> Query(const std::string &table, const std::string &sql,
        Values &&args) override;
    std::pair<int32_t, std::shared_ptr<Cursor>> Query(const std::string &table, GenQuery &query) override;
    int32_t Sync(
        const Devices &devices, GenQuery &query, DetailAsync async, DistributedData::SyncParam &syncParam) override;
    std::pair<int32_t, std::shared_ptr<Cursor>> PreSharing(GenQuery &query) override;
    int32_t Clean(const std::vector<std::string> &devices, int32_t mode, const std::string &tableName) override;
    int32_t Watch(int32_t origin, Watcher &watcher) override;
    int32_t Unwatch(int32_t origin, Watcher &watcher) override;
    int32_t RegisterDetailProgressObserver(DetailAsync async) override;
    int32_t UnregisterDetailProgressObserver() override;
    int32_t Close(bool isForce = false) override;
    int32_t AddRef() override;
    int32_t Release() override;
    int32_t BindSnapshots(std::shared_ptr<std::map<std::string, std::shared_ptr<Snapshot>>> bindAssets) override;
    int32_t MergeMigratedData(const std::string &tableName, VBuckets&& values) override;
    int32_t CleanTrackerData(const std::string &tableName, int64_t cursor) override;
    std::vector<std::string> GetWaterVersion(const std::string &deviceId) override;
    std::pair<int32_t, uint32_t> LockCloudDB() override;
    int32_t UnLockCloudDB() override;

private:
    RdbGeneralStore(const RdbGeneralStore& rdbGeneralStore);
    RdbGeneralStore& operator=(const RdbGeneralStore& rdbGeneralStore);
    using RdbDelegate = DistributedDB::RelationalStoreDelegate;
    using RdbManager = DistributedDB::RelationalStoreManager;
    using SyncProcess = DistributedDB::SyncProcess;
    using DBBriefCB = DistributedDB::SyncStatusCallback;
    using DBProcessCB = std::function<void(const std::map<std::string, SyncProcess> &processes)>;
    using TaskId = ExecutorPool::TaskId;
    using Time = std::chrono::steady_clock::time_point;
    using SyncId = uint64_t;
    static GenErr ConvertStatus(DistributedDB::DBStatus status);
    // GetIntersection and return results in the order of collecter1
    static std::vector<std::string> GetIntersection(std::vector<std::string> &&syncTables,
        const std::set<std::string> &localTables);
    static constexpr inline uint64_t REMOTE_QUERY_TIME_OUT = 30 * 1000;
    static constexpr int64_t INTERVAL = 1;
    static constexpr const char* CLOUD_GID = "cloud_gid";
    static constexpr const char* DATE_KEY = "data_key";
    static constexpr const char* QUERY_TABLES_SQL = "select name from sqlite_master where type = 'table';";
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
        enum ChangeType {
            CLOUD_DATA_CHANGE = 0,
            CLOUD_DATA_CLEAN
        };
        void PostDataChange(const StoreMetaData &meta, const std::vector<std::string> &tables, ChangeType type);
        friend RdbGeneralStore;
        Watcher *watcher_ = nullptr;
        std::string storeId_;
        StoreMetaData meta_;
    };
    DBBriefCB GetDBBriefCB(DetailAsync async);
    DBProcessCB GetCB(SyncId syncId);
    DBProcessCB GetDBProcessCB(DetailAsync async, uint32_t syncMode, SyncId syncId,
        uint32_t highMode = AUTO_SYNC_MODE);
    Executor::Task GetFinishTask(SyncId syncId);
    std::shared_ptr<Cursor> RemoteQuery(const std::string &device,
        const DistributedDB::RemoteCondition &remoteCondition);
    std::string BuildSql(const std::string& table, const std::string& statement,
        const std::vector<std::string>& columns) const;
    std::pair<int32_t, VBuckets> QuerySql(const std::string& sql, Values &&args);
    std::set<std::string> GetTables();
    VBuckets ExtractExtend(VBuckets& values) const;
    size_t SqlConcatenate(VBucket &value, std::string &strColumnSql, std::string &strRowValueSql);
    bool IsPrintLog(DistributedDB::DBStatus status);
    std::shared_ptr<RdbCloud> GetRdbCloud() const;
    bool IsFinished(uint64_t syncId) const;
    void RemoveTasks();

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
    mutable std::shared_timed_mutex rwMutex_;

    BindAssets snapshots_;
    DistributedData::StoreInfo storeInfo_;

    DistributedDB::DBStatus lastError_ = DistributedDB::DBStatus::OK;
    static constexpr uint32_t PRINT_ERROR_CNT = 150;
    uint32_t lastErrCnt_ = 0;
    uint32_t syncNotifyFlag_ = 0;
    std::atomic<SyncId> syncTaskId_ = 0;
    std::shared_mutex asyncMutex_ {};
    mutable std::shared_mutex rdbCloudMutex_;
    struct FinishTask {
        TaskId taskId = Executor::INVALID_TASK_ID;
        DBProcessCB cb = nullptr;
    };
    std::shared_ptr<Executor> executor_ = nullptr;
    std::shared_ptr<ConcurrentMap<SyncId, FinishTask>> tasks_;
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_GENERAL_STORE_H
