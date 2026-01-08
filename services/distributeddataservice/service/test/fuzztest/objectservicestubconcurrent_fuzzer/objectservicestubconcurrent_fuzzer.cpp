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

#include "objectservicestubconcurrent_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "ipc_skeleton.h"
#include "object_service_impl.h"
#include "message_parcel.h"
#include "securec.h"

using namespace OHOS::DistributedObject;

namespace OHOS {
const std::u16string INTERFACE_TOKEN = u"OHOS.DistributedObject.IObjectService";
constexpr uint32_t CODE_MIN = 0;
constexpr uint32_t CODE_MAX = static_cast<uint32_t>(ObjectCode::OBJECTSTORE_SERVICE_CMD_MAX) + 1;
constexpr size_t NUM_MIN = 5;
constexpr size_t NUM_MAX = 12;

static ObjectServiceImpl *g_objectServiceImpl = nullptr;

bool OnRemoteRequestFuzz(FuzzedDataProvider &provider)
{
    uint32_t code = provider.ConsumeIntegralInRange<uint32_t>(CODE_MIN, CODE_MAX);
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(static_cast<void *>(remainingData.data()), remainingData.size());
    request.RewindRead(0);
    MessageParcel reply;
    ObjectServiceStub *objectServiceStub = g_objectServiceImpl;
    if (objectServiceStub == nullptr) {
        return false;
    }
    objectServiceStub->OnRemoteRequest(code, request, reply);
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    OHOS::g_objectServiceImpl = new ObjectServiceImpl();
    std::shared_ptr<OHOS::ExecutorPool> executor = std::make_shared<OHOS::ExecutorPool>(OHOS::NUM_MAX, OHOS::NUM_MIN);
    OHOS::g_objectServiceImpl->OnBind(
        { "ObjectServiceStubFuzz", static_cast<uint32_t>(OHOS::IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::OnRemoteRequestFuzz(provider);
    return 0;
}