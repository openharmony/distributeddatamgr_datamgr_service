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
#define LOG_TAG "DeviceManagerAdapter"
#include "device_manager_adapter.h"
#include <thread>
#include "log_print.h"
#include "kvstore_utils.h"

namespace OHOS::DistributedData {
using namespace OHOS::DistributedHardware;
using KvStoreUtils = OHOS::DistributedKv::KvStoreUtils;
constexpr int32_t DM_OK = 0;
constexpr const char *PKG_NAME = "ohos.distributeddata.service";
class DataMgrDmStateCall final : public DistributedHardware::DeviceStateCallback {
public:
    explicit DataMgrDmStateCall(DeviceManagerAdapter &dmAdapter) : dmAdapter_(dmAdapter) {}
    void OnDeviceOnline(const DmDeviceInfo &info) override;
    void OnDeviceOffline(const DmDeviceInfo &info) override;
    void OnDeviceChanged(const DmDeviceInfo &info) override;
    void OnDeviceReady(const DmDeviceInfo &info) override;

private:
    DeviceManagerAdapter &dmAdapter_;
};

void DataMgrDmStateCall::OnDeviceOnline(const DmDeviceInfo &info)
{
    dmAdapter_.Online(info);
}

void DataMgrDmStateCall::OnDeviceOffline(const DmDeviceInfo &info)
{
    dmAdapter_.Offline(info);
}

void DataMgrDmStateCall::OnDeviceChanged(const DmDeviceInfo &info)
{
    dmAdapter_.OnChanged(info);
}

void DataMgrDmStateCall::OnDeviceReady(const DmDeviceInfo &info)
{
    dmAdapter_.OnReady(info);
}

class DataMgrDmInitCall final : public DistributedHardware::DmInitCallback {
public:
    explicit DataMgrDmInitCall(DeviceManagerAdapter &dmAdapter) : dmAdapter_(dmAdapter) {}
    void OnRemoteDied() override;

private:
    DeviceManagerAdapter &dmAdapter_;
};

void DataMgrDmInitCall::OnRemoteDied()
{
    ZLOGI("device manager died, init again");
    dmAdapter_.Init();
}

DeviceManagerAdapter::DeviceManagerAdapter()
{
    ZLOGI("construct");
    threadPool_ = KvStoreThreadPool::GetPool(POOL_SIZE, true);
}

DeviceManagerAdapter::~DeviceManagerAdapter()
{
    ZLOGI("Destruct");
    if (threadPool_ != nullptr) {
        threadPool_->Stop();
        threadPool_ = nullptr;
    }
}

DeviceManagerAdapter &DeviceManagerAdapter::GetInstance()
{
    static DeviceManagerAdapter dmAdapter;
    return dmAdapter;
}

void DeviceManagerAdapter::Init()
{
    ZLOGI("begin");
    Execute(RegDevCallback());
}

std::function<void()> DeviceManagerAdapter::RegDevCallback()
{
    return [this]() {
        auto &devManager = DeviceManager::GetInstance();
        auto dmStateCall = std::make_shared<DataMgrDmStateCall>(*this);
        auto dmInitCall = std::make_shared<DataMgrDmInitCall>(*this);
        auto initResult = devManager.InitDeviceManager(PKG_NAME, dmInitCall);
        if (initResult != DM_OK) {
            constexpr int32_t INTERVAL = 500;
            auto time = std::chrono::system_clock::now() + std::chrono::milliseconds(INTERVAL);
            scheduler_.At(time, RegDevCallback());
            return;
        }
        devManager.RegisterDevStateCallback(PKG_NAME, "", dmStateCall);
    };
}

Status DeviceManagerAdapter::StartWatchDeviceChange(const AppDeviceChangeListener *observer,
    __attribute__((unused)) const PipeInfo &pipeInfo)
{
    if (observer == nullptr) {
        ZLOGE("observer is nullptr");
        return Status::INVALID_ARGUMENT;
    }
    if (!observers_.Insert(observer, observer)) {
        ZLOGE("insert observer fail");
        return Status::ERROR;
    }
    return Status::SUCCESS;
}

Status DeviceManagerAdapter::StopWatchDeviceChange(const AppDeviceChangeListener *observer,
    __attribute__((unused)) const PipeInfo &pipeInfo)
{
    if (observer == nullptr) {
        ZLOGE("observer is nullptr");
        return Status::INVALID_ARGUMENT;
    }
    if (!observers_.Erase(observer)) {
        ZLOGE("erase observer fail");
        return Status::ERROR;
    }
    return Status::SUCCESS;
}

void DeviceManagerAdapter::Online(const DmDeviceInfo &info)
{
    DeviceInfo dvInfo;
    if (!GetDeviceInfo(info, dvInfo)) {
        ZLOGE("get device info fail");
        return;
    }
    ZLOGI("[online] uuid:%{public}s, name:%{public}s, type:%{public}d",
        KvStoreUtils::ToBeAnonymous(dvInfo.uuid).c_str(), dvInfo.deviceName.c_str(), dvInfo.deviceType);
    SaveDeviceInfo(dvInfo, DeviceChangeType::DEVICE_ONLINE);
    std::vector<const AppDeviceChangeListener *> observers;
    observers.resize(observers_.Size());
    observers_.ForEach([&observers](const auto &key, auto &value) {
        observers.emplace_back(value);
        return false;
    });
    for (const auto &item : observers) { // notify db
        if (item == nullptr) {
            continue;
        }
        if (item->GetChangeLevelType() == ChangeLevelType::HIGH) {
            item->OnDeviceChanged(dvInfo, DeviceChangeType::DEVICE_OFFLINE);
            item->OnDeviceChanged(dvInfo, DeviceChangeType::DEVICE_ONLINE);
        }
    }
    for (const auto &item : observers) { // sync meta, get device security level
        if (item == nullptr) {
            continue;
        }
        if (item->GetChangeLevelType() == ChangeLevelType::LOW) {
            item->OnDeviceChanged(dvInfo, DeviceChangeType::DEVICE_ONLINE);
        }
    }
    for (const auto &item : observers) { // set compatible identify, sync service meta
        if (item == nullptr) {
            continue;
        }
        if (item->GetChangeLevelType() == ChangeLevelType::MIN) {
            item->OnDeviceChanged(dvInfo, DeviceChangeType::DEVICE_ONLINE);
        }
    }
    std::string event = R"({"extra": {"deviceId":")" + dvInfo.uuid + R"(" } })";
    DeviceManager::GetInstance().NotifyEvent(PKG_NAME, DmNotifyEvent::DM_NOTIFY_EVENT_ONDEVICEREADY, event);
}

void DeviceManagerAdapter::Offline(const DmDeviceInfo &info)
{
    DeviceInfo dvInfo;
    if (!GetDeviceInfo(info, dvInfo)) {
        ZLOGE("get device info fail");
        return;
    }
    ZLOGI("[offline] uuid:%{public}s, name:%{public}s, type:%{public}d",
        KvStoreUtils::ToBeAnonymous(dvInfo.uuid).c_str(), dvInfo.deviceName.c_str(), dvInfo.deviceType);
    SaveDeviceInfo(dvInfo, DeviceChangeType::DEVICE_OFFLINE);
    KvStoreTask task([this, &dvInfo]() {
        std::vector<const AppDeviceChangeListener *> observers;
        observers.resize(observers_.Size());
        observers_.ForEach([&observers](const auto &key, auto &value) {
            observers.emplace_back(value);
            return false;
        });
        for (const auto &item : observers) {
            if (item == nullptr) {
                continue;
            }
            item->OnDeviceChanged(dvInfo, DeviceChangeType::DEVICE_OFFLINE);
        }
    }, "deviceOffline");
    Execute(std::move(task));
}

void DeviceManagerAdapter::OnChanged(const DmDeviceInfo &info)
{
    DeviceInfo dvInfo;
    if (!GetDeviceInfo(info, dvInfo)) {
        ZLOGE("get device info fail");
        return;
    }
    ZLOGI("[OnChanged] uuid:%{public}s, name:%{public}s, type:%{public}d",
        KvStoreUtils::ToBeAnonymous(dvInfo.uuid).c_str(), dvInfo.deviceName.c_str(), dvInfo.deviceType);
}

void DeviceManagerAdapter::OnReady(const DmDeviceInfo &info)
{
    DeviceInfo dvInfo;
    if (!GetDeviceInfo(info, dvInfo)) {
        ZLOGE("get device info fail");
        return;
    }
    ZLOGI("[OnReady] uuid:%{public}s, name:%{public}s, type:%{public}d",
        KvStoreUtils::ToBeAnonymous(dvInfo.uuid).c_str(), dvInfo.deviceName.c_str(), dvInfo.deviceType);
    KvStoreTask task([this, &dvInfo]() {
        std::vector<const AppDeviceChangeListener *> observers;
        observers.resize(observers_.Size());
        observers_.ForEach([&observers](const auto &key, auto &value) {
            observers.emplace_back(value);
            return false;
        });
        for (const auto &item : observers) {
            if (item == nullptr) {
                continue;
            }
            if (item->GetChangeLevelType() == ChangeLevelType::READY) {
                item->OnDeviceChanged(dvInfo, DeviceChangeType::DEVICE_ONLINE);
            }
        }
    }, "deviceReady");
    Execute(std::move(task));
}

bool DeviceManagerAdapter::GetDeviceInfo(const DmDeviceInfo &dmInfo, DeviceInfo &dvInfo)
{
    std::string networkId = std::string(dmInfo.networkId);
    if (networkId.empty()) {
        return false;
    }
    auto uuid = GetUuidByNetworkId(networkId);
    auto udid = GetUdidByNetworkId(networkId);
    if (uuid.empty() || udid.empty()) {
        return false;
    }
    dvInfo = { uuid, udid, networkId, std::string(dmInfo.deviceName), dmInfo.deviceTypeId };
    return true;
}

void DeviceManagerAdapter::SaveDeviceInfo(const DeviceInfo &dvInfo, const DeviceChangeType &type)
{
    switch (type) {
        case DeviceChangeType::DEVICE_ONLINE: {
            deviceInfos_.Set(dvInfo.networkId, dvInfo);
            deviceInfos_.Set(dvInfo.uuid, dvInfo);
            deviceInfos_.Set(dvInfo.udid, dvInfo);
            break;
        }
        case DeviceChangeType::DEVICE_OFFLINE: {
            deviceInfos_.Delete(dvInfo.networkId);
            deviceInfos_.Delete(dvInfo.uuid);
            deviceInfos_.Delete(dvInfo.udid);
            break;
        }
        default: {
            ZLOGW("unknown type.");
            break;
        }
    }
}

DeviceInfo DeviceManagerAdapter::GetLocalDevice()
{
    std::lock_guard<decltype(devInfoMutex_)> lock(devInfoMutex_);
    if (!localInfo_.uuid.empty()) {
        return localInfo_;
    }

    DmDeviceInfo info;
    auto ret = DeviceManager::GetInstance().GetLocalDeviceInfo(PKG_NAME, info);
    if (ret != DM_OK) {
        ZLOGE("get local device info fail");
        return {};
    }
    auto networkId = std::string(info.networkId);
    auto uuid = GetUuidByNetworkId(networkId);
    auto udid = GetUdidByNetworkId(networkId);
    if (uuid.empty() || udid.empty()) {
        return {};
    }
    ZLOGI("[LocalDevice] uuid:%{public}s, name:%{public}s, type:%{public}d",
        KvStoreUtils::ToBeAnonymous(uuid).c_str(), info.deviceName, info.deviceTypeId);
    localInfo_ = { std::move(uuid), std::move(udid), std::move(networkId),
                   std::string(info.deviceName), info.deviceTypeId };
    return localInfo_;
}

std::vector<DeviceInfo> DeviceManagerAdapter::GetRemoteDevices()
{
    std::vector<DeviceInfo> dvInfos;
    std::vector<DmDeviceInfo> dmInfos;
    auto ret = DeviceManager::GetInstance().GetTrustedDeviceList(PKG_NAME, "", dmInfos);
    if (ret != DM_OK) {
        ZLOGE("get trust device list  fail");
        return {};
    }

    for (const auto &dmInfo : dmInfos) {
        auto networkId = std::string(dmInfo.networkId);
        auto uuid = GetUuidByNetworkId(networkId);
        auto udid = GetUdidByNetworkId(networkId);
        DeviceInfo dvInfo = { std::move(uuid), std::move(udid), std::move(networkId),
                              std::string(dmInfo.deviceName), dmInfo.deviceTypeId };
        dvInfos.emplace_back(std::move(dvInfo));
    }
    return dvInfos;
}

DeviceInfo DeviceManagerAdapter::GetDeviceInfo(const std::string &id)
{
    return GetDeviceInfoFromCache(id);
}

DeviceInfo DeviceManagerAdapter::GetDeviceInfoFromCache(const std::string &id)
{
    DeviceInfo dvInfo;
    if (!deviceInfos_.Get(id, dvInfo)) {
        UpdateDeviceInfo();
        deviceInfos_.Get(id, dvInfo);
    }
    if (dvInfo.uuid.empty()) {
        ZLOGE("invalid id");
    }
    return dvInfo;
}

bool DeviceManagerAdapter::Execute(KvStoreTask &&task)
{
    if (threadPool_ == nullptr) {
        return false;
    }
    threadPool_->AddTask(std::move(task));
    return true;
}

void DeviceManagerAdapter::UpdateDeviceInfo()
{
    std::vector<DeviceInfo> dvInfos;
    dvInfos = GetRemoteDevices();
    auto locInfo = GetLocalDevice();
    dvInfos.emplace_back(locInfo);
    for (const auto &info : dvInfos) {
        if (info.networkId.empty() || info.uuid.empty() || info.udid.empty()) {
            continue;
        }
        deviceInfos_.Set(info.networkId, info);
        deviceInfos_.Set(info.uuid, info);
        deviceInfos_.Set(info.udid, info);
    }
}

std::string DeviceManagerAdapter::GetUuidByNetworkId(const std::string &networkId)
{
    if (networkId.empty()) {
        return "";
    }
    DeviceInfo dvInfo;
    if (deviceInfos_.Get(networkId, dvInfo)) {
        return dvInfo.uuid;
    }
    std::string uuid;
    auto ret = DeviceManager::GetInstance().GetUuidByNetworkId(PKG_NAME, networkId, uuid);
    if (ret != DM_OK || uuid.empty()) {
        return "";
    }
    return uuid;
}

std::string DeviceManagerAdapter::GetUdidByNetworkId(const std::string &networkId)
{
    if (networkId.empty()) {
        return "";
    }
    DeviceInfo dvInfo;
    if (deviceInfos_.Get(networkId, dvInfo)) {
        return dvInfo.udid;
    }
    std::string udid;
    auto ret = DeviceManager::GetInstance().GetUdidByNetworkId(PKG_NAME, networkId, udid);
    if (ret != DM_OK || udid.empty()) {
        return "";
    }
    return udid;
}

DeviceInfo DeviceManagerAdapter::GetLocalBasicInfo()
{
    return GetLocalDevice();
}

std::string DeviceManagerAdapter::ToUUID(const std::string &id)
{
    return GetDeviceInfoFromCache(id).uuid;
}

std::string DeviceManagerAdapter::ToNetworkID(const std::string &id)
{
    return GetDeviceInfoFromCache(id).networkId;
}
} // namespace OHOS::DistributedData
