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
#include "checker/checker_manager.h"
#include "cloud/cloud_event.h"
#include "cloud/cloud_server.h"
#include "communicator/device_manager_adapter.h"
#include "eventcenter/event_center.h"
#include "feature/feature_system.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "utils/anonymous.h"
#include "store/auto_cache.h"

namespace OHOS::CloudData {
using namespace DistributedData;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
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
    FeatureInit();
    EventCenter::GetInstance().Subscribe(CloudEvent::GET_SCHEMA, [this](const Event &event) {
        GetSchema(event);
        return;
    });
}

int32_t CloudServiceImpl::EnableCloud(const std::string &id, const std::map<std::string, int32_t> &switches)
{
    CloudInfo cloudInfo;
    auto status = GetCloudInfo(IPCSkeleton::GetCallingTokenID(), id, cloudInfo);
    if (status != SUCCESS) {
        return status;
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
    auto status = GetCloudInfo(IPCSkeleton::GetCallingTokenID(), id, cloudInfo);
    if (status != SUCCESS) {
        return status;
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
    auto status = GetCloudInfo(IPCSkeleton::GetCallingTokenID(), id, cloudInfo);
    if (status != SUCCESS) {
        return status;
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
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    cloudInfo.user = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(tokenId);
    if (GetCloudInfoFromMeta(cloudInfo) != SUCCESS) {
        return ERROR;
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
    CloudInfo cloudInfo;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    cloudInfo.user = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(tokenId);
    if (GetCloudInfoFromMeta(cloudInfo) != SUCCESS) {
        return ERROR;
    }
    if (cloudInfo.id != id) {
        ZLOGE("invalid args, [input] id:%{public}s, [exist] id:%{public}s", Anonymous::Change(id).c_str(),
            Anonymous::Change(cloudInfo.id).c_str());
        return INVALID_ARGUMENT;
    }
    if (!cloudInfo.enableCloud) {
        return CLOUD_DISABLE;
    }
    auto it = std::find_if(cloudInfo.apps.begin(), cloudInfo.apps.end(),
        [&bundleName](const CloudInfo::AppInfo &appInfo) -> bool {
            return appInfo.bundleName == bundleName;
        });
    if (it == cloudInfo.apps.end()) {
        ZLOGE("bundleName:%{public}s", bundleName.c_str());
        return INVALID_ARGUMENT;
    }
    if (!it->cloudSwitch) {
        return CLOUD_DISABLE_SWITCH;
    }

    auto key = cloudInfo.GetSchemaKey(bundleName);
    SchemaMeta schemaMeta;
    if (!MetaDataManager::GetInstance().LoadMeta(key, schemaMeta, true)) {
        ZLOGE("bundleName:%{public}s", bundleName.c_str());
        return INVALID_ARGUMENT;
    }
    for (const auto &database : schemaMeta.databases) {
        EventCenter::Defer defer;
        CloudEvent::StoreInfo storeInfo;
        storeInfo.bundleName = it->bundleName;
        storeInfo.instanceId = it->instanceId;
        storeInfo.user = cloudInfo.user;
        storeInfo.storeName = database.name;
        auto evt = std::make_unique<CloudEvent>(CloudEvent::DATA_CHANGE, storeInfo);
        EventCenter::GetInstance().PostEvent(std::move(evt));
    }
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

void CloudServiceImpl::FeatureInit()
{
    CloudInfo cloudInfo;
    std::vector<int> users;
    if (!DistributedKv::AccountDelegate::GetInstance()->QueryUsers(users) || users.empty()) {
        return;
    }
    for (const auto &user : users) {
        if (user == USER_ID) {
            continue;
        }
        cloudInfo.user = user;
        if (GetCloudInfoFromServer(cloudInfo) != SUCCESS) {
            ZLOGE("failed, user:%{public}d", user);
            continue;
        }
        UpdateCloudInfo(cloudInfo);
        AddSchema(cloudInfo);
    }
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
} // namespace OHOS::CloudData