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

#include "objectservicelistener_fuzzer.h"
#include "object_data_listener.h"

using namespace OHOS::DistributedObject;

namespace OHOS {
void OnStartFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectAssetsRecvListener> objectAssetsRecvListener = std::make_shared<ObjectAssetsRecvListener>();
    std::string srcNetworkId = provider.ConsumeRandomLengthString(100);
    std::string dstNetworkId = provider.ConsumeRandomLengthString(100);
    std::string sessionId = provider.ConsumeRandomLengthString(100);
    std::string dstBundleName = provider.ConsumeRandomLengthString(100);
    objectAssetsRecvListener->OnStart(srcNetworkId, dstNetworkId, sessionId, dstBundleName);
}

void OnFinishedFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectAssetsRecvListener> objectAssetsRecvListener = std::make_shared<ObjectAssetsRecvListener>();
    std::string srcNetworkId = provider.ConsumeRandomLengthString(100);
    int32_t result = provider.ConsumeIntegral<int32_t>();
    sptr<AssetObj> assetObj = new AssetObj();
    objectAssetsRecvListener->OnFinished(srcNetworkId, assetObj, result);
}

void OnRecvProgressFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectAssetsRecvListener> objectAssetsRecvListener = std::make_shared<ObjectAssetsRecvListener>();
    std::string srcNetworkId = provider.ConsumeRandomLengthString(100);
    uint64_t totalBytes = provider.ConsumeIntegral<uint64_t>();
    uint64_t processBytes = provider.ConsumeIntegral<uint64_t>();
    sptr<AssetObj> assetObj1 = new AssetObj();
    objectAssetsRecvListener->OnRecvProgress(srcNetworkId, assetObj1, totalBytes, processBytes);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::OnStartFuzzTest(provider);
    OHOS::OnFinishedFuzzTest(provider);
    OHOS::OnRecvProgressFuzzTest(provider);
    return 0;
}