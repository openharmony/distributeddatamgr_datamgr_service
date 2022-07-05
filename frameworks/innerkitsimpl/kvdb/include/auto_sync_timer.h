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

#ifndef SDB_AUTO_SYNC_TIMER_H
#define SDB_AUTO_SYNC_TIMER_H
#include <set>
#include "kv_scheduler.h"
#include "kvdb_service.h"
#include "concurrent_map.h"

namespace OHOS::DistributedKv {
class AutoSyncTimer {
public:
    static constexpr uint32_t FORCE_SYNC_INTERVAL = 200;
    static constexpr uint32_t AUTO_SYNC_INTERVAL = 50;
    static AutoSyncTimer &GetInstance();
    void DoAutoSync(const std::string &appId, const std::set<StoreId> &storeIds);
private:
    static constexpr size_t TIME_TASK_NUM = 5;
    static constexpr size_t SYNC_STORE_NUM = 10;
    AutoSyncTimer() = default;
    ~AutoSyncTimer() = default;
    std::map<std::string, std::set<StoreId>> GetStoreIds();
    std::function<void()> ProcessTask();
    void StartTimer();
    void StopTimer();
    void AddSyncStores(const std::string &appId, std::set<StoreId> storeIds);
    bool HasSyncStores();
    ConcurrentMap<std::string, std::set<StoreId>> stores_;
    SchedulerTask delaySyncTask_;
    SchedulerTask forceSyncTask_;
    std::mutex mutex_;
    KvScheduler scheduler_ { TIME_TASK_NUM };
};
}

#endif // SDB_AUTO_SYNC_TIMER_H
