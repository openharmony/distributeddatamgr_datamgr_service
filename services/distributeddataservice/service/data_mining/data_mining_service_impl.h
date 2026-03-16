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

#ifndef OHOS_DISTRIBUTEDDATASERVICE_SERVICE_DATA_MINING_SERVICE_IMPL_H
#define OHOS_DISTRIBUTEDDATASERVICE_SERVICE_DATA_MINING_SERVICE_IMPL_H

#include <memory>

#include "feature/feature_system.h"
#include "service/data_mining_manager.h"

namespace OHOS {
class MessageParcel;
}

namespace OHOS::DistributedData {
class DataMiningServiceImpl final : public FeatureSystem::Feature {
public:
    static constexpr const char *SERVICE_NAME = "data_mining";

    class Factory final {
    public:
        Factory();
        ~Factory();

    private:
        std::shared_ptr<DataMiningServiceImpl> product_;
    };

    DataMiningServiceImpl() = default;
    ~DataMiningServiceImpl() override = default;

    int OnRemoteRequest(uint32_t code, OHOS::MessageParcel &data, OHOS::MessageParcel &reply) override;
    int32_t OnInitialize() override;
    int32_t OnBind(const FeatureSystem::Feature::BindInfo &bindInfo) override;

private:
    int32_t RegisterPlugin(const std::string &pluginConfigPath);
    int32_t RegisterPipeline(const std::string &pipelineConfigPath);
    int32_t StartPipeline(const std::string &name);
    int32_t StopPipeline(const std::string &name);
    int32_t TriggerPipeline(const std::string &name, std::shared_ptr<OHOS::DataMining::Context> context);

    static Factory factory_;
    OHOS::DataMining::DataMiningManager manager_;
};
} // namespace OHOS::DistributedData

#endif // OHOS_DISTRIBUTEDDATASERVICE_SERVICE_DATA_MINING_SERVICE_IMPL_H
