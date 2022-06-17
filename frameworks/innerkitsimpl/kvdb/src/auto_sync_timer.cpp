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

#define LOG_TAG "AutoSyncTimer"

#include <set>
#include "kvdb_service_client.h"
#include "auto_sync_timer.h"
namespace OHOS::DistributedKv {
AutoSyncTimer &AutoSyncTimer::GetInstance()
{
    static AutoSyncTimer instance;
    return instance;
}

void AutoSyncTimer::AddAutoSyncStore(const std::string &appId, const std::set<StoreId> &storeIds)
{
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    if (forceSyncTask_ == SchedulerTask()) {
        auto expiredTime = std::chrono::system_clock::now() + std::chrono::milliseconds(FORCE_SYNC__DELAY_MS);
        delaySyncTask_ = scheduler_.At(expiredTime, ProcessTask());
    }
    if (delaySyncTask_ == SchedulerTask()) {
        auto expiredTime = std::chrono::system_clock::now() + std::chrono::milliseconds(SYNC_DELAY_MS);
        delaySyncTask_ = scheduler_.At(expiredTime, ProcessTask());
    } else {
        delaySyncTask_ = scheduler_.Reset(delaySyncTask_, delaySyncTask_->first,
                                          std::chrono::milliseconds(SYNC_DELAY_MS));
    }
    if (stores_.Contains(appId)) {
        stores_[appId].insert(storeIds.begin(), storeIds.end());
    } else {
        stores_.Insert(appId, storeIds);
    }
}

std::map<std::string, std::set<StoreId>> AutoSyncTimer::GetStoreIds()
{
    std::map<std::string, std::set<StoreId>> stores;
    int count = SYNC_STORE_NUM;
    stores_.EraseIf([this, &stores, &count](const std::string &key, std::set<StoreId> &value) {
        int size = stores_[key].size();
        if (size <= count) {
            stores.insert({ key, value });
            count = count - size;
            return true;
        }
        auto &innerStore = stores[key];
        for(auto it = value.begin(); it != value.end() && count > 0;) {
            innerStore.insert(*it);
            it = value.erase(it);
            count--;
        }
        return value.empty();
    });
    return stores;
}

std::function<void()> AutoSyncTimer::ProcessTask()
{
    return [this]() {
        auto service = KVDBServiceClient::GetInstance();
        if (service == nullptr) {
            return;
        }
        {
            std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
            scheduler_.Clean();
            forceSyncTask_ = {};
            delaySyncTask_ = {};
        }
        std::map<std::string, std::set<StoreId>> storeIds = GetStoreIds();
        KVDBService::SyncInfo syncInfo;
        for(auto &id : storeIds) {
            for (const auto &storeId : id.second) {
                service->Sync({ id.first }, storeId, syncInfo);
            }
        }
        if (!stores_.Empty()) {
            stores_.ForEach([this](const std::string &key, const std::set<StoreId> &value) {
                AddAutoSyncStore(key, value);
                return false;
            });
        }
    };
}
}

