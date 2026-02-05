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

#include "utdservice_fuzzer.h"
#include "accesstoken_kit.h"
#include "distributeddata_udmf_ipc_interface_code.h"
#include "ipc_skeleton.h"
#include "message_parcel.h"
#include "securec.h"
#include "token_setproc.h"
#include "utd_service_impl.h"
#include "utd_tlv_util.h"

using namespace OHOS::UDMF;

namespace OHOS {
constexpr size_t NUM_MIN = 5;
constexpr size_t NUM_MAX = 12;
const std::u16string INTERFACE_TOKEN = u"OHOS.UDMF.UtdService";

void OnRegisterTypeDescriptorsFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UtdServiceImpl> utdServiceImpl = std::make_shared<UtdServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    utdServiceImpl->OnBind({ "UtdServiceFuzzTest",
        static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::string str = provider.ConsumeRandomLengthString();
    std::vector<std::string> vec;
    size_t dataSize = provider.ConsumeIntegralInRange<size_t>(1, 50);
    for (size_t i = 0; i < dataSize; i++) {
        vec.push_back(str);
    }

    std::vector<TypeDescriptorCfg> descriptors;
    for (size_t i = 0; i < dataSize; i++) {
        TypeDescriptorCfg descriptor;
        descriptor.typeId = str;
        descriptor.filenameExtensions = vec;
        descriptor.belongingToTypes = vec;
        descriptor.mimeTypes = vec;
        descriptor.description = str;
        descriptor.ownerBundle = str;
        descriptors.push_back(descriptor);
    }
    
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, descriptors);
    MessageParcel reply;
    utdServiceImpl->OnRemoteRequest(
        static_cast<uint32_t>(UtdServiceInterfaceCode::REGISTER_UTD_TYPES), request, reply);
    utdServiceImpl->OnBind(
        { "UtdServiceFuzzTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), nullptr });
    executor = nullptr;
}

void OnUnregisterTypeDescriptorsFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UtdServiceImpl> utdServiceImpl = std::make_shared<UtdServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    utdServiceImpl->OnBind({ "UtdServiceFuzzTest",
        static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::string str = provider.ConsumeRandomLengthString();
    std::vector<std::string> vec;
    size_t dataSize = provider.ConsumeIntegralInRange<size_t>(1, 50);
    for (size_t i = 0; i < dataSize; i++) {
        vec.push_back(str);
    }

    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, vec);
    MessageParcel reply;
    utdServiceImpl->OnRemoteRequest(
        static_cast<uint32_t>(UtdServiceInterfaceCode::UNREGISTER_UTD_TYPES), request, reply);
    utdServiceImpl->OnBind(
        { "UtdServiceFuzzTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), nullptr });
    executor = nullptr;
}

void OnRegServiceNotifierFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UtdServiceImpl> utdServiceImpl = std::make_shared<UtdServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    utdServiceImpl->OnBind({ "UtdServiceFuzzTest",
        static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    sptr<IRemoteObject> iUdmfNotifier = nullptr;
    ITypesUtil::Marshal(request, iUdmfNotifier);
    utdServiceImpl->OnRemoteRequest(
        static_cast<uint32_t>(UtdServiceInterfaceCode::SET_UTD_NOTIFIER), request, reply);
    std::string str = provider.ConsumeRandomLengthString();
    std::vector<std::string> vec;
    size_t dataSize = provider.ConsumeIntegralInRange<size_t>(1, 50);
    for (size_t i = 0; i < dataSize; i++) {
        vec.push_back(str);
    }
    MessageParcel requestWrong;
    requestWrong.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(requestWrong, vec);
    utdServiceImpl->OnRemoteRequest(
        static_cast<uint32_t>(UtdServiceInterfaceCode::SET_UTD_NOTIFIER), requestWrong, reply);
    utdServiceImpl->OnBind(
        { "UtdServiceFuzzTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), nullptr });
    executor = nullptr;
}

void NotifyUtdClientsFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UtdServiceImpl> utdServiceImpl = std::make_shared<UtdServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    utdServiceImpl->OnBind({ "UtdServiceFuzzTest",
        static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    
    utdServiceImpl->NotifyUtdClients(provider.ConsumeIntegral<uint32_t>());
    utdServiceImpl->OnBind(
        { "UtdServiceFuzzTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), nullptr });
    executor = nullptr;
}

void GetHapBundleNameByTokenFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UtdServiceImpl> utdServiceImpl = std::make_shared<UtdServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    utdServiceImpl->OnBind({ "UtdServiceFuzzTest",
        static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    
    uint32_t tokenId = provider.ConsumeIntegral<uint32_t>();
    std::string bundleName = provider.ConsumeRandomLengthString();
    utdServiceImpl->GetHapBundleNameByToken(tokenId, bundleName);
    utdServiceImpl->OnBind(
        { "UtdServiceFuzzTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), nullptr });
    executor = nullptr;
}

void VerifyPermissionFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UtdServiceImpl> utdServiceImpl = std::make_shared<UtdServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    utdServiceImpl->OnBind({ "UtdServiceFuzzTest",
        static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    
    std::string permission = provider.ConsumeRandomLengthString();
    uint32_t callerTokenId = provider.ConsumeIntegral<uint32_t>();
    utdServiceImpl->VerifyPermission(permission, callerTokenId);
    utdServiceImpl->OnBind(
        { "UtdServiceFuzzTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), nullptr });
    executor = nullptr;
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
    OHOS::OnRegisterTypeDescriptorsFuzz(provider);
    OHOS::OnUnregisterTypeDescriptorsFuzz(provider);
    OHOS::OnRegServiceNotifierFuzz(provider);
    OHOS::NotifyUtdClientsFuzz(provider);
    OHOS::GetHapBundleNameByTokenFuzz(provider);
    OHOS::VerifyPermissionFuzz(provider);
    return 0;
}
