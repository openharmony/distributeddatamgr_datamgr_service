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

#include "objectserviceimp2_fuzzer.h"
#include "object_service_impl.h"
#include "account/account_delegate.h"

using namespace OHOS::DistributedObject;

namespace OHOS {
    
void RegisterDataObserverFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    sptr<IRemoteObject> callback;
    std::string bundleName = provider.ConsumeRandomLengthString(100);
    std::string sessionId = provider.ConsumeRandomLengthString(100);
    objectServiceImpl->RegisterDataObserver(bundleName, sessionId, callback);
}

void OnAppUninstallFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    std::string bundleName = provider.ConsumeRandomLengthString(100);
    int32_t user = provider.ConsumeIntegral<int32_t>();
    int32_t index = provider.ConsumeIntegral<int32_t>();
    objectServiceImpl->factory_.staticActs_->OnAppUninstall(bundleName, user, index);
}

void ResolveAutoLaunchFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    std::string identifier = provider.ConsumeRandomLengthString(100);
    DistributedDB::AutoLaunchParam param;
    objectServiceImpl->ResolveAutoLaunch(identifier, param);
}

void OnAppExitFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    pid_t uid = provider.ConsumeIntegral<uint32_t>();
    pid_t pid = provider.ConsumeIntegral<uint32_t>();
    uint32_t tokenId = provider.ConsumeIntegral<uint32_t>();
    std::string bundleName = provider.ConsumeRandomLengthString(100);
    objectServiceImpl->OnAppExit(uid, pid, tokenId, bundleName);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::RegisterDataObserverFuzzTest(provider);
    OHOS::OnAppUninstallFuzzTest(provider);
    OHOS::ResolveAutoLaunchFuzzTest(provider);
    OHOS::OnAppExitFuzzTest(provider);
    return 0;
}