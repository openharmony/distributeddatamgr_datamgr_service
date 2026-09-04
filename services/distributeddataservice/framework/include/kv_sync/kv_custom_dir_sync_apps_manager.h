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
#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_KV_SYNC_KV_CUSTOM_DIR_SYNC_APPS_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_KV_SYNC_KV_CUSTOM_DIR_SYNC_APPS_MANAGER_H
#include <string>
#include <set>
#include "visibility.h"
namespace OHOS {
namespace DistributedData {
class KvCustomDirSyncAppsManager {
public:
    API_EXPORT static KvCustomDirSyncAppsManager &GetInstance();
    API_EXPORT void Initialize(const std::set<std::string> &bundleNames);
    API_EXPORT bool IsAllowed(const std::string &bundleName) const;

private:
    std::set<std::string> syncApps_;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_KV_SYNC_KV_CUSTOM_DIR_SYNC_APPS_MANAGER_H
