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

#ifndef OHOS_DISTRIBUTEDDATASERVICE_SERVICE_DATA_MINING_SERVICE_ETL_RUNTIME_CLIENT_H
#define OHOS_DISTRIBUTEDDATASERVICE_SERVICE_DATA_MINING_SERVICE_ETL_RUNTIME_CLIENT_H

#include <memory>
#include <mutex>

#include "ipc/etl_service_ipc.h"
#include "model/pipeline_description.h"
#include "model/plugin_description.h"
#include "runtime/transport_types.h"

namespace OHOS::DataMining {
class IEtlRuntimeClient {
public:
    virtual ~IEtlRuntimeClient() = default;

    virtual int32_t RegisterPlugin(const PluginDescription &description) = 0;
    virtual int32_t RegisterPipeline(const PipelineDescription &description) = 0;
    virtual int32_t Dispatch(const DispatchRequest &request) = 0;
};

class SaEtlRuntimeClient final : public IEtlRuntimeClient {
public:
    SaEtlRuntimeClient() = default;
    ~SaEtlRuntimeClient() override = default;

    int32_t RegisterPlugin(const PluginDescription &description) override;
    int32_t RegisterPipeline(const PipelineDescription &description) override;
    int32_t Dispatch(const DispatchRequest &request) override;

private:
    sptr<IEtlService> GetServiceLocked();
    int32_t SendPluginLocked(const PluginDescription &description);
    int32_t SendPipelineLocked(const PipelineDescription &description);
    int32_t SendDispatchLocked(const DispatchRequest &request);

    std::mutex mutex_;
    sptr<IEtlService> service_;
};
} // namespace OHOS::DataMining

#endif // OHOS_DISTRIBUTEDDATASERVICE_SERVICE_DATA_MINING_SERVICE_ETL_RUNTIME_CLIENT_H
