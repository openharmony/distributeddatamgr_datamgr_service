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
#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_NETWORK_SYNC_STRATEGY_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_NETWORK_SYNC_STRATEGY_H
#include "cloud/sync_strategy.h"
#include "concurrent_map.h"
#include "serializable/serializable.h"
namespace OHOS::CloudData {
class NetworkSyncStrategy : public DistributedData::SyncStrategy {
public:
    using StoreInfo = DistributedData::StoreInfo;
    enum Strategy : uint32_t {
        // should equal to NetWorkStrategy::WIFI in cloud_types.h
        WIFI = 0x01,
        // should equal to NetWorkStrategy::CELLULAR in cloud_types.h
        CELLULAR = 0x02,
        BUTT
    };
    struct StrategyInfo : public DistributedData::Serializable {
        int32_t user = INVALID_USER;
        std::string bundleName;
        uint32_t strategy = DEFAULT_STRATEGY;
        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
        std::string GetKey();
        static constexpr int32_t INVALID_USER = -1;
        static constexpr const char *PREFIX = "NETWORK_SYNC_STRATEGY";
    };
    NetworkSyncStrategy();
    ~NetworkSyncStrategy();
    int32_t CheckSyncAction(const StoreInfo& storeInfo) override;

    static std::string GetKey(int32_t user);
    static std::string GetKey(int32_t user, const std::string& bundleName);
private:
    StrategyInfo GetStrategy(int32_t user, const std::string& bundleName);

    static bool Check(uint32_t strategy);
    static constexpr uint32_t DEFAULT_STRATEGY = WIFI | CELLULAR;
    std::shared_ptr<SyncStrategy> next_;
    int32_t user_ = -1;
    ConcurrentMap<std::string, StrategyInfo> strategies_;
};
}
#endif //OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_NETWORK_SYNC_STRATEGY_H
