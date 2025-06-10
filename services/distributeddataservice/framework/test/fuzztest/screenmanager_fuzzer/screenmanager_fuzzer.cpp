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
#include "screenmanager_fuzzer.h"
#include "screen/screen_manager.h"

using namespace OHOS::DistributedData;
 
namespace OHOS {
void TestScreenManager(FuzzedDataProvider &provider)
{
    ScreenManager::GetInstance()->Subscribe(nullptr);
    ScreenManager::GetInstance()->Unsubscribe(nullptr);
    ScreenManager::GetInstance()->BindExecutor(nullptr);
    ScreenManager::GetInstance()->SubscribeScreenEvent();
    ScreenManager::GetInstance()->UnsubscribeScreenEvent();
}
 
} // namespace OHOS
 
/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::TestScreenManager(provider);
    return 0;
}