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

#define LOG_TAG "DataMiningPipeRt"
#include "runtime/pipeline_runtime.h"

#include <algorithm>

#include "log_print.h"

namespace OHOS::DataMining {
namespace {
constexpr int32_t E_OK = 0;
constexpr int32_t E_ERROR = -1;

const OPDescription *FindDescription(const PluginDescription &description, const std::string &opName)
{
    for (const auto &item : description.ops) {
        if (item.name == opName) {
            return &item;
        }
    }
    return nullptr;
}

bool AppendUnique(std::vector<std::string> &items, const std::string &value)
{
    if (std::find(items.begin(), items.end(), value) != items.end()) {
        return false;
    }
    items.push_back(value);
    return true;
}

class MergedDataValue final : public QueryInterfaceSupport<MergedDataValue, IRecordBatchView> {
public:
    explicit MergedDataValue(std::vector<std::shared_ptr<DataValue>> values) : values_(std::move(values))
    {
    }

    const std::vector<std::shared_ptr<DataValue>> &GetItems() const override
    {
        return values_;
    }

private:
    std::vector<std::shared_ptr<DataValue>> values_;
};
} // namespace

PipelineRuntime::PipelineRuntime() : loader_(std::make_shared<PluginLoader>())
{
}

PipelineRuntime::NodeNotifier::NodeNotifier(PipelineRuntime &runtime, std::string currentNode)
    : runtime_(runtime), currentNode_(std::move(currentNode))
{
}

void PipelineRuntime::NodeNotifier::Notify(
    std::shared_ptr<Context> context, const std::string &topic, std::shared_ptr<DataValue> data)
{
    runtime_.OnNodeOutput(currentNode_, context, topic, data);
}

int32_t PipelineRuntime::Load(const PipelineDescription &pipeline,
    const std::unordered_map<std::string, PluginDescription> &pluginsByOp)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // Load 做两件事：
    // 1. 用 pipeline tree 展开有向图
    // 2. 为每个 operator/sink 绑定对应的插件端点信息
    pipeline_ = pipeline;
    pluginsByOp_ = pluginsByOp;
    nodes_.clear();
    routes_.clear();
    inputParents_.clear();
    pendingInputs_.clear();
    lastError_ = E_OK;
    return BuildGraph(pipeline_.tree, "");
}

int32_t PipelineRuntime::DispatchFrom(const std::string &fromNode, std::shared_ptr<Context> context,
    const std::string &topic, std::shared_ptr<DataValue> data)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (nodes_.find(fromNode) == nodes_.end()) {
            return E_ERROR;
        }
        lastError_ = E_OK;
    }
    OnNodeOutput(fromNode, context, topic, data);
    std::lock_guard<std::mutex> lock(mutex_);
    return lastError_;
}

const PipelineDescription &PipelineRuntime::GetDescription() const
{
    return pipeline_;
}

int32_t PipelineRuntime::BuildGraph(const OpNode &node, const std::string &parent)
{
    // tree 根节点允许为空壳节点，真正的 source/operator/sink 挂在 children 下。
    if (node.opName.empty()) {
        for (const auto &child : node.children) {
            int32_t status = BuildGraph(child, parent);
            if (status != E_OK) {
                return status;
            }
        }
        return E_OK;
    }

    bool existed = nodes_.find(node.opName) != nodes_.end();
    int32_t status = EnsureNodeBound(node);
    if (status != E_OK) {
        return status;
    }

    if (!parent.empty()) {
        AddRouteLocked(parent, node);
    }

    if (existed) {
        return E_OK;
    }

    for (const auto &child : node.children) {
        status = BuildGraph(child, node.opName);
        if (status != E_OK) {
            return status;
        }
    }
    return E_OK;
}

void PipelineRuntime::AddRouteLocked(const std::string &parent, const OpNode &node)
{
    // routes_ 记录单向边，inputParents_ 记录某个节点的所有上游父节点。
    // 后者主要用于多父场景的 merge。
    auto &routes = routes_[parent];
    auto routeIt = std::find_if(routes.begin(), routes.end(), [&node](const auto &route) {
        return route.child == node.opName;
    });
    if (routeIt == routes.end()) {
        routes.push_back(RouteNode { node.opName, node.output });
    }
    AppendUnique(inputParents_[node.opName], parent);
}

int32_t PipelineRuntime::EnsureNodeBound(const OpNode &node)
{
    auto existed = nodes_.find(node.opName);
    if (existed != nodes_.end()) {
        return E_OK;
    }

    auto pluginIt = pluginsByOp_.find(node.opName);
    if (pluginIt == pluginsByOp_.end()) {
        return E_ERROR;
    }

    const auto *description = FindDescription(pluginIt->second, node.opName);
    if (description == nullptr) {
        return E_ERROR;
    }

    RuntimeNode runtimeNode;
    runtimeNode.name = node.opName;

    if (description->type == "source") {
        // source 节点由 DDMS 驱动，SA 运行时只需要知道它是一个合法入口。
        runtimeNode.type = NodeType::SOURCE;
    } else if (description->type == "operator") {
        runtimeNode.type = NodeType::OPERATOR;
        // operator 的输出仍然通过 notifier 回到当前 runtime，继续沿 routes_ 往后派发。
        runtimeNode.notifier = std::make_shared<NodeNotifier>(*this, node.opName);
        runtimeNode.op = std::make_shared<OperatorProxy>(
            node.opName, BuildEndpointConfig(pluginIt->second), loader_);
    } else if (description->type == "sink") {
        runtimeNode.type = NodeType::SINK;
        runtimeNode.sink = std::make_shared<SinkProxy>(node.opName, BuildEndpointConfig(pluginIt->second), loader_);
    } else {
        return E_ERROR;
    }

    nodes_.emplace(node.opName, std::move(runtimeNode));
    return E_OK;
}

int32_t PipelineRuntime::DispatchToNode(
    const std::string &nodeName, std::shared_ptr<Context> context, std::shared_ptr<DataValue> data)
{
    RuntimeNode node;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodes_.find(nodeName);
        if (it == nodes_.end()) {
            return E_ERROR;
        }
        node = it->second;
    }

    if (node.type == NodeType::OPERATOR && node.op != nullptr && node.notifier != nullptr) {
        return node.op->Process(context, data, node.notifier);
    }
    if (node.type == NodeType::SINK && node.sink != nullptr) {
        return node.sink->Save(context, data);
    }
    return E_ERROR;
}

void PipelineRuntime::OnNodeOutput(
    const std::string &from, std::shared_ptr<Context> context, const std::string &topic, std::shared_ptr<DataValue> data)
{
    // 任意节点输出后都走统一派发逻辑：
    // 1. 查找下游边
    // 2. 如有多父节点先 merge
    // 3. 再把结果交给 operator/sink
    std::vector<RouteNode> nextRoutes;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = routes_.find(from);
        if (it == routes_.end()) {
            return;
        }
        nextRoutes = it->second;
    }

    for (const auto &route : nextRoutes) {
        bool ready = false;
        std::shared_ptr<DataValue> merged;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            merged = MergeInputsLocked(route.child, from, data, ready);
        }
        if (!ready) {
            continue;
        }
        int32_t status = DispatchToNode(route.child, context, merged);
        if (status != E_OK) {
            std::lock_guard<std::mutex> lock(mutex_);
            RecordDispatchErrorLocked(from, route.child, status);
        }
    }
    (void)topic;
}

void PipelineRuntime::RecordDispatchErrorLocked(const std::string &from, const std::string &child, int32_t status)
{
    // 只保留首个错误作为本次 dispatch 的返回值，但日志仍记录具体失败边。
    if (lastError_ == E_OK) {
        lastError_ = status;
    }
    ZLOGE("dispatch failed, from:%{public}s, child:%{public}s, status:%{public}d", from.c_str(), child.c_str(),
        status);
}

std::shared_ptr<DataValue> PipelineRuntime::MergeInputsLocked(
    const std::string &nodeName, const std::string &from, std::shared_ptr<DataValue> data, bool &ready)
{
    ready = false;
    // 单父节点不需要 merge，直接透传即可。
    auto parentIt = inputParents_.find(nodeName);
    if (parentIt == inputParents_.end() || parentIt->second.size() <= 1) {
        ready = true;
        return data;
    }

    pendingInputs_[nodeName][from] = data;
    return BuildMergedValueLocked(nodeName, parentIt->second, ready);
}

std::shared_ptr<DataValue> PipelineRuntime::BuildMergedValueLocked(
    const std::string &nodeName, const std::vector<std::string> &parents, bool &ready)
{
    // 多父节点 merge 拆到独立函数，避免 MergeInputsLocked 同时承担“缓存”和“组装结果”两类职责。
    if (pendingInputs_[nodeName].size() < parents.size()) {
        // 还没等到全部父节点的数据时先缓存，直到最后一个父节点到达再统一放行。
        return nullptr;
    }
    std::vector<std::shared_ptr<DataValue>> values;
    values.reserve(parents.size());
    for (const auto &parent : parents) {
        auto pending = pendingInputs_[nodeName].find(parent);
        if (pending != pendingInputs_[nodeName].end()) {
            values.push_back(pending->second);
        }
    }
    pendingInputs_[nodeName].clear();
    ready = values.size() == parents.size();
    if (!ready) {
        return nullptr;
    }
    return std::make_shared<MergedDataValue>(std::move(values));
}
} // namespace OHOS::DataMining
