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

#ifndef OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_IPC_ETL_SERVICE_PROXY_H
#define OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_IPC_ETL_SERVICE_PROXY_H

#include <iremote_proxy.h>

#include "etl_service_ipc.h"

namespace OHOS::DataMining {
class EtlServiceProxy final : public OHOS::IRemoteProxy<IEtlService> {
public:
    explicit EtlServiceProxy(const sptr<IRemoteObject> &remote);
    ~EtlServiceProxy() override = default;

    int32_t RegisterPlugin(const std::string &pluginContent) override;
    int32_t RegisterPipeline(const std::string &pipelineContent) override;
    int32_t Dispatch(const std::string &dispatchContent) override;

private:
    int32_t SendStringRequest(uint32_t code, const std::string &content);

    static inline BrokerDelegator<EtlServiceProxy> delegator_;
};
} // namespace OHOS::DataMining

#endif // OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_IPC_ETL_SERVICE_PROXY_H
