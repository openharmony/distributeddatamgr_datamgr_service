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

#include "datashareservicestub3_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "data_share_service_impl.h"
#include "ipc_skeleton.h"
#include "itypes_util.h"
#include "message_parcel.h"
#include "securec.h"

using namespace OHOS::DataShare;

namespace OHOS {
constexpr size_t NUM_MIN = 5;
constexpr size_t NUM_MAX = 12;

bool OnRemoteRequestFuzz(uint32_t code, FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    uint8_t len = provider.ConsumeIntegral<uint8_t>();
    std::vector<std::string> uris(len);
    for (int i = 0; i < len; i++) {
        std::string uri = provider.ConsumeRandomLengthString();
        uris[i] = uri;
    }
    TemplateId templateId;
    MessageParcel request;
    request.WriteInterfaceToken(IDataShareService::GetDescriptor());
    ITypesUtil::Marshal(request, uris, templateId);
    MessageParcel reply;
    dataShareServiceStub->OnRemoteRequest(code, request, reply);
    return true;
}

bool OnSubscribePublishedDataFuzz(uint32_t code, FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    uint8_t len = provider.ConsumeIntegral<uint8_t>();
    std::vector<std::string> uris(len);
    for (int i = 0; i < len; i++) {
        std::string uri = provider.ConsumeRandomLengthString();
        uris[i] = uri;
    }
    int64_t subscriberId = provider.ConsumeIntegral<int64_t>();
    MessageParcel request;
    request.WriteInterfaceToken(IDataShareService::GetDescriptor());
    ITypesUtil::Marshal(request, uris, subscriberId);
    MessageParcel reply;
    dataShareServiceStub->OnRemoteRequest(code, request, reply);
    return true;
}

bool OnDeleteProxyDataFuzz(uint32_t code, FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    uint8_t len = provider.ConsumeIntegral<uint8_t>();
    std::vector<std::string> uris(len);
    for (int i = 0; i < len; i++) {
        std::string uri = provider.ConsumeRandomLengthString();
        uris[i] = uri;
    }
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    MessageParcel request;
    request.WriteInterfaceToken(IDataShareService::GetDescriptor());
    ITypesUtil::Marshal(request, uris, config);
    MessageParcel reply;
    dataShareServiceStub->OnRemoteRequest(code, request, reply);
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    uint32_t codeTest = IDataShareService::DATA_SHARE_SERVICE_CMD_SUBSCRIBE_RDB;
    while (codeTest <= IDataShareService::DATA_SHARE_SERVICE_CMD_DISABLE_SUBSCRIBE_PUBLISHED) {
        OHOS::OnRemoteRequestFuzz(codeTest, provider);
        codeTest++;
    }

    codeTest = IDataShareService::DATA_SHARE_SERVICE_CMD_SUBSCRIBE_PUBLISHED;
    while (codeTest <= IDataShareService::DATA_SHARE_SERVICE_CMD_DISABLE_SUBSCRIBE_PUBLISHED) {
        OHOS::OnSubscribePublishedDataFuzz(codeTest, provider);
        codeTest++;
    }

    codeTest = IDataShareService::DATA_SHARE_SERVICE_CMD_PROXY_DELETE;
    while (codeTest <= IDataShareService::DATA_SHARE_SERVICE_CMD_UNSUBSCRIBE_PROXY_DATA) {
        OHOS::OnDeleteProxyDataFuzz(codeTest, provider);
        codeTest++;
    }
    return 0;
}