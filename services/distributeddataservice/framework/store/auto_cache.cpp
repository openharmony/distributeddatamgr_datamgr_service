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
#define LOG_TAG "AutoCache"
#include <cinttypes>
#include "utils/anonymous.h"
#include "store/auto_cache.h"

#include "log_print.h"
namespace OHOS::DistributedData {
AutoCache &AutoCache::GetInstance()
{
    static AutoCache cache;
    return cache;
}

int32_t AutoCache::RegCreator(int32_t type, Creator creator)
{
    if (type >= MAX_CREATOR_NUM) {
        return E_ERROR;
    }
    creators_[type] = creator;
    return 0;
}

void AutoCache::Bind(std::shared_ptr<Executor> executor)
{
    if (executor == nullptr || taskId_ != Executor::INVALID_TASK_ID) {
        return;
    }
    executor_ = executor;
}

AutoCache::AutoCache() {}

AutoCache::~AutoCache()
{
    GarbageCollect(true);
    if (executor_ != nullptr) {
        executor_->Remove(taskId_, true);
    }
}

AutoCache::Store AutoCache::GetStore(const StoreMetaData &meta, const Watchers &watchers)
{
    Store store;
    if (meta.storeType >= MAX_CREATOR_NUM || meta.storeType < 0 || !creators_[meta.storeType] ||
        disables_.ContainIf(meta.tokenId, [&meta](const std::set<std::string>& stores) -> bool {
            return stores.count(meta.storeId) != 0;
        })) {
        return store;
    }

    stores_.Compute(meta.tokenId,
        [this, &meta, &watchers, &store](auto &, std::map<std::string, Delegate> &stores) -> bool {
            auto it = stores.find(meta.storeId);
            if (it != stores.end()) {
                if (!watchers.empty()) {
                    it->second.SetObservers(watchers);
                }
                store = it->second;
                return !stores.empty();
            }
            auto *dbStore = creators_[meta.storeType](meta);
            if (dbStore == nullptr) {
                ZLOGE("creator failed. storeName:%{public}s", meta.GetStoreAlias().c_str());
                return !stores.empty();
            }
            auto result = stores.emplace(std::piecewise_construct, std::forward_as_tuple(meta.storeId),
                std::forward_as_tuple(dbStore, watchers, atoi(meta.user.c_str())));
            store = result.first->second;
            StartTimer();
            return !stores.empty();
        });
    return store;
}

AutoCache::Stores AutoCache::GetStoresIfPresent(uint32_t tokenId, const std::string& storeName)
{
    Stores stores;
    stores_.ComputeIfPresent(tokenId, [&stores, &storeName](auto&, std::map<std::string, Delegate>& delegates) -> bool {
        if (storeName.empty()) {
            for (auto& [_, delegate] : delegates) {
                stores.push_back(delegate);
            }
        } else {
            auto it = delegates.find(storeName);
            if (it != delegates.end()) {
                stores.push_back(it->second);
            }
        }
        return !stores.empty();
    });
    return stores;
}

// Should be used within stores_'s thread safe methods
void AutoCache::StartTimer()
{
    if (executor_ == nullptr || taskId_ != Executor::INVALID_TASK_ID) {
        return;
    }
    taskId_ = executor_->Schedule(
        [this]() {
            GarbageCollect(false);
            stores_.DoActionIfEmpty([this]() {
                if (executor_ == nullptr || taskId_ == Executor::INVALID_TASK_ID) {
                    return;
                }
                executor_->Remove(taskId_);
                ZLOGD("remove timer,taskId: %{public}" PRIu64, taskId_);
                taskId_ = Executor::INVALID_TASK_ID;
            });
        },
        std::chrono::minutes(INTERVAL), std::chrono::minutes(INTERVAL));
    ZLOGD("start timer,taskId: %{public}" PRIu64, taskId_);
}

void AutoCache::CloseStore(uint32_t tokenId, const std::string &storeId)
{
    stores_.ComputeIfPresent(tokenId, [&storeId](auto &key, std::map<std::string, Delegate> &delegates) {
        auto it = delegates.find(storeId);
        if (it != delegates.end()) {
            it->second.Close();
            delegates.erase(it);
        }
        return !delegates.empty();
    });
}

void AutoCache::CloseStore(uint32_t tokenId)
{
    stores_.Erase(tokenId);
}

void AutoCache::CloseExcept(const std::set<int32_t> &users)
{
    stores_.EraseIf([&users](const auto &tokenId, std::map<std::string, Delegate> &delegates) {
        if (delegates.empty() || users.count(delegates.begin()->second.GetUser()) != 0) {
            return delegates.empty();
        }

        for (auto it = delegates.begin(); it != delegates.end();) {
            // if the kv store is BUSY we wait more INTERVAL minutes again
            if (!it->second.Close()) {
                ++it;
            } else {
                it = delegates.erase(it);
            }
        }
        return delegates.empty();
    });
}

void AutoCache::SetObserver(uint32_t tokenId, const std::string &storeId, const AutoCache::Watchers &watchers)
{
    stores_.ComputeIfPresent(tokenId, [&storeId, &watchers](auto &key, auto &stores) {
        ZLOGD("tokenId:0x%{public}x storeId:%{public}s observers:%{public}zu", key, Anonymous::Change(storeId).c_str(),
            watchers.size());
        auto it = stores.find(storeId);
        if (it != stores.end()) {
            it->second.SetObservers(watchers);
        }
        return true;
    });
}

void AutoCache::GarbageCollect(bool isForce)
{
    auto current = std::chrono::steady_clock::now();
    stores_.EraseIf([&current, isForce](auto &key, std::map<std::string, Delegate> &delegates) {
        for (auto it = delegates.begin(); it != delegates.end();) {
            // if the store is BUSY we wait more INTERVAL minutes again
            if ((isForce || it->second < current) && it->second.Close()) {
                it = delegates.erase(it);
            } else {
                ++it;
            }
        }
        return delegates.empty();
    });
}

void AutoCache::Enable(uint32_t tokenId, const std::string& storeId)
{
    disables_.ComputeIfPresent(tokenId, [&storeId](auto key, std::set<std::string>& stores) {
        stores.erase(storeId);
        return !(stores.empty() || storeId.empty());
    });
}

void AutoCache::Disable(uint32_t tokenId, const std::string& storeId)
{
    disables_.Compute(tokenId, [&storeId](auto key, std::set<std::string>& stores) {
        stores.insert(storeId);
        return !stores.empty();
    });
    CloseStore(tokenId, storeId);
}

AutoCache::Delegate::Delegate(GeneralStore *delegate, const Watchers &watchers, int32_t user)
    : store_(delegate), watchers_(watchers), user_(user)
{
    time_ = std::chrono::steady_clock::now() + std::chrono::minutes(INTERVAL);
    if (store_ != nullptr) {
        store_->Watch(Origin::ORIGIN_ALL, *this);
    }
}

AutoCache::Delegate::~Delegate()
{
    if (store_ != nullptr) {
        store_->Unwatch(Origin::ORIGIN_ALL, *this);
        store_->Close();
        store_->Release();
        store_ = nullptr;
    }
}

AutoCache::Delegate::operator Store()
{
    time_ = std::chrono::steady_clock::now() + std::chrono::minutes(INTERVAL);
    if (store_ != nullptr) {
        store_->AddRef();
        return Store(store_, [](GeneralStore *store) { store->Release(); });
    }
    return nullptr;
}

bool AutoCache::Delegate::operator<(const AutoCache::Time &time) const
{
    return time_ < time;
}

bool AutoCache::Delegate::Close()
{
    std::unique_lock<decltype(mutex_)> lock(mutex_);
    if (store_ != nullptr) {
        auto status = store_->Close();
        if (status == Error::E_BUSY) {
            return false;
        }
        store_->Unwatch(Origin::ORIGIN_ALL, *this);
    }
    return true;
}

void AutoCache::Delegate::SetObservers(const AutoCache::Watchers &watchers)
{
    std::unique_lock<decltype(mutex_)> lock(mutex_);
    watchers_ = watchers;
}

int32_t AutoCache::Delegate::GetUser() const
{
    return user_;
}

int32_t AutoCache::Delegate::OnChange(const Origin &origin, const PRIFields &primaryFields, ChangeInfo &&values)
{
    Watchers watchers;
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        watchers = watchers_;
    }
    size_t remain = watchers.size();
    for (auto &watcher : watchers) {
        remain--;
        if (watcher == nullptr) {
            continue;
        }
        watcher->OnChange(origin, primaryFields, (remain != 0) ? ChangeInfo(values) : std::move(values));
    }
    return Error::E_OK;
}

int32_t AutoCache::Delegate::OnChange(const Origin &origin, const Fields &fields, ChangeData &&datas)
{
    Watchers watchers;
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        watchers = watchers_;
    }
    size_t remain = watchers.size();
    for (auto &watcher : watchers) {
        remain--;
        if (watcher == nullptr) {
            continue;
        }
        watcher->OnChange(origin, fields, (remain != 0) ? ChangeData(datas) : std::move(datas));
    }
    return Error::E_OK;
}
} // namespace OHOS::DistributedData
