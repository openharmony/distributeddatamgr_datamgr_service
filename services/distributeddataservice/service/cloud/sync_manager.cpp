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
#include <unordered_set>

#include "account/account_delegate.h"
#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "cloud/cloud_lock_event.h"
#include "cloud/cloud_report.h"
#include "cloud/cloud_server.h"
#include "cloud/schema_meta.h"
#include "cloud_value_util.h"
#include "device_manager_adapter.h"
#include "dfx/dfx_types.h"
#include "dfx/reporter.h"
#include "eventcenter/event_center.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "network/network_delegate.h"
#include "screen/screen_manager.h"
#include "sync_strategies/network_sync_strategy.h"
#include "user_delegate.h"
#include "utils/anonymous.h"
namespace OHOS::CloudData {
using namespace DistributedData;
using namespace DistributedDataDfx;
using namespace DistributedKv;
using namespace SharingUtil;
using namespace std::chrono;
using Account = OHOS::DistributedData::AccountDelegate;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Defer = EventCenter::Defer;
std::atomic<uint32_t> SyncManager::genId_ = 0;
constexpr int32_t SYSTEM_USER_ID = 0;
constexpr int32_t NETWORK_DISCONNECT_TIMEOUT_HOURS = 20;
static constexpr const char *FT_GET_STORE = "GET_STORE";
static constexpr const char *FT_CALLBACK = "CALLBACK";
SyncManager::SyncInfo::SyncInfo(
    int32_t user, const std::string &bundleName, const Store &store, const Tables &tables, int32_t triggerMode)
    : user_(user), bundleName_(bundleName), triggerMode_(triggerMode)
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

SyncManager::SyncInfo::SyncInfo(const Param &param)
    : user_(param.user), bundleName_(param.bundleName), triggerMode_(param.triggerMode)
{
    if (!param.store.empty()) {
        tables_[param.store] = param.tables;
    }
    syncId_ = SyncManager::GenerateId(param.user);
    prepareTraceId_ = param.prepareTraceId;
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

std::vector<std::string> SyncManager::SyncInfo::GetTables(const Database &database)
{
    if (query_ != nullptr) {
        return query_->GetTables();
    }
    auto it = tables_.find(database.name);
    if (it != tables_.end() && !it->second.empty()) {
        return it->second;
    } 
    return database.GetTableNames();
}

void SyncManager::SyncInfo::SetCompensation(bool isCompensation)
{
    isCompensation_ = isCompensation;
}

void SyncManager::SyncInfo::SetTriggerMode(int32_t triggerMode)
{
    triggerMode_ = triggerMode;
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

std::shared_ptr<GenQuery> SyncManager::SyncInfo::GenerateQuery(const Tables &tables)
{
    if (query_ != nullptr && query_->GetTables().size() == 1) {
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
    return std::make_shared<SyncQuery>(tables);
}

bool SyncManager::SyncInfo::Contains(const std::string &storeName)
{
    return tables_.empty() || tables_.find(storeName) != tables_.end();
}

std::function<void(const Event &)> SyncManager::GetLockChangeHandler()
{
    return [](const Event &event) {
        auto &evt = static_cast<const CloudLockEvent &>(event);
        auto storeInfo = evt.GetStoreInfo();
        auto callback = evt.GetCallback();
        if (callback == nullptr) {
            ZLOGE("callback is nullptr. bundleName: %{public}s, storeName: %{public}s, user: %{public}d.",
                storeInfo.bundleName.c_str(), Anonymous::Change(storeInfo.storeName).c_str(), storeInfo.user);
            return;
        }
        CloudInfo cloud;
        cloud.user = storeInfo.user;
        SyncInfo info(storeInfo.user, storeInfo.bundleName);
        auto code = IsValid(info, cloud);
        if (code != E_OK) {
            return;
        }

        StoreMetaMapping meta(storeInfo);
        meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
        if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true)) {
            ZLOGE("no store mapping. bundleName: %{public}s, storeName: %{public}s, user: %{public}d.",
                storeInfo.bundleName.c_str(), Anonymous::Change(storeInfo.storeName).c_str(), storeInfo.user);
            return;
        }
        if (!meta.cloudPath.empty() && meta.dataDir != meta.cloudPath &&
            !MetaDataManager::GetInstance().LoadMeta(meta.GetCloudStoreMetaKey(), meta, true)) {
            ZLOGE("failed, no store meta. bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
                meta.GetStoreAlias().c_str());
            return;
        }
        auto [status, store] = GetStore(meta, storeInfo.user);
        if (store == nullptr) {
            ZLOGE("failed to get store. bundleName: %{public}s, storeName: %{public}s, user: %{public}d.",
                storeInfo.bundleName.c_str(), Anonymous::Change(storeInfo.storeName).c_str(), storeInfo.user);
            return;
        }
        if (evt.GetEventId() == CloudEvent::LOCK_CLOUD_CONTAINER) {
            auto [result, expiredTime] = store->LockCloudDB();
            callback(result, expiredTime);
        } else {
            auto result = store->UnLockCloudDB();
            callback(result, 0);
        }
    };
}

SyncManager::SyncManager()
{
    EventCenter::GetInstance().Subscribe(CloudEvent::LOCK_CLOUD_CONTAINER, SyncManager::GetLockChangeHandler());
    EventCenter::GetInstance().Subscribe(CloudEvent::UNLOCK_CLOUD_CONTAINER, SyncManager::GetLockChangeHandler());
    EventCenter::GetInstance().Subscribe(CloudEvent::LOCAL_CHANGE, GetClientChangeHandler());
    EventCenter::GetInstance().Subscribe(CloudEvent::SYNC_TRIGGER, GetSyncTriggerHandler());
    EventCenter::GetInstance().Subscribe(CloudEvent::SYNC_TRIGGER_CLEAN, GetSyncTriggerCleanHandler());
    syncStrategy_ = std::make_shared<NetworkSyncStrategy>();
    auto metaName = Bootstrap::GetInstance().GetProcessLabel();
    kvApps_.insert(std::move(metaName));
    auto stores = CheckerManager::GetInstance().GetStaticStores();
    for (auto &store : stores) {
        kvApps_.insert(std::move(store.bundleName));
    }
    stores = CheckerManager::GetInstance().GetDynamicStores();
    for (auto &store : stores) {
        kvApps_.insert(std::move(store.bundleName));
    }
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
    if (!NetworkDelegate::GetInstance()->IsNetworkAvailable()) {
        info.SetError(E_NETWORK_ERROR);
        ZLOGD("network unavailable");
        return E_NETWORK_ERROR;
    }
    if (!Account::GetInstance()->IsVerified(info.user_)) {
        info.SetError(E_USER_LOCKED);
        ZLOGD("user unverified");
        return E_ERROR;
    }
    return E_OK;
}

std::function<void()> SyncManager::GetPostEventTask(const std::vector<SchemaMeta> &schemas, CloudInfo &cloud,
    SyncInfo &info, bool retry, const TraceIds &traceIds)
{
    return [this, &cloud, &info, &schemas, retry, &traceIds]() {
        bool isPostEvent = false;
        auto syncId = info.syncId_;
        for (auto &schema : schemas) {
            std::string bundleName = schema.bundleName;
            std::string traceId = traceIds.count(bundleName) ? traceIds.at(bundleName) : "";
            if (!cloud.IsOn(bundleName)) {
                HandleSyncError({ cloud, bundleName, "", syncId, E_ERROR, "!IsOn:" + bundleName, traceId });
                continue;
            }
            for (const auto &database : schema.databases) {
                if (!info.Contains(database.name) && GetDataBaseCloudEnable(info.user_, bundleName, database.name)) {
                    HandleSyncError(
                        { cloud, bundleName, database.name, syncId, E_ERROR, "!Contains:" + database.name, traceId });
                    continue;
                }
                StoreInfo storeInfo = { 0, bundleName, database.name, cloud.apps[bundleName].instanceId, info.user_,
                    "", syncId };
                auto status = syncStrategy_->CheckSyncAction(storeInfo);
                if (status != SUCCESS) {
                    ZLOGW("Verification strategy failed, status:%{public}d. %{public}d:%{public}s:%{public}s", status,
                        storeInfo.user, storeInfo.bundleName.c_str(), Anonymous::Change(storeInfo.storeName).c_str());
                    HandleSyncError({ cloud, bundleName, database.name, syncId, status, "CheckSyncAction", traceId });
                    info.SetError(status);
                    continue;
                }

                auto tables = info.GetTables(database);
                if (!GetTableCloudEnable(info.user_, bundleName, database.name, tables)) {
                    HandleSyncError(
                        { cloud, bundleName, database.name, syncId, E_CLOUD_DISABLED, "tableDisable", traceId });
                    info.SetError(E_CLOUD_DISABLED);
                    continue;
                }
                auto query = info.GenerateQuery(tables);
                SyncParam syncParam = { info.mode_, info.wait_, info.isCompensation_, info.triggerMode_, traceId,
                    info.user_ };
                auto evt = std::make_unique<SyncEvent>(std::move(storeInfo),
                    SyncEvent::EventInfo{ syncParam, retry, std::move(query), info.async_ });
                EventCenter::GetInstance().PostEvent(std::move(evt));
                isPostEvent = true;
            }
        }
        if (!isPostEvent) {
            ZLOGE("schema is invalid, user: %{public}d", cloud.user);
            info.SetError(E_ERROR);
        }
    };
}

ExecutorPool::Task SyncManager::GetSyncTask(int32_t times, bool retry, RefCount ref, SyncInfo &&syncInfo)
{
    times++;
    return [this, times, retry, keep = std::move(ref), info = std::move(syncInfo)]() mutable {
        activeInfos_.Erase(info.syncId_);
        bool createdByDefaultUser = InitDefaultUser(info.user_);
        CloudInfo cloud;
        cloud.user = info.user_;

        auto cloudSyncInfos = GetCloudSyncInfo(info, cloud);
        if (cloudSyncInfos.empty()) {
            ZLOGD("get cloud info failed, user: %{public}d.", cloud.user);
            info.SetError(E_CLOUD_DISABLED);
            return;
        }
        auto traceIds = SyncManager::GetPrepareTraceId(info, cloud);
        BatchReport(info.user_, traceIds, SyncStage::PREPARE, E_OK);
        UpdateStartSyncInfo(cloudSyncInfos);
        auto code = IsValid(info, cloud);
        if (code != E_OK) {
            if (code == E_NETWORK_ERROR) {
                networkRecoveryManager_.RecordSyncApps(info.user_, info.bundleName_);
            }
            BatchUpdateFinishState(cloudSyncInfos, code);
            BatchReport(info.user_, traceIds, SyncStage::END, code, "!IsValid");
            return;
        }

        auto retryer = GetRetryer(times, info, cloud.user);
        auto schemas = GetSchemaMeta(cloud, info.bundleName_);
        if (schemas.empty()) {
            UpdateSchema(info);
            schemas = GetSchemaMeta(cloud, info.bundleName_);
            if (schemas.empty()) {
                auto it = traceIds.find(info.bundleName_);
                retryer(RETRY_INTERVAL, E_RETRY_TIMEOUT, GenStore::CLOUD_ERR_OFFSET + E_CLOUD_DISABLED,
                    it == traceIds.end() ? "" : it->second);
                BatchUpdateFinishState(cloudSyncInfos, E_CLOUD_DISABLED);
                BatchReport(info.user_, traceIds, SyncStage::END, E_CLOUD_DISABLED, "empty schema:" + info.bundleName_);
                return;
            }
        }
        Defer defer(GetSyncHandler(std::move(retryer), info), CloudEvent::CLOUD_SYNC);
        if (createdByDefaultUser) {
            info.user_ = 0;
        }
        auto task = GetPostEventTask(schemas, cloud, info, retry, traceIds);
        task();
    };
}

void SyncManager::StartCloudSync(const DistributedData::SyncEvent &evt, const StoreMetaData &meta,
    const AutoCache::Store &store, Retryer retryer, DistributedData::GenDetails &details)
{
    auto &storeInfo = evt.GetStoreInfo();
    GenAsync async = evt.GetAsyncDetail();
    auto prepareTraceId = evt.GetPrepareTraceId();
    auto user = evt.GetUser();
    auto &detail = details[SyncInfo::DEFAULT_ID];
    ReportSyncEvent(evt, BizState::BEGIN, E_OK);
    SyncParam syncParam = { evt.GetMode(), evt.GetWait(), evt.IsCompensation(), MODE_DEFAULT, prepareTraceId };
    syncParam.asyncDownloadAsset = meta.asyncDownloadAsset;
    auto [status, dbCode] = store->Sync({ SyncInfo::DEFAULT_ID }, *(evt.GetQuery()),
        evt.AutoRetry() ? RetryCallback(storeInfo, retryer, evt.GetTriggerMode(), prepareTraceId, user)
                        : GetCallback(async, storeInfo, evt.GetTriggerMode(), prepareTraceId, user), syncParam);
    if (status != E_OK) {
        if (async) {
            detail.code = ConvertValidGeneralCode(status);
            async(std::move(details));
        }
        UpdateFinishSyncInfo({ storeInfo.user, GetAccountId(storeInfo.user), storeInfo.bundleName,
            storeInfo.storeName }, storeInfo.syncId, E_ERROR);
        if (status != GeneralError::E_NOT_SUPPORT) {
            auto code = dbCode == 0 ? GenStore::CLOUD_ERR_OFFSET + status : dbCode;
            ReportSyncEvent(evt, BizState::END, code);
        }
    }
}

std::function<void(const Event &)> SyncManager::GetSyncHandler(Retryer retryer, const SyncInfo &info)
{
    return [this, retryer, info](const Event &event) {
        auto &evt = static_cast<const SyncEvent &>(event);
        auto &storeInfo = evt.GetStoreInfo();
        GenAsync async = evt.GetAsyncDetail();
        auto prepareTraceId = evt.GetPrepareTraceId();
        auto isAutoSync = evt.AutoRetry();
        GenDetails details;
        auto &detail = details[SyncInfo::DEFAULT_ID];
        detail.progress = GenProgress::SYNC_FINISH;
        auto exCallback = [this, async, &details, &storeInfo, &prepareTraceId](int32_t code, const std::string &msg) {
            return DoExceptionalCallback(async, details, storeInfo, {0, "", prepareTraceId, SyncStage::END, code, msg});
        };
        auto [hasMeta, meta] = GetMetaData(storeInfo);
        if (!hasMeta) {
            return exCallback(GeneralError::E_ERROR, "no meta");
        }
        auto [code, store] = GetStore(meta, storeInfo.user);
        if (code == E_SCREEN_LOCKED) {
            AddCompensateSync(meta);
        }
        if (store == nullptr) {
            ZLOGE("store null, storeId:%{public}s, prepareTraceId:%{public}s", meta.GetStoreAlias().c_str(),
                prepareTraceId.c_str());
            return exCallback(GeneralError::E_ERROR, "store null");
        }
        if (!meta.enableCloud) {
            ZLOGW("meta.enableCloud is false, storeId:%{public}s, prepareTraceId:%{public}s",
                meta.GetStoreAlias().c_str(), prepareTraceId.c_str());
            return exCallback(E_CLOUD_DISABLED, "disable cloud");
        }
        if (!meta.autoSyncSwitch) {
            auto ret = SetCloudConflictHandler(store);
            if (ret != E_OK) {
                return exCallback(GeneralError::E_ERROR, "SetCloudConflictHandler failed");
            }
            if ((info.triggerMode_ == MODE_PUSH) || (info.triggerMode_ == MODE_SWITCHON) ||
                (info.triggerMode_ == MODE_PROCESSSTART)) {
                ZLOGW("triggerMode: %{public}d, bundleName: %{public}s", info.triggerMode_, info.bundleName_.c_str());
                store->OnSyncTrigger(storeInfo.storeName, info.triggerMode_);
                return exCallback(E_OK, "syncTrigger");
            }
            if (isAutoSync) {
                return exCallback(E_OK, "syncTrigger isAutoSync");
            }
        }
        ZLOGI("database:<%{public}d:%{public}s:%{public}s:%{public}s> sync start, asyncDownloadAsset?[%{public}d],"
            "mode:%{public}d", storeInfo.user, storeInfo.bundleName.c_str(), meta.GetStoreAlias().c_str(),
            prepareTraceId.c_str(), meta.asyncDownloadAsset, info.triggerMode_);
        StartCloudSync(evt, meta, store, retryer, details);
    };
}

void SyncManager::ReportSyncEvent(const SyncEvent &evt, BizState bizState, int32_t code)
{
    auto &storeInfo = evt.GetStoreInfo();
    if (bizState == BizState::BEGIN) {
        RadarReporter::Report({storeInfo.bundleName.c_str(), CLOUD_SYNC, TRIGGER_SYNC,
            storeInfo.syncId, evt.GetTriggerMode()}, "GetSyncHandler", bizState);
    } else {
        RadarReporter::Report({storeInfo.bundleName.c_str(), CLOUD_SYNC, FINISH_SYNC,
            storeInfo.syncId, evt.GetTriggerMode(), code}, "GetSyncHandler", bizState);
    }
    SyncStage syncStage = (bizState == BizState::BEGIN) ? SyncStage::START : SyncStage::END;
    SyncManager::Report({evt.GetUser(), storeInfo.bundleName, evt.GetPrepareTraceId(), syncStage, code,
        "GetSyncHandler"});
}

std::function<void(const Event &)> SyncManager::GetClientChangeHandler()
{
    return [this](const Event &event) {
        if (executor_ == nullptr) {
            return;
        }
        auto &evt = static_cast<const SyncEvent &>(event);
        auto store = evt.GetStoreInfo();
        SyncInfo syncInfo(store.user, store.bundleName, store.storeName);
        syncInfo.SetMode(evt.GetMode());
        syncInfo.SetWait(evt.GetWait());
        syncInfo.SetAsyncDetail(evt.GetAsyncDetail());
        syncInfo.SetQuery(evt.GetQuery());
        syncInfo.SetCompensation(evt.IsCompensation());
        syncInfo.SetTriggerMode(evt.GetTriggerMode());
        auto times = evt.AutoRetry() ? RETRY_TIMES - CLIENT_RETRY_TIMES : RETRY_TIMES;
        executor_->Execute(GetSyncTask(times, evt.AutoRetry(), RefCount(), std::move(syncInfo)));
    };
}

std::function<void(const Event &)> SyncManager::GetSyncTriggerHandler()
{
    return [this](const Event &event) {
        auto &evt = static_cast<const CloudEvent &>(event);
        auto storeInfo = evt.GetStoreInfo();
        std::string key = storeInfo.bundleName + storeInfo.storeName + std::to_string(storeInfo.user);
        syncTriggerMap_.Insert(key, storeInfo);
    };
}
std::function<void(const Event &)> SyncManager::GetSyncTriggerCleanHandler()
{
    return [this](const Event &event) {
        auto &evt = static_cast<const CloudEvent &>(event);
        auto storeInfo = evt.GetStoreInfo();
        std::string key = storeInfo.bundleName + storeInfo.storeName + std::to_string(storeInfo.user);
        syncTriggerMap_.Erase(key);
    };
}

void SyncManager::Report(
    const std::string &faultType, const std::string &bundleName, int32_t errCode, const std::string &appendix)
{
    ArkDataFaultMsg msg = { .faultType = faultType,
        .bundleName = bundleName,
        .moduleName = ModuleName::CLOUD_SERVER,
        .errorType = errCode + GenStore::CLOUD_ERR_OFFSET,
        .appendixMsg = appendix };
    Reporter::GetInstance()->CloudSyncFault()->Report(msg);
}

bool SyncManager::HandleRetryFinished(const SyncInfo &info, int32_t user, int32_t code, int32_t dbCode,
    const std::string &prepareTraceId)
{
    if (code == E_OK || code == E_SYNC_TASK_MERGED) {
        return true;
    }
    if (code == E_NETWORK_ERROR) {
        networkRecoveryManager_.RecordSyncApps(user, info.bundleName_);
    }
    info.SetError(code);
    RadarReporter::Report({ info.bundleName_.c_str(), CLOUD_SYNC, FINISH_SYNC, info.syncId_, info.triggerMode_,
                            dbCode },
        "GetRetryer", BizState::END);
    SyncManager::Report({ user, info.bundleName_, prepareTraceId, SyncStage::END,
        dbCode == GenStore::DB_ERR_OFFSET ? 0 : dbCode, "GetRetryer finish" });
    Report(FT_CALLBACK, info.bundleName_, static_cast<int32_t>(Fault::CSF_GS_CLOUD_SYNC),
        "code=" + std::to_string(code) + ",dbCode=" + std::to_string(static_cast<int32_t>(dbCode)));
    return true;
}

SyncManager::Retryer SyncManager::GetRetryer(int32_t times, const SyncInfo &syncInfo, int32_t user)
{
    if (times >= RETRY_TIMES) {
        return [this, user, info = SyncInfo(syncInfo)](Duration, int32_t code, int32_t dbCode,
                   const std::string &prepareTraceId) mutable {
            return HandleRetryFinished(info, user, code, dbCode, prepareTraceId);
        };
    }
    return [this, times, user, info = SyncInfo(syncInfo)](Duration interval, int32_t code, int32_t dbCode,
               const std::string &prepareTraceId) mutable {
        if (code == E_OK || code == E_SYNC_TASK_MERGED) {
            return true;
        }
        if (code == E_NO_SPACE_FOR_ASSET || code == E_RECODE_LIMIT_EXCEEDED) {
            info.SetError(code);
            RadarReporter::Report({ info.bundleName_.c_str(), CLOUD_SYNC, FINISH_SYNC, info.syncId_, info.triggerMode_,
                                    dbCode },
                "GetRetryer", BizState::END);
            SyncManager::Report({ user, info.bundleName_, prepareTraceId, SyncStage::END,
                dbCode == GenStore::DB_ERR_OFFSET ? 0 : dbCode, "GetRetryer continue" });
            Report(FT_CALLBACK, info.bundleName_, static_cast<int32_t>(Fault::CSF_GS_CLOUD_SYNC),
                   "code=" + std::to_string(code) + ",dbCode=" + std::to_string(static_cast<int32_t>(dbCode)));
            return true;
        }

        if (executor_ == nullptr) {
            return false;
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
    const std::vector<int32_t> &users, const Database &schemaDatabase)
{
    auto instance = CloudServer::GetInstance();
    if (instance == nullptr) {
        ZLOGD("not support cloud sync");
        return {};
    }
    std::map<uint32_t, GeneralStore::BindInfo> bindInfos;
    for (auto &activeUser : users) {
        if (activeUser == 0) {
            continue;
        }
        auto cloudDB = instance->ConnectCloudDB(meta.bundleName, activeUser, schemaDatabase);
        if (cloudDB == nullptr) {
            ZLOGE("failed, no cloud DB <%{public}d:0x%{public}x %{public}s<->%{public}s>", meta.tokenId, activeUser,
                Anonymous::Change(schemaDatabase.name).c_str(), Anonymous::Change(schemaDatabase.alias).c_str());
            Report(FT_GET_STORE, meta.bundleName, static_cast<int32_t>(Fault::CSF_CONNECT_CLOUD_DB),
                "ConnectCloudDB failed, database=" + schemaDatabase.name);
            return {};
        }
        if (meta.storeType >= StoreMetaData::StoreType::STORE_KV_BEGIN &&
            meta.storeType <= StoreMetaData::StoreType::STORE_KV_END) {
            bindInfos.insert_or_assign(activeUser, GeneralStore::BindInfo{ std::move(cloudDB), nullptr });
            continue;
        }
        auto assetLoader = instance->ConnectAssetLoader(meta.bundleName, activeUser, schemaDatabase);
        if (assetLoader == nullptr) {
            ZLOGE("failed, no cloud DB <%{public}d:0x%{public}x %{public}s<->%{public}s>", meta.tokenId, activeUser,
                Anonymous::Change(schemaDatabase.name).c_str(), Anonymous::Change(schemaDatabase.alias).c_str());
            Report(FT_GET_STORE, meta.bundleName, static_cast<int32_t>(Fault::CSF_CONNECT_CLOUD_ASSET_LOADER),
                "ConnectAssetLoader failed, database=" + schemaDatabase.name);
            return {};
        }
        bindInfos.insert_or_assign(activeUser, GeneralStore::BindInfo{ std::move(cloudDB), std::move(assetLoader) });
    }
    return bindInfos;
}

std::pair<int32_t, AutoCache::Store> SyncManager::GetStore(const StoreMetaData &meta, int32_t user, bool mustBind)
{
    if (user != 0 && !Account::GetInstance()->IsVerified(user)) {
        ZLOGW("user:%{public}d is locked!", user);
        return { E_USER_LOCKED, nullptr };
    }
    if (CloudServer::GetInstance() == nullptr) {
        return { E_NOT_SUPPORT, nullptr };
    }
    auto [status, store] = AutoCache::GetInstance().GetDBStore(meta, {});
    if (status == E_SCREEN_LOCKED) {
        return { E_SCREEN_LOCKED, nullptr };
    } else if (store == nullptr) {
        return { E_ERROR, nullptr };
    }
    CloudInfo info;
    info.user = user;
    std::vector<int32_t> users{};
    if (info.user == SYSTEM_USER_ID) {
        AccountDelegate::GetInstance()->QueryForegroundUsers(users);
        info.user = users.empty() ? SYSTEM_USER_ID : users[0];
    } else {
        users.push_back(user);
    }
    if (info.user == SYSTEM_USER_ID) {
        ZLOGE("invalid cloud users, bundleName:%{public}s", meta.bundleName.c_str());
        return { E_ERROR, nullptr };
    }
    if (!store->IsBound(info.user)) {
        SchemaMeta schemaMeta;
        std::string schemaKey = info.GetSchemaKey(meta.bundleName, meta.instanceId);
        if (!MetaDataManager::GetInstance().LoadMeta(schemaKey, schemaMeta, true)) {
            ZLOGE("failed, no schema bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
                meta.GetStoreAlias().c_str());
            return { E_ERROR, nullptr };
        }
        auto dbMeta = schemaMeta.GetDataBase(meta.storeId);
        std::map<uint32_t, GeneralStore::BindInfo> bindInfos = GetBindInfos(meta, users, dbMeta);
        if (mustBind && bindInfos.size() != users.size()) {
            return { E_ERROR, nullptr };
        }
        GeneralStore::CloudConfig config;
        if (MetaDataManager::GetInstance().LoadMeta(info.GetKey(), info, true)) {
            config.maxNumber = info.maxNumber;
            config.maxSize = info.maxSize;
            config.isSupportEncrypt = schemaMeta.e2eeEnable;
        }
        store->Bind(dbMeta, bindInfos, config);
    }
    return { E_OK, store };
}

void SyncManager::Report(const ReportParam &reportParam)
{
    auto cloudReport = CloudReport::GetInstance();
    if (cloudReport == nullptr) {
        return;
    }
    cloudReport->Report(reportParam);
}

SyncManager::TraceIds SyncManager::GetPrepareTraceId(const SyncInfo &info, const CloudInfo &cloud)
{
    TraceIds traceIds;
    if (!info.prepareTraceId_.empty()) {
        traceIds.emplace(info.bundleName_, info.prepareTraceId_);
        return traceIds;
    }
    auto cloudReport = CloudReport::GetInstance();
    if (cloudReport == nullptr) {
        return traceIds;
    }
    if (info.bundleName_.empty()) {
        for (const auto &it : cloud.apps) {
            traceIds.emplace(it.first, cloudReport->GetPrepareTraceId(info.user_));
        }
    } else {
        traceIds.emplace(info.bundleName_, cloudReport->GetPrepareTraceId(info.user_));
    }
    return traceIds;
}

bool SyncManager::NeedGetCloudInfo(CloudInfo &cloud)
{
    return (!MetaDataManager::GetInstance().LoadMeta(cloud.GetKey(), cloud, true) || !cloud.enableCloud) &&
           NetworkDelegate::GetInstance()->IsNetworkAvailable() && Account::GetInstance()->IsLoginAccount();
}

std::vector<std::tuple<QueryKey, uint64_t>> SyncManager::GetCloudSyncInfo(const SyncInfo &info, CloudInfo &cloud)
{
    std::vector<std::tuple<QueryKey, uint64_t>> cloudSyncInfos;
    if (NeedGetCloudInfo(cloud)) {
        ZLOGI("get cloud info from server, user: %{public}d.", cloud.user);
        auto instance = CloudServer::GetInstance();
        if (instance == nullptr) {
            return cloudSyncInfos;
        }
        int32_t errCode = SUCCESS;
        std::tie(errCode, cloud) = instance->GetServerInfo(cloud.user, false);
        if (!cloud.IsValid()) {
            ZLOGE("cloud is empty, user: %{public}d", cloud.user);
            return cloudSyncInfos;
        }
        CloudInfo oldInfo;
        if (!MetaDataManager::GetInstance().LoadMeta(cloud.GetKey(), oldInfo, true) || oldInfo != cloud) {
            MetaDataManager::GetInstance().SaveMeta(cloud.GetKey(), cloud, true);
        }
    }
    auto schemaKey = CloudInfo::GetSchemaKey(cloud.user, info.bundleName_);
    SchemaMeta schemaMeta;
    if (!MetaDataManager::GetInstance().LoadMeta(schemaKey, schemaMeta, true)) {
        ZLOGE("load schema fail, bundleName: %{public}s, user %{public}d", info.bundleName_.c_str(), info.user_);
        return cloudSyncInfos;
    }
    auto stores = schemaMeta.GetStores();
    for (auto &storeId : stores) {
        QueryKey queryKey{ cloud.user, cloud.id, info.bundleName_, std::move(storeId) };
        cloudSyncInfos.emplace_back(std::make_tuple(std::move(queryKey), info.syncId_));
    }
    return cloudSyncInfos;
}

std::pair<int32_t, CloudLastSyncInfo> SyncManager::GetLastResults(std::map<SyncId, CloudLastSyncInfo> &infos)
{
    auto iter = infos.rbegin();
    if (iter != infos.rend() && iter->second.code != -1) {
        return { SUCCESS, iter->second };
    }
    return { E_ERROR, {} };
}

bool SyncManager::NeedSaveSyncInfo(const QueryKey &queryKey)
{
    if (queryKey.accountId.empty()) {
        return false;
    }
    return kvApps_.find(queryKey.bundleName) == kvApps_.end();
}

std::pair<int32_t, std::map<std::string, CloudLastSyncInfo>> SyncManager::QueryLastSyncInfo(
    const std::vector<QueryKey> &queryKeys)
{
    std::map<std::string, CloudLastSyncInfo> lastSyncInfoMap;
    for (const auto &queryKey : queryKeys) {
        std::string storeId = queryKey.storeId;
        QueryKey key{ queryKey.user, queryKey.accountId, queryKey.bundleName, queryKey.storeId };
        lastSyncInfos_.ComputeIfPresent(
            key, [&storeId, &lastSyncInfoMap](auto &key, std::map<SyncId, CloudLastSyncInfo> &vals) {
                auto [status, syncInfo] = GetLastResults(vals);
                if (status == SUCCESS) {
                    lastSyncInfoMap.insert(std::make_pair(std::move(storeId), std::move(syncInfo)));
                }
                return !vals.empty();
            });
        if (lastSyncInfoMap.find(queryKey.storeId) != lastSyncInfoMap.end()) {
            continue;
        }
        auto [status, syncInfo] = SyncManager::GetLastSyncInfoFromMeta(queryKey);
        if (status == SUCCESS) {
            lastSyncInfoMap.insert(std::make_pair(std::move(syncInfo.storeId), std::move(syncInfo)));
        }
    }
    return { SUCCESS, lastSyncInfoMap };
}

void SyncManager::UpdateStartSyncInfo(const std::vector<std::tuple<QueryKey, uint64_t>> &cloudSyncInfos)
{
    int64_t startTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    for (const auto &[queryKey, syncId] : cloudSyncInfos) {
        if (!NeedSaveSyncInfo(queryKey)) {
            continue;
        }
        lastSyncInfos_.Compute(queryKey, [id = syncId, startTime](auto &key, std::map<SyncId, CloudLastSyncInfo> &val) {
            CloudLastSyncInfo syncInfo;
            syncInfo.id = key.accountId;
            syncInfo.storeId = key.storeId;
            syncInfo.startTime = startTime;
            syncInfo.code = 0;
            val[id] = std::move(syncInfo);
            return !val.empty();
        });
    }
}

void SyncManager::UpdateFinishSyncInfo(const QueryKey &queryKey, uint64_t syncId, int32_t code)
{
    if (!NeedSaveSyncInfo(queryKey)) {
        return;
    }
    lastSyncInfos_.ComputeIfPresent(queryKey, [syncId, code](auto &key, std::map<SyncId, CloudLastSyncInfo> &val) {
        auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        for (auto iter = val.begin(); iter != val.end();) {
            bool isExpired = ((now - iter->second.startTime) >= EXPIRATION_TIME) && iter->second.code == -1;
            if ((iter->first != syncId && ((iter->second.code != -1) || isExpired))) {
                iter = val.erase(iter);
            } else if (iter->first == syncId) {
                iter->second.finishTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                iter->second.code = ConvertValidGeneralCode(code);
                iter->second.syncStatus = SyncStatus::FINISHED;
                SaveLastSyncInfo(key, std::move(iter->second));
                iter = val.erase(iter);
            } else {
                iter++;
            }
        }
        return true;
    });
}

std::function<void(const GenDetails &result)> SyncManager::GetCallback(const GenAsync &async,
    const StoreInfo &storeInfo, int32_t triggerMode, const std::string &prepareTraceId, int32_t user)
{
    return [this, async, storeInfo, triggerMode, prepareTraceId, user](const GenDetails &result) {
        if (async != nullptr) {
            async(ConvertGenDetailsCode(result));
        }

        if (result.empty()) {
            ZLOGE("result is empty");
            return;
        }

        if (result.begin()->second.progress != GenProgress::SYNC_FINISH) {
            return;
        }

        int32_t dbCode = (result.begin()->second.dbCode == GenStore::DB_ERR_OFFSET) ? 0 : result.begin()->second.dbCode;
        RadarReporter::Report({ storeInfo.bundleName.c_str(), CLOUD_SYNC, FINISH_SYNC, storeInfo.syncId, triggerMode,
                                dbCode, result.begin()->second.changeCount },
            "GetCallback", BizState::END);
        SyncManager::Report({ user, storeInfo.bundleName, prepareTraceId, SyncStage::END, dbCode, "GetCallback" });
        if (dbCode != 0) {
            Report(FT_CALLBACK, storeInfo.bundleName, static_cast<int32_t>(Fault::CSF_GS_CLOUD_SYNC),
                "callback failed, dbCode=" + std::to_string(dbCode));
        }
        auto id = GetAccountId(storeInfo.user);
        if (id.empty()) {
            ZLOGD("account id is empty");
            return;
        }
        int32_t code = result.begin()->second.code;
        UpdateFinishSyncInfo({ user, id, storeInfo.bundleName, storeInfo.storeName }, storeInfo.syncId, code);
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

void SyncManager::DoExceptionalCallback(const GenAsync &async, GenDetails &details, const StoreInfo &storeInfo,
    const ReportParam &param)
{
    if (async) {
        details[SyncInfo::DEFAULT_ID].code = param.errCode;
        async(details);
    }
    QueryKey queryKey{ storeInfo.user, GetAccountId(storeInfo.user), storeInfo.bundleName, storeInfo.storeName };
    UpdateFinishSyncInfo(queryKey, storeInfo.syncId, param.errCode);
    SyncManager::Report({ storeInfo.user, storeInfo.bundleName, param.prepareTraceId, SyncStage::END,
        param.errCode, param.message });
}

bool SyncManager::InitDefaultUser(int32_t &user)
{
    if (user != 0) {
        return false;
    }
    std::vector<int32_t> users;
    AccountDelegate::GetInstance()->QueryUsers(users);
    if (!users.empty()) {
        user = users[0];
    }
    return true;
}

std::function<void(const DistributedData::GenDetails &result)> SyncManager::RetryCallback(const StoreInfo &storeInfo,
    Retryer retryer, int32_t triggerMode, const std::string &prepareTraceId, int32_t user)
{
    return [this, retryer, storeInfo, triggerMode, prepareTraceId, user](const GenDetails &details) {
        if (details.empty()) {
            ZLOGE("retry, details empty");
            return;
        }
        int32_t code = details.begin()->second.code;
        int32_t dbCode = details.begin()->second.dbCode;
        if (details.begin()->second.progress == GenProgress::SYNC_FINISH) {
            auto user = storeInfo.user;
            QueryKey queryKey{ user, GetAccountId(user), storeInfo.bundleName, storeInfo.storeName };
            UpdateFinishSyncInfo(queryKey, storeInfo.syncId, code);
            if (code == E_OK) {
                RadarReporter::Report({ storeInfo.bundleName.c_str(), CLOUD_SYNC, FINISH_SYNC, storeInfo.syncId,
                                        triggerMode, code, details.begin()->second.changeCount },
                    "RetryCallback", BizState::END);
                SyncManager::Report({ user, storeInfo.bundleName, prepareTraceId, SyncStage::END,
                    dbCode == GenStore::DB_ERR_OFFSET ? 0 : dbCode, "RetryCallback" });
            }
        }
        retryer(GetInterval(code), code, dbCode, prepareTraceId);
    };
}

void SyncManager::BatchUpdateFinishState(const std::vector<std::tuple<QueryKey, uint64_t>> &cloudSyncInfos,
    int32_t code)
{
    for (const auto &[queryKey, syncId] : cloudSyncInfos) {
        UpdateFinishSyncInfo(queryKey, syncId, code);
    }
}

void SyncManager::BatchReport(int32_t userId, const TraceIds &traceIds, SyncStage syncStage, int32_t errCode,
    const std::string &message)
{
    for (const auto &[bundle, id] : traceIds) {
        SyncManager::Report({ userId, bundle, id, syncStage, errCode, message });
    }
}

std::string SyncManager::GetPath(const StoreMetaData &meta)
{
    StoreMetaMapping mapping(meta);
    MetaDataManager::GetInstance().LoadMeta(mapping.GetKey(), mapping, true);
    return mapping.cloudPath.empty() ? mapping.dataDir : mapping.cloudPath;
}

std::pair<bool, StoreMetaData> SyncManager::GetMetaData(const StoreInfo &storeInfo)
{
    StoreMetaData meta(storeInfo);
    meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    meta.dataDir = GetPath(meta);
    if (!meta.dataDir.empty() && MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true)) {
        return { true, meta };
    }
    meta.user = "0"; // check if it is a public store.
    meta.dataDir = GetPath(meta);
    if (meta.dataDir.empty() || !MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true)) {
        ZLOGE("failed, no store meta. bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
            meta.GetStoreAlias().c_str());
        return { false, meta };
    }
    StoreMetaDataLocal localMetaData;
    if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKeyLocal(), localMetaData, true) || !localMetaData.isPublic) {
        ZLOGE("failed, no store LocalMeta. bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
            meta.GetStoreAlias().c_str());
        return { false, meta };
    }
    return { true, meta };
}

void SyncManager::OnScreenUnlocked(int32_t user)
{
    std::vector<SyncInfo> infos;
    compensateSyncInfos_.ComputeIfPresent(user,
        [&infos](auto &userId, const std::map<std::string, std::set<std::string>> &apps) {
            for (const auto &[bundle, storeIds] : apps) {
                std::vector<std::string> stores(storeIds.begin(), storeIds.end());
                infos.push_back(SyncInfo(userId, bundle, stores));
            }
            return false;
        });
    compensateSyncInfos_.ComputeIfPresent(SYSTEM_USER_ID,
        [&infos](auto &userId, const std::map<std::string, std::set<std::string>> &apps) {
            for (const auto &[bundle, storeIds] : apps) {
                std::vector<std::string> stores(storeIds.begin(), storeIds.end());
                infos.push_back(SyncInfo(userId, bundle, stores));
            }
            return false;
        });
    for (auto &info : infos) {
        info.SetTriggerMode(MODE_UNLOCK);
        DoCloudSync(info);
    }
}

void SyncManager::CleanCompensateSync(int32_t userId)
{
    compensateSyncInfos_.Erase(userId);
}

void SyncManager::AddCompensateSync(const StoreMetaData &meta)
{
    compensateSyncInfos_.Compute(std::atoi(meta.user.c_str()),
        [&meta](auto &, std::map<std::string, std::set<std::string>> &apps) {
            apps[meta.bundleName].insert(meta.storeId);
            return true;
        });
}

std::pair<int32_t, CloudLastSyncInfo> SyncManager::GetLastSyncInfoFromMeta(const QueryKey &queryKey)
{
    CloudLastSyncInfo lastSyncInfo;
    if (!MetaDataManager::GetInstance().LoadMeta(CloudLastSyncInfo::GetKey(queryKey.user, queryKey.bundleName,
        queryKey.storeId), lastSyncInfo, true)) {
        ZLOGE("load last sync info fail, bundleName: %{public}s, user:%{public}d",
              queryKey.bundleName.c_str(), queryKey.user);
        return { E_ERROR, lastSyncInfo };
    }
    if (queryKey.accountId != lastSyncInfo.id || (!queryKey.storeId.empty() &&
        queryKey.storeId != lastSyncInfo.storeId)) {
        return { E_ERROR, lastSyncInfo };
    }
    return { SUCCESS, std::move(lastSyncInfo) };
}

void SyncManager::SaveLastSyncInfo(const QueryKey &queryKey, CloudLastSyncInfo &&info)
{
    if (!MetaDataManager::GetInstance().SaveMeta(CloudLastSyncInfo::GetKey(queryKey.user,
        queryKey.bundleName, queryKey.storeId), info, true)) {
        ZLOGE("save cloud last info fail, bundleName: %{public}s, user:%{public}d",
              queryKey.bundleName.c_str(), queryKey.user);
    }
}

GenDetails SyncManager::ConvertGenDetailsCode(const GenDetails &details)
{
    GenDetails newDetails;
    for (const auto &it : details) {
        GenProgressDetail detail = it.second;
        detail.code = ConvertValidGeneralCode(detail.code);
        newDetails.emplace(std::make_pair(it.first, std::move(detail)));
    }
    return newDetails;
}

int32_t SyncManager::ConvertValidGeneralCode(int32_t code)
{
    return (code >= E_OK && code < E_BUSY) ? code : E_ERROR;
}

void SyncManager::OnNetworkDisconnected()
{
    networkRecoveryManager_.OnNetworkDisconnected();
}

void SyncManager::OnNetworkConnected(const std::vector<int32_t> &users)
{
    syncTriggerMap_.ForEach([this](const std::string &key, StoreInfo &value) {
        auto &storeInfo = value;
        auto evt = std::make_unique<CloudEvent>(CloudEvent::SYNC_TRIGGER_REGISTER, storeInfo);
        EventCenter::GetInstance().PostEvent(std::move(evt));
        auto [hasMeta, meta] = GetMetaData(storeInfo);
        if (!hasMeta) {
            return false;
        }
        auto [code, store] = GetStore(meta, storeInfo.user);
        if (store == nullptr) {
            ZLOGE("store null, storeId:%{public}s", meta.GetStoreAlias().c_str());
            return false;
        }
        store->OnSyncTrigger(storeInfo.storeName, MODE_ONLINE);
        return false;
    });

    networkRecoveryManager_.OnNetworkConnected(users);
}

void SyncManager::NetworkRecoveryManager::OnNetworkDisconnected()
{
    ZLOGI("network disconnected.");
    std::lock_guard<std::mutex> lock(eventMutex_);
    currentEvent_ = std::make_unique<NetWorkEvent>();
    currentEvent_->disconnectTime = std::chrono::system_clock::now();
}

void SyncManager::NetworkRecoveryManager::OnNetworkConnected(const std::vector<int32_t> &users)
{
    std::unique_ptr<NetWorkEvent> event;
    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        if (currentEvent_ == nullptr) {
            ZLOGE("network connected, but currentEvent_ is not initialized.");
            return;
        }
        event = std::move(currentEvent_);
    }
    auto now = std::chrono::system_clock::now();
    auto duration = now - event->disconnectTime;
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
    bool timeout = (hours > NETWORK_DISCONNECT_TIMEOUT_HOURS);
    for (auto user : users) {
        const auto &syncApps = timeout ? GetAppList(user) : event->syncApps[user];
        for (const auto &bundleName : syncApps) {
            ZLOGI("sync start bundleName:%{public}s, user:%{public}d", bundleName.c_str(), user);
            syncManager_.DoCloudSync(SyncInfo(user, bundleName, "", {}, MODE_ONLINE));
        }
    }
    ZLOGI("network connected success, network disconnect duration :%{public}ld hours", hours);
}

void SyncManager::NetworkRecoveryManager::RecordSyncApps(const int32_t user, const std::string &bundleName)
{
    std::lock_guard<std::mutex> lock(eventMutex_);
    if (currentEvent_ != nullptr) {
        auto &syncApps = currentEvent_->syncApps[user];
        if (std::find(syncApps.begin(), syncApps.end(), bundleName) == syncApps.end()) {
            syncApps.push_back(bundleName);
            ZLOGI("record sync user:%{public}d, bundleName:%{public}s", user, bundleName.c_str());
        }
    }
}

std::vector<std::string> SyncManager::NetworkRecoveryManager::GetAppList(const int32_t user)
{
    CloudInfo cloud;
    cloud.user = user;
    if (!MetaDataManager::GetInstance().LoadMeta(cloud.GetKey(), cloud, true)) {
        ZLOGE("load cloud info fail, user:%{public}d", user);
        return {};
    }
    const size_t totalCount = cloud.apps.size();
    std::vector<std::string> appList;
    appList.reserve(totalCount);
    std::unordered_set<std::string> uniqueSet;
    uniqueSet.reserve(totalCount);
    auto addApp = [&](std::string bundleName) {
        if (uniqueSet.insert(bundleName).second) {
            appList.push_back(std::move(bundleName));
        }
    };
    auto stores = CheckerManager::GetInstance().GetDynamicStores();
    auto staticStores = CheckerManager::GetInstance().GetStaticStores();
    stores.insert(stores.end(), staticStores.begin(), staticStores.end());
    for (const auto &store : stores) {
        addApp(std::move(store.bundleName));
    }
    for (const auto &[_, app] : cloud.apps) {
        addApp(std::move(app.bundleName));
    }
    return appList;
}

int32_t SyncManager::SetCloudConflictHandler(const AutoCache::Store &store)
{
    if (store == nullptr) {
        ZLOGE("Store is null");
        return E_ERROR;
    }

    auto instance = CloudServer::GetInstance();
    if (instance == nullptr) {
        ZLOGE("CloudServer instance is null");
        return E_ERROR;
    }

    auto handler = instance->GetConflictHandler();
    if (handler == nullptr) {
        ZLOGE("Conflict handler is null");
        return E_ERROR;
    }

    return store->SetCloudConflictHandler(handler);
}

void SyncManager::HandleSyncError(const ErrorContext &context)
{
    UpdateFinishSyncInfo({ context.cloud.user, context.cloud.id, context.bundleName, context.dbName }, context.syncId,
        context.errorCode);
    SyncManager::Report(
        { context.cloud.user, context.bundleName, context.traceId, SyncStage::END, context.errorCode, context.reason });
}

bool SyncManager::GetDataBaseCloudEnable(int32_t user, const std::string &bundleName, const std::string &dbName)
{
    CloudDbSyncConfig syncConfig;
    if (!MetaDataManager::GetInstance().LoadMeta(syncConfig.GetKey(user), syncConfig, true)) {
        return true;
    }

    auto dbConfig = FindDbConfig(syncConfig, bundleName, dbName);
    return dbConfig.has_value() ? dbConfig->cloudSyncEnabled : true;
}

bool SyncManager::GetTableCloudEnable(int32_t user, const std::string &bundleName, const std::string &dbName,
    std::vector<std::string> &tables)
{
    CloudDbSyncConfig syncConfig;
    if (!MetaDataManager::GetInstance().LoadMeta(syncConfig.GetKey(user), syncConfig, true)) {
        return true;
    }

    auto dbConfig = FindDbConfig(syncConfig, bundleName, dbName);
    if (!dbConfig.has_value()) {
        return true;
    }
    auto isTableDisabled = [&dbConfig](const std::string &table) {
        const auto &tableConfigs = dbConfig.value().tableConfigs;
        auto tableIter = std::find_if(tableConfigs.begin(), tableConfigs.end(),
            [&table](const TableSyncConfig &config) { return config.tableName == table; });
        return (tableIter != tableConfigs.end()) ? !tableIter->cloudSyncEnabled : false;
    };
    tables.erase(std::remove_if(tables.begin(), tables.end(), isTableDisabled), tables.end());

    return !tables.empty();
}

std::optional<SyncManager::DbSyncConfig> SyncManager::FindDbConfig(const CloudDbSyncConfig &syncConfig,
    const std::string &bundleName, const std::string &dbName) const
{
    auto appIter = syncConfig.appConfigs.find(bundleName);
    if (appIter == syncConfig.appConfigs.end()) {
        return std::nullopt;
    }

    const auto &dbConfigs = appIter->second.dbConfigs;
    auto dbIter = std::find_if(dbConfigs.begin(), dbConfigs.end(),
        [&dbName](const DbSyncConfig &config) { return config.dbName == dbName; });

    return (dbIter != dbConfigs.end()) ? std::make_optional(*dbIter) : std::nullopt;
}
} // namespace OHOS::CloudData