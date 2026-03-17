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

#ifndef OHOS_DISTRIBUTEDDATASERVICE_SERVICE_DATA_MINING_SERVICE_DATA_MINING_MANAGER_H
#define OHOS_DISTRIBUTEDDATASERVICE_SERVICE_DATA_MINING_SERVICE_DATA_MINING_MANAGER_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "executor_pool.h"
#include "model/pipeline_description.h"
#include "model/plugin_description.h"
#include "runtime/source_proxy.h"
#include "service/etl_runtime_client.h"

namespace OHOS::DataMining {
class DataMiningManager final {
public:
    DataMiningManager();
    ~DataMiningManager();

    void BindExecutors(std::shared_ptr<OHOS::ExecutorPool> executors);
    void SetRuntimeClient(std::shared_ptr<IEtlRuntimeClient> client);
    int32_t LoadDefaultConfigs(const std::string &pluginConfigDir, const std::string &pipelineConfigDir);
    int32_t RegisterPlugin(const std::string &pluginConfigPath);
    int32_t RegisterPipeline(const std::string &pipelineConfigPath);
    int32_t StartAutoPipelines();
    int32_t StartAllPipelines();
    int32_t StartPipeline(const std::string &name);
    int32_t StartScene(const std::string &scene);
    int32_t StopPipeline(const std::string &name);
    int32_t StopScene(const std::string &scene);
    int32_t TriggerPipeline(const std::string &name, std::shared_ptr<Context> context);

private:
    struct SourceBinding final {
        std::shared_ptr<SourceProxy> proxy;
        std::shared_ptr<AsyncNotifier> notifier;
    };

    struct PipelineState final {
        PipelineDescription description;
        std::unordered_map<std::string, SourceBinding> sources;
        std::vector<SourceSubscription> subscriptions;
        std::vector<SourceTimer> timers;
        std::vector<SourceTimer> triggerSources;
        std::unordered_map<std::string, OHOS::ExecutorPool::TaskId> timerTaskIds;
        bool started = false;
    };

    class SourceNotifier final : public AsyncNotifier {
    public:
        SourceNotifier(DataMiningManager &manager, std::string pipelineName, std::string sourceName);
        ~SourceNotifier() override = default;

        void Notify(std::shared_ptr<Context> context, const std::string &topic,
            std::shared_ptr<DataValue> data) override;

    private:
        DataMiningManager &manager_;
        std::string pipelineName_;
        std::string sourceName_;
    };

    struct PendingStart final {
        std::unordered_map<std::string, SourceBinding> sources;
        std::vector<SourceSubscription> subscriptions;
        std::vector<SourceTimer> timers;
    };

    struct PendingStop final {
        std::shared_ptr<OHOS::ExecutorPool> executors;
        std::unordered_map<std::string, SourceBinding> sources;
        std::vector<SourceSubscription> subscriptions;
        std::vector<OHOS::ExecutorPool::TaskId> timerTaskIds;
    };

    struct RuntimeDispatchSnapshot final {
        std::shared_ptr<IEtlRuntimeClient> client;
        PipelineDescription description;
        std::vector<PluginDescription> plugins;
    };

    bool LoadFile(const std::string &path, std::string &content) const;
    int32_t ParsePluginConfig(const std::string &pluginConfigPath, PluginDescription &description) const;
    int32_t ParsePipelineConfig(const std::string &pipelineConfigPath, PipelineDescription &description) const;
    std::string ResolveRelativePath(const std::string &configPath, const std::string &targetPath) const;
    int32_t StartPipelines(const std::vector<std::string> &pipelineNames);
    int32_t PreparePipelineLocked(const std::string &name, PipelineState &state);
    int32_t BuildSourceBindingsLocked(const std::string &name, PipelineState &state,
        const std::unordered_map<std::string, OpNode> &sourceNodes);
    void ApplyTriggerPolicyLocked(PipelineState &state, const std::unordered_map<std::string, OpNode> &sourceNodes);
    void EnsureTriggerSourcesLocked(PipelineState &state,
        const std::unordered_map<std::string, OpNode> &sourceNodes) const;
    int32_t StartPipelineInternal(const std::string &name, const PendingStart &startInfo);
    void FillPendingStopLocked(PipelineState &state, PendingStop &stopInfo) const;
    bool BuildRuntimeDispatchSnapshotLocked(
        const std::string &pipelineName, RuntimeDispatchSnapshot &snapshot) const;
    int32_t RegisterRuntimeSnapshot(const RuntimeDispatchSnapshot &snapshot) const;
    int32_t BuildDispatchRequest(const std::string &pipelineName, const std::string &sourceName,
        const std::shared_ptr<Context> &context, const std::string &topic, const std::shared_ptr<DataValue> &data,
        DispatchRequest &request) const;
    int32_t DispatchToRuntime(const std::string &pipelineName, const std::string &sourceName,
        std::shared_ptr<Context> context, const std::string &topic, std::shared_ptr<DataValue> data);
    void OnSourceOutput(const std::string &pipelineName, const std::string &sourceName,
        std::shared_ptr<Context> context, const std::string &topic, std::shared_ptr<DataValue> data);
    void ScheduleTimerTask(const std::string &pipelineName, const SourceTimer &timer);
    void StopSources(const std::unordered_map<std::string, SourceBinding> &sources,
        const std::vector<SourceSubscription> &subscriptions);
    std::shared_ptr<Context> BuildContext(const std::shared_ptr<Context> &context, const std::string &parameters) const;

    std::mutex mutex_;
    std::shared_ptr<OHOS::ExecutorPool> executors_;
    std::unordered_map<std::string, PluginDescription> pluginsByOp_;
    std::unordered_map<std::string, PipelineState> pipelines_;
    std::shared_ptr<IEtlRuntimeClient> runtimeClient_;
    std::shared_ptr<PluginLoader> sourceLoader_;
};
} // namespace OHOS::DataMining

#endif // OHOS_DISTRIBUTEDDATASERVICE_SERVICE_DATA_MINING_SERVICE_DATA_MINING_MANAGER_H
