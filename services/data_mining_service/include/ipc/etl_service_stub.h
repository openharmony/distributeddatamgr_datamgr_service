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

#ifndef OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_IPC_ETL_SERVICE_STUB_H
#define OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_IPC_ETL_SERVICE_STUB_H

#include <iremote_stub.h>

#include "etl_service_ipc.h"

namespace OHOS::DataMining {
class EtlServiceStub : public OHOS::IRemoteStub<IEtlService> {
public:
    ~EtlServiceStub() override = default;

    int32_t OnRemoteRequest(uint32_t code, OHOS::MessageParcel &data, OHOS::MessageParcel &reply,
        OHOS::MessageOption &option) override;
};
} // namespace OHOS::DataMining

#endif // OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_IPC_ETL_SERVICE_STUB_H
