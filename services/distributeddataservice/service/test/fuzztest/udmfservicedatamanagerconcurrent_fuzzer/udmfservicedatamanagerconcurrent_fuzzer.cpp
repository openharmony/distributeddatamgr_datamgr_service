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

#include "udmfservicedatamanagerconcurrent_fuzzer.h"

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
static constexpr int MAXIMUM = 121;
static std::shared_ptr<UdmfServiceImpl> g_udmfServiceImpl = nullptr;

QueryOption GenerateFuzzQueryOption(FuzzedDataProvider &provider)
{
    std::vector<uint8_t> groupId(ID_LEN, '0');
    for (size_t i = 0; i < groupId.size(); ++i) {
        groupId[i] = provider.ConsumeIntegralInRange<uint8_t>(MINIMUM, MAXIMUM);
    }
    std::string groupIdStr(groupId.begin(), groupId.end());
    UnifiedKey udKey = UnifiedKey("drag", "com.test.demo", groupIdStr);
    QueryOption query;
    query.key = udKey.GetUnifiedKey();
    query.intention = Intention::UD_INTENTION_DRAG;
    query.tokenId = provider.ConsumeIntegral<uint32_t>();
    return query;
}

void SetDataFuzz(FuzzedDataProvider &provider)
{
    CustomOption option1 = {.intention = Intention::UD_INTENTION_DRAG};

    std::string svalue = provider.ConsumeRandomLengthString();
    UnifiedData data1;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[FILE_URI_PARAM] = svalue;
    obj->value_[FILE_TYPE] = provider.ConsumeRandomLengthString();
    auto record = std::make_shared<UnifiedRecord>(FILE_URI, obj);
    data1.AddRecord(record);
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, option1, data1);
    MessageParcel reply;
    std::shared_ptr<UdmfServiceStub> udmfServiceStub = g_udmfServiceImpl;
    if (udmfServiceStub == nullptr) {
        return;
    }
    udmfServiceStub->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::SET_DATA), request, reply);
}

void GetDataFuzz(FuzzedDataProvider &provider)
{
    QueryOption query = GenerateFuzzQueryOption(provider);
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, query);
    MessageParcel reply;
    std::shared_ptr<UdmfServiceStub> udmfServiceStub = g_udmfServiceImpl;
    if (udmfServiceStub == nullptr) {
        return;
    }
    udmfServiceStub->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::GET_DATA), request, reply);
}

void UpdateDataFuzz(FuzzedDataProvider &provider)
{
    std::vector<uint8_t> groupId(ID_LEN, '0');
    for (size_t i = 0; i < groupId.size(); ++i) {
        groupId[i] = provider.ConsumeIntegralInRange<uint8_t>(MINIMUM, MAXIMUM);
    }
    std::string groupIdStr(groupId.begin(), groupId.end());
    UnifiedKey udKey = UnifiedKey("DataHub", "com.test.demo", groupIdStr);
    udKey.intention = Intention::UD_INTENTION_DATA_HUB;
    QueryOption query;
    query.key = udKey.GetUnifiedKey();\
    query.intention = Intention::UD_INTENTION_DATA_HUB;
    query.tokenId = provider.ConsumeIntegral<uint32_t>();
    std::string svalue = provider.ConsumeRandomLengthString();
    UnifiedData data1;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[FILE_URI_PARAM] = svalue;
    obj->value_[FILE_TYPE] = provider.ConsumeRandomLengthString();
    auto record = std::make_shared<UnifiedRecord>(FILE_URI, obj);
    data1.AddRecord(record);
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, query, data1);
    MessageParcel reply;
    std::shared_ptr<UdmfServiceStub> udmfServiceStub = g_udmfServiceImpl;
    if (udmfServiceStub == nullptr) {
        return;
    }
    udmfServiceStub->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::UPDATE_DATA), request, reply);
}

void DeleteDataFuzz(FuzzedDataProvider &provider)
{
    QueryOption query = GenerateFuzzQueryOption(provider);
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, query);
    MessageParcel reply;
    std::shared_ptr<UdmfServiceStub> udmfServiceStub = g_udmfServiceImpl;
    if (udmfServiceStub == nullptr) {
        return;
    }
    udmfServiceStub->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::DELETE_DATA), request, reply);
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
        { "UdmfServiceDataManagerConcurrentFuzzTest", static_cast<uint32_t>(OHOS::IPCSkeleton::GetSelfTokenID()),
        std::move(executor) });
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::SetDataFuzz(provider);
    OHOS::GetDataFuzz(provider);
    OHOS::UpdateDataFuzz(provider);
    OHOS::DeleteDataFuzz(provider);
    return 0;
}