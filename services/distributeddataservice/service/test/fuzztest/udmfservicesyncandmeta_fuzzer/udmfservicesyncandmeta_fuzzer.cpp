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

#include "udmfservicesyncandmeta_fuzzer.h"

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

void SyncDataFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind({ "UdmfServiceSyncAndMetaFuzzTest",
        static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    QueryOption query = GenerateFuzzQueryOption(provider);
    std::vector<std::string> devices = { "11", "22" };
    MessageParcel requestUpdate;
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(requestUpdate, query, devices);
    MessageParcel replyUpdate;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::SYNC),
        requestUpdate, replyUpdate);
    udmfServiceImpl->OnBind(
        { "UdmfServiceSyncAndMetaFuzzTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), nullptr });
    executor = nullptr;
}

void StoreSyncFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::vector<uint8_t> groupId(ID_LEN, '0');
    for (size_t i = 0; i < groupId.size(); ++i) {
        groupId[i] = provider.ConsumeIntegralInRange<uint8_t>(MINIMUM, MAXIMUM);
    }
    std::string groupIdStr(groupId.begin(), groupId.end());
    UnifiedKey key = UnifiedKey("drag", "com.test.demo", groupIdStr);
    QueryOption query;
    query.key = key.GetUnifiedKey();
    query.intention = Intention::UD_INTENTION_DRAG;
    query.tokenId = provider.ConsumeIntegral<uint32_t>();
    std::vector<std::string> devices = { "11", "22" };
    udmfServiceImpl->StoreSync(key, query, devices);
}

void BuildMetaFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::string storeId = provider.ConsumeRandomLengthString();
    int userId = provider.ConsumeIntegral<int>();
    udmfServiceImpl->BuildMeta(storeId, userId);
}

void IsNeedMetaSyncFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    DistributedData::StoreMetaData meta;
    uint8_t len = provider.ConsumeIntegral<uint8_t>();
    std::vector<std::string> uuids(len);
    for (int i = 0; i < len; i++) {
        std::string uuid = provider.ConsumeRandomLengthString();
        uuids[i] = uuid;
    }
    udmfServiceImpl->IsNeedMetaSync(meta, uuids);
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
    OHOS::SyncDataFuzz(provider);
    OHOS::StoreSyncFuzz(provider);
    OHOS::BuildMetaFuzz(provider);
    OHOS::IsNeedMetaSyncFuzz(provider);
    return 0;
}