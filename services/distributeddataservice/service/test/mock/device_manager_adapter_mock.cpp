/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "device_manager_adapter_mock.h"

namespace OHOS::DistributedData {
OHOS::DistributedData::DeviceManagerAdapter &OHOS::DistributedData::DeviceManagerAdapter::GetInstance()
{
    static DeviceManagerAdapter dmAdapter;
    return dmAdapter;
}

OHOS::DistributedData::DeviceManagerAdapter::DeviceManagerAdapter()
    : cloudDmInfo({ "cloudDeviceId", "cloudDeviceName", 0, "cloudNetworkId", 0 })
{}

std::vector<std::string> OHOS::DistributedData::DeviceManagerAdapter::ToUUID(std::vector<DeviceInfo> devices)
{
    if (BDeviceManagerAdapter::deviceManagerAdapter == nullptr) {
        std::vector<std::string> vec;
        return vec;
    }
    return BDeviceManagerAdapter::deviceManagerAdapter->ToUUID(devices);
}

std::vector<std::string> OHOS::DistributedData::DeviceManagerAdapter::ToUUID(const std::vector<std::string> &devices)
{
    if (BDeviceManagerAdapter::deviceManagerAdapter == nullptr) {
        std::vector<std::string> vec;
        return vec;
    }
    return BDeviceManagerAdapter::deviceManagerAdapter->ToUUID(devices);
}

std::string OHOS::DistributedData::DeviceManagerAdapter::ToUUID(const std::string &id)
{
    if (BDeviceManagerAdapter::deviceManagerAdapter == nullptr) {
        return id;
    }
    return BDeviceManagerAdapter::deviceManagerAdapter->ToUUID(id);
}

bool OHOS::DistributedData::DeviceManagerAdapter::IsOHOSType(const std::string &id)
{
    if (BDeviceManagerAdapter::deviceManagerAdapter == nullptr) {
        return false;
    }
    return BDeviceManagerAdapter::deviceManagerAdapter->IsOHOSType(id);
}

bool DeviceManagerAdapter::IsSameAccount(const AccessCaller &accCaller, const AccessCallee &accCallee)
{
    if (BDeviceManagerAdapter::deviceManagerAdapter == nullptr) {
        return false;
    }
    return BDeviceManagerAdapter::deviceManagerAdapter->IsSameAccount(accCaller, accCallee);
}

bool DeviceManagerAdapter::IsSameAccount(const std::string &devicdId)
{
    if (BDeviceManagerAdapter::deviceManagerAdapter == nullptr) {
        return false;
    }
    return BDeviceManagerAdapter::deviceManagerAdapter->IsSameAccount(devicdId);
}

bool DeviceManagerAdapter::CheckAccessControl(const AccessCaller &accCaller, const AccessCallee &accCallee)
{
    if (BDeviceManagerAdapter::deviceManagerAdapter == nullptr) {
        return false;
    }
    return BDeviceManagerAdapter::deviceManagerAdapter->CheckAccessControl(accCaller, accCallee);
}

Status OHOS::DistributedData::DeviceManagerAdapter::StartWatchDeviceChange(const AppDeviceChangeListener *observer,
    __attribute__((unused)) const PipeInfo &pipeInfo)
{
    if (BDeviceManagerAdapter::deviceManagerAdapter == nullptr) {
        return Status::SUCCESS;
    }
    return BDeviceManagerAdapter::deviceManagerAdapter->StartWatchDeviceChange(observer, pipeInfo);
}

Status OHOS::DistributedData::DeviceManagerAdapter::StopWatchDeviceChange(const AppDeviceChangeListener *observer,
    __attribute__((unused)) const PipeInfo &pipeInfo)
{
    if (BDeviceManagerAdapter::deviceManagerAdapter == nullptr) {
        return Status::SUCCESS;
    }
    return BDeviceManagerAdapter::deviceManagerAdapter->StopWatchDeviceChange(observer, pipeInfo);
}

std::vector<DeviceInfo> OHOS::DistributedData::DeviceManagerAdapter::GetRemoteDevices()
{
    if (BDeviceManagerAdapter::deviceManagerAdapter == nullptr) {
        std::vector<DeviceInfo> info;
        return info;
    }
    return BDeviceManagerAdapter::deviceManagerAdapter->GetRemoteDevices();
}

DeviceInfo DeviceManagerAdapter::GetLocalDevice()
{
    if (BDeviceManagerAdapter::deviceManagerAdapter == nullptr) {
        DeviceInfo info;
        return info;
    }
    return BDeviceManagerAdapter::deviceManagerAdapter->GetLocalDevice();
}

std::string OHOS::DistributedData::DeviceManagerAdapter::GetUuidByNetworkId(const std::string &networkId)
{
    if (BDeviceManagerAdapter::deviceManagerAdapter == nullptr) {
        return " ";
    }
    return BDeviceManagerAdapter::deviceManagerAdapter->GetUuidByNetworkId(networkId);
}

DeviceInfo OHOS::DistributedData::DeviceManagerAdapter::GetDeviceInfo(const std::string &id)
{
    if (BDeviceManagerAdapter::deviceManagerAdapter == nullptr) {
        return {};
    }
    return BDeviceManagerAdapter::deviceManagerAdapter->GetDeviceInfo(id);
}

std::string OHOS::DistributedData::DeviceManagerAdapter::ToNetworkID(const std::string &id)
{
    if (BDeviceManagerAdapter::deviceManagerAdapter == nullptr) {
        return " ";
    }
    return BDeviceManagerAdapter::deviceManagerAdapter->ToNetworkID(id);
}

std::string OHOS::DistributedData::DeviceManagerAdapter::CalcClientUuid(
    const std::string &appId, const std::string &uuid)
{
    if (BDeviceManagerAdapter::deviceManagerAdapter == nullptr) {
        return " ";
    }
    return BDeviceManagerAdapter::deviceManagerAdapter->CalcClientUuid(appId, uuid);
}

DeviceManagerAdapter::~DeviceManagerAdapter()
{
}
}