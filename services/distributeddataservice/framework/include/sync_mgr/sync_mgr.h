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

#ifndef DISTRIBUTEDDATAMGR_DEVICE_SYNC_APP_MANAGER_H
#define DISTRIBUTEDDATAMGR_DEVICE_SYNC_APP_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include "visibility.h"
namespace OHOS::DistributedData {
class SyncManager {
public:
    struct AutoSyncInfo {
        uint32_t version = 0;
        std::string appId;
        std::string bundleName;
        std::vector<std::string> storeIds;
    };
    API_EXPORT static SyncManager &GetInstance();
    API_EXPORT void Initialize(const std::vector<AutoSyncInfo> &autoSyncApps);
    API_EXPORT void SetAutoSyncAppInfo(const AutoSyncInfo &autoSyncApp);
    API_EXPORT bool IsAutoSyncApp(const std::string &bundleName, const std::string &appId);
    API_EXPORT bool IsAutoSyncStore(const std::string &bundleName, const std::string &appId,
        const std::string &store);
    API_EXPORT bool NeedForceReplaceSchema(const AutoSyncInfo &autoSyncApp);

private:
    SyncManager();
    std::map<std::string, AutoSyncInfo> autoSyncApps_;
};
} // namespace OHOS::DistributedData
#endif // DISTRIBUTEDDATAMGR_DEVICE_SYNC_APP_MANAGER_H