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

#include "rdbflowcontrolmanager_fuzzer.h"

#include "rdb_flow_control_manager.h"

using namespace OHOS::DistributedRdb;

namespace OHOS {
void OnRdbFlowControlManagerPoolNullFuzz(FuzzedDataProvider &provider)
{
    auto poolNull = nullptr;
    int valueRangeDuration = provider.ConsumeIntegralInRange<int>(100, 200);
    int valueRangeApp = provider.ConsumeIntegralInRange<int>(5, 20);
    int valueRangeGlobal = provider.ConsumeIntegralInRange<int>(15, 20);
    RdbFlowControlManager flowControlManagerNull(valueRangeApp, valueRangeGlobal, valueRangeDuration);
    flowControlManagerNull.Init(poolNull);
    auto task = []() mutable {};
    const int taskCount = provider.ConsumeIntegralInRange<int>(100, 200);
    for (int i = 0; i < taskCount; i++) {
        flowControlManagerNull.Execute(task, { 0, "bulkTask" });
    }
    flowControlManagerNull.Remove("bulkTask");
}

void OnRdbFlowControlManagerFuzz(FuzzedDataProvider &provider)
{
    int valueRangeMinPool = provider.ConsumeIntegralInRange<int>(1, 2);
    int valueRangeMaxPool = provider.ConsumeIntegralInRange<int>(3, 4);
    auto pool = std::make_shared<ExecutorPool>(valueRangeMinPool, valueRangeMaxPool);
    int valueRangeDuration = provider.ConsumeIntegralInRange<int>(100, 200);
    int valueRangeApp = provider.ConsumeIntegralInRange<int>(5, 20);
    int valueRangeGlobal = provider.ConsumeIntegralInRange<int>(15, 20);
    RdbFlowControlManager flowControlManager(valueRangeApp, valueRangeGlobal, valueRangeDuration);
    flowControlManager.Init(pool);
    auto flag = std::make_shared<std::atomic_uint32_t>(0);
    auto task = [flag]() mutable {
        (*flag)++;
    };
    const int taskCount = provider.ConsumeIntegralInRange<int>(100, 200);
    for (int i = 0; i < taskCount; i++) {
        flowControlManager.Execute(task, { 0, "bulkTask" });
    }
    flowControlManager.Remove("bulkTask");
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::OnRdbFlowControlManagerPoolNullFuzz(provider);
    OHOS::OnRdbFlowControlManagerFuzz(provider);
    return 0;
}