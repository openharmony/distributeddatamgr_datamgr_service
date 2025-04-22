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
#define LOG_TAG "DeviceSyncAppManager"
#include "device_sync_app/device_sync_app_manager.h"

#include "log_print.h"
#include "types.h"
#include "unistd.h"
namespace OHOS::DistributedData {
DeviceSyncAppManager::DeviceSyncAppManager()
{
}

DeviceSyncAppManager &DeviceSyncAppManager::GetInstance()
{
    static DeviceSyncAppManager instance;
    return instance;
}

void DeviceSyncAppManager::Initialize(const std::vector<WhiteList> &lists)
{
    for (const auto &list : lists) {
        whiteLists_.push_back(list);
    }
}

bool DeviceSyncAppManager::Check(const WhiteList &whiteList)
{
    for (auto &info : whiteLists_) {
        if (info.appId == whiteList.appId && (info.bundleName == whiteList.bundleName) &&
            (info.version == whiteList.version)) {
            return true;
        }
    }
    return false;
}

} // namespace OHOS::DistributedData