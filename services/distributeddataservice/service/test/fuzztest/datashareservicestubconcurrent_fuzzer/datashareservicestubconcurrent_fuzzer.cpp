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

#include "datashareservicestubconcurrent_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "dataproxy_handle_common.h"
#include "data_share_service_impl.h"
#include "ipc_skeleton.h"
#include "itypes_util.h"
#include "log_print.h"
#include "message_parcel.h"
#include "securec.h"

using namespace OHOS::DataShare;

namespace OHOS {
constexpr size_t NUM_MIN = 5;
constexpr size_t NUM_MAX = 12;
constexpr uint32_t CODE_MIN = 0;
constexpr uint32_t CODE_MAX = static_cast<uint32_t>(IDataShareService::DATA_SHARE_SERVICE_CMD_MAX) + 1;

static std::shared_ptr<DataShareServiceImpl> g_dataShareServiceImpl = nullptr;

void GetFuzzDataInsertEx(FuzzedDataProvider &provider, MessageParcel &request)
{
    std::string uri = provider.ConsumeRandomLengthString();
    std::string extUri = provider.ConsumeRandomLengthString();
    DataShareValueObject::Type intValue = provider.ConsumeIntegral<int64_t>();
    std::map<std::string, DataShareValueObject::Type> values;
    values["int_key"] = intValue;
    DataShareValuesBucket bucket(values);
    ITypesUtil::Marshal(request, uri, extUri, bucket.valuesMap);
}

void GetFuzzDataUpdateEx(FuzzedDataProvider &provider, MessageParcel &request)
{
    std::string uri = provider.ConsumeRandomLengthString();
    std::string extUri = provider.ConsumeRandomLengthString();
    DataSharePredicates predicate;
    DataShareValueObject::Type intValue = provider.ConsumeIntegral<int64_t>();
    std::map<std::string, DataShareValueObject::Type> values;
    values["int_key"] = intValue;
    DataShareValuesBucket bucket(values);
    ITypesUtil::Marshal(request, uri, extUri, predicate, bucket.valuesMap);
}

void GetFuzzDataDeleteEx(FuzzedDataProvider &provider, MessageParcel &request)
{
    std::string uri = provider.ConsumeRandomLengthString();
    std::string extUri = provider.ConsumeRandomLengthString();
    DataSharePredicates predicate;
    ITypesUtil::Marshal(request, uri, extUri, predicate);
}

void GetFuzzDataQuery(FuzzedDataProvider &provider, MessageParcel &request)
{
    std::string uri = provider.ConsumeRandomLengthString();
    std::string extUri = provider.ConsumeRandomLengthString();
    DataSharePredicates predicate;
    uint8_t len = provider.ConsumeIntegral<uint8_t>();
    std::vector<std::string> columns(len);
    for (uint8_t i = 0; i < len; i++) {
        std::string column = provider.ConsumeRandomLengthString();
        columns.emplace_back(column);
    }
    ITypesUtil::Marshal(request, uri, extUri, predicate, columns);
}

void GetFuzzDataAddTemplate(FuzzedDataProvider &provider, MessageParcel &request)
{
    std::string uri = provider.ConsumeRandomLengthString();
    int64_t subscriberId = provider.ConsumeIntegral<int64_t>();
    std::string update = provider.ConsumeRandomLengthString();
    std::vector<PredicateTemplateNode> predicates;
    std::string scheduler = provider.ConsumeRandomLengthString();
    Template tpl(update, predicates, scheduler);
    ITypesUtil::Marshal(request, uri, subscriberId, tpl.update_, tpl.predicates_,  tpl.scheduler_);
}

void GetFuzzDataDelTemplate(FuzzedDataProvider &provider, MessageParcel &request)
{
    std::string uri = provider.ConsumeRandomLengthString();
    int64_t subscriberId = provider.ConsumeIntegral<int64_t>();
    ITypesUtil::Marshal(request, uri, subscriberId);
}

void GetFuzzDataPublish(FuzzedDataProvider &provider, MessageParcel &request)
{
    Data publishData;
    publishData.version_ = provider.ConsumeIntegral<int>();
    std::string bundleName = provider.ConsumeRandomLengthString();
    ITypesUtil::Marshal(request, publishData.datas_, publishData.version_, bundleName);
}

void GetFuzzDataGetData(FuzzedDataProvider &provider, MessageParcel &request)
{
    std::string bundleName = provider.ConsumeRandomLengthString();
    ITypesUtil::Marshal(request, bundleName);
}

void MarshalRandUri(FuzzedDataProvider &provider, MessageParcel &request)
{
    std::string uri = provider.ConsumeRandomLengthString();
    ITypesUtil::Marshal(request, uri);
}

void GetFuzzDataSetSilentSwitch(FuzzedDataProvider &provider, MessageParcel &request)
{
    std::string uri = provider.ConsumeRandomLengthString();
    bool enable = provider.ConsumeBool();
    ITypesUtil::Marshal(request, uri, enable);
}

void GetFuzzDataSubAndUnsubData(FuzzedDataProvider &provider, MessageParcel &request)
{
    uint8_t len = provider.ConsumeIntegral<uint8_t>();
    std::vector<std::string> uris;
    for (uint8_t i = 0; i < len; i++) {
        std::string uri = provider.ConsumeRandomLengthString();
        uris.emplace_back(uri);
    }
    int64_t subscriberId = provider.ConsumeIntegral<int64_t>();
    ITypesUtil::Marshal(request, uris, subscriberId);
}

DataProxyValue GetRandomDataProxyValue(FuzzedDataProvider &provider)
{
    int index = provider.ConsumeIntegralInRange<int>(VALUE_INT, VALUE_BOOL + 1);
    switch (index) {
        case (VALUE_INT): {
            return provider.ConsumeIntegral<int>();
        }
        case (VALUE_DOUBLE): {
            return provider.ConsumeFloatingPoint<double>();
        }
        case (VALUE_STRING): {
            return provider.ConsumeRandomLengthString();
        }
        case (VALUE_BOOL): {
            return provider.ConsumeBool();
        }
        default: {
            // return int by default
            return provider.ConsumeIntegral<int>();
        }
    }
}

void GetFuzzDataPublishProxyData(FuzzedDataProvider &provider, MessageParcel &request)
{
    uint8_t len = provider.ConsumeIntegral<uint8_t>();
    std::vector<DataShareProxyData> proxyDatas;
    for (uint8_t j = 0; j < len; j++) {
        std::string uri = provider.ConsumeRandomLengthString();
        DataProxyValue val = GetRandomDataProxyValue(provider);
        uint8_t lenAllowList = provider.ConsumeIntegral<uint8_t>();
        std::vector<std::string> allowList;
        for (uint8_t n = 0; n < lenAllowList; n++) {
            allowList.emplace_back(provider.ConsumeRandomLengthString());
        }
        DataShareProxyData value = {uri, val, allowList};
        proxyDatas.emplace_back(value);
    }
    len = provider.ConsumeIntegral<uint8_t>();
    std::vector<std::string> uris(len);
    for (uint8_t i = 0; i < len; i++) {
        std::string uri = provider.ConsumeRandomLengthString();
        uris.emplace_back(uri);
    }
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    ITypesUtil::Marshal(request, proxyDatas, config);
}

void GetFuzzDataDelAndGetProxyData(FuzzedDataProvider &provider, MessageParcel &request)
{
    uint8_t len = provider.ConsumeIntegral<uint8_t>();
    std::vector<std::string> uris(len);
    for (uint8_t i = 0; i < len; i++) {
        std::string uri = provider.ConsumeRandomLengthString();
        uris.emplace_back(uri);
    }
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    ITypesUtil::Marshal(request, uris, config);
}

void GetFuzzDataSubAndUnsubProxyData(FuzzedDataProvider &provider, MessageParcel &request)
{
    uint8_t len = provider.ConsumeIntegral<uint8_t>();
    std::vector<std::string> uris(len);
    for (uint8_t i = 0; i < len; i++) {
        std::string uri = provider.ConsumeRandomLengthString();
        uris.emplace_back(uri);
    }
    ITypesUtil::Marshal(request, uris);
}

void GetFuzzDataCommon(FuzzedDataProvider &provider, MessageParcel &request)
{
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    request.WriteBuffer(static_cast<void *>(remainingData.data()), remainingData.size());
    request.RewindRead(0);
}

using FuzzFunc = void (*)(FuzzedDataProvider &, MessageParcel &);
static FuzzFunc g_codeMap[] = {
    GetFuzzDataQuery,                   // IDataShareService::DATA_SHARE_SERVICE_CMD_QUERY
    GetFuzzDataAddTemplate,             // IDataShareService::DATA_SHARE_SERVICE_CMD_ADD_TEMPLATE
    GetFuzzDataDelTemplate,             // IDataShareService::DATA_SHARE_SERVICE_CMD_DEL_TEMPLATE
    GetFuzzDataPublish,                 // IDataShareService::DATA_SHARE_SERVICE_CMD_PUBLISH
    GetFuzzDataGetData,                 // IDataShareService::DATA_SHARE_SERVICE_CMD_GET_DATA
    GetFuzzDataSubAndUnsubData,         // IDataShareService::DATA_SHARE_SERVICE_CMD_SUBSCRIBE_RDB
    GetFuzzDataSubAndUnsubData,         // IDataShareService::DATA_SHARE_SERVICE_CMD_UNSUBSCRIBE_RDB
    GetFuzzDataSubAndUnsubData,         // IDataShareService::DATA_SHARE_SERVICE_CMD_ENABLE_SUBSCRIBE_RDB
    GetFuzzDataSubAndUnsubData,         // IDataShareService::DATA_SHARE_SERVICE_CMD_DISABLE_SUBSCRIBE_RDB
    GetFuzzDataSubAndUnsubData,         // IDataShareService::DATA_SHARE_SERVICE_CMD_SUBSCRIBE_PUBLISHED
    GetFuzzDataSubAndUnsubData,         // IDataShareService::DATA_SHARE_SERVICE_CMD_UNSUBSCRIBE_PUBLISHED
    GetFuzzDataSubAndUnsubData,         // IDataShareService::DATA_SHARE_SERVICE_CMD_ENABLE_SUBSCRIBE_PUBLISHED
    GetFuzzDataSubAndUnsubData,         // IDataShareService::DATA_SHARE_SERVICE_CMD_DISABLE_SUBSCRIBE_PUBLISHED
    GetFuzzDataCommon,                  // IDataShareService::DATA_SHARE_SERVICE_CMD_NOTIFY
    MarshalRandUri,                     // IDataShareService::DATA_SHARE_SERVICE_CMD_NOTIFY_OBSERVERS
    GetFuzzDataSetSilentSwitch,         // IDataShareService::DATA_SHARE_SERVICE_CMD_SET_SILENT_SWITCH
    MarshalRandUri,                     // IDataShareService::DATA_SHARE_SERVICE_CMD_GET_SILENT_PROXY_STATUS
    GetFuzzDataCommon,                  // IDataShareService::DATA_SHARE_SERVICE_CMD_REGISTER_OBSERVER
    GetFuzzDataCommon,                  // IDataShareService::DATA_SHARE_SERVICE_CMD_UNREGISTER_OBSERVER
    GetFuzzDataInsertEx,                // IDataShareService::DATA_SHARE_SERVICE_CMD_INSERTEX
    GetFuzzDataDeleteEx,                // IDataShareService::DATA_SHARE_SERVICE_CMD_DELETEEX
    GetFuzzDataUpdateEx,                // IDataShareService::DATA_SHARE_SERVICE_CMD_UPDATEEX
    GetFuzzDataPublishProxyData,        // IDataShareService::DATA_SHARE_SERVICE_CMD_PROXY_PUBLISH
    GetFuzzDataDelAndGetProxyData,      // IDataShareService::DATA_SHARE_SERVICE_CMD_PROXY_DELETE
    GetFuzzDataDelAndGetProxyData,      // IDataShareService::DATA_SHARE_SERVICE_CMD_PROXY_GET
    GetFuzzDataSubAndUnsubProxyData,    // IDataShareService::DATA_SHARE_SERVICE_CMD_SUBSCRIBE_PROXY_DATA
    GetFuzzDataSubAndUnsubProxyData,    // IDataShareService::DATA_SHARE_SERVICE_CMD_UNSUBSCRIBE_PROXY_DATA
    GetFuzzDataCommon,                  // IDataShareService::DATA_SHARE_SERVICE_CMD_MAX
};

void GetFuzzData(FuzzedDataProvider &provider, MessageParcel &request, uint32_t code)
{
    if ((code >= 0) && (code < IDataShareService::DATA_SHARE_SERVICE_CMD_MAX)) {
        g_codeMap[code](provider, request);
    } else {
        GetFuzzDataCommon(provider, request);
    }
}

/**
 * @tc.name: OnRemoteRequestConcurrentFuzz
 * @tc.desc: Concurrent fuzz on OnRemoteRequest method
 * @tc.type: FUNC
 * @tc.require: None
 * @tc.precon: DataShareServiceImpl instance is initialized
 * @tc.step:
    1. Prepare random code and data. Code is within the range of valid requests
    2. Call OnRemoteRequest with random code and data
 * @tc.experct:
    1. Program runs safely without segmentation fault
 */
bool OnRemoteRequestConcurrentFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = g_dataShareServiceImpl;
    if (dataShareServiceStub == nullptr) {
        ZLOGE("%s OnRemoteRequestConcurrentFuzz stub nullptr", FUZZ_PROJECT_NAME);
        return false;
    }

    // Prepare request
    MessageParcel request;
    request.WriteInterfaceToken(IDataShareService::GetDescriptor());
    uint32_t code = provider.ConsumeIntegralInRange<uint32_t>(CODE_MIN, CODE_MAX);
    GetFuzzData(provider, request, code);

    // Call
    MessageParcel reply;
    dataShareServiceStub->OnRemoteRequest(code, request, reply);
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
// concurrent fuzz entry
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    ZLOGI("%s LLVMFuzzerInitialize start", FUZZ_PROJECT_NAME);
    OHOS::g_dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    if (OHOS::g_dataShareServiceImpl == nullptr) {
        ZLOGE("%s LLVMFuzzerInitialize err, g_dataShareServiceImpl nullptr", FUZZ_PROJECT_NAME);
        return -1;
    }
    std::shared_ptr<OHOS::ExecutorPool> executor = std::make_shared<OHOS::ExecutorPool>(OHOS::NUM_MAX, OHOS::NUM_MIN);
    if (executor == nullptr) {
        ZLOGE("%s LLVMFuzzerInitialize err, executor nullptr", FUZZ_PROJECT_NAME);
        return -1;
    }
    OHOS::g_dataShareServiceImpl->OnBind(
        {"DataShareServiceStubFuzz", static_cast<uint32_t>(OHOS::IPCSkeleton::GetSelfTokenID()), std::move(executor)});
    ZLOGI("%s LLVMFuzzerInitialize end", FUZZ_PROJECT_NAME);
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    ZLOGI("%s LLVMFuzzerTestOneInput start", FUZZ_PROJECT_NAME);
    FuzzedDataProvider provider(data, size);
    OHOS::OnRemoteRequestConcurrentFuzz(provider);
    ZLOGI("%s LLVMFuzzerTestOneInput end", FUZZ_PROJECT_NAME);
    return 0;
}