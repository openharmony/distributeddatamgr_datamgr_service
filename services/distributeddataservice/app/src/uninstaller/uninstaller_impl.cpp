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

#define LOG_TAG "UninstallerImpl"

#include "uninstaller_impl.h"
#include <thread>
#include <unistd.h>
#include "common_event_manager.h"
#include "common_event_support.h"
#include "communication_provider.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "permit_delegate.h"
#include "utils/anonymous.h"
#include "utils/block_integer.h"

namespace OHOS::DistributedKv {
using namespace OHOS::AppDistributedKv;
using namespace OHOS::AAFwk;
using namespace OHOS::AppExecFwk;
using namespace OHOS::DistributedData;
using namespace OHOS::EventFwk;

UninstallEventSubscriber::UninstallEventSubscriber(const CommonEventSubscribeInfo &info,
    UninstallEventCallback callback)
    : CommonEventSubscriber(info), callback_(callback)
{}

void UninstallEventSubscriber::OnReceiveEvent(const CommonEventData &event)
{
    ZLOGI("Intent Action Rec");
    Want want = event.GetWant();
    std::string action = want.GetAction();
    if (action != CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED &&
        action != CommonEventSupport::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED) {
        return;
    }

    std::string bundleName = want.GetElement().GetBundleName();
    int32_t userId = want.GetIntParam(USER_ID, -1);
    int32_t appIndex = want.GetIntParam(SANDBOX_APP_INDEX, 0);
    ZLOGI("bundleName:%s, user:%d, appIndex:%d", bundleName.c_str(), userId, appIndex);
    callback_(bundleName, userId, appIndex);
}

UninstallerImpl::~UninstallerImpl()
{
    ZLOGD("destruct");
    auto res = CommonEventManager::UnSubscribeCommonEvent(subscriber_);
    if (!res) {
        ZLOGW("unsubscribe fail res:%d", res);
    }
}

void UninstallerImpl::UnsubscribeEvent()
{
    auto res = CommonEventManager::UnSubscribeCommonEvent(subscriber_);
    if (!res) {
        ZLOGW("unsubscribe fail res:%d", res);
    }
}

Status UninstallerImpl::Init(KvStoreDataService *kvStoreDataService)
{
    if (kvStoreDataService == nullptr) {
        ZLOGW("kvStoreDataService is null.");
        return Status::INVALID_ARGUMENT;
    }
    MatchingSkills matchingSkills;
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_SANDBOX_PACKAGE_REMOVED);
    CommonEventSubscribeInfo info(matchingSkills);
    auto callback = [kvStoreDataService](const std::string &bundleName, int32_t userId, int32_t appIndex) {
        kvStoreDataService->OnUninstall(bundleName, userId, appIndex, IPCSkeleton::GetCallingTokenID());
        std::string prefix = StoreMetaData::GetPrefix({ CommunicationProvider::GetInstance().GetLocalDevice().uuid,
            std::to_string(userId), "default", bundleName });
        std::vector<StoreMetaData> storeMetaData;
        if (!MetaDataManager::GetInstance().LoadMeta(prefix, storeMetaData)) {
            ZLOGE("load meta failed!");
            return;
        }
        for (auto &meta : storeMetaData) {
            if (meta.instanceId == appIndex && !meta.appId.empty() && !meta.storeId.empty()) {
                ZLOGI("uninstalled bundleName:%s, stordId:%s", bundleName.c_str(),
                    Anonymous::Change(meta.storeId).c_str());
                MetaDataManager::GetInstance().DelMeta(meta.GetKey());
                MetaDataManager::GetInstance().DelMeta(meta.GetSecretKey(), true);
                MetaDataManager::GetInstance().DelMeta(meta.GetStrategyKey());
                MetaDataManager::GetInstance().DelMeta(meta.appId, true);
                MetaDataManager::GetInstance().DelMeta(meta.GetKeyLocal(), true);
                PermitDelegate::GetInstance().DelCache(meta.GetKey());
            }
        }
    };
    auto subscriber = std::make_shared<UninstallEventSubscriber>(info, callback);
    subscriber_ = subscriber;
    std::thread th = std::thread([subscriber] {
        constexpr int32_t RETRY_TIME = 300;
        constexpr int32_t RETRY_INTERVAL = 100 * 1000;
        for (BlockInteger retry(RETRY_INTERVAL); retry < RETRY_TIME; ++retry) {
            if (CommonEventManager::SubscribeCommonEvent(subscriber)) {
                ZLOGI("subscribe uninstall event success");
                break;
            }
            ZLOGE("subscribe uninstall event fail, try times:%d", static_cast<int>(retry));
        }
    });
    th.detach();
    return Status::SUCCESS;
}
} // namespace OHOS::DistributedKv
