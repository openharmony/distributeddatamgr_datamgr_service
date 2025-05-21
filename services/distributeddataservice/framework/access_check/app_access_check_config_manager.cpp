/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define LOG_TAG "AppAccessCheckConfigManager"
#include "access_check/app_access_check_config_manager.h"
#include "log_print.h"
#include "utils/anonymous.h"

namespace OHOS::DistributedData {
AppAccessCheckConfigManager &AppAccessCheckConfigManager::GetInstance()
{
    static AppAccessCheckConfigManager instance;
    return instance;
}

void AppAccessCheckConfigManager::Initialize(const std::vector<AppMappingInfo> &mapper)
{
    for (const auto &info : mapper) {
        appMapper_.insert_or_assign(info.bundleName, info.appId);
    }
}

bool AppAccessCheckConfigManager::IsTrust(const std::string &appId)
{
    auto it = appMapper_.find(appId);
    if (it != appMapper_.end() && (it->second == appId)) {
        return true;
    }
    ZLOGW("check access failed, appId:%{public}s", Anonymous::Change(appId).c_str());
    return false;
}

} // namespace OHOS::DistributedData