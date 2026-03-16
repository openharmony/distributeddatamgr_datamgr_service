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

#define LOG_TAG "DataMiningEtlSvc"
#include "data_mining_etl_service.h"

#include "log_print.h"
#include "serializable/serializable.h"
#include "system_ability_definition.h"

namespace OHOS::DataMining {
namespace {
constexpr int32_t E_ERROR = -1;
}

REGISTER_SYSTEM_ABILITY_BY_ID(DataMiningEtlService, DATA_MINING_ETL_SERVICE_ID, false);

DataMiningEtlService::DataMiningEtlService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate)
{
}

void DataMiningEtlService::OnStart()
{
    if (!Publish(this)) {
        ZLOGE("publish etl service failed");
    }
}

void DataMiningEtlService::OnStop()
{
}

int32_t DataMiningEtlService::RegisterPlugin(const std::string &pluginContent)
{
    return manager_.RegisterPlugin(pluginContent);
}

int32_t DataMiningEtlService::RegisterPipeline(const std::string &pipelineContent)
{
    return manager_.RegisterPipeline(pipelineContent);
}

int32_t DataMiningEtlService::Dispatch(const std::string &dispatchContent)
{
    DispatchRequest request;
    if (!OHOS::DistributedData::Serializable::Unmarshall(dispatchContent, request)) {
        return E_ERROR;
    }
    return manager_.Dispatch(request);
}
} // namespace OHOS::DataMining
