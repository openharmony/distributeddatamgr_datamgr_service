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
constexpr uint32_t WAIT_TIME = 800;

SyncedDeviceContainer &SyncedDeviceContainer::GetInstance()
{
    static SyncedDeviceContainer instance;
    return instance;
}

void SyncedDeviceContainer::SaveSyncedDeviceInfo(const std::string &key, const std::string &deviceId)
{
    if (deviceId.empty()) {
        ZLOGE("DeviceId is empty");
        return;
    }
    if (!key.empty()) {
        std::lock_guard<std::mutex> lock(receivedDeviceMutex_);
        receivedDeviceInfo_.insert_or_assign(key, deviceId);
        return;
    }
    std::vector<SyncedDeiviceInfo> devices;
    auto current = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(pulledDeviceMutex_);
    for (const auto &info : delayDragDeviceInfo_) {
        if (info < current) {
            continue;
        }
        devices.emplace_back(std::move(info));
    }
    SyncedDeiviceInfo info;
    info.deviceId = deviceId;
    devices.emplace_back(std::move(info));
    delayDragDeviceInfo_.clear();
    delayDragDeviceInfo_ = std::move(devices);
}

std::vector<std::string> SyncedDeviceContainer::QueryDeviceInfo(const std::string &key)
{
    {
        std::lock_guard<std::mutex> lock(receivedDeviceMutex_);
        auto it = receivedDeviceInfo_.find(key);
        if (it != receivedDeviceInfo_.end()) {
            ZLOGI("Query deviceId from receivedDeviceInfo");
            std::string deviceId = it->second;
            receivedDeviceInfo_.erase(it);
            return { deviceId };
        }
    }
    std::vector<std::string> deviceIds;
    std::vector<SyncedDeiviceInfo> devices;
    {
        std::lock_guard<std::mutex> lock(pulledDeviceMutex_);
        devices = delayDragDeviceInfo_;
        delayDragDeviceInfo_.clear();
    }
    auto current = std::chrono::steady_clock::now();
    for (const auto &info : devices) {
        if (info < current) {
            continue;
        }
        deviceIds.emplace_back(info.deviceId);
    }
    return deviceIds;
}
} // namespace UDMF
} // namespace OHOS