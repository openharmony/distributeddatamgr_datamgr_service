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

#include "kv_sync/kv_custom_dir_sync_apps_manager.h"
#include "log_print.h"
namespace OHOS {
namespace DistributedData {
KvCustomDirSyncAppsManager &KvCustomDirSyncAppsManager::GetInstance()
{
    static KvCustomDirSyncAppsManager instance;
    return instance;
}

void KvCustomDirSyncAppsManager::Initialize(const std::set<std::string> &bundleNames)
{
    syncApps_ = bundleNames;
}

bool KvCustomDirSyncAppsManager::IsAllowed(const std::string &bundleName) const
{
    return syncApps_.find(bundleName) != syncApps_.end();
}

} // namespace DistributedData
} // namespace OHOS
