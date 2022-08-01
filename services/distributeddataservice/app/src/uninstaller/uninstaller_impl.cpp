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
#include "device_kvstore_impl.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "log_print.h"

namespace OHOS::DistributedKv {
using namespace OHOS::AAFwk;
using namespace OHOS::AppExecFwk;
using namespace OHOS::DistributedData;
using namespace OHOS::EventFwk;
const std::string UninstallEventSubscriber::USER_ID = "userId";

UninstallEventSubscriber::UninstallEventSubscriber(const CommonEventSubscribeInfo &info,
    UninstallEventCallback callback)
    : CommonEventSubscriber(info), callback_(callback)
{}

void UninstallEventSubscriber::OnReceiveEvent(const CommonEventData &event)
{
    ZLOGI("Intent Action Rec");
    Want want = event.GetWant();
    std::string action = want.GetAction();
    if (action != CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED) {
        return;
    }

    std::string bundleName = want.GetElement().GetBundleName();
    int userId = want.GetIntParam(USER_ID, -1);
    ZLOGI("bundleName:%s, user:%d", bundleName.c_str(), userId);
    callback_(bundleName, userId);
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
    CommonEventSubscribeInfo info(matchingSkills);
    auto callback = [kvStoreDataService](const std::string &bundleName, int userId) {
        kvStoreDataService->DeleteObjectsByAppId(bundleName);
        std::string prefix = StoreMetaData::GetPrefix({ DeviceKvStoreImpl::GetLocalDeviceId(),
            std::to_string(userId), "default", bundleName });
        std::vector<StoreMetaData> storeMetaData;
        if (!MetaDataManager::GetInstance().LoadMeta(prefix, storeMetaData)) {
            ZLOGE("LoadKeys failed!");
            return;
        }
        for (auto &meta : storeMetaData) {
            if (!meta.appId.empty() && !meta.storeId.empty()) {
                ZLOGI("uninstalled bundleName:%s, stordId:%s", bundleName.c_str(), meta.storeId.c_str());
                kvStoreDataService->DeleteKvStore(meta);
            }
        }
    };
    subscriber_ = std::make_shared<UninstallEventSubscriber>(info, callback);
    std::thread th = std::thread([this] {
        int tryTimes = 0;
        constexpr int MAX_RETRY_TIME = 300;
        constexpr int RETRY_WAIT_TIME_S = 1;

        // we use this method to make sure regist success
        while (tryTimes < MAX_RETRY_TIME) {
            auto result = CommonEventManager::SubscribeCommonEvent(subscriber_);
            if (result) {
                ZLOGI("EventManager: Success");
                break;
            } else {
                ZLOGE("EventManager: Fail to Register Subscriber, error:%d", result);
                sleep(RETRY_WAIT_TIME_S);
            }
            tryTimes++;
        }
        if (MAX_RETRY_TIME == tryTimes) {
            ZLOGE("EventManager: Fail to Register Subscriber!!!");
        }
        ZLOGI("Register listener End!!");
    });
    th.detach();
    return Status::SUCCESS;
}
} // namespace OHOS::DistributedKv
