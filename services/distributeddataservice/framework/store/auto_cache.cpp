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
#include "store/auto_cache.h"

#include <cinttypes>

#include "account_delegate.h"
#include "changeevent/remote_change_event.h"
#include "eventcenter/event_center.h"
#include "log_print.h"
#include "screenlock/screen_lock.h"
#include "utils/anonymous.h"
namespace OHOS::DistributedData {
using Account = DistributedKv::AccountDelegate;
AutoCache &AutoCache::GetInstance()
{
    static AutoCache cache;
    return cache;
}

int32_t AutoCache::RegCreator(int32_t type, Creator creator)
{
    if (type < 0 || type >= MAX_CREATOR_NUM) {
        return E_ERROR;
    }
    creators_[type] = creator;
    return E_OK;
}

void AutoCache::Bind(std::shared_ptr<Executor> executor)
{
    if (executor == nullptr || taskId_ != Executor::INVALID_TASK_ID) {
        return;
    }
    executor_ = std::move(executor);
}

AutoCache::AutoCache()
{
}

AutoCache::~AutoCache()
{
    GarbageCollect(true);
    if (executor_ != nullptr) {
        executor_->Remove(taskId_, true);
    }
}

std::pair<int32_t, AutoCache::Store> AutoCache::GetDBStore(const StoreMetaData &meta, const Watchers &watchers)
{
    Store store;
    if (meta.storeType >= MAX_CREATOR_NUM || meta.storeType < 0 || !creators_[meta.storeType] ||
        disables_.ContainIf(meta.tokenId,
            [&meta](const std::set<std::string> &stores) -> bool { return stores.count(meta.storeId) != 0; })) {
        ZLOGW("storeType is invalid or store is disabled, user:%{public}s, bundleName:%{public}s, "
              "storeName:%{public}s",
            meta.user.c_str(), meta.bundleName.c_str(), meta.GetStoreAlias().c_str());
        return { E_ERROR, store };
    }
    if (meta.area == GeneralStore::EL4 && ScreenManager::GetInstance()->IsLocked()) {
        ZLOGW("screen is locked, user:%{public}s, bundleName:%{public}s, storeName:%{public}s", meta.user.c_str(),
              meta.bundleName.c_str(), meta.GetStoreAlias().c_str());
        return { E_SCREEN_LOCKED, store };
    }
    if (Account::GetInstance()->IsDeactivating(atoi(meta.user.c_str()))) {
        ZLOGW("user %{public}s is deactivating, bundleName:%{public}s, storeName: %{public}s", meta.user.c_str(),
              meta.bundleName.c_str(), meta.GetStoreAlias().c_str());
        return { E_USER_DEACTIVATING, store };
    }
    stores_.Compute(meta.tokenId,
        [this, &meta, &watchers, &store](auto &, std::map<std::string, Delegate> &stores) -> bool {
            if (disableStores_.count(meta.dataDir) != 0) {
                ZLOGW("store is closing, tokenId:0x%{public}x user:%{public}s"
                      "bundleName:%{public}s storeName:%{public}s",
                    meta.tokenId, meta.user.c_str(), meta.bundleName.c_str(), meta.GetStoreAlias().c_str());
                return !stores.empty();
            }
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
            dbStore->SetExecutor(executor_);
            auto result = stores.emplace(std::piecewise_construct, std::forward_as_tuple(meta.storeId),
                std::forward_as_tuple(dbStore, watchers, atoi(meta.user.c_str()), meta));
            store = result.first->second;
            StartTimer();
            return !stores.empty();
        });
    return { E_OK, store };
}

AutoCache::Store AutoCache::GetStore(const StoreMetaData &meta, const Watchers &watchers)
{
    return GetDBStore(meta, watchers).second;
}

AutoCache::Stores AutoCache::GetStoresIfPresent(uint32_t tokenId, const std::string &storeName)
{
    Stores stores;
    stores_.ComputeIfPresent(
        tokenId, [&stores, &storeName](auto &, std::map<std::string, Delegate> &delegates) -> bool {
            if (storeName.empty()) {
                for (auto &[_, delegate] : delegates) {
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
    ZLOGD("close store start, store:%{public}s, token:%{public}u", Anonymous::Change(storeId).c_str(), tokenId);
    std::set<std::string> storeIds;
    std::list<Delegate> closeStores;
    bool isScreenLocked = ScreenManager::GetInstance()->IsLocked();
    stores_.ComputeIfPresent(tokenId,
        [this, &storeId, isScreenLocked, &storeIds, &closeStores](auto &key, auto &delegates) {
            auto it = delegates.begin();
            while (it != delegates.end()) {
                if ((storeId == it->first || storeId.empty()) &&
                    (!isScreenLocked || it->second.GetArea() != GeneralStore::EL4) &&
                    disableStores_.count(it->second.GetDataDir()) == 0) {
                    disableStores_.insert(it->second.GetDataDir());
                    storeIds.insert(it->first);
                    closeStores.emplace(closeStores.end(), it->second);
                }
                ++it;
            }
            return !delegates.empty();
        });
    closeStores.clear();
    stores_.ComputeIfPresent(tokenId, [this, &storeIds](auto &key, auto &delegates) {
        for (auto it = delegates.begin(); it != delegates.end();) {
            if (storeIds.count(it->first) != 0) {
                disableStores_.erase(it->second.GetDataDir());
                it = delegates.erase(it);
            } else {
                ++it;
            }
        }
        return !delegates.empty();
    });
}

void AutoCache::CloseStore(const AutoCache::Filter &filter)
{
    if (filter == nullptr) {
        return;
    }
    std::map<uint32_t, std::set<std::string>> storeIds;
    std::list<Delegate> closeStores;
    stores_.ForEach(
        [this, &filter, &storeIds, &closeStores](const auto &tokenId, std::map<std::string, Delegate> &delegates) {
            auto it = delegates.begin();
            while (it != delegates.end()) {
                if (disableStores_.count(it->second.GetDataDir()) == 0 && filter(it->second.GetMeta())) {
                    disableStores_.insert(it->second.GetDataDir());
                    storeIds[tokenId].insert(it->first);
                    closeStores.emplace(closeStores.end(), it->second);
                }
                ++it;
            }
            return false;
        });
    closeStores.clear();
    stores_.EraseIf([this, &storeIds](auto &tokenId, auto &delegates) {
        for (auto it = delegates.begin(); it != delegates.end();) {
            auto ids = storeIds.find(tokenId);
            if (ids != storeIds.end() && ids->second.count(it->first) != 0) {
                disableStores_.erase(it->second.GetDataDir());
                it = delegates.erase(it);
            } else {
                ++it;
            }
        }
        return delegates.empty();
    });
}

void AutoCache::SetObserver(uint32_t tokenId, const std::string &storeId, const AutoCache::Watchers &watchers)
{
    stores_.ComputeIfPresent(tokenId, [&storeId, &watchers](auto &key, auto &stores) {
        ZLOGD("tokenId:0x%{public}x storeId:%{public}s observers:%{public}zu", key,
            Anonymous::Change(storeId).c_str(), watchers.size());
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
    bool isScreenLocked = ScreenManager::GetInstance()->IsLocked();
    stores_.EraseIf([&current, isForce, isScreenLocked](auto &key, std::map<std::string, Delegate> &delegates) {
        for (auto it = delegates.begin(); it != delegates.end();) {
            // if the store is BUSY we wait more INTERVAL minutes again
            if ((!isScreenLocked || it->second.GetArea() != GeneralStore::EL4) && (isForce || it->second < current) &&
                it->second.Close()) {
                it = delegates.erase(it);
            } else {
                ++it;
            }
        }
        return delegates.empty();
    });
}

void AutoCache::Enable(uint32_t tokenId, const std::string &storeId)
{
    disables_.ComputeIfPresent(tokenId, [&storeId](auto key, std::set<std::string> &stores) {
        stores.erase(storeId);
        return !(stores.empty() || storeId.empty());
    });
}

void AutoCache::Disable(uint32_t tokenId, const std::string &storeId)
{
    disables_.Compute(tokenId, [&storeId](auto key, std::set<std::string> &stores) {
        stores.insert(storeId);
        return !stores.empty();
    });
    CloseStore(tokenId, storeId);
}

AutoCache::Delegate::Delegate(GeneralStore *delegate, const Watchers &watchers, int32_t user, const StoreMetaData &meta)
    : store_(delegate), watchers_(watchers), user_(user), meta_(meta)
{
    time_ = std::chrono::steady_clock::now() + std::chrono::minutes(INTERVAL);
    if (store_ != nullptr) {
        store_->Watch(Origin::ORIGIN_ALL, *this);
    }
}

AutoCache::Delegate::Delegate(const Delegate& delegate)
{
    store_ = delegate.store_;
    if (store_ != nullptr) {
        store_->AddRef();
    }
}

AutoCache::Delegate::~Delegate()
{
    if (store_ != nullptr) {
        store_->Close(true);
        store_->Unwatch(Origin::ORIGIN_ALL, *this);
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

int32_t AutoCache::Delegate::GetArea() const
{
    return meta_.area;
}

const std::string& AutoCache::Delegate::GetDataDir() const
{
    return meta_.dataDir;
}

const StoreMetaData &AutoCache::Delegate::GetMeta() const
{
    return meta_;
}

int32_t AutoCache::Delegate::OnChange(const Origin &origin, const PRIFields &primaryFields, ChangeInfo &&values)
{
    std::vector<std::string> tables;
    for (const auto &[table, value] : values) {
        tables.emplace_back(table);
    }
    PostDataChange(meta_, tables);
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
    std::vector<std::string> tables;
    for (const auto &[table, value] : datas) {
        tables.emplace_back(table);
    }
    PostDataChange(meta_, tables);
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

void AutoCache::Delegate::PostDataChange(const StoreMetaData &meta, const std::vector<std::string> &tables)
{
    RemoteChangeEvent::DataInfo info;
    info.userId = meta.user;
    info.storeId = meta.storeId;
    info.deviceId = meta.deviceId;
    info.bundleName = meta.bundleName;
    info.tables = tables;
    auto evt = std::make_unique<RemoteChangeEvent>(RemoteChangeEvent::DATA_CHANGE, std::move(info));
    EventCenter::GetInstance().PostEvent(std::move(evt));
}
} // namespace OHOS::DistributedData