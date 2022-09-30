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
#include "feature_stub_impl.h"
namespace OHOS::DistributedData {
FeatureStubImpl::FeatureStubImpl(std::shared_ptr<FeatureSystem::Feature> feature)
    : featureImpl_(std::move(feature))
{
}

FeatureStubImpl::~FeatureStubImpl()
{
    featureImpl_ = nullptr;
}

int FeatureStubImpl::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (featureImpl_ == nullptr) {
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    return featureImpl_->OnRemoteRequest(code, data, reply);
}

int32_t FeatureStubImpl::OnInitialize()
{
    if (featureImpl_ == nullptr) {
        return -1;
    }
    return featureImpl_->OnInitialize();
}

int32_t FeatureStubImpl::OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &bundleName)
{
    if (featureImpl_ == nullptr) {
        return -1;
    }
    return featureImpl_->OnAppExit(uid, pid, tokenId, bundleName);
}

int32_t FeatureStubImpl::OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId)
{
    if (featureImpl_ == nullptr) {
        return -1;
    }
    return featureImpl_->OnAppUninstall(bundleName, user, index, tokenId);
}

int32_t FeatureStubImpl::ResolveAutoLaunch(const std::string &identifier, DistributedDB::AutoLaunchParam &param)
{
    if (featureImpl_ == nullptr) {
        return -1;
    }
    return featureImpl_->ResolveAutoLaunch(identifier, param);
}

int32_t FeatureStubImpl::OnUserChange(uint32_t code, const std::string &user, const std::string &account)
{
    if (featureImpl_ == nullptr) {
        return -1;
    }
    return featureImpl_->OnUserChange(code, user, account);
}

int32_t FeatureStubImpl::Online(const std::string &device)
{
    if (featureImpl_ == nullptr) {
        return -1;
    }
    return featureImpl_->Online(device);
}

int32_t FeatureStubImpl::Offline(const std::string &device)
{
    if (featureImpl_ == nullptr) {
        return -1;
    }
    return featureImpl_->Offline(device);
}

int32_t FeatureStubImpl::OnReady(const std::string &device)
{
    if (featureImpl_ == nullptr) {
        return -1;
    }
    return featureImpl_->OnReady(device);
}
}