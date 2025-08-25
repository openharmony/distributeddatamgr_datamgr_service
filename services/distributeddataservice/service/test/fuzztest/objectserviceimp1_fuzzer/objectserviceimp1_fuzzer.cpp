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

#include "objectserviceimp1_fuzzer.h"
#include "object_service_impl.h"
#include "account/account_delegate.h"

using namespace OHOS::DistributedObject;

namespace OHOS {

void ObjectStoreSaveFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    sptr<IRemoteObject> callback;
    std::string bundleName = provider.ConsumeRandomLengthString(100);
    std::string sessionId = provider.ConsumeRandomLengthString(100);
    std::string deviceId = provider.ConsumeRandomLengthString(100);
    std::map<std::string, std::vector<uint8_t>> data;
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    data["key1"] = remainingData;
    objectServiceImpl->ObjectStoreSave(bundleName, sessionId, deviceId, data, callback);
}

void OnUserChangeFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    uint32_t code = static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_SWITCHED);
    std::string user = provider.ConsumeRandomLengthString(100);
    std::string account = provider.ConsumeRandomLengthString(100);
    objectServiceImpl->OnUserChange(code, user, account);
}

void ObjectStoreRevokeSaveFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    sptr<IRemoteObject> callback;
    std::string bundleName = provider.ConsumeRandomLengthString(100);
    std::string sessionId = provider.ConsumeRandomLengthString(100);
    objectServiceImpl->ObjectStoreRevokeSave(bundleName, sessionId, callback);
}

void ObjectStoreRetrieveFuzzTest(FuzzedDataProvider &provider)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    sptr<IRemoteObject> callback;
    std::string bundleName = provider.ConsumeRandomLengthString(100);
    std::string sessionId = provider.ConsumeRandomLengthString(100);
    objectServiceImpl->ObjectStoreRetrieve(bundleName, sessionId, callback);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::ObjectStoreSaveFuzzTest(provider);
    OHOS::OnUserChangeFuzzTest(provider);
    OHOS::ObjectStoreRevokeSaveFuzzTest(provider);
    OHOS::ObjectStoreRetrieveFuzzTest(provider);
    return 0;
}