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

#include "account/account_delegate.h"
#include "account_delegate.h"
#include "checker/checker_manager.h"
#include "cloud/cloud_event.h"
#include "cloud/cloud_server.h"
#include "cloud/subscription.h"
#include "communicator/device_manager_adapter.h"
#include "eventcenter/event_center.h"
#include "feature/feature_system.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "store/auto_cache.h"
#include "utils/anonymous.h"
namespace OHOS::CloudData {
using namespace DistributedData;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Account = OHOS::DistributedKv::AccountDelegate;
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
}

CloudServiceImpl::Factory::~Factory() {}

CloudServiceImpl::CloudServiceImpl()
{
    EventCenter::GetInstance().Subscribe(CloudEvent::FEATURE_INIT, [this](const Event &event) {
        FeatureInit(event);
        return;
    });

    EventCenter::GetInstance().Subscribe(CloudEvent::GET_SCHEMA, [this](const Event &event) {
        GetSchema(event);
        return;
    });
}

int32_t CloudServiceImpl::EnableCloud(const std::string &id, const std::map<std::string, int32_t> &switches)
{
    CloudInfo cloudInfo;
    if (GetCloudInfo(IPCSkeleton::GetCallingTokenID(), id, cloudInfo) != SUCCESS) {
        return INVALID_ARGUMENT;
    }
    cloudInfo.enableCloud = true;
    for (const auto &item : switches) {
        auto it = std::find_if(cloudInfo.apps.begin(), cloudInfo.apps.end(),
            [&item](const CloudInfo::AppInfo &appInfo) -> bool {
                return appInfo.bundleName == item.first;
            });
        if (it == cloudInfo.apps.end()) {
            continue;
        }
        it->cloudSwitch = item.second;
    }
    if (!MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        return ERROR;
    }
    return SUCCESS;
}

int32_t CloudServiceImpl::DisableCloud(const std::string &id)
{
    CloudInfo cloudInfo;
    if (GetCloudInfo(IPCSkeleton::GetCallingTokenID(), id, cloudInfo) != SUCCESS) {
        return INVALID_ARGUMENT;
    }
    cloudInfo.enableCloud = false;
    if (!MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        return ERROR;
    }
    return SUCCESS;
}

int32_t CloudServiceImpl::ChangeAppSwitch(const std::string &id, const std::string &bundleName, int32_t appSwitch)
{
    CloudInfo cloudInfo;
    if (GetCloudInfo(IPCSkeleton::GetCallingTokenID(), id, cloudInfo) != SUCCESS) {
        return INVALID_ARGUMENT;
    }
    auto it = std::find_if(cloudInfo.apps.begin(), cloudInfo.apps.end(),
        [&bundleName](const CloudInfo::AppInfo &appInfo) -> bool {
            return appInfo.bundleName == bundleName;
        });
    if (it == cloudInfo.apps.end()) {
        ZLOGE("bundleName:%{public}s", bundleName.c_str());
        return INVALID_ARGUMENT;
    }
    (*it).cloudSwitch = appSwitch;
    if (!MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        return ERROR;
    }
    return SUCCESS;
}

int32_t CloudServiceImpl::Clean(const std::string &id, const std::map<std::string, int32_t> &actions)
{
    CloudInfo cloudInfo;
    if (GetCloudInfo(IPCSkeleton::GetCallingTokenID(), id, cloudInfo) != SUCCESS) {
        return INVALID_ARGUMENT;
    }
    auto keys = cloudInfo.GetSchemaKey();
    for (const auto &action : actions) {
        if (!cloudInfo.IsExist(action.first)) {
            continue;
        }
        SchemaMeta schemaMeta;
        if (MetaDataManager::GetInstance().LoadMeta(keys[action.first], schemaMeta, true)) {
            // action
        }
    }
    return SUCCESS;
}

int32_t CloudServiceImpl::NotifyDataChange(const std::string &id, const std::string &bundleName)
{
    return 0;
}

int32_t CloudServiceImpl::OnInitialize()
{
    executor_->Execute(GetCloudTask(0, 0));
    return E_OK;
}

int32_t CloudServiceImpl::OnExecutor(std::shared_ptr<ExecutorPool> executor)
{
    if (executor_ != nullptr || executor == nullptr) {
        return E_INVALID_ARGS;
    }

    executor_ = std::move(executor);
    return E_OK;
}

int32_t CloudServiceImpl::OnUserChange(uint32_t code, const std::string &user, const std::string &account)
{
    executor_->Execute(GetCloudTask(0, atoi(user.c_str())));
    return E_OK;
}

int32_t CloudServiceImpl::GetCloudInfo(uint32_t tokenId, const std::string &id, CloudInfo &cloudInfo)
{
    cloudInfo.user = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(tokenId);
    if (!MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetKey(), cloudInfo, true) &&
        GetServerInfo(cloudInfo) != SUCCESS) {
        ZLOGE("invalid args, user:%{public}d", cloudInfo.user);
        return INVALID_ARGUMENT;
    }
    if (cloudInfo.id != id) {
        ZLOGE("invalid args, [input] id:%{public}s, [exist] id:%{public}s", Anonymous::Change(id).c_str(),
            Anonymous::Change(cloudInfo.id).c_str());
        return INVALID_ARGUMENT;
    }
    return SUCCESS;
}

int32_t CloudServiceImpl::GetServerInfo(CloudInfo &cloudInfo)
{
    auto instance = CloudServer::GetInstance();
    if (instance == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    cloudInfo = instance->GetServerInfo(cloudInfo.user);
    if (!cloudInfo.IsValid()) {
        return ERROR;
    }
    return SUCCESS;
}

void CloudServiceImpl::UpdateCloudInfo(CloudInfo &cloudInfo)
{
    CloudInfo oldInfo;
    if (!MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetKey(), oldInfo, true)) {
        MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true);
        return;
    }
    oldInfo.totalSpace = cloudInfo.totalSpace;
    oldInfo.remainSpace = cloudInfo.remainSpace;
    oldInfo.apps = std::move(cloudInfo.apps);
    cloudInfo = oldInfo;
    MetaDataManager::GetInstance().SaveMeta(oldInfo.GetKey(), oldInfo, true);
}

void CloudServiceImpl::AddSchema(CloudInfo &cloudInfo)
{
    auto keys = cloudInfo.GetSchemaKey();
    for (const auto &key : keys) {
        SchemaMeta schemaMeta;
        if (MetaDataManager::GetInstance().LoadMeta(key.second, schemaMeta, true)) {
            continue;
        }
        if (GetAppSchema(cloudInfo.user, key.first, schemaMeta) != SUCCESS) {
            continue;
        }
        MetaDataManager::GetInstance().SaveMeta(key.second, schemaMeta, true);
    }
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

ExecutorPool::Task CloudServiceImpl::GetCloudTask(int32_t retry, int32_t user)
{
    return [this, retry, user]() -> void {
        if (retry >= RETRY_TIMES) {
            return;
        }

        bool finished = true;
        std::vector<int32_t> users;
        if (user == 0) {
            finished = Account::GetInstance()->QueryUsers(users);
        } else {
            users.push_back(user);
        }

        for (auto user : users) {
            Subscription subscription;
            subscription.userId = user;
            MetaDataManager::GetInstance().LoadMeta(subscription.GetKey(), subscription, true);
            finished = Subscribe(subscription) && finished;
        }
        if (!finished) {
            executor_->Schedule(std::chrono::seconds(RETRY_INTERVAL), GetCloudTask(retry + 1, user));
        }
    };
}

SchemaMeta CloudServiceImpl::GetSchemaMata(int32_t userId, const std::string &bundleName, int32_t instanceId)
{
    SchemaMeta schemaMeta;
    auto instance = CloudServer::GetInstance();
    if (instance == nullptr) {
        ZLOGE("instance is nullptr");
        return schemaMeta;
    }
    auto cloudInfo = instance->GetServerInfo(userId);
    if (!cloudInfo.IsValid()) {
        ZLOGE("cloudInfo is invalid");
        return schemaMeta;
    }
    std::string schemaKey = cloudInfo.GetSchemaKey(bundleName, instanceId);
    if (!MetaDataManager::GetInstance().LoadMeta(schemaKey, schemaMeta, true)) {
        schemaMeta = instance->GetAppSchema(cloudInfo.user, bundleName);
        MetaDataManager::GetInstance().SaveMeta(schemaKey, schemaMeta, true);
    }
    return schemaMeta;
}

StoreMetaData CloudServiceImpl::GetStoreMata(int32_t userId, const std::string &bundleName,
    const std::string &storeName, int32_t instanceId)
{
    StoreMetaData storeMetaData;
    storeMetaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    storeMetaData.user = std::to_string(userId);
    storeMetaData.bundleName = bundleName;
    storeMetaData.storeId = storeName;
    storeMetaData.instanceId = instanceId;
    MetaDataManager::GetInstance().LoadMeta(storeMetaData.GetKey(), storeMetaData);
    return storeMetaData;
}

void CloudServiceImpl::FeatureInit(const Event &event)
{
    CloudInfo cloudInfo;
    std::vector<int> users;
    if (!DistributedKv::AccountDelegate::GetInstance()->QueryUsers(users) || users.empty()) {
        return;
    }
    cloudInfo.user = *users.begin();
    if (GetServerInfo(cloudInfo) != SUCCESS) {
        ZLOGE("failed, user:%{public}d", cloudInfo.user);
        return;
    }
    UpdateCloudInfo(cloudInfo);
    AddSchema(cloudInfo);
}

void CloudServiceImpl::GetSchema(const Event &event)
{
    auto &rdbEvent = static_cast<const CloudEvent &>(event);
    auto userId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(rdbEvent.GetStoreInfo().tokenId);
    auto schemaMeta = GetSchemaMata(userId, rdbEvent.GetStoreInfo().bundleName, rdbEvent.GetStoreInfo().instanceId);
    auto storeMeta = GetStoreMata(userId, rdbEvent.GetStoreInfo().bundleName, rdbEvent.GetStoreInfo().storeName,
        rdbEvent.GetStoreInfo().instanceId);

    AutoCache::Watchers watchers;
    auto store = AutoCache::GetInstance().GetStore(storeMeta, watchers, false);
    if (store == nullptr) {
        ZLOGE("store is nullptr");
        return;
    }
    store->SetSchema(schemaMeta);
    auto instance = CloudServer::GetInstance();
    if (instance == nullptr) {
        ZLOGE("instance is nullptr");
        return;
    }
    for (auto &database : schemaMeta.databases) {
        if (database.name != rdbEvent.GetStoreInfo().storeName /* || don't need sync */) {
            continue;
        }
        auto cloudDB = instance->ConnectCloudDB(rdbEvent.GetStoreInfo().tokenId, database);
        if (cloudDB != nullptr) {
            store->Bind(cloudDB);
        }
        // do sync
    }
    return;
}

bool CloudServiceImpl::Subscribe(const Subscription &subscription)
{
    if (CloudServer::GetInstance() == nullptr) {
        ZLOGI("not support cloud server");
        return true;
    }

    CloudInfo cloudInfo;
    cloudInfo.user = subscription.userId;
    auto exits = MetaDataManager::GetInstance().LoadMeta(cloudInfo.id, cloudInfo, true);
    if (!exits) {
        ZLOGW("error, there is no cloud info for user(%{public}d)", subscription.userId);
        return false;
    }

    ZLOGD("begin to subscribe user:%{public}d apps:%{public}zu", subscription.userId, cloudInfo.apps.size());
    bool finished = true;
    auto now = std::chrono::system_clock::now() + std::chrono::hours(7 * 24);
    std::map<std::string, std::vector<SchemaMeta::Database>> dbs;
    for (auto &app : cloudInfo.apps) {
        auto it = subscription.expiresTime.find(app.bundleName);
        if (it != subscription.expiresTime.end() && it->second <= now.time_since_epoch().count()) {
            continue;
        }

        SchemaMeta schemaMeta;
        exits = MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(app.bundleName), schemaMeta, true);
        if (exits) {
            dbs[it->first] = std::move(schemaMeta.databases);
            continue;
        }
        finished = false;
    }
    ZLOGI("Subscribe user%{public}d, size:%{public}zu", subscription.userId, dbs.size());
    ZLOGD("Subscribe user%{public}d, details:%{public}s", subscription.userId, Serializable::Marshall(dbs).c_str());
    if (!dbs.empty()) {
        auto ret = CloudServer::GetInstance()->Subscribe(subscription.userId, dbs);
        finished = (ret == E_OK) ? finished : false;
    }
    return finished;
}
} // namespace OHOS::CloudData