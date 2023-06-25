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

#include "error_code.h"
#include "udmf_service.h"

namespace OHOS {
namespace UDMF {
/*
 * UDMF server stub
 */
class UdmfServiceStub : public UdmfService, public DistributedData::FeatureSystem::Feature {
public:
    UdmfServiceStub();
    virtual ~UdmfServiceStub() override;
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

    bool VerifyPermission(const std::string &permission);

    const std::string READ_PERMISSION = "ohos.permission.READ_UDMF_DATA";
    const std::string WRITE_PERMISSION = "ohos.permission.WRITE_UDMF_DATA";
    const std::string SYNC_PERMISSION = "ohos.permission.SYNC_UDMF_DATA";

    using UdmfServiceFunc = int32_t (UdmfServiceStub::*)(MessageParcel &data, MessageParcel &reply);
    std::map<uint32_t, UdmfServiceFunc> memberFuncMap_;
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_SERVICE_STUB_H
