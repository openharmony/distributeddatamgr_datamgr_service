/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_SYNC_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_SYNC_MANAGER_H

#include "cloud/cloud_event.h"
#include "cloud/cloud_info.h"
#include "cloud/cloud_last_sync_info.h"
#include "cloud/sync_strategy.h"
#include "cloud_types.h"
#include "cloud/sync_event.h"
#include "concurrent_map.h"
#include "eventcenter/event.h"
#include "executor_pool.h"
#include "metadata/store_meta_data_local.h"
#include "store/auto_cache.h"
#include "store/general_store.h"
#include "store/general_value.h"
#include "utils/ref_count.h"
#include "radar_reporter.h"

namespace OHOS::CloudData {
class SyncManager {
public:
    using CloudLastSyncInfo = DistributedData::CloudLastSyncInfo;
    using GenAsync = DistributedData::GenAsync;
    using GenStore = DistributedData::GeneralStore;
    using GenQuery = DistributedData::GenQuery;
    using RefCount = DistributedData::RefCount;
    using AutoCache = DistributedData::AutoCache;
    using StoreMetaData = DistributedData::StoreMetaData;
    using SchemaMeta = DistributedData::SchemaMeta;
    using TraceIds = std::map<std::string, std::string>;
    using SyncStage = DistributedData::SyncStage;
    using ReportParam = DistributedData::ReportParam;
    static std::pair<int32_t, AutoCache::Store> GetStore(const StoreMetaData &meta, int32_t user, bool mustBind = true);
    class SyncInfo final {
    public:
        using Store = std::string;
        using Stores = std::vector<Store>;
        using Tables = std::vector<std::string>;
        struct Param {
            int32_t user;
            std::string bundleName;
            Store store;
            Tables tables;
            int32_t triggerMode = 0;
            std::string prepareTraceId;
        };
        using MutliStoreTables = std::map<Store, Tables>;
        explicit SyncInfo(int32_t user, const std::string &bundleName = "", const Store &store = "",
            const Tables &tables = {}, int32_t triggerMode = 0);
        SyncInfo(int32_t user, const std::string &bundleName, const Stores &stores);
        SyncInfo(int32_t user, const std::string &bundleName, const MutliStoreTables &tables);
        explicit SyncInfo(const Param &param);
        void SetMode(int32_t mode);
        void SetWait(int32_t wait);
        void SetAsyncDetail(GenAsync asyncDetail);
        void SetQuery(std::shared_ptr<GenQuery> query);
        void SetError(int32_t code) const;
        void SetCompensation(bool isCompensation);
        void SetTriggerMode(int32_t triggerMode);
        void SetPrepareTraceId(const std::string &prepareTraceId);
        std::shared_ptr<GenQuery> GenerateQuery(const std::string &store, const Tables &tables);
        bool Contains(const std::string &storeName);
        inline static constexpr const char *DEFAULT_ID = "default";

    private:
        friend SyncManager;
        uint64_t syncId_ = 0;
        int32_t mode_ = GenStore::MixMode(GenStore::CLOUD_TIME_FIRST, GenStore::AUTO_SYNC_MODE);
        int32_t user_ = 0;
        int32_t wait_ = 0;
        std::string id_ = DEFAULT_ID;
        std::string bundleName_;
        std::map<std::string, std::vector<std::string>> tables_;
        GenAsync async_;
        std::shared_ptr<GenQuery> query_;
        bool isCompensation_ = false;
        int32_t triggerMode_ = 0;
        std::string prepareTraceId_;
    };
    SyncManager();
    ~SyncManager();
    int32_t Bind(std::shared_ptr<ExecutorPool> executor);
    int32_t DoCloudSync(SyncInfo syncInfo);
    int32_t StopCloudSync(int32_t user = 0);
    std::pair<int32_t, std::map<std::string, CloudLastSyncInfo>> QueryLastSyncInfo(
        const std::vector<QueryKey> &queryKeys);
    void Report(const ReportParam &reportParam);
    void OnScreenUnlocked(int32_t user);
    void CleanCompensateSync(int32_t userId);

private:
    using Event = DistributedData::Event;
    using Task = ExecutorPool::Task;
    using TaskId = ExecutorPool::TaskId;
    using Duration = ExecutorPool::Duration;
    using Retryer =
        std::function<bool(Duration interval, int32_t status, int32_t dbCode, const std::string &prepareTraceId)>;
    using CloudInfo = DistributedData::CloudInfo;
    using StoreInfo = DistributedData::StoreInfo;
    using SyncStrategy = DistributedData::SyncStrategy;
    using SyncId = uint64_t;
    using GeneralError = DistributedData::GeneralError;
    using GenProgress = DistributedData::GenProgress;
    using GenDetails = DistributedData::GenDetails;

    static constexpr ExecutorPool::Duration RETRY_INTERVAL = std::chrono::seconds(10);  // second
    static constexpr ExecutorPool::Duration LOCKED_INTERVAL = std::chrono::seconds(30); // second
    static constexpr ExecutorPool::Duration BUSY_INTERVAL = std::chrono::seconds(180);  // second
    static constexpr int32_t RETRY_TIMES = 6;                                           // normal retry
    static constexpr int32_t CLIENT_RETRY_TIMES = 3;                                    // normal retry
    static constexpr uint64_t USER_MARK = 0xFFFFFFFF00000000;                           // high 32 bit
    static constexpr int32_t MV_BIT = 32;
    static constexpr int32_t EXPIRATION_TIME = 6 * 60 * 60 * 1000;                      // 6 hours

    static uint64_t GenerateId(int32_t user);
    static ExecutorPool::Duration GetInterval(int32_t code);
    static std::map<uint32_t, GenStore::BindInfo> GetBindInfos(
        const StoreMetaData &meta, const std::vector<int32_t> &users, const DistributedData::Database &schemaDatabase);
    static std::string GetAccountId(int32_t user);
    static std::vector<std::tuple<QueryKey, uint64_t>> GetCloudSyncInfo(const SyncInfo &info, CloudInfo &cloud);
    static std::vector<SchemaMeta> GetSchemaMeta(const CloudInfo &cloud, const std::string &bundleName);
    static bool NeedGetCloudInfo(CloudInfo &cloud);
    static GeneralError IsValid(SyncInfo &info, CloudInfo &cloud);
    Task GetSyncTask(int32_t times, bool retry, RefCount ref, SyncInfo &&syncInfo);
    void UpdateSchema(const SyncInfo &syncInfo);
    std::function<void(const Event &)> GetSyncHandler(Retryer retryer);
    std::function<void(const Event &)> GetClientChangeHandler();
    Retryer GetRetryer(int32_t times, const SyncInfo &syncInfo, int32_t user);
    RefCount GenSyncRef(uint64_t syncId);
    int32_t Compare(uint64_t syncId, int32_t user);
    void UpdateStartSyncInfo(const std::vector<std::tuple<QueryKey, uint64_t>> &cloudSyncInfos);
    void UpdateFinishSyncInfo(const QueryKey &queryKey, uint64_t syncId, int32_t code);
    std::function<void(const DistributedData::GenDetails &result)> GetCallback(const GenAsync &async,
        const StoreInfo &storeInfo, int32_t triggerMode, const std::string &prepareTraceId, int32_t user);
    std::function<void()> GetPostEventTask(const std::vector<SchemaMeta> &schemas, CloudInfo &cloud, SyncInfo &info,
        bool retry, const TraceIds &traceIds);
    void DoExceptionalCallback(const GenAsync &async, GenDetails &details, const StoreInfo &storeInfo,
        const std::string &prepareTraceId, int32_t code = GeneralError::E_ERROR);
    bool InitDefaultUser(int32_t &user);
    std::function<void(const DistributedData::GenDetails &result)> RetryCallback(const StoreInfo &storeInfo,
        Retryer retryer, int32_t triggerMode, const std::string &prepareTraceId, int32_t user);
    static std::map<std::string, CloudLastSyncInfo> GetLastResults(
        const std::string &storeId, std::map<SyncId, CloudLastSyncInfo> &infos);
    void BatchUpdateFinishState(const std::vector<std::tuple<QueryKey, uint64_t>> &cloudSyncInfos, int32_t code);
    bool NeedSaveSyncInfo(const QueryKey &queryKey);
    std::function<void(const Event &)> GetLockChangeHandler();
    void BatchReport(int32_t userId, const TraceIds &traceIds, SyncStage syncStage, int32_t errCode);
    TraceIds GetPrepareTraceId(const SyncInfo &info, const CloudInfo &cloud);
    std::pair<bool, StoreMetaData> GetMetaData(const StoreInfo &storeInfo);
    void AddCompensateSync(const StoreMetaData &meta);
    static void Report(
        const std::string &faultType, const std::string &bundleName, int32_t errCode, const std::string &appendix);
    void ReportSyncEvent(const DistributedData::SyncEvent &evt, DistributedDataDfx::BizState bizState, int32_t code);
    std::pair<std::string, CloudLastSyncInfo> GetLastSyncInfoFromMeta(const QueryKey &queryKey);
    static void SaveLastSyncInfo(const QueryKey &queryKey, int64_t startTime, int64_t finishTime, int32_t code);

    static std::atomic<uint32_t> genId_;
    std::shared_ptr<ExecutorPool> executor_;
    ConcurrentMap<uint64_t, TaskId> actives_;
    ConcurrentMap<uint64_t, uint64_t> activeInfos_;
    std::shared_ptr<SyncStrategy> syncStrategy_;
    ConcurrentMap<QueryKey, std::map<SyncId, CloudLastSyncInfo>> lastSyncInfos_;
    std::set<std::string> kvApps_;
    ConcurrentMap<int32_t, std::map<std::string, std::set<std::string>>> compensateSyncInfos_;
};
} // namespace OHOS::CloudData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_SYNC_MANAGER_H