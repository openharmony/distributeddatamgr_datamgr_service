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

#define LOG_TAG "AppIdMappingConfigManager"
#include "app_id_mapping/app_id_mapping_config_manager.h"

namespace OHOS::DistributedData {
AppIdMappingConfigManager &AppIdMappingConfigManager::GetInstance()
{
    static AppIdMappingConfigManager instance;
    return instance;
}

void AppIdMappingConfigManager::Initialize(const std::vector<AppMappingInfo> &mapper)
{
    for (const auto &info : mapper) {
        toDstMapper_.insert_or_assign(info.srcAppId, info.dstAppId);
    }
}

std::pair<std::string, std::string> AppIdMappingConfigManager::Convert(const std::string &appId,
    const std::string &accountId)
{
    auto it = toDstMapper_.find(appId);
    if (it == toDstMapper_.end()) {
        return std::make_pair(appId, accountId);
    }
    return std::make_pair(it->second, "default");
}

} // namespace OHOS::DistributedData