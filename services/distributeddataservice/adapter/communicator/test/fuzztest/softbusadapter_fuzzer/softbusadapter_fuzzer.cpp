/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "softbusadapter_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "message_parcel.h"
#include "securec.h"
#include "softbus_adapter_standard.cpp"

using namespace OHOS::AppDistributedKv;

namespace OHOS {
bool OnBytesReceivedFuzz(FuzzedDataProvider &provider)
{
    int connId = provider.ConsumeIntegral<int>();
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    AppDataListenerWrap::OnClientBytesReceived(connId, static_cast<void *>(remainingData.data()),
        remainingData.size());
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::OnBytesReceivedFuzz(provider);
    return 0;
}