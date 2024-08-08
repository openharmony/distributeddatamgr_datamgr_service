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

#include "kv_delegate.h"
#include "log_print.h"
#include "rdb_delegate.h"
namespace OHOS::DataShare {
ExecutorPool::TaskId DBDelegate::taskId_ = ExecutorPool::INVALID_TASK_ID;
ConcurrentMap<uint32_t, std::map<std::string, std::shared_ptr<DBDelegate::Entity>>> DBDelegate::stores_ = {};
std::shared_ptr<ExecutorPool> DBDelegate::executor_ = nullptr;
std::shared_ptr<DBDelegate> DBDelegate::Create(DistributedData::StoreMetaData &metaData,
    const std::string &extUri)
{
    if (metaData.tokenId == 0) {
        return std::make_shared<RdbDelegate>(metaData, NO_CHANGE_VERSION, true, extUri);
    }
    std::shared_ptr<DBDelegate> store;
    stores_.Compute(metaData.tokenId,
        [&metaData, &store, extUri](auto &, std::map<std::string, std::shared_ptr<Entity>> &stores) -> bool {
            auto it = stores.find(metaData.storeId);
            if (it != stores.end()) {
                store = it->second->store_;
                it->second->time_ = std::chrono::steady_clock::now() + std::chrono::minutes(INTERVAL);
                return !stores.empty();
            }
            store = std::make_shared<RdbDelegate>(metaData, NO_CHANGE_VERSION, true, extUri);
            if (store->IsInvalid()) {
                store = nullptr;
                ZLOGE("creator failed, storeName: %{public}s", metaData.GetStoreAlias().c_str());
                return false;
            }
            auto entity = std::make_shared<Entity>(store);
            stores.emplace(metaData.storeId, entity);
            StartTimer();
            return !stores.empty();
        });
    return store;
}

void DBDelegate::SetExecutorPool(std::shared_ptr<ExecutorPool> executor)
{
    executor_ = std::move(executor);
}

void DBDelegate::GarbageCollect()
{
    stores_.EraseIf([](auto &, std::map<std::string, std::shared_ptr<Entity>> &stores) {
        auto current = std::chrono::steady_clock::now();
        for (auto it = stores.begin(); it != stores.end();) {
            // if the store is BUSY we wait more INTERVAL minutes again
            if (it->second->time_ < current) {
                it = stores.erase(it);
            } else {
                ++it;
            }
        }
        return stores.empty();
    });
}

void DBDelegate::StartTimer()
{
    if (executor_ == nullptr || taskId_ != Executor::INVALID_TASK_ID) {
        return;
    }
    taskId_ = executor_->Schedule(
        []() {
            GarbageCollect();
            stores_.DoActionIfEmpty([]() {
                if (executor_ == nullptr || taskId_ == Executor::INVALID_TASK_ID) {
                    return;
                }
                executor_->Remove(taskId_);
                ZLOGD("remove timer, taskId: %{public}" PRIu64, taskId_);
                taskId_ = Executor::INVALID_TASK_ID;
            });
        },
        std::chrono::minutes(INTERVAL), std::chrono::minutes(INTERVAL));
    ZLOGD("start timer, taskId: %{public}" PRIu64, taskId_);
}

DBDelegate::Entity::Entity(std::shared_ptr<DBDelegate> store)
{
    store_ = std::move(store);
    time_ = std::chrono::steady_clock::now() + std::chrono::minutes(INTERVAL);
}

void DBDelegate::EraseStoreCache(const int32_t tokenId)
{
    stores_.Erase(tokenId);
}

std::shared_ptr<KvDBDelegate> KvDBDelegate::GetInstance(
    bool reInit, const std::string &dir, const std::shared_ptr<ExecutorPool> &executors)
{
    static std::shared_ptr<KvDBDelegate> delegate = nullptr;
    static std::mutex mutex;
    std::lock_guard<decltype(mutex)> lock(mutex);
    if ((delegate == nullptr || reInit) && executors != nullptr) {
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