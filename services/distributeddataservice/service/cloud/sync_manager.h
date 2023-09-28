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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_SYNC_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_SYNC_MANAGER_H
#include <list>
#include "eventcenter/event.h"
#include "executor_pool.h"
#include "store/auto_cache.h"
#include "store/general_store.h"
#include "store/general_value.h"
#include "utils/ref_count.h"
#include "concurrent_map.h"
#include "cloud/cloud_info.h"
namespace OHOS::CloudData {
class SyncManager {
public:
    using GenAsync = DistributedData::GenAsync;
    using GenDetails = DistributedData::GenDetails;
    using GenStore = DistributedData::GeneralStore;
    using GenQuery = DistributedData::GenQuery;
    using RefCount = DistributedData::RefCount;
    using AutoCache = DistributedData::AutoCache;
    using StoreMetaData = DistributedData::StoreMetaData;
    static AutoCache::Store GetStore(const StoreMetaData &meta, int32_t user, bool mustBind = true);
    class SyncInfo final {
    public:
        using Store = std::string;
        using Stores = std::vector<Store>;
        using Tables = std::vector<std::string>;
        struct StoreInfo {
            StoreInfo(const std::string& bundle, const std::string& store, GenAsync genAsync = nullptr)
                : bundleName(bundle), storeName(store), async(genAsync)
            {
            }
            std::string bundleName;
            std::string storeName;
            Tables tables;
            GenAsync async;
            std::shared_ptr<GenQuery> query;
        };
        using MutliStores = std::list<StoreInfo>;
        using Iterator = MutliStores::iterator;
        SyncInfo(int32_t user, const std::string &bundleName = "", const Store &store = "", const Tables &tables = {});
        SyncInfo(int32_t user, const std::string &bundleName, const Stores &stores);
        SyncInfo CreateSyncInfo();
        void SetMode(int32_t mode);
        void SetWait(int32_t wait);
        void SetAsync(const std::string &bundleName, const Store& store, GenAsync asyncDetail);
        GenAsync GetAsync(const std::string &bundleName, const Store &store);
        void SetQuery(const std::string& bundleName, const Store& store, std::shared_ptr<GenQuery> query);
        void SetStoreInfo(StoreInfo&& storeInfo);
        void SetError(int32_t code) const;
        void SetDetails(GenDetails&& details, const std::string &bundleName = "", const Store &store = "") const;
        std::shared_ptr<GenQuery> GenerateQuery(const std::string& bundleName, const std::string& store,
            const Tables& tables);
        bool Contains(const std::string& bundleName, const std::string& storeName);
        inline static constexpr const char* DEFAULT_ID = "default";

    private:
        Iterator GetStoreInfo(const std::string& bundleName, const Store& store);
        friend SyncManager;
        uint64_t syncId_ = 0;
        int32_t mode_ = GenStore::CLOUD_TIME_FIRST;
        int32_t user_ = 0;
        int32_t wait_ = 0;
        std::string id_ = DEFAULT_ID;
        MutliStores stores_;
    };
    SyncManager();
    ~SyncManager();
    int32_t Bind(std::shared_ptr<ExecutorPool> executor);
    int32_t DoCloudSync(SyncInfo syncInfo);
    int32_t StopCloudSync(int32_t user = 0);

private:
    using Event = DistributedData::Event;
    using Task = ExecutorPool::Task;
    using TaskId = ExecutorPool::TaskId;
    using Duration = ExecutorPool::Duration;
    class Details {
    public:
        Details(int32_t code);
        Details(const std::string& storeName, int32_t code);
        Details(GenDetails&& details);
        Details(const GenDetails& details);
        operator GenDetails() const;
        operator GenDetails();

    private:
        GenDetails details_;
    };
    using StoreInfo = SyncInfo::StoreInfo;
    using Retryer = std::function<bool(Duration interval, Details&& details, StoreInfo&& storeInfo)>;

    static constexpr ExecutorPool::Duration RETRY_INTERVAL = std::chrono::seconds(10); // second
    static constexpr ExecutorPool::Duration LOCKED_INTERVAL = std::chrono::seconds(30); // second
    static constexpr int32_t RETRY_TIMES = 6; // normal retry
    static constexpr int32_t CLIENT_RETRY_TIMES = 3; // client retry
    static constexpr int32_t ONCE_TIME = 1; // no retry
    static constexpr uint64_t USER_MARK = 0xFFFFFFFF00000000; // high 32 bit
    static constexpr int32_t MV_BIT = 32;

    Task GetSyncTask(int32_t times, RefCount ref, SyncInfo &&syncInfo);
    void UpdateSchema(int32_t user, const std::string &bundleName);
    std::function<void(const Event &)> GetSyncHandler(Retryer retryer);
    std::function<void(const Event &)> GetClientChangeHandler();
    Retryer GetRetryer(int32_t times, SyncInfo&& syncInfo);
    std::vector<DistributedData::SchemaMeta> GetSchemas(const SyncInfo& info, const DistributedData::CloudInfo& cloud);
    void ExecuteSync(int32_t times, SyncInfo&& info, DistributedData::CloudInfo& cloud);
    static uint64_t GenerateId(int32_t user);
    RefCount GenSyncRef(uint64_t syncId);
    int32_t Compare(uint64_t syncId, int32_t user);

    static std::atomic<uint32_t> genId_;
    std::shared_ptr<ExecutorPool> executor_;
    ConcurrentMap<uint64_t, TaskId> actives_;
};
} // namespace OHOS::CloudData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_SYNC_MANAGER_H
