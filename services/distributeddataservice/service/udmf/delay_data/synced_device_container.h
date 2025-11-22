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

#ifndef UDMF_SYNCED_DEVICE_CONTAINER_H
#define UDMF_SYNCED_DEVICE_CONTAINER_H

#include <map>
#include <mutex>
#include <string>

namespace OHOS {
namespace UDMF {
using Time = std::chrono::steady_clock::time_point;

class SyncedDeviceContainer {
public:
    static SyncedDeviceContainer &GetInstance();
    void SaveSyncedDeviceInfo(const std::string &deviceId);
    std::vector<std::string> QueryDeviceInfo();
private:
    SyncedDeviceContainer() = default;
    ~SyncedDeviceContainer() = default;
    SyncedDeviceContainer(const SyncedDeviceContainer &obj) = delete;
    SyncedDeviceContainer &operator=(const SyncedDeviceContainer &obj) = delete;

    struct SyncedDeiviceInfo {
        std::string deviceId;
        Time expiredTime = std::chrono::steady_clock::now() + std::chrono::hours(12);

        bool operator<(const Time &time) const
        {
            return expiredTime < time;
        }
    };

    std::mutex pulledDeviceMutex_;
    std::vector<SyncedDeiviceInfo> pulledDeviceInfo_ {};
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_SYNCED_DEVICE_CONTAINER_H