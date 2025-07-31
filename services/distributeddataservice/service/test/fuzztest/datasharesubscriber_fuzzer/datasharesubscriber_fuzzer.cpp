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
#define LOG_TAG "DataShareSubscriberFuzzTest"
#include <fuzzer/FuzzedDataProvider.h>

#include "datasharesubscriber_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "log_print.h"
#include "proxy_data_subscriber_manager.h"
#include "ipc_skeleton.h"
#include "message_parcel.h"
#include "securec.h"

using namespace OHOS::DataShare;
namespace OHOS {

void AddFuzz(FuzzedDataProvider &provider)
{
    std::string uri = provider.ConsumeRandomLengthString();
    std::string bundleName = provider.ConsumeRandomLengthString();
    ProxyDataKey key(uri, bundleName);
    sptr<IProxyDataObserver> observer;
    std::string callerAppIdentifier = provider.ConsumeRandomLengthString();
    int32_t userId = provider.ConsumeIntegral<int32_t>();
    auto& manager = ProxyDataSubscriberManager::GetInstance();
    manager.Add(key, observer, bundleName, callerAppIdentifier, userId);
}

void CheckAllowListFuzz(FuzzedDataProvider &provider)
{
    std::string callerAppIdentifier = provider.ConsumeRandomLengthString();
    uint8_t len = provider.ConsumeIntegralInRange<uint8_t>(0, 20);
    std::vector<std::string> allowLists(len);
    for (int i = 0; i < len; i++) {
        std::string allowList = provider.ConsumeRandomLengthString();
        allowLists[i] = allowList;
    }
    auto& manager = ProxyDataSubscriberManager::GetInstance();
    manager.CheckAllowList(allowLists, callerAppIdentifier);
}

void EmitFuzz(FuzzedDataProvider &provider)
{
    uint8_t keyNum = provider.ConsumeIntegralInRange<uint8_t>(0, 20);
    std::vector<ProxyDataKey> keys;
    for (int i = 0; i < keyNum; i++) {
        std::string uri = provider.ConsumeRandomLengthString();
        std::string bundleName = provider.ConsumeRandomLengthString();
        keys.push_back(ProxyDataKey(uri, bundleName));
    }
    std::map<DataShareObserver::ChangeType, std::vector<DataShareProxyData>> datas;
    std::vector<DataShareProxyData> insertData;
    DataShareProxyData data1;
    insertData.push_back(data1);
    datas[DataShareObserver::ChangeType::INSERT] = insertData;
    int32_t userId = provider.ConsumeIntegral<int32_t>();
    auto& manager = ProxyDataSubscriberManager::GetInstance();
    manager.Emit(keys, datas, userId);
}

void ObserverNodeFuzz(FuzzedDataProvider &provider)
{
    sptr<IProxyDataObserver> observer;
    uint32_t tokenId = provider.ConsumeIntegral<uint32_t>();
    std::string name = provider.ConsumeRandomLengthString();
    std::string caller = provider.ConsumeRandomLengthString();
    int32_t userId = provider.ConsumeIntegral<int32_t>();
    ProxyDataSubscriberManager::ObserverNode observerNode(observer, tokenId, name, caller, userId);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::AddFuzz(provider);
    OHOS::CheckAllowListFuzz(provider);
    OHOS::EmitFuzz(provider);
    OHOS::ObserverNodeFuzz(provider);
    return 0;
}