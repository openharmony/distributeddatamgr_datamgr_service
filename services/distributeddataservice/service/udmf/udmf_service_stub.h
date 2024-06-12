/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef UDMF_SERVICE_STUB_H
#define UDMF_SERVICE_STUB_H

#include <map>
#include <string>

#include "feature/feature_system.h"
#include "message_parcel.h"

#include "distributeddata_udmf_ipc_interface_code.h"
#include "error_code.h"
#include "udmf_service.h"

namespace OHOS {
namespace UDMF {
/*
 * UDMF server stub
 */
class UdmfServiceStub : public UdmfService, public DistributedData::FeatureSystem::Feature {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply) override;

private:
    int32_t OnSetData(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetData(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetBatchData(MessageParcel &data, MessageParcel &reply);
    int32_t OnUpdateData(MessageParcel &data, MessageParcel &reply);
    int32_t OnDeleteData(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetSummary(MessageParcel &data, MessageParcel &reply);
    int32_t OnAddPrivilege(MessageParcel &data, MessageParcel &reply);
    int32_t OnSync(MessageParcel &data, MessageParcel &reply);
    int32_t OnIsRemoteData(MessageParcel &data, MessageParcel &reply);
    int32_t OnSetAppShareOption(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetAppShareOption(MessageParcel &data, MessageParcel &reply);
    int32_t OnRemoveAppShareOption(MessageParcel &data, MessageParcel &reply);

    using Handler = int32_t (UdmfServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    static constexpr Handler HANDLERS[static_cast<uint32_t>(UdmfServiceInterfaceCode::CODE_BUTT)] = {
        &UdmfServiceStub::OnSetData,
        &UdmfServiceStub::OnGetData,
        &UdmfServiceStub::OnGetBatchData,
        &UdmfServiceStub::OnUpdateData,
        &UdmfServiceStub::OnDeleteData,
        &UdmfServiceStub::OnGetSummary,
        &UdmfServiceStub::OnAddPrivilege,
        &UdmfServiceStub::OnSync,
        &UdmfServiceStub::OnIsRemoteData,
        &UdmfServiceStub::OnSetAppShareOption,
        &UdmfServiceStub::OnGetAppShareOption,
        &UdmfServiceStub::OnRemoveAppShareOption
    };
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_SERVICE_STUB_H
