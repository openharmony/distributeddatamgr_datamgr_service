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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_DEV_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_DEV_MANAGER_H
#include <string>

#include "types.h"
#include "lru_bucket.h"
namespace OHOS::DistributedKv {
class API_EXPORT DevManager {
public:
    struct DetailInfo {
        std::string uuid;
        std::string udid;
        std::string networkId;
        std::string deviceName;
        std::string deviceType;
    };
    static DevManager &GetInstance();
    std::string ToUUID(const std::string &networkId) const;
    const DetailInfo &GetLocalDevice();
    std::vector<DetailInfo> GetRemoteDevices() const;
private:
    const DetailInfo invalidDetail_ {};
    std::mutex mutex_ {};
    DetailInfo localInfo_ {};
    mutable LRUBucket<std::string, DetailInfo> deviceInfos_ {16};
    std::string GetUuidByNetworkId(const std::string &networkId) const;
    std::string GetUdidByNetworkId(const std::string &networkId) const;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_DEV_MANAGER_H
