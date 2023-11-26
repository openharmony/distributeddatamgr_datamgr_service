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
#include <queue>
#include "cloud/cloud_event.h"
#include "cloud/cloud_info.h"
#include "cloud/schema_meta.h"
#include "cloud/subscription.h"
#include "cloud_service_stub.h"
#include "feature/static_acts.h"
#include "sync_manager.h"
#include "values_bucket.h"
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
    int32_t NotifyDataChange(const std::string& eventId, const std::string& extraData, int32_t userId) override;
    int32_t OnInitialize() override;
    int32_t OnBind(const BindInfo &info) override;
    int32_t OnUserChange(uint32_t code, const std::string &user, const std::string &account) override;
    int32_t Online(const std::string &device) override;
    int32_t Offline(const std::string &device) override;

    std::pair<int32_t, std::vector<NativeRdb::ValuesBucket>> AllocResourceAndShare(const std::string& storeId,
        const DistributedRdb::PredicatesMemo& predicates, const std::vector<std::string>& columns,
        const std::vector<Participant>& participants) override;

private:
    using StaticActs = DistributedData::StaticActs;
    class CloudStatic : public StaticActs {
    public:
        ~CloudStatic() override {};
        int32_t OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index) override;
    };
    class Factory {
    public:
        Factory() noexcept;
        ~Factory();
    private:
        std::shared_ptr<CloudServiceImpl> product_;
        std::shared_ptr<CloudStatic> staticActs_;
    };
    static Factory factory_;

    using CloudInfo = DistributedData::CloudInfo;
    using SchemaMeta = DistributedData::SchemaMeta;
    using Event = DistributedData::Event;
    using CloudEvent = DistributedData::CloudEvent;
    using Subscription = DistributedData::Subscription;
    using Handle = bool (CloudServiceImpl::*)(int32_t);
    using Handles = std::deque<Handle>;
    using Task = ExecutorPool::Task;

    struct ExtraData final : public DistributedData::Serializable {
        struct ExtInfo final : public Serializable {
            std::string accountId;
            std::string bundleName;
            std::string containerName;
            std::string recordTypes;
            std::vector<std::string> tables;
            bool Marshal(json &node) const override;
            bool Unmarshal(const json &node) override;
        };
        std::string header;
        std::string data;
        ExtInfo extInfo;
        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
    };

    static std::map<std::string, int32_t> ConvertAction(const std::map<std::string, int32_t> &actions);
    static std::pair<std::string, int32_t> GetHapInfo(uint32_t tokenId);

    static constexpr int32_t RETRY_TIMES = 3;
    static constexpr int32_t RETRY_INTERVAL = 60;
    static constexpr int32_t EXPIRE_INTERVAL = 2 * 24; // 2 day
    static constexpr int32_t WAIT_TIME = 30; // 30 seconds
    static constexpr int32_t DEFAULT_USER = 0;
    static constexpr const char *DATA_CHANGE_EVENT_ID = "cloud_data_change";

    bool UpdateCloudInfo(int32_t user);
    bool UpdateSchema(int32_t user);
    bool DoSubscribe(int32_t user);
    bool ReleaseUserInfo(int32_t user);
    bool DoCloudSync(int32_t user);
    bool StopCloudSync(int32_t user);

    std::pair<int32_t, CloudInfo> GetCloudInfo(int32_t userId);
    std::pair<int32_t, CloudInfo> GetCloudInfoFromMeta(int32_t userId);
    std::pair<int32_t, CloudInfo> GetCloudInfoFromServer(int32_t userId);

    std::pair<int32_t, SchemaMeta> GetSchemaMeta(int32_t userId, const std::string &bundleName, int32_t instanceId);
    std::pair<int32_t, SchemaMeta> GetAppSchemaFromServer(int32_t user, const std::string &bundleName);

    void GetSchema(const Event &event);
    void CloudShare(const Event &event);

    Task GenTask(int32_t retry, int32_t user, Handles handles = { WORK_SUB });
    void Execute(Task task);
    void CleanSubscription(Subscription &sub);
    int32_t DoClean(CloudInfo &cloudInfo, const std::map<std::string, int32_t> &actions);
    std::pair<int32_t, std::shared_ptr<DistributedData::Cursor>> PreShare(const CloudEvent::StoreInfo& storeInfo,
        DistributedData::GenQuery& query);
    std::vector<NativeRdb::ValuesBucket> Convert(std::shared_ptr<DistributedData::Cursor> cursor) const;
    int32_t CheckNotifyConditions(const std::string &id, const std::string &bundleName, CloudInfo &cloudInfo);
    int32_t GetDbInfoFromExtraData(const ExtraData &exData, int32_t userId, std::string &storeId,
                                   std::vector<std::string> &table);
    std::shared_ptr<ExecutorPool> executor_;
    SyncManager syncManager_;

    static constexpr Handle WORK_CLOUD_INFO_UPDATE = &CloudServiceImpl::UpdateCloudInfo;
    static constexpr Handle WORK_SCHEMA_UPDATE = &CloudServiceImpl::UpdateSchema;
    static constexpr Handle WORK_SUB = &CloudServiceImpl::DoSubscribe;
    static constexpr Handle WORK_RELEASE = &CloudServiceImpl::ReleaseUserInfo;
    static constexpr Handle WORK_DO_CLOUD_SYNC = &CloudServiceImpl::DoCloudSync;
    static constexpr Handle WORK_STOP_CLOUD_SYNC = &CloudServiceImpl::StopCloudSync;
};
} // namespace OHOS::DistributedData

#endif // OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_SERVICE_IMPL_H
