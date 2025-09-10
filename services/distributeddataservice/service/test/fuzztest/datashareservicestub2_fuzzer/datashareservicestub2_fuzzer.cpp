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

#include "datashareservicestub2_fuzzer.h"

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

bool OnInsertExFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    std::string uri = provider.ConsumeRandomLengthString();
    std::string extUri = provider.ConsumeRandomLengthString();
    DataShareValueObject::Type intValue = provider.ConsumeIntegral<int64_t>();
    std::map<std::string, DataShareValueObject::Type> values;
    values["int_key"] = intValue;
    DataShareValuesBucket bucket(values);
    MessageParcel request;
    request.WriteInterfaceToken(IDataShareService::GetDescriptor());
    ITypesUtil::Marshal(request, uri, extUri, bucket.valuesMap);
    MessageParcel reply;
    dataShareServiceStub->OnRemoteRequest(
        static_cast<uint32_t>(IDataShareService::DATA_SHARE_SERVICE_CMD_INSERTEX), request, reply);
    return true;
}

bool OnUpdateExFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    std::string uri = provider.ConsumeRandomLengthString();
    std::string extUri = provider.ConsumeRandomLengthString();
    DataSharePredicates predicate;
    DataShareValueObject::Type intValue = provider.ConsumeIntegral<int64_t>();
    std::map<std::string, DataShareValueObject::Type> values;
    values["int_key"] = intValue;
    DataShareValuesBucket bucket(values);
    MessageParcel request;
    request.WriteInterfaceToken(IDataShareService::GetDescriptor());
    ITypesUtil::Marshal(request, uri, extUri, predicate, bucket.valuesMap);
    MessageParcel reply;
    dataShareServiceStub->OnRemoteRequest(
        static_cast<uint32_t>(IDataShareService::DATA_SHARE_SERVICE_CMD_UPDATEEX), request, reply);
    return true;
}

bool OnDeleteExFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    std::string uri = provider.ConsumeRandomLengthString();
    std::string extUri = provider.ConsumeRandomLengthString();
    DataSharePredicates predicate;
    MessageParcel request;
    request.WriteInterfaceToken(IDataShareService::GetDescriptor());
    ITypesUtil::Marshal(request, uri, extUri, predicate);
    MessageParcel reply;
    dataShareServiceStub->OnRemoteRequest(
        static_cast<uint32_t>(IDataShareService::DATA_SHARE_SERVICE_CMD_DELETEEX), request, reply);
    return true;
}

bool OnQueryFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    std::string uri = provider.ConsumeRandomLengthString();
    std::string extUri = provider.ConsumeRandomLengthString();
    DataSharePredicates predicate;
    uint8_t len = provider.ConsumeIntegral<uint8_t>();
    std::vector<std::string> columns(len);
    for (int i = 0; i < len; i++) {
        std::string column = provider.ConsumeRandomLengthString();
        columns[i] = column;
    }
    MessageParcel request;
    request.WriteInterfaceToken(IDataShareService::GetDescriptor());
    ITypesUtil::Marshal(request, uri, extUri, predicate, columns);
    MessageParcel reply;
    dataShareServiceStub->OnRemoteRequest(
        static_cast<uint32_t>(IDataShareService::DATA_SHARE_SERVICE_CMD_QUERY), request, reply);
    return true;
}

bool OnAddTemplateFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    std::string uri = provider.ConsumeRandomLengthString();
    int64_t subscriberId = provider.ConsumeIntegral<int64_t>();
    std::string update = provider.ConsumeRandomLengthString();
    std::vector<PredicateTemplateNode> predicates;
    std::string scheduler = provider.ConsumeRandomLengthString();
    Template tpl(update, predicates, scheduler);
    MessageParcel request;
    request.WriteInterfaceToken(IDataShareService::GetDescriptor());
    ITypesUtil::Marshal(request, uri, subscriberId, tpl.update_, tpl.predicates_,  tpl.scheduler_);
    MessageParcel reply;
    dataShareServiceStub->OnRemoteRequest(
        static_cast<uint32_t>(IDataShareService::DATA_SHARE_SERVICE_CMD_ADD_TEMPLATE), request, reply);
    return true;
}

bool OnDelTemplateFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    std::string uri = provider.ConsumeRandomLengthString();
    int64_t subscriberId = provider.ConsumeIntegral<int64_t>();
    MessageParcel request;
    request.WriteInterfaceToken(IDataShareService::GetDescriptor());
    ITypesUtil::Marshal(request, uri, subscriberId);
    MessageParcel reply;
    dataShareServiceStub->OnRemoteRequest(
        static_cast<uint32_t>(IDataShareService::DATA_SHARE_SERVICE_CMD_DEL_TEMPLATE), request, reply);
    return true;
}

bool OnPublishFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    Data publishData;
    publishData.version_ = provider.ConsumeIntegral<int>();
    std::string bundleName = provider.ConsumeRandomLengthString();
    MessageParcel request;
    request.WriteInterfaceToken(IDataShareService::GetDescriptor());
    ITypesUtil::Marshal(request, publishData.datas_, publishData.version_, bundleName);
    MessageParcel reply;
    dataShareServiceStub->OnRemoteRequest(
        static_cast<uint32_t>(IDataShareService::DATA_SHARE_SERVICE_CMD_PUBLISH), request, reply);
    return true;
}

bool OnGetDataFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    std::string bundleName = provider.ConsumeRandomLengthString();
    MessageParcel request;
    request.WriteInterfaceToken(IDataShareService::GetDescriptor());
    ITypesUtil::Marshal(request, bundleName);
    MessageParcel reply;
    dataShareServiceStub->OnRemoteRequest(
        static_cast<uint32_t>(IDataShareService::DATA_SHARE_SERVICE_CMD_GET_DATA), request, reply);
    return true;
}

bool OnNotifyObserverFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    std::string uri = provider.ConsumeRandomLengthString();
    MessageParcel request;
    request.WriteInterfaceToken(IDataShareService::GetDescriptor());
    ITypesUtil::Marshal(request, uri);
    MessageParcel reply;
    dataShareServiceStub->OnRemoteRequest(
        IDataShareService::DATA_SHARE_SERVICE_CMD_NOTIFY_OBSERVERS, request, reply);
    return true;
}

bool OnSetSilentSwitchFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    std::string uri = provider.ConsumeRandomLengthString();
    bool enable = provider.ConsumeBool();
    MessageParcel request;
    request.WriteInterfaceToken(IDataShareService::GetDescriptor());
    ITypesUtil::Marshal(request, uri, enable);
    MessageParcel reply;
    dataShareServiceStub->OnRemoteRequest(
        IDataShareService::DATA_SHARE_SERVICE_CMD_SET_SILENT_SWITCH, request, reply);
    return true;
}

bool OnGetSilentProxyStatusFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    dataShareServiceImpl->OnBind(
        { "DataShareServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    std::string uri = provider.ConsumeRandomLengthString();
    MessageParcel request;
    request.WriteInterfaceToken(IDataShareService::GetDescriptor());
    ITypesUtil::Marshal(request, uri);
    MessageParcel reply;
    dataShareServiceStub->OnRemoteRequest(
        IDataShareService::DATA_SHARE_SERVICE_CMD_GET_SILENT_PROXY_STATUS, request, reply);
    return true;
}

} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::OnInsertExFuzz(provider);
    OHOS::OnUpdateExFuzz(provider);
    OHOS::OnDeleteExFuzz(provider);
    OHOS::OnQueryFuzz(provider);
    OHOS::OnAddTemplateFuzz(provider);
    OHOS::OnDelTemplateFuzz(provider);
    OHOS::OnPublishFuzz(provider);
    OHOS::OnGetDataFuzz(provider);
    OHOS::OnNotifyObserverFuzz(provider);
    OHOS::OnSetSilentSwitchFuzz(provider);
    OHOS::OnGetSilentProxyStatusFuzz(provider);
    return 0;
}