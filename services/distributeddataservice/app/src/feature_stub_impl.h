/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_APP_FEATURE_STUB_IMPL_H
#define OHOS_DISTRIBUTED_DATA_APP_FEATURE_STUB_IMPL_H
#include "feature/feature_system.h"
#include "iremote_stub.h"
namespace OHOS::DistributedData {
class FeatureStub : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedData.ServiceProxy");
};

class FeatureStubImpl final : public IRemoteStub<FeatureStub> {
public:
    explicit FeatureStubImpl(std::shared_ptr<FeatureSystem::Feature> feature);
    ~FeatureStubImpl();
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
        MessageOption &option) override;
    int32_t OnInitialize();
    int32_t OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &bundleName);
    int32_t OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId);
    int32_t ResolveAutoLaunch(const std::string &identifier, DistributedDB::AutoLaunchParam &param);
    int32_t OnUserChange(uint32_t code, const std::string &user, const std::string &account);
    int32_t Online(const std::string &device);
    int32_t Offline(const std::string &device);
    int32_t OnReady(const std::string &device);
private:
    std::shared_ptr<FeatureSystem::Feature> featureImpl_;
};
}
#endif // OHOS_DISTRIBUTED_DATA_APP_FEATURE_STUB_IMPL_H
