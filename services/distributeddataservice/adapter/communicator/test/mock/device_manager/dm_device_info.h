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

#ifndef MOCK_DM_DEVICE_INFO_H
#define MOCK_DM_DEVICE_INFO_H

#include "serializable/serializable.h"

#define DM_MAX_DEVICE_ID_LEN (97)

namespace OHOS::DistributedHardware {
struct DeviceExtraInfo final : public DistributedData::Serializable {
    static constexpr int32_t OH_OS_TYPE = 10;

    int32_t OS_TYPE = OH_OS_TYPE;

    DeviceExtraInfo() {};
    ~DeviceExtraInfo() {};
    bool Marshal(json &node) const override
    {
        return SetValue(node[GET_NAME(OS_TYPE)], OS_TYPE);
    };
    bool Unmarshal(const json &node) override
    {
        return GetValue(node, GET_NAME(OS_TYPE), OS_TYPE);
    };
};

struct DmDeviceInfo {
    char deviceId[DM_MAX_DEVICE_ID_LEN] = {0};
    char deviceName[DM_MAX_DEVICE_ID_LEN] = {0};
    uint16_t deviceTypeId;
    char networkId[DM_MAX_DEVICE_ID_LEN] = {0};
    int32_t range;
    int32_t networkType;
    int32_t authForm;
    std::string extraData;
};

struct DmAccessCaller {
    std::string accountId;
    std::string pkgName;
    std::string networkId;
    int32_t userId;
    uint64_t tokenId = 0;
    std::string extra;
};

struct DmAccessCallee {
    std::string accountId;
    std::string networkId;
    std::string peerId;
    std::string pkgName;
    int32_t userId;
    uint64_t tokenId = 0;
    std::string extra;
};

enum DmNotifyEvent {
    DM_NOTIFY_EVENT_ONDEVICEREADY,
};
} // namespace OHOS::DistributedHardware
#endif // MOCK_DM_DEVICE_INFO_H