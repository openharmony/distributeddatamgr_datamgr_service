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

#define LOG_TAG "DataMiningEtlCli"
#include "service/etl_runtime_client.h"

#include "if_system_ability_manager.h"
#include "ipc/etl_service_ipc.h"
#include "ipc/etl_service_proxy.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "serializable/serializable.h"

namespace OHOS::DataMining {
namespace {
constexpr int32_t E_ERROR = -1;
}

int32_t SaEtlRuntimeClient::RegisterPlugin(const PluginDescription &description)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return SendPluginLocked(description);
}

int32_t SaEtlRuntimeClient::RegisterPipeline(const PipelineDescription &description)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return SendPipelineLocked(description);
}

int32_t SaEtlRuntimeClient::Dispatch(const DispatchRequest &request)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return SendDispatchLocked(request);
}

sptr<IEtlService> SaEtlRuntimeClient::GetServiceLocked()
{
    if (service_ != nullptr) {
        return service_;
    }

    // DDMS 侧按需拉起 ETL SA。
    // 如果 SA 尚未存在，先 CheckSystemAbility，再通过 LoadSystemAbility 拉起 1311。
    auto manager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (manager == nullptr) {
        return nullptr;
    }

    auto remote = manager->CheckSystemAbility(DATA_MINING_ETL_SERVICE_ID);
    if (remote == nullptr) {
        remote = manager->LoadSystemAbility(DATA_MINING_ETL_SERVICE_ID, DATA_MINING_LOAD_SA_TIMEOUT);
    }
    if (remote == nullptr) {
        ZLOGE("load etl sa failed, said:%{public}d", DATA_MINING_ETL_SERVICE_ID);
        return nullptr;
    }
    service_ = iface_cast<IEtlService>(remote);
    return service_;
}

int32_t SaEtlRuntimeClient::SendPluginLocked(const PluginDescription &description)
{
    auto service = GetServiceLocked();
    if (service == nullptr) {
        return E_ERROR;
    }
    return service->RegisterPlugin(OHOS::DistributedData::Serializable::Marshall(description));
}

int32_t SaEtlRuntimeClient::SendPipelineLocked(const PipelineDescription &description)
{
    auto service = GetServiceLocked();
    if (service == nullptr) {
        return E_ERROR;
    }
    return service->RegisterPipeline(OHOS::DistributedData::Serializable::Marshall(description));
}

int32_t SaEtlRuntimeClient::SendDispatchLocked(const DispatchRequest &request)
{
    auto service = GetServiceLocked();
    if (service == nullptr) {
        return E_ERROR;
    }
    return service->Dispatch(OHOS::DistributedData::Serializable::Marshall(request));
}
} // namespace OHOS::DataMining
