/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <cstddef>
#include <cstdint>
#include <string>

#include <fuzzer/FuzzedDataProvider.h>

#include "dataminingetlservicestub_fuzzer.h"
#include "ipc/etl_service_ipc.h"
#include "ipc/etl_service_stub.h"
#include "message_option.h"
#include "message_parcel.h"
#include "serializable/serializable.h"
#include "service/etl_runtime_manager.h"

namespace OHOS {
namespace {
constexpr uint32_t MAX_CODE = static_cast<uint32_t>(DataMining::IEtlService::DISPATCH) + 1;

class FuzzEtlServiceStub final : public DataMining::EtlServiceStub {
public:
    int32_t RegisterPlugin(const std::string &pluginContent) override
    {
        return manager_.RegisterPlugin(pluginContent);
    }

    int32_t RegisterPipeline(const std::string &pipelineContent) override
    {
        return manager_.RegisterPipeline(pipelineContent);
    }

    int32_t Dispatch(const std::string &dispatchContent) override
    {
        DataMining::DispatchRequest request;
        if (!DistributedData::Serializable::Unmarshall(dispatchContent, request)) {
            return DataMining::DATA_MINING_ERROR;
        }
        return manager_.Dispatch(request);
    }

private:
    DataMining::EtlRuntimeManager manager_;
};

FuzzEtlServiceStub &GetStub()
{
    static FuzzEtlServiceStub stub;
    return stub;
}

std::string MaybeCorruptContent(const std::string &content, FuzzedDataProvider &provider)
{
    if (provider.ConsumeBool()) {
        return content;
    }
    auto noise = provider.ConsumeRandomLengthString();
    if (provider.ConsumeBool()) {
        return noise;
    }
    return content + noise;
}

std::string BuildPluginContent(FuzzedDataProvider &provider)
{
    static const std::string plugin = R"({
        "ops":[
            {
                "name":"source_a",
                "type":"source",
                "input":{"typeName":"json","elements":[]},
                "output":{"typeName":"json","elements":[]}
            }
        ],
        "libs":{"path":"libs/libsource_a.so"},
        "extension":{"uri":""},
        "sa":{"said":0,"code":0}
    })";
    return MaybeCorruptContent(plugin, provider);
}

std::string BuildPipelineContent(FuzzedDataProvider &provider)
{
    static const std::string pipeline = R"({
        "name":"pipeline",
        "scene":["test"],
        "trigger":{
            "type":"manual",
            "interval":0,
            "eventType":0,
            "nextOrAnd":"",
            "subscriptions":[],
            "timers":[]
        },
        "tree":{
            "opName":"source_a",
            "mode":"trigger",
            "topic":"topic",
            "children":[],
            "output":[]
        }
    })";
    return MaybeCorruptContent(pipeline, provider);
}

std::string BuildDispatchContent(FuzzedDataProvider &provider)
{
    static const std::string dispatch = R"({
        "pipelineName":"pipeline",
        "fromNode":"source_a",
        "topic":"topic",
        "contextData":"",
        "contextParameters":"",
        "value":{
            "type":1,
            "text":"payload",
            "bytes":[]
        }
    })";
    return MaybeCorruptContent(dispatch, provider);
}

void InvokeRequest(uint32_t code, std::string content, FuzzedDataProvider &provider)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::u16string token = provider.ConsumeBool() ? std::u16string(DataMining::IEtlService::GetDescriptor())
                                                  : u"invalid.etl.service";
    if (!data.WriteInterfaceToken(token) || !data.WriteString(content)) {
        return;
    }
    GetStub().OnRemoteRequest(code, data, reply, option);
}

} // namespace

void FuzzStub(FuzzedDataProvider &provider)
{
    InvokeRequest(DataMining::IEtlService::REGISTER_PLUGIN, BuildPluginContent(provider), provider);
    InvokeRequest(DataMining::IEtlService::REGISTER_PIPELINE, BuildPipelineContent(provider), provider);
    InvokeRequest(provider.ConsumeIntegralInRange<uint32_t>(0, MAX_CODE), BuildDispatchContent(provider), provider);
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return 0;
    }
    FuzzedDataProvider provider(data, size);
    OHOS::FuzzStub(provider);
    return 0;
}
