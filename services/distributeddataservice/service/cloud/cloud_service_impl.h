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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_SERVICE_IMPL_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_SERVICE_IMPL_H

#include <mutex>
#include "cloud/cloud_event.h"
#include "cloud/cloud_info.h"
#include "cloud/schema_meta.h"
#include "cloud/subscription.h"
#include "cloud_service_stub.h"
#include "sync_manager.h"
namespace OHOS::CloudData {
class CloudServiceImpl : public CloudServiceStub {
public:
    using StoreMetaData = DistributedData::StoreMetaData;
    CloudServiceImpl();
    ~CloudServiceImpl() = default;
    int32_t EnableCloud(const std::string &id, const std::map<std::string, int32_t> &switches) override;
    int32_t DisableCloud(const std::string &id) override;
    int32_t ChangeAppSwitch(const std::string &id, const std::string &bundleName, int32_t appSwitch) override;
    int32_t Clean(const std::string &id, const std::map<std::string, int32_t> &actions) override;
    int32_t NotifyDataChange(const std::string &id, const std::string &bundleName) override;
    int32_t OnInitialize() override;
    int32_t OnBind(const BindInfo &info) override;
    int32_t OnUserChange(uint32_t code, const std::string &user, const std::string &account) override;
    int32_t OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId) override;

private:
    class Factory {
    public:
        Factory() noexcept;
        ~Factory();
    private:
        std::shared_ptr<CloudServiceImpl> product_;
    };
    static Factory factory_;

    using CloudInfo = DistributedData::CloudInfo;
    using SchemaMeta = DistributedData::SchemaMeta;
    using Event = DistributedData::Event;
    using Subscription = DistributedData::Subscription;
    using Work = bool (CloudServiceImpl::*)(int32_t);
    using Tasks = std::vector<ExecutorPool::Task>;

    enum AsyncWork : int32_t {
        WORK_SUB = 0,
        WORK_CLOUD_INFO_UPDATE,
        WORK_SCHEMA_UPDATE,
        WORK_BUTT,
    };

    static constexpr int32_t RETRY_TIMES = 10;
    static constexpr int32_t RETRY_INTERVAL = 30;
    static constexpr int32_t EXPIRE_INTERVAL = 7 * 24; // 7 day
    static constexpr int32_t TO_MS = 1000;

    bool UpdateCloudInfo(int32_t user);
    bool UpdateSchema(int32_t user);
    SchemaMeta GetSchemaMeta(int32_t userId, const std::string &bundleName, int32_t instanceId);
    CloudInfo GetCloudInfo(int32_t userId);
    int32_t GetCloudInfo(uint32_t tokenId, const std::string &id, CloudInfo &cloudInfo);
    int32_t GetCloudInfoFromMeta(CloudInfo &cloudInfo);
    int32_t GetCloudInfoFromServer(CloudInfo &cloudInfo);
    int32_t GetAppSchema(int32_t user, const std::string &bundleName, SchemaMeta &schemaMeta);
    void GetSchema(const Event &event);
    Tasks GetCloudTask(int32_t retry, int32_t user, const std::initializer_list<AsyncWork> &works = {});
    ExecutorPool::Task GenTask(int32_t retry, int32_t user, AsyncWork work);
    void Execute(Tasks tasks);
    bool DoSubscribe(int32_t user);
    std::shared_ptr<ExecutorPool> executor_;
    SyncManager syncManager_;
    static const Work HANDLERS[WORK_BUTT];
};
} // namespace OHOS::DistributedData

#endif // OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_SERVICE_IMPL_H
