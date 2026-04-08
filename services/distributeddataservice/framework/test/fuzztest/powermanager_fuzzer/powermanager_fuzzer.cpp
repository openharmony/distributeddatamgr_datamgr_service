/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "powermanager_fuzzer.h"
#include "power_manager/power_manager.h"

using namespace OHOS::DistributedData;

namespace OHOS {
class MockPowerManager final : public PowerManager {
public:
    int32_t Subscribe(std::shared_ptr<Observer> observer) override
    {
        return 0;
    }

    int32_t Unsubscribe(std::shared_ptr<Observer> observer) override
    {
        return 0;
    }

    bool IsCharging() override
    {
        return false;
    }

    void SubscribePowerEvent() override {}

    void UnsubscribePowerEvent() override {}
};

void GetInstanceFuzz()
{
    auto *instance = PowerManager::GetInstance();
    (void)instance;
}

void RegisterInstanceFuzz(FuzzedDataProvider &provider)
{
    // Reset the singleton so fuzz runs are independent
    // Note: instance_ is private and not resettable from outside,
    // so we test the two paths based on whether a registration already exists.

    bool shouldRegisterFirst = provider.ConsumeBool();
    if (shouldRegisterFirst) {
        MockPowerManager mock;
        // First registration should succeed (if no instance registered yet)
        bool result = PowerManager::RegisterInstance(&mock);
        (void)result;
        // Second registration should return false (already registered)
        bool result2 = PowerManager::RegisterInstance(&mock);
        (void)result2;
    } else {
        // Direct registration attempt without prior setup
        MockPowerManager mock;
        bool result = PowerManager::RegisterInstance(&mock);
        (void)result;
    }
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::GetInstanceFuzz();
    OHOS::RegisterInstanceFuzz(provider);
    return 0;
}
