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
#include "kvdbservicestub_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "bootstrap.h"
#include "ipc_skeleton.h"
#include "kvdb_service_impl.h"
#include "message_parcel.h"
#include "securec.h"

using namespace OHOS::DistributedData;
using namespace OHOS::DistributedKv;

namespace OHOS {
const std::u16string INTERFACE_TOKEN = u"OHOS.DistributedKv.KVFeature";
constexpr uint32_t CODE_MIN = 0;
constexpr uint32_t CODE_MAX = static_cast<uint32_t>(KVDBServiceInterfaceCode::TRANS_BUTT) + 1;
constexpr size_t NUM_MIN = 5;
constexpr size_t NUM_MAX = 12;

bool OnRemoteRequestFuzz(const uint8_t *data, size_t size)
{
    std::shared_ptr<KVDBServiceImpl> kvdbServiceImpl = std::make_shared<KVDBServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    kvdbServiceImpl->OnBind(
        { "KvdbServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });

    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadDirectory();
    Bootstrap::GetInstance().LoadCheckers();
    Bootstrap::GetInstance().LoadNetworks();

    uint32_t code = static_cast<uint32_t>(*data) % (CODE_MAX - CODE_MIN + 1) + CODE_MIN;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;
    std::shared_ptr<KVDBServiceStub> kvdbServiceStub = kvdbServiceImpl;
    kvdbServiceStub->OnRemoteRequest(code, request, reply);
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }

    OHOS::OnRemoteRequestFuzz(data, size);

    return 0;
}