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

#include "objectserviceimp_fuzzer.h"
#include "object_service_impl.h"
#include "account/account_delegate.h"

using namespace OHOS::DistributedObject;

namespace OHOS {

void ObjectStoreSaveFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    sptr<IRemoteObject> callback;
    std::string bundleName = provider.ConsumeRandomLengthString();
    std::string sessionId = provider.ConsumeRandomLengthString();
    std::string deviceId = provider.ConsumeRandomLengthString();
    std::map<std::string, std::vector<uint8_t>> data;
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    data["key1"] = remainingData;
    objectServiceImpl->ObjectStoreSave(bundleName, sessionId, deviceId, data, callback);
}

void SaveMetaDataFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    StoreMetaData saveMeta;
    objectServiceImpl->SaveMetaData(saveMeta);
    auto &dmAdapter = DeviceManagerAdapter::GetInstance();
    std::string uuid = provider.ConsumeRandomLengthString();
    std::string udid = provider.ConsumeRandomLengthString();
    if (uuid.empty()) {
        uuid = "123";
    }
    if (udid.empty()) {
        udid = "234";
    }
    dmAdapter.localInfo_.uuid = uuid;
    dmAdapter.localInfo_.udid = udid;
    objectServiceImpl->SaveMetaData(saveMeta);
}

void OnUserChangeFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    uint32_t code = static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_SWITCHED);
    std::string user = provider.ConsumeRandomLengthString();
    std::string account = provider.ConsumeRandomLengthString();
    objectServiceImpl->OnUserChange(code, user, account);
}

void ObjectStoreRevokeSaveFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    sptr<IRemoteObject> callback;
    std::string bundleName = provider.ConsumeRandomLengthString();
    std::string sessionId = provider.ConsumeRandomLengthString();
    objectServiceImpl->ObjectStoreRevokeSave(bundleName, sessionId, callback);
}

void ObjectStoreRetrieveFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    sptr<IRemoteObject> callback;
    std::string bundleName = provider.ConsumeRandomLengthString();
    std::string sessionId = provider.ConsumeRandomLengthString();
    objectServiceImpl->ObjectStoreRetrieve(bundleName, sessionId, callback);
}

void RegisterDataObserverFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    sptr<IRemoteObject> callback;
    std::string bundleName = provider.ConsumeRandomLengthString();
    std::string sessionId = provider.ConsumeRandomLengthString();
    objectServiceImpl->RegisterDataObserver(bundleName, sessionId, callback);
}

void OnAppUninstallFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    std::string bundleName = provider.ConsumeRandomLengthString();
    int32_t user = provider.ConsumeIntegral<int32_t>();
    int32_t index = provider.ConsumeIntegral<int32_t>();
    objectServiceImpl->OnAppUninstall(bundleName, user, index);
}

void ResolveAutoLaunchFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    std::string identifier = provider.ConsumeRandomLengthString();
    DistributedDB::AutoLaunchParam param;
    objectServiceImpl->ResolveAutoLaunch(identifier, param);
}

void OnAppExitFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    pid_t uid = provider.ConsumeIntegral<uint32_t>();
    pid_t pid = provider.ConsumeIntegral<uint32_t>();
    uint32_t tokenId = provider.ConsumeIntegral<uint32_t>();
    std::string bundleName = provider.ConsumeRandomLengthString();
    objectServiceImpl->OnAppExit(uid, pid, tokenId, bundleName);
    objectServiceImpl->RegisterObjectServiceInfo();
    objectServiceImpl->RegisterHandler();
}

void DumpObjectServiceInfoFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    int fd = provider.ConsumeIntegral<int>();
    std::string key = provider.ConsumeRandomLengthString();
    std::string value1 = provider.ConsumeRandomLengthString();
    std::string value2 = provider.ConsumeRandomLengthString();
    std::map<std::string, std::vector<std::string>> params;
    std::vector<std::string> value = {value1, value2};
    params.emplace(std::make_pair(key, value));
    objectServiceImpl->DumpObjectServiceInfo(fd, params);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::ObjectStoreSaveFuzzTest(provider);
    OHOS::OnUserChangeFuzzTest(provider);
    OHOS::SaveMetaDataFuzzTest(provider);
    OHOS::ObjectStoreRevokeSaveFuzzTest(provider);
    OHOS::ObjectStoreRetrieveFuzzTest(provider);
    OHOS::RegisterDataObserverFuzzTest(provider);
    OHOS::OnAppUninstallFuzzTest(provider);
    OHOS::ResolveAutoLaunchFuzzTest(provider);
    OHOS::OnAppExitFuzzTest(provider);
    OHOS::DumpObjectServiceInfoFuzzTest(provider);
    return 0;
}