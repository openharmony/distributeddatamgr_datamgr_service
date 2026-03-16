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

#ifndef OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_SERVICE_ETL_RUNTIME_MANAGER_H
#define OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_SERVICE_ETL_RUNTIME_MANAGER_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "runtime/pipeline_runtime.h"
#include "runtime/transport_types.h"

namespace OHOS::DataMining {
class EtlRuntimeManager final {
public:
    EtlRuntimeManager() = default;
    ~EtlRuntimeManager() = default;

    int32_t RegisterPlugin(const std::string &pluginContent);
    int32_t RegisterPipeline(const std::string &pipelineContent);
    int32_t Dispatch(const DispatchRequest &request);

private:
    bool UpdatePluginsLocked(const PluginDescription &description);
    bool UpdatePipelineLocked(const PipelineDescription &description);
    int32_t BuildRuntime(const std::string &pipelineName, const PipelineDescription &description,
        std::shared_ptr<PipelineRuntime> &runtime);

    std::mutex mutex_;
    std::unordered_map<std::string, PluginDescription> pluginsByOp_;
    std::unordered_map<std::string, PipelineDescription> pipelines_;
    std::unordered_map<std::string, std::shared_ptr<PipelineRuntime>> runtimes_;
};
} // namespace OHOS::DataMining

#endif // OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_SERVICE_ETL_RUNTIME_MANAGER_H
