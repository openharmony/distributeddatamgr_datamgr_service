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

#include "udmfservicecheckkeyandintention_fuzzer.h"

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

void CheckDragParamsFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::string intention = provider.ConsumeRandomLengthString();
    std::string bundleName = provider.ConsumeRandomLengthString();
    std::vector<uint8_t> groupId(ID_LEN, '0');
    for (size_t i = 0; i < groupId.size(); ++i) {
        groupId[i] = provider.ConsumeIntegralInRange<uint8_t>(MINIMUM, MAXIMUM);
    }
    std::string groupIdStr(groupId.begin(), groupId.end());
    UnifiedKey udKey(intention, bundleName, groupIdStr);
    QueryOption query;
    udmfServiceImpl->CheckDragParams(udKey, query);
}

void IsFileMangerIntentionFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    auto intention = provider.ConsumeRandomLengthString();
    udmfServiceImpl->IsFileMangerIntention(intention);
}

void CheckAppIdFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::string intention = provider.ConsumeRandomLengthString();
    std::string bundleName = provider.ConsumeRandomLengthString();
    std::string groupId = provider.ConsumeRandomLengthString();
    Privilege privilege;
    privilege.tokenId = provider.ConsumeIntegral<uint32_t>();
    UnifiedData data;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[FILE_URI_PARAM] = provider.ConsumeRandomLengthString();
    obj->value_[FILE_TYPE] = provider.ConsumeRandomLengthString();
    auto record = std::make_shared<UnifiedRecord>(FILE_URI, obj);
    data.AddRecord(record);
    UnifiedKey key(intention, bundleName, groupId);
    std::shared_ptr<Runtime> runtime = std::make_shared<Runtime>();
    runtime->key = key;
    runtime->privileges.emplace_back(privilege);
    runtime->sourcePackage = bundleName;
    runtime->createPackage = bundleName;
    runtime->recordTotalNum = static_cast<uint32_t>(data.GetRecords().size());
    runtime->tokenId = provider.ConsumeIntegral<uint32_t>();
    runtime->appId = provider.ConsumeRandomLengthString();
    udmfServiceImpl->CheckAppId(runtime, bundleName);
}

void IsValidOptionsNonDragFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::vector<uint8_t> groupId(ID_LEN, '0');
    for (size_t i = 0; i < groupId.size(); ++i) {
        groupId[i] = provider.ConsumeIntegralInRange<uint8_t>(MINIMUM, MAXIMUM);
    }
    std::string groupIdStr(groupId.begin(), groupId.end());
    UnifiedKey key = UnifiedKey("drag", "com.test.demo", groupIdStr);
    CustomOption option = {.intention = Intention::UD_INTENTION_DRAG};
    std::string intention = UD_INTENTION_MAP.at(option.intention);
    udmfServiceImpl->IsValidOptionsNonDrag(key, intention);
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
    OHOS::CheckDragParamsFuzz(provider);
    OHOS::IsFileMangerIntentionFuzz(provider);
    OHOS::CheckAppIdFuzz(provider);
    OHOS::IsValidOptionsNonDragFuzz(provider);
    return 0;
}