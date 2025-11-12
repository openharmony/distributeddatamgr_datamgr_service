/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef UTD_SERVICE_STUB_H
#define UTD_SERVICE_STUB_H

#include "feature/feature_system.h"
#include "distributeddata_udmf_ipc_interface_code.h"
#include "utd_service.h"

namespace OHOS {
namespace UDMF {
/*
 * UTD server stub
 */
class UtdServiceStub : public UtdService, public DistributedData::FeatureSystem::Feature {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply) override;

private:
    int32_t OnRegServiceNotifier(MessageParcel &data, MessageParcel &reply);
    int32_t OnRegisterTypeDescriptors(MessageParcel &data, MessageParcel &reply);
    int32_t OnUnregisterTypeDescriptors(MessageParcel &data, MessageParcel &reply);

    using Handler = int32_t (UtdServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    static constexpr Handler HANDLERS[static_cast<uint32_t>(UtdServiceInterfaceCode::CODE_BUTT)] = {
        &UtdServiceStub::OnRegServiceNotifier,
        &UtdServiceStub::OnRegisterTypeDescriptors,
        &UtdServiceStub::OnUnregisterTypeDescriptors,
    };
};
} // namespace UDMF
} // namespace OHOS
#endif // UTD_SERVICE_STUB_H
