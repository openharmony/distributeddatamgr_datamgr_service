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

#include "datashareservicestub_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "data_share_service_impl.h"
#include "ipc_skeleton.h"
#include "message_parcel.h"
#include "securec.h"

using namespace OHOS::DataShare;

namespace OHOS {
const std::u16string INTERFACE_TOKEN = u"OHOS.DataShare.IDataShareService";
constexpr uint32_t CODE_MIN = 0;
constexpr uint32_t CODE_MAX = static_cast<uint32_t>(IDataShareService::DATA_SHARE_SERVICE_CMD_MAX) + 1;
constexpr size_t NUM_MIN = 5;
constexpr size_t NUM_MAX = 12;

bool OnRemoteRequestFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });

    uint32_t code = provider.ConsumeIntegralInRange<uint32_t>(CODE_MIN, CODE_MAX);
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(static_cast<void *>(remainingData.data()), remainingData.size());
    request.RewindRead(0);
    MessageParcel reply;
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    dataShareServiceStub->OnRemoteRequest(code, request, reply);
    return true;
}

bool OnRemoteRequestFuzz01(uint32_t code, FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(static_cast<void *>(remainingData.data()), remainingData.size());
    request.RewindRead(0);
    MessageParcel reply;
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    dataShareServiceStub->OnRemoteRequest(code, request, reply);
    return true;
}

bool OnRemoteRequestFuzz02(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(static_cast<void *>(remainingData.data()), remainingData.size());
    request.RewindRead(0);
    MessageParcel reply;
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    dataShareServiceStub->OnRemoteRequest(IDataShareService::DATA_SHARE_CMD_SYSTEM_CODE, request, reply);
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::OnRemoteRequestFuzz(provider);
    uint32_t codeTest = OHOS::CODE_MIN;
    while (codeTest <= OHOS::CODE_MAX) {
        OHOS::OnRemoteRequestFuzz01(codeTest, provider);
        codeTest++;
    }
    OHOS::OnRemoteRequestFuzz02(provider);
    return 0;
}