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
#include "device_manager/device_manager_types.h"
#include "store_errno.h"
#include "visibility.h"
namespace OHOS::AppDistributedKv {
using Status = DistributedKv::Status;
using DeviceInfo = DistributedData::DeviceInfo;

struct API_EXPORT AccessCaller {
    std::string accountId;
    std::string bundleName;
    std::string networkId;
    int32_t userId;
};

struct API_EXPORT AccessCallee {
    std::string accountId;
    std::string networkId;
    int32_t userId;
};

struct API_EXPORT AclParams {
    AccessCaller accCaller;
    AccessCallee accCallee;
    int32_t authType = 0;
};

enum RouteType : int32_t {
    INVALID_ROUTE_TYPE = -1,
    ROUTE_TYPE_ALL = 0,
    WIFI_STA,
    WIFI_P2P,
    BT_BR,
    BT_BLE,
    BUTT,
};

struct ExtraDataInfo {
    std::string userId;
    std::string appId;
    std::string bundleName;
    std::string storeId;
    uint32_t tokenId = 0;
};

struct DataInfo {
    uint8_t *data;
    uint32_t length;
    ExtraDataInfo extraInfo;
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

struct API_EXPORT LevelInfo {
    uint16_t dynamic;
    uint16_t statics;
    uint32_t switches;
    uint16_t switchesLen;
};

struct API_EXPORT SessionAccessInfo {
    int32_t userId = -1;
    std::string bundleName;
    std::string storeId;
    std::string accountId;
    uint32_t tokenId = 0;
    bool isOHType = true;
};

struct BundleEventInfo {
    std::string bundleName;
    int32_t userId = 0;
    int32_t appIndex = 0;
    int32_t tokenId = -1;
};
}
#endif // OHOS_DISTRIBUTED_DATA_ADAPTER_COMMUNICATOR_COMMU_TYPES_H
