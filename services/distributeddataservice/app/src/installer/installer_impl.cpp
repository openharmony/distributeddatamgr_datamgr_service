/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#define LOG_TAG "InstallerImpl"

#include "installer_impl.h"
#include <thread>
#include <unistd.h>
#include "bundle_common_event.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "device_manager_adapter.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "permit_delegate.h"
#include "cloud/cloud_info.h"
#include "utils/anonymous.h"

namespace OHOS::DistributedKv {
using namespace OHOS::AppDistributedKv;
using namespace OHOS::AAFwk;
using namespace OHOS::AppExecFwk;
using namespace OHOS::DistributedData;
using namespace OHOS::EventFwk;

InstallEventSubscriber::InstallEventSubscriber(const CommonEventSubscribeInfo &info,
    KvStoreDataService *kvStoreDataService)
    : CommonEventSubscriber(info), kvStoreDataService_(kvStoreDataService)
{
    callbacks_ = { { CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED, &InstallEventSubscriber::OnUninstall },
        { OHOS::AppExecFwk::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED, &InstallEventSubscriber::OnUninstall },
        { CommonEventSupport::COMMON_EVENT_PACKAGE_CHANGED, &InstallEventSubscriber::OnUpdate },
        { CommonEventSupport::COMMON_EVENT_PACKAGE_ADDED, &InstallEventSubscriber::OnInstall },
        { OHOS::AppExecFwk::COMMON_EVENT_SANDBOX_PACKAGE_ADDED, &InstallEventSubscriber::OnInstall }};
}

void InstallEventSubscriber::OnReceiveEvent(const CommonEventData &event)
{
    ZLOGI("Action Rec");
    Want want = event.GetWant();
    std::string action = want.GetAction();
    auto it = callbacks_.find(action);
    if (it != callbacks_.end()) {
        std::string bundleName = want.GetElement().GetBundleName();
        int32_t userId = want.GetIntParam(USER_ID, -1);
        int32_t appIndex = want.GetIntParam(SANDBOX_APP_INDEX, 0);
        ZLOGI("bundleName:%{public}s, user:%{public}d, appIndex:%{public}d", bundleName.c_str(), userId, appIndex);
        (this->*(it->second))(bundleName, userId, appIndex);
    }
}

void InstallEventSubscriber::OnUninstall(const std::string &bundleName, int32_t userId, int32_t appIndex)
{
    kvStoreDataService_->OnUninstall(bundleName, userId, appIndex);
    std::string prefix = StoreMetaData::GetPrefix(
        { DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid, std::to_string(userId), "default", bundleName });
    std::vector<StoreMetaData> storeMetaData;
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, storeMetaData, true)) {
        ZLOGE("load meta failed! bundleName:%{public}s, userId:%{public}d, appIndex:%{public}d", bundleName.c_str(),
            userId, appIndex);
        return;
    }
    for (auto &meta : storeMetaData) {
        if (meta.instanceId == appIndex && !meta.appId.empty() && !meta.storeId.empty()) {
            ZLOGI("uninstalled bundleName:%{public}s storeId:%{public}s, userId:%{public}d, appIndex:%{public}d",
                bundleName.c_str(), Anonymous::Change(meta.storeId).c_str(), userId, appIndex);
            MetaDataManager::GetInstance().DelMeta(meta.GetKey());
            MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetSecretKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetStrategyKey());
            MetaDataManager::GetInstance().DelMeta(meta.appId, true);
            MetaDataManager::GetInstance().DelMeta(meta.GetKeyLocal(), true);
            PermitDelegate::GetInstance().DelCache(meta.GetKey());
        }
    }
}

void InstallEventSubscriber::OnUpdate(const std::string &bundleName, int32_t userId, int32_t appIndex)
{
    kvStoreDataService_->OnUpdate(bundleName, userId, appIndex);
    std::string prefix = StoreMetaData::GetPrefix(
        { DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid, std::to_string(userId), "default", bundleName });
    std::vector<StoreMetaData> storeMetaData;
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, storeMetaData, true)) {
        ZLOGE("load meta failed! bundleName:%{public}s, userId:%{public}d, appIndex:%{public}d", bundleName.c_str(),
            userId, appIndex);
        return;
    }
    for (auto &meta : storeMetaData) {
        if (meta.instanceId == appIndex && !meta.appId.empty() && !meta.storeId.empty()) {
            ZLOGI("updated bundleName:%{public}s, storeId:%{public}s, userId:%{public}d, appIndex:%{public}d",
                bundleName.c_str(), Anonymous::Change(meta.storeId).c_str(), userId, appIndex);
            MetaDataManager::GetInstance().DelMeta(CloudInfo::GetSchemaKey(meta), true);
        }
    }
}

void InstallEventSubscriber::OnInstall(const std::string &bundleName, int32_t userId, int32_t appIndex)
{
    kvStoreDataService_->OnInstall(bundleName, userId, appIndex);
}

InstallerImpl::~InstallerImpl()
{
    ZLOGD("destruct");
    auto res = CommonEventManager::UnSubscribeCommonEvent(subscriber_);
    if (!res) {
        ZLOGW("unsubscribe fail res:%d", res);
    }
}

void InstallerImpl::UnsubscribeEvent()
{
    auto res = CommonEventManager::UnSubscribeCommonEvent(subscriber_);
    if (!res) {
        ZLOGW("unsubscribe fail res:%d", res);
    }
}

Status InstallerImpl::Init(KvStoreDataService *kvStoreDataService, std::shared_ptr<ExecutorPool> executors)
{
    if (kvStoreDataService == nullptr) {
        ZLOGW("kvStoreDataService is null.");
        return Status::INVALID_ARGUMENT;
    }
    MatchingSkills matchingSkills;
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED);
    matchingSkills.AddEvent(OHOS::AppExecFwk::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_PACKAGE_CHANGED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_PACKAGE_ADDED);
    matchingSkills.AddEvent(OHOS::AppExecFwk::COMMON_EVENT_SANDBOX_PACKAGE_ADDED);
    CommonEventSubscribeInfo info(matchingSkills);

    auto subscriber = std::make_shared<InstallEventSubscriber>(info, kvStoreDataService);
    subscriber_ = subscriber;
    executors_ = executors;
    executors_->Execute(GetTask());
    return Status::SUCCESS;
}

ExecutorPool::Task InstallerImpl::GetTask()
{
    return [this] {
        auto succ = CommonEventManager::SubscribeCommonEvent(subscriber_);
        if (succ) {
            ZLOGI("subscribe install event success");
            return;
        }
        ZLOGE("subscribe common event fail, try times:%{public}d", retryTime_);
        if (retryTime_++ >= RETRY_TIME) {
            return;
        }
        executors_->Schedule(std::chrono::milliseconds(RETRY_INTERVAL), GetTask());
    };
}
} // namespace OHOS::DistributedKv
