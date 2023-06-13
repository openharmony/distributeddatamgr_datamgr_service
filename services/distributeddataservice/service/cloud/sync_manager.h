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
#include "eventcenter/event.h"
#include "executor_pool.h"
#include "store/general_store.h"
#include "store/general_value.h"
#include "utils/ref_count.h"
#include "concurrent_map.h"
namespace OHOS::CloudData {
class SyncManager {
public:
    using GenAsync = DistributedData::GenAsync;
    using GenStore = DistributedData::GeneralStore;
    using GenQuery = DistributedData::GenQuery;
    using RefCount = DistributedData::RefCount;
    class SyncInfo final {
    public:
        using Store = std::string;
        using Stores = std::vector<Store>;
        using Tables = std::vector<std::string>;
        using MutliStoreTables = std::map<Store, Tables>;
        SyncInfo(int32_t user, const std::string &bundleName = "", const Store &store = "", const Tables &tables = {});
        SyncInfo(int32_t user, const std::string &bundleName, const Stores &stores);
        SyncInfo(int32_t user, const std::string &bundleName, const MutliStoreTables &tables);
        void SetMode(int32_t mode);
        void SetWait(int32_t wait);
        void SetAsyncDetail(GenAsync asyncDetail);
        void SetQuery(std::shared_ptr<GenQuery> query);
        void SetError(int32_t code);
        std::shared_ptr<GenQuery> GenerateQuery(const std::string &store, const Tables &tables);
        inline static constexpr const char *DEFAULT_ID = "default";

    private:
        friend SyncManager;
        int32_t mode_ = GenStore::CLOUD_TIME_FIRST;
        int32_t user_ = 0;
        int32_t wait_ = 0;
        std::string id_ = DEFAULT_ID;
        std::string bundleName_;
        std::map<std::string, std::vector<std::string>> tables_;
        GenAsync async_;
        std::shared_ptr<GenQuery> query_;
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
    static constexpr ExecutorPool::Duration RETRY_INTERVAL = std::chrono::seconds(10); // second
    static constexpr int32_t RETRY_TIMES = 6; // second
    static constexpr uint64_t USER_MARK = 0xFFFFFFFF00000000; // high 32 bit
    static constexpr int32_t MV_BIT = 32;

    Task GetSyncTask(int32_t retry, RefCount ref, SyncInfo &&syncInfo);
    void DoRetry(int32_t retry, SyncInfo &&info);
    std::function<void(const Event &)> GetSyncHandler();
    std::function<void(const Event &)> GetClientChangeHandler();
    uint64_t GenSyncId(int32_t user);
    RefCount GenSyncRef(uint64_t syncId);
    int32_t Compare(uint64_t syncId, int32_t user);

    std::atomic<uint32_t> syncId_ = 0;
    std::shared_ptr<ExecutorPool> executor_;
    ConcurrentMap<uint64_t, TaskId> actives_;
};
} // namespace OHOS::CloudData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_SYNC_MANAGER_H
