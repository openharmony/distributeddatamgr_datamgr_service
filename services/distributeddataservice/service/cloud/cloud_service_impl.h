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
#include "cloud_service_stub.h"
#include "cloud/cloud_info.h"
#include "cloud/schema_meta.h"
#include "cloud/cloud_event.h"

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

private:
    static const inline int USER_ID = 0;
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

    void UpdateCloudInfo(CloudInfo &cloudInfo);
    void AddSchema(CloudInfo &cloudInfo);
    SchemaMeta GetSchemaMata(int32_t userId, const std::string &bundleName, int32_t instanceId);
    StoreMetaData GetStoreMata(int32_t userId, const std::string &bundleName, const std::string &storeName,
        int32_t instanceId);
    int32_t GetCloudInfo(uint32_t tokenId, const std::string &id, CloudInfo &cloudInfo);
    int32_t GetCloudInfoFromMeta(CloudInfo &cloudInfo);
    int32_t GetCloudInfoFromServer(CloudInfo &cloudInfo);
    int32_t GetAppSchema(int32_t user, const std::string &bundleName, SchemaMeta &schemaMeta);
    void FeatureInit();
    void GetSchema(const Event &event);
};
} // namespace OHOS::DistributedData

#endif // OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_SERVICE_IMPL_H
