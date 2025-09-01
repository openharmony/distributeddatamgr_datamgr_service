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

#include <fuzzer/FuzzedDataProvider.h>
#include "src/network_delegate_normal_impl.h"
#include "syncstrategies_fuzzer.h"
#include "sync_strategies/network_sync_strategy.h"

using namespace OHOS::DistributedData;
using namespace OHOS::CloudData;

namespace OHOS {
constexpr int NUM_MAX = 100;

void SyncStrategiesFuzz001(FuzzedDataProvider &provider)
{
    int32_t user = provider.ConsumeIntegral<int32_t>();
    std::string bundleName = provider.ConsumeRandomLengthString(NUM_MAX);
    static NetworkDelegateNormalImpl delegate;
    NetworkDelegate::RegisterNetworkInstance(&delegate);
    NetworkSyncStrategy strategy;
    StoreInfo storeInfo;
    storeInfo.user = user;
    storeInfo.bundleName = bundleName;
    strategy.CheckSyncAction(storeInfo);
}

void SyncStrategiesFuzz002(FuzzedDataProvider &provider)
{
    uint32_t strategy = provider.ConsumeIntegral<uint32_t>();
    NetworkSyncStrategy strategyInstance;
    strategyInstance.Check(strategy);
}

void SyncStrategiesFuzz003(FuzzedDataProvider &provider)
{
    int32_t user = provider.ConsumeIntegral<int32_t>();
    std::string bundleName = provider.ConsumeRandomLengthString(NUM_MAX);
    NetworkSyncStrategy strategyInstance;
    strategyInstance.GetStrategy(user, bundleName);
    strategyInstance.GetKey(user);

    NetworkSyncStrategy::StrategyInfo info;
    info.user = provider.ConsumeIntegral<int32_t>();
    info.bundleName = provider.ConsumeRandomLengthString(NUM_MAX);
    info.strategy = provider.ConsumeIntegral<uint32_t>();
    Serializable::json node;
    std::string key = provider.ConsumeRandomLengthString(NUM_MAX);
    std::string valueStr = provider.ConsumeRandomLengthString(NUM_MAX);
    int valueInt = provider.ConsumeIntegral<int>();
    float valueFloat = provider.ConsumeFloatingPoint<float>();
    bool valueBool = provider.ConsumeBool();
    int valueRange = provider.ConsumeIntegralInRange<int>(0, NUM_MAX);
    node[key] = valueStr;
    node["integer"] = valueInt;
    node["float"] = valueFloat;
    node["boolean"] = valueBool;
    node["range"] = valueRange;
    info.Marshal(node);
    info.Unmarshal(node);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::SyncStrategiesFuzz001(provider);
    OHOS::SyncStrategiesFuzz002(provider);
    OHOS::SyncStrategiesFuzz003(provider);
    return 0;
}