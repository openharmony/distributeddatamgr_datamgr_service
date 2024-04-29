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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_STORE_INFO_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_STORE_INFO_H

#include <string>

namespace OHOS::DistributedData {
struct StoreInfo {
    uint32_t tokenId = 0;
    std::string bundleName;
    std::string storeName;
    int32_t instanceId = 0;
    int32_t user = 0;
    std::string deviceId;
    uint64_t syncId = 0;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_STORE_INFO_H
