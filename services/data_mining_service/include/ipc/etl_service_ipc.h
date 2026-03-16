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

#ifndef OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_IPC_ETL_SERVICE_IPC_H
#define OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_IPC_ETL_SERVICE_IPC_H

#include <iremote_broker.h>

namespace OHOS::DataMining {
inline constexpr int32_t DATA_MINING_ETL_SERVICE_ID = 1311;
inline constexpr int32_t DATA_MINING_LOAD_SA_TIMEOUT = 5000;

class IEtlService : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DataMining.IEtlService");

    enum RequestCode : uint32_t {
        REGISTER_PLUGIN = 1,
        REGISTER_PIPELINE,
        DISPATCH,
    };

    virtual int32_t RegisterPlugin(const std::string &pluginContent) = 0;
    virtual int32_t RegisterPipeline(const std::string &pipelineContent) = 0;
    virtual int32_t Dispatch(const std::string &dispatchContent) = 0;
};
} // namespace OHOS::DataMining

#endif // OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_IPC_ETL_SERVICE_IPC_H
