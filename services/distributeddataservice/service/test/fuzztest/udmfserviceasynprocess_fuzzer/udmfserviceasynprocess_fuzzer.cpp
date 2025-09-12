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

#include "udmfserviceasynprocess_fuzzer.h"

#include "accesstoken_kit.h"
#include "distributeddata_udmf_ipc_interface_code.h"
#include "itypes_util.h"
#include "ipc_skeleton.h"
#include "message_parcel.h"
#include "securec.h"
#include "token_setproc.h"
#include "udmf_service_impl.h"
#include "udmf_types_util.h"

using namespace OHOS::UDMF;

namespace OHOS {
const std::u16string INTERFACE_TOKEN = u"OHOS.UDMF.UdmfService";
constexpr size_t NUM_MIN = 5;
constexpr size_t NUM_MAX = 12;
static constexpr int ID_LEN = 32;
static constexpr int MINIMUM = 48;

void ObtainAsynProcessFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind({ "UdmfServiceAsynProcessFuzzTest",
        static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::vector<uint8_t> groupId(ID_LEN, '0');
    for (size_t i = 0; i < groupId.size(); ++i) {
        groupId[i] = provider.ConsumeIntegralInRange<uint8_t>(MINIMUM, MINIMUM);
    }
    std::string businessUdKey(groupId.begin(), groupId.end());
    AsyncProcessInfo processInfo;
    processInfo.businessUdKey = businessUdKey;
    MessageParcel requestUpdate;
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(requestUpdate, processInfo);
    MessageParcel replyUpdate;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::OBTAIN_ASYN_PROCESS),
        requestUpdate, replyUpdate);
    udmfServiceImpl->OnBind(
        { "UdmfServiceAsynProcessFuzzTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), nullptr });
    executor = nullptr;
}

void ClearAsynProcessFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind({ "UdmfServiceAsynProcessFuzzTest",
        static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::vector<uint8_t> groupId(ID_LEN, '0');
    for (size_t i = 0; i < groupId.size(); ++i) {
        groupId[i] = provider.ConsumeIntegralInRange<uint8_t>(MINIMUM, MINIMUM);
    }
    std::string businessUdKey(groupId.begin(), groupId.end());
    MessageParcel requestUpdate;
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(requestUpdate, businessUdKey);
    MessageParcel replyUpdate;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::CLEAR_ASYN_PROCESS_BY_KEY),
        requestUpdate, replyUpdate);
    udmfServiceImpl->OnBind(
        { "UdmfServiceAsynProcessFuzzTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), nullptr });
    executor = nullptr;
}

void ClearAsynProcessByKeyFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind({ "UdmfServiceAsynProcessFuzzTest",
        static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });

    MessageParcel requestUpdate;
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    requestUpdate.WriteBuffer(static_cast<void *>(remainingData.data()), remainingData.size());
    requestUpdate.RewindRead(0);
    std::string businessUdKey = provider.ConsumeRandomLengthString();
    ITypesUtil::Marshal(requestUpdate, businessUdKey);

    MessageParcel replyUpdate;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::CLEAR_ASYN_PROCESS_BY_KEY),
        requestUpdate, replyUpdate);
    udmfServiceImpl->OnBind(
        { "UdmfServiceAsynProcessFuzzTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), nullptr });
    executor = nullptr;
}

void RegisterAsyncProcessInfoFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::string businessUdKey = provider.ConsumeRandomLengthString();
    udmfServiceImpl->RegisterAsyncProcessInfo(businessUdKey);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Security::AccessToken::AccessTokenID tokenId =
        OHOS::Security::AccessToken::AccessTokenKit::GetHapTokenID(100, "com.ohos.dlpmanager", 0);
    SetSelfTokenID(tokenId);
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::ObtainAsynProcessFuzz(provider);
    OHOS::ClearAsynProcessFuzz(provider);
    OHOS::ClearAsynProcessByKeyFuzz(provider);
    OHOS::RegisterAsyncProcessInfoFuzz(provider);
    return 0;
}