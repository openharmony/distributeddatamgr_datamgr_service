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

#define LOG_TAG "DataMiningEtlTest"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "executor_pool.h"
#include "interfaces/basic_value.h"
#include "interfaces/data.h"
#include "interfaces/payload.h"
#include "runtime/endpoint_config.h"
#include "runtime/operator_proxy.h"
#include "runtime/pipeline_runtime.h"
#include "runtime/sink_proxy.h"
#include "runtime/source_proxy.h"
#include "runtime/transport_types.h"
#include "serializable/serializable.h"
#include "service/data_mining_manager.h"
#include "service/etl_runtime_manager.h"
#include "service/etl_runtime_client.h"

using namespace testing::ext;

namespace OHOS::Test {
namespace {
constexpr int32_t E_OK = 0;
constexpr int32_t E_ERROR = -1;
constexpr int32_t FAIL_STATUS = -101;
const std::string TEST_ROOT = "/data/test/data_mining_etl_test";

bool WriteFile(const std::string &path, const std::string &content)
{
    auto parent = std::filesystem::path(path).parent_path();
    std::error_code errorCode;
    std::filesystem::create_directories(parent, errorCode);
    if (errorCode) {
        return false;
    }

    std::ofstream output(path, std::ios::out | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }
    output << content;
    return output.good();
}

class FakeRuntimeClient final : public DataMining::IEtlRuntimeClient {
public:
    int32_t RegisterPlugin(const DataMining::PluginDescription &description) override
    {
        plugins.push_back(description);
        return E_OK;
    }

    int32_t RegisterPipeline(const DataMining::PipelineDescription &description) override
    {
        pipelines.push_back(description);
        return E_OK;
    }

    int32_t Dispatch(const DataMining::DispatchRequest &request) override
    {
        dispatches.push_back(request);
        return E_OK;
    }

    std::vector<DataMining::PluginDescription> plugins;
    std::vector<DataMining::PipelineDescription> pipelines;
    std::vector<DataMining::DispatchRequest> dispatches;
};

class TestBatchView final : public DataMining::QueryInterfaceSupport<TestBatchView, DataMining::IRecordBatchView> {
public:
    explicit TestBatchView(std::vector<std::shared_ptr<DataMining::DataValue>> items) : items_(std::move(items))
    {
    }

    const std::vector<std::shared_ptr<DataMining::DataValue>> &GetItems() const override
    {
        return items_;
    }

private:
    std::vector<std::shared_ptr<DataMining::DataValue>> items_;
};

class TestSubscribeSource final : public DataMining::Source {
public:
    std::string GetName() const override
    {
        return "emit_source";
    }

    int32_t OnInitialize() override
    {
        ++initializeCount_;
        return E_OK;
    }

    int32_t Trigger(
        std::shared_ptr<DataMining::Context> context, std::shared_ptr<DataMining::AsyncNotifier> notifier) override
    {
        (void)context;
        (void)notifier;
        return E_OK;
    }

    int32_t Subscribe(
        const std::string &topic, std::shared_ptr<DataMining::Context> context,
        std::shared_ptr<DataMining::AsyncNotifier> notifier) override
    {
        topic_ = topic;
        context_ = context;
        notifier_ = notifier;
        ++subscribeCount_;
        return E_OK;
    }

    int32_t Unsubscribe(const std::string &topic, std::shared_ptr<DataMining::Context> context) override
    {
        (void)topic;
        (void)context;
        ++unsubscribeCount_;
        notifier_ = nullptr;
        context_.reset();
        return E_OK;
    }

    int32_t OnStop() override
    {
        ++stopCount_;
        return E_OK;
    }

    void Emit(const std::string &payload)
    {
        if (notifier_ == nullptr) {
            return;
        }
        notifier_->Notify(context_, topic_, std::make_shared<DataMining::StringValue>(payload));
    }

    int32_t initializeCount_ = 0;
    int32_t subscribeCount_ = 0;
    int32_t unsubscribeCount_ = 0;
    int32_t stopCount_ = 0;

private:
    std::string topic_;
    std::shared_ptr<DataMining::Context> context_;
    std::shared_ptr<DataMining::AsyncNotifier> notifier_;
};

class TestTriggerSource final : public DataMining::Source {
public:
    std::string GetName() const override
    {
        return "timer_source";
    }

    int32_t OnInitialize() override
    {
        ++initializeCount_;
        return E_OK;
    }

    int32_t Trigger(
        std::shared_ptr<DataMining::Context> context, std::shared_ptr<DataMining::AsyncNotifier> notifier) override
    {
        if (notifier == nullptr) {
            return E_ERROR;
        }
        ++triggerCount_;
        notifier->Notify(context, "timer.topic", std::make_shared<DataMining::StringValue>("tick"));
        return E_OK;
    }

    int32_t Subscribe(
        const std::string &topic, std::shared_ptr<DataMining::Context> context,
        std::shared_ptr<DataMining::AsyncNotifier> notifier) override
    {
        (void)topic;
        (void)context;
        (void)notifier;
        return E_OK;
    }

    int32_t Unsubscribe(const std::string &topic, std::shared_ptr<DataMining::Context> context) override
    {
        (void)topic;
        (void)context;
        return E_OK;
    }

    int32_t OnStop() override
    {
        return E_OK;
    }

    int32_t initializeCount_ = 0;
    int32_t triggerCount_ = 0;
};

class FailingOperator final : public DataMining::Operator {
public:
    std::string GetName() const override
    {
        return "failing_operator";
    }

    int32_t Process(std::shared_ptr<DataMining::Context> context, std::shared_ptr<DataMining::DataValue> input,
        std::shared_ptr<DataMining::AsyncNotifier> notifier) override
    {
        (void)context;
        (void)input;
        (void)notifier;
        return FAIL_STATUS;
    }
};

class MergeAwareOperator final : public DataMining::Operator {
public:
    std::string GetName() const override
    {
        return "merge_operator";
    }

    int32_t Process(std::shared_ptr<DataMining::Context> context, std::shared_ptr<DataMining::DataValue> input,
        std::shared_ptr<DataMining::AsyncNotifier> notifier) override
    {
        if (notifier == nullptr) {
            return E_ERROR;
        }
        auto batch = DataMining::QueryInterface<DataMining::IRecordBatchView>(input);
        if (batch == nullptr) {
            return E_ERROR;
        }
        batchSize_ = batch->GetItems().size();
        notifier->Notify(context, "merged", std::make_shared<DataMining::StringValue>(std::to_string(batchSize_)));
        return E_OK;
    }

    size_t batchSize_ = 0;
};

class RecordingSink final : public DataMining::Sink {
public:
    std::string GetName() const override
    {
        return "recording_sink";
    }

    int32_t Save(std::shared_ptr<DataMining::Context> context, std::shared_ptr<DataMining::DataValue> data) override
    {
        (void)context;
        auto value = DataMining::QueryInterface<DataMining::StringValue>(data);
        if (value == nullptr) {
            return E_ERROR;
        }
        values_.push_back(value->GetValue());
        return E_OK;
    }

    std::vector<std::string> values_;
};
} // namespace

class DataMiningEtlTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        std::error_code errorCode;
        std::filesystem::remove_all(TEST_ROOT, errorCode);
        std::filesystem::create_directories(TEST_ROOT, errorCode);
    }

    static void TearDownTestCase(void)
    {
        std::error_code errorCode;
        std::filesystem::remove_all(TEST_ROOT, errorCode);
    }

    void SetUp() override
    {
        std::error_code errorCode;
        std::filesystem::remove_all(TEST_ROOT, errorCode);
        std::filesystem::create_directories(TEST_ROOT, errorCode);
    }

    void TearDown() override
    {
        std::error_code errorCode;
        std::filesystem::remove_all(TEST_ROOT, errorCode);
    }

protected:
    std::string CreatePluginConfigWithOps(
        const std::string &fileName, const std::string &opsJson, const std::string &libraryPath)
    {
        const auto configPath = (std::filesystem::path(TEST_ROOT) / "plugins" / fileName).string();
        std::string content = "{\n"
            "  \"ops\": " + opsJson + ",\n"
            "  \"libs\": { \"path\": \"" + libraryPath + "\" },\n"
            "  \"extension\": { \"uri\": \"\" },\n"
            "  \"sa\": { \"said\": 0, \"code\": 0 }\n"
            "}\n";
        EXPECT_TRUE(WriteFile(configPath, content));
        return configPath;
    }

    std::string CreatePluginConfig(const std::string &fileName, const std::string &opName, const std::string &type,
        const std::string &libraryPath)
    {
        std::string opsJson = "[\n"
            "    {\n"
            "      \"name\": \"" + opName + "\",\n"
            "      \"type\": \"" + type + "\",\n"
            "      \"input\": { \"typeName\": \"json\", \"elements\": [] },\n"
            "      \"output\": { \"typeName\": \"json\", \"elements\": [] }\n"
            "    }\n"
            "  ]";
        return CreatePluginConfigWithOps(fileName, opsJson, libraryPath);
    }

    std::string CreatePipelineConfig(const std::string &fileName, const std::string &name, const std::string &tree,
        const std::string &subscriptionsJson, const std::string &timersJson, const std::string &type = "manual",
        int32_t interval = 0)
    {
        const auto configPath = (std::filesystem::path(TEST_ROOT) / "pipelines" / fileName).string();
        std::string content = "{\n"
            "  \"name\": \"" + name + "\",\n"
            "  \"scene\": [\"test\"],\n"
            "  \"trigger\": {\n"
            "    \"type\": \"" + type + "\",\n"
            "    \"interval\": " + std::to_string(interval) + ",\n"
            "    \"eventType\": 0,\n"
            "    \"nextOrAnd\": \"\",\n"
            "    \"subscriptions\": " + subscriptionsJson + ",\n"
            "    \"timers\": " + timersJson + "\n"
            "  },\n"
            "  \"tree\": " + tree + "\n"
            "}\n";
        EXPECT_TRUE(WriteFile(configPath, content));
        return configPath;
    }
};

HWTEST_F(DataMiningEtlTest, RegisterPluginRejectsTraversalPath001, TestSize.Level1)
{
    DataMining::DataMiningManager manager;
    auto configPath = CreatePluginConfig("reject_traversal.json", "source_a", "source", "../libunsafe.so");
    EXPECT_EQ(manager.RegisterPlugin(configPath), E_ERROR);
}

HWTEST_F(DataMiningEtlTest, RegisterPluginResolvesSafeRelativePath002, TestSize.Level1)
{
    DataMining::DataMiningManager manager;
    auto configPath = CreatePluginConfig("accept_relative.json", "source_c", "source", "libs/libsafe.so");
    ASSERT_EQ(manager.RegisterPlugin(configPath), E_OK);

    auto it = manager.pluginsByOp_.find("source_c");
    ASSERT_NE(it, manager.pluginsByOp_.end());
    auto expected = (std::filesystem::path(TEST_ROOT) / "plugins" / "libs" / "libsafe.so").lexically_normal();
    EXPECT_EQ(std::filesystem::path(it->second.libs.path).lexically_normal().string(), expected.string());
}

HWTEST_F(DataMiningEtlTest, StartPipelineSubscribeDispatchesToRuntime003, TestSize.Level1)
{
    DataMining::DataMiningManager manager;
    auto runtimeClient = std::make_shared<FakeRuntimeClient>();
    manager.SetRuntimeClient(runtimeClient);

    auto pluginConfig = CreatePluginConfig("subscribe_plugin.json", "emit_source", "source", "libs/libemit.so");
    auto tree = "{"
        "\"opName\":\"emit_source\","
        "\"mode\":\"subscribe\","
        "\"topic\":\"donation.changed\","
        "\"children\":[],"
        "\"output\":[]"
    "}";
    auto pipelineConfig = CreatePipelineConfig("subscribe_pipeline.json", "subscribe_pipeline", tree,
        "[{\"sourceName\":\"emit_source\",\"topic\":\"donation.changed\",\"parameters\":\"{\\\"source\\\":\\\"sub\\\"}\"}]",
        "[]");

    ASSERT_EQ(manager.RegisterPlugin(pluginConfig), E_OK);
    ASSERT_EQ(manager.RegisterPipeline(pipelineConfig), E_OK);

    auto source = std::make_shared<TestSubscribeSource>();
    DataMining::EndpointConfig endpoint;
    endpoint.kind = DataMining::EndpointKind::LIBRARY;
    manager.pipelines_["subscribe_pipeline"].sources["emit_source"] = {
        std::make_shared<DataMining::SourceProxy>("emit_source", endpoint, std::make_shared<DataMining::PluginLoader>(),
            nullptr, source),
        std::make_shared<DataMining::DataMiningManager::SourceNotifier>(manager, "subscribe_pipeline", "emit_source")
    };
    manager.pipelines_["subscribe_pipeline"].subscriptions = {
        { "emit_source", "donation.changed", "{\"source\":\"sub\"}" }
    };

    ASSERT_EQ(manager.StartPipeline("subscribe_pipeline"), E_OK);
    source->Emit("payload");

    ASSERT_EQ(runtimeClient->dispatches.size(), 1U);
    EXPECT_EQ(runtimeClient->dispatches[0].pipelineName, "subscribe_pipeline");
    EXPECT_EQ(runtimeClient->dispatches[0].fromNode, "emit_source");
    EXPECT_EQ(runtimeClient->dispatches[0].contextParameters, "{\"source\":\"sub\"}");

    auto value = DataMining::ConvertFromTransportValue(runtimeClient->dispatches[0].value);
    auto text = DataMining::QueryInterface<DataMining::StringValue>(value);
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->GetValue(), "payload");
    EXPECT_EQ(manager.StopPipeline("subscribe_pipeline"), E_OK);
}

HWTEST_F(DataMiningEtlTest, StartPipelineTimerDispatchesToRuntime004, TestSize.Level1)
{
    DataMining::DataMiningManager manager;
    manager.BindExecutors(std::make_shared<ExecutorPool>(1, 1));
    auto runtimeClient = std::make_shared<FakeRuntimeClient>();
    manager.SetRuntimeClient(runtimeClient);

    auto pluginConfig = CreatePluginConfig("timer_plugin.json", "timer_source", "source", "libs/libtimer.so");
    auto tree = "{"
        "\"opName\":\"timer_source\","
        "\"mode\":\"trigger\","
        "\"topic\":\"timer.topic\","
        "\"children\":[],"
        "\"output\":[]"
    "}";
    auto pipelineConfig = CreatePipelineConfig("timer_pipeline.json", "timer_pipeline", tree, "[]",
        "[{\"sourceName\":\"timer_source\",\"interval\":1,\"parameters\":\"{\\\"trigger\\\":\\\"timer\\\"}\"}]",
        "timer", 1);

    ASSERT_EQ(manager.RegisterPlugin(pluginConfig), E_OK);
    ASSERT_EQ(manager.RegisterPipeline(pipelineConfig), E_OK);

    auto source = std::make_shared<TestTriggerSource>();
    DataMining::EndpointConfig endpoint;
    endpoint.kind = DataMining::EndpointKind::LIBRARY;
    manager.pipelines_["timer_pipeline"].sources["timer_source"] = {
        std::make_shared<DataMining::SourceProxy>("timer_source", endpoint, std::make_shared<DataMining::PluginLoader>(),
            nullptr, source),
        std::make_shared<DataMining::DataMiningManager::SourceNotifier>(manager, "timer_pipeline", "timer_source")
    };
    manager.pipelines_["timer_pipeline"].timers = {
        { "timer_source", 1, "{\"trigger\":\"timer\"}" }
    };
    manager.pipelines_["timer_pipeline"].triggerSources = manager.pipelines_["timer_pipeline"].timers;

    ASSERT_EQ(manager.StartPipeline("timer_pipeline"), E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    EXPECT_GE(source->triggerCount_, 1);
    ASSERT_FALSE(runtimeClient->dispatches.empty());
    EXPECT_EQ(runtimeClient->dispatches.back().fromNode, "timer_source");
    EXPECT_EQ(manager.StopPipeline("timer_pipeline"), E_OK);
}

HWTEST_F(DataMiningEtlTest, TriggerPipelineDispatchesManualSource005, TestSize.Level1)
{
    DataMining::DataMiningManager manager;
    auto runtimeClient = std::make_shared<FakeRuntimeClient>();
    manager.SetRuntimeClient(runtimeClient);

    auto pluginConfig = CreatePluginConfig("manual_plugin.json", "manual_source", "source", "libs/libmanual.so");
    auto tree = "{"
        "\"opName\":\"manual_source\","
        "\"mode\":\"trigger\","
        "\"topic\":\"manual.topic\","
        "\"children\":[],"
        "\"output\":[]"
    "}";
    auto pipelineConfig = CreatePipelineConfig("manual_pipeline.json", "manual_pipeline", tree, "[]", "[]");

    ASSERT_EQ(manager.RegisterPlugin(pluginConfig), E_OK);
    ASSERT_EQ(manager.RegisterPipeline(pipelineConfig), E_OK);

    auto source = std::make_shared<TestTriggerSource>();
    DataMining::EndpointConfig endpoint;
    endpoint.kind = DataMining::EndpointKind::LIBRARY;
    manager.pipelines_["manual_pipeline"].sources["manual_source"] = {
        std::make_shared<DataMining::SourceProxy>("manual_source", endpoint, std::make_shared<DataMining::PluginLoader>(),
            nullptr, source),
        std::make_shared<DataMining::DataMiningManager::SourceNotifier>(manager, "manual_pipeline", "manual_source")
    };
    manager.pipelines_["manual_pipeline"].triggerSources = {
        { "manual_source", 0, "{\"trigger\":\"manual\"}" }
    };

    auto context = std::make_shared<DataMining::Context>();
    context->SetParameters("{\"manual\":true}");
    ASSERT_EQ(manager.TriggerPipeline("manual_pipeline", context), E_OK);
    ASSERT_EQ(runtimeClient->dispatches.size(), 1U);
    EXPECT_EQ(runtimeClient->dispatches[0].contextParameters, "{\"manual\":true}");
}

HWTEST_F(DataMiningEtlTest, StartAutoPipelinesSkipsManualAndStartsAutoTriggered010, TestSize.Level1)
{
    DataMining::DataMiningManager manager;

    auto subscribePlugin = CreatePluginConfig("auto_sub_plugin.json", "auto_sub_source", "source", "libs/libsub.so");
    auto timerPlugin = CreatePluginConfig("auto_timer_plugin.json", "auto_timer_source", "source", "libs/libtimer.so");
    auto manualPlugin = CreatePluginConfig("auto_manual_plugin.json", "auto_manual_source", "source", "libs/libmanual.so");

    auto subscribeTree = "{"
        "\"opName\":\"auto_sub_source\","
        "\"mode\":\"subscribe\","
        "\"topic\":\"auto.subscribe\","
        "\"children\":[],"
        "\"output\":[]"
    "}";
    auto timerTree = "{"
        "\"opName\":\"auto_timer_source\","
        "\"mode\":\"trigger\","
        "\"topic\":\"auto.timer\","
        "\"children\":[],"
        "\"output\":[]"
    "}";
    auto manualTree = "{"
        "\"opName\":\"auto_manual_source\","
        "\"mode\":\"trigger\","
        "\"topic\":\"auto.manual\","
        "\"children\":[],"
        "\"output\":[]"
    "}";

    auto subscribePipeline = CreatePipelineConfig(
        "auto_sub_pipeline.json", "auto_sub_pipeline", subscribeTree, "[]", "[]");
    auto timerPipeline = CreatePipelineConfig(
        "auto_timer_pipeline.json", "auto_timer_pipeline", timerTree, "[]",
        "[{\"sourceName\":\"auto_timer_source\",\"interval\":1,\"parameters\":\"{}\"}]", "timer", 1);
    auto manualPipeline = CreatePipelineConfig(
        "auto_manual_pipeline.json", "auto_manual_pipeline", manualTree, "[]", "[]");

    ASSERT_EQ(manager.RegisterPlugin(subscribePlugin), E_OK);
    ASSERT_EQ(manager.RegisterPlugin(timerPlugin), E_OK);
    ASSERT_EQ(manager.RegisterPlugin(manualPlugin), E_OK);
    ASSERT_EQ(manager.RegisterPipeline(subscribePipeline), E_OK);
    ASSERT_EQ(manager.RegisterPipeline(timerPipeline), E_OK);
    ASSERT_EQ(manager.RegisterPipeline(manualPipeline), E_OK);

    auto subscribeSource = std::make_shared<TestSubscribeSource>();
    auto timerSource = std::make_shared<TestTriggerSource>();
    auto manualSource = std::make_shared<TestTriggerSource>();
    DataMining::EndpointConfig endpoint;
    endpoint.kind = DataMining::EndpointKind::LIBRARY;

    manager.pipelines_["auto_sub_pipeline"].sources["auto_sub_source"] = {
        std::make_shared<DataMining::SourceProxy>("auto_sub_source", endpoint,
            std::make_shared<DataMining::PluginLoader>(), nullptr, subscribeSource),
        std::make_shared<DataMining::DataMiningManager::SourceNotifier>(manager, "auto_sub_pipeline", "auto_sub_source")
    };
    manager.pipelines_["auto_sub_pipeline"].subscriptions = {
        { "auto_sub_source", "auto.subscribe", "{}" }
    };

    manager.pipelines_["auto_timer_pipeline"].sources["auto_timer_source"] = {
        std::make_shared<DataMining::SourceProxy>("auto_timer_source", endpoint,
            std::make_shared<DataMining::PluginLoader>(), nullptr, timerSource),
        std::make_shared<DataMining::DataMiningManager::SourceNotifier>(
            manager, "auto_timer_pipeline", "auto_timer_source")
    };
    manager.pipelines_["auto_timer_pipeline"].timers = {
        { "auto_timer_source", 1, "{}" }
    };
    manager.pipelines_["auto_timer_pipeline"].triggerSources = manager.pipelines_["auto_timer_pipeline"].timers;

    manager.pipelines_["auto_manual_pipeline"].sources["auto_manual_source"] = {
        std::make_shared<DataMining::SourceProxy>("auto_manual_source", endpoint,
            std::make_shared<DataMining::PluginLoader>(), nullptr, manualSource),
        std::make_shared<DataMining::DataMiningManager::SourceNotifier>(
            manager, "auto_manual_pipeline", "auto_manual_source")
    };
    manager.pipelines_["auto_manual_pipeline"].triggerSources = {
        { "auto_manual_source", 0, "{}" }
    };

    ASSERT_EQ(manager.StartAutoPipelines(), E_OK);

    EXPECT_TRUE(manager.pipelines_["auto_sub_pipeline"].started);
    EXPECT_TRUE(manager.pipelines_["auto_timer_pipeline"].started);
    EXPECT_FALSE(manager.pipelines_["auto_manual_pipeline"].started);
    EXPECT_EQ(subscribeSource->subscribeCount_, 1);
    EXPECT_EQ(timerSource->initializeCount_, 1);
    EXPECT_EQ(manualSource->initializeCount_, 0);
}

HWTEST_F(DataMiningEtlTest, RegisterPipelineRejectsDuplicateName011, TestSize.Level1)
{
    DataMining::DataMiningManager manager;

    auto pluginConfig = CreatePluginConfig("duplicate_pipeline_plugin.json", "duplicate_source", "source", "libs/libdup.so");
    auto tree = "{"
        "\"opName\":\"duplicate_source\","
        "\"mode\":\"subscribe\","
        "\"topic\":\"duplicate.topic\","
        "\"children\":[],"
        "\"output\":[]"
    "}";
    auto pipelineConfigV1 = CreatePipelineConfig("duplicate_pipeline_v1.json", "duplicate_pipeline", tree,
        "[{\"sourceName\":\"duplicate_source\",\"topic\":\"duplicate.topic\",\"parameters\":\"{}\"}]", "[]");
    auto pipelineConfigV2 = CreatePipelineConfig("duplicate_pipeline_v2.json", "duplicate_pipeline", tree, "[]", "[]");

    ASSERT_EQ(manager.RegisterPlugin(pluginConfig), E_OK);
    ASSERT_EQ(manager.RegisterPipeline(pipelineConfigV1), E_OK);
    EXPECT_EQ(manager.RegisterPipeline(pipelineConfigV2), E_ERROR);
}

HWTEST_F(DataMiningEtlTest, RegisterPluginRejectsDuplicateOpName012, TestSize.Level1)
{
    DataMining::DataMiningManager manager;

    auto pluginConfigV1 = CreatePluginConfig("duplicate_plugin_v1.json", "duplicate_source", "source", "libs/libdup1.so");
    auto pluginConfigV2 = CreatePluginConfig("duplicate_plugin_v2.json", "duplicate_source", "source", "libs/libdup2.so");

    ASSERT_EQ(manager.RegisterPlugin(pluginConfigV1), E_OK);
    EXPECT_EQ(manager.RegisterPlugin(pluginConfigV2), E_ERROR);
}

HWTEST_F(DataMiningEtlTest, RegisterPluginRejectsDuplicateOpsInSameConfig013, TestSize.Level1)
{
    DataMining::DataMiningManager manager;

    auto pluginConfig = CreatePluginConfigWithOps("duplicate_plugin_same_file.json",
        "["
        "  {"
        "    \"name\":\"duplicate_source\","
        "    \"type\":\"source\","
        "    \"input\":{\"typeName\":\"json\",\"elements\":[]},"
        "    \"output\":{\"typeName\":\"json\",\"elements\":[]}"
        "  },"
        "  {"
        "    \"name\":\"duplicate_source\","
        "    \"type\":\"source\","
        "    \"input\":{\"typeName\":\"json\",\"elements\":[]},"
        "    \"output\":{\"typeName\":\"json\",\"elements\":[]}"
        "  }"
        "]",
        "libs/libdup.so");

    EXPECT_EQ(manager.RegisterPlugin(pluginConfig), E_ERROR);
}

HWTEST_F(DataMiningEtlTest, StopPipelineUnsubscribesAndStopsSource014, TestSize.Level1)
{
    DataMining::DataMiningManager manager;
    manager.BindExecutors(std::make_shared<ExecutorPool>(1, 1));

    auto pluginConfig = CreatePluginConfig("stop_pipeline_plugin.json", "stop_source", "source", "libs/libstop.so");
    auto tree = "{"
        "\"opName\":\"stop_source\","
        "\"mode\":\"subscribe\","
        "\"topic\":\"stop.topic\","
        "\"children\":[],"
        "\"output\":[]"
    "}";
    auto pipelineConfig = CreatePipelineConfig("stop_pipeline.json", "stop_pipeline", tree,
        "[{\"sourceName\":\"stop_source\",\"topic\":\"stop.topic\",\"parameters\":\"{}\"}]", "[]");

    ASSERT_EQ(manager.RegisterPlugin(pluginConfig), E_OK);
    ASSERT_EQ(manager.RegisterPipeline(pipelineConfig), E_OK);

    auto source = std::make_shared<TestSubscribeSource>();
    DataMining::EndpointConfig endpoint;
    endpoint.kind = DataMining::EndpointKind::LIBRARY;
    manager.pipelines_["stop_pipeline"].sources["stop_source"] = {
        std::make_shared<DataMining::SourceProxy>("stop_source", endpoint, std::make_shared<DataMining::PluginLoader>(),
            nullptr, source),
        std::make_shared<DataMining::DataMiningManager::SourceNotifier>(manager, "stop_pipeline", "stop_source")
    };
    manager.pipelines_["stop_pipeline"].subscriptions = {
        { "stop_source", "stop.topic", "{}" }
    };
    manager.pipelines_["stop_pipeline"].timerTaskIds["stop_source"] = 123;

    ASSERT_EQ(manager.StartPipeline("stop_pipeline"), E_OK);
    ASSERT_EQ(manager.StopPipeline("stop_pipeline"), E_OK);
    EXPECT_EQ(source->unsubscribeCount_, 1);
    EXPECT_EQ(source->stopCount_, 1);
    EXPECT_TRUE(manager.pipelines_["stop_pipeline"].timerTaskIds.empty());
    EXPECT_FALSE(manager.pipelines_["stop_pipeline"].started);
}

HWTEST_F(DataMiningEtlTest, EtlRuntimeManagerReusesRuntimeWhenConfigUnchanged015, TestSize.Level1)
{
    DataMining::EtlRuntimeManager manager;

    DataMining::PluginDescription plugin;
    DataMining::OPDescription op;
    op.name = "runtime_source";
    op.type = "source";
    plugin.ops.push_back(op);

    DataMining::PipelineDescription pipeline;
    pipeline.name = "runtime_pipeline";
    pipeline.tree.opName = "runtime_source";

    auto pluginContent = OHOS::DistributedData::Serializable::Marshall(plugin);
    auto pipelineContent = OHOS::DistributedData::Serializable::Marshall(pipeline);

    ASSERT_EQ(manager.RegisterPlugin(pluginContent), E_OK);
    ASSERT_EQ(manager.RegisterPipeline(pipelineContent), E_OK);

    DataMining::DispatchRequest request;
    request.pipelineName = "runtime_pipeline";
    request.fromNode = "runtime_source";
    request.topic = "runtime.topic";
    ASSERT_EQ(manager.Dispatch(request), E_OK);

    auto firstRuntime = manager.runtimes_["runtime_pipeline"];
    ASSERT_NE(firstRuntime, nullptr);

    ASSERT_EQ(manager.RegisterPlugin(pluginContent), E_OK);
    ASSERT_EQ(manager.RegisterPipeline(pipelineContent), E_OK);
    ASSERT_EQ(manager.Dispatch(request), E_OK);
    EXPECT_EQ(manager.runtimes_["runtime_pipeline"], firstRuntime);
}

HWTEST_F(DataMiningEtlTest, EtlRuntimeManagerRejectsDuplicateOpsInSamePlugin016, TestSize.Level1)
{
    DataMining::EtlRuntimeManager manager;

    DataMining::PluginDescription plugin;
    DataMining::OPDescription firstOp;
    firstOp.name = "duplicate_source";
    firstOp.type = "source";
    plugin.ops.push_back(firstOp);
    plugin.ops.push_back(firstOp);

    EXPECT_EQ(manager.RegisterPlugin(OHOS::DistributedData::Serializable::Marshall(plugin)), E_ERROR);
    EXPECT_TRUE(manager.pluginsByOp_.empty());
}

HWTEST_F(DataMiningEtlTest, PipelineRuntimePropagatesOperatorFailure006, TestSize.Level1)
{
    DataMining::PipelineDescription pipeline;
    pipeline.name = "pipeline";
    pipeline.tree.opName = "source_node";
    DataMining::OpNode child;
    child.opName = "operator_node";
    pipeline.tree.children.push_back(child);

    DataMining::PluginDescription sourceDescription;
    DataMining::OPDescription sourceOp;
    sourceOp.name = "source_node";
    sourceOp.type = "source";
    sourceDescription.ops.push_back(sourceOp);

    DataMining::PluginDescription operatorDescription;
    DataMining::OPDescription operatorOp;
    operatorOp.name = "operator_node";
    operatorOp.type = "operator";
    operatorDescription.ops.push_back(operatorOp);

    std::unordered_map<std::string, DataMining::PluginDescription> plugins = {
        { "source_node", sourceDescription },
        { "operator_node", operatorDescription },
    };

    DataMining::PipelineRuntime runtime;
    ASSERT_EQ(runtime.Load(pipeline, plugins), E_OK);

    DataMining::EndpointConfig endpoint;
    endpoint.kind = DataMining::EndpointKind::LIBRARY;
    runtime.nodes_["operator_node"].op = std::make_shared<DataMining::OperatorProxy>(
        "operator_node", endpoint, std::make_shared<DataMining::PluginLoader>(), std::make_shared<FailingOperator>());

    auto data = std::make_shared<DataMining::StringValue>("payload");
    EXPECT_EQ(runtime.DispatchFrom("source_node", std::make_shared<DataMining::Context>(), "topic", data), FAIL_STATUS);
    EXPECT_EQ(runtime.lastError_, FAIL_STATUS);
}

HWTEST_F(DataMiningEtlTest, PipelineRuntimeMergesParentsBeforeSink007, TestSize.Level1)
{
    DataMining::PipelineDescription pipeline;
    pipeline.name = "merge";
    pipeline.tree.opName = "";

    DataMining::OpNode sourceA;
    sourceA.opName = "source_a";
    DataMining::OpNode sourceB;
    sourceB.opName = "source_b";
    DataMining::OpNode mergeOp;
    mergeOp.opName = "merge_operator";
    DataMining::OpNode sink;
    sink.opName = "sink_node";
    mergeOp.children.push_back(sink);
    sourceA.children.push_back(mergeOp);
    sourceB.children.push_back(mergeOp);
    pipeline.tree.children = { sourceA, sourceB };

    DataMining::PluginDescription sourceDescription;
    DataMining::OPDescription sourceAOp;
    sourceAOp.name = "source_a";
    sourceAOp.type = "source";
    DataMining::OPDescription sourceBOp;
    sourceBOp.name = "source_b";
    sourceBOp.type = "source";
    sourceDescription.ops = { sourceAOp, sourceBOp };

    DataMining::PluginDescription operatorDescription;
    DataMining::OPDescription operatorOp;
    operatorOp.name = "merge_operator";
    operatorOp.type = "operator";
    operatorDescription.ops.push_back(operatorOp);

    DataMining::PluginDescription sinkDescription;
    DataMining::OPDescription sinkOp;
    sinkOp.name = "sink_node";
    sinkOp.type = "sink";
    sinkDescription.ops.push_back(sinkOp);

    std::unordered_map<std::string, DataMining::PluginDescription> plugins = {
        { "source_a", sourceDescription },
        { "source_b", sourceDescription },
        { "merge_operator", operatorDescription },
        { "sink_node", sinkDescription },
    };

    DataMining::PipelineRuntime runtime;
    ASSERT_EQ(runtime.Load(pipeline, plugins), E_OK);

    auto merge = std::make_shared<MergeAwareOperator>();
    auto sinkRecorder = std::make_shared<RecordingSink>();
    DataMining::EndpointConfig endpoint;
    endpoint.kind = DataMining::EndpointKind::LIBRARY;
    runtime.nodes_["merge_operator"].op = std::make_shared<DataMining::OperatorProxy>(
        "merge_operator", endpoint, std::make_shared<DataMining::PluginLoader>(), merge);
    runtime.nodes_["sink_node"].sink = std::make_shared<DataMining::SinkProxy>(
        "sink_node", endpoint, std::make_shared<DataMining::PluginLoader>(), sinkRecorder);

    EXPECT_EQ(runtime.DispatchFrom("source_a", std::make_shared<DataMining::Context>(), "topic",
        std::make_shared<DataMining::StringValue>("a")), E_OK);
    EXPECT_TRUE(sinkRecorder->values_.empty());

    EXPECT_EQ(runtime.DispatchFrom("source_b", std::make_shared<DataMining::Context>(), "topic",
        std::make_shared<DataMining::StringValue>("b")), E_OK);
    EXPECT_EQ(merge->batchSize_, 2U);
    ASSERT_EQ(sinkRecorder->values_.size(), 1U);
    EXPECT_EQ(sinkRecorder->values_[0], "2");
}

HWTEST_F(DataMiningEtlTest, TransportValueRoundTripSupportsJsonAndBytes008, TestSize.Level1)
{
    DataMining::Json json = {
        { "scene", "one_touch_taxi" },
        { "confidence", 0.9 }
    };
    DataMining::TransportValue jsonValue;
    DataMining::TransportValue bytesValue;

    ASSERT_EQ(DataMining::ConvertToTransportValue(std::make_shared<DataMining::JsonValue>(json), jsonValue), E_OK);
    ASSERT_EQ(DataMining::ConvertToTransportValue(
        std::make_shared<DataMining::BytesValue>(std::vector<uint8_t> { 1, 2, 3 }), bytesValue), E_OK);

    auto restoredJson = DataMining::ConvertFromTransportValue(jsonValue);
    auto restoredBytes = DataMining::ConvertFromTransportValue(bytesValue);

    auto jsonData = DataMining::QueryInterface<DataMining::JsonValue>(restoredJson);
    auto bytesData = DataMining::QueryInterface<DataMining::BytesValue>(restoredBytes);
    ASSERT_NE(jsonData, nullptr);
    ASSERT_NE(bytesData, nullptr);
    EXPECT_EQ(jsonData->GetValue().at("scene").get<std::string>(), "one_touch_taxi");
    EXPECT_EQ(bytesData->GetValue().size(), 3U);
}

HWTEST_F(DataMiningEtlTest, DataQueryInterfaceReturnsAliasedSharedPtr009, TestSize.Level1)
{
    auto payload = std::make_shared<DataMining::StringValue>("payload");
    std::shared_ptr<DataMining::DataValue> data(payload);

    auto text = DataMining::QueryInterface<DataMining::StringValue>(data);
    auto base = DataMining::QueryInterface<DataMining::DataValue>(data);

    ASSERT_NE(text, nullptr);
    ASSERT_NE(base, nullptr);
    EXPECT_EQ(text->GetValue(), "payload");
    EXPECT_EQ(base.get(), static_cast<DataMining::DataValue *>(payload.get()));
    EXPECT_FALSE(text.owner_before(base));
    EXPECT_FALSE(base.owner_before(text));
}
} // namespace OHOS::Test
