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

#include "udmfservicehandleappshareoptionconcurrent_fuzzer.h"

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
static std::shared_ptr<UdmfServiceImpl> g_udmfServiceImpl = nullptr;

void OnGetAppShareOptionFuzz(FuzzedDataProvider &provider)
{
    MessageParcel requestUpdate;
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    requestUpdate.WriteBuffer(static_cast<void *>(remainingData.data()), remainingData.size());
    requestUpdate.RewindRead(0);
    CustomOption option = {.intention = Intention::UD_INTENTION_DRAG};
    std::string intention = UD_INTENTION_MAP.at(option.intention);
    ITypesUtil::Marshal(requestUpdate, intention);

    MessageParcel replyUpdate;
    std::shared_ptr<UdmfServiceStub> udmfServiceStub = g_udmfServiceImpl;
    if (udmfServiceStub == nullptr) {
        return;
    }
    udmfServiceStub->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::GET_APP_SHARE_OPTION),
        requestUpdate, replyUpdate);
}

void SetAppShareOptionFuzz(FuzzedDataProvider &provider)
{
    MessageParcel requestUpdate;
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    requestUpdate.WriteBuffer(static_cast<void *>(remainingData.data()), remainingData.size());
    requestUpdate.RewindRead(0);
    CustomOption option = {.intention = Intention::UD_INTENTION_DRAG};
    std::string intention = UD_INTENTION_MAP.at(option.intention);
    int32_t shareOption = provider.ConsumeIntegral<int32_t>();
    ITypesUtil::Marshal(requestUpdate, intention, shareOption);

    MessageParcel replyUpdate;
    std::shared_ptr<UdmfServiceStub> udmfServiceStub = g_udmfServiceImpl;
    if (udmfServiceStub == nullptr) {
        return;
    }
    udmfServiceStub->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::SET_APP_SHARE_OPTION),
        requestUpdate, replyUpdate);
}

void RemoveAppShareOptionFuzz(FuzzedDataProvider &provider)
{
    MessageParcel requestUpdate;
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    requestUpdate.WriteBuffer(static_cast<void *>(remainingData.data()), remainingData.size());
    requestUpdate.RewindRead(0);
    CustomOption option = {.intention = Intention::UD_INTENTION_DRAG};
    std::string intention = UD_INTENTION_MAP.at(option.intention);
    ITypesUtil::Marshal(requestUpdate, intention);

    MessageParcel replyUpdate;
    std::shared_ptr<UdmfServiceStub> udmfServiceStub = g_udmfServiceImpl;
    if (udmfServiceStub == nullptr) {
        return;
    }
    udmfServiceStub->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::REMOVE_APP_SHARE_OPTION),
        requestUpdate, replyUpdate);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::g_udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    OHOS::Security::AccessToken::AccessTokenID tokenId =
        OHOS::Security::AccessToken::AccessTokenKit::GetHapTokenID(100, "com.ohos.dlpmanager", 0);
    SetSelfTokenID(tokenId);
    std::shared_ptr<OHOS::ExecutorPool> executor = std::make_shared<OHOS::ExecutorPool>(OHOS::NUM_MAX, OHOS::NUM_MIN);
    OHOS::g_udmfServiceImpl->OnBind(
        { "UdmfServiceHandleAppShareOptionConcurrentFuzzTest", static_cast<uint32_t>(OHOS::IPCSkeleton::GetSelfTokenID()),
        std::move(executor) });
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::OnGetAppShareOptionFuzz(provider);
    OHOS::SetAppShareOptionFuzz(provider);
    OHOS::RemoveAppShareOptionFuzz(provider);
    return 0;
}