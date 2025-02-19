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

QueryOption GenerateFuzzQueryOption(const uint8_t* data, size_t size)
{
    std::vector<uint8_t> groupId(ID_LEN, '0');
    size_t length = groupId.size() > size ? size : groupId.size();
    for (size_t i = 0; i < length; ++i) {
        groupId[i] = data[i] % MINIMUM + MINIMUM;
    }
    std::string groupIdStr(groupId.begin(), groupId.end());
    UnifiedKey udKey = UnifiedKey("drag", "com.test.demo", groupIdStr);
    QueryOption query = {
        .key = udKey.GetUnifiedKey(),
        .intention = Intention::UD_INTENTION_DRAG
    };
    return query;
}

bool OnRemoteRequestFuzz(const uint8_t* data, size_t size)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });

    uint32_t code = static_cast<uint32_t>(*data) % (CODE_MAX - CODE_MIN + 1) + CODE_MIN;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;
    udmfServiceImpl->OnRemoteRequest(code, request, reply);
    return true;
}

void SetDataFuzz(const uint8_t *data, size_t size)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    CustomOption option1 = {.intention = Intention::UD_INTENTION_DRAG};
    std::string svalue(data, data + size);
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

void GetDataFuzz(const uint8_t *data, size_t size)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(data, size);
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, query);
    MessageParcel reply;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::GET_DATA), request, reply);
}

void GetBatchDataFuzz(const uint8_t *data, size_t size)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(data, size);
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, query);
    MessageParcel reply;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::GET_BATCH_DATA), request, reply);
}

void UpdateDataFuzz(const uint8_t *data, size_t size)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(data, size);
    std::string svalue(data, data + size);
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

void DeleteDataFuzz(const uint8_t *data, size_t size)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(data, size);
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, query);
    MessageParcel reply;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::DELETE_DATA), request, reply);
}

void GetSummaryFuzz(const uint8_t *data, size_t size)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(data, size);
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, query);
    MessageParcel reply;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::GET_SUMMARY), request, reply);
}

void AddPrivilegeDataFuzz(const uint8_t *data, size_t size)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(data, size);

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

void SyncDataFuzz(const uint8_t *data, size_t size)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(data, size);
    std::vector<std::string> devices = { "11", "22" };
    MessageParcel requestUpdate;
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(requestUpdate, query, devices);
    MessageParcel replyUpdate;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::SYNC),
        requestUpdate, replyUpdate);
}

void IsRemoteDataFuzz(const uint8_t *data, size_t size)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(data, size);
    MessageParcel requestUpdate;
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(requestUpdate, query);
    MessageParcel replyUpdate;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::IS_REMOTE_DATA),
        requestUpdate, replyUpdate);
}

void ObtainAsynProcessFuzz(const uint8_t *data, size_t size)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::vector<uint8_t> groupId(ID_LEN, '0');
    size_t length = groupId.size() > size ? size : groupId.size();
    for (size_t i = 0; i < length; ++i) {
        groupId[i] = data[i] % MINIMUM + MINIMUM;
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

void ClearAsynProcessFuzz(const uint8_t *data, size_t size)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::vector<uint8_t> groupId(ID_LEN, '0');
    size_t length = groupId.size() > size ? size : groupId.size();
    for (size_t i = 0; i < length; ++i) {
        groupId[i] = data[i] % MINIMUM + MINIMUM;
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
    if (data == nullptr) {
        return 0;
    }

    OHOS::OnRemoteRequestFuzz(data, size);
    OHOS::SetDataFuzz(data, size);
    OHOS::GetDataFuzz(data, size);

    OHOS::GetBatchDataFuzz(data, size);
    OHOS::UpdateDataFuzz(data, size);
    OHOS::DeleteDataFuzz(data, size);

    OHOS::GetSummaryFuzz(data, size);
    OHOS::AddPrivilegeDataFuzz(data, size);
    OHOS::SyncDataFuzz(data, size);

    OHOS::IsRemoteDataFuzz(data, size);
    OHOS::ObtainAsynProcessFuzz(data, size);
    OHOS::ClearAsynProcessFuzz(data, size);

    return 0;
}