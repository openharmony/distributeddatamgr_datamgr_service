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

#define LOG_TAG "SyncedDeviceContainer"

#include "synced_device_container.h"
#include "log_print.h"

namespace OHOS {
namespace UDMF {

SyncedDeviceContainer &SyncedDeviceContainer::GetInstance()
{
    static SyncedDeviceContainer instance;
    return instance;
}

void SyncedDeviceContainer::SaveSyncedDeviceInfo(const std::string &deviceId)
{
    if (deviceId.empty()) {
        ZLOGE("DeviceId is empty");
        return;
    }
    std::vector<SyncedDeviceInfo> devices;
    auto current = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(pulledDeviceMutex_);
    pulledDeviceInfo_.erase(std::remove_if(pulledDeviceInfo_.begin(), pulledDeviceInfo_.end(),
        [&devices, &current](SyncedDeviceInfo &info) {
        if (info < current) {
            return true;
        }
        return false;
    }), pulledDeviceInfo_.end());
    SyncedDeviceInfo info;
    info.deviceId = deviceId;
    pulledDeviceInfo_.emplace_back(std::move(info));
}

std::vector<std::string> SyncedDeviceContainer::QueryDeviceInfo()
{
    std::vector<std::string> deviceIds;
    auto current = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(pulledDeviceMutex_);
    pulledDeviceInfo_.erase(std::remove_if(pulledDeviceInfo_.begin(), pulledDeviceInfo_.end(),
        [&deviceIds, &current](SyncedDeviceInfo &info) {
        if (info < current) {
            return true;
        }
        deviceIds.emplace_back(info.deviceId);
        return false;
    }), pulledDeviceInfo_.end());
    return deviceIds;
}
} // namespace UDMF
} // namespace OHOS