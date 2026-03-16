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

#define LOG_TAG "DataMiningEtlMgr"
#include "service/etl_runtime_manager.h"

#include "log_print.h"
#include "serializable/serializable.h"

namespace OHOS::DataMining {
namespace {
constexpr int32_t E_OK = 0;
constexpr int32_t E_ERROR = -1;
}

int32_t EtlRuntimeManager::RegisterPlugin(const std::string &pluginContent)
{
    // ETL SA 只保存“按 opName 索引后的插件元数据”。
    // 真正的 operator 和 sink 实例会在 runtime 构图时按需创建，避免 SA 启动时一次性加载全部插件。
    PluginDescription description;
    if (!OHOS::DistributedData::Serializable::Unmarshall(pluginContent, description)) {
        return E_ERROR;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (UpdatePluginsLocked(description)) {
        runtimes_.clear();
    }
    return E_OK;
}

int32_t EtlRuntimeManager::RegisterPipeline(const std::string &pipelineContent)
{
    // pipeline 更新后只在描述真正变化时才丢弃旧 runtime，避免重复注册把缓存打废。
    PipelineDescription description;
    if (!OHOS::DistributedData::Serializable::Unmarshall(pipelineContent, description) || description.name.empty()) {
        return E_ERROR;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (UpdatePipelineLocked(description)) {
        runtimes_.erase(description.name);
    }
    return E_OK;
}

int32_t EtlRuntimeManager::Dispatch(const DispatchRequest &request)
{
    // DDMS 送来的 dispatch 已经确定了入口 source 节点。
    // SA 侧只负责确保 runtime 就绪，然后从 fromNode 往后执行 operator 和 sink。
    std::shared_ptr<PipelineRuntime> runtime;
    PipelineDescription description;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto runtimeIt = runtimes_.find(request.pipelineName);
        if (runtimeIt != runtimes_.end()) {
            runtime = runtimeIt->second;
        }
        auto pipelineIt = pipelines_.find(request.pipelineName);
        if (pipelineIt == pipelines_.end()) {
            return E_ERROR;
        }
        description = pipelineIt->second;
    }

    if (runtime == nullptr) {
        int32_t status = BuildRuntime(request.pipelineName, description, runtime);
        if (status != E_OK || runtime == nullptr) {
            return E_ERROR;
        }
    }

    auto context = std::make_shared<Context>();
    context->SetData(request.contextData);
    context->SetParameters(request.contextParameters);
    auto data = ConvertFromTransportValue(request.value);
    return runtime->DispatchFrom(request.fromNode, context, request.topic, data);
}

bool EtlRuntimeManager::UpdatePluginsLocked(const PluginDescription &description)
{
    bool changed = false;
    const auto serialized = OHOS::DistributedData::Serializable::Marshall(description);
    for (const auto &op : description.ops) {
        auto pluginIt = pluginsByOp_.find(op.name);
        if (pluginIt != pluginsByOp_.end() &&
            OHOS::DistributedData::Serializable::Marshall(pluginIt->second) == serialized) {
            continue;
        }
        pluginsByOp_[op.name] = description;
        changed = true;
    }
    return changed;
}

bool EtlRuntimeManager::UpdatePipelineLocked(const PipelineDescription &description)
{
    auto pipelineIt = pipelines_.find(description.name);
    const auto serialized = OHOS::DistributedData::Serializable::Marshall(description);
    if (pipelineIt != pipelines_.end() &&
        OHOS::DistributedData::Serializable::Marshall(pipelineIt->second) == serialized) {
        return false;
    }
    pipelines_[description.name] = description;
    return true;
}

int32_t EtlRuntimeManager::BuildRuntime(
    const std::string &pipelineName, const PipelineDescription &description, std::shared_ptr<PipelineRuntime> &runtime)
{
    // runtime 按 pipelineName 懒加载缓存，避免每次 dispatch 都重复构图。
    std::unordered_map<std::string, PluginDescription> plugins;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        plugins = pluginsByOp_;
    }

    auto candidate = std::make_shared<PipelineRuntime>();
    int32_t status = candidate->Load(description, plugins);
    if (status != E_OK) {
        ZLOGE("load runtime failed, pipeline:%{public}s, status:%{public}d", pipelineName.c_str(), status);
        return status;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto &slot = runtimes_[pipelineName];
    if (slot == nullptr) {
        slot = candidate;
    }
    runtime = slot;
    return E_OK;
}
} // namespace OHOS::DataMining
