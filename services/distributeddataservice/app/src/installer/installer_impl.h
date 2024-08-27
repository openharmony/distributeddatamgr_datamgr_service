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

#ifndef DISTRIBUTEDDATAMGR_INSTALLER_IMPL_H
#define DISTRIBUTEDDATAMGR_INSTALLER_IMPL_H

#include "common_event_subscriber.h"
#include "kvstore_data_service.h"
#include "installer.h"

namespace OHOS::DistributedKv {
class InstallEventSubscriber : public EventFwk::CommonEventSubscriber {
public:
using InstallEventCallback = void (InstallEventSubscriber::*)
    (const std::string &bundleName, int32_t userId, int32_t appIndex);
    InstallEventSubscriber(const EventFwk::CommonEventSubscribeInfo &info, KvStoreDataService *kvStoreDataService);

    ~InstallEventSubscriber() {}
    void OnReceiveEvent(const EventFwk::CommonEventData &event) override;

private:
    static constexpr const char *USER_ID = "userId";
    static constexpr const char *SANDBOX_APP_INDEX = "sandbox_app_index";
    void OnUninstall(const std::string &bundleName, int32_t userId, int32_t appIndex);
    void OnUpdate(const std::string &bundleName, int32_t userId, int32_t appIndex);
    void OnInstall(const std::string &bundleName, int32_t userId, int32_t appIndex);
    std::map<std::string, InstallEventCallback> callbacks_;
    KvStoreDataService *kvStoreDataService_;
};

class InstallerImpl : public Installer {
public:
    InstallerImpl() = default;
    ~InstallerImpl();

    Status Init(KvStoreDataService *kvStoreDataService, std::shared_ptr<ExecutorPool> executors) override;

    void UnsubscribeEvent() override;

private:
    static constexpr int32_t RETRY_TIME = 300;
    static constexpr int32_t RETRY_INTERVAL = 100;
    int32_t retryTime_ = 0;
    ExecutorPool::Task GetTask();
    std::shared_ptr<InstallEventSubscriber> subscriber_ {};
    std::shared_ptr<ExecutorPool> executors_ {};
};
} // namespace OHOS::DistributedKv
#endif // DISTRIBUTEDDATAMGR_INSTALLER_IMPL_H
