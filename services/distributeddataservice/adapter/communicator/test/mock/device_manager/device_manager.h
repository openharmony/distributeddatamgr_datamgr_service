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

#ifndef MOCK_DEVICE_MANAGER_H
#define MOCK_DEVICE_MANAGER_H

#include <vector>

#include "device_manager_callback.h"
#include "dm_device_info.h"

namespace OHOS::DistributedHardware {
class DeviceManager {
public:
    static DeviceManager &GetInstance();

    int32_t InitDeviceManager(const std::string &pkgName, std::shared_ptr<DmInitCallback> dmInitCallback);
    int32_t RegisterDevStateCallback(const std::string &pkgName, const std::string &extra,
        std::shared_ptr<DeviceStateCallback> callback);
    int32_t NotifyEvent(const std::string &pkgName, const int32_t eventId, const std::string &event);
    int32_t GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
        std::vector<DmDeviceInfo> &deviceList);
    int32_t GetLocalDeviceInfo(const std::string &pkgName, DmDeviceInfo &deviceInfo);
    int32_t GetUuidByNetworkId(const std::string &pkgName, const std::string &networkId, std::string &uuid);
    int32_t GetUdidByNetworkId(const std::string &pkgName, const std::string &networkId, std::string &udid);
    int32_t GenerateEncryptedUuid(const std::string &pkgName, const std::string &uuid, const std::string &appId,
        std::string &encryptedUuid);
    int32_t GetEncryptedUuidByNetworkId(const std::string &pkgName, const std::string &networkId, std::string &uuid);
    bool IsSameAccount(const std::string &networkId);
    bool CheckAccessControl(const DmAccessCaller &caller, const DmAccessCallee &callee);
    bool CheckIsSameAccount(const DmAccessCaller &caller, const DmAccessCallee &callee);
    void Online(const DmDeviceInfo &info);
    void Offline(const DmDeviceInfo &info);
    void OnChanged(const DmDeviceInfo &info);
    void OnReady(const DmDeviceInfo &info);

private:
    DeviceManager() = default;
    ~DeviceManager() = default;
    std::shared_ptr<DeviceStateCallback> callback_;
};
} // namespace OHOS::DistributedHardware
#endif // MOCK_DEVICE_MANAGER_H