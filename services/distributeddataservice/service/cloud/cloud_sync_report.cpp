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
#define LOG_TAG "CloudSyncReport"

#include "cloud_sync_report.h"
#include "communicator/device_manager_adapter.h"
#include "cloud/cloud_sync_status_event.h"
#include "eventcenter/event_center.h"
#include "log_print.h"

namespace OHOS::CloudData {
constexpr int32_t NUM_START = 1;
using namespace DistributedData;
CloudSyncReport &CloudSyncReport::GetInstance()
{
    static CloudSyncReport instance;
    return instance;
}

void CloudSyncReport::CloudSyncStart(const StoreInfo &storeInfo)
{
    bool postEvent = false;
    syncDbInfo_.Compute(storeInfo.bundleName, [&storeInfo, &postEvent](const std::string &key,
        std::map<std::string, int32_t> &value) {
        auto it = value.find(storeInfo.storeName);
        if (it == value.end()) {
            value.insert_or_assign(storeInfo.storeName, NUM_START);
            postEvent = true;
        } else {
            it->second++;
        }
        return true;
    });
    if (postEvent) {
        StoreInfo info = storeInfo;
        auto evt = std::make_unique<CloudSyncStatusEvent>(std::move(info), CloudSyncStatusEvent::SyncStatus::SATAR);
        EventCenter::GetInstance().PostEvent(std::move(evt));
    }
}

void CloudSyncReport::CloudSyncFinish(const StoreInfo &storeInfo)
{
    bool postEvent = false;
    syncDbInfo_.EraseIf([&storeInfo, &postEvent](const std::string &key, std::map<std::string, int32_t> &value) {
        if (key != storeInfo.bundleName) {
            return false;
        }
        auto it = value.find(storeInfo.storeName);
        if (it == value.end()) {
            return false;
        }
        it->second--;
        if (it->second == 0) {
            value.erase(storeInfo.storeName);
            postEvent = true;
        }
        return value.empty();
    });
    if (postEvent) {
        StoreInfo info = storeInfo;
        info.deviceId = DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
        auto evt = std::make_unique<CloudSyncStatusEvent>(std::move(info), CloudSyncStatusEvent::SyncStatus::FINISH);
        EventCenter::GetInstance().PostEvent(std::move(evt));
    }
}
} // OHOS::CloudData