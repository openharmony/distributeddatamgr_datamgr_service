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

#define LOG_TAG "CloudServiceImpl"

#include "cloud_service_impl.h"

#include <chrono>
#include <cinttypes>

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "checker/checker_manager.h"
#include "cloud/change_event.h"
#include "cloud/cloud_last_sync_info.h"
#include "cloud/cloud_mark.h"
#include "cloud/cloud_server.h"
#include "cloud/cloud_share_event.h"
#include "cloud/make_query_event.h"
#include "cloud/sharing_center.h"
#include "cloud_data_translate.h"
#include "cloud_value_util.h"
#include "device_manager_adapter.h"
#include "dfx/radar_reporter.h"
#include "dfx/reporter.h"
#include "eventcenter/event_center.h"
#include "get_schema_helper.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "network/network_delegate.h"
#include "rdb_types.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "store/auto_cache.h"
#include "sync_manager.h"
#include "sync_strategies/network_sync_strategy.h"
#include "utils/anonymous.h"
#include "utils/constant.h"
#include "values_bucket.h"
#include "xcollie.h"

namespace OHOS::CloudData {
using namespace DistributedData;
using namespace std::chrono;
using namespace SharingUtil;
using namespace Security::AccessToken;
using namespace DistributedDataDfx;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Account = AccountDelegate;
using AccessTokenKit = Security::AccessToken::AccessTokenKit;
static constexpr uint32_t RESTART_SERVICE_TIME_THRESHOLD = 120;
static constexpr const char *FT_ENABLE_CLOUD = "ENABLE_CLOUD";
static constexpr const char *FT_DISABLE_CLOUD = "DISABLE_CLOUD";
static constexpr const char *FT_SWITCH_ON = "SWITCH_ON";
static constexpr const char *FT_SWITCH_OFF = "SWITCH_OFF";
static constexpr const char *FT_QUERY_INFO = "QUERY_SYNC_INFO";
static constexpr const char *FT_USER_CHANGE = "USER_CHANGE";
static constexpr const char *FT_USER_UNLOCK = "USER_UNLOCK";
static constexpr const char *FT_NETWORK_RECOVERY = "NETWORK_RECOVERY";
static constexpr const char *FT_SERVICE_INIT = "SERVICE_INIT";
static constexpr const char *FT_SYNC_TASK = "SYNC_TASK";
static constexpr const char *CLOUD_SCHEMA = "arkdata/cloud/cloud_schema.json";
__attribute__((used)) CloudServiceImpl::Factory CloudServiceImpl::factory_;
const CloudServiceImpl::SaveStrategy CloudServiceImpl::STRATEGY_SAVERS[Strategy::STRATEGY_BUTT] = {
    &CloudServiceImpl::SaveNetworkStrategy
};

CloudServiceImpl::Factory::Factory() noexcept
{
    FeatureSystem::GetInstance().RegisterCreator(
        CloudServiceImpl::SERVICE_NAME,
        [this]() {
            if (product_ == nullptr) {
                product_ = std::make_shared<CloudServiceImpl>();
            }
            return product_;
        },
        FeatureSystem::BIND_NOW);
    staticActs_ = std::make_shared<CloudStatic>();
    FeatureSystem::GetInstance().RegisterStaticActs(CloudServiceImpl::SERVICE_NAME, staticActs_);
}

CloudServiceImpl::Factory::~Factory() {}

CloudServiceImpl::CloudServiceImpl()
{
    EventCenter::GetInstance().Subscribe(CloudEvent::GET_SCHEMA, [this](const Event &event) {
        GetSchema(event);
    });
    EventCenter::GetInstance().Subscribe(CloudEvent::CLOUD_SHARE, [this](const Event &event) {
        CloudShare(event);
    });
    EventCenter::GetInstance().Subscribe(CloudEvent::UPGRADE_SCHEMA, [this](const Event &event) {
        DoSync(event);
    });
    MetaDataManager::GetInstance().Subscribe(
        Subscription::GetPrefix({ "" }), [this](const std::string &key,
            const std::string &value, int32_t flag) -> auto {
            if (flag != MetaDataManager::INSERT && flag != MetaDataManager::UPDATE) {
                return true;
            }
            Subscription sub;
            Subscription::Unmarshall(value, sub);
            InitSubTask(sub, SUBSCRIPTION_INTERVAL);
            return true;
        }, true);
}

int32_t CloudServiceImpl::EnableCloud(const std::string &id, const std::map<std::string, int32_t> &switches)
{
    XCollie xcollie(
        __FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY, RESTART_SERVICE_TIME_THRESHOLD);
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto user = Account::GetInstance()->GetUserByToken(tokenId);
    auto [status, cloudInfo] = GetCloudInfo(user);
    if (status != SUCCESS) {
        Report(FT_ENABLE_CLOUD, Fault::CSF_CLOUD_INFO, "", "EnableCloud ret=" + std::to_string(status));
        return status;
    }
    cloudInfo.enableCloud = true;
    for (const auto &[bundle, value] : switches) {
        if (!cloudInfo.Exist(bundle)) {
            continue;
        }
        cloudInfo.apps[bundle].cloudSwitch = (value == SWITCH_ON);
        ZLOGI("EnableCloud: change app[%{public}s] switch to %{public}d", bundle.c_str(), value);
    }
    if (!MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        return ERROR;
    }
    Execute(GenTask(0, cloudInfo.user, CloudSyncScene::ENABLE_CLOUD,
        { WORK_CLOUD_INFO_UPDATE, WORK_SCHEMA_UPDATE, WORK_DO_CLOUD_SYNC, WORK_SUB }));
    ZLOGI("EnableCloud success, id:%{public}s, count:%{public}zu", Anonymous::Change(id).c_str(), switches.size());
    return SUCCESS;
}

void CloudServiceImpl::Report(
    const std::string &faultType, Fault errCode, const std::string &bundleName, const std::string &appendix)
{
    ArkDataFaultMsg msg = { .faultType = faultType,
        .bundleName = bundleName,
        .moduleName = ModuleName::CLOUD_SERVER,
        .errorType = static_cast<int32_t>(errCode) + SyncManager::GenStore::CLOUD_ERR_OFFSET,
        .appendixMsg = appendix };
    Reporter::GetInstance()->CloudSyncFault()->Report(msg);
}

int32_t CloudServiceImpl::DisableCloud(const std::string &id)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto user = Account::GetInstance()->GetUserByToken(tokenId);
    ReleaseUserInfo(user, CloudSyncScene::DISABLE_CLOUD);
    std::lock_guard<decltype(rwMetaMutex_)> lock(rwMetaMutex_);
    auto [status, cloudInfo] = GetCloudInfo(user);
    if (status != SUCCESS) {
        Report(FT_DISABLE_CLOUD, Fault::CSF_CLOUD_INFO, "", "DisableCloud ret=" + std::to_string(status));
        return status;
    }
    if (cloudInfo.id != id) {
        ZLOGE("invalid args, [input] id:%{public}s, [exist] id:%{public}s", Anonymous::Change(id).c_str(),
            Anonymous::Change(cloudInfo.id).c_str());
        return INVALID_ARGUMENT;
    }
    cloudInfo.enableCloud = false;
    if (!MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        return ERROR;
    }
    Execute(GenTask(0, cloudInfo.user, CloudSyncScene::DISABLE_CLOUD, { WORK_STOP_CLOUD_SYNC, WORK_SUB }));
    ZLOGI("DisableCloud success, id:%{public}s", Anonymous::Change(id).c_str());
    return SUCCESS;
}

int32_t CloudServiceImpl::ChangeAppSwitch(const std::string &id, const std::string &bundleName, int32_t appSwitch)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto user = Account::GetInstance()->GetUserByToken(tokenId);
    CloudSyncScene scene;
    if (appSwitch == SWITCH_ON) {
        scene = CloudSyncScene::SWITCH_ON;
    } else {
        scene = CloudSyncScene::SWITCH_OFF;
    }
    std::lock_guard<decltype(rwMetaMutex_)> lock(rwMetaMutex_);
    auto [status, cloudInfo] = GetCloudInfo(user);
    if (status != SUCCESS || !cloudInfo.enableCloud) {
        Report(appSwitch == SWITCH_ON ? FT_SWITCH_ON : FT_SWITCH_OFF, Fault::CSF_CLOUD_INFO, bundleName,
            "ChangeAppSwitch ret = " + std::to_string(status));
        return status;
    }
    if (cloudInfo.id != id) {
        ZLOGW("invalid args, [input] id:%{public}s, [exist] id:%{public}s, bundleName:%{public}s",
            Anonymous::Change(id).c_str(), Anonymous::Change(cloudInfo.id).c_str(), bundleName.c_str());
        Execute(GenTask(0, cloudInfo.user, scene, { WORK_CLOUD_INFO_UPDATE, WORK_SCHEMA_UPDATE, WORK_SUB,
            WORK_DO_CLOUD_SYNC }));
        return INVALID_ARGUMENT;
    }
    if (!cloudInfo.Exist(bundleName)) {
        std::tie(status, cloudInfo) = GetCloudInfoFromServer(user);
        if (status != SUCCESS || !cloudInfo.enableCloud || cloudInfo.id != id || !cloudInfo.Exist(bundleName)) {
            ZLOGE("invalid args, status:%{public}d, enableCloud:%{public}d, [input] id:%{public}s,"
                  "[exist] id:%{public}s, bundleName:%{public}s", status, cloudInfo.enableCloud,
                  Anonymous::Change(id).c_str(), Anonymous::Change(cloudInfo.id).c_str(), bundleName.c_str());
            Report(appSwitch == SWITCH_ON ? FT_SWITCH_ON : FT_SWITCH_OFF, Fault::CSF_CLOUD_INFO, bundleName,
                "ChangeAppSwitch ret=" + std::to_string(status));
            return INVALID_ARGUMENT;
        }
        ZLOGI("add app switch, bundleName:%{public}s", bundleName.c_str());
    }
    cloudInfo.apps[bundleName].cloudSwitch = (appSwitch == SWITCH_ON);
    if (!MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        return ERROR;
    }
    Execute(GenTask(0, cloudInfo.user, scene, { WORK_CLOUD_INFO_UPDATE, WORK_SCHEMA_UPDATE, WORK_SUB }));
    if (cloudInfo.enableCloud && appSwitch == SWITCH_ON) {
        SyncManager::SyncInfo info(cloudInfo.user, bundleName);
        syncManager_.DoCloudSync(info);
    }
    ZLOGI("ChangeAppSwitch success, id:%{public}s app:%{public}s, switch:%{public}d", Anonymous::Change(id).c_str(),
        bundleName.c_str(), appSwitch);
    return SUCCESS;
}

int32_t CloudServiceImpl::DoClean(const CloudInfo &cloudInfo, const std::map<std::string, int32_t> &actions)
{
    syncManager_.StopCloudSync(cloudInfo.user);
    for (const auto &[bundle, action] : actions) {
        if (!cloudInfo.Exist(bundle)) {
            ZLOGW("user:%{public}d, bundleName:%{public}s is not exist", cloudInfo.user, bundle.c_str());
            continue;
        }
        SchemaMeta schemaMeta;
        if (!MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(bundle), schemaMeta, true)) {
            ZLOGE("failed, no schema meta:bundleName:%{public}s", bundle.c_str());
            continue;
        }
        DoClean(cloudInfo.user, schemaMeta, action);
    }
    return SUCCESS;
}

void CloudServiceImpl::DoClean(int32_t user, const SchemaMeta &schemaMeta, int32_t action)
{
    StoreMetaData meta;
    meta.bundleName = schemaMeta.bundleName;
    meta.user = std::to_string(user);
    meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    meta.instanceId = 0;
    for (const auto &database : schemaMeta.databases) {
        // action
        meta.storeId = database.name;
        if (!GetStoreMetaData(meta)) {
            continue;
        }
        auto store = AutoCache::GetInstance().GetStore(meta, {});
        if (store == nullptr) {
            ZLOGE("store null, storeId:%{public}s", meta.GetStoreAlias().c_str());
            continue;
        }
        DistributedData::StoreInfo storeInfo;
        storeInfo.bundleName = meta.bundleName;
        storeInfo.user = atoi(meta.user.c_str());
        storeInfo.storeName = meta.storeId;
        storeInfo.path = meta.dataDir;
        if (action != GeneralStore::CLEAN_WATER) {
            EventCenter::GetInstance().PostEvent(std::make_unique<CloudEvent>(CloudEvent::CLEAN_DATA, storeInfo));
        }
        auto status = store->Clean({}, action, "");
        if (status != E_OK) {
            ZLOGW("remove device data status:%{public}d, user:%{public}d, bundleName:%{public}s, "
                  "storeId:%{public}s",
                status, static_cast<int>(user), meta.bundleName.c_str(), meta.GetStoreAlias().c_str());
        } else {
            ZLOGD("clean success, user:%{public}d, bundleName:%{public}s, storeId:%{public}s",
                static_cast<int>(user), meta.bundleName.c_str(), meta.GetStoreAlias().c_str());
        }
    }
}

bool CloudServiceImpl::GetStoreMetaData(StoreMetaData &meta)
{
    StoreMetaMapping metaMapping(meta);
    meta.dataDir = SyncManager::GetPath(meta);
    if (!meta.dataDir.empty() && MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true)) {
        return true;
    }
    meta.user = "0"; // check if it is a public store
    meta.dataDir = SyncManager::GetPath(meta);

    if (meta.dataDir.empty() || !MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true)) {
        ZLOGE("failed, no store meta. bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
            meta.GetStoreAlias().c_str());
        return false;
    }
    StoreMetaDataLocal localMetaData;
    if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKeyLocal(), localMetaData, true) || !localMetaData.isPublic) {
        ZLOGE("failed, no store LocalMeta. bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
            meta.GetStoreAlias().c_str());
        return false;
    }
    return true;
}

int32_t CloudServiceImpl::Clean(const std::string &id, const std::map<std::string, int32_t> &actions)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto user = Account::GetInstance()->GetUserByToken(tokenId);
    auto [status, cloudInfo] = GetCloudInfoFromMeta(user);
    if (status != SUCCESS) {
        ZLOGE("get cloud meta failed user:%{public}d", static_cast<int>(cloudInfo.user));
        return ERROR;
    }
    if (id != cloudInfo.id) {
        ZLOGE("different id, [server] id:%{public}s, [meta] id:%{public}s", Anonymous::Change(cloudInfo.id).c_str(),
            Anonymous::Change(id).c_str());
        return ERROR;
    }
    auto dbActions = ConvertAction(actions);
    if (dbActions.empty()) {
        ZLOGE("invalid actions. id:%{public}s", Anonymous::Change(cloudInfo.id).c_str());
        return ERROR;
    }
    return DoClean(cloudInfo, dbActions);
}

int32_t CloudServiceImpl::CheckNotifyConditions(const std::string &id, const std::string &bundleName,
    CloudInfo &cloudInfo)
{
    if (cloudInfo.id != id) {
        ZLOGE("invalid args, [input] id:%{public}s, [meta] id:%{public}s", Anonymous::Change(id).c_str(),
            Anonymous::Change(cloudInfo.id).c_str());
        return INVALID_ARGUMENT;
    }
    if (!cloudInfo.enableCloud) {
        ZLOGD("cloud is disabled, id:%{public}s, app:%{public}s ", Anonymous::Change(id).c_str(), bundleName.c_str());
        return CLOUD_DISABLE;
    }
    if (!cloudInfo.Exist(bundleName)) {
        ZLOGE("bundleName:%{public}s is not exist", bundleName.c_str());
        return INVALID_ARGUMENT;
    }
    if (!cloudInfo.apps[bundleName].cloudSwitch) {
        ZLOGD("cloud switch is disabled, id:%{public}s, app:%{public}s ", Anonymous::Change(id).c_str(),
            bundleName.c_str());
        return CLOUD_DISABLE_SWITCH;
    }
    return SUCCESS;
}

std::map<std::string, std::vector<std::string>> CloudServiceImpl::GetDbInfoFromExtraData(
    const ExtraData &extraData, const SchemaMeta &schemaMeta)
{
    std::map<std::string, std::vector<std::string>> dbInfos;
    for (auto &db : schemaMeta.databases) {
        if (db.alias != extraData.info.containerName) {
            continue;
        }
        std::vector<std::string> tables;
        if (extraData.info.tables.size() == 0) {
            dbInfos.emplace(db.name, std::move(tables));
            continue;
        }
        for (auto &table : db.tables) {
            const auto &tbs = extraData.info.tables;
            if (std::find(tbs.begin(), tbs.end(), table.alias) == tbs.end()) {
                continue;
            }
            if (extraData.isPrivate()) {
                ZLOGD("private table change, name:%{public}s", Anonymous::Change(table.name).c_str());
                tables.emplace_back(table.name);
            }
            if (extraData.isShared() && !table.sharedTableName.empty()) {
                ZLOGD("sharing table change, name:%{public}s", Anonymous::Change(table.sharedTableName).c_str());
                tables.emplace_back(table.sharedTableName);
            }
        }
        if (!tables.empty()) {
            dbInfos.emplace(db.name, std::move(tables));
        }
    }
    if (dbInfos.empty()) {
        for (auto &db : schemaMeta.databases) {
            if (db.alias != extraData.info.containerName) {
                continue;
            }
            std::vector<std::string> tables;
            dbInfos.emplace(db.name, std::move(tables));
        }
    }
    return dbInfos;
}

bool CloudServiceImpl::DoKvCloudSync(int32_t userId, const std::string &bundleName, int32_t triggerMode)
{
    auto stores = CheckerManager::GetInstance().GetDynamicStores();
    auto staticStores = CheckerManager::GetInstance().GetStaticStores();
    stores.insert(stores.end(), staticStores.begin(), staticStores.end());
    bool found = std::any_of(stores.begin(), stores.end(), [&bundleName](const CheckerManager::StoreInfo &storeInfo) {
        return bundleName.empty() || storeInfo.bundleName == bundleName;
    });
    if (!found) {
        return found;
    }
    std::vector<int32_t> users;
    if (userId != INVALID_USER_ID) {
        users.emplace_back(userId);
    } else {
        Account::GetInstance()->QueryForegroundUsers(users);
    }
    for (auto user : users) {
        for (const auto &store : stores) {
            int32_t mode = (store.bundleName != bundleName && triggerMode == MODE_PUSH) ? MODE_CONSISTENCY
                                                                                        : triggerMode;
            syncManager_.DoCloudSync(SyncManager::SyncInfo(user, store.bundleName, store.storeId, {}, mode));
        }
    }
    return found;
}

int32_t CloudServiceImpl::NotifyDataChange(const std::string &id, const std::string &bundleName)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto user = Account::GetInstance()->GetUserByToken(tokenId);
    auto [status, cloudInfo] = GetCloudInfoFromMeta(user);
    if (CheckNotifyConditions(id, bundleName, cloudInfo) != E_OK) {
        return INVALID_ARGUMENT;
    }
    syncManager_.DoCloudSync(SyncManager::SyncInfo(cloudInfo.user, bundleName));
    return SUCCESS;
}

int32_t CloudServiceImpl::NotifyDataChange(const std::string &eventId, const std::string &extraData, int32_t userId)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    ExtraData exData;
    if (eventId != DATA_CHANGE_EVENT_ID || extraData.empty() || !exData.Unmarshall(extraData)) {
        ZLOGE("invalid args, eventId:%{public}s, user:%{public}d, extraData is %{public}s", eventId.c_str(),
            userId, extraData.empty() ? "empty" : "not empty");
        return INVALID_ARGUMENT;
    }
    std::vector<int32_t> users;
    if (userId != INVALID_USER_ID) {
        users.emplace_back(userId);
    } else {
        Account::GetInstance()->QueryForegroundUsers(users);
    }
    for (auto user : users) {
        if (user == DEFAULT_USER) {
            continue;
        }
        auto &bundleName = exData.info.bundleName;
        auto &prepareTraceId = exData.info.context.prepareTraceId;
        auto [status, cloudInfo] = GetCloudInfoFromMeta(user);
        if (CheckNotifyConditions(exData.info.accountId, bundleName, cloudInfo) != E_OK) {
            ZLOGD("invalid user:%{public}d, traceId:%{public}s", user, prepareTraceId.c_str());
            syncManager_.Report({ user, bundleName, prepareTraceId, SyncStage::END, INVALID_ARGUMENT });
            return INVALID_ARGUMENT;
        }
        auto schemaKey = CloudInfo::GetSchemaKey(user, bundleName);
        SchemaMeta schemaMeta;
        if (!MetaDataManager::GetInstance().LoadMeta(schemaKey, schemaMeta, true)) {
            ZLOGE("no exist meta, user:%{public}d, traceId:%{public}s", user, prepareTraceId.c_str());
            syncManager_.Report({ user, bundleName, prepareTraceId, SyncStage::END, INVALID_ARGUMENT });
            return INVALID_ARGUMENT;
        }
        auto dbInfos = GetDbInfoFromExtraData(exData, schemaMeta);
        if (dbInfos.empty()) {
            ZLOGE("GetDbInfoFromExtraData failed, empty database info. traceId:%{public}s.", prepareTraceId.c_str());
            syncManager_.Report({ user, bundleName, prepareTraceId, SyncStage::END, INVALID_ARGUMENT });
            return INVALID_ARGUMENT;
        }
        for (const auto &dbInfo : dbInfos) {
            syncManager_.DoCloudSync(SyncManager::SyncInfo(
                { cloudInfo.user, bundleName, dbInfo.first, dbInfo.second, MODE_PUSH, prepareTraceId }));
        }
    }
    return SUCCESS;
}

std::map<std::string, StatisticInfos> CloudServiceImpl::ExecuteStatistics(const std::string &storeId,
    const CloudInfo &cloudInfo, const SchemaMeta &schemaMeta)
{
    std::map<std::string, StatisticInfos> result;
    for (auto &database : schemaMeta.databases) {
        if (!storeId.empty() && database.alias != storeId) {
            continue;
        }
        auto it = cloudInfo.apps.find(schemaMeta.bundleName);
        if (it == cloudInfo.apps.end()) {
            ZLOGE("bundleName:%{public}s is not exist", schemaMeta.bundleName.c_str());
            break;
        }
        StoreMetaMapping meta;
        meta.bundleName = schemaMeta.bundleName;
        meta.storeId = database.name;
        meta.user = std::to_string(cloudInfo.user);
        meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
        meta.instanceId = it->second.instanceId;
        MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true);
        if (!meta.cloudPath.empty() && meta.cloudPath != meta.dataDir) {
            MetaDataManager::GetInstance().LoadMeta(meta.GetCloudStoreMetaKey(), meta, true);
        }
        result.insert_or_assign(database.alias, QueryStatistics(meta, database));
    }
    return result;
}

StatisticInfos CloudServiceImpl::QueryStatistics(const StoreMetaData &storeMetaData,
    const DistributedData::Database &database)
{
    std::vector<StatisticInfo> infos;
    auto store = AutoCache::GetInstance().GetStore(storeMetaData, {});
    if (store == nullptr) {
        ZLOGE("store failed, store is nullptr,bundleName:%{public}s",
            Anonymous::Change(storeMetaData.bundleName).c_str());
        return infos;
    }
    infos.reserve(database.tables.size());
    for (const auto &table : database.tables) {
        auto [success, info] = QueryTableStatistic(table.name, store);
        if (success) {
            info.table = table.alias;
            infos.push_back(std::move(info));
        }
    }
    return infos;
}

std::pair<bool, StatisticInfo> CloudServiceImpl::QueryTableStatistic(const std::string &tableName,
    AutoCache::Store store)
{
    StatisticInfo info;
    auto sql = BuildStatisticSql(tableName);
    auto [errCode, cursor] = store->Query(tableName, sql, {});
    if (errCode != E_OK || cursor == nullptr || cursor->GetCount() != 1 || cursor->MoveToFirst() != E_OK) {
        ZLOGE("query failed, cursor is nullptr or move to first failed,tableName:%{public}s",
            Anonymous::Change(tableName).c_str());
        return { false, info };
    }
    DistributedData::VBucket entry;
    if (cursor->GetEntry(entry) != E_OK) {
        ZLOGE("get entry failed,tableName:%{public}s", Anonymous::Change(tableName).c_str());
        return { false, info };
    }
    auto it = entry.find("inserted");
    if (it != entry.end() && it->second.index() == TYPE_INDEX<int64_t>) {
        info.inserted = std::get<int64_t>(it->second);
    }
    it = entry.find("updated");
    if (it != entry.end() && it->second.index() == TYPE_INDEX<int64_t>) {
        info.updated = std::get<int64_t>(it->second);
    }
    it = entry.find("normal");
    if (it != entry.end() && it->second.index() == TYPE_INDEX<int64_t>) {
        info.normal = std::get<int64_t>(it->second);
    }
    return { true, info };
}

std::string CloudServiceImpl::BuildStatisticSql(const std::string &tableName)
{
    std::string logTable = DistributedDB::RelationalStoreManager::GetDistributedLogTableName(tableName);
    std::string sql = "select ";
    sql.append("count(case when cloud_gid = '' and flag&(0x1|0x8|0x20) = 0x20 then 1 end) as inserted,");
    sql.append("count(case when cloud_gid <> '' and flag&0x20 != 0  then 1 end) as updated,");
    sql.append("count(case when cloud_gid <> '' and flag&(0x1|0x8|0x20) = 0 then 1 end) as normal");
    sql.append(" from ").append(logTable);

    return sql;
}

std::pair<int32_t, std::map<std::string, StatisticInfos>> CloudServiceImpl::QueryStatistics(const std::string &id,
    const std::string &bundleName, const std::string &storeId)
{
    std::map<std::string, StatisticInfos> result;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto user = Account::GetInstance()->GetUserByToken(tokenId);
    auto [status, cloudInfo] = GetCloudInfoFromMeta(user);
    if (status != SUCCESS) {
        ZLOGE("get cloud meta failed user:%{public}d", static_cast<int>(cloudInfo.user));
        return { ERROR, result };
    }
    if (id != cloudInfo.id || bundleName.empty() || cloudInfo.apps.find(bundleName) == cloudInfo.apps.end()) {
        ZLOGE("[server] id:%{public}s, [meta] id:%{public}s, bundleName:%{public}s",
            Anonymous::Change(cloudInfo.id).c_str(), Anonymous::Change(id).c_str(), bundleName.c_str());
        return { ERROR, result };
    }
    SchemaMeta schemaMeta;
    std::string schemaKey = cloudInfo.GetSchemaKey(bundleName);
    if (!MetaDataManager::GetInstance().LoadMeta(schemaKey, schemaMeta, true)) {
        ZLOGE("get load meta failed user:%{public}d", static_cast<int>(cloudInfo.user));
        return { ERROR, result };
    }
    result = ExecuteStatistics(storeId, cloudInfo, schemaMeta);
    return { SUCCESS, result };
}

int32_t CloudServiceImpl::SetGlobalCloudStrategy(Strategy strategy, const std::vector<CommonType::Value> &values)
{
    if (strategy >= Strategy::STRATEGY_BUTT) {
        ZLOGE("invalid strategy:%{public}d, size:%{public}zu", strategy, values.size());
        return INVALID_ARGUMENT;
    }
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    HapInfo hapInfo;
    hapInfo.user = Account::GetInstance()->GetUserByToken(tokenId);
    if (hapInfo.user == INVALID_USER_ID || hapInfo.user == 0) {
        ZLOGE("invalid user:%{public}d, strategy:%{public}d, size:%{public}zu", hapInfo.user, strategy,
            values.size());
        return ERROR;
    }
    hapInfo.bundleName = SyncStrategy::GLOBAL_BUNDLE;
    return STRATEGY_SAVERS[strategy](values, hapInfo);
}

std::pair<int32_t, QueryLastResults> CloudServiceImpl::QueryLastSyncInfo(const std::string &id,
    const std::string &bundleName, const std::string &storeId)
{
    QueryLastResults results;
    auto user = Account::GetInstance()->GetUserByToken(IPCSkeleton::GetCallingTokenID());
    auto [status, cloudInfo] = GetCloudInfo(user);
    if (status != SUCCESS) {
        Report(FT_QUERY_INFO, Fault::CSF_CLOUD_INFO, bundleName,
            "QueryLastSyncInfo ret=" + std::to_string(status) + ",storeId=" + storeId);
        return { ERROR, results };
    }
    if (cloudInfo.apps.find(bundleName) == cloudInfo.apps.end()) {
        ZLOGE("Invalid bundleName: %{public}s", bundleName.c_str());
        return { INVALID_ARGUMENT, results };
    }
    std::vector<SchemaMeta> schemas;
    auto key = cloudInfo.GetSchemaPrefix(bundleName);
    if (!MetaDataManager::GetInstance().LoadMeta(key, schemas, true) || schemas.empty()) {
        return { ERROR, results };
    }

    std::vector<QueryKey> queryKeys;
    std::vector<Database> databases;
    for (const auto &schema : schemas) {
        if (schema.bundleName != bundleName) {
            continue;
        }
        databases = schema.databases;
        for (const auto &database : schema.databases) {
            if (storeId.empty() || database.alias == storeId) {
                queryKeys.push_back({ user, id, bundleName, database.name });
            }
        }
        if (queryKeys.empty()) {
            ZLOGE("Invalid storeId: %{public}s", Anonymous::Change(storeId).c_str());
            return { INVALID_ARGUMENT, results };
        }
    }

    auto [ret, lastSyncInfos] = syncManager_.QueryLastSyncInfo(queryKeys);
    ZLOGI("code:%{public}d, id:%{public}s, bundleName:%{public}s, storeId:%{public}s, size:%{public}d", ret,
        Anonymous::Change(id).c_str(), bundleName.c_str(), Anonymous::Change(storeId).c_str(),
        static_cast<int32_t>(lastSyncInfos.size()));
    if (lastSyncInfos.empty()) {
        return { ret, results };
    }
    return { ret, AssembleLastResults(databases, lastSyncInfos) };
}

int32_t CloudServiceImpl::OnInitialize()
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    DistributedDB::RuntimeConfig::SetCloudTranslate(std::make_shared<RdbCloudDataTranslate>());
    std::vector<int> users;
    Account::GetInstance()->QueryUsers(users);
    for (auto user : users) {
        if (user == DEFAULT_USER) {
            continue;
        }
        CleanWaterVersion(user);
    }
    Execute(GenTask(0, 0, CloudSyncScene::SERVICE_INIT,
        { WORK_CLOUD_INFO_UPDATE, WORK_SCHEMA_UPDATE, WORK_DO_CLOUD_SYNC, WORK_SUB }));
    for (auto user : users) {
        if (user == DEFAULT_USER) {
            continue;
        }
        Subscription sub;
        sub.userId = user;
        if (!MetaDataManager::GetInstance().LoadMeta(sub.GetKey(), sub, true)) {
            continue;
        }
        InitSubTask(sub);
    }
    return E_OK;
}

bool CloudServiceImpl::CleanWaterVersion(int32_t user)
{
    auto [status, cloudInfo] = GetCloudInfoFromMeta(user);
    if (status != SUCCESS) {
        return false;
    }
    auto stores = CheckerManager::GetInstance().GetDynamicStores();
    auto staticStores = CheckerManager::GetInstance().GetStaticStores();
    stores.insert(stores.end(), staticStores.begin(), staticStores.end());
    auto keys = cloudInfo.GetSchemaKey();
    for (const auto &[bundle, key] : keys) {
        SchemaMeta schemaMeta;
        if (MetaDataManager::GetInstance().LoadMeta(key, schemaMeta, true) &&
            schemaMeta.metaVersion < SchemaMeta::CLEAN_WATER_VERSION) {
            bool found = std::any_of(stores.begin(), stores.end(),
                [&schemaMeta](const CheckerManager::StoreInfo &storeInfo) {
                    return storeInfo.bundleName == schemaMeta.bundleName;
                });
            if (!found) {
                continue;
            }
            DoClean(user, schemaMeta, GeneralStore::CleanMode::CLEAN_WATER);
            schemaMeta.metaVersion = SchemaMeta::CLEAN_WATER_VERSION;
        }
        MetaDataManager::GetInstance().SaveMeta(key, schemaMeta, true);
    }
    return true;
}

int32_t CloudServiceImpl::OnBind(const BindInfo &info)
{
    if (executor_ != nullptr || info.executors == nullptr) {
        return E_INVALID_ARGS;
    }

    executor_ = std::move(info.executors);
    syncManager_.Bind(executor_);
    auto instance = CloudServer::GetInstance();
    if (instance != nullptr) {
        instance->Bind(executor_);
    }
    return E_OK;
}

int32_t CloudServiceImpl::OnUserChange(uint32_t code, const std::string &user, const std::string &account)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    int32_t userId = atoi(user.c_str());
    ZLOGI("code:%{public}d, user:%{public}s, account:%{public}s", code, user.c_str(),
        Anonymous::Change(account).c_str());
    switch (code) {
        case static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_SWITCHED):
            Execute(GenTask(0, userId, CloudSyncScene::USER_CHANGE,
                { WORK_CLOUD_INFO_UPDATE, WORK_SCHEMA_UPDATE, WORK_DO_CLOUD_SYNC, WORK_SUB }));
            break;
        case static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_DELETE):
        case static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_STOPPING):
        case static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_STOPPED):
            Execute(GenTask(0, userId, CloudSyncScene::ACCOUNT_STOP, { WORK_STOP_CLOUD_SYNC, WORK_RELEASE }));
            break;
        case static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_UNLOCKED):
            Execute(GenTask(0, userId, CloudSyncScene::USER_UNLOCK,
                { WORK_CLOUD_INFO_UPDATE, WORK_SCHEMA_UPDATE, WORK_DO_CLOUD_SYNC, WORK_SUB }));
            break;
        default:
            break;
    }
    return E_OK;
}

int32_t CloudServiceImpl::OnReady(const std::string &device)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    if (device != DeviceManagerAdapter::CLOUD_DEVICE_UUID) {
        return SUCCESS;
    }
    std::vector<int32_t> users;
    Account::GetInstance()->QueryForegroundUsers(users);
    if (users.empty()) {
        return SUCCESS;
    }
    if (!NetworkDelegate::GetInstance()->IsNetworkAvailable()) {
        return NETWORK_ERROR;
    }
    for (auto user : users) {
        DoKvCloudSync(user, "", MODE_ONLINE);
        Execute(GenTask(0, user, CloudSyncScene::NETWORK_RECOVERY,
            { WORK_CLOUD_INFO_UPDATE, WORK_SCHEMA_UPDATE, WORK_DO_CLOUD_SYNC, WORK_SUB }));
    }
    return SUCCESS;
}

int32_t CloudServiceImpl::Offline(const std::string &device)
{
    if (device != DeviceManagerAdapter::CLOUD_DEVICE_UUID) {
        ZLOGI("Not network offline");
        return SUCCESS;
    }
    std::vector<int32_t> users;
    Account::GetInstance()->QueryUsers(users);
    if (users.empty()) {
        return SUCCESS;
    }
    auto it = users.begin();
    syncManager_.StopCloudSync(*it);
    return SUCCESS;
}

std::pair<int32_t, CloudInfo> CloudServiceImpl::GetCloudInfoFromMeta(int32_t userId)
{
    CloudInfo cloudInfo;
    cloudInfo.user = userId;
    if (!MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        auto time = static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
        ZLOGE("no exist meta, user:%{public}d times:%{public}" PRIu64 ".", cloudInfo.user, time);
        return { ERROR, cloudInfo };
    }
    return { SUCCESS, cloudInfo };
}

std::pair<int32_t, CloudInfo> CloudServiceImpl::GetCloudInfoFromServer(int32_t userId)
{
    CloudInfo cloudInfo;
    cloudInfo.user = userId;
    if (!NetworkDelegate::GetInstance()->IsNetworkAvailable()) {
        ZLOGD("network is not available");
        return { NETWORK_ERROR, cloudInfo };
    }
    auto instance = CloudServer::GetInstance();
    if (instance == nullptr) {
        ZLOGD("cloud server is nullptr, user:%{public}d", userId);
        return { SERVER_UNAVAILABLE, cloudInfo };
    }
    auto [code, info] = instance->GetServerInfo(cloudInfo.user, false);
    return { code != E_OK ? code : info.IsValid() ? E_OK : E_ERROR, info };
}

int32_t CloudServiceImpl::UpdateCloudInfoFromServer(int32_t user)
{
    auto [status, cloudInfo] = GetCloudInfoFromServer(user);
    if (status != SUCCESS || !cloudInfo.IsValid()) {
        ZLOGE("userId:%{public}d, status:%{public}d", user, status);
        return E_ERROR;
    }
    return MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true) ? E_OK : E_ERROR;
}

bool CloudServiceImpl::UpdateCloudInfo(int32_t user, CloudSyncScene scene)
{
    auto [status, cloudInfo] = GetCloudInfoFromServer(user);
    if (status != SUCCESS) {
        ZLOGE("user:%{public}d, status:%{public}d", user, status);
        Report(GetDfxFaultType(scene), Fault::CSF_CLOUD_INFO, "", "UpdateCloudInfo ret=" + std::to_string(status));
        return false;
    }
    ZLOGI("[server] id:%{public}s, enableCloud:%{public}d, user:%{public}d, app size:%{public}zu",
          Anonymous::Change(cloudInfo.id).c_str(), cloudInfo.enableCloud, cloudInfo.user, cloudInfo.apps.size());
    CloudInfo oldInfo;
    if (!MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetKey(), oldInfo, true)) {
        MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true);
        return true;
    }
    MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true);
    if (oldInfo.id != cloudInfo.id) {
        ReleaseUserInfo(user, scene);
        ZLOGE("different id, [server] id:%{public}s, [meta] id:%{public}s", Anonymous::Change(cloudInfo.id).c_str(),
            Anonymous::Change(oldInfo.id).c_str());
        MetaDataManager::GetInstance().DelMeta(Subscription::GetKey(user), true);
        std::map<std::string, int32_t> actions;
        for (auto &[bundle, app] : cloudInfo.apps) {
            MetaDataManager::GetInstance().DelMeta(Subscription::GetRelationKey(user, bundle), true);
            actions[bundle] = GeneralStore::CleanMode::CLOUD_INFO;
        }
        DoClean(oldInfo, actions);
    }
    return true;
}

bool CloudServiceImpl::UpdateSchema(int32_t user, CloudSyncScene scene)
{
    auto [status, cloudInfo] = GetCloudInfo(user);
    if (status != SUCCESS) {
        Report(GetDfxFaultType(scene), Fault::CSF_APP_SCHEMA, "", "UpdateSchema ret=" + std::to_string(status));
        return false;
    }
    auto keys = cloudInfo.GetSchemaKey();
    for (const auto &[bundle, key] : keys) {
        HapInfo hapInfo{ .user = user, .instIndex = 0, .bundleName = bundle };
        auto appInfoOpt = cloudInfo.GetAppInfo(bundle);
        if (appInfoOpt.has_value()) {
            const CloudInfo::AppInfo &appInfo = appInfoOpt.value();
            hapInfo.instIndex = appInfo.instanceId;
        }

        SchemaMeta schemaMeta;
        std::tie(status, schemaMeta) = GetSchemaFromHap(hapInfo);
        if (status != SUCCESS) {
            std::tie(status, schemaMeta) = GetAppSchemaFromServer(user, bundle);
        }
        if (status == NOT_SUPPORT) {
            ZLOGW("app not support, del cloudInfo! user:%{public}d, bundleName:%{public}s", user, bundle.c_str());
            MetaDataManager::GetInstance().DelMeta(cloudInfo.GetKey(), true);
            return false;
        }
        if (status != SUCCESS) {
            continue;
        }
        SchemaMeta oldMeta;
        if (MetaDataManager::GetInstance().LoadMeta(key, oldMeta, true)) {
            UpgradeSchemaMeta(user, oldMeta);
            UpdateClearWaterMark(hapInfo, schemaMeta, oldMeta);
        }
        MetaDataManager::GetInstance().SaveMeta(key, schemaMeta, true);
    }
    return true;
}

/**
* Will be called when 'cloudDriver' OnAppUpdate/OnAppInstall to get newest 'e2eeEnable'.
*/
int32_t CloudServiceImpl::UpdateSchemaFromServer(int32_t user)
{
    auto [status, cloudInfo] = GetCloudInfo(user);
    if (status != SUCCESS) {
        ZLOGW("get cloudinfo failed, user:%{public}d, status:%{public}d", user, status);
        return status;
    }
    auto keys = cloudInfo.GetSchemaKey();
    for (const auto &[bundle, key] : keys) {
        HapInfo hapInfo{ .user = user, .instIndex = 0, .bundleName = bundle };
        auto appInfoOpt = cloudInfo.GetAppInfo(bundle);
        if (appInfoOpt.has_value()) {
            const CloudInfo::AppInfo &appInfo = appInfoOpt.value();
            hapInfo.instIndex = appInfo.instanceId;
        }

        SchemaMeta schemaMeta;
        std::tie(status, schemaMeta) = GetAppSchemaFromServer(user, bundle);
        if (status == NOT_SUPPORT) {
            ZLOGW("app not support, del cloudInfo! user:%{public}d, bundleName:%{public}s", user, bundle.c_str());
            MetaDataManager::GetInstance().DelMeta(cloudInfo.GetKey(), true);
            return status;
        }
        if (status != SUCCESS) {
            continue;
        }
        SchemaMeta oldMeta;
        if (MetaDataManager::GetInstance().LoadMeta(key, oldMeta, true) &&
            oldMeta.e2eeEnable != schemaMeta.e2eeEnable) {
            ZLOGI("Update e2eeEnable: %{public}d->%{public}d, user:%{public}d, bundleName:%{public}s",
                oldMeta.e2eeEnable, schemaMeta.e2eeEnable, user, bundle.c_str());
            oldMeta.e2eeEnable = schemaMeta.e2eeEnable;
            MetaDataManager::GetInstance().SaveMeta(key, oldMeta, true);
        }
    }
    return SUCCESS;
}

std::pair<int32_t, SchemaMeta> CloudServiceImpl::GetAppSchemaFromServer(int32_t user, const std::string &bundleName)
{
    SchemaMeta schemaMeta;
    if (!NetworkDelegate::GetInstance()->IsNetworkAvailable()) {
        ZLOGD("network is not available");
        return { NETWORK_ERROR, schemaMeta };
    }
    auto instance = CloudServer::GetInstance();
    if (instance == nullptr) {
        return { SERVER_UNAVAILABLE, schemaMeta };
    }
    int32_t status = E_OK;
    std::tie(status, schemaMeta) = instance->GetAppSchema(user, bundleName);
    if (status != E_OK || !schemaMeta.IsValid()) {
        ZLOGE("schema is InValid, user:%{public}d, bundleName:%{public}s, status:%{public}d", user, bundleName.c_str(),
            status);
        return { status == E_NOT_SUPPORT ? NOT_SUPPORT : SCHEMA_INVALID, schemaMeta };
    }
    return { SUCCESS, schemaMeta };
}

void CloudServiceImpl::UpgradeSchemaMeta(int32_t user, const SchemaMeta &schemaMeta)
{
    if (schemaMeta.metaVersion == SchemaMeta::CURRENT_VERSION) {
        return;
    }
    // A major schema upgrade requires flag cleaning
    if (SchemaMeta::GetHighVersion(schemaMeta.metaVersion) != SchemaMeta::GetHighVersion()) {
        ZLOGI("start clean. user:%{public}d, bundleName:%{public}s, metaVersion:%{public}d", user,
            schemaMeta.bundleName.c_str(), schemaMeta.metaVersion);
        DoClean(user, schemaMeta, GeneralStore::CleanMode::CLOUD_INFO);
    }
}

ExecutorPool::Task CloudServiceImpl::GenTask(int32_t retry, int32_t user, CloudSyncScene scene, Handles handles)
{
    return [this, retry, user, scene, works = std::move(handles)]() mutable {
        auto executor = executor_;
        if (retry >= RETRY_TIMES || executor == nullptr || works.empty()) {
            return;
        }
        if (!NetworkDelegate::GetInstance()->IsNetworkAvailable()) {
            ZLOGD("network is not available");
            return;
        }
        bool finished = true;
        std::vector<int32_t> users;
        if (user == 0) {
            auto account = Account::GetInstance();
            finished = (account != nullptr) && account->QueryUsers(users);
        } else {
            users.push_back(user);
        }

        auto handle = works.front();
        for (auto user : users) {
            if (user == 0 || !Account::GetInstance()->IsVerified(user)) {
                continue;
            }
            finished = (this->*handle)(user, scene) && finished;
        }
        if (!finished || users.empty()) {
            executor->Schedule(std::chrono::seconds(RETRY_INTERVAL), GenTask(retry + 1, user, scene, std::move(works)));
            return;
        }
        works.pop_front();
        if (!works.empty()) {
            executor->Execute(GenTask(retry, user, scene, std::move(works)));
        }
    };
}

std::pair<int32_t, SchemaMeta> CloudServiceImpl::GetSchemaMeta(int32_t userId, const std::string &bundleName,
    int32_t instanceId)
{
    SchemaMeta schemaMeta;
    auto [status, cloudInfo] = GetCloudInfoFromMeta(userId);
    if (status != SUCCESS) {
        // GetCloudInfo has print the log info. so we don`t need print again.
        return { status, schemaMeta };
    }
    if (!bundleName.empty() && !cloudInfo.Exist(bundleName, instanceId)) {
        ZLOGD("bundleName:%{public}s instanceId:%{public}d is not exist", bundleName.c_str(), instanceId);
        return { ERROR, schemaMeta };
    }
    std::string schemaKey = cloudInfo.GetSchemaKey(bundleName, instanceId);
    if (MetaDataManager::GetInstance().LoadMeta(schemaKey, schemaMeta, true) &&
        schemaMeta.metaVersion == SchemaMeta::CURRENT_VERSION) {
        return { SUCCESS, schemaMeta };
    }
    UpgradeSchemaMeta(userId, schemaMeta);
    HapInfo hapInfo{ .user = userId, .instIndex = instanceId, .bundleName = bundleName };
    std::tie(status, schemaMeta) = GetSchemaFromHap(hapInfo);
    if (status == SUCCESS) {
        MetaDataManager::GetInstance().SaveMeta(schemaKey, schemaMeta, true);
        return { status, schemaMeta };
    }
    if (!Account::GetInstance()->IsVerified(userId)) {
        ZLOGE("user:%{public}d is locked!", userId);
        return { ERROR, schemaMeta };
    }
    std::tie(status, schemaMeta) = GetAppSchemaFromServer(userId, bundleName);
    if (status == NOT_SUPPORT) {
        ZLOGW("app not support, del cloudInfo! userId:%{public}d, bundleName:%{public}s", userId, bundleName.c_str());
        MetaDataManager::GetInstance().DelMeta(cloudInfo.GetKey(), true);
        return { status, schemaMeta };
    }
    if (status != SUCCESS) {
        return { status, schemaMeta };
    }
    MetaDataManager::GetInstance().SaveMeta(schemaKey, schemaMeta, true);
    return { SUCCESS, schemaMeta };
}

std::pair<int32_t, CloudInfo> CloudServiceImpl::GetCloudInfo(int32_t userId)
{
    auto [status, cloudInfo] = GetCloudInfoFromMeta(userId);
    if (status == SUCCESS) {
        return { status, cloudInfo };
    }
    if (!Account::GetInstance()->IsVerified(userId)) {
        ZLOGW("user:%{public}d is locked!", userId);
        return { ERROR, cloudInfo };
    }
    std::tie(status, cloudInfo) = GetCloudInfoFromServer(userId);
    if (status != SUCCESS) {
        ZLOGE("userId:%{public}d, status:%{public}d", userId, status);
        return { status, cloudInfo };
    }
    MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true);
    return { SUCCESS, cloudInfo };
}

int32_t CloudServiceImpl::CloudStatic::OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index)
{
    Subscription sub;
    if (MetaDataManager::GetInstance().LoadMeta(Subscription::GetKey(user), sub, true)) {
        sub.expiresTime.erase(bundleName);
        MetaDataManager::GetInstance().SaveMeta(Subscription::GetKey(user), sub, true);
    }

    CloudInfo cloudInfo;
    cloudInfo.user = user;
    if (MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        cloudInfo.apps.erase(bundleName);
        MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true);
    }

    MetaDataManager::GetInstance().DelMeta(Subscription::GetRelationKey(user, bundleName), true);
    MetaDataManager::GetInstance().DelMeta(CloudInfo::GetSchemaKey(user, bundleName, index), true);
    MetaDataManager::GetInstance().DelMeta(NetworkSyncStrategy::GetKey(user, bundleName), true);
    return E_OK;
}

int32_t CloudServiceImpl::CloudStatic::OnAppInstall(const std::string &bundleName, int32_t user, int32_t index)
{
    ZLOGI("bundleName:%{public}s,user:%{public}d,instanceId:%{public}d", bundleName.c_str(), user, index);
    CheckerManager::StoreInfo info;
    info.uid = IPCSkeleton::GetCallingUid();
    info.tokenId = IPCSkeleton::GetCallingTokenID();
    info.bundleName = bundleName;
    if (CheckerManager::GetInstance().IsValid(info)) {
        // cloudDriver install, update schema(for update 'e2eeEnable')
        ZLOGI("cloud driver install, bundleName:%{public}s, user:%{public}d, instanceId:%{public}d",
            bundleName.c_str(), user, index);
        Execute([this, user]() { UpdateSchemaFromServer(user); });
        return SUCCESS;
    }
    auto ret = UpdateCloudInfoFromServer(user);
    if (ret == E_OK) {
        StoreInfo info{.bundleName = bundleName,  .instanceId = index, .user = user};
        EventCenter::GetInstance().PostEvent(std::make_unique<CloudEvent>(CloudEvent::GET_SCHEMA, info));
    }
    return ret;
}

int32_t CloudServiceImpl::CloudStatic::OnAppUpdate(const std::string &bundleName, int32_t user, int32_t index)
{
    ZLOGI("bundleName:%{public}s,user:%{public}d,instanceId:%{public}d", bundleName.c_str(), user, index);
    CheckerManager::StoreInfo info;
    info.uid = IPCSkeleton::GetCallingUid();
    info.tokenId = IPCSkeleton::GetCallingTokenID();
    info.bundleName = bundleName;
    if (CheckerManager::GetInstance().IsValid(info)) {
        // cloudDriver updated, update schema(for update 'e2eeEnable')
        ZLOGI("cloud driver updated, bundleName:%{public}s, user:%{public}d, instanceId:%{public}d",
            bundleName.c_str(), user, index);
        Execute([this, user]() { UpdateSchemaFromServer(user); });
        return SUCCESS;
    }
    HapInfo hapInfo{ .user = user, .instIndex = index, .bundleName = bundleName };
    Execute([this, hapInfo]() { UpdateSchemaFromHap(hapInfo); });
    return SUCCESS;
}

int32_t CloudServiceImpl::UpdateSchemaFromHap(const HapInfo &hapInfo)
{
    auto [status, cloudInfo] = GetCloudInfoFromMeta(hapInfo.user);
    if (status != SUCCESS) {
        return status;
    }
    if (!cloudInfo.Exist(hapInfo.bundleName, hapInfo.instIndex)) {
        return ERROR;
    }

    std::string schemaKey = CloudInfo::GetSchemaKey(hapInfo.user, hapInfo.bundleName, hapInfo.instIndex);
    auto [ret, newSchemaMeta] = GetSchemaFromHap(hapInfo);
    if (ret != SUCCESS) {
        std::tie(ret, newSchemaMeta) = GetAppSchemaFromServer(hapInfo.user, hapInfo.bundleName);
    }
    if (ret != SUCCESS) {
        MetaDataManager::GetInstance().DelMeta(schemaKey, true);
        return ret;
    }

    SchemaMeta schemaMeta;
    if (MetaDataManager::GetInstance().LoadMeta(schemaKey, schemaMeta, true)) {
        UpdateClearWaterMark(hapInfo, newSchemaMeta, schemaMeta);
    }
    MetaDataManager::GetInstance().SaveMeta(schemaKey, newSchemaMeta, true);
    return SUCCESS;
}

void CloudServiceImpl::UpdateClearWaterMark(
    const HapInfo &hapInfo, const SchemaMeta &newSchemaMeta, const SchemaMeta &schemaMeta)
{
    if (newSchemaMeta.version == schemaMeta.version) {
        return;
    }
    ZLOGI("update schemaMeta newVersion:%{public}d,oldVersion:%{public}d", newSchemaMeta.version, schemaMeta.version);
    CloudMark metaData;
    metaData.bundleName = hapInfo.bundleName;
    metaData.userId = hapInfo.user;
    metaData.index = hapInfo.instIndex;
    metaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;

    std::unordered_map<std::string, uint32_t> dbMap;
    for (const auto &database : schemaMeta.databases) {
        dbMap[database.name] = database.version;
    }

    for (const auto &database : newSchemaMeta.databases) {
        if (dbMap.find(database.name) != dbMap.end() && database.version != dbMap[database.name]) {
            metaData.storeId = database.name;
            metaData.isClearWaterMark = true;
            MetaDataManager::GetInstance().SaveMeta(metaData.GetKey(), metaData, true);
            ZLOGI("clear watermark, storeId:%{public}s, newVersion:%{public}d, oldVersion:%{public}d",
                Anonymous::Change(metaData.storeId).c_str(), database.version, dbMap[database.name]);
        }
    }
}

std::pair<int32_t, SchemaMeta> CloudServiceImpl::GetSchemaFromHap(const HapInfo &hapInfo)
{
    SchemaMeta schemaMeta;
    AppInfo info{ .bundleName = hapInfo.bundleName, .userId = hapInfo.user, .appIndex = hapInfo.instIndex };
    auto schemas = GetSchemaHelper::GetInstance().GetSchemaFromHap(CLOUD_SCHEMA, info);
    for (auto &schema : schemas) {
        if (schemaMeta.Unmarshall(schema)) {
            return { SUCCESS, schemaMeta };
        }
    }
    ZLOGD("get schema from hap failed, bundleName:%{public}s", hapInfo.bundleName.c_str());
    return { ERROR, schemaMeta };
}

void CloudServiceImpl::GetSchema(const Event &event)
{
    auto &rdbEvent = static_cast<const CloudEvent &>(event);
    auto &storeInfo = rdbEvent.GetStoreInfo();
    ZLOGD("Start GetSchema, bundleName:%{public}s, storeName:%{public}s, instanceId:%{public}d",
        storeInfo.bundleName.c_str(), Anonymous::Change(storeInfo.storeName).c_str(), storeInfo.instanceId);
    if (storeInfo.user == 0) {
        std::vector<int32_t> users;
        AccountDelegate::GetInstance()->QueryUsers(users);
        GetSchemaMeta(users[0], storeInfo.bundleName, storeInfo.instanceId);
    } else {
        GetSchemaMeta(storeInfo.user, storeInfo.bundleName, storeInfo.instanceId);
    }
}

void CloudServiceImpl::CloudShare(const Event &event)
{
    auto &cloudShareEvent = static_cast<const CloudShareEvent &>(event);
    auto &storeInfo = cloudShareEvent.GetStoreInfo();
    auto query = cloudShareEvent.GetQuery();
    auto callback = cloudShareEvent.GetCallback();
    if (query == nullptr) {
        ZLOGE("query is null, bundleName:%{public}s, storeName:%{public}s, instanceId:%{public}d",
            storeInfo.bundleName.c_str(), Anonymous::Change(storeInfo.storeName).c_str(), storeInfo.instanceId);
        if (callback) {
            callback(GeneralError::E_ERROR, nullptr);
        }
        return;
    }
    ZLOGD("Start PreShare, bundleName:%{public}s, storeName:%{public}s, instanceId:%{public}d",
        storeInfo.bundleName.c_str(), Anonymous::Change(storeInfo.storeName).c_str(), storeInfo.instanceId);
    auto [status, cursor] = PreShare(storeInfo, *query);

    if (callback) {
        callback(status, cursor);
    }
}

void CloudServiceImpl::DoSync(const Event &event)
{
    auto &cloudEvent = static_cast<const CloudEvent &>(event);
    auto &storeInfo = cloudEvent.GetStoreInfo();
    SyncManager::SyncInfo info(storeInfo.user, storeInfo.bundleName);
    syncManager_.DoCloudSync(info);
    return;
}

std::pair<int32_t, std::shared_ptr<DistributedData::Cursor>> CloudServiceImpl::PreShare(
    const StoreInfo &storeInfo, GenQuery &query)
{
    StoreMetaMapping meta(storeInfo);
    meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true)) {
        ZLOGE("failed, no store mapping bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
            meta.GetStoreAlias().c_str());
        return { GeneralError::E_ERROR, nullptr };
    }
    if (!meta.cloudPath.empty() && meta.dataDir != meta.cloudPath &&
        !MetaDataManager::GetInstance().LoadMeta(meta.GetCloudStoreMetaKey(), meta, true)) {
        ZLOGE("failed, no store meta bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
            meta.GetStoreAlias().c_str());
        return { GeneralError::E_ERROR, nullptr };
    }
    auto [status, store] = SyncManager::GetStore(meta, storeInfo.user, true);
    if (store == nullptr) {
        ZLOGE("store null, storeId:%{public}s", meta.GetStoreAlias().c_str());
        return { GeneralError::E_ERROR, nullptr };
    }
    return store->PreSharing(query);
}

bool CloudServiceImpl::ReleaseUserInfo(int32_t user, CloudSyncScene scene)
{
    auto instance = CloudServer::GetInstance();
    if (instance == nullptr) {
        return true;
    }
    instance->ReleaseUserInfo(user);
    ZLOGI("notify release user info, user:%{public}d", user);
    return true;
}

int32_t CloudServiceImpl::InitNotifier(sptr<IRemoteObject> notifier)
{
    if (notifier == nullptr) {
        ZLOGE("no notifier.");
        return INVALID_ARGUMENT;
    }
    auto notifierProxy = iface_cast<CloudNotifierProxy>(notifier);
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    syncAgents_.Compute(tokenId, [notifierProxy](auto, SyncAgent &agent) {
        agent = SyncAgent();
        agent.notifier_ = notifierProxy;
        return true;
    });
    return SUCCESS;
}

Details CloudServiceImpl::HandleGenDetails(const GenDetails &details)
{
    Details dbDetails;
    for (const auto& [id, detail] : details) {
        auto &dbDetail = dbDetails[id];
        dbDetail.progress = detail.progress;
        dbDetail.code = detail.code;
        for (auto &[name, table] : detail.details) {
            auto &dbTable = dbDetail.details[name];
            Constant::Copy(&dbTable, &table);
        }
    }
    return dbDetails;
}

void CloudServiceImpl::OnAsyncComplete(uint32_t tokenId, uint32_t seqNum, Details &&result)
{
    ZLOGI("tokenId=%{public}x, seqnum=%{public}u", tokenId, seqNum);
    sptr<CloudNotifierProxy> notifier = nullptr;
    syncAgents_.ComputeIfPresent(tokenId, [&notifier](auto, SyncAgent &syncAgent) {
        notifier = syncAgent.notifier_;
        return true;
    });
    if (notifier != nullptr) {
        notifier->OnComplete(seqNum, std::move(result));
    }
}

int32_t CloudServiceImpl::CloudSync(const std::string &bundleName, const std::string &storeId,
    const Option &option, const AsyncDetail &async)
{
    if (option.syncMode < DistributedData::GeneralStore::CLOUD_BEGIN ||
        option.syncMode >= DistributedData::GeneralStore::CLOUD_END) {
        ZLOGE("not support cloud sync, syncMode = %{public}d", option.syncMode);
        return INVALID_ARGUMENT;
    }
    StoreInfo storeInfo;
    storeInfo.bundleName = bundleName;
    storeInfo.tokenId = IPCSkeleton::GetCallingTokenID();
    storeInfo.user = AccountDelegate::GetInstance()->GetUserByToken(storeInfo.tokenId);
    storeInfo.storeName = storeId;
    GenAsync asyncCallback =
        [this, tokenId = storeInfo.tokenId, seqNum = option.seqNum](const GenDetails &result) mutable {
        OnAsyncComplete(tokenId, seqNum, HandleGenDetails(result));
    };
    auto highMode = GeneralStore::MANUAL_SYNC_MODE;
    auto mixMode = static_cast<int32_t>(GeneralStore::MixMode(option.syncMode, highMode));
    SyncParam syncParam = { mixMode, 0, false };
    auto info = ChangeEvent::EventInfo(syncParam, false, nullptr, asyncCallback);
    auto evt = std::make_unique<ChangeEvent>(std::move(storeInfo), std::move(info));
    EventCenter::GetInstance().PostEvent(std::move(evt));
    return SUCCESS;
}

bool CloudServiceImpl::DoCloudSync(int32_t user, CloudSyncScene scene)
{
    auto [status, cloudInfo] = GetCloudInfo(user);
    if (status != SUCCESS) {
        Report(GetDfxFaultType(scene), Fault::CSF_CLOUD_INFO, "", "DoCloudSync ret=" + std::to_string(status));
        return false;
    }
    for (const auto &appInfo : cloudInfo.apps) {
        SyncManager::SyncInfo info(user, appInfo.first);
        syncManager_.DoCloudSync(info);
    }
    return true;
}

bool CloudServiceImpl::StopCloudSync(int32_t user, CloudSyncScene scene)
{
    syncManager_.StopCloudSync(user);
    syncManager_.CleanCompensateSync(user);
    return true;
}

bool CloudServiceImpl::DoSubscribe(int32_t user, CloudSyncScene scene)
{
    Subscription sub;
    sub.userId = user;
    MetaDataManager::GetInstance().LoadMeta(sub.GetKey(), sub, true);
    if (CloudServer::GetInstance() == nullptr) {
        ZLOGE("not support cloud server");
        return true;
    }
    CloudInfo cloudInfo;
    cloudInfo.user = sub.userId;
    if (!MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        ZLOGW("error, there is no cloud info for user(%{public}d)", sub.userId);
        return false;
    }
    if (!sub.id.empty() && sub.id != cloudInfo.id) {
        CleanSubscription(sub);
    }
    auto onThreshold = duration_cast<milliseconds>((system_clock::now() + hours(EXPIRE_INTERVAL)).time_since_epoch());
    auto offThreshold = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    std::map<std::string, std::vector<SchemaMeta::Database>> subDbs;
    std::map<std::string, std::vector<SchemaMeta::Database>> unsubDbs;
    for (auto &[bundle, app] : cloudInfo.apps) {
        auto enabled = cloudInfo.enableCloud && app.cloudSwitch;
        auto &dbs = enabled ? subDbs : unsubDbs;
        auto it = sub.expiresTime.find(bundle);
        // cloud is enabled, but the subscription won't expire
        if (enabled && (it != sub.expiresTime.end() && it->second >= static_cast<uint64_t>(onThreshold.count()))) {
            continue;
        }
        // cloud is disabled, we don't care the subscription which was expired or didn't subscribe.
        if (!enabled && (it == sub.expiresTime.end() || it->second <= static_cast<uint64_t>(offThreshold.count()))) {
            continue;
        }
        SchemaMeta schemaMeta;
        if (MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(bundle), schemaMeta, true)) {
            dbs.insert_or_assign(bundle, std::move(schemaMeta.databases));
        }
    }
    ZLOGI("cloud switch:%{public}d user%{public}d, sub:%{public}zu, unsub:%{public}zu", cloudInfo.enableCloud,
        sub.userId, subDbs.size(), unsubDbs.size());
    auto status = CloudServer::GetInstance()->Subscribe(sub.userId, subDbs);
    if (status != SUCCESS) {
        Report(GetDfxFaultType(scene), Fault::CSF_SUBSCRIBE, "", "Subscribe ret=" + std::to_string(status));
    }
    status = CloudServer::GetInstance()->Unsubscribe(sub.userId, unsubDbs);
    if (status != SUCCESS && scene != CloudSyncScene::DISABLE_CLOUD) {
        Report(GetDfxFaultType(scene), Fault::CSF_UNSUBSCRIBE, "", "Unsubscribe, ret=" + std::to_string(status));
    }
    return subDbs.empty() && unsubDbs.empty();
}

void CloudServiceImpl::CleanSubscription(Subscription &sub)
{
    ZLOGD("id:%{public}s, size:%{public}zu", Anonymous::Change(sub.id).c_str(), sub.expiresTime.size());
    MetaDataManager::GetInstance().DelMeta(sub.GetKey(), true);
    for (const auto &[bundle, expireTime] : sub.expiresTime) {
        MetaDataManager::GetInstance().DelMeta(sub.GetRelationKey(bundle), true);
    }
    sub.id.clear();
    sub.expiresTime.clear();
}

void CloudServiceImpl::Execute(Task task)
{
    auto executor = executor_;
    if (executor == nullptr) {
        return;
    }
    executor->Execute(std::move(task));
}

std::map<std::string, int32_t> CloudServiceImpl::ConvertAction(const std::map<std::string, int32_t> &actions)
{
    std::map<std::string, int32_t> genActions;
    for (const auto &[bundleName, action] : actions) {
        switch (action) {
            case CloudService::Action::CLEAR_CLOUD_INFO:
                genActions.emplace(bundleName, GeneralStore::CleanMode::CLOUD_INFO);
                break;
            case CloudService::Action::CLEAR_CLOUD_DATA_AND_INFO:
                genActions.emplace(bundleName, GeneralStore::CleanMode::CLOUD_DATA);
                break;
            default:
                ZLOGE("invalid action. action:%{public}d, bundleName:%{public}s", action, bundleName.c_str());
                return {};
        }
    }
    return genActions;
}

std::pair<int32_t, std::vector<NativeRdb::ValuesBucket>> CloudServiceImpl::AllocResourceAndShare(
    const std::string &storeId, const DistributedRdb::PredicatesMemo &predicates,
    const std::vector<std::string> &columns, const Participants &participants)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto hapInfo = GetHapInfo(tokenId);
    if (hapInfo.bundleName.empty() || hapInfo.user == INVALID_USER_ID) {
        ZLOGE("bundleName is empty or invalid user, user:%{public}d, storeId:%{public}s", hapInfo.user,
            Anonymous::Change(storeId).c_str());
        return { E_ERROR, {} };
    }
    if (predicates.tables_.empty()) {
        ZLOGE("invalid args, tables size:%{public}zu, storeId:%{public}s", predicates.tables_.size(),
            Anonymous::Change(storeId).c_str());
        return { E_INVALID_ARGS, {} };
    }
    auto memo = std::make_shared<DistributedRdb::PredicatesMemo>(predicates);
    StoreInfo storeInfo;
    storeInfo.bundleName = hapInfo.bundleName;
    storeInfo.tokenId = tokenId;
    storeInfo.user = hapInfo.user;
    storeInfo.storeName = storeId;
    std::shared_ptr<GenQuery> query;
    MakeQueryEvent::Callback asyncCallback = [&query](std::shared_ptr<GenQuery> genQuery) {
        query = genQuery;
    };
    auto evt = std::make_unique<MakeQueryEvent>(storeInfo, memo, columns, asyncCallback);
    EventCenter::GetInstance().PostEvent(std::move(evt));
    if (query == nullptr) {
        ZLOGE("query is null, storeId:%{public}s,", Anonymous::Change(storeId).c_str());
        return { E_ERROR, {} };
    }
    auto [status, cursor] = PreShare(storeInfo, *query);
    if (status != GeneralError::E_OK || cursor == nullptr) {
        ZLOGE("PreShare fail, storeId:%{public}s, status:%{public}d", Anonymous::Change(storeId).c_str(), status);
        return { E_ERROR, {} };
    }
    auto valueBuckets = ConvertCursor(cursor);
    Results results;
    for (auto &valueBucket : valueBuckets) {
        NativeRdb::ValueObject object;
        if (!valueBucket.GetObject(DistributedRdb::Field::SHARING_RESOURCE_FIELD, object)) {
            continue;
        }
        std::string shareRes;
        if (object.GetString(shareRes) != E_OK) {
            continue;
        }
        Share(shareRes, participants, results);
    }
    return { SUCCESS, std::move(valueBuckets) };
}

std::vector<NativeRdb::ValuesBucket> CloudServiceImpl::ConvertCursor(std::shared_ptr<Cursor> cursor) const
{
    std::vector<NativeRdb::ValuesBucket> valueBuckets;
    int32_t count = cursor->GetCount();
    valueBuckets.reserve(count);
    auto err = cursor->MoveToFirst();
    while (err == E_OK && count > 0) {
        VBucket entry;
        err = cursor->GetEntry(entry);
        if (err != E_OK) {
            break;
        }
        NativeRdb::ValuesBucket bucket;
        for (auto &[key, value] : entry) {
            NativeRdb::ValueObject object;
            DistributedData::Convert(std::move(value), object.value);
            bucket.values_.insert_or_assign(key, std::move(object));
        }
        valueBuckets.emplace_back(std::move(bucket));
        err = cursor->MoveToNext();
        count--;
    }
    return valueBuckets;
}

CloudServiceImpl::HapInfo CloudServiceImpl::GetHapInfo(uint32_t tokenId)
{
    HapTokenInfo tokenInfo;
    int errCode = AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (errCode != RET_SUCCESS) {
        ZLOGE("GetHapTokenInfo error:%{public}d, tokenId:0x%{public}x", errCode, tokenId);
        return { INVALID_USER_ID, -1, "" };
    }
    return { tokenInfo.userID, tokenInfo.instIndex, tokenInfo.bundleName };
}

std::string CloudServiceImpl::GetDfxFaultType(CloudSyncScene scene)
{
    switch (scene) {
        case CloudSyncScene::ENABLE_CLOUD:
            return FT_ENABLE_CLOUD;
        case CloudSyncScene::DISABLE_CLOUD:
            return FT_DISABLE_CLOUD;
        case CloudSyncScene::SWITCH_ON:
            return FT_SWITCH_ON;
        case CloudSyncScene::SWITCH_OFF:
            return FT_SWITCH_OFF;
        case CloudSyncScene::QUERY_SYNC_INFO:
            return FT_QUERY_INFO;
        case CloudSyncScene::USER_CHANGE:
            return FT_USER_CHANGE;
        case CloudSyncScene::USER_UNLOCK:
            return FT_USER_UNLOCK;
        case CloudSyncScene::NETWORK_RECOVERY:
            return FT_NETWORK_RECOVERY;
        case CloudSyncScene::SERVICE_INIT:
            return FT_SERVICE_INIT;
        default:
            return FT_SYNC_TASK;
    }
}

int32_t CloudServiceImpl::Share(const std::string &sharingRes, const Participants &participants, Results &results)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    auto hapInfo = GetHapInfo(IPCSkeleton::GetCallingTokenID());
    if (hapInfo.bundleName.empty()) {
        ZLOGE("bundleName is empty, sharingRes:%{public}s", Anonymous::Change(sharingRes).c_str());
        return E_ERROR;
    }
    RadarReporter radar(EventName::CLOUD_SHARING_BEHAVIOR, BizScene::SHARE, hapInfo.bundleName.c_str(), __FUNCTION__);
    auto instance = GetSharingHandle(hapInfo);
    if (instance == nullptr) {
        radar = CenterCode::NOT_SUPPORT + SharingCenter::SHARING_ERR_OFFSET;
        return NOT_SUPPORT;
    }
    results = instance->Share(hapInfo.user, hapInfo.bundleName, sharingRes, Convert(participants));
    int32_t status = std::get<0>(results);
    radar = (status == CenterCode::SUCCESS) ? CenterCode::SUCCESS : (status + SharingCenter::SHARING_ERR_OFFSET);
    ZLOGD("status:%{public}d", status);
    return Convert(static_cast<CenterCode>(status));
}

int32_t CloudServiceImpl::Unshare(const std::string &sharingRes, const Participants &participants, Results &results)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    auto hapInfo = GetHapInfo(IPCSkeleton::GetCallingTokenID());
    if (hapInfo.bundleName.empty()) {
        ZLOGE("bundleName is empty, sharingRes:%{public}s", Anonymous::Change(sharingRes).c_str());
        return E_ERROR;
    }
    RadarReporter radar(EventName::CLOUD_SHARING_BEHAVIOR, BizScene::UNSHARE, hapInfo.bundleName.c_str(), __FUNCTION__);
    auto instance = GetSharingHandle(hapInfo);
    if (instance == nullptr) {
        radar = CenterCode::NOT_SUPPORT + SharingCenter::SHARING_ERR_OFFSET;
        return NOT_SUPPORT;
    }
    results = instance->Unshare(hapInfo.user, hapInfo.bundleName, sharingRes, Convert(participants));
    int32_t status = std::get<0>(results);
    radar = (status == CenterCode::SUCCESS) ? CenterCode::SUCCESS : (status + SharingCenter::SHARING_ERR_OFFSET);
    ZLOGD("status:%{public}d", status);
    return Convert(static_cast<CenterCode>(status));
}

int32_t CloudServiceImpl::Exit(const std::string &sharingRes, std::pair<int32_t, std::string> &result)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    auto hapInfo = GetHapInfo(IPCSkeleton::GetCallingTokenID());
    if (hapInfo.bundleName.empty()) {
        ZLOGE("bundleName is empty, sharingRes:%{public}s", Anonymous::Change(sharingRes).c_str());
        return E_ERROR;
    }
    RadarReporter radar(
        EventName::CLOUD_SHARING_BEHAVIOR, BizScene::EXIT_SHARING, hapInfo.bundleName.c_str(), __FUNCTION__);
    auto instance = GetSharingHandle(hapInfo);
    if (instance == nullptr) {
        radar = CenterCode::NOT_SUPPORT + SharingCenter::SHARING_ERR_OFFSET;
        return NOT_SUPPORT;
    }
    result = instance->Exit(hapInfo.user, hapInfo.bundleName, sharingRes);
    int32_t status = result.first;
    radar = (status == CenterCode::SUCCESS) ? CenterCode::SUCCESS : (status + SharingCenter::SHARING_ERR_OFFSET);
    ZLOGD("status:%{public}d", status);
    return Convert(static_cast<CenterCode>(status));
}

int32_t CloudServiceImpl::ChangePrivilege(const std::string &sharingRes, const Participants &participants,
    Results &results)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    auto hapInfo = GetHapInfo(IPCSkeleton::GetCallingTokenID());
    if (hapInfo.bundleName.empty()) {
        ZLOGE("bundleName is empty, sharingRes:%{public}s", Anonymous::Change(sharingRes).c_str());
        return E_ERROR;
    }
    RadarReporter radar(
        EventName::CLOUD_SHARING_BEHAVIOR, BizScene::CHANGE_PRIVILEGE, hapInfo.bundleName.c_str(), __FUNCTION__);
    auto instance = GetSharingHandle(hapInfo);
    if (instance == nullptr) {
        radar = CenterCode::NOT_SUPPORT + SharingCenter::SHARING_ERR_OFFSET;
        return NOT_SUPPORT;
    }
    results = instance->ChangePrivilege(hapInfo.user, hapInfo.bundleName, sharingRes, Convert(participants));
    int32_t status = std::get<0>(results);
    radar = (status == CenterCode::SUCCESS) ? CenterCode::SUCCESS : (status + SharingCenter::SHARING_ERR_OFFSET);
    ZLOGD("status:%{public}d", status);
    return Convert(static_cast<CenterCode>(status));
}

int32_t CloudServiceImpl::Query(const std::string &sharingRes, QueryResults &results)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    auto hapInfo = GetHapInfo(IPCSkeleton::GetCallingTokenID());
    if (hapInfo.bundleName.empty()) {
        ZLOGE("bundleName is empty, sharingRes:%{public}s", Anonymous::Change(sharingRes).c_str());
        return E_ERROR;
    }
    auto instance = GetSharingHandle(hapInfo);
    if (instance == nullptr) {
        return NOT_SUPPORT;
    }
    auto queryResults = instance->Query(hapInfo.user, hapInfo.bundleName, sharingRes);
    results = Convert(queryResults);
    int32_t status = std::get<0>(queryResults);
    ZLOGD("status:%{public}d", status);
    return Convert(static_cast<CenterCode>(status));
}

int32_t CloudServiceImpl::QueryByInvitation(const std::string &invitation, QueryResults &results)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    auto hapInfo = GetHapInfo(IPCSkeleton::GetCallingTokenID());
    if (hapInfo.bundleName.empty()) {
        ZLOGE("bundleName is empty, invitation:%{public}s", Anonymous::Change(invitation).c_str());
        return E_ERROR;
    }
    auto instance = GetSharingHandle(hapInfo);
    if (instance == nullptr) {
        return NOT_SUPPORT;
    }
    auto queryResults = instance->QueryByInvitation(hapInfo.user, hapInfo.bundleName, invitation);
    results = Convert(queryResults);
    int32_t status = std::get<0>(queryResults);
    ZLOGD("status:%{public}d", status);
    return Convert(static_cast<CenterCode>(status));
}

int32_t CloudServiceImpl::ConfirmInvitation(const std::string &invitation, int32_t confirmation,
    std::tuple<int32_t, std::string, std::string> &result)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    auto hapInfo = GetHapInfo(IPCSkeleton::GetCallingTokenID());
    if (hapInfo.bundleName.empty()) {
        ZLOGE("bundleName is empty, invitation:%{public}s, confirmation:%{public}d",
            Anonymous::Change(invitation).c_str(), confirmation);
        return E_ERROR;
    }
    RadarReporter radar(
        EventName::CLOUD_SHARING_BEHAVIOR, BizScene::CONFIRM_INVITATION, hapInfo.bundleName.c_str(), __FUNCTION__);
    auto instance = GetSharingHandle(hapInfo);
    if (instance == nullptr) {
        radar = CenterCode::NOT_SUPPORT + SharingCenter::SHARING_ERR_OFFSET;
        return NOT_SUPPORT;
    }
    result = instance->ConfirmInvitation(hapInfo.user, hapInfo.bundleName, invitation, confirmation);
    int32_t status = std::get<0>(result);
    radar = (status == CenterCode::SUCCESS) ? CenterCode::SUCCESS : (status + SharingCenter::SHARING_ERR_OFFSET);
    ZLOGD("status:%{public}d", status);
    return Convert(static_cast<CenterCode>(status));
}

int32_t CloudServiceImpl::ChangeConfirmation(const std::string &sharingRes, int32_t confirmation,
    std::pair<int32_t, std::string> &result)
{
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    auto hapInfo = GetHapInfo(IPCSkeleton::GetCallingTokenID());
    if (hapInfo.bundleName.empty()) {
        ZLOGE("bundleName is empty, sharingRes:%{public}s", Anonymous::Change(sharingRes).c_str());
        return E_ERROR;
    }
    RadarReporter radar(
        EventName::CLOUD_SHARING_BEHAVIOR, BizScene::CHANGE_CONFIRMATION, hapInfo.bundleName.c_str(), __FUNCTION__);
    auto instance = GetSharingHandle(hapInfo);
    if (instance == nullptr) {
        radar = CenterCode::NOT_SUPPORT + SharingCenter::SHARING_ERR_OFFSET;
        return NOT_SUPPORT;
    }
    result = instance->ChangeConfirmation(hapInfo.user, hapInfo.bundleName, sharingRes, confirmation);
    int32_t status = result.first;
    radar = (status == CenterCode::SUCCESS) ? CenterCode::SUCCESS : (status + SharingCenter::SHARING_ERR_OFFSET);
    ZLOGD("status:%{public}d", status);
    return Convert(static_cast<CenterCode>(status));
}

std::shared_ptr<SharingCenter> CloudServiceImpl::GetSharingHandle(const HapInfo &hapInfo)
{
    auto instance = CloudServer::GetInstance();
    if (instance == nullptr) {
        return nullptr;
    }
    auto handle = instance->ConnectSharingCenter(hapInfo.user, hapInfo.bundleName);
    return handle;
}

ExecutorPool::Task CloudServiceImpl::GenSubTask(Task task, int32_t user)
{
    return [this, user, work = std::move(task)]() {
        {
            std::lock_guard<decltype(mutex_)> lock(mutex_);
            subTask_ = ExecutorPool::INVALID_TASK_ID;
        }
        auto [status, cloudInfo] = GetCloudInfoFromMeta(user);
        if (status != SUCCESS || !cloudInfo.enableCloud || cloudInfo.IsAllSwitchOff()) {
            ZLOGW("[sub task] all switch off, status:%{public}d user:%{public}d enableCloud:%{public}d",
                status, user, cloudInfo.enableCloud);
            return;
        }
        work();
    };
}

void CloudServiceImpl::InitSubTask(const Subscription &sub, uint64_t minInterval)
{
    auto expire = sub.GetMinExpireTime();
    if (expire == INVALID_SUB_TIME) {
        return;
    }
    auto executor = executor_;
    if (executor == nullptr) {
        return;
    }
    ZLOGI("Subscription Info, subTask:%{public}" PRIu64 ", user:%{public}d", subTask_, sub.userId);
    expire = expire - TIME_BEFORE_SUB; // before 12 hours
    auto now = static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    Duration delay = milliseconds(std::max(expire > now ? expire - now : 0, minInterval));
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (subTask_ != ExecutorPool::INVALID_TASK_ID) {
        if (expire < expireTime_) {
            subTask_ = executor->Reset(subTask_, delay);
            expireTime_ = expire > now ? expire : now;
        }
        return;
    }
    subTask_ = executor->Schedule(delay, GenSubTask(GenTask(0, sub.userId, CloudSyncScene::SERVICE_INIT), sub.userId));
    expireTime_ = expire > now ? expire : now;
}

int32_t CloudServiceImpl::SetCloudStrategy(Strategy strategy, const std::vector<CommonType::Value> &values)
{
    if (strategy >= Strategy::STRATEGY_BUTT) {
        ZLOGE("invalid strategy:%{public}d, size:%{public}zu", strategy, values.size());
        return INVALID_ARGUMENT;
    }
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto hapInfo = GetHapInfo(tokenId);
    if (hapInfo.bundleName.empty() || hapInfo.user == INVALID_USER_ID || hapInfo.user == 0) {
        ZLOGE("invalid, user:%{public}d, bundleName:%{public}s, strategy:%{public}d, values size:%{public}zu",
            hapInfo.user, hapInfo.bundleName.c_str(), strategy, values.size());
        return ERROR;
    }
    return STRATEGY_SAVERS[strategy](values, hapInfo);
}

int32_t CloudServiceImpl::SaveNetworkStrategy(const std::vector<CommonType::Value> &values, const HapInfo &hapInfo)
{
    NetworkSyncStrategy::StrategyInfo info;
    info.strategy = 0;
    info.user = hapInfo.user;
    info.bundleName = hapInfo.bundleName;
    if (values.empty()) {
        return MetaDataManager::GetInstance().DelMeta(info.GetKey(), true) ? SUCCESS : ERROR;
    }
    for (auto &value : values) {
        auto strategy = std::get_if<int64_t>(&value);
        if (strategy != nullptr) {
            info.strategy |= static_cast<uint32_t>(*strategy);
        }
    }
    NetworkSyncStrategy::StrategyInfo oldInfo;
    MetaDataManager::GetInstance().LoadMeta(info.GetKey(), oldInfo, true);
    ZLOGI("Strategy[user:%{public}d,bundleName:%{public}s] to [%{public}d] from [%{public}d]",
          info.user, info.bundleName.c_str(), info.strategy, oldInfo.strategy);
    return MetaDataManager::GetInstance().SaveMeta(info.GetKey(), info, true) ? SUCCESS : ERROR;
}

int32_t CloudServiceImpl::OnScreenUnlocked(int32_t user)
{
    syncManager_.OnScreenUnlocked(user);
    return E_OK;
}

QueryLastResults CloudServiceImpl::AssembleLastResults(const std::vector<Database> &databases,
    const std::map<std::string, CloudLastSyncInfo> &lastSyncInfos)
{
    QueryLastResults results;
    for (const auto &database : databases) {
        auto iter = lastSyncInfos.find(database.name);
        if (iter != lastSyncInfos.end()) {
            CloudSyncInfo syncInfo = { .startTime = iter->second.startTime, .finishTime = iter->second.finishTime,
                                       .code = iter->second.code, .syncStatus = iter->second.syncStatus };
            results.insert({ database.alias, std::move(syncInfo) });
        }
    }
    return results;
}
} // namespace OHOS::CloudData