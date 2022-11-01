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

#include "kvstorescheduler_fuzzer.h"

#include <chrono>
#include <thread>

#include "kv_scheduler.h"

using namespace OHOS::DistributedKv;
namespace OHOS {
static constexpr int MAX_DELAY_TIME = 5;
static constexpr int MAX_INTERVAL_TIME = 3;
void AtFuzz(size_t time)
{
    KvScheduler kvScheduler;
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now() +
                                               std::chrono::duration<int>(time % MAX_DELAY_TIME);
    auto task = kvScheduler.At(tp, []() { });
    std::this_thread::sleep_for(std::chrono::seconds(MAX_INTERVAL_TIME));
    kvScheduler.Remove(task);
}

void EveryFUZZ(size_t time)
{
    KvScheduler kvScheduler;
    std::chrono::duration<int> delay(time % MAX_DELAY_TIME);
    std::chrono::duration<int> interval(time % MAX_INTERVAL_TIME);
    kvScheduler.Every(delay, interval, []() { });
    std::this_thread::sleep_for(std::chrono::seconds(MAX_INTERVAL_TIME));
    kvScheduler.Every(0, delay, interval, []() { });
    kvScheduler.Every(2, delay, interval, []() { });
    std::this_thread::sleep_for(std::chrono::seconds(MAX_INTERVAL_TIME));
    kvScheduler.Every(interval, []() { });
    kvScheduler.Clean();
}

void ResetFuzz(size_t time)
{
    KvScheduler kvScheduler;
    std::chrono::duration<int> interval(time % MAX_INTERVAL_TIME);
    auto schedulerTask = kvScheduler.At(std::chrono::system_clock::now() +
                                    std::chrono::duration<int>(MAX_DELAY_TIME / 2), []() {});
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now() +
                                               std::chrono::duration<int>(time % MAX_DELAY_TIME);
    kvScheduler.Reset(schedulerTask, tp, interval);
}
} // namespace OHOS
/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AtFuzz(size);
    OHOS::EveryFUZZ(size);
    OHOS::ResetFuzz(size);
    return 0;
}
