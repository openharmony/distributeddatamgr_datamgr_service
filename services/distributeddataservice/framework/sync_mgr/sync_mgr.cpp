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
#define LOG_TAG "SyncManager"
#include "sync_mgr/sync_mgr.h"

namespace OHOS::DistributedData {
SyncManager::SyncManager()
{
}

SyncManager &SyncManager::GetInstance()
{
    static SyncManager instance;
    return instance;
}

void SyncManager::Initialize(const std::vector<AutoSyncInfo> &autoSyncApps)
{
    for (const auto &app : autoSyncApps) {
        SetAutoSyncAppInfo(app);
    }
}

void SyncManager::SetAutoSyncAppInfo(const AutoSyncInfo &autoSyncApp)
{
    autoSyncApps_[autoSyncApp.bundleName] = autoSyncApp;
}

bool SyncManager::IsAutoSyncApp(const std::string &bundleName, const std::string &appId)
{
    auto it = autoSyncApps_.find(bundleName);
    if (it == autoSyncApps_.end()) {
        return false;
    }
    return it->second.appId == appId;
}

bool SyncManager::IsAutoSyncStore(const std::string &bundleName, const std::string &appId, const std::string &store)
{
    auto it = autoSyncApps_.find(bundleName);
    if (it == autoSyncApps_.end()) {
        return false;
    }
    if (it->second.appId != appId) {
        return false;
    }

    for (auto &storeId : it->second.storeIds) {
        if (storeId == store) {
            return true;
        }
    }
    return false;
}

bool SyncManager::NeedForceReplaceSchema(const AutoSyncInfo &autoSyncApp)
{
    auto it = autoSyncApps_.find(autoSyncApp.bundleName);
    if (it == autoSyncApps_.end()) {
        return false;
    }
    return ((it->second.version == autoSyncApp.version) && (it->second.appId == autoSyncApp.appId));
}
} // namespace OHOS::DistributedData