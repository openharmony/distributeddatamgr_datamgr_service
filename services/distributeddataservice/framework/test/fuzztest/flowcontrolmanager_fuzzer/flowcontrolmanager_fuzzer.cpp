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
#include "flowcontrolmanager_fuzzer.h"
#include "flow_control_manager/flow_control_manager.h"

using namespace OHOS::DistributedData;

namespace OHOS {

class PriorityStrategy : public FlowControlManager::Strategy {
public:
    FlowControlManager::Tp GetExecuteTime(FlowControlManager::Task task,
        const FlowControlManager::TaskInfo &info) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();

        // Type 0: High priority, execute immediately
        // Type 1: Medium priority, delay 100ms to execute
        // Type 2: Low priority, delay 500ms to execute
        // All Medium tasks delayed by 100ms, all Low tasks delayed by 500ms
        return now + std::chrono::milliseconds(info.type == 0 ? 0 : info.type == 1 ? 100 : 500);
    }

private:
    std::mutex mutex_;
};

void FlowControlManagerFuzz(FuzzedDataProvider &provider)
{
    int valueRangeMinPool = provider.ConsumeIntegralInRange<int>(1, 2);
    int valueRangeMaxPool = provider.ConsumeIntegralInRange<int>(3, 4);
    auto pool = std::make_shared<ExecutorPool>(valueRangeMinPool, valueRangeMaxPool);
    FlowControlManager flowControlManager(pool, std::make_shared<PriorityStrategy>());

    auto highPriorityFlag = std::make_shared<std::atomic_uint32_t>(0);
    auto mediumPriorityFlag = std::make_shared<std::atomic_uint32_t>(0);
    auto lowPriorityFlag = std::make_shared<std::atomic_uint32_t>(0);
    auto highPriorityTask = [highPriorityFlag]() mutable {
        (*highPriorityFlag)++;
    };
    auto mediumPriorityTask = [mediumPriorityFlag]() mutable {
        (*mediumPriorityFlag)++;
    };
    auto lowPriorityTask = [lowPriorityFlag]() mutable {
        (*lowPriorityFlag)++;
    };

    // Submit tasks with different priorities
    int taskType1 = provider.ConsumeIntegralInRange<int>(0, 2);
    flowControlManager.Execute(lowPriorityTask, taskType1);
    int taskType2 = provider.ConsumeIntegralInRange<int>(0, 2);
    flowControlManager.Execute(highPriorityTask, taskType2);
    int taskType3 = provider.ConsumeIntegralInRange<int>(0, 2);
    flowControlManager.Execute(mediumPriorityTask, taskType3);
    flowControlManager.Remove(taskType1);
}

void FlowControlManagerPoolNullFuzz(FuzzedDataProvider &provider)
{
    auto poolNull = nullptr;
    FlowControlManager flowControlManagerNull(poolNull, std::make_shared<PriorityStrategy>());

    auto highPriorityFlag = std::make_shared<std::atomic_uint32_t>(0);
    auto mediumPriorityFlag = std::make_shared<std::atomic_uint32_t>(0);
    auto lowPriorityFlag = std::make_shared<std::atomic_uint32_t>(0);
    auto highPriorityTask = [highPriorityFlag]() mutable {};
    auto mediumPriorityTask = [mediumPriorityFlag]() mutable {};
    auto lowPriorityTask = [lowPriorityFlag]() mutable {};

    // Submit tasks with different priorities
    int taskType1 = provider.ConsumeIntegralInRange<int>(0, 2);
    flowControlManagerNull.Execute(lowPriorityTask, taskType1);
    int taskType2 = provider.ConsumeIntegralInRange<int>(0, 2);
    flowControlManagerNull.Execute(highPriorityTask, taskType2);
    int taskType3 = provider.ConsumeIntegralInRange<int>(0, 2);
    flowControlManagerNull.Execute(mediumPriorityTask, taskType3);
    flowControlManagerNull.Remove(taskType1);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::FlowControlManagerFuzz(provider);
    OHOS::FlowControlManagerPoolNullFuzz(provider);
    return 0;
}