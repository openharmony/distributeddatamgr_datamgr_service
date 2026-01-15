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

#define LOG_TAG "DeviceManagerAdapter"

#include "device_manager_adapter.h"
#include "log_print.h"
#include "serializable/serializable.h"
#include "utils/anonymous.h"

namespace OHOS::DistributedData {
using namespace OHOS::AppDistributedKv;
struct DmDeviceInfo {
};
__attribute__((used)) static bool g_delegateInit =
    DeviceManagerDelegate::RegisterInstance(&DeviceManagerAdapter::GetInstance());

bool GetDeviceInfo(const DmDeviceInfo &dmInfo, DeviceInfo &dvInfo, std::string uuid, std::string udid)
{
    (void)dmInfo;
    (void)dvInfo;
    (void)uuid;
    (void)udid;
    return true;
}

DeviceManagerAdapter::DeviceManagerAdapter()
    : cloudDeviceInfo({ "", "", "cloudNetworkId", "cloudDeviceName", 0 })
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
    (void)executors;
    return;
}

std::function<void()> DeviceManagerAdapter::RegDevCallback()
{
    return []() {};
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

void DeviceManagerAdapter::Online(const DeviceInfo &info)
{
    (void)info;
    return;
}

void DeviceManagerAdapter::NotifyReadyEvent(const std::string &uuid)
{
    (void)uuid;
    return;
}

std::vector<const AppDeviceChangeListener *> DeviceManagerAdapter::GetObservers()
{
    return {};
}

void DeviceManagerAdapter::Offline(const DeviceInfo &info)
{
    (void)info;
    return;
}

void DeviceManagerAdapter::OnChanged(const DeviceInfo &info)
{
    (void)info;
    return;
}

void DeviceManagerAdapter::OnReady(const DeviceInfo &info)
{
    (void)info;
    return;
}

void DeviceManagerAdapter::SaveDeviceInfo(const DeviceInfo &dvInfo, const DeviceChangeType &type)
{
    (void)dvInfo;
    (void)type;
    return;
}

DeviceInfo DeviceManagerAdapter::GetLocalDevice()
{
    return {"localuuid", "localudid", "localnetworkId", "localdeviceName"};
}

std::vector<DeviceInfo> DeviceManagerAdapter::GetRemoteDevices()
{
    return {};
}

std::vector<DeviceInfo> DeviceManagerAdapter::GetOnlineDevices()
{
    return {};
}

bool DeviceManagerAdapter::IsDeviceReady(const std::string& id)
{
    (void)id;
    return true;
}

bool DeviceManagerAdapter::IsOHOSType(const std::string &id)
{
    (void)id;
    return true;
}

int32_t DeviceManagerAdapter::GetAuthType(const std::string &id)
{
    (void)id;
    return 0;
}

size_t DeviceManagerAdapter::GetOnlineSize()
{
    return 0;
}

DeviceInfo DeviceManagerAdapter::GetDeviceInfo(const std::string &id)
{
    (void)id;
    return {};
}

DeviceInfo DeviceManagerAdapter::GetDeviceInfoFromCache(const std::string &id)
{
    (void)id;
    return {};
}

void DeviceManagerAdapter::InitDeviceInfo(bool onlyCache)
{
    (void)onlyCache;
    return;
}

DeviceInfo DeviceManagerAdapter::GetLocalDeviceInfo()
{
    return {"localuuid", "localudid", "localnetworkId", "localdeviceName"};
}

std::string DeviceManagerAdapter::GetUuidByNetworkId(const std::string &networkId)
{
    (void)networkId;
    return "";
}

uint32_t DeviceManagerAdapter::GetDeviceTypeByUuid(const std::string &uuid)
{
    return 0;
}

std::string DeviceManagerAdapter::GetUdidByNetworkId(const std::string &networkId)
{
    (void)networkId;
    return "";
}

std::string DeviceManagerAdapter::ToUUID(const std::string &id)
{
    (void)id;
    return "";
}

std::string DeviceManagerAdapter::ToUDID(const std::string &id)
{
    (void)id;
    return "";
}

std::vector<std::string> DeviceManagerAdapter::ToUUID(const std::vector<std::string> &devices)
{
    (void)devices;
    return {};
}

std::vector<std::string> DeviceManagerAdapter::ToUUID(std::vector<DeviceInfo> devices)
{
    (void)devices;
    return {};
}

std::string DeviceManagerAdapter::ToNetworkID(const std::string &id)
{
    (void)id;
    return "";
}

std::string DeviceManagerAdapter::CalcClientUuid(const std::string &appId, const std::string &uuid)
{
    (void)appId;
    (void)uuid;
    return "";
}

std::string DeviceManagerAdapter::GetEncryptedUuidByNetworkId(const std::string &networkId)
{
    (void)networkId;
    return "";
}

bool DeviceManagerAdapter::IsSameAccount(const std::string &id)
{
    (void)id;
    return true;
}

bool DeviceManagerAdapter::CheckAccessControl(const AccessCaller &accCaller, const AccessCallee &accCallee)
{
    (void)accCaller;
    (void)accCallee;
    return true;
}

bool DeviceManagerAdapter::IsSameAccount(const AccessCaller &accCaller, const AccessCallee &accCallee)
{
    (void)accCaller;
    (void)accCallee;
    return true;
}

void DeviceManagerAdapter::ResetLocalDeviceInfo()
{
    return;
}
} // namespace OHOS::DistributedData