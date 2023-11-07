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
#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "checker/checker_manager.h"
#include "cloud/cloud_server.h"
#include "communicator/device_manager_adapter.h"
#include "eventcenter/event_center.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "rdb_cloud_data_translate.h"
#include "runtime_config.h"
#include "store/auto_cache.h"
#include "store/general_store.h"
#include "utils/anonymous.h"
#include "sync_manager.h"
namespace OHOS::CloudData {
using namespace DistributedData;
using namespace std::chrono;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Account = OHOS::DistributedKv::AccountDelegate;
using AccessTokenKit = Security::AccessToken::AccessTokenKit;
__attribute__((used)) CloudServiceImpl::Factory CloudServiceImpl::factory_;

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
    FeatureSystem::GetInstance().RegisterStaticActs(CloudServiceImpl::SERVICE_NAME,
        staticActs_);
}

CloudServiceImpl::Factory::~Factory() {}

CloudServiceImpl::CloudServiceImpl()
{
    EventCenter::GetInstance().Subscribe(CloudEvent::GET_SCHEMA, [this](const Event &event) {
        GetSchema(event);
    });
}

int32_t CloudServiceImpl::EnableCloud(const std::string &id, const std::map<std::string, int32_t> &switches)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto user = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(tokenId);
    auto [status, cloudInfo] = GetCloudInfo(user);
    if (status != SUCCESS) {
        return status;
    }
    cloudInfo.enableCloud = true;
    for (const auto &[bundle, value] : switches) {
        if (!cloudInfo.Exist(bundle)) {
            continue;
        }
        cloudInfo.apps[bundle].cloudSwitch = (value == SWITCH_ON);
    }
    if (!MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        return ERROR;
    }
    Execute(GenTask(0, cloudInfo.user, { WORK_CLOUD_INFO_UPDATE, WORK_SCHEMA_UPDATE, WORK_SUB }));
    syncManager_.DoCloudSync({ cloudInfo.user });
    return SUCCESS;
}

int32_t CloudServiceImpl::DisableCloud(const std::string &id)
{
    CloudInfo cloudInfo;
    auto status = GetCloudInfo(IPCSkeleton::GetCallingTokenID(), id, cloudInfo);
    if (status != SUCCESS) {
        return status;
    }
    cloudInfo.enableCloud = false;
    if (!MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        return ERROR;
    }
    Execute(GenTask(0, cloudInfo.user, { WORK_SUB, WORK_RELEASE }));
    syncManager_.StopCloudSync(cloudInfo.user);
    return SUCCESS;
}

int32_t CloudServiceImpl::ChangeAppSwitch(const std::string &id, const std::string &bundleName, int32_t appSwitch)
{
    CloudInfo cloudInfo;
    auto status = GetCloudInfo(IPCSkeleton::GetCallingTokenID(), id, cloudInfo);
    if (status != SUCCESS) {
        return status;
    }
    if (!cloudInfo.Exist(bundleName)) {
        ZLOGE("bundleName:%{public}s", bundleName.c_str());
        return INVALID_ARGUMENT;
    }
    cloudInfo.apps[bundleName].cloudSwitch = (appSwitch == SWITCH_ON);
    if (!MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        return ERROR;
    }
    Execute(GenTask(0, cloudInfo.user, { WORK_CLOUD_INFO_UPDATE, WORK_SCHEMA_UPDATE, WORK_SUB }));
    if (cloudInfo.enableCloud && appSwitch == SWITCH_ON) {
        syncManager_.DoCloudSync({ cloudInfo.user, bundleName });
    }
    return SUCCESS;
}

int32_t CloudServiceImpl::DoClean(CloudInfo &cloudInfo, const std::map<std::string, int32_t> &actions)
{
    syncManager_.StopCloudSync(cloudInfo.user);
    auto keys = cloudInfo.GetSchemaKey();
    for (const auto &[bundle, action] : actions) {
        if (!cloudInfo.Exist(bundle)) {
            continue;
        }
        SchemaMeta schemaMeta;
        if (!MetaDataManager::GetInstance().LoadMeta(keys[bundle], schemaMeta, true)) {
            ZLOGE("failed, no schema meta:bundleName:%{public}s", bundle.c_str());
            return ERROR;
        }
        for (const auto &database : schemaMeta.databases) {
            // action
            StoreMetaData meta;
            meta.bundleName = schemaMeta.bundleName;
            meta.storeId = database.name;
            meta.user = std::to_string(cloudInfo.user);
            meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
            meta.instanceId = cloudInfo.apps[bundle].instanceId;
            if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta)) {
                ZLOGE("failed, no store meta bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
                    meta.GetStoreAlias().c_str());
                continue;
            }
            AutoCache::Store store = SyncManager::GetStore(meta, cloudInfo.user, false);
            if (store == nullptr) {
                ZLOGE("store null, storeId:%{public}s", meta.GetStoreAlias().c_str());
                return ERROR;
            }
            auto status = store->Clean({}, action, "");
            if (status != E_OK) {
                ZLOGW("remove device data status:%{public}d, user:%{pubilc}d, bundleName:%{public}s, "
                      "storeId:%{public}s",
                    status, static_cast<int>(cloudInfo.user), meta.bundleName.c_str(), meta.GetStoreAlias().c_str());
                continue;
            }
        }
    }
    return SUCCESS;
}

int32_t CloudServiceImpl::Convert(const std::string &extraData, ExtraData &exData)
{
    if (!exData.Unmarshall(extraData)) {
        ZLOGE("extraData cannot be parsed to valid JSON");
        return ERROR;
    }
    return SUCCESS;
}

int32_t CloudServiceImpl::Clean(const std::string &id, const std::map<std::string, int32_t> &actions)
{
    CloudInfo cloudInfo;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    cloudInfo.user = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(tokenId);
    if (GetCloudInfoFromMeta(cloudInfo) != SUCCESS) {
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
    if (GetCloudInfoFromMeta(cloudInfo) != SUCCESS) {
        return ERROR;
    }
    if (cloudInfo.id != id) {
        ZLOGE("invalid args, [input] id:%{public}s, [meta] id:%{public}s", Anonymous::Change(id).c_str(),
            Anonymous::Change(cloudInfo.id).c_str());
        return INVALID_ARGUMENT;
    }
    if (!cloudInfo.enableCloud) {
        return CLOUD_DISABLE;
    }
    if (!cloudInfo.Exist(bundleName)) {
        ZLOGE("bundleName:%{public}s", bundleName.c_str());
        return INVALID_ARGUMENT;
    }
    if (!cloudInfo.apps[bundleName].cloudSwitch) {
        return CLOUD_DISABLE_SWITCH;
    }
    return SUCCESS;
}

int32_t CloudServiceImpl::GetSchemaMetaInfo(const ExtraData &exData, CloudInfo &cloudInfo)
{
    int status = CheckNotifyConditions(exData.extInfo.accountId, exData.extInfo.bundleName, cloudInfo);
    if (status != E_OK) {
        return INVALID_ARGUMENT;
    }
    auto schemaKey = cloudInfo.GetSchemaKey(cloudInfo.user, exData.extInfo.bundleName);
    SchemaMeta schemaMeta;
    if (!MetaDataManager::GetInstance().LoadMeta(schemaKey, schemaMeta, true)) {
        ZLOGE("no exist meta, user:%{public}d", cloudInfo.user);
        return ERROR;
    }
    std::string storeId;
    std::vector<std::string> table;
    for (auto &db : schemaMeta.databases) {
        if (db.alias != exData.extInfo.containerName) {
            continue;
        }
        storeId = db.name;
        for (auto &tb : db.tables) {
            if (std::find(exData.extInfo.table.tables.begin(), exData.extInfo.table.tables.end(), tb.alias) !=
                exData.extInfo.table.tables.end()) {
                table.emplace_back(tb.name);
            }
        }
    }
    syncManager_.DoCloudSync(SyncManager::SyncInfo(cloudInfo.user, exData.extInfo.bundleName, storeId, table));
    return SUCCESS;
}

int32_t CloudServiceImpl::NotifyDataChange(const std::string &id, const std::string &bundleName)
{
    CloudInfo cloudInfo;
	auto tokenId = IPCSkeleton::GetCallingTokenID();
    cloudInfo.user = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(tokenId);
    int32_t status = CheckNotifyConditions(id, bundleName, cloudInfo);
    if (status != E_OK) {
        return INVALID_ARGUMENT;
    }
    syncManager_.DoCloudSync(SyncManager::SyncInfo(cloudInfo.user, bundleName));
    return SUCCESS;
}

int32_t CloudServiceImpl::NotifyChange(const std::string& eventId, const std::string& extraData, int32_t userId)
{
    if (eventId != DATA_CHANGE_EVENT_ID || extraData.empty()) {
        return INVALID_ARGUMENT;
    }
    ExtraData exData;
    int32_t status = Convert(extraData, exData);
    if (status != E_OK) {
        return INVALID_ARGUMENT;
    }
    CloudInfo cloudInfo;
    if (userId != INVALID_USER_ID) {
        cloudInfo.user = userId;
        if (GetSchemaMetaInfo(exData, cloudInfo) != SUCCESS)
        {
            return INVALID_ARGUMENT;
        }
        return SUCCESS;
    } else {
        std::vector<int32_t> users;
        Account::GetInstance()->QueryUsers(users);
        if (users.empty()) {
            return SUCCESS;
        }
        for (auto user : users) {
            if (user == 0) {
                continue;
            } else {
                if (GetSchemaMetaInfo(exData, cloudInfo) != SUCCESS) {
                    return INVALID_ARGUMENT;
                }
                return SUCCESS;
            }
        }
    }
    return SUCCESS;
}

int32_t CloudServiceImpl::OnInitialize()
{
    DistributedDB::RuntimeConfig::SetCloudTranslate(std::make_shared<DistributedRdb::RdbCloudDataTranslate>());
    Execute(GenTask(0, 0, { WORK_CLOUD_INFO_UPDATE, WORK_SCHEMA_UPDATE, WORK_SUB }));
    return E_OK;
}

int32_t CloudServiceImpl::OnBind(const BindInfo &info)
{
    if (executor_ != nullptr || info.executors == nullptr) {
        return E_INVALID_ARGS;
    }

    executor_ = std::move(info.executors);
    syncManager_.Bind(executor_);
    return E_OK;
}

int32_t CloudServiceImpl::OnUserChange(uint32_t code, const std::string &user, const std::string &account)
{
    int32_t userId = atoi(user.c_str());
    if (code == static_cast<uint32_t>(DistributedKv::AccountStatus::DEVICE_ACCOUNT_SWITCHED)) {
        Execute(GenTask(0, userId, { WORK_CLOUD_INFO_UPDATE, WORK_SCHEMA_UPDATE, WORK_SUB }));
    }
    syncManager_.StopCloudSync(userId);
    return E_OK;
}

int32_t CloudServiceImpl::Online(const std::string &device)
{
    if (device != DeviceManagerAdapter::CLOUD_DEVICE_UUID) {
        ZLOGI("Not network online");
        return SUCCESS;
    }
    std::vector<int32_t> users;
    Account::GetInstance()->QueryUsers(users);
    if (users.empty()) {
        return SUCCESS;
    }
    auto it = users.begin();
    syncManager_.DoCloudSync({ *it });
    Execute(GenTask(0, *it, { WORK_CLOUD_INFO_UPDATE, WORK_SCHEMA_UPDATE, WORK_SUB }));
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

int32_t CloudServiceImpl::GetCloudInfo(uint32_t tokenId, const std::string &id, CloudInfo &cloudInfo)
{
    cloudInfo.user = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(tokenId);
    if (GetCloudInfoFromMeta(cloudInfo) != SUCCESS) {
        auto status = GetCloudInfoFromServer(cloudInfo);
        if (status != SUCCESS) {
            ZLOGE("user:%{public}d", cloudInfo.user);
            return status;
        }
    }
    if (cloudInfo.id != id) {
        ZLOGE("invalid args, [input] id:%{public}s, [exist] id:%{public}s", Anonymous::Change(id).c_str(),
            Anonymous::Change(cloudInfo.id).c_str());
        return INVALID_ARGUMENT;
    }
    return SUCCESS;
}

int32_t CloudServiceImpl::GetCloudInfoFromMeta(CloudInfo &cloudInfo)
{
    if (!MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        ZLOGE("no exist meta, user:%{public}d", cloudInfo.user);
        return ERROR;
    }
    return SUCCESS;
}

int32_t CloudServiceImpl::GetCloudInfoFromServer(CloudInfo &cloudInfo)
{
    auto instance = CloudServer::GetInstance();
    if (instance == nullptr) {
        return NOT_SUPPORT;
    }
    cloudInfo = instance->GetServerInfo(cloudInfo.user);
    if (!cloudInfo.IsValid()) {
        ZLOGE("cloud is empty, user%{public}d", cloudInfo.user);
        return ERROR;
    }
    return SUCCESS;
}

bool CloudServiceImpl::UpdateCloudInfo(int32_t user)
{
    CloudInfo cloudInfo;
    cloudInfo.user = user;
    if (GetCloudInfoFromServer(cloudInfo) != SUCCESS) {
        ZLOGE("failed, user:%{public}d", user);
        return false;
    }
    CloudInfo oldInfo;
    if (!MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetKey(), oldInfo, true)) {
        MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true);
        return true;
    }
    if (oldInfo.id != cloudInfo.id) {
        ZLOGE("different id, [server] id:%{public}s, [meta] id:%{public}s", Anonymous::Change(cloudInfo.id).c_str(),
            Anonymous::Change(oldInfo.id).c_str());
        std::map<std::string, int32_t> actions;
        for (auto &[bundle, app] : cloudInfo.apps) {
            actions[bundle] = GeneralStore::CleanMode::CLOUD_INFO;
        }
        DoClean(oldInfo, actions);
    }
    if (cloudInfo.enableCloud) {
        for (auto &[bundle, app] : cloudInfo.apps) {
            if (app.cloudSwitch && !oldInfo.apps[bundle].cloudSwitch) {
                syncManager_.DoCloudSync({ cloudInfo.user, bundle });
            }
        }
    }
    MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true);
    return true;
}

bool CloudServiceImpl::UpdateSchema(int32_t user)
{
    CloudInfo cloudInfo;
    cloudInfo.user = user;
    if (GetCloudInfoFromServer(cloudInfo) != SUCCESS) {
        ZLOGE("failed, user:%{public}d", user);
        return false;
    }
    auto keys = cloudInfo.GetSchemaKey();
    for (const auto &[bundle, key] : keys) {
        SchemaMeta schemaMeta;
        if (MetaDataManager::GetInstance().LoadMeta(key, schemaMeta, true)) {
            continue;
        }
        if (GetAppSchema(cloudInfo.user, bundle, schemaMeta) != SUCCESS) {
            return false;
        }
        MetaDataManager::GetInstance().SaveMeta(key, schemaMeta, true);
    }
    return true;
}

int32_t CloudServiceImpl::GetAppSchema(int32_t user, const std::string &bundleName, SchemaMeta &schemaMeta)
{
    auto instance = CloudServer::GetInstance();
    if (instance == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    schemaMeta = instance->GetAppSchema(user, bundleName);
    return SUCCESS;
}

ExecutorPool::Task CloudServiceImpl::GenTask(int32_t retry, int32_t user, Handles handles)
{
    return [this, retry, user, works = std::move(handles)]() mutable {
        auto executor = executor_;
        if (retry >= RETRY_TIMES || executor == nullptr || works.empty()) {
            return;
        }
        if (!DmAdapter::GetInstance().IsNetworkAvailable()) {
            return;
        }
        bool finished = true;
        std::vector<int32_t> users;
        if (user == 0) {
            auto account = Account::GetInstance();
            finished = !(account == nullptr) && account->QueryUsers(users);
        } else {
            users.push_back(user);
        }

        auto handle = works.front();
        for (auto user : users) {
            finished = (this->*handle)(user) && finished;
        }
        if (!finished || users.empty()) {
            executor->Schedule(std::chrono::seconds(RETRY_INTERVAL), GenTask(retry + 1, user, std::move(works)));
            return;
        }
        works.pop_front();
        if (!works.empty()) {
            executor->Execute(GenTask(retry, user, std::move(works)));
        }
    };
}

SchemaMeta CloudServiceImpl::GetSchemaMeta(int32_t userId, const std::string &bundleName, int32_t instanceId)
{
    SchemaMeta schemaMeta;
    auto [status, cloudInfo] = GetCloudInfo(userId);
    if (status != SUCCESS) {
        // GetCloudInfo has print the log info. so we don`t need print again.
        return schemaMeta;
    }

    if (!bundleName.empty() && !cloudInfo.Exist(bundleName, instanceId)) {
        ZLOGD("bundleName:%{public}s instanceId:%{public}d is not exist", bundleName.c_str(), instanceId);
        return schemaMeta;
    }
    std::string schemaKey = cloudInfo.GetSchemaKey(bundleName, instanceId);
    if (MetaDataManager::GetInstance().LoadMeta(schemaKey, schemaMeta, true)) {
        return schemaMeta;
    }

    auto instance = CloudServer::GetInstance();
    if (instance == nullptr) {
        return schemaMeta;
    }
    schemaMeta = instance->GetAppSchema(userId, bundleName);
    if (!schemaMeta.IsValid()) {
        ZLOGE("download schema from cloud failed, user:%{public}d, bundleName:%{public}s", userId, bundleName.c_str());
    }
    MetaDataManager::GetInstance().SaveMeta(schemaKey, schemaMeta, true);
    return schemaMeta;
}

std::pair<int32_t, CloudInfo> CloudServiceImpl::GetCloudInfo(int32_t userId)
{
    CloudInfo cloudInfo;
    cloudInfo.user = userId;
    if (MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        return { SUCCESS, cloudInfo };
    }
    auto instance = CloudServer::GetInstance();
    if (instance == nullptr) {
        return { SERVER_UNAVAILABLE, cloudInfo };
    }

    cloudInfo = instance->GetServerInfo(userId);
    if (!cloudInfo.IsValid()) {
        ZLOGE("no cloud info %{public}d", userId);
        return { ERROR, cloudInfo };
    }

    MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true);
    return { SUCCESS, cloudInfo };
}

int32_t CloudServiceImpl::CloudStatic::OnAppUninstall(
    const std::string &bundleName, int32_t user, int32_t index)
{
    MetaDataManager::GetInstance().DelMeta(Subscription::GetRelationKey(user, bundleName), true);
    MetaDataManager::GetInstance().DelMeta(CloudInfo::GetSchemaKey(user, bundleName, index), true);
    return E_OK;
}

void CloudServiceImpl::GetSchema(const Event &event)
{
    auto &rdbEvent = static_cast<const CloudEvent &>(event);
    auto &storeInfo = rdbEvent.GetStoreInfo();
    ZLOGD("Start GetSchema, bundleName:%{public}s, storeName:%{public}s, instanceId:%{public}d",
        storeInfo.bundleName.c_str(), Anonymous::Change(storeInfo.storeName).c_str(), storeInfo.instanceId);
    GetSchemaMeta(storeInfo.user, storeInfo.bundleName, storeInfo.instanceId);
}

bool CloudServiceImpl::ReleaseUserInfo(int32_t user)
{
    auto instance = CloudServer::GetInstance();
    if (instance == nullptr) {
        return true;
    }
    instance->ReleaseUserInfo(user);
    return true;
}

bool CloudServiceImpl::DoSubscribe(int32_t user)
{
    Subscription sub;
    sub.userId = user;
    MetaDataManager::GetInstance().LoadMeta(sub.GetKey(), sub, true);
    if (CloudServer::GetInstance() == nullptr) {
        ZLOGI("not support cloud server");
        return true;
    }

    CloudInfo cloudInfo;
    cloudInfo.user = sub.userId;
    auto exits = MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetKey(), cloudInfo, true);
    if (!exits) {
        ZLOGW("error, there is no cloud info for user(%{public}d)", sub.userId);
        return false;
    }
    if (!sub.id.empty() && sub.id != cloudInfo.id) {
        CleanSubscription(sub);
        sub.id.clear();
        sub.expiresTime.clear();
    }

    ZLOGD("begin cloud:%{public}d user:%{public}d apps:%{public}zu", cloudInfo.enableCloud, sub.userId,
        cloudInfo.apps.size());
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
        exits = MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(bundle), schemaMeta, true);
        if (exits) {
            dbs.insert_or_assign(bundle, std::move(schemaMeta.databases));
        }
    }

    ZLOGI("cloud switch:%{public}d user%{public}d, sub:%{public}zu, unsub:%{public}zu", cloudInfo.enableCloud,
        sub.userId, subDbs.size(), unsubDbs.size());
    ZLOGD("Subscribe user%{public}d details:%{public}s", sub.userId, Serializable::Marshall(subDbs).c_str());
    ZLOGD("Unsubscribe user%{public}d details:%{public}s", sub.userId, Serializable::Marshall(unsubDbs).c_str());
    CloudServer::GetInstance()->Subscribe(sub.userId, subDbs);
    CloudServer::GetInstance()->Unsubscribe(sub.userId, unsubDbs);
    return subDbs.empty() && unsubDbs.empty();
}

void CloudServiceImpl::CleanSubscription(Subscription &sub)
{
    ZLOGD("id:%{public}s, size:%{public}zu", Anonymous::Change(sub.id).c_str(), sub.expiresTime.size());
    MetaDataManager::GetInstance().DelMeta(sub.GetKey(), true);
    for (const auto &[bundle, expireTime] : sub.expiresTime) {
        MetaDataManager::GetInstance().DelMeta(sub.GetRelationKey(bundle), true);
    }
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

bool CloudServiceImpl::ExtraData::Marshal(Serializable::json& node) const
{
    SetValue(node[GET_NAME(header)], header);
    SetValue(node[GET_NAME(data)], data);
    return true;
}

bool CloudServiceImpl::ExtraData::Unmarshal(const Serializable::json& node)
{
    GetValue(node, GET_NAME(header), header);
    GetValue(node, GET_NAME(data), data);
    return extInfo.Unmarshall(data);
}

bool CloudServiceImpl::ExtraData::ExtInfo::Marshal(Serializable::json& node) const
{
    SetValue(node[GET_NAME(accountId)], accountId);
    SetValue(node[GET_NAME(bundleName)], bundleName);
    SetValue(node[GET_NAME(containerName)], containerName);
    SetValue(node[GET_NAME(recordTypes)], recordTypes);
    return true;
}

bool CloudServiceImpl::ExtraData::ExtInfo::Unmarshal(const Serializable::json& node)
{
    GetValue(node, GET_NAME(accountId), accountId);
    GetValue(node, GET_NAME(bundleName), bundleName);
    GetValue(node, GET_NAME(containerName), containerName);
    GetValue(node, GET_NAME(recordTypes), recordTypes);
    return table.Unmarshall(recordTypes);
}

bool CloudServiceImpl::ExtraData::Table::Marshal(Serializable::json& node) const
{
    return true;
}

bool CloudServiceImpl::ExtraData::Table::Unmarshal(const Serializable::json& node)
{
    GetValue(node, "", tables);
    return true;
}
} // namespace OHOS::CloudData