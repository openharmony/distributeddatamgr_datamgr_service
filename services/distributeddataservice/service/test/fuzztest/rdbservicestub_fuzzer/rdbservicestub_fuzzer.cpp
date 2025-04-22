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

#include "rdbservicestub_fuzzer.h"

#include <fuzzer/FuzzedDataProvider.h>

#include <cstddef>
#include <cstdint>

#include "ipc_skeleton.h"
#include "message_parcel.h"
#include "rdb_service_impl.h"
#include "securec.h"

using namespace OHOS::DistributedRdb;

namespace OHOS {
const std::u16string INTERFACE_TOKEN = u"OHOS.DistributedRdb.IRdbService";
constexpr size_t NUM_MIN = 5;
constexpr size_t NUM_MAX = 12;

bool OnRemoteRequestFuzz(const uint8_t *data, size_t size)
{
    std::shared_ptr<RdbServiceImpl> rdbServiceImpl = std::make_shared<RdbServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    rdbServiceImpl->OnBind(
        { "RdbServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });

    FuzzedDataProvider provider(data, size);
    uint32_t code =
        provider.ConsumeIntegralInRange<uint32_t>(0, static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_MAX));
    std::vector<uint8_t> remaining_data = provider.ConsumeRemainingBytes<uint8_t>();
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(static_cast<void *>(remaining_data.data()), remaining_data.size());
    request.RewindRead(0);
    MessageParcel reply;
    std::shared_ptr<RdbServiceStub> rdbServiceStub = rdbServiceImpl;
    rdbServiceStub->OnRemoteRequest(code, request, reply);
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::OnRemoteRequestFuzz(data, size);
    return 0;
}