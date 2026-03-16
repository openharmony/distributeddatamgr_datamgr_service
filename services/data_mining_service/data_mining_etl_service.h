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

#ifndef OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_DATA_MINING_ETL_SERVICE_H
#define OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_DATA_MINING_ETL_SERVICE_H

#include "ipc/etl_service_stub.h"
#include "service/etl_runtime_manager.h"
#include "system_ability.h"

namespace OHOS::DataMining {
class DataMiningEtlService final : public SystemAbility, public EtlServiceStub {
    DECLARE_SYSTEM_ABILITY(DataMiningEtlService);

public:
    explicit DataMiningEtlService(int32_t systemAbilityId = DATA_MINING_ETL_SERVICE_ID, bool runOnCreate = false);
    ~DataMiningEtlService() override = default;

    void OnStart() override;
    void OnStop() override;

    int32_t RegisterPlugin(const std::string &pluginContent) override;
    int32_t RegisterPipeline(const std::string &pipelineContent) override;
    int32_t Dispatch(const std::string &dispatchContent) override;

private:
    EtlRuntimeManager manager_;
};
} // namespace OHOS::DataMining

#endif // OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_DATA_MINING_ETL_SERVICE_H
