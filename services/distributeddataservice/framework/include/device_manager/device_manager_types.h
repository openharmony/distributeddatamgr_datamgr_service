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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORK_DEVICE_MANAGER_TYPES_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORK_DEVICE_MANAGER_TYPES_H
#include <string>
#include "visibility.h"
namespace OHOS::DistributedData {
struct API_EXPORT DeviceInfo {
    std::string uuid;
    std::string udid;
    std::string networkId;
    std::string deviceName;
    uint32_t deviceType;
    int32_t osType;
    int32_t authForm;
};
}
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORK_DEVICE_MANAGER_TYPES_H
