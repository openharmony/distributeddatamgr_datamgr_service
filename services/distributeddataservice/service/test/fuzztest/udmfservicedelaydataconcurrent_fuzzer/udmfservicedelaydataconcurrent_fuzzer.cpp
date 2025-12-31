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

#include "udmfservicedelaydataconcurrent_fuzzer.h"

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
static constexpr int USERID = 100;
static constexpr int INSTINDEX = 0;
static constexpr const char *BUNDLENAME = "com.test.demo";
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

void SetDelayInfoFuzz(FuzzedDataProvider &provider)
{
    MessageParcel requestUpdate;
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    DataLoadInfo dataLoadInfo;
    dataLoadInfo.recordCount = provider.ConsumeIntegral<uint32_t>();
    dataLoadInfo.types.emplace(provider.ConsumeRandomLengthString());
    dataLoadInfo.sequenceKey = provider.ConsumeRandomLengthString();
    ITypesUtil::Marshal(requestUpdate, dataLoadInfo);
    sptr<IRemoteObject> iUdmfNotifier = nullptr;
    ITypesUtil::Marshal(requestUpdate, iUdmfNotifier);
    MessageParcel replyUpdate;
    std::shared_ptr<UdmfServiceStub> udmfServiceStub = g_udmfServiceImpl;
    if (udmfServiceStub == nullptr) {
        return;
    }
    udmfServiceStub->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::SET_DELAY_INFO),
        requestUpdate, replyUpdate);
}

void PushDelayDataFuzz(FuzzedDataProvider &provider)
{
    MessageParcel requestUpdate;
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    std::string key = provider.ConsumeRandomLengthString();
    ITypesUtil::Marshal(requestUpdate, key);
    UnifiedData data;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[FILE_URI_PARAM] = provider.ConsumeRandomLengthString();
    obj->value_[FILE_TYPE] = provider.ConsumeRandomLengthString();
    auto record = std::make_shared<UnifiedRecord>(FILE_URI, obj);
    data.AddRecord(record);
    ITypesUtil::Marshal(requestUpdate, data);

    MessageParcel replyUpdate;
    std::shared_ptr<UdmfServiceStub> udmfServiceStub = g_udmfServiceImpl;
    if (udmfServiceStub == nullptr) {
        return;
    }
    udmfServiceStub->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::SET_DELAY_DATA),
        requestUpdate, replyUpdate);
}

void GetDataIfAvailableFuzz(FuzzedDataProvider &provider)
{
    MessageParcel requestUpdate;
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    std::string key = provider.ConsumeRandomLengthString();
    ITypesUtil::Marshal(requestUpdate, key);
    DataLoadInfo dataLoadInfo;
    dataLoadInfo.recordCount = provider.ConsumeIntegral<uint32_t>();
    dataLoadInfo.types.emplace(provider.ConsumeRandomLengthString());
    dataLoadInfo.sequenceKey = provider.ConsumeRandomLengthString();
    ITypesUtil::Marshal(requestUpdate, dataLoadInfo);
    sptr<IRemoteObject> iUdmfNotifier = nullptr;
    ITypesUtil::Marshal(requestUpdate, iUdmfNotifier);

    MessageParcel replyUpdate;
    std::shared_ptr<UdmfServiceStub> udmfServiceStub = g_udmfServiceImpl;
    if (udmfServiceStub == nullptr) {
        return;
    }
    udmfServiceStub->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::GET_DELAY_DATA),
        requestUpdate, replyUpdate);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::g_udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    OHOS::Security::AccessToken::AccessTokenID tokenId =
        OHOS::Security::AccessToken::AccessTokenKit::GetHapTokenID(OHOS::USERID, OHOS::BUNDLENAME, OHOS::INSTINDEX);
    SetSelfTokenID(tokenId);
    std::shared_ptr<OHOS::ExecutorPool> executor = std::make_shared<OHOS::ExecutorPool>(OHOS::NUM_MAX, OHOS::NUM_MIN);
    OHOS::g_udmfServiceImpl->OnBind(
        { "UdmfServiceDelayDataConcurrentFuzzTest", static_cast<uint32_t>(OHOS::IPCSkeleton::GetSelfTokenID()),
        std::move(executor) });
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::SetDelayInfoFuzz(provider);
    OHOS::PushDelayDataFuzz(provider);
    OHOS::GetDataIfAvailableFuzz(provider);
    return 0;
}