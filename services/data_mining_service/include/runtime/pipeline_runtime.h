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

#ifndef OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_RUNTIME_PIPELINE_RUNTIME_H
#define OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_RUNTIME_PIPELINE_RUNTIME_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "etl_interfaces.h"
#include "model/pipeline_description.h"
#include "model/plugin_description.h"
#include "runtime/operator_proxy.h"
#include "runtime/plugin_loader.h"
#include "runtime/sink_proxy.h"

namespace OHOS::DataMining {
class PipelineRuntime final {
public:
    PipelineRuntime();
    ~PipelineRuntime() = default;

    int32_t Load(const PipelineDescription &pipeline,
        const std::unordered_map<std::string, PluginDescription> &pluginsByOp);
    int32_t DispatchFrom(const std::string &fromNode, std::shared_ptr<Context> context, const std::string &topic,
        std::shared_ptr<DataValue> data);
    const PipelineDescription &GetDescription() const;

private:
    enum class NodeType {
        SOURCE = 0,
        OPERATOR,
        SINK,
    };

    struct RuntimeNode final {
        std::string name;
        NodeType type = NodeType::SOURCE;
        std::shared_ptr<OperatorProxy> op;
        std::shared_ptr<SinkProxy> sink;
        std::shared_ptr<AsyncNotifier> notifier;
    };

    class NodeNotifier final : public AsyncNotifier {
    public:
        NodeNotifier(PipelineRuntime &runtime, std::string currentNode);
        ~NodeNotifier() override = default;

        void Notify(std::shared_ptr<Context> context, const std::string &topic,
            std::shared_ptr<DataValue> data) override;

    private:
        PipelineRuntime &runtime_;
        std::string currentNode_;
    };

    struct RouteNode final {
        std::string child;
        std::vector<std::string> outputs;
    };

    int32_t BuildGraph(const OpNode &node, const std::string &parent);
    int32_t EnsureNodeBound(const OpNode &node);
    int32_t DispatchToNode(const std::string &nodeName, std::shared_ptr<Context> context,
        std::shared_ptr<DataValue> data);
    void OnNodeOutput(const std::string &from, std::shared_ptr<Context> context, const std::string &topic,
        std::shared_ptr<DataValue> data);
    std::shared_ptr<DataValue> MergeInputsLocked(const std::string &nodeName, const std::string &from,
        std::shared_ptr<DataValue> data, bool &ready);

    std::shared_ptr<PluginLoader> loader_;
    PipelineDescription pipeline_;
    std::unordered_map<std::string, PluginDescription> pluginsByOp_;
    std::unordered_map<std::string, RuntimeNode> nodes_;
    std::unordered_map<std::string, std::vector<RouteNode>> routes_;
    std::unordered_map<std::string, std::vector<std::string>> inputParents_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::shared_ptr<DataValue>>> pendingInputs_;
    int32_t lastError_ = 0;
    mutable std::mutex mutex_;
};
} // namespace OHOS::DataMining

#endif // OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_RUNTIME_PIPELINE_RUNTIME_H
