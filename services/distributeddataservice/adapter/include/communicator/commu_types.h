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

#ifndef OHOS_DISTRIBUTED_DATA_ADAPTER_COMMUNICATOR_COMMU_TYPES_H
#define OHOS_DISTRIBUTED_DATA_ADAPTER_COMMUNICATOR_COMMU_TYPES_H
#include <string>
#include "store_errno.h"
#include "visibility.h"
namespace OHOS::AppDistributedKv {
using Status = DistributedKv::Status;
struct API_EXPORT DeviceInfo {
    std::string uuid;
    std::string udid;
    std::string networkId;
    std::string deviceName;
    uint32_t deviceType;
};

enum DeviceType {
    SMART_WATCH,
    KID_WATCH,
    PHONE,
    OTHERS,
};

struct API_EXPORT PipeInfo {
    std::string pipeId;
    std::string userId;
};

struct API_EXPORT DeviceId {
    std::string deviceId;
};

enum class API_EXPORT MessageType {
    DEFAULT = 0,
    FILE = 1,
};

struct API_EXPORT MessageInfo {
    MessageType msgType;
};

enum class API_EXPORT DeviceChangeType {
    DEVICE_OFFLINE = 0,
    DEVICE_ONLINE = 1,
    DEVICE_ONREADY = 2,
};

enum class API_EXPORT DeviceStatus {
    OFFLINE = 0,
    ONLINE = 1,
};
}
#endif // OHOS_DISTRIBUTED_DATA_ADAPTER_COMMUNICATOR_COMMU_TYPES_H
