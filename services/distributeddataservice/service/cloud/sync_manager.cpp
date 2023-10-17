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
using Defer = EventCenter::Defer;
std::atomic<uint32_t> SyncManager::genId_ = 0;
SyncManager::SyncInfo::SyncInfo(int32_t user, const std::string& bundleName, const Store& store, const Tables& tables)
{
    user_ = user;
    syncId_ = SyncManager::GenerateId(user);
    if (!bundleName.empty()) {
        stores_.push_back({ bundleName, store });
    }
}

SyncManager::SyncInfo::SyncInfo(int32_t user, const std::string& bundleName, const Stores& stores)
{
    user_ = user;
    syncId_ = SyncManager::GenerateId(user);
    if (!bundleName.empty()) {
        for (auto& store : stores) {
            stores_.push_back({ bundleName, store });
        }
    }
}

void SyncManager::SyncInfo::SetMode(int32_t mode)
{
    mode_ = mode;
}

void SyncManager::SyncInfo::SetWait(int32_t wait)
{
    wait_ = wait;
}

void SyncManager::SyncInfo::SetAsync(const std::string &bundleName, const Store& store, GenAsync asyncDetail)
{
    auto iterator = GetStoreInfo(bundleName, store);
    if (iterator != Iterator()) {
        iterator->async = std::move(asyncDetail);
    }
}

SyncManager::SyncInfo::Iterator SyncManager::SyncInfo::GetStoreInfo(const std::string& bundleName, const Store& store)
{
    for (auto it = stores_.begin(); it != stores_.end(); ++it) {
        if (it->bundleName == bundleName && it->storeName == store) {
            return it;
        }
    }
    return Iterator();
}

GenAsync SyncManager::SyncInfo::GetAsync(const std::string& bundleName, const Store& store)
{
    auto iterator = GetStoreInfo(bundleName, store);
    return iterator != Iterator() ? iterator->async : nullptr;
}

void SyncManager::SyncInfo::SetQuery(const std::string& bundleName, const Store& store,
    std::shared_ptr<GenQuery> query)
{
    auto iterator = GetStoreInfo(bundleName, store);
    if (iterator != Iterator()) {
        iterator->query = std::move(query);
    }
}

void SyncManager::SyncInfo::SetError(int32_t code) const
{
    GenDetails details;
    auto& detail = details[id_];
    detail.progress = SYNC_FINISH;
    detail.code = code;
    SetDetails(std::move(details));
}

void SyncManager::SyncInfo::SetDetails(GenDetails&& details, const std::string& bundleName, const Store& store) const
{
    for (auto& storeInfo : stores_) {
        if ((storeInfo.bundleName != bundleName && !bundleName.empty()) ||
            (storeInfo.storeName != store && !store.empty())) {
            continue;
        }
        if (storeInfo.async) {
            storeInfo.async(details);
        }
    }
}

std::shared_ptr<GenQuery> SyncManager::SyncInfo::GenerateQuery(const std::string& bundleName, const std::string& store,
    const Tables& tables)
{
    class SyncQuery final : public GenQuery {
    public:
        explicit SyncQuery(const std::vector<std::string>& tables) : tables_(tables) {}

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
    auto it = GetStoreInfo(bundleName, store);
    if (it != Iterator() && it->query != nullptr) {
        return it->query;
    }
    return std::make_shared<SyncQuery>(it == Iterator() || it->tables.empty() ? tables : it->tables);
}

bool SyncManager::SyncInfo::Contains(const std::string& bundleName, const std::string& storeName)
{
    if (stores_.empty()) {
        return true;
    }
    for (auto& storeInfo : stores_) {
        if (storeInfo.bundleName == bundleName && (storeInfo.storeName.empty() || storeInfo.storeName == storeName)) {
            return true;
        }
    }
    return false;
}

void SyncManager::SyncInfo::SetStoreInfo(StoreInfo&& storeInfo)
{
    if (storeInfo.bundleName.empty()) {
        return;
    }
    stores_.clear();
    stores_.push_back(std::move(storeInfo));
}

SyncManager::SyncInfo SyncManager::SyncInfo::CreateSyncInfo()
{
    SyncInfo info(user_);
    info.syncId_ = syncId_;
    info.mode_ = mode_;
    info.wait_ = wait_;
    info.id_ = id_;

    return info;
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

    actives_.Compute(GenerateId(syncInfo.user_), [this, &syncInfo](const uint64_t &key, TaskId &taskId) mutable {
        taskId = executor_->Execute(GetSyncTask(RETRY_TIMES, GenSyncRef(key), std::move(syncInfo)));
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

ExecutorPool::Task SyncManager::GetSyncTask(int32_t times, RefCount ref, SyncInfo &&syncInfo)
{
    times--;
    return [this, times, ref = std::move(ref), info = std::move(syncInfo)]() mutable {
        CloudInfo cloud;
        cloud.user = info.user_;
        if (!MetaDataManager::GetInstance().LoadMeta(cloud.GetKey(), cloud, true) || !cloud.enableCloud ||
            (info.id_ != SyncInfo::DEFAULT_ID && cloud.id != info.id_)) {
            info.SetError(E_CLOUD_DISABLED);
            ZLOGE("cloudInfo invalid:%{public}d, enable:%{public}d, <syncId:%{public}s, metaId:%{public}s>",
                cloud.IsValid(), cloud.enableCloud, Anonymous::Change(info.id_).c_str(),
                Anonymous::Change(cloud.id).c_str());
            return;
        }
        if (!DmAdapter::GetInstance().IsNetworkAvailable()) {
            info.SetError(E_NETWORK_ERROR);
            return;
        }
        ExecuteSync(times, std::move(info), cloud);
    };
}

void SyncManager::ExecuteSync(int32_t times, SyncManager::SyncInfo&& info, CloudInfo& cloud)
{
    auto schemas = GetSchemas(info, cloud);
    Defer defer(GetSyncHandler(GetRetryer(times, info.CreateSyncInfo())), CloudEvent::CLOUD_SYNC);
    for (auto& schema : schemas) {
        if (!cloud.IsOn(schema.bundleName)) {
            continue;
        }
        for (const auto& database : schema.databases) {
            if (!info.Contains(schema.bundleName, database.name)) {
                continue;
            }
            CloudEvent::StoreInfo storeInfo;
            storeInfo.bundleName = schema.bundleName;
            storeInfo.user = cloud.user;
            storeInfo.storeName = database.name;
            storeInfo.instanceId = cloud.apps[schema.bundleName].instanceId;
            auto async = info.GetAsync(storeInfo.bundleName, storeInfo.storeName);
            auto query = info.GenerateQuery(storeInfo.bundleName, storeInfo.storeName, database.GetTableNames());
            auto evt = std::make_unique<SyncEvent>(std::move(storeInfo),
                SyncEvent::EventInfo{ info.mode_, info.wait_, std::move(query), std::move(async)});
            EventCenter::GetInstance().PostEvent(std::move(evt));
        }
    }
}
std::vector<SchemaMeta> SyncManager::GetSchemas(const SyncManager::SyncInfo& info, const CloudInfo& cloud)
{
    std::vector<SchemaMeta> schemas;
    if (info.stores_.empty()) {
        MetaDataManager::GetInstance().LoadMeta(cloud.GetSchemaPrefix(""), schemas, true);
        return schemas;
    }
    for (auto& store : info.stores_) {
        auto key = cloud.GetSchemaPrefix(store.bundleName);
        std::vector<SchemaMeta> tmpSchemas;
        if (!MetaDataManager::GetInstance().LoadMeta(key, tmpSchemas, true) || schemas.empty()) {
            UpdateSchema(info.user_, store.bundleName);
        }
        MetaDataManager::GetInstance().LoadMeta(cloud.GetSchemaPrefix(""), schemas, true);
        schemas.insert(schemas.end(), std::make_move_iterator(tmpSchemas.begin()),
            std::make_move_iterator(tmpSchemas.end()));
    }
    return schemas;
}

std::function<void(const Event &)> SyncManager::GetSyncHandler(Retryer retryer)
{
    return [retryer](const Event &event) {
        auto &evt = static_cast<const SyncEvent &>(event);
        auto &storeInfo = evt.GetStoreInfo();
        StoreMetaData meta;
        meta.storeId = storeInfo.storeName;
        meta.bundleName = storeInfo.bundleName;
        meta.user = std::to_string(storeInfo.user);
        meta.instanceId = storeInfo.instanceId;
        meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
        StoreInfo retryStore(storeInfo.bundleName, storeInfo.storeName, evt.GetAsyncDetail());
        if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta)) {
            ZLOGE("failed, no store meta bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
                meta.GetStoreAlias().c_str());
            retryer(RETRY_INTERVAL, { storeInfo.storeName, E_UNOPENED }, std::move(retryStore));
            return;
        }
        auto store = GetStore(meta, storeInfo.user);
        if (store == nullptr) {
            retryer(RETRY_INTERVAL, {storeInfo.storeName, E_UNOPENED}, std::move(retryStore));
            return;
        }

        ZLOGD("database:<%{public}d:%{public}s:%{public}s> sync start", storeInfo.user, storeInfo.bundleName.c_str(),
            meta.GetStoreAlias().c_str());
        auto status = store->Sync(
            { SyncInfo::DEFAULT_ID }, evt.GetMode(), *(evt.GetQuery()),
            [retryer, retryStore](const GenDetails& details) mutable {
                int32_t code = details.empty() ? E_ERROR : details.begin()->second.code;
                retryer(code == E_LOCKED_BY_OTHERS ? LOCKED_INTERVAL : RETRY_INTERVAL, details, std::move(retryStore));
            },
            evt.GetWait());
        if (status != E_OK) {
            GenDetails details;
            auto& detail = details[SyncInfo::DEFAULT_ID];
            detail.progress = SYNC_FINISH;
            detail.code = status;
            retryer(RETRY_INTERVAL, std::move(details), std::move(retryStore));
        }
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
        syncInfo.SetAsync(store.bundleName, store.storeName, evt.GetAsyncDetail());
        syncInfo.SetQuery(store.bundleName, store.storeName, evt.GetQuery());
        auto times = evt.AutoSync() ? CLIENT_RETRY_TIMES : ONCE_TIME;
        auto task = GetSyncTask(times, RefCount(), std::move(syncInfo));
        task();
    };
}

SyncManager::Retryer SyncManager::GetRetryer(int32_t times, SyncInfo&& syncInfo)
{
    if (times <= 0) {
        return [info = std::move(syncInfo)](Duration, GenDetails&& details, StoreInfo&& storeInfo) mutable {
            for (auto& [key, value] : details) {
                value.progress = SYNC_FINISH;
            }
            info.SetStoreInfo(std::move(storeInfo));
            info.SetDetails(std::move(details));
            return true;
        };
    }
    return [this, times, info = std::move(syncInfo)](Duration interval, GenDetails&& details,
               StoreInfo&& storeInfo) mutable {
        if (!details.empty() && details.begin()->second.code == E_OK) {
            for (auto& [key, value] : details) {
                value.progress = SYNC_FINISH;
            }
            info.SetStoreInfo(std::move(storeInfo));
            info.SetDetails(std::move(details));
            return true;
        }

        auto syncId = GenerateId(info.user_);
        info.SetStoreInfo(std::move(storeInfo));
        actives_.Compute(syncId, [this, times, interval, &info](const uint64_t& key, TaskId& value) mutable {
            value = executor_->Schedule(interval, GetSyncTask(times, GenSyncRef(key), std::move(info)));
            return true;
        });
        return true;
    };
}

uint64_t SyncManager::GenerateId(int32_t user)
{
    uint64_t syncId = static_cast<uint64_t>(user) & 0xFFFFFFFF;
    return (syncId << MV_BIT) | (++genId_);
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

void SyncManager::UpdateSchema(int32_t user, const std::string& bundleName)
{
    CloudEvent::StoreInfo storeInfo;
    storeInfo.user = user;
    storeInfo.bundleName = bundleName;
    EventCenter::GetInstance().PostEvent(std::make_unique<CloudEvent>(CloudEvent::GET_SCHEMA, storeInfo));
}

AutoCache::Store SyncManager::GetStore(const StoreMetaData &meta, int32_t user, bool mustBind)
{
    auto instance = CloudServer::GetInstance();
    if (instance == nullptr) {
        ZLOGD("not support cloud sync");
        return nullptr;
    }

    auto store = AutoCache::GetInstance().GetStore(meta, {});
    if (store == nullptr) {
        ZLOGE("store null, storeId:%{public}s", meta.GetStoreAlias().c_str());
        return nullptr;
    }

    if (!store->IsBound()) {
        CloudInfo info;
        info.user = user;
        SchemaMeta schemaMeta;
        std::string schemaKey = info.GetSchemaKey(meta.bundleName, meta.instanceId);
        if (!MetaDataManager::GetInstance().LoadMeta(std::move(schemaKey), schemaMeta, true)) {
            ZLOGE("failed, no schema bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
                meta.GetStoreAlias().c_str());
            return nullptr;
        }
        auto dbMeta = schemaMeta.GetDataBase(meta.storeId);
        auto cloudDB = instance->ConnectCloudDB(meta.tokenId, dbMeta);
        auto assetLoader = instance->ConnectAssetLoader(meta.tokenId, dbMeta);
        if (mustBind && (cloudDB == nullptr || assetLoader == nullptr)) {
            ZLOGE("failed, no cloud DB <0x%{public}x %{public}s<->%{public}s>", meta.tokenId,
                Anonymous::Change(dbMeta.name).c_str(), Anonymous::Change(dbMeta.alias).c_str());
            return nullptr;
        }

        if (cloudDB != nullptr || assetLoader != nullptr) {
            store->Bind(dbMeta, { std::move(cloudDB), std::move(assetLoader) });
        }
    }
    return store;
}

SyncManager::Details::Details(int32_t code)
{
    details_[SyncInfo::DEFAULT_ID].code = code;
}

SyncManager::Details::Details(const std::string& storeName, int32_t code)
{
    details_[storeName].code = code;
}

SyncManager::Details::Details(GenDetails&& details) : details_(std::move(details)) {}

SyncManager::Details::Details(const GenDetails& details) : details_(details) {}

SyncManager::Details::operator GenDetails() const
{
    return details_;
}

SyncManager::Details::operator GenDetails()
{
    return std::move(details_);
}
} // namespace OHOS::CloudData