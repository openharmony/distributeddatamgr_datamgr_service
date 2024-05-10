/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include <chrono>

#include "account/account_delegate.h"
#include "cloud/cloud_server.h"
#include "cloud/schema_meta.h"
#include "cloud/sync_event.h"
#include "device_manager_adapter.h"
#include "eventcenter/event_center.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "sync_strategies/network_sync_strategy.h"
#include "user_delegate.h"
#include "utils/anonymous.h"

namespace OHOS::CloudData {
using namespace DistributedData;
using namespace DistributedKv;
using Account = OHOS::DistributedKv::AccountDelegate;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Defer = EventCenter::Defer;
std::atomic<uint32_t> SyncManager::genId_ = 0;
SyncManager::SyncInfo::SyncInfo(int32_t user, const std::string &bundleName, const Store &store, const Tables &tables)
    : user_(user), bundleName_(bundleName)
{
    if (!store.empty()) {
        tables_[store] = tables;
    }
    syncId_ = SyncManager::GenerateId(user);
}

SyncManager::SyncInfo::SyncInfo(int32_t user, const std::string &bundleName, const Stores &stores)
    : user_(user), bundleName_(bundleName)
{
    for (auto &store : stores) {
        tables_[store] = {};
    }
    syncId_ = SyncManager::GenerateId(user);
}

SyncManager::SyncInfo::SyncInfo(int32_t user, const std::string &bundleName, const MutliStoreTables &tables)
    : user_(user), bundleName_(bundleName), tables_(tables)
{
    tables_ = tables;
    syncId_ = SyncManager::GenerateId(user);
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

void SyncManager::SyncInfo::SetCompensation(bool isCompensation)
{
    isCompensation_ = isCompensation;
}

void SyncManager::SyncInfo::SetError(int32_t code) const
{
    if (async_) {
        GenDetails details;
        auto &detail = details[id_];
        detail.progress = GenProgress::SYNC_FINISH;
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
        explicit SyncQuery(const std::vector<std::string> &tables) : tables_(tables) {}

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
    auto it = tables_.find(store);
    return std::make_shared<SyncQuery>(it == tables_.end() || it->second.empty() ? tables : it->second);
}

bool SyncManager::SyncInfo::Contains(const std::string &storeName)
{
    return tables_.empty() || tables_.find(storeName) != tables_.end();
}

SyncManager::SyncManager()
{
    EventCenter::GetInstance().Subscribe(CloudEvent::LOCAL_CHANGE, GetClientChangeHandler());
    syncStrategy_ = std::make_shared<NetworkSyncStrategy>();
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
    auto syncId = GenerateId(syncInfo.user_);
    auto ref = GenSyncRef(syncId);
    actives_.Compute(syncId, [this, &ref, &syncInfo](const uint64_t &key, TaskId &taskId) mutable {
        taskId = executor_->Execute(GetSyncTask(0, true, ref, std::move(syncInfo)));
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

GeneralError SyncManager::IsValid(SyncInfo &info, CloudInfo &cloud)
{
    if (!MetaDataManager::GetInstance().LoadMeta(cloud.GetKey(), cloud, true) ||
        (info.id_ != SyncInfo::DEFAULT_ID && cloud.id != info.id_)) {
        info.SetError(E_CLOUD_DISABLED);
        ZLOGE("cloudInfo invalid:%{public}d, <syncId:%{public}s, metaId:%{public}s>", cloud.IsValid(),
            Anonymous::Change(info.id_).c_str(), Anonymous::Change(cloud.id).c_str());
        return E_CLOUD_DISABLED;
    }
    if (!cloud.enableCloud || (!info.bundleName_.empty() && !cloud.IsOn(info.bundleName_))) {
        info.SetError(E_CLOUD_DISABLED);
        ZLOGD("enable:%{public}d, bundleName:%{public}s", cloud.enableCloud, info.bundleName_.c_str());
        return E_CLOUD_DISABLED;
    }
    if (!DmAdapter::GetInstance().IsNetworkAvailable()) {
        info.SetError(E_NETWORK_ERROR);
        ZLOGD("network unavailable");
        return E_NETWORK_ERROR;
    }
    if (!Account::GetInstance()->IsVerified(info.user_)) {
        info.SetError(E_USER_UNLOCK);
        ZLOGD("user unverified");
        return E_ERROR;
    }
    return E_OK;
}

std::function<void()> SyncManager::GetPostEventTask(const std::vector<SchemaMeta> &schemas, CloudInfo &cloud,
    SyncInfo &info, bool retry)
{
    return [this, &cloud, &info, &schemas, retry]() {
        for (auto &schema : schemas) {
            if (!cloud.IsOn(schema.bundleName)) {
                continue;
            }
            for (const auto &database : schema.databases) {
                if (!info.Contains(database.name)) {
                    continue;
                }
                StoreInfo storeInfo = { 0, schema.bundleName, database.name, cloud.apps[schema.bundleName].instanceId,
                                        info.user_, "", info.syncId_ };
                auto status = syncStrategy_->CheckSyncAction(storeInfo);
                if (status != SUCCESS) {
                    ZLOGW("Verification strategy failed, status:%{public}d. %{public}d:%{public}s:%{public}s", status,
                        storeInfo.user, storeInfo.bundleName.c_str(), Anonymous::Change(storeInfo.storeName).c_str());
                    QueryKey queryKey{ cloud.id, schema.bundleName, "" };
                    UpdateFinishSyncInfo(queryKey, info.syncId_, E_BLOCKED_BY_NETWORK_STRATEGY);
                    info.SetError(status);
                    continue;
                }
                auto query = info.GenerateQuery(database.name, database.GetTableNames());
                SyncParam syncParam = { info.mode_, info.wait_, info.isCompensation_ };
                auto evt = std::make_unique<SyncEvent>(std::move(storeInfo),
                    SyncEvent::EventInfo{ syncParam, retry, std::move(query), info.async_ });
                EventCenter::GetInstance().PostEvent(std::move(evt));
            }
        }
    };
}

ExecutorPool::Task SyncManager::GetSyncTask(int32_t times, bool retry, RefCount ref, SyncInfo &&syncInfo)
{
    times++;
    return [this, times, retry, keep = std::move(ref), info = std::move(syncInfo)]() mutable {
        activeInfos_.Erase(info.syncId_);
        bool createdByDefaultUser = false;
        if (info.user_ == 0) {
            std::vector<int32_t> users;
            AccountDelegate::GetInstance()->QueryUsers(users);
            if (!users.empty()) {
                info.user_ = users[0];
            }
            createdByDefaultUser = true;
        }
        CloudInfo cloud;
        cloud.user = info.user_;

        std::vector<std::tuple<QueryKey, uint64_t>> cloudSyncInfos;
        GetCloudSyncInfo(info, cloud, cloudSyncInfos);
        if (cloudSyncInfos.empty()) {
            info.SetError(E_CLOUD_DISABLED);
            return;
        }
        UpdateStartSyncInfo(info, cloud, cloudSyncInfos);
        auto code = IsValid(info, cloud);
        if (code != E_OK) {
            for (const auto &[queryKey, syncId] : cloudSyncInfos) {
                UpdateFinishSyncInfo(queryKey, syncId, code);
            }
            return;
        }

        auto retryer = GetRetryer(times, info);
        auto schemas = GetSchemaMeta(cloud, info.bundleName_);
        if (schemas.empty()) {
            UpdateSchema(info);
            retryer(RETRY_INTERVAL, E_RETRY_TIMEOUT);
            return;
        }

        Defer defer(GetSyncHandler(std::move(retryer)), CloudEvent::CLOUD_SYNC);
        if (createdByDefaultUser) {
            info.user_ = 0;
        }
        auto task = GetPostEventTask(schemas, cloud, info, retry);
        if (task != nullptr) {
            task();
        }
    };
}

std::function<void(const Event &)> SyncManager::GetSyncHandler(Retryer retryer)
{
    return [this, retryer](const Event &event) {
        auto &evt = static_cast<const SyncEvent &>(event);
        auto &storeInfo = evt.GetStoreInfo();
        GenAsync async = evt.GetAsyncDetail();
        GenDetails details;
        auto &detail = details[SyncInfo::DEFAULT_ID];
        detail.progress = GenProgress::SYNC_FINISH;
        StoreMetaData meta(storeInfo);
        meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
        if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true)) {
            meta.user = "0"; // check if it is a public store.
            StoreMetaDataLocal localMetaData;
            if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKeyLocal(), localMetaData, true) ||
                !localMetaData.isPublic || !MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true)) {
                ZLOGE("failed, no store meta. bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
                    meta.GetStoreAlias().c_str());
                return DoExceptionalCallback(async, details, storeInfo);
            }
        }
        auto store = GetStore(meta, storeInfo.user);
        if (store == nullptr) {
            ZLOGE("store null, storeId:%{public}s", meta.GetStoreAlias().c_str());
            DoExceptionalCallback(async, details, storeInfo);
            return;
        }
        ZLOGD("database:<%{public}d:%{public}s:%{public}s> sync start", storeInfo.user, storeInfo.bundleName.c_str(),
            meta.GetStoreAlias().c_str());
        SyncParam syncParam = { evt.GetMode(), evt.GetWait(), evt.IsCompensation() };
        auto status = store->Sync({ SyncInfo::DEFAULT_ID }, *(evt.GetQuery()), evt.AutoRetry()
            ? [this, retryer, storeInfo](const GenDetails &details) {
                if (details.empty()) {
                    ZLOGE("retry, details empty");
                    return;
                }
                int32_t code = details.begin()->second.code;
                if (details.begin()->second.progress == GenProgress::SYNC_FINISH) {
                    QueryKey queryKey{ GetAccountId(storeInfo.user), storeInfo.bundleName, "" };
                    UpdateFinishSyncInfo(queryKey, storeInfo.syncId, code);
                }
                retryer(GetInterval(code), code);
            }
            : GetCallback(evt.GetAsyncDetail(), storeInfo), syncParam);
        if (status != E_OK && async) {
            detail.code = status;
            async(std::move(details));
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
        syncInfo.SetAsyncDetail(evt.GetAsyncDetail());
        syncInfo.SetQuery(evt.GetQuery());
        syncInfo.SetCompensation(evt.IsCompensation());
        auto times = evt.AutoRetry() ? RETRY_TIMES - CLIENT_RETRY_TIMES : RETRY_TIMES;
        auto task = GetSyncTask(times, evt.AutoRetry(), RefCount(), std::move(syncInfo));
        task();
    };
}

SyncManager::Retryer SyncManager::GetRetryer(int32_t times, const SyncInfo &syncInfo)
{
    if (times >= RETRY_TIMES) {
        return [info = SyncInfo(syncInfo)](Duration, int32_t code) mutable {
            if (code == E_OK || code == E_SYNC_TASK_MERGED) {
                return true;
            }
            info.SetError(code);
            return true;
        };
    }
    return [this, times, info = SyncInfo(syncInfo)](Duration interval, int32_t code) mutable {
        if (code == E_OK || code == E_SYNC_TASK_MERGED) {
            return true;
        }
        if (code == E_NO_SPACE_FOR_ASSET || code == E_RECODE_LIMIT_EXCEEDED) {
            info.SetError(code);
            return true;
        }

        activeInfos_.ComputeIfAbsent(info.syncId_, [this, times, interval, &info](uint64_t key) mutable {
            auto syncId = GenerateId(info.user_);
            auto ref = GenSyncRef(syncId);
            actives_.Compute(syncId, [this, times, interval, &ref, &info](const uint64_t &key, TaskId &value) mutable {
                value = executor_->Schedule(interval, GetSyncTask(times, true, ref, std::move(info)));
                return true;
            });
            return syncId;
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

void SyncManager::UpdateSchema(const SyncManager::SyncInfo &syncInfo)
{
    StoreInfo storeInfo;
    storeInfo.user = syncInfo.user_;
    storeInfo.bundleName = syncInfo.bundleName_;
    EventCenter::GetInstance().PostEvent(std::make_unique<CloudEvent>(CloudEvent::GET_SCHEMA, storeInfo));
}

std::map<uint32_t, GeneralStore::BindInfo> SyncManager::GetBindInfos(const StoreMetaData &meta,
    const std::vector<int32_t> &users, CloudInfo &info, Database &schemaDatabase, bool mustBind)
{
    auto instance = CloudServer::GetInstance();
    if (instance == nullptr) {
        ZLOGD("not support cloud sync");
        return {};
    }
    std::map<uint32_t, GeneralStore::BindInfo> bindInfos = {};
    for (auto &activeUser : users) {
        if (activeUser == 0) {
            continue;
        }
        auto cloudDB = instance->ConnectCloudDB(meta.bundleName, activeUser, schemaDatabase);
        auto assetLoader = instance->ConnectAssetLoader(meta.bundleName, activeUser, schemaDatabase);
        if (mustBind && (cloudDB == nullptr || assetLoader == nullptr)) {
            ZLOGE("failed, no cloud DB <0x%{public}x %{public}s<->%{public}s>", meta.tokenId,
                Anonymous::Change(schemaDatabase.name).c_str(), Anonymous::Change(schemaDatabase.alias).c_str());
            return {};
        }
        if (cloudDB != nullptr || assetLoader != nullptr) {
            GeneralStore::BindInfo bindInfo((cloudDB != nullptr) ? std::move(cloudDB) : cloudDB,
                (assetLoader != nullptr) ? std::move(assetLoader) : assetLoader);
            bindInfos[activeUser] = bindInfo;
        }
    }
    return bindInfos;
}

AutoCache::Store SyncManager::GetStore(const StoreMetaData &meta, int32_t user, bool mustBind)
{
    if (user != 0 && !Account::GetInstance()->IsVerified(user)) {
        ZLOGW("user:%{public}d is locked!", user);
        return nullptr;
    }
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
        std::vector<int32_t> users{};
        CloudInfo info;
        if (user == 0) {
            AccountDelegate::GetInstance()->QueryForegroundUsers(users);
            if (!users.empty()) {
                info.user = users[0];
            }
        } else {
            info.user = user;
            users.push_back(user);
        }
        SchemaMeta schemaMeta;
        std::string schemaKey = info.GetSchemaKey(meta.bundleName, meta.instanceId);
        if (!MetaDataManager::GetInstance().LoadMeta(std::move(schemaKey), schemaMeta, true)) {
            ZLOGE("failed, no schema bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
                meta.GetStoreAlias().c_str());
            return nullptr;
        }
        auto dbMeta = schemaMeta.GetDataBase(meta.storeId);
        std::map<uint32_t, GeneralStore::BindInfo> bindInfos = GetBindInfos(meta, users, info, dbMeta, mustBind);
        store->Bind(dbMeta, bindInfos);
    }
    return store;
}

void SyncManager::GetCloudSyncInfo(SyncInfo &info, CloudInfo &cloud,
    std::vector<std::tuple<QueryKey, uint64_t>> &cloudSyncInfos)
{
    if (!MetaDataManager::GetInstance().LoadMeta(cloud.GetKey(), cloud, true)) {
        ZLOGE("not exist cloud metadata, user: %{public}d.", cloud.user);
        return;
    }
    if (info.bundleName_.empty()) {
        for (const auto &it : cloud.apps) {
            QueryKey queryKey{ .accountId = cloud.id, .bundleName = it.first, .storeId = "" };
            cloudSyncInfos.emplace_back(std::make_tuple(queryKey, info.syncId_));
        }
    } else {
        QueryKey queryKey{ .accountId = cloud.id, .bundleName = info.bundleName_, .storeId = "" };
        cloudSyncInfos.emplace_back(std::make_tuple(queryKey, info.syncId_));
    }
}

int32_t SyncManager::QueryLastSyncInfo(const std::vector<QueryKey> &queryKeys, QueryLastResults &results)
{
    for (auto &queryKey : queryKeys) {
        std::string storeId = queryKey.storeId;
        QueryKey key{ .accountId = queryKey.accountId, .bundleName = queryKey.bundleName, .storeId = "" };
        auto it = lastSyncInfos_.find(key);
        if (it == lastSyncInfos_.end()) {
            return SUCCESS;
        }
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        it->second.ForEach([&storeId, &results](uint64_t syncId, CloudSyncInfo &info) {
            // -1 means sync not finish
            if (info.code != -1) {
                results.insert(std::pair<std::string, CloudSyncInfo>(storeId, info));
            }
            return SUCCESS;
        });
    }
    return SUCCESS;
}

void SyncManager::UpdateStartSyncInfo(SyncInfo &syncInfo, CloudInfo &cloud,
    std::vector<std::tuple<QueryKey, uint64_t>> &cloudSyncInfos)
{
    CloudSyncInfo info;
    info.startTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    for (auto &[queryKey, syncId] : cloudSyncInfos) {
        lastSyncInfos_[queryKey][syncId] = info;
    }
}

void SyncManager::UpdateFinishSyncInfo(const QueryKey &queryKey, uint64_t syncId, int32_t code)
{
    auto it = lastSyncInfos_.find(queryKey);
    if (it == lastSyncInfos_.end()) {
        return;
    }
    auto [isExist, info] = it->second.Find(syncId);
    if (!isExist) {
        return;
    }
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    it->second.EraseIf([syncId](uint64_t id, CloudSyncInfo &info) {
        // -1 means sync not finish
        return syncId != id && info.code != -1;
    });
    info.finishTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    info.code = code;
    lastSyncInfos_[queryKey][syncId] = info;
}

std::function<void(const GenDetails &result)> SyncManager::GetCallback(const GenAsync &async,
    const StoreInfo &storeInfo)
{
    return [this, async, storeInfo](const GenDetails &result) {
        if (async != nullptr) {
            async(result);
        }

        if (result.empty()) {
            ZLOGE("result is empty");
            return;
        }

        if (result.begin()->second.progress != GenProgress::SYNC_FINISH) {
            return;
        }

        auto id = GetAccountId(storeInfo.user);
        if (id.empty()) {
            return;
        }
        QueryKey queryKey{
            .accountId = id,
            .bundleName = storeInfo.bundleName,
            .storeId = ""
        };

        int32_t code = result.begin()->second.code;
        UpdateFinishSyncInfo(queryKey, storeInfo.syncId, code);
    };
}

std::string SyncManager::GetAccountId(int32_t user)
{
    CloudInfo cloudInfo;
    cloudInfo.user = user;
    if (!MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        ZLOGE("not exist meta, user:%{public}d.", cloudInfo.user);
        return "";
    }
    return cloudInfo.id;
}

ExecutorPool::Duration SyncManager::GetInterval(int32_t code)
{
    switch (code) {
        case E_LOCKED_BY_OTHERS:
            return LOCKED_INTERVAL;
        case E_BUSY:
            return BUSY_INTERVAL;
        default:
            return RETRY_INTERVAL;
    }
}

std::vector<SchemaMeta> SyncManager::GetSchemaMeta(const CloudInfo &cloud, const std::string &bundleName)
{
    std::vector<SchemaMeta> schemas;
    auto key = cloud.GetSchemaPrefix(bundleName);
    MetaDataManager::GetInstance().LoadMeta(key, schemas, true);
    return schemas;
}

void SyncManager::DoExceptionalCallback(const GenAsync &async, GenDetails &details, const StoreInfo &storeInfo)
{
    auto &detail = details[SyncInfo::DEFAULT_ID];
    if (async) {
        detail.code = E_ERROR;
        async(std::move(details));
    }
    QueryKey queryKey{ GetAccountId(storeInfo.user), storeInfo.bundleName, "" };
    UpdateFinishSyncInfo(queryKey, storeInfo.syncId, E_ERROR);
}
} // namespace OHOS::CloudData