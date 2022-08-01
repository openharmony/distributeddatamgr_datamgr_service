/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#define LOG_TAG "DevManager"
#include "dev_manager.h"
#include <thread>
#include "log_print.h"
#include "device_manager_callback.h"
#include "dm_device_info.h"
#include "store_util.h"
namespace OHOS::DistributedKv {
using namespace OHOS::DistributedHardware;
constexpr int32_t DM_OK = 0;
constexpr int32_t DM_ERROR = -1;
constexpr size_t DevManager::MAX_ID_LEN;
constexpr const char *PKG_NAME = "ohos.distributeddata";
class KvDeviceStateCallback : public DeviceStateCallback {
public:
    void OnDeviceOnline(const DmDeviceInfo &deviceInfo) override;
    void OnDeviceOffline(const DmDeviceInfo &deviceInfo) override;
    void OnDeviceChanged(const DmDeviceInfo &deviceInfo) override;
    void OnDeviceReady(const DmDeviceInfo &deviceInfo) override;
};

void KvDeviceStateCallback::OnDeviceOnline(const DmDeviceInfo &deviceInfo)
{
    DevManager::GetInstance().Online(deviceInfo.networkId);
}

void KvDeviceStateCallback::OnDeviceOffline(const DmDeviceInfo &deviceInfo)
{
    DevManager::GetInstance().Offline(deviceInfo.networkId);
}

void KvDeviceStateCallback::OnDeviceChanged(const DmDeviceInfo &deviceInfo)
{
    DevManager::GetInstance().OnChanged(deviceInfo.networkId);
}

void KvDeviceStateCallback::OnDeviceReady(const DmDeviceInfo &deviceInfo)
{
}

class DmDeathCallback : public DmInitCallback {
public:
    void OnRemoteDied() override;
};

void DmDeathCallback::OnRemoteDied()
{
    ZLOGI("dm device manager died, init it again");
    DevManager::GetInstance().RegisterDevCallback();
}

DevManager::DevManager()
{
    RegisterDevCallback();
}

int32_t DevManager::Init()
{
    auto &deviceManager = DeviceManager::GetInstance();
    auto deviceInitCallback = std::make_shared<DmDeathCallback>();
    auto deviceCallback = std::make_shared<KvDeviceStateCallback>();
    int32_t errNo = deviceManager.InitDeviceManager(PKG_NAME, deviceInitCallback);
    if (errNo != DM_OK) {
        ZLOGE("register dm death recipient failed: %{public}d", errNo);
        return errNo;
    }
    errNo = deviceManager.RegisterDevStateCallback(PKG_NAME, "", deviceCallback);
    if (errNo != DM_OK) {
        ZLOGE("register device state callback fail: %{public}d", errNo);
    }
    return errNo;
}

void DevManager::RegisterDevCallback()
{
    int32_t errNo = Init();
    if (errNo != DM_OK) {
        ZLOGE("register device failed, try again");
    }
    std::thread th = std::thread([this]() {
        constexpr int RETRY_TIMES = 300;
        int i = 0;
        int32_t errNo = DM_ERROR;
        while (i++ < RETRY_TIMES) {
            errNo = Init();
            if (errNo == DM_OK) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        ZLOGI("reg device exit now: %{public}d times, errNo: %{public}d", i, errNo);
    });
    th.detach();
}

DevManager &DevManager::GetInstance()
{
    static DevManager instance;
    return instance;
}

std::string DevManager::ToUUID(const std::string &networkId) const
{
    DetailInfo deviceInfo;
    if (deviceInfos_.Get(networkId, deviceInfo)) {
        return deviceInfo.uuid;
    }

    std::string uuid;
    std::string udid;
    auto &deviceManager = DeviceManager::GetInstance();
    deviceManager.GetUuidByNetworkId(PKG_NAME, networkId, uuid);
    deviceManager.GetUdidByNetworkId(PKG_NAME, networkId, udid);
    if (uuid.empty() || udid.empty() || networkId.empty()) {
        return "";
    }
    deviceInfo = { uuid, std::move(udid), networkId, "", "" };
    deviceInfos_.Set(networkId, deviceInfo);
    deviceInfos_.Set(uuid, deviceInfo);
    return uuid;
}

std::string DevManager::ToNetworkId(const std::string &uuid) const
{
    DetailInfo deviceInfo;
    if (deviceInfos_.Get(uuid, deviceInfo)) {
        return deviceInfo.networkId;
    }
    auto infos = GetRemoteDevices();
    for (auto &info : infos) {
        if (info.uuid == uuid) {
            deviceInfos_.Set(info.uuid, info);
            deviceInfos_.Set(info.networkId, info);
            return info.networkId;
        }
    }

    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    return (localInfo_.uuid == uuid) ? localInfo_.networkId : "";
}

const DevManager::DetailInfo &DevManager::GetLocalDevice()
{
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    if (!localInfo_.uuid.empty()) {
        return localInfo_;
    }

    DmDeviceInfo info;
    auto &deviceManager = DeviceManager::GetInstance();
    int32_t ret = deviceManager.GetLocalDeviceInfo(PKG_NAME, info);
    if (ret != DM_OK) {
        ZLOGE("GetLocalNodeDeviceInfo error");
        return invalidDetail_;
    }
    std::string networkId = std::string(info.networkId);
    std::string uuid;
    deviceManager.GetUuidByNetworkId(PKG_NAME, networkId, uuid);
    std::string udid;
    deviceManager.GetUdidByNetworkId(PKG_NAME, networkId, udid);
    if (uuid.empty() || udid.empty() || networkId.empty()) {
        return invalidDetail_;
    }
    ZLOGI("[LocalDevice] id:%{public}s, name:%{public}s, type:%{public}d",
          StoreUtil::Anonymous(uuid).c_str(), info.deviceName, info.deviceTypeId);
    localInfo_ = { std::move(uuid), std::move(udid), std::move(networkId),
                   std::string(info.deviceName), std::string(info.deviceName) };
    return localInfo_;
}

std::vector<DevManager::DetailInfo> DevManager::GetRemoteDevices() const
{
    std::vector<DetailInfo> devices;
    std::vector<DmDeviceInfo> dmDeviceInfos {};
    auto &deviceManager = DeviceManager::GetInstance();
    int32_t ret = deviceManager.GetTrustedDeviceList(PKG_NAME, "", dmDeviceInfos);
    if (ret != DM_OK) {
        ZLOGE("GetTrustedDeviceList error");
        return devices;
    }

    for (const auto &dmDeviceInfo : dmDeviceInfos) {
        std::string networkId = dmDeviceInfo.networkId;
        std::string uuid;
        std::string udid;
        deviceManager.GetUuidByNetworkId(PKG_NAME, networkId, uuid);
        deviceManager.GetUdidByNetworkId(PKG_NAME, networkId, udid);
        DetailInfo deviceInfo = { std::move(uuid), std::move(udid), std::move(networkId),
                                  std::string(dmDeviceInfo.deviceName), std::to_string(dmDeviceInfo.deviceTypeId) };
        devices.push_back(std::move(deviceInfo));
    }
    return devices;
}

void DevManager::Online(const std::string &networkId)
{
    // do nothing
}

void DevManager::Offline(const std::string &networkId)
{
    DetailInfo deviceInfo;
    if (deviceInfos_.Get(networkId, deviceInfo)) {
        deviceInfos_.Delete(networkId);
        deviceInfos_.Delete(deviceInfo.uuid);
    }
}

void DevManager::OnChanged(const std::string &networkId)
{
    // do nothing
}
} // namespace OHOS::DistributedKv
