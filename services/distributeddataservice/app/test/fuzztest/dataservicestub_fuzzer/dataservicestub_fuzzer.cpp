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

#include "dataservicestub_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "kvstore_data_service.h"
#include "message_parcel.h"
#include "securec.h"

using namespace OHOS::DistributedKv;

namespace OHOS {
const std::u16string INTERFACE_TOKEN = u"OHOS.DistributedKv.IKvStoreDataService";
const uint32_t CODE_MIN = 0;
const uint32_t CODE_MAX = 10;

bool OnRemoteRequestFuzz(FuzzedDataProvider &provider)
{
    uint32_t code = provider.ConsumeIntegralInRange<uint32_t>(CODE_MIN, CODE_MAX);
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    std::vector<uint8_t> remainingData  = provider.ConsumeRemainingBytes<uint8_t>();
    request.WriteBuffer(static_cast<void *>(remainingData .data()), remainingData .size());
    request.RewindRead(0);
    MessageParcel reply;
    MessageOption option;
    std::shared_ptr<KvStoreDataServiceStub> kvStoreDataServiceStub = std::make_shared<KvStoreDataService>();
    kvStoreDataServiceStub->OnRemoteRequest(code, request, reply, option);
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::OnRemoteRequestFuzz(provider);
    return 0;
}