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
#include "utils/anonymous.h"

namespace OHOS::DistributedKv {
using namespace OHOS::AppDistributedKv;
using namespace OHOS::AAFwk;
using namespace OHOS::AppExecFwk;
using namespace OHOS::DistributedData;
using namespace OHOS::EventFwk;
using BundleInfo = KvStoreDataService::BundleInfo;

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
        BundleEventInfo bundleEventInfo;
        bundleEventInfo.bundleName = want.GetElement().GetBundleName();
        bundleEventInfo.userId = want.GetIntParam(USER_ID, -1);
        bundleEventInfo.appIndex = want.GetIntParam(SANDBOX_APP_INDEX, 0);
        bundleEventInfo.tokenId = want.GetIntParam(TOKEN_ID, -1);
        int32_t newAppIndex = want.GetIntParam(APP_INDEX, 0);
        ZLOGI("bundleName:%{public}s, user:%{public}d, appIndex:%{public}d, newAppIndex:%{public}d",
            bundleEventInfo.bundleName.c_str(), bundleEventInfo.userId, bundleEventInfo.appIndex, newAppIndex);
        // appIndex's key in want is "appIndex", the value of the elder key "sandbox_app_index" is unsure,
        // to avoid effecting historical function, passing non-zero value to the function
        if (bundleEventInfo.appIndex == 0 && newAppIndex != 0) {
            bundleEventInfo.appIndex = newAppIndex;
        }
        (this->*(it->second))(bundleEventInfo);
    }
}

void InstallEventSubscriber::OnUninstall(const AppDistributedKv::BundleEventInfo &bundleEventInfo)
{
    if (kvStoreDataService_ == nullptr) {
        ZLOGW("kvStoreDataService is null. bundleName:%{public}s, userId:%{public}d, appIndex:%{public}d",
            bundleEventInfo.bundleName.c_str(), bundleEventInfo.userId,
            bundleEventInfo.appIndex);
        return;
    }
    kvStoreDataService_->OnUninstall(bundleEventInfo);
    std::string prefix = StoreMetaData::GetPrefix({DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid,
        std::to_string(bundleEventInfo.userId), "default", bundleEventInfo.bundleName});
    std::vector<StoreMetaData> storeMetaData;
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, storeMetaData, true)) {
        ZLOGE("load meta failed! bundleName:%{public}s, userId:%{public}d, appIndex:%{public}d",
            bundleEventInfo.bundleName.c_str(), bundleEventInfo.userId,
            bundleEventInfo.appIndex);
        return;
    }
    for (auto &meta : storeMetaData) {
        if (meta.instanceId == bundleEventInfo.appIndex && !meta.appId.empty() && !meta.storeId.empty()) {
            ZLOGI("storeMetaData uninstalled bundleName:%{public}s storeId:%{public}s, userId:%{public}d, "
                  "appIndex:%{public}d",
                bundleEventInfo.bundleName.c_str(), Anonymous::Change(meta.storeId).c_str(),
                bundleEventInfo.userId, bundleEventInfo.appIndex);
            MetaDataManager::GetInstance().DelMeta(meta.GetKeyWithoutPath());
            MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetKeyLocal(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetSecretKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetStrategyKey());
            MetaDataManager::GetInstance().DelMeta(meta.appId, true);
            MetaDataManager::GetInstance().DelMeta(meta.GetBackupSecretKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetAutoLaunchKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetDebugInfoKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetDfxInfoKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetCloneSecretKey(), true);
            MetaDataManager::GetInstance().DelMeta(StoreMetaMapping(meta).GetKey(), true);
            PermitDelegate::GetInstance().DelCache(meta.GetKeyWithoutPath());
        }
    }
}

void InstallEventSubscriber::OnUpdate(const AppDistributedKv::BundleEventInfo &bundleEventInfo)
{
    if (kvStoreDataService_ == nullptr) {
        ZLOGW("kvStoreDataService is null. bundleName:%{public}s, userId:%{public}d, appIndex:%{public}d",
            bundleEventInfo.bundleName.c_str(), bundleEventInfo.userId, bundleEventInfo.appIndex);
        return;
    }
    kvStoreDataService_->OnUpdate(bundleEventInfo);
}

void InstallEventSubscriber::OnInstall(const AppDistributedKv::BundleEventInfo &bundleEventInfo)
{
    if (kvStoreDataService_ == nullptr) {
        ZLOGW("kvStoreDataService is null. bundleName:%{public}s, userId:%{public}d, appIndex:%{public}d",
            bundleEventInfo.bundleName.c_str(), bundleEventInfo.userId, bundleEventInfo.appIndex);
        return;
    }
    kvStoreDataService_->OnInstall(bundleEventInfo);
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
    if (executors_ == nullptr) {
        ZLOGW("executors_ is null.");
        return Status::ERROR;
    }
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
        if (executors_ == nullptr) {
            ZLOGW("executors_ is null.");
            return;
        }
        executors_->Schedule(std::chrono::milliseconds(RETRY_INTERVAL), GetTask());
    };
}
} // namespace OHOS::DistributedKv