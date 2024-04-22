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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_STORE_AUTO_CACHE_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_STORE_AUTO_CACHE_H
#include <list>
#include <memory>
#include <shared_mutex>
#include <set>
#include "concurrent_map.h"
#include "error/general_error.h"
#include "executor_pool.h"
#include "metadata/store_meta_data.h"
#include "store/general_store.h"
#include "store/general_value.h"
#include "store/general_watcher.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class SchemaMeta;
class AutoCache {
public:
    using Error = GeneralError;
    using Store = std::shared_ptr<GeneralStore>;
    using Stores = std::list<std::shared_ptr<GeneralStore>>;
    using Watcher = GeneralWatcher;
    using Watchers = std::set<std::shared_ptr<GeneralWatcher>>;
    using Time = std::chrono::steady_clock::time_point;
    using Executor = ExecutorPool;
    using TaskId = ExecutorPool::TaskId;
    using Creator = std::function<GeneralStore *(const StoreMetaData &)>;
    API_EXPORT static AutoCache &GetInstance();

    API_EXPORT int32_t RegCreator(int32_t type, Creator creator);

    API_EXPORT void Bind(std::shared_ptr<Executor> executor);

    API_EXPORT Store GetStore(const StoreMetaData &meta, const Watchers &watchers);

    API_EXPORT Stores GetStoresIfPresent(uint32_t tokenId, const std::string &storeName = "");

    API_EXPORT void CloseStore(uint32_t tokenId, const std::string &storeId);

    API_EXPORT void CloseStore(uint32_t tokenId);

    API_EXPORT void CloseExcept(const std::set<int32_t> &users);

    API_EXPORT void SetObserver(uint32_t tokenId, const std::string &storeId, const Watchers &watchers);

    API_EXPORT void Enable(uint32_t tokenId, const std::string &storeId = "");

    API_EXPORT void Disable(uint32_t tokenId, const std::string &storeId);

private:
    AutoCache();
    ~AutoCache();
    void GarbageCollect(bool isForce);
    void StartTimer();
    struct Delegate : public GeneralWatcher {
        Delegate(GeneralStore *delegate, const Watchers &watchers, int32_t user);
        ~Delegate();
        operator Store();
        bool operator<(const Time &time) const;
        bool Close();
        int32_t GetUser() const;
        void SetObservers(const Watchers &watchers);
        int32_t OnChange(const Origin &origin, const PRIFields &primaryFields, ChangeInfo &&values) override;
        int32_t OnChange(const Origin &origin, const Fields &fields, ChangeData &&datas) override;

    private:
        mutable Time time_;
        GeneralStore *store_ = nullptr;
        Watchers watchers_;
        int32_t user_;
        std::shared_mutex mutex_;
    };

    static constexpr int64_t INTERVAL = 1;
    static constexpr int32_t MAX_CREATOR_NUM = 30;

    std::shared_ptr<Executor> executor_;
    TaskId taskId_ = Executor::INVALID_TASK_ID;
    ConcurrentMap<uint32_t, std::map<std::string, Delegate>> stores_;
    ConcurrentMap<uint32_t, std::set<std::string>> disables_;
    Creator creators_[MAX_CREATOR_NUM];
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_STORE_AUTO_CACHE_H
