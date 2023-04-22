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
using namespace OHOS::AppDistributedKv;
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
    threadPool_ = KvStoreThreadPool::GetPool(POOL_SIZE, "DeviceMgr", true);
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
        auto resultInit = devManager.InitDeviceManager(PKG_NAME, dmInitCall);
        auto resultState = devManager.RegisterDevStateCallback(PKG_NAME, "", dmStateCall);
        if (resultInit == DM_OK && resultState == DM_OK) {
            return;
        }
        constexpr int32_t INTERVAL = 500;
        auto time = std::chrono::steady_clock::now() + std::chrono::milliseconds(INTERVAL);
        scheduler_.At(time, RegDevCallback());
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
    auto observers = GetObservers();
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
    auto time = std::chrono::steady_clock::now() + std::chrono::milliseconds(SYNC_TIMEOUT);
    scheduler_.At(time, [this, dvInfo]() { TimeOut(dvInfo.uuid); });
    syncTask_.Insert(dvInfo.uuid, dvInfo.uuid);
    for (const auto &item : observers) { // set compatible identify, sync service meta
        if (item == nullptr) {
            continue;
        }
        if (item->GetChangeLevelType() == ChangeLevelType::MIN) {
            item->OnDeviceChanged(dvInfo, DeviceChangeType::DEVICE_ONLINE);
        }
    }
}

void DeviceManagerAdapter::TimeOut(const std::string uuid)
{
    if (uuid.empty()) {
        ZLOGE("uuid empty!");
        return;
    }
    if (syncTask_.Contains(uuid)) {
        ZLOGI("[TimeOutReadyEvent] uuid:%{public}s", KvStoreUtils::ToBeAnonymous(uuid).c_str());
        std::string event = R"({"extra": {"deviceId":")" + uuid + R"(" } })";
        DeviceManager::GetInstance().NotifyEvent(PKG_NAME, DmNotifyEvent::DM_NOTIFY_EVENT_ONDEVICEREADY, event);
    }
    syncTask_.Erase(uuid);
}

void DeviceManagerAdapter::NotifyReadyEvent(const std::string &uuid)
{
    if (uuid.empty() || !syncTask_.Contains(uuid)) {
        return;
    }

    syncTask_.Erase(uuid);
    ZLOGI("[NotifyReadyEvent] uuid:%{public}s", KvStoreUtils::ToBeAnonymous(uuid).c_str());
    std::string event = R"({"extra": {"deviceId":")" + uuid + R"(" } })";
    DeviceManager::GetInstance().NotifyEvent(PKG_NAME, DmNotifyEvent::DM_NOTIFY_EVENT_ONDEVICEREADY, event);
}

std::vector<const AppDeviceChangeListener *> DeviceManagerAdapter::GetObservers()
{
    std::vector<const AppDeviceChangeListener *> observers;
    observers.resize(observers_.Size());
    observers_.ForEach([&observers](const auto &key, auto &value) {
        observers.emplace_back(value);
        return false;
    });
    return observers;
}

void DeviceManagerAdapter::Offline(const DmDeviceInfo &info)
{
    DeviceInfo dvInfo;
    if (!GetDeviceInfo(info, dvInfo)) {
        ZLOGE("get device info fail");
        return;
    }
    syncTask_.Erase(dvInfo.uuid);
    ZLOGI("[offline] uuid:%{public}s, name:%{public}s, type:%{public}d",
        KvStoreUtils::ToBeAnonymous(dvInfo.uuid).c_str(), dvInfo.deviceName.c_str(), dvInfo.deviceType);
    SaveDeviceInfo(dvInfo, DeviceChangeType::DEVICE_OFFLINE);
    KvStoreTask task([this, dvInfo]() {
        auto observers = GetObservers();
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
    KvStoreTask task([this, dvInfo]() {
        auto observers = GetObservers();
        for (const auto &item : observers) {
            if (item == nullptr) {
                continue;
            }
            item->OnDeviceChanged(dvInfo, DeviceChangeType::DEVICE_ONREADY);
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
    std::vector<DmDeviceInfo> dmInfos;
    auto ret = DeviceManager::GetInstance().GetTrustedDeviceList(PKG_NAME, "", dmInfos);
    if (ret != DM_OK) {
        ZLOGE("get trusted device:%{public}d", ret);
        return {};
    }

    std::vector<DeviceInfo> dvInfos;
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
        ZLOGE("invalid id:%{public}s", KvStoreUtils::ToBeAnonymous(id).c_str());
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
    std::vector<DeviceInfo> dvInfos = GetRemoteDevices();
    if (dvInfos.empty()) {
        ZLOGW("there is no trusted device!");
        return;
    }
    dvInfos.emplace_back(GetLocalDevice());
    for (const auto &info : dvInfos) {
        if (info.networkId.empty() || info.uuid.empty() || info.udid.empty()) {
            ZLOGE("networkId:%{public}s, uuid:%{public}d, udid:%{public}d",
                KvStoreUtils::ToBeAnonymous(info.networkId).c_str(), info.uuid.empty(), info.udid.empty());
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
        ZLOGE("failed, result:%{public}d, networkId:%{public}s", ret, KvStoreUtils::ToBeAnonymous(networkId).c_str());
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
        ZLOGE("failed, result:%{public}d, networkId:%{public}s", ret, KvStoreUtils::ToBeAnonymous(networkId).c_str());
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

std::string DeviceManagerAdapter::ToUDID(const std::string &id)
{
    return GetDeviceInfoFromCache(id).udid;
}

std::vector<std::string> DeviceManagerAdapter::ToUUID(const std::vector<std::string> &devices)
{
    std::vector<std::string> uuids;
    for (auto &device : devices) {
        auto uuid = DeviceManagerAdapter::GetInstance().ToUUID(device);
        if (uuid.empty()) {
            continue ;
        }
        uuids.push_back(std::move(uuid));
    }
    return uuids;
}

std::vector<std::string> DeviceManagerAdapter::ToUUID(std::vector<DeviceInfo> devices)
{
    std::vector<std::string> uuids;
    for (auto &device : devices) {
        if (device.uuid.empty()) {
            continue ;
        }
        uuids.push_back(std::move(device.uuid));
    }
    return uuids;
}

std::string DeviceManagerAdapter::ToNetworkID(const std::string &id)
{
    return GetDeviceInfoFromCache(id).networkId;
}

std::string DeviceManagerAdapter::CalcClientUuid(const std::string &appId, const std::string &uuid)
{
    if (uuid.empty()) {
        return "";
    }
    std::string encryptedUuid;
    auto ret = DeviceManager::GetInstance().GenerateEncryptedUuid(PKG_NAME, uuid, appId, encryptedUuid);
    if (ret != DM_OK) {
        ZLOGE("failed, result:%{public}d", ret);
        return "";
    }
    return encryptedUuid;
}
} // namespace OHOS::DistributedData
