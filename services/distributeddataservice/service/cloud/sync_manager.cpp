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
#define LOG_TAG "SyncManager"
#include "sync_manager.h"

#include "cloud/cloud_info.h"
#include "cloud/cloud_server.h"
#include "cloud/schema_meta.h"
#include "cloud/sync_event.h"
#include "device_manager_adapter.h"
#include "eventcenter/event_center.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "store/auto_cache.h"
#include "utils/anonymous.h"
namespace OHOS::CloudData {
using namespace DistributedData;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
SyncManager::SyncInfo::SyncInfo(int32_t user, const std::string &bundleName, const Store &store, const Tables &tables)
    : user_(user), bundleName_(bundleName)
{
    if (!store.empty()) {
        tables_[store] = tables;
    }
}

SyncManager::SyncInfo::SyncInfo(int32_t user, const std::string &bundleName, const Stores &stores)
    : user_(user), bundleName_(bundleName)
{
    for (auto &store : stores) {
        tables_[store] = {};
    }
}

SyncManager::SyncInfo::SyncInfo(int32_t user, const std::string &bundleName, const MutliStoreTables &tables)
    : user_(user), bundleName_(bundleName), tables_(tables)
{
}

void SyncManager::SyncInfo::SetMode(int32_t mode)
{
    mode_ = mode;
}

void SyncManager::SyncInfo::SetWait(int32_t wait)
{
    wait_ = wait;
}

void SyncManager::SyncInfo::SetAsyncDetail(GenAsync asyncDetail)
{
    async_ = std::move(asyncDetail);
}

void SyncManager::SyncInfo::SetQuery(std::shared_ptr<GenQuery> query)
{
    query_ = query;
}

void SyncManager::SyncInfo::SetError(int32_t code)
{
    if (async_) {
        GenDetails details;
        auto &detail = details[id_];
        detail.progress = SYNC_FINISH;
        detail.code = code;
        async_(std::move(details));
    }
}

std::shared_ptr<GenQuery> SyncManager::SyncInfo::GenerateQuery(const std::string &store, const Tables &tables)
{
    if (query_ != nullptr) {
        return query_;
    }
    class SyncQuery final : public GenQuery {
    public:
        explicit SyncQuery(const std::vector<std::string> &tables) : tables_(tables)
        {
        }

        bool IsEqual(uint64_t tid) override
        {
            return false;
        }

        std::vector<std::string> GetTables() override
        {
            return tables_;
        }

    private:
        std::vector<std::string> tables_;
    };
    auto &syncTables = tables_[store];
    return std::make_shared<SyncQuery>(syncTables.empty() ? tables : syncTables);
}

SyncManager::SyncManager()
{
    EventCenter::GetInstance().Subscribe(CloudEvent::LOCAL_CHANGE, GetClientChangeHandler());
}

SyncManager::~SyncManager()
{
    if (executor_ != nullptr) {
        actives_.ForEachCopies([this](auto &syncId, auto &taskId) {
            executor_->Remove(taskId);
            return false;
        });
        executor_ = nullptr;
    }
}

int32_t SyncManager::Bind(std::shared_ptr<ExecutorPool> executor)
{
    executor_ = executor;
    return E_OK;
}

int32_t SyncManager::DoCloudSync(SyncInfo syncInfo)
{
    if (executor_ == nullptr) {
        return E_NOT_INIT;
    }

    actives_.Compute(GenSyncId(syncInfo.user_), [this, &syncInfo](const uint64_t &key, TaskId &taskId) mutable {
        taskId = executor_->Execute(GetSyncTask(0, GenSyncRef(key), std::move(syncInfo)));
        return true;
    });

    return E_OK;
}

int32_t SyncManager::StopCloudSync(int32_t user)
{
    if (executor_ == nullptr) {
        return E_NOT_INIT;
    }
    actives_.ForEachCopies([this, user](auto &syncId, auto &taskId) {
        if (Compare(syncId, user) == 0) {
            executor_->Remove(taskId);
        }
        return false;
    });
    return E_OK;
}

ExecutorPool::Task SyncManager::GetSyncTask(int32_t retry, RefCount ref, SyncInfo &&syncInfo)
{
    retry++;
    return [this, retry, ref = std::move(ref), info = std::move(syncInfo)]() mutable {
        EventCenter::Defer defer(GetSyncHandler(), CloudEvent::CLOUD_SYNC);
        CloudInfo cloud;
        cloud.user = info.user_;
        if (!MetaDataManager::GetInstance().LoadMeta(cloud.GetKey(), cloud, true)) {
            info.SetError(E_NOT_INIT);
            ZLOGE("no cloud info for user:%{public}d", info.user_);
            return;
        }

        if (!cloud.enableCloud || (info.id_ != SyncInfo::DEFAULT_ID && cloud.id != info.id_) ||
            (!info.bundleName_.empty() && !cloud.IsOn(info.bundleName_))) {
            info.SetError(E_UNOPENED);
            return;
        }

        std::vector<SchemaMeta> schemas;
        auto key = cloud.GetSchemaPrefix(info.bundleName_);
        if (!MetaDataManager::GetInstance().LoadMeta(key, schemas, true)) {
            DoRetry(retry, std::move(info));
            return;
        }

        for (auto &schema : schemas) {
            if (!cloud.IsOn(schema.bundleName)) {
                continue;
            }

            for (const auto &database : schema.databases) {
                CloudEvent::StoreInfo storeInfo;
                storeInfo.bundleName = schema.bundleName;
                storeInfo.user = cloud.user;
                storeInfo.storeName = database.name;
                storeInfo.instanceId = cloud.apps[schema.bundleName].instanceId;
                auto query = info.GenerateQuery(database.name, database.GetTableNames());
                auto evt = std::make_unique<SyncEvent>(std::move(storeInfo),
                    SyncEvent::EventInfo { info.mode_, info.wait_, std::move(query), info.async_ });
                EventCenter::GetInstance().PostEvent(std::move(evt));
            }
        }
    };
}

std::function<void(const Event &)> SyncManager::GetSyncHandler()
{
    return [](const Event &event) {
        auto &evt = static_cast<const SyncEvent &>(event);
        auto &storeInfo = evt.GetStoreInfo();
        auto instance = CloudServer::GetInstance();
        if (instance == nullptr) {
            ZLOGD("not support cloud sync");
            return;
        }

        StoreMetaData meta;
        meta.storeId = storeInfo.storeName;
        meta.bundleName = storeInfo.bundleName;
        meta.user = std::to_string(storeInfo.user);
        meta.instanceId = storeInfo.instanceId;
        meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
        if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta)) {
            ZLOGE("failed, no store meta bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
                meta.GetStoreAlias().c_str());
            return;
        }
        auto store = AutoCache::GetInstance().GetStore(meta, {});
        if (store == nullptr) {
            ZLOGE("store null, storeId:%{public}s", meta.GetStoreAlias().c_str());
            return;
        }

        if (!store->IsBound()) {
            CloudInfo info;
            info.user = storeInfo.user;
            SchemaMeta schemaMeta;
            std::string schemaKey = info.GetSchemaKey(storeInfo.bundleName, storeInfo.instanceId);
            if (!MetaDataManager::GetInstance().LoadMeta(std::move(schemaKey), schemaMeta, true)) {
                ZLOGE("failed, no schema bundleName:%{public}s, storeId:%{public}s", storeInfo.bundleName.c_str(),
                    Anonymous::Change(storeInfo.storeName).c_str());
                return;
            }
            auto dbMeta = schemaMeta.GetDataBase(storeInfo.storeName);
            auto cloudDB = instance->ConnectCloudDB(meta.tokenId, dbMeta);
            auto assetLoader = instance->ConnectAssetLoader(meta.tokenId, dbMeta);
            if (cloudDB == nullptr || assetLoader == nullptr) {
                ZLOGE("failed, no cloud DB or no assetLoader <0x%{public}x %{public}s<->%{public}s>", storeInfo.tokenId,
                    dbMeta.name.c_str(), dbMeta.alias.c_str());
                return;
            }
            store->Bind(dbMeta, {cloudDB, assetLoader});
        }
        ZLOGD("database:<%{public}d:%{public}s:%{public}s> sync start", storeInfo.user, storeInfo.bundleName.c_str(),
            Anonymous::Change(storeInfo.storeName).c_str());
        store->Sync({ SyncInfo::DEFAULT_ID }, evt.GetMode(), *(evt.GetQuery()), evt.GetAsyncDetail(), evt.GetWait());
    };
}

std::function<void(const Event &)> SyncManager::GetClientChangeHandler()
{
    return [this](const Event &event) {
        auto &evt = static_cast<const SyncEvent &>(event);
        auto store = evt.GetStoreInfo();
        SyncInfo syncInfo(store.user, store.bundleName, store.storeName);
        syncInfo.SetMode(evt.GetMode());
        syncInfo.SetWait(evt.GetWait());
        syncInfo.SetAsyncDetail(evt.GetAsyncDetail());
        syncInfo.SetQuery(evt.GetQuery());
        auto task = GetSyncTask(RETRY_TIMES, RefCount(), std::move(syncInfo));
        task();
    };
}

uint64_t SyncManager::GenSyncId(int32_t user)
{
    uint64_t syncId = static_cast<uint64_t>(user) & 0xFFFFFFFF;
    return (syncId << MV_BIT) | (++syncId_);
}

RefCount SyncManager::GenSyncRef(uint64_t syncId)
{
    return RefCount([syncId, this]() {
        actives_.Erase(syncId);
    });
}

int32_t SyncManager::Compare(uint64_t syncId, int32_t user)
{
    uint64_t inner = static_cast<uint64_t>(user) & 0xFFFFFFFF;
    return (syncId & USER_MARK) == (inner << MV_BIT);
}

void SyncManager::DoRetry(int32_t retry, SyncInfo &&info)
{
    CloudEvent::StoreInfo storeInfo;
    storeInfo.user = info.user_;
    storeInfo.bundleName = info.bundleName_;
    EventCenter::GetInstance().PostEvent(std::make_unique<CloudEvent>(CloudEvent::GET_SCHEMA, storeInfo));
    if (retry > RETRY_TIMES) {
        info.SetError(E_RETRY_TIMEOUT);
        return;
    }
    actives_.Compute(GenSyncId(info.user_), [this, retry, &info](const uint64_t &key, TaskId &value) mutable {
        value = executor_->Schedule(RETRY_INTERVAL, GetSyncTask(retry, GenSyncRef(key), std::move(info)));
        return true;
    });
}
} // namespace OHOS::CloudData