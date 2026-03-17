/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <cstddef>
#include <cstdint>
#include <string>

#include <fuzzer/FuzzedDataProvider.h>

#include "data_mining_service_impl.h"
#include "message_parcel.h"
#include "scene_etl_control.h"

namespace OHOS {
namespace {
constexpr int32_t MAX_CODE = static_cast<int32_t>(DataMining::SceneEtlRequestCode::DISABLE_SCENE) + 1;
constexpr char16_t FEATURE_DESCRIPTOR[] = u"OHOS.DistributedData.ServiceProxy";

DistributedData::DataMiningServiceImpl &GetService()
{
    static DistributedData::DataMiningServiceImpl service;
    return service;
}

void InvokeRequest(uint32_t code, const std::string &scene, bool validToken)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(validToken ? FEATURE_DESCRIPTOR : u"invalid.data_mining.feature") ||
        !data.WriteString(scene)) {
        return;
    }
    GetService().OnRemoteRequest(code, data, reply);
}
} // namespace

void FuzzSceneFeature(FuzzedDataProvider &provider)
{
    auto code = static_cast<uint32_t>(provider.ConsumeIntegralInRange<int32_t>(0, MAX_CODE));
    auto scene = provider.ConsumeRandomLengthString();
    InvokeRequest(code, scene, provider.ConsumeBool());
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    FuzzedDataProvider provider(data, size);
    OHOS::FuzzSceneFeature(provider);
    return 0;
}