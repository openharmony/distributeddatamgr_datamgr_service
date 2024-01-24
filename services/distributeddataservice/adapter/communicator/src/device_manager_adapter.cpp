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

#include "kvstore_utils.h"
#include "log_print.h"
#include "net_conn_callback_stub.h"
#include "net_conn_client.h"
#include "net_handle.h"

namespace OHOS::DistributedData {
using namespace OHOS::DistributedHardware;
using namespace OHOS::AppDistributedKv;
using namespace OHOS::NetManagerStandard;
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
    explicit DataMgrDmInitCall(DeviceManagerAdapter &dmAdapter, std::shared_ptr<ExecutorPool> executors)
        : dmAdapter_(dmAdapter), executors_(executors) {}
    void OnRemoteDied() override;

private:
    DeviceManagerAdapter &dmAdapter_;
    std::shared_ptr<ExecutorPool> executors_;
};

void DataMgrDmInitCall::OnRemoteDied()
{
    ZLOGI("device manager died, init again");
    dmAdapter_.Init(executors_);
}

class NetConnCallbackObserver : public NetConnCallbackStub {
public:
    explicit NetConnCallbackObserver(DeviceManagerAdapter &dmAdapter) : dmAdapter_(dmAdapter) {}
    ~NetConnCallbackObserver() override = default;
    int32_t NetAvailable(sptr<NetHandle> &netHandle) override;
    int32_t NetCapabilitiesChange(sptr<NetHandle> &netHandle, const sptr<NetAllCapabilities> &netAllCap) override;
    int32_t NetConnectionPropertiesChange(sptr<NetHandle> &netHandle, const sptr<NetLinkInfo> &info) override;
    int32_t NetLost(sptr<NetHandle> &netHandle) override;
    int32_t NetUnavailable() override;
    int32_t NetBlockStatusChange(sptr<NetHandle> &netHandle, bool blocked) override;

private:
    DeviceManagerAdapter &dmAdapter_;
};

int32_t NetConnCallbackObserver::NetAvailable(sptr<NetManagerStandard::NetHandle> &netHandle)
{
    ZLOGI("OnNetworkAvailable");
    dmAdapter_.SetNetAvailable(true);
    dmAdapter_.Online(dmAdapter_.cloudDmInfo);
    return DistributedKv::SUCCESS;
}

int32_t NetConnCallbackObserver::NetUnavailable()
{
    ZLOGI("OnNetworkUnavailable");
    dmAdapter_.SetNetAvailable(false);
    return DistributedKv::SUCCESS;
}

int32_t NetConnCallbackObserver::NetCapabilitiesChange(sptr<NetHandle> &netHandle,
    const sptr<NetAllCapabilities> &netAllCap)
{
    ZLOGI("OnNetCapabilitiesChange");
    return DistributedKv::SUCCESS;
}

int32_t NetConnCallbackObserver::NetConnectionPropertiesChange(sptr<NetHandle> &netHandle,
    const sptr<NetLinkInfo> &info)
{
    ZLOGI("OnNetConnectionPropertiesChange");
    return DistributedKv::SUCCESS;
}

int32_t NetConnCallbackObserver::NetLost(sptr<NetHandle> &netHandle)
{
    ZLOGI("OnNetLost");
    dmAdapter_.SetNetAvailable(false);
    dmAdapter_.Offline(dmAdapter_.cloudDmInfo);
    return DistributedKv::SUCCESS;
}

int32_t NetConnCallbackObserver::NetBlockStatusChange(sptr<NetHandle> &netHandle, bool blocked)
{
    ZLOGI("OnNetBlockStatusChange");
    return DistributedKv::SUCCESS;
}

DeviceManagerAdapter::DeviceManagerAdapter()
    : cloudDmInfo({ "cloudDeviceId", "cloudDeviceName", 0, "cloudNetworkId", 0 })
{
    ZLOGI("construct");
}

DeviceManagerAdapter::~DeviceManagerAdapter()
{
    ZLOGI("Destruct");
}

DeviceManagerAdapter &DeviceManagerAdapter::GetInstance()
{
    static DeviceManagerAdapter dmAdapter;
    return dmAdapter;
}

void DeviceManagerAdapter::Init(std::shared_ptr<ExecutorPool> executors)
{
    ZLOGI("begin");
    if (executors_ == nullptr) {
        executors_ = std::move(executors);
    }
    RegDevCallback()();
}

std::function<void()> DeviceManagerAdapter::RegDevCallback()
{
    return [this]() {
        auto &devManager = DeviceManager::GetInstance();
        auto dmStateCall = std::make_shared<DataMgrDmStateCall>(*this);
        auto dmInitCall = std::make_shared<DataMgrDmInitCall>(*this, executors_);
        auto resultInit = devManager.InitDeviceManager(PKG_NAME, dmInitCall);
        auto resultState = devManager.RegisterDevStateCallback(PKG_NAME, "", dmStateCall);
        auto resultNet = RegOnNetworkChange();
        if (resultInit == DM_OK && resultState == DM_OK && resultNet) {
            InitDeviceInfo(false);
            return;
        }
        constexpr int32_t INTERVAL = 500;
        executors_->Schedule(std::chrono::milliseconds(INTERVAL), RegDevCallback());
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
    syncTask_.Insert(dvInfo.uuid, dvInfo.uuid);
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

    executors_->Schedule(std::chrono::milliseconds(SYNC_TIMEOUT), [this, dvInfo]() {
        TimeOut(dvInfo.uuid);
    });

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
    if (syncTask_.Contains(uuid) && uuid != CLOUD_DEVICE_UUID) {
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
    observers.reserve(observers_.Size());
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
    auto task = [this, dvInfo]() {
        observers_.ForEachCopies([&dvInfo](const auto &key, auto &value) {
            if (value != nullptr) {
                value->OnDeviceChanged(dvInfo, DeviceChangeType::DEVICE_OFFLINE);
            }
            return false;
        });
    };
    executors_->Execute(std::move(task));
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
    auto task = [this, dvInfo]() {
        observers_.ForEachCopies([&dvInfo](const auto &key, auto &value) {
            if (value != nullptr) {
                value->OnDeviceChanged(dvInfo, DeviceChangeType::DEVICE_ONREADY);
            }
            return false;
        });
    };
    executors_->Execute(std::move(task));
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
        ZLOGW("uuid or udid empty");
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
            readyDevices_.InsertOrAssign(dvInfo.uuid, { DeviceState::DEVICE_ONLINE, dvInfo });
            break;
        }
        case DeviceChangeType::DEVICE_ONREADY: {
            readyDevices_.InsertOrAssign(dvInfo.uuid, { DeviceState::DEVICE_ONREADY, dvInfo });
            break;
        }
        case DeviceChangeType::DEVICE_OFFLINE: {
            deviceInfos_.Delete(dvInfo.networkId);
            deviceInfos_.Delete(dvInfo.uuid);
            deviceInfos_.Delete(dvInfo.udid);
            readyDevices_.Erase(dvInfo.uuid);
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
    if (!localInfo_.uuid.empty() && !localInfo_.udid.empty()) {
        return localInfo_;
    }
    localInfo_ = GetLocalDeviceInfo();
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

std::vector<DeviceInfo> DeviceManagerAdapter::GetOnlineDevices()
{
    std::vector<DeviceInfo> devices;
    devices.reserve(readyDevices_.Size());
    readyDevices_.ForEach([&devices](auto&, DeviceInfo& info) {
        devices.push_back(info.second);
        return false;
    });
    return devices;
}

bool DeviceManagerAdapter::IsDeviceReady(const std::string& id)
{
    auto it = readyDevices_.Find(id);
    return (it.first && it.second.first == DeviceState::DEVICE_OREADY);
}

size_t DeviceManagerAdapter::GetOnlineSize()
{
    return readyDevices_.Size();
}

DeviceInfo DeviceManagerAdapter::GetDeviceInfo(const std::string &id)
{
    return GetDeviceInfoFromCache(id);
}

DeviceInfo DeviceManagerAdapter::GetDeviceInfoFromCache(const std::string &id)
{
    DeviceInfo dvInfo;
    if (!deviceInfos_.Get(id, dvInfo)) {
        InitDeviceInfo();
        deviceInfos_.Get(id, dvInfo);
    }
    if (dvInfo.uuid.empty()) {
        ZLOGE("invalid id:%{public}s", KvStoreUtils::ToBeAnonymous(id).c_str());
    }
    return dvInfo;
}

void DeviceManagerAdapter::InitDeviceInfo(bool onlyCache)
{
    std::vector<DeviceInfo> dvInfos = GetRemoteDevices();
    if (dvInfos.empty()) {
        ZLOGW("there is no trusted device!");
    }
    if (!onlyCache) {
        readyDevices_.Clear();
    }
    for (const auto &info : dvInfos) {
        if (info.networkId.empty() || info.uuid.empty() || info.udid.empty()) {
            ZLOGE("networkId:%{public}s, uuid:%{public}d, udid:%{public}d",
                KvStoreUtils::ToBeAnonymous(info.networkId).c_str(), info.uuid.empty(), info.udid.empty());
            continue;
        }
        deviceInfos_.Set(info.networkId, info);
        deviceInfos_.Set(info.uuid, info);
        deviceInfos_.Set(info.udid, info);
        if (!onlyCache) {
            readyDevices_.InsertOrAssign(info.uuid, { DeviceState::DEVICE_ONREADY, info });
        }
    }
    auto local = GetLocalDeviceInfo();
    deviceInfos_.Set(local.networkId, local);
    deviceInfos_.Set(local.uuid, local);
    deviceInfos_.Set(local.udid, local);
}

DeviceInfo DeviceManagerAdapter::GetLocalDeviceInfo()
{
    DmDeviceInfo info;
    auto ret = DeviceManager::GetInstance().GetLocalDeviceInfo(PKG_NAME, info);
    if (ret != DM_OK) {
        ZLOGE("get local device info fail");
        return {};
    }
    auto networkId = std::string(info.networkId);
    auto uuid = GetUuidByNetworkId(networkId);
    auto udid = GetUdidByNetworkId(networkId);
    if (uuid.empty()) {
        return {};
    }
    ZLOGI("[LocalDevice] uuid:%{public}s, name:%{public}s, type:%{public}d", KvStoreUtils::ToBeAnonymous(uuid).c_str(),
        info.deviceName, info.deviceTypeId);
    return { std::move(uuid), std::move(udid), std::move(networkId), std::string(info.deviceName), info.deviceTypeId };
}

std::string DeviceManagerAdapter::GetUuidByNetworkId(const std::string &networkId)
{
    if (networkId.empty()) {
        return "";
    }
    if (networkId == DeviceManagerAdapter::cloudDmInfo.networkId) {
        return CLOUD_DEVICE_UUID;
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
    if (networkId == DeviceManagerAdapter::cloudDmInfo.networkId) {
        return CLOUD_DEVICE_UDID;
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
            continue;
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
            continue;
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

bool DeviceManagerAdapter::RegOnNetworkChange()
{
    sptr<NetConnCallbackObserver> observer = new (std::nothrow) NetConnCallbackObserver(*this);
    if (observer == nullptr) {
        ZLOGE("new operator error.observer is nullptr");
        return false;
    }
    int nRet = NetConnClient::GetInstance().RegisterNetConnCallback(observer);
    if (nRet != NETMANAGER_SUCCESS) {
        ZLOGE("RegisterNetConnCallback failed, ret = %{public}d", nRet);
        return false;
    }
    return true;
}

bool DeviceManagerAdapter::IsNetworkAvailable()
{
    {
        std::shared_lock<decltype(mutex_)> lock(mutex_);
        if (isNetAvailable_ || expireTime_ > std::chrono::steady_clock::now()) {
            return isNetAvailable_;
        }
    }
    NetHandle handle;
    auto status = NetConnClient::GetInstance().GetDefaultNet(handle);
    return SetNetAvailable(status == 0 && handle.GetNetId() != 0);
}

bool DeviceManagerAdapter::SetNetAvailable(bool isNetAvailable)
{
    std::unique_lock<decltype(mutex_)> lock(mutex_);
    isNetAvailable_ = isNetAvailable;
    expireTime_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(EFFECTIVE_DURATION);
    return isNetAvailable_;
}
} // namespace OHOS::DistributedData
