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

#define LOG_TAG "DataMiningService"
#include "data_mining_service_impl.h"

#include "log_print.h"
#include "scene_etl_control.h"

namespace OHOS::DistributedData {
namespace {
constexpr const char *PLUGIN_CONFIG_DIR = "/system/etc/distributeddata/data_mining/plugins";
constexpr const char *PIPELINE_CONFIG_DIR = "/system/etc/distributeddata/data_mining/pipelines";
constexpr int32_t IPC_SUCCESS = 0;
constexpr int32_t IPC_ERROR = -1;
constexpr char16_t FEATURE_DESCRIPTOR[] = u"OHOS.DistributedData.ServiceProxy";
}

__attribute__((used)) DataMiningServiceImpl::Factory DataMiningServiceImpl::factory_;

DataMiningServiceImpl::Factory::Factory()
{
    FeatureSystem::GetInstance().RegisterCreator(DataMiningServiceImpl::SERVICE_NAME, [this]() {
        if (product_ == nullptr) {
            product_ = std::make_shared<DataMiningServiceImpl>();
        }
        return product_;
    });
}

DataMiningServiceImpl::Factory::~Factory()
{
    product_ = nullptr;
}

int DataMiningServiceImpl::OnRemoteRequest(uint32_t code, OHOS::MessageParcel &data, OHOS::MessageParcel &reply)
{
    if (data.ReadInterfaceToken() != FEATURE_DESCRIPTOR) {
        return IPC_ERROR;
    }

    int32_t status = OHOS::DataMining::DATA_MINING_ERROR;
    std::string scene = data.ReadString();
    switch (static_cast<OHOS::DataMining::SceneEtlRequestCode>(code)) {
        case OHOS::DataMining::SceneEtlRequestCode::ENABLE_SCENE:
            status = manager_.StartScene(scene);
            break;
        case OHOS::DataMining::SceneEtlRequestCode::DISABLE_SCENE:
            status = manager_.StopScene(scene);
            break;
        default:
            ZLOGE("unknown data mining request code:%{public}u", code);
            break;
    }

    if (!reply.WriteInt32(status)) {
        return IPC_ERROR;
    }
    return IPC_SUCCESS;
}

int32_t DataMiningServiceImpl::OnInitialize()
{
    int32_t status = manager_.LoadDefaultConfigs(PLUGIN_CONFIG_DIR, PIPELINE_CONFIG_DIR);
    if (status != 0) {
        ZLOGW("load default data mining configs finished with warnings");
    }
    // OnInitialize 只启动带订阅/定时策略的 pipeline。
    // 纯手动 pipeline 留给 TriggerPipeline 按需拉起。
    status = manager_.StartAutoPipelines();
    if (status != 0) {
        ZLOGW("start data mining pipelines finished with warnings");
    }
    return FeatureSystem::STUB_SUCCESS;
}

int32_t DataMiningServiceImpl::OnBind(const FeatureSystem::Feature::BindInfo &bindInfo)
{
    manager_.BindExecutors(bindInfo.executors);
    return FeatureSystem::STUB_SUCCESS;
}

int32_t DataMiningServiceImpl::RegisterPlugin(const std::string &pluginConfigPath)
{
    return manager_.RegisterPlugin(pluginConfigPath);
}

int32_t DataMiningServiceImpl::RegisterPipeline(const std::string &pipelineConfigPath)
{
    return manager_.RegisterPipeline(pipelineConfigPath);
}

int32_t DataMiningServiceImpl::StartPipeline(const std::string &name)
{
    return manager_.StartPipeline(name);
}

int32_t DataMiningServiceImpl::StopPipeline(const std::string &name)
{
    return manager_.StopPipeline(name);
}

int32_t DataMiningServiceImpl::TriggerPipeline(
    const std::string &name, std::shared_ptr<OHOS::DataMining::Context> context)
{
    return manager_.TriggerPipeline(name, context);
}
} // namespace OHOS::DistributedData
