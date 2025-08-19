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

#define LOG_TAG "DBAdaptor"
#include "db_delegate.h"

#include "account/account_delegate.h"
#include "kv_delegate.h"
#include "log_print.h"
#include "rdb_delegate.h"
#include "log_debug.h"

namespace OHOS::DataShare {
using Account = DistributedData::AccountDelegate;
ExecutorPool::TaskId DBDelegate::taskId_ = ExecutorPool::INVALID_TASK_ID;
ExecutorPool::TaskId DBDelegate::taskIdEncrypt_ = ExecutorPool::INVALID_TASK_ID;
ConcurrentMap<uint32_t, std::map<std::string, std::shared_ptr<DBDelegate::Entity>>> DBDelegate::stores_ = {};
ConcurrentMap<uint32_t, std::map<std::string, std::shared_ptr<DBDelegate::Entity>>> DBDelegate::storesEncrypt_ = {};
std::shared_ptr<ExecutorPool> DBDelegate::executor_ = nullptr;
std::shared_ptr<DBDelegate> DBDelegate::Create(DistributedData::StoreMetaData &metaData,
    const std::string &extUri, const std::string &backup)
{
    if (Account::GetInstance()->IsDeactivating(atoi(metaData.user.c_str()))) {
        ZLOGW("user %{public}s is deactivating, storeName: %{public}s", metaData.user.c_str(),
              StringUtils::GeneralAnonymous(metaData.GetStoreAlias()).c_str());
        return nullptr;
    }
    std::shared_ptr<DBDelegate> store;
    auto storeFunc = [&metaData, &store, extUri, &backup]
        (auto &, std::map<std::string, std::shared_ptr<Entity>> &stores) -> bool {
        auto it = stores.find(metaData.storeId);
        if (it != stores.end()) {
            store = it->second->store_;
            it->second->time_ = std::chrono::steady_clock::now() + std::chrono::seconds(INTERVAL);
            return !stores.empty();
        }
        store = std::make_shared<RdbDelegate>();
        auto entity = std::make_shared<Entity>(store, metaData);
        stores.emplace(metaData.storeId, entity);
        StartTimer(metaData.isEncrypt);
        return !stores.empty();
    };
    if (metaData.isEncrypt) {
        storesEncrypt_.Compute(metaData.tokenId, storeFunc);
    } else {
        stores_.Compute(metaData.tokenId, storeFunc);
    }

    // rdbStore is initialized outside the ConcurrentMap, because this maybe a time-consuming operation.
    bool success = store->Init(metaData, NO_CHANGE_VERSION, true, extUri, backup);
    if (success) {
        return store;
    }
    ZLOGE("creator failed, storeName: %{public}s", StringUtils::GeneralAnonymous(metaData.GetStoreAlias()).c_str());
    auto eraseFunc = [&metaData]
        (auto &, std::map<std::string, std::shared_ptr<Entity>> &stores) -> bool {
        stores.erase(metaData.storeId);
        return !stores.empty();
    };
    if (metaData.isEncrypt) {
        storesEncrypt_.Compute(metaData.tokenId, eraseFunc);
    } else {
        stores_.Compute(metaData.tokenId, eraseFunc);
    }
    return nullptr;
}

void DBDelegate::SetExecutorPool(std::shared_ptr<ExecutorPool> executor)
{
    executor_ = std::move(executor);
}

void DBDelegate::Close(const DBDelegate::Filter &filter)
{
    if (filter == nullptr) {
        return;
    }
    std::list<std::shared_ptr<DBDelegate::Entity>> closeStores;
    auto eraseFunc = [&closeStores, &filter](auto &, std::map<std::string, std::shared_ptr<Entity>> &stores) {
        for (auto it = stores.begin(); it != stores.end();) {
            if (it->second == nullptr || filter(it->second->user)) {
                closeStores.push_back(it->second);
                it = stores.erase(it);
            } else {
                ++it;
            }
        }
        return stores.empty();
    };
    stores_.EraseIf(eraseFunc);
    storesEncrypt_.EraseIf(eraseFunc);
}

void DBDelegate::GarbageCollect(bool encrypt)
{
    std::list<std::shared_ptr<DBDelegate::Entity>> closeStores;
    auto eraseFunc = [&closeStores](auto &, std::map<std::string, std::shared_ptr<Entity>> &stores) {
        auto current = std::chrono::steady_clock::now();
        for (auto it = stores.begin(); it != stores.end();) {
            if (it->second->time_ < current) {
                closeStores.push_back(it->second);
                it = stores.erase(it);
            } else {
                ++it;
            }
        }
        return stores.empty();
    };
    if (encrypt) {
        storesEncrypt_.EraseIf(eraseFunc);
    } else {
        stores_.EraseIf(eraseFunc);
    }
}

void DBDelegate::StartTimer(bool encrypt)
{
    ExecutorPool::TaskId& dstTaskId = encrypt ? taskIdEncrypt_ : taskId_;

    if (executor_ == nullptr || dstTaskId != Executor::INVALID_TASK_ID) {
        return;
    }
    dstTaskId = executor_->Schedule(
        [encrypt]() {
            GarbageCollect(encrypt);
            auto task = [encrypt]() {
                ExecutorPool::TaskId& dstTaskIdTemp = encrypt ? taskIdEncrypt_ : taskId_;
                if (executor_ == nullptr || dstTaskIdTemp == Executor::INVALID_TASK_ID) {
                    return;
                }
                executor_->Remove(dstTaskIdTemp);
                ZLOGD_MACRO("remove timer, taskId: %{public}" PRIu64, dstTaskIdTemp);
                dstTaskIdTemp = Executor::INVALID_TASK_ID;
            };
            if (encrypt) {
                stores_.DoActionIfEmpty(task);
            } else {
                storesEncrypt_.DoActionIfEmpty(task);
            }
        },
        std::chrono::seconds(INTERVAL), std::chrono::seconds(INTERVAL));
    ZLOGD_MACRO("start timer, taskId: %{public}" PRIu64, dstTaskId);
}

DBDelegate::Entity::Entity(std::shared_ptr<DBDelegate> store, const DistributedData::StoreMetaData &meta)
{
    store_ = std::move(store);
    time_ = std::chrono::steady_clock::now() + std::chrono::seconds(INTERVAL);
    user = meta.user;
}

void DBDelegate::EraseStoreCache(const int32_t tokenId)
{
    stores_.Erase(tokenId);
    storesEncrypt_.Erase(tokenId);
}

std::shared_ptr<KvDBDelegate> KvDBDelegate::GetInstance(const std::string &dir,
    const std::shared_ptr<ExecutorPool> &executors)
{
    static std::shared_ptr<KvDBDelegate> delegate = nullptr;
    static std::mutex mutex;
    std::lock_guard<decltype(mutex)> lock(mutex);
    if (delegate == nullptr && executors != nullptr) {
        delegate = std::make_shared<KvDelegate>(dir, executors);
    }
    return delegate;
}

bool Id::Marshal(DistributedData::Serializable::json &node) const
{
    auto ret = false;
    if (userId == INVALID_USER) {
        ret = SetValue(node[GET_NAME(_id)], _id);
    } else {
        ret = SetValue(node[GET_NAME(_id)], _id + "_" + std::to_string(userId));
    }
    return ret;
}

bool Id::Unmarshal(const DistributedData::Serializable::json &node)
{
    return false;
}

Id::Id(const std::string &id, const int32_t userId) : _id(id), userId(userId) {}

VersionData::VersionData(int version) : version(version) {}

bool VersionData::Unmarshal(const DistributedData::Serializable::json &node)
{
    return GetValue(node, GET_NAME(version), version);
}

bool VersionData::Marshal(DistributedData::Serializable::json &node) const
{
    return SetValue(node[GET_NAME(version)], version);
}

const std::string &KvData::GetId() const
{
    return id;
}

KvData::KvData(const Id &id) : id(DistributedData::Serializable::Marshall(id)) {}
} // namespace OHOS::DataShare
