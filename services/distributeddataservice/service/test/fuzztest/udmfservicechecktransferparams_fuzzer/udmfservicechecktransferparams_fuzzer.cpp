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

#include "udmfservicechecktransferparams_fuzzer.h"

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
static constexpr int ID_LEN = 32;
static constexpr int MINIMUM = 48;
static constexpr int MAXIMUM = 121;

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

void IsNeedTransferDeviceTypeFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    QueryOption query = GenerateFuzzQueryOption(provider);
    udmfServiceImpl->IsNeedTransferDeviceType(query);
}

void TransferToEntriesIfNeedFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
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
    UnifiedData data;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[FILE_URI_PARAM] = provider.ConsumeRandomLengthString();
    obj->value_[FILE_TYPE] = provider.ConsumeRandomLengthString();
    std::shared_ptr<Object> obj1 = std::make_shared<Object>();
    obj1->value_[UNIFORM_DATA_TYPE] = "general.image-uri";
    obj1->value_[FILE_URI_PARAM] = provider.ConsumeRandomLengthString();
    obj1->value_[FILE_TYPE] = provider.ConsumeRandomLengthString();
    std::shared_ptr<Object> obj2 = std::make_shared<Object>();
    obj2->value_[UNIFORM_DATA_TYPE] = "general.txt-uri";
    obj2->value_[FILE_URI_PARAM] = provider.ConsumeRandomLengthString();
    obj2->value_[FILE_TYPE] = provider.ConsumeRandomLengthString();
    std::shared_ptr<Object> obj3 = std::make_shared<Object>();
    obj3->value_[UNIFORM_DATA_TYPE] = "general.html-uri";
    obj3->value_[FILE_URI_PARAM] = provider.ConsumeRandomLengthString();
    obj3->value_[FILE_TYPE] = provider.ConsumeRandomLengthString();
    auto record = std::make_shared<UnifiedRecord>(FILE_URI, obj);
    auto record1 = std::make_shared<UnifiedRecord>(FILE_URI, obj1);
    auto record2 = std::make_shared<UnifiedRecord>(FILE_URI, obj2);
    auto record3 = std::make_shared<UnifiedRecord>(FILE_URI, obj3);
    data.AddRecord(record);
    data.AddRecord(record1);
    data.AddRecord(record2);
    data.AddRecord(record3);
    udmfServiceImpl->TransferToEntriesIfNeed(query, data);
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
    OHOS::IsNeedTransferDeviceTypeFuzz(provider);
    OHOS::TransferToEntriesIfNeedFuzz(provider);
    return 0;
}