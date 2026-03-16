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

#define LOG_TAG "DataMiningMgr"
#include "service/data_mining_manager.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "log_print.h"
#include "serializable/serializable.h"

namespace OHOS::DataMining {
namespace {
constexpr int32_t E_OK = 0;
constexpr int32_t E_ERROR = -1;
constexpr int32_t DEFAULT_DAILY_INTERVAL = 24 * 60 * 60;
constexpr const char *JSON_EXTENSION = ".json";
constexpr const char *SHARED_LIB_EXTENSION = ".so";

const OPDescription *FindDescription(const PluginDescription &description, const std::string &opName)
{
    for (const auto &item : description.ops) {
        if (item.name == opName) {
            return &item;
        }
    }
    return nullptr;
}

bool IsSafeRelativeLibraryPath(const std::filesystem::path &path)
{
    if (path.empty() || path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
        return false;
    }
    if (path.extension().string() != SHARED_LIB_EXTENSION) {
        return false;
    }
    for (const auto &part : path) {
        const auto item = part.string();
        if (item.empty() || item == "." || item == "..") {
            return false;
        }
    }
    return true;
}

bool IsSubPath(const std::filesystem::path &base, const std::filesystem::path &target)
{
    auto baseIt = base.begin();
    auto targetIt = target.begin();
    for (; baseIt != base.end() && targetIt != target.end(); ++baseIt, ++targetIt) {
        if (*baseIt != *targetIt) {
            return false;
        }
    }
    return baseIt == base.end();
}

bool CollectJsonFiles(const std::string &directoryPath, std::vector<std::string> &files)
{
    std::error_code errorCode;
    if (!std::filesystem::exists(directoryPath, errorCode) ||
        !std::filesystem::is_directory(directoryPath, errorCode)) {
        return false;
    }

    for (const auto &entry : std::filesystem::directory_iterator(directoryPath, errorCode)) {
        if (errorCode) {
            break;
        }
        if (!entry.is_regular_file(errorCode)) {
            if (errorCode) {
                break;
            }
            continue;
        }
        const auto &path = entry.path();
        if (path.extension().string() != JSON_EXTENSION) {
            continue;
        }
        files.emplace_back(path.string());
    }
    std::sort(files.begin(), files.end());
    return !errorCode;
}

void CollectSourceNodes(const OpNode &node, const std::unordered_map<std::string, PluginDescription> &pluginsByOp,
    std::unordered_map<std::string, OpNode> &sources)
{
    if (!node.opName.empty()) {
        auto pluginIt = pluginsByOp.find(node.opName);
        if (pluginIt != pluginsByOp.end()) {
            const auto *description = FindDescription(pluginIt->second, node.opName);
            if (description != nullptr && description->type == "source") {
                sources[node.opName] = node;
            }
        }
    }
    for (const auto &child : node.children) {
        CollectSourceNodes(child, pluginsByOp, sources);
    }
}

bool ContainsSubscription(const std::vector<SourceSubscription> &subscriptions, const std::string &sourceName)
{
    return std::any_of(subscriptions.begin(), subscriptions.end(), [&sourceName](const auto &item) {
        return item.sourceName == sourceName;
    });
}

bool ContainsTriggerSource(const std::vector<SourceTimer> &triggerSources, const std::string &sourceName)
{
    return std::any_of(triggerSources.begin(), triggerSources.end(), [&sourceName](const auto &item) {
        return item.sourceName == sourceName;
    });
}

bool HasSubscribeSource(const OpNode &node)
{
    if (node.mode == "subscribe") {
        return true;
    }
    for (const auto &child : node.children) {
        if (HasSubscribeSource(child)) {
            return true;
        }
    }
    return false;
}

bool ShouldAutoStartPipeline(const PipelineDescription &description)
{
    if (!description.trigger.subscriptions.empty() || !description.trigger.timers.empty()) {
        return true;
    }
    if (description.trigger.type == "timer" || description.trigger.type == "hybrid") {
        return true;
    }
    return HasSubscribeSource(description.tree);
}

void AppendTimerTaskIds(const std::unordered_map<std::string, ExecutorPool::TaskId> &timerTaskIds,
    std::vector<ExecutorPool::TaskId> &tasks)
{
    for (const auto &[sourceName, taskId] : timerTaskIds) {
        (void)sourceName;
        if (taskId != ExecutorPool::INVALID_TASK_ID) {
            tasks.push_back(taskId);
        }
    }
}

std::vector<PluginDescription> CollectUniquePlugins(
    const std::unordered_map<std::string, PluginDescription> &pluginsByOp)
{
    std::vector<PluginDescription> plugins;
    std::unordered_set<std::string> sentPlugins;
    for (const auto &[opName, plugin] : pluginsByOp) {
        (void)opName;
        auto key = OHOS::DistributedData::Serializable::Marshall(plugin);
        if (sentPlugins.insert(key).second) {
            plugins.push_back(plugin);
        }
    }
    return plugins;
}
} // namespace

DataMiningManager::DataMiningManager()
    : runtimeClient_(std::make_shared<SaEtlRuntimeClient>()), sourceLoader_(std::make_shared<PluginLoader>())
{
}

DataMiningManager::~DataMiningManager()
{
    std::vector<ExecutorPool::TaskId> timerTaskIds;
    std::vector<std::shared_ptr<SourceProxy>> sources;
    std::shared_ptr<ExecutorPool> executors;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        executors = executors_;
        for (auto &[pipelineName, state] : pipelines_) {
            (void)pipelineName;
            state.started = false;
            AppendTimerTaskIds(state.timerTaskIds, timerTaskIds);
            state.timerTaskIds.clear();
            for (auto &[sourceName, binding] : state.sources) {
                (void)sourceName;
                if (binding.proxy != nullptr) {
                    sources.push_back(binding.proxy);
                }
            }
            state.sources.clear();
            state.subscriptions.clear();
            state.timers.clear();
            state.triggerSources.clear();
        }
    }
    if (executors != nullptr) {
        for (auto taskId : timerTaskIds) {
            if (taskId != ExecutorPool::INVALID_TASK_ID) {
                executors->Remove(taskId, true);
            }
        }
    }
    for (const auto &source : sources) {
        if (source != nullptr) {
            source->OnStop();
        }
    }
}

DataMiningManager::SourceNotifier::SourceNotifier(
    DataMiningManager &manager, std::string pipelineName, std::string sourceName)
    : manager_(manager), pipelineName_(std::move(pipelineName)), sourceName_(std::move(sourceName))
{
}

void DataMiningManager::SourceNotifier::Notify(
    std::shared_ptr<Context> context, const std::string &topic, std::shared_ptr<DataValue> data)
{
    manager_.OnSourceOutput(pipelineName_, sourceName_, std::move(context), topic, std::move(data));
}

void DataMiningManager::BindExecutors(std::shared_ptr<OHOS::ExecutorPool> executors)
{
    // FeatureSystem 的正常顺序是先 OnBind，再 OnInitialize。
    // 这里仍然保留“重新挂载 timer”的逻辑，原因有两个：
    // 1. executor 可能发生替换，需要先移除旧任务，再按当前 started 状态重新调度。
    // 2. 单测、fuzz 或未来扩展路径不一定严格复用主链路顺序。
    std::shared_ptr<OHOS::ExecutorPool> previousExecutors;
    std::vector<OHOS::ExecutorPool::TaskId> timerTaskIds;
    std::vector<std::pair<std::string, SourceTimer>> pendingTimers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        previousExecutors = executors_;
        for (auto &[pipelineName, state] : pipelines_) {
            (void)pipelineName;
            AppendTimerTaskIds(state.timerTaskIds, timerTaskIds);
            state.timerTaskIds.clear();
        }
        executors_ = executors;
        if (executors_ != nullptr) {
            for (const auto &[pipelineName, state] : pipelines_) {
                if (!state.started) {
                    continue;
                }
                for (const auto &timer : state.timers) {
                    pendingTimers.emplace_back(pipelineName, timer);
                }
            }
        }
    }
    if (previousExecutors != nullptr) {
        for (auto taskId : timerTaskIds) {
            previousExecutors->Remove(taskId, true);
        }
    }
    if (executors == nullptr) {
        return;
    }
    for (const auto &[pipelineName, timer] : pendingTimers) {
        ScheduleTimerTask(pipelineName, timer);
    }
}
void DataMiningManager::SetRuntimeClient(std::shared_ptr<IEtlRuntimeClient> client)
{
    std::lock_guard<std::mutex> lock(mutex_);
    runtimeClient_ = std::move(client);
}

int32_t DataMiningManager::LoadDefaultConfigs(const std::string &pluginConfigDir, const std::string &pipelineConfigDir)
{
    std::vector<std::string> pluginFiles;
    std::vector<std::string> pipelineFiles;
    bool pluginDirReady = CollectJsonFiles(pluginConfigDir, pluginFiles);
    bool pipelineDirReady = CollectJsonFiles(pipelineConfigDir, pipelineFiles);
    if (!pluginDirReady) {
        ZLOGW("plugin config dir is not ready, dir:%{public}s", pluginConfigDir.c_str());
    }
    if (!pipelineDirReady) {
        ZLOGW("pipeline config dir is not ready, dir:%{public}s", pipelineConfigDir.c_str());
    }

    int32_t result = E_OK;
    for (const auto &file : pluginFiles) {
        int32_t status = RegisterPlugin(file);
        if (status != E_OK) {
            result = E_ERROR;
        }
    }
    for (const auto &file : pipelineFiles) {
        int32_t status = RegisterPipeline(file);
        if (status != E_OK) {
            result = E_ERROR;
        }
    }
    return result;
}

int32_t DataMiningManager::RegisterPlugin(const std::string &pluginConfigPath)
{
    PluginDescription description;
    if (ParsePluginConfig(pluginConfigPath, description) != E_OK) {
        return E_ERROR;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::unordered_map<std::string, PluginDescription> stagedPlugins;
        std::unordered_set<std::string> stagedNames;
        for (const auto &op : description.ops) {
            if (op.name.empty() || !stagedNames.insert(op.name).second || pluginsByOp_.find(op.name) != pluginsByOp_.end()) {
                return E_ERROR;
            }
            stagedPlugins.emplace(op.name, description);
        }
        for (const auto &[opName, plugin] : stagedPlugins) {
            pluginsByOp_.emplace(opName, plugin);
        }
    }
    return E_OK;
}

int32_t DataMiningManager::RegisterPipeline(const std::string &pipelineConfigPath)
{
    PipelineDescription description;
    if (ParsePipelineConfig(pipelineConfigPath, description) != E_OK) {
        return E_ERROR;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pipelines_.find(description.name) != pipelines_.end()) {
            return E_ERROR;
        }
        PipelineState state;
        state.description = description;
        pipelines_.emplace(description.name, std::move(state));
    }
    return E_OK;
}

int32_t DataMiningManager::StartPipelines(const std::vector<std::string> &pipelineNames)
{
    int32_t result = E_OK;
    for (const auto &name : pipelineNames) {
        if (StartPipeline(name) != E_OK) {
            result = E_ERROR;
        }
    }
    return result;
}

int32_t DataMiningManager::ParsePluginConfig(const std::string &pluginConfigPath, PluginDescription &description) const
{
    // 配置文件解析和路径安全校验放在一起，调用方只关心能否拿到可用的 description。
    std::string content;
    if (!LoadFile(pluginConfigPath, content)) {
        return E_ERROR;
    }
    if (!OHOS::DistributedData::Serializable::Unmarshall(content, description)) {
        return E_ERROR;
    }
    if (description.libs.path.empty()) {
        return E_OK;
    }
    description.libs.path = ResolveRelativePath(pluginConfigPath, description.libs.path);
    return description.libs.path.empty() ? E_ERROR : E_OK;
}

int32_t DataMiningManager::ParsePipelineConfig(
    const std::string &pipelineConfigPath, PipelineDescription &description) const
{
    std::string content;
    if (!LoadFile(pipelineConfigPath, content)) {
        return E_ERROR;
    }
    if (!OHOS::DistributedData::Serializable::Unmarshall(content, description) || description.name.empty()) {
        return E_ERROR;
    }
    return E_OK;
}

int32_t DataMiningManager::StartAllPipelines()
{
    std::vector<std::string> pipelineNames;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto &entry : pipelines_) {
            pipelineNames.push_back(entry.first);
        }
    }
    return StartPipelines(pipelineNames);
}

int32_t DataMiningManager::StartAutoPipelines()
{
    std::vector<std::string> pipelineNames;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // OnInitialize 只自动拉起“有自动触发策略”的 pipeline：
        // 1. 显式声明了 subscriptions 或 timers。
        // 2. trigger.type 为 timer 或 hybrid。
        // 3. tree 上声明了 subscribe source。
        // 纯 manual pipeline 保持未启动，等显式 TriggerPipeline 再进入运行态。
        for (const auto &[name, state] : pipelines_) {
            if (ShouldAutoStartPipeline(state.description)) {
                pipelineNames.push_back(name);
            }
        }
    }
    return StartPipelines(pipelineNames);
}

int32_t DataMiningManager::StartPipeline(const std::string &name)
{
    PendingStart startInfo;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = pipelines_.find(name);
        if (it == pipelines_.end()) {
            return E_ERROR;
        }
        int32_t status = PreparePipelineLocked(name, it->second);
        if (status != E_OK) {
            return status;
        }
        if (it->second.started) {
            return E_OK;
        }
        startInfo.sources = it->second.sources;
        startInfo.subscriptions = it->second.subscriptions;
        startInfo.timers = it->second.timers;
    }

    int32_t status = StartPipelineInternal(name, startInfo);
    if (status != E_OK) {
        StopSources(startInfo.sources, startInfo.subscriptions);
        return status;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = pipelines_.find(name);
        if (it == pipelines_.end()) {
            return E_ERROR;
        }
        it->second.started = true;
    }
    for (const auto &timer : startInfo.timers) {
        ScheduleTimerTask(name, timer);
    }
    return E_OK;
}

int32_t DataMiningManager::StopPipeline(const std::string &name)
{
    PendingStop stopInfo;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = pipelines_.find(name);
        if (it == pipelines_.end()) {
            return E_ERROR;
        }
        FillPendingStopLocked(it->second, stopInfo);
    }

    if (stopInfo.executors != nullptr) {
        for (auto taskId : stopInfo.timerTaskIds) {
            stopInfo.executors->Remove(taskId, true);
        }
    }
    StopSources(stopInfo.sources, stopInfo.subscriptions);
    return E_OK;
}

int32_t DataMiningManager::TriggerPipeline(const std::string &name, std::shared_ptr<Context> context)
{
    std::unordered_map<std::string, SourceBinding> sources;
    std::vector<SourceTimer> triggerSources;
    bool needStart = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = pipelines_.find(name);
        if (it == pipelines_.end()) {
            return E_ERROR;
        }
        int32_t status = PreparePipelineLocked(name, it->second);
        if (status != E_OK) {
            return status;
        }
        needStart = !it->second.started;
        sources = it->second.sources;
        triggerSources = it->second.triggerSources;
    }

    if (needStart) {
        int32_t status = StartPipeline(name);
        if (status != E_OK) {
            return status;
        }
    }

    for (const auto &item : triggerSources) {
        auto sourceIt = sources.find(item.sourceName);
        if (sourceIt == sources.end() || sourceIt->second.proxy == nullptr || sourceIt->second.notifier == nullptr) {
            return E_ERROR;
        }
        int32_t status = sourceIt->second.proxy->Trigger(BuildContext(context, item.parameters),
            sourceIt->second.notifier);
        if (status != E_OK) {
            return status;
        }
    }
    return E_OK;
}

bool DataMiningManager::LoadFile(const std::string &path, std::string &content) const
{
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    content = buffer.str();
    return true;
}

std::string DataMiningManager::ResolveRelativePath(const std::string &configPath, const std::string &targetPath) const
{
    if (targetPath.empty()) {
        return targetPath;
    }

    const std::filesystem::path libraryPath(targetPath);
    if (!IsSafeRelativeLibraryPath(libraryPath)) {
        return "";
    }

    const auto configDir = std::filesystem::path(configPath).parent_path().lexically_normal();
    if (configDir.empty()) {
        return "";
    }

    const auto resolvedPath = (configDir / libraryPath).lexically_normal();
    if (!IsSubPath(configDir, resolvedPath)) {
        return "";
    }
    return resolvedPath.string();
}

int32_t DataMiningManager::PreparePipelineLocked(const std::string &name, PipelineState &state)
{
    if (!state.sources.empty()) {
        return E_OK;
    }

    // 每次都按最新的 pipeline.json 重新展开 source 视图。
    // DDMS 侧只关心 source 节点，因为 operator 和 sink 已经下沉到 ETL SA。
    state.subscriptions.clear();
    state.timers.clear();
    state.triggerSources.clear();

    std::unordered_map<std::string, OpNode> sourceNodes;
    CollectSourceNodes(state.description.tree, pluginsByOp_, sourceNodes);
    if (sourceNodes.empty()) {
        return E_ERROR;
    }
    if (BuildSourceBindingsLocked(name, state, sourceNodes) != E_OK) {
        return E_ERROR;
    }
    ApplyTriggerPolicyLocked(state, sourceNodes);
    EnsureTriggerSourcesLocked(state, sourceNodes);
    return E_OK;
}

int32_t DataMiningManager::BuildSourceBindingsLocked(const std::string &name, PipelineState &state,
    const std::unordered_map<std::string, OpNode> &sourceNodes)
{
    std::unordered_map<std::string, SourceBinding> sources;
    std::vector<SourceSubscription> subscriptions;
    std::vector<SourceTimer> triggerSources;
    for (const auto &[sourceName, node] : sourceNodes) {
        auto pluginIt = pluginsByOp_.find(sourceName);
        if (pluginIt == pluginsByOp_.end()) {
            return E_ERROR;
        }
        auto endpoint = BuildEndpointConfig(pluginIt->second);
        auto client = CreateSourceClient(endpoint);
        SourceBinding binding;
        binding.notifier = std::make_shared<SourceNotifier>(*this, name, sourceName);
        binding.proxy = std::make_shared<SourceProxy>(sourceName, endpoint, sourceLoader_, client);
        sources.emplace(sourceName, std::move(binding));
        if (node.mode == "subscribe") {
            subscriptions.push_back({ sourceName, node.topic, "" });
        } else {
            SourceTimer trigger;
            trigger.sourceName = sourceName;
            trigger.parameters = "";
            triggerSources.push_back(trigger);
        }
    }
    state.sources = std::move(sources);
    state.subscriptions = std::move(subscriptions);
    state.triggerSources = std::move(triggerSources);
    return E_OK;
}

void DataMiningManager::ApplyTriggerPolicyLocked(
    PipelineState &state, const std::unordered_map<std::string, OpNode> &sourceNodes)
{
    // 这一层只负责决定 subscriptions 和 timers 的最终值，不再处理 source 创建。
    // 显式 trigger 配置优先级最高；如果 pipeline.json 没写，则退回到 tree 上的 mode/type 推导。
    if (!state.description.trigger.subscriptions.empty()) {
        state.subscriptions = state.description.trigger.subscriptions;
    }
    if (!state.description.trigger.timers.empty()) {
        state.timers = state.description.trigger.timers;
    } else if (state.description.trigger.type == "timer" || state.description.trigger.type == "hybrid") {
        for (const auto &[sourceName, node] : sourceNodes) {
            if (node.mode == "subscribe") {
                continue;
            }
            SourceTimer timer;
            timer.sourceName = sourceName;
            timer.interval =
                state.description.trigger.interval > 0 ? state.description.trigger.interval : DEFAULT_DAILY_INTERVAL;
            state.timers.push_back(timer);
        }
    }
}

void DataMiningManager::EnsureTriggerSourcesLocked(
    PipelineState &state, const std::unordered_map<std::string, OpNode> &sourceNodes) const
{
    // triggerSources 表示“可被手动或定时主动触发的 source 集合”。
    // 这里做最后补齐，保证非订阅 source 都能被 TriggerPipeline 或 timer 命中。
    if (state.triggerSources.empty()) {
        for (const auto &timer : state.timers) {
            state.triggerSources.push_back(timer);
        }
    }
    for (const auto &[sourceName, node] : sourceNodes) {
        (void)node;
        if (ContainsSubscription(state.subscriptions, sourceName)) {
            continue;
        }
        if (!ContainsTriggerSource(state.triggerSources, sourceName)) {
            SourceTimer trigger;
            trigger.sourceName = sourceName;
            state.triggerSources.push_back(trigger);
        }
    }
}

int32_t DataMiningManager::StartPipelineInternal(const std::string &name, const PendingStart &startInfo)
{
    (void)name;
    // 启动顺序固定为：
    // 1. 先初始化全部 source。
    // 2. 再建立订阅。
    // 3. 定时任务由 StartPipeline 外层统一挂到 executor。
    for (const auto &[sourceName, binding] : startInfo.sources) {
        (void)sourceName;
        if (binding.proxy == nullptr) {
            return E_ERROR;
        }
        int32_t status = binding.proxy->OnInitialize();
        if (status != E_OK) {
            return status;
        }
    }
    for (const auto &subscription : startInfo.subscriptions) {
        auto sourceIt = startInfo.sources.find(subscription.sourceName);
        if (sourceIt == startInfo.sources.end() || sourceIt->second.proxy == nullptr ||
            sourceIt->second.notifier == nullptr) {
            return E_ERROR;
        }
        int32_t status = sourceIt->second.proxy->Subscribe(
            subscription.topic, BuildContext(nullptr, subscription.parameters), sourceIt->second.notifier);
        if (status != E_OK) {
            return status;
        }
    }
    return E_OK;
}

void DataMiningManager::FillPendingStopLocked(PipelineState &state, PendingStop &stopInfo) const
{
    state.started = false;
    stopInfo.executors = executors_;
    stopInfo.sources = state.sources;
    stopInfo.subscriptions = state.subscriptions;
    AppendTimerTaskIds(state.timerTaskIds, stopInfo.timerTaskIds);
    state.timerTaskIds.clear();
}

bool DataMiningManager::BuildRuntimeDispatchSnapshotLocked(
    const std::string &pipelineName, RuntimeDispatchSnapshot &snapshot) const
{
    auto pipelineIt = pipelines_.find(pipelineName);
    if (pipelineIt == pipelines_.end() || !pipelineIt->second.started || runtimeClient_ == nullptr) {
        return false;
    }
    snapshot.client = runtimeClient_;
    snapshot.description = pipelineIt->second.description;
    snapshot.plugins = CollectUniquePlugins(pluginsByOp_);
    return true;
}

int32_t DataMiningManager::RegisterRuntimeSnapshot(const RuntimeDispatchSnapshot &snapshot) const
{
    for (const auto &plugin : snapshot.plugins) {
        if (snapshot.client->RegisterPlugin(plugin) != E_OK) {
            return E_ERROR;
        }
    }
    return snapshot.client->RegisterPipeline(snapshot.description);
}

int32_t DataMiningManager::BuildDispatchRequest(const std::string &pipelineName, const std::string &sourceName,
    const std::shared_ptr<Context> &context, const std::string &topic, const std::shared_ptr<DataValue> &data,
    DispatchRequest &request) const
{
    request.pipelineName = pipelineName;
    request.fromNode = sourceName;
    request.topic = topic;
    if (context != nullptr) {
        request.contextData = context->GetData();
        request.contextParameters = context->GetParameters();
    }
    return ConvertToTransportValue(data, request.value);
}

int32_t DataMiningManager::DispatchToRuntime(const std::string &pipelineName, const std::string &sourceName,
    std::shared_ptr<Context> context, const std::string &topic, std::shared_ptr<DataValue> data)
{
    // DDMS 不执行 operator 和 sink，只负责把 source 输出连同 pipeline 描述一起送到 ETL SA。
    // 这样可以把真正的运行时内存和 DDMS feature 的常驻内存拆开。
    RuntimeDispatchSnapshot snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!BuildRuntimeDispatchSnapshotLocked(pipelineName, snapshot)) {
            return E_ERROR;
        }
    }
    if (RegisterRuntimeSnapshot(snapshot) != E_OK) {
        return E_ERROR;
    }

    DispatchRequest request;
    if (BuildDispatchRequest(pipelineName, sourceName, context, topic, data, request) != E_OK) {
        return E_ERROR;
    }
    return snapshot.client->Dispatch(request);
}

void DataMiningManager::OnSourceOutput(const std::string &pipelineName, const std::string &sourceName,
    std::shared_ptr<Context> context, const std::string &topic, std::shared_ptr<DataValue> data)
{
    int32_t status = DispatchToRuntime(pipelineName, sourceName, std::move(context), topic, std::move(data));
    if (status != E_OK) {
        ZLOGE("dispatch to etl runtime failed, pipeline:%{public}s, source:%{public}s, status:%{public}d",
            pipelineName.c_str(), sourceName.c_str(), status);
    }
}

void DataMiningManager::ScheduleTimerTask(const std::string &pipelineName, const SourceTimer &timer)
{
    std::shared_ptr<ExecutorPool> executors;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        executors = executors_;
    }
    if (executors == nullptr) {
        return;
    }
    // 定时器采用“单次调度 + 回调里续约”的方式：
    // 1. 便于 StopPipeline 时精确移除当前 taskId。
    // 2. 避免长周期定时器在状态切换时保留过期上下文。
    int32_t interval = timer.interval > 0 ? timer.interval : DEFAULT_DAILY_INTERVAL;
    auto taskId = executors->Schedule(std::chrono::seconds(interval), [this, pipelineName, timer]() {
        SourceBinding binding;
        bool started = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto pipelineIt = pipelines_.find(pipelineName);
            if (pipelineIt == pipelines_.end()) {
                return;
            }
            pipelineIt->second.timerTaskIds.erase(timer.sourceName);
            started = pipelineIt->second.started;
            auto bindingIt = pipelineIt->second.sources.find(timer.sourceName);
            if (bindingIt != pipelineIt->second.sources.end()) {
                binding = bindingIt->second;
            }
        }
        if (!started || binding.proxy == nullptr || binding.notifier == nullptr) {
            return;
        }
        int32_t status = binding.proxy->Trigger(BuildContext(nullptr, timer.parameters), binding.notifier);
        if (status != E_OK) {
            ZLOGE("timer trigger failed, pipeline:%{public}s, source:%{public}s, status:%{public}d",
                pipelineName.c_str(), timer.sourceName.c_str(), status);
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto pipelineIt = pipelines_.find(pipelineName);
            if (pipelineIt == pipelines_.end() || !pipelineIt->second.started) {
                return;
            }
        }
        ScheduleTimerTask(pipelineName, timer);
    });
    if (taskId == ExecutorPool::INVALID_TASK_ID) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto pipelineIt = pipelines_.find(pipelineName);
    if (pipelineIt != pipelines_.end()) {
        pipelineIt->second.timerTaskIds[timer.sourceName] = taskId;
    }
}
void DataMiningManager::StopSources(
    const std::unordered_map<std::string, SourceBinding> &sources, const std::vector<SourceSubscription> &subscriptions)
{
    auto context = std::make_shared<Context>();
    for (const auto &subscription : subscriptions) {
        auto sourceIt = sources.find(subscription.sourceName);
        if (sourceIt != sources.end() && sourceIt->second.proxy != nullptr) {
            sourceIt->second.proxy->Unsubscribe(subscription.topic, context);
        }
    }
    for (const auto &[sourceName, binding] : sources) {
        (void)sourceName;
        if (binding.proxy != nullptr) {
            binding.proxy->OnStop();
        }
    }
}

std::shared_ptr<Context> DataMiningManager::BuildContext(
    const std::shared_ptr<Context> &context, const std::string &parameters) const
{
    auto result = std::make_shared<Context>();
    if (context != nullptr) {
        result->SetData(context->GetData());
        result->SetParameters(context->GetParameters());
    }
    if (!parameters.empty() && result->GetParameters().empty()) {
        result->SetParameters(parameters);
    }
    return result;
}
} // namespace OHOS::DataMining
