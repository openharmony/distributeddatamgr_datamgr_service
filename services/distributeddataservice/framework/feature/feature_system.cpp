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

int32_t FeatureSystem::RegisterCreator(const std::string &name, Creator creator)
{
    creators_.InsertOrAssign(name, std::move(creator));
    return STUB_SUCCESS;
}

FeatureSystem::Creator FeatureSystem::GetCreator(const std::string &name)
{
    auto it = creators_.Find(name);
    if (it.first) {
        return it.second;
    }
    return nullptr;
}

FeatureSystem::Feature::~Feature()
{
}

int32_t FeatureSystem::Feature::OnInitialize()
{
    return STUB_SUCCESS;
}

int32_t FeatureSystem::Feature::OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &bundleName)
{
    return STUB_SUCCESS;
}

int32_t FeatureSystem::Feature::OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index,
    uint32_t tokenId)
{
    return STUB_SUCCESS;
}

int32_t FeatureSystem::Feature::ResolveAutoLaunch(const std::string &identifier, DistributedDB::AutoLaunchParam &param)
{
    return STUB_SUCCESS;
}

int32_t FeatureSystem::Feature::OnUserChange(uint32_t code, const std::string &user, const std::string &account)
{
    return STUB_SUCCESS;
}

int32_t FeatureSystem::Feature::Online(const std::string &device)
{
    return STUB_SUCCESS;
}

int32_t FeatureSystem::Feature::Offline(const std::string &device)
{
    return STUB_SUCCESS;
}

int32_t FeatureSystem::Feature::OnReady(const std::string &device)
{
    return STUB_SUCCESS;
}
}
}