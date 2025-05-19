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

#include "udmfservice_fuzzer.h"

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
constexpr uint32_t CODE_MIN = 0;
constexpr uint32_t CODE_MAX = static_cast<uint32_t>(UdmfServiceInterfaceCode::CODE_BUTT) + 1;
constexpr size_t NUM_MIN = 5;
constexpr size_t NUM_MAX = 12;
static constexpr int ID_LEN = 32;
static constexpr int MINIMUM = 48;

QueryOption GenerateFuzzQueryOption(FuzzedDataProvider &provider)
{
    std::vector<uint8_t> groupId(ID_LEN, '0');
    for (size_t i = 0; i < groupId.size(); ++i) {
        groupId[i] = provider.ConsumeIntegralInRange<uint8_t>(MINIMUM, MINIMUM);
    }
    std::string groupIdStr(groupId.begin(), groupId.end());
    UnifiedKey udKey = UnifiedKey("drag", "com.test.demo", groupIdStr);
    QueryOption query = {
        .key = udKey.GetUnifiedKey(),
        .intention = Intention::UD_INTENTION_DRAG
    };
    return query;
}

bool OnRemoteRequestFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });

    uint32_t code = provider.ConsumeIntegralInRange<uint32_t>(CODE_MIN, CODE_MAX);
    std::vector<uint8_t> remainingData  = provider.ConsumeRemainingBytes<uint8_t>();
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(static_cast<void *>(remainingData .data()), remainingData .size());
    request.RewindRead(0);
    MessageParcel reply;
    udmfServiceImpl->OnRemoteRequest(code, request, reply);
    return true;
}

void SetDataFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    CustomOption option1 = {.intention = Intention::UD_INTENTION_DRAG};

    std::string svalue = provider.ConsumeRandomLengthString();
    UnifiedData data1;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[FILE_URI_PARAM] = svalue;
    obj->value_[FILE_TYPE] = "abcdefg";
    auto record = std::make_shared<UnifiedRecord>(FILE_URI, obj);
    data1.AddRecord(record);
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, option1, data1);
    MessageParcel reply;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::SET_DATA), request, reply);
}

void GetDataFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(provider);
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, query);
    MessageParcel reply;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::GET_DATA), request, reply);
}

void GetBatchDataFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(provider);
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, query);
    MessageParcel reply;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::GET_BATCH_DATA), request, reply);
}

void UpdateDataFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(provider);
    std::string svalue = provider.ConsumeRandomLengthString();
    UnifiedData data1;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[FILE_URI_PARAM] = svalue;
    obj->value_[FILE_TYPE] = "abcdefg";
    auto record = std::make_shared<UnifiedRecord>(FILE_URI, obj);
    data1.AddRecord(record);
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, query, data1);
    MessageParcel reply;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::UPDATE_DATA), request, reply);
}

void DeleteDataFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(provider);
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, query);
    MessageParcel reply;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::DELETE_DATA), request, reply);
}

void GetSummaryFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(provider);
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, query);
    MessageParcel reply;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::GET_SUMMARY), request, reply);
}

void AddPrivilegeDataFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(provider);

    Privilege privilege = {
        .tokenId = 1,
        .readPermission = "read",
        .writePermission = "write"
    };

    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, query, privilege);
    MessageParcel replyUpdate;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::ADD_PRIVILEGE),
        request, replyUpdate);
}

void SyncDataFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(provider);
    std::vector<std::string> devices = { "11", "22" };
    MessageParcel requestUpdate;
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(requestUpdate, query, devices);
    MessageParcel replyUpdate;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::SYNC),
        requestUpdate, replyUpdate);
}

void IsRemoteDataFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(provider);
    MessageParcel requestUpdate;
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(requestUpdate, query);
    MessageParcel replyUpdate;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::IS_REMOTE_DATA),
        requestUpdate, replyUpdate);
}

void ObtainAsynProcessFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::vector<uint8_t> groupId(ID_LEN, '0');
    for (size_t i = 0; i < groupId.size(); ++i) {
        groupId[i] = provider.ConsumeIntegralInRange<uint8_t>(MINIMUM, MINIMUM);
    }
    std::string businessUdKey(groupId.begin(), groupId.end());
    AsyncProcessInfo processInfo = {
        .businessUdKey = businessUdKey,
    };
    MessageParcel requestUpdate;
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(requestUpdate, processInfo);
    MessageParcel replyUpdate;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::OBTAIN_ASYN_PROCESS),
        requestUpdate, replyUpdate);
}

void ClearAsynProcessFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
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
    OHOS::OnRemoteRequestFuzz(provider);
    OHOS::SetDataFuzz(provider);
    OHOS::GetDataFuzz(provider);

    OHOS::GetBatchDataFuzz(provider);
    OHOS::UpdateDataFuzz(provider);
    OHOS::DeleteDataFuzz(provider);

    OHOS::GetSummaryFuzz(provider);
    OHOS::AddPrivilegeDataFuzz(provider);
    OHOS::SyncDataFuzz(provider);

    OHOS::IsRemoteDataFuzz(provider);
    OHOS::ObtainAsynProcessFuzz(provider);
    OHOS::ClearAsynProcessFuzz(provider);

    return 0;
}