/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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

#define LOG_TAG "CloudConfigManager"
#include <mutex>
#include "cloud/cloud_config_manager.h"

namespace OHOS::DistributedData {
CloudConfigManager &CloudConfigManager::GetInstance()
{
    static CloudConfigManager instance;
    return instance;
}

void CloudConfigManager::Initialize(const std::vector<Info> &mapper)
{
    // Only called once during initialization, so it can be unlocked
    for (const auto &info : mapper) {
        toLocalMapper_.insert_or_assign(info.cloudBundle, info.localBundle);
        toCloudMapper_.insert_or_assign(info.localBundle, info.cloudBundle);
    }
}

std::string CloudConfigManager::ToLocal(const std::string &bundleName)
{
    auto it = toLocalMapper_.find(bundleName);
    return it == toLocalMapper_.end() ? bundleName : it->second;
}

std::string CloudConfigManager::ToCloud(const std::string &bundleName)
{
    auto it = toCloudMapper_.find(bundleName);
    return it == toCloudMapper_.end() ? bundleName : it->second;
}
} // namespace OHOS::DistributedData