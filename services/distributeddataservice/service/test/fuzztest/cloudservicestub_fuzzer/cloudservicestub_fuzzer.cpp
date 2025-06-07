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

#include "cloudservicestub_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "accesstoken_kit.h"
#include "cloud_service_impl.h"
#include "ipc_skeleton.h"
#include "message_parcel.h"
#include "securec.h"
#include "token_setproc.h"
#include "screen/screen_manager.h"
#include "sync_strategies/network_sync_strategy.h"
#include "metadata/store_debug_info.h"

using namespace OHOS::CloudData;
using namespace OHOS::Security::AccessToken;
using namespace OHOS::DistributedData;

namespace OHOS {
constexpr int USER_ID = 100;
constexpr int INST_INDEX = 0;
const std::u16string INTERFACE_TOKEN = u"OHOS.CloudData.CloudServer";
constexpr uint32_t CODE_MIN = 0;
constexpr uint32_t CODE_MAX = CloudService::TransId::TRANS_BUTT + 1;
constexpr size_t NUM_MIN = 5;
constexpr size_t NUM_MAX = 12;

void AllocAndSetHapToken()
{
    HapInfoParams info = {
        .userID = USER_ID,
        .bundleName = "ohos.test.demo",
        .instIndex = INST_INDEX,
        .appIDDesc = "ohos.test.demo",
        .isSystemApp = true
    };

    HapPolicyParams policy = {
        .apl = APL_NORMAL,
        .domain = "test.domain",
        .permList = {},
        .permStateList = {
            {
                .permissionName = "ohos.permission.CLOUDDATA_CONFIG",
                .isGeneral = true,
                .resDeviceID = { "local" },
                .grantStatus = { PermissionState::PERMISSION_GRANTED },
                .grantFlags = { 1 }
            }
        }
    };
    auto tokenID = AccessTokenKit::AllocHapToken(info, policy);
    SetSelfTokenID(tokenID.tokenIDEx);
}

void TestScreenManager(FuzzedDataProvider &provider)
{
   ScreenManager::GetInstance()->Subscribe(nullptr);
   ScreenManager::GetInstance()->Unsubscribe(nullptr);
   ScreenManager::GetInstance()->BindExecutor(nullptr);
   ScreenManager::GetInstance()->SubscribeScreenEvent();
   ScreenManager::GetInstance()->UnsubscribeScreenEvent();
}

void SyncStrategiesFuzz(FuzzedDataProvider &provider)
{
  int32_t user = provider.ConsumeIntegral<int32_t>();
  std::string bundleName = provider.ConsumeRandomLengthString(); 
  NetworkSyncStrategy strategy;
  StoreInfo storeInfo;
  storeInfo.user = user;
  storeInfo.bundleName = bundleName;
  strategy.CheckSyncAction(storeInfo);
}

void SyncStrategiesFuzz001(FuzzedDataProvider &provider)
{
    uint32_t strategy = provider.ConsumeIntegral<uint32_t>(); 
    NetworkSyncStrategy strategyInstance;
    strategyInstance.Check(strategy);
}

void SyncStrategiesFuzz002(FuzzedDataProvider &provider)
{
    int32_t user = provider.ConsumeIntegral<int32_t>();
    std::string bundleName = provider.ConsumeRandomLengthString();
    NetworkSyncStrategy strategyInstance;
    strategyInstance.GetStrategy(user, bundleName);
    strategyInstance.Getkey(user);
    NetworkSyncStrategy::StrategyInfo info;
    info.user = 1;
    info.bundleName = "StrategyInfo";
    info.Marshal(nullptr);
    Serializable::json node;
    std::string key = provider.ConsumeRandomLengthString();
    std::string valueStr = provider.ConsumeRandomLengthString();
    int valueInt = provider.ConsumeIntegral<int>();
    float valueFloat = provider.ConsumeFloatingPoint<float>();
    bool valueBool = provider.ConsumeBool();
    int valueRange = provider.ConsumeIntegralInRange<int>(0, 100);
    node[key] = valueStr;
    node["integer"] = valueInt;
    node["float"] = valueFloat;
    node["bool"] = valueBool;
    node["range"] = valueRange;
    info.Marshal(node);
    info.Unmarshal(node);
}

void StoreDebugInfoFuzz(FuzzedDataProvider &provider)
{
    StoreDebugInfo::FileInfo fileInfo;
    StoreDebugInfo debugInfo;
    std::string fileName = "fileName";
    debugInfo.fileInfos.insert(std::make_pair(fileName, fileInfo));
    Serializable::json node;
    std::string key = provider.ConsumeRandomLengthString();
    std::string valueStr = provider.ConsumeRandomLengthString();
    int valueInt = provider.ConsumeIntegral<int>();
    float valueFloat = provider.ConsumeFloatingPoint<float>();
    bool valueBool = provider.ConsumeBool();
    int valueRange = provider.ConsumeIntegralInRange<int>(0, 100);
    node[key] = valueStr;
    node["integer"] = valueInt;
    node["float"] = valueFloat;
    node["bool"] = valueBool;
    node["range"] = valueRange;
    fileInfo.Marshal(node);
    fileInfo.Unmarshal(node);
    debugInfo.Marshal(node);
    debugInfo.Unmarshal(node);
}

bool OnRemoteRequestFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<CloudServiceImpl> cloudServiceImpl = std::make_shared<CloudServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    cloudServiceImpl->OnBind(
        { "CloudServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });

    AllocAndSetHapToken();

    uint32_t code = provider.ConsumeIntegralInRange<uint32_t>(CODE_MIN, CODE_MAX);
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(static_cast<void *>(remainingData.data()), remainingData.size());
    request.RewindRead(0);
    MessageParcel reply;
    std::shared_ptr<CloudServiceStub> cloudServiceStub = cloudServiceImpl;
    cloudServiceStub->OnRemoteRequest(code, request, reply);
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::TestScreenManager(provider);
    OHOS::SyncStrategiesFuzz(provider);
    OHOS::SyncStrategiesFuzz001(provider);
    OHOS::SyncStrategiesFuzz002(provider);
    OHOS::StoreDebugInfoFuzz(provider);
    OHOS::OnRemoteRequestFuzz(provider);
    return 0;
}