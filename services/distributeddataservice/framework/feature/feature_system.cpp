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

#include "feature/feature_system.h"
namespace OHOS {
namespace DistributedData {
FeatureSystem &FeatureSystem::GetInstance()
{
    static FeatureSystem instance;
    return instance;
}

int32_t FeatureSystem::RegisterCreator(const std::string &name, Creator creator, int32_t flag)
{
    creators_.InsertOrAssign(name, std::pair{ std::move(creator), flag });
    return E_OK;
}

FeatureSystem::Creator FeatureSystem::GetCreator(const std::string &name)
{
    auto [success, pair] = creators_.Find(name);
    if (!success) {
        return nullptr;
    }
    auto [creator, flag] = std::move(pair);
    return creator;
}

int32_t FeatureSystem::RegisterStaticActs(const std::string &name, std::shared_ptr<StaticActs> staticActs)
{
    staticActs_.InsertOrAssign(name, std::move(staticActs));
    return E_OK;
}

const ConcurrentMap<std::string, std::shared_ptr<StaticActs>> &FeatureSystem::GetStaticActs()
{
    return staticActs_;
}

std::vector<std::string> FeatureSystem::GetFeatureName(int32_t flag)
{
    std::vector<std::string> features;
    creators_.ForEach([flag, &features](const std::string &key, auto &pair) -> bool {
        auto &[creator, bindFlag] = pair;
        if (bindFlag == flag) {
            features.push_back(key);
        }
        return false;
    });
    return features;
}

FeatureSystem::Feature::~Feature()
{
}

int32_t FeatureSystem::Feature::OnInitialize()
{
    return E_OK;
}

int32_t FeatureSystem::Feature::OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &bundleName)
{
    return E_OK;
}

int32_t FeatureSystem::Feature::OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index)
{
    return E_OK;
}

int32_t FeatureSystem::Feature::OnAppUpdate(const std::string &bundleName, int32_t user, int32_t index)
{
    return E_OK;
}

int32_t FeatureSystem::Feature::OnAppInstall(const std::string &bundleName, int32_t user, int32_t index)
{
    return E_OK;
}

int32_t FeatureSystem::Feature::ResolveAutoLaunch(const std::string &identifier, DistributedDB::AutoLaunchParam &param)
{
    return E_OK;
}

int32_t FeatureSystem::Feature::OnUserChange(uint32_t code, const std::string &user, const std::string &account)
{
    return E_OK;
}

int32_t FeatureSystem::Feature::Online(const std::string &device)
{
    return E_OK;
}

int32_t FeatureSystem::Feature::Offline(const std::string &device)
{
    return E_OK;
}

int32_t FeatureSystem::Feature::OnReady(const std::string &device)
{
    return E_OK;
}

int32_t FeatureSystem::Feature::OnSessionReady(const std::string &device)
{
    return E_OK;
}

int32_t FeatureSystem::Feature::OnBind(const FeatureSystem::Feature::BindInfo &bindInfo)
{
    return E_OK;
}
} // namespace DistributedData
} // namespace OHOS