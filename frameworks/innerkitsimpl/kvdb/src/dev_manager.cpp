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
#include "softbus_bus_center.h"
#include "log_print.h"
#include "store_util.h"
namespace OHOS::DistributedKv {
constexpr int32_t SOFTBUS_OK = 0;
constexpr int32_t ID_BUF_LEN = 65;
DevManager &DevManager::GetInstance()
{
    static DevManager instance;
    return instance;
}

std::string DevManager::ToUUID(const std::string &networkId) const
{
    if (networkId.empty()) {
        ZLOGE("networkId is empty, get uuid failed");
        return networkId;
    }
    DetailInfo deviceInfo;
    if (deviceInfos_.Get(networkId, deviceInfo)) {
        return deviceInfo.uuid;
    }

    std::string uuid = GetUuidByNetworkId(networkId);
    std::string udid = GetUdidByNetworkId(networkId);
    if (uuid.empty() && udid.empty()) {
        return uuid.empty() ? networkId : uuid;
    }
    deviceInfo = { uuid, std::move(udid), networkId, "", "" };
    deviceInfos_.Set(networkId, deviceInfo);
    return uuid;
}

const DevManager::DetailInfo &DevManager::GetLocalDevice()
{
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    if (!localInfo_.uuid.empty()) {
        return localInfo_;
    }

    NodeBasicInfo info;
    int32_t ret = GetLocalNodeDeviceInfo("ohos.distributeddata", &info);
    if (ret != SOFTBUS_OK) {
        ZLOGE("GetLocalNodeDeviceInfo error");
        return invalidDetail_;
    }
    std::string networkId = std::string(info.networkId);
    std::string uuid = GetUuidByNetworkId(networkId);
    std::string udid = GetUdidByNetworkId(networkId);
    if (uuid.empty() || udid.empty() || networkId.empty()) {
        return invalidDetail_;
    }
    ZLOGD("[LocalDevice] id:%{public}s, name:%{public}s, type:%{public}d",
          StoreUtil::Anonymous(uuid).c_str(), info.deviceName, info.deviceTypeId);
    localInfo_ = { std::move(uuid), std::move(udid), std::move(networkId),
                   std::string(info.deviceName), std::string(info.deviceName) };
    return localInfo_;
}

std::vector<DevManager::DetailInfo> DevManager::GetRemoteDevices() const
{
    std::vector<DetailInfo> devices;
    NodeBasicInfo *info = nullptr;
    int32_t infoNum = 0;

    int32_t ret = GetAllNodeDeviceInfo("ohos.distributeddata", &info, &infoNum);
    if (ret != SOFTBUS_OK) {
        ZLOGE("GetAllNodeDeviceInfo error");
        return devices;
    }
    ZLOGD("GetAllNodeDeviceInfo success infoNum=%{public}d", infoNum);

    for (int i = 0; i < infoNum; i++) {
        std::string networkId = std::string(info[i].networkId);
        std::string uuid = GetUuidByNetworkId(networkId);
        std::string udid = GetUdidByNetworkId(networkId);
        DetailInfo deviceInfo = { std::move(uuid), std::move(udid), std::move(networkId),
                                  std::string(info[i].deviceName), std::string(info[i].deviceName) };
        devices.push_back(std::move(deviceInfo));
    }
    if (info != nullptr) {
        FreeNodeInfo(info);
    }
    return devices;
}

std::string DevManager::GetUuidByNetworkId(const std::string &networkId) const
{
    char uuid[ID_BUF_LEN] = {0};
    int32_t ret = GetNodeKeyInfo("ohos.distributeddata", networkId.c_str(),
                                 NodeDeviceInfoKey::NODE_KEY_UUID, reinterpret_cast<uint8_t *>(uuid), ID_BUF_LEN);
    if (ret != SOFTBUS_OK) {
        ZLOGW("GetNodeKeyInfo error, nodeId:%{public}s", StoreUtil::Anonymous(networkId).c_str());
        return "";
    }
    return std::string(uuid);
}

std::string DevManager::GetUdidByNetworkId(const std::string &networkId) const
{
    char udid[ID_BUF_LEN] = {0};
    int32_t ret = GetNodeKeyInfo("ohos.distributeddata", networkId.c_str(),
                                 NodeDeviceInfoKey::NODE_KEY_UDID, reinterpret_cast<uint8_t *>(udid), ID_BUF_LEN);
    if (ret != SOFTBUS_OK) {
        ZLOGW("GetNodeKeyInfo error, nodeId:%{public}s", StoreUtil::Anonymous(networkId).c_str());
        return "";
    }
    return std::string(udid);
}
} // namespace OHOS::DistributedKv