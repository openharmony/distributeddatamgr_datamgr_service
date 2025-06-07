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
#include "network_delegate.h"
#include "network_delegate_normal_impl.h"
#include "network_delegate_normal_impl.cpp"
#include <unistd.h>

using namespace OHOS::DistributedData;

namespace OHOS {
void GetNetworkTypeFuzz(FuzzedDataProvider &provider)
{
    NetworkDelegateNormalImpl delegate;
    delegate.Init();
    bool retrieve = provider.ConsumeIntegral<bool>();
    delegate.GetNetworkType(retrieve);

    sptr<NetConnCallbackObserver> observer = new (std::nothrow) NetConnCallbackObserver(delegate);
    sptr<NetManagerStandard::NetHandle> netHandle = new (std::nothrow) NetHandle();
    sptr<NetManagerStandard::NetAllCapabilities> netAllCap = new (std::nothrow) NetAllCapabilities();
    observer->NetCapabilitiesChange(netHandle, netAllCap);
    netAllCap->netCaps_.insert(NetManagerStandard::NET_CAPABILITY_VALIDATED);
    observer->NetCapabilitiesChange(netHandle, netAllCap);
    observer->NetAvailable(netHandle);
    observer->NetUnavailable();
}

void GetTaskFuzz(FuzzedDataProvider &provider)
{
    NetworkDelegateNormalImpl delegate;
    delegate.Init();
    uint32_t retry = provider.ConsumeIntegral<uint32_t>();
    delegate.GetTask(retry);

    sptr<NetConnCallbackObserver> observer = new (std::nothrow) NetConnCallbackObserver(delegate);
    sptr<NetManagerStandard::NetHandle> netHandle = new (std::nothrow) NetHandle();
    sptr<NetManagerStandard::NetLinkInfo> info = new (std::nothrow) NetLinkInfo();
    observer->NetConnectionPropertiesChange(netHandle, info);
    observer->NetLost(netHandle);
    delegate.RegOnNetworkChange();
}

void BindExecutorFuzz(FuzzedDataProvider &provider)
{
    NetworkDelegateNormalImpl delegate;
    delegate.Init();
    size_t max = provider.ConsumeIntegralInRange<size_t>(12, 15);
    size_t min = provider.ConsumeIntegralInRange<size_t>(5, 8);
    auto executor = std::make_shared<OHOS::ExecutorPool>(max, min);
    delegate.BindExecutor(executor);
    delegate.IsNetworkAvailable();
}

void NetBlockStatusChangeFuzz(FuzzedDataProvider &provider)
{
    NetworkDelegateNormalImpl delegate;
    delegate.Init();
    sptr<NetConnCallbackObserver> observer = new (std::nothrow) NetConnCallbackObserver(delegate);
    sptr<NetManagerStandard::NetHandle> netHandle = nullptr;
    bool blocked = provider.ConsumeIntegral<bool>();
    observer->NetBlockStatusChange(netHandle, blocked);
    delegate.RefreshNet();
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::GetNetworkTypeFuzz(provider);
    OHOS::GetTaskFuzz(provider);
    OHOS::BindExecutorFuzz(provider);
    OHOS::NetBlockStatusChangeFuzz(provider);
    return 0;
}