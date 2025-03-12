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
#ifndef OHOS_DISTRIBUTEDDATA_SERVICE_TEST_BDEVICE_MANAGER_ADAPTER_MOCK_H
#define OHOS_DISTRIBUTEDDATA_SERVICE_TEST_BDEVICE_MANAGER_ADAPTER_MOCK_H

#include <gmock/gmock.h>
#include "device_manager_adapter.h"
#include "commu_types.h"

namespace OHOS {
namespace DistributedData {
using namespace OHOS::AppDistributedKv;
using AccessCaller = OHOS::AppDistributedKv::AccessCaller;
using AccessCallee = OHOS::AppDistributedKv::AccessCallee;
class BDeviceManagerAdapter {
public:
    virtual std::vector<DeviceInfo> GetRemoteDevices() = 0;
    virtual bool IsOHOSType(const std::string &) = 0;
    virtual std::vector<std::string> ToUUID(std::vector<DeviceInfo>) = 0;
    virtual Status StartWatchDeviceChange(const AppDeviceChangeListener *, const PipeInfo &) = 0;
    virtual Status StopWatchDeviceChange(const AppDeviceChangeListener *, const PipeInfo &) = 0;
    virtual bool IsSameAccount(const AccessCaller &, const AccessCallee &) = 0;
    virtual bool CheckAccessControl(const AccessCaller &, const AccessCallee &) = 0;
    virtual DeviceInfo GetLocalDevice() = 0;
    static inline std::shared_ptr<BDeviceManagerAdapter> deviceManagerAdapter = nullptr;
    BDeviceManagerAdapter() = default;
    virtual ~BDeviceManagerAdapter() = default;
};

class DeviceManagerAdapterMock : public BDeviceManagerAdapter {
public:
    MOCK_METHOD(std::vector<DeviceInfo>, GetRemoteDevices, ());
    MOCK_METHOD(bool, IsOHOSType, (const std::string &));
    MOCK_METHOD((std::vector<std::string>), ToUUID, (std::vector<DeviceInfo>));
    MOCK_METHOD(Status, StartWatchDeviceChange, (const AppDeviceChangeListener *, const PipeInfo &));
    MOCK_METHOD(Status, StopWatchDeviceChange, (const AppDeviceChangeListener *, const PipeInfo &));
    MOCK_METHOD(bool, IsSameAccount, (const AccessCaller &, const AccessCallee &));
    MOCK_METHOD(bool, CheckAccessControl, (const AccessCaller &, const AccessCallee &));
    MOCK_METHOD(DeviceInfo, GetLocalDevice, ());
};

} // namespace DistributedData
} // namespace OHOS
#endif //OHOS_DISTRIBUTEDDATA_SERVICE_TEST_BDEVICE_MANAGER_ADAPTER_MOCK_H
