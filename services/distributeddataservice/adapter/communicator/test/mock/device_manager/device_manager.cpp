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

#include "device_manager.h"

#include "securec.h"

namespace OHOS::DistributedHardware {
static constexpr const char *LOCAL_DEVICE_NAME = "local_device_name";
static constexpr const char *LOCAL_NETWORK_ID = "local_network_id";
static constexpr const char *LOCAL_UUID = "local_uuid";
static constexpr const char *LOCAL_UDID = "local_udid";
static constexpr const char *REMOTE_DEVICE_NAME = "remote_device_name";
static constexpr const char *REMOTE_NETWORK_ID = "remote_network_id";
static constexpr const char *REMOTE_UUID = "remote_uuid";
static constexpr const char *REMOTE_UDID = "remote_udid";

DeviceManager &DeviceManager::GetInstance()
{
    static DeviceManager dmManager;
    return dmManager;
}

int32_t DeviceManager::InitDeviceManager(const std::string &pkgName, std::shared_ptr<DmInitCallback> dmInitCallback)
{
    (void)pkgName;
    (void)dmInitCallback;
    return 0;
}

int32_t DeviceManager::RegisterDevStateCallback(const std::string &pkgName, const std::string &extra,
    std::shared_ptr<DeviceStateCallback> callback)
{
    (void)pkgName;
    (void)extra;
    callback_ = callback;
    return 0;
}

int32_t DeviceManager::NotifyEvent(const std::string &pkgName, const int32_t eventId, const std::string &event)
{
    (void)pkgName;
    (void)eventId;
    (void)event;
    return 0;
}

int32_t DeviceManager::GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
    std::vector<DmDeviceInfo> &deviceList)
{
    (void)pkgName;
    (void)extra;
    DeviceExtraInfo extraInfo;
    DmDeviceInfo remoteDeviceInfo = { .extraData = DistributedData::Serializable::Marshall(extraInfo) };
    (void)strcpy_s(remoteDeviceInfo.deviceName, DM_MAX_DEVICE_ID_LEN, REMOTE_DEVICE_NAME);
    (void)strcpy_s(remoteDeviceInfo.networkId, DM_MAX_DEVICE_ID_LEN, REMOTE_NETWORK_ID);
    deviceList.emplace_back(std::move(remoteDeviceInfo));
    return 0;
}

int32_t DeviceManager::GetLocalDeviceInfo(const std::string &pkgName, DmDeviceInfo &deviceInfo)
{
    (void)pkgName;
    (void)strcpy_s(deviceInfo.deviceName, DM_MAX_DEVICE_ID_LEN, LOCAL_DEVICE_NAME);
    (void)strcpy_s(deviceInfo.networkId, DM_MAX_DEVICE_ID_LEN, LOCAL_NETWORK_ID);
    DeviceExtraInfo extraInfo;
    deviceInfo.extraData = DistributedData::Serializable::Marshall(extraInfo);
    return 0;
}

int32_t DeviceManager::GetUuidByNetworkId(const std::string &pkgName, const std::string &networkId, std::string &uuid)
{
    (void)pkgName;
    if (networkId == LOCAL_NETWORK_ID) {
        uuid = LOCAL_UUID;
    }
    if (networkId == REMOTE_NETWORK_ID) {
        uuid = REMOTE_UUID;
    }
    return 0;
}

int32_t DeviceManager::GetUdidByNetworkId(const std::string &pkgName, const std::string &networkId, std::string &udid)
{
    (void)pkgName;
    if (networkId == LOCAL_NETWORK_ID) {
        udid = LOCAL_UDID;
    }
    if (networkId == REMOTE_NETWORK_ID) {
        udid = REMOTE_UDID;
    }
    return 0;
}

int32_t DeviceManager::GenerateEncryptedUuid(const std::string &pkgName, const std::string &uuid,
    const std::string &appId, std::string &encryptedUuid)
{
    (void)pkgName;
    (void)uuid;
    (void)appId;
    (void)encryptedUuid;
    return 0;
}

int32_t DeviceManager::GetEncryptedUuidByNetworkId(const std::string &pkgName, const std::string &networkId,
    std::string &uuid)
{
    (void)pkgName;
    (void)networkId;
    (void)uuid;
    return 0;
}

bool DeviceManager::IsSameAccount(const std::string &networkId)
{
    (void)networkId;
    return true;
}

bool DeviceManager::CheckAccessControl(const DmAccessCaller &caller, const DmAccessCallee &callee)
{
    (void)caller;
    (void)callee;
    return true;
}

bool DeviceManager::CheckIsSameAccount(const DmAccessCaller &caller, const DmAccessCallee &callee)
{
    (void)caller;
    (void)callee;
    return true;
}

void DeviceManager::Online(const DmDeviceInfo &info)
{
    if (callback_) {
        callback_->OnDeviceOnline(info);
    }
}
 
void DeviceManager::Offline(const DmDeviceInfo &info)
{
    if (callback_) {
        callback_->OnDeviceOffline(info);
    }
}
 
void DeviceManager::OnChanged(const DmDeviceInfo &info)
{
    if (callback_) {
        callback_->OnDeviceChanged(info);
    }
}
 
void DeviceManager::OnReady(const DmDeviceInfo &info)
{
    if (callback_) {
        callback_->OnDeviceReady(info);
    }
}
} // namespace OHOS::DistributedHardware