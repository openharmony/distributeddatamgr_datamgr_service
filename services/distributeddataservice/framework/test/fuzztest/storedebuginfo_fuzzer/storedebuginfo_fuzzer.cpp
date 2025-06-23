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
#include "storedebuginfo_fuzzer.h"
#include "metadata/store_debug_info.h"

using namespace OHOS::DistributedData;

namespace OHOS {

void StoreDebugInfoFuzz(FuzzedDataProvider &provider)
{
    StoreDebugInfo::FileInfo fileInfo;
    StoreDebugInfo debugInfo;
    std::string fileName = "filename";
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
    node["boolean"] = valueBool;
    node["range"] = valueRange;
    fileInfo.Marshal(node);
    fileInfo.Unmarshal(node);
    debugInfo.Marshal(node);
    debugInfo.Unmarshal(node);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::StoreDebugInfoFuzz(provider);
    return 0;
}