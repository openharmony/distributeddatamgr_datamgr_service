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

#include "softbusadapter_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "softbus_adapter_standard.cpp"
#include "message_parcel.h"
#include "securec.h"

using namespace OHOS::AppDistributedKv;

namespace OHOS {
bool OnBytesReceivedFuzz(const uint8_t *data, size_t size)
{
    int connId = static_cast<int>(*data);
    unsigned int dataLen = static_cast<unsigned int>(size);
    AppDataListenerWrap::OnClientBytesReceived(connId, data, dataLen);
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }

    OHOS::OnBytesReceivedFuzz(data, size);

    return 0;
}