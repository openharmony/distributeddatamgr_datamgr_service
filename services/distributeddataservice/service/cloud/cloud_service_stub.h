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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_SERVICE_STUB_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_SERVICE_STUB_H
#include "cloud_service.h"
#include "feature/feature_system.h"
#include "iremote_broker.h"
namespace OHOS::CloudData {
class CloudServiceStub : public CloudService, public DistributedData::FeatureSystem::Feature {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.CloudData.CloudServer");
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply) override;

private:
    using Handler = int32_t (CloudServiceStub::*)(const std::string &id, MessageParcel &data, MessageParcel &reply);
    int32_t OnEnableCloud(const std::string &id, MessageParcel &data, MessageParcel &reply);
    int32_t OnDisableCloud(const std::string &id, MessageParcel &data, MessageParcel &reply);
    int32_t OnChangeAppSwitch(const std::string &id, MessageParcel &data, MessageParcel &reply);
    int32_t OnClean(const std::string &id, MessageParcel &data, MessageParcel &reply);
    int32_t OnNotifyDataChange(const std::string &id, MessageParcel &data, MessageParcel &reply);
    static const Handler HANDLERS[TRANS_BUTT];
};
} // namespace OHOS::CloudData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_SERVICE_STUB_H
