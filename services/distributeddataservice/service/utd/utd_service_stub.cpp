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

#define LOG_TAG "UtdServiceStub"
#include "utd_service_stub.h"

#include "ipc_skeleton.h"
#include "log_print.h"
#include "qos_manager.h"
#include "string_ex.h"
#include "utd_tlv_util.h"

namespace OHOS {
namespace UDMF {
using OHOS::DistributedData::QosManager;
constexpr UtdServiceStub::Handler
    UtdServiceStub::HANDLERS[static_cast<uint32_t>(UtdServiceInterfaceCode::CODE_BUTT)];
int UtdServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    QosManager qos(false);

    ZLOGI("start##code = %{public}u callingPid:%{public}u callingUid:%{public}u.", code, IPCSkeleton::GetCallingPid(),
        IPCSkeleton::GetCallingUid());
    std::u16string myDescripter = UtdServiceStub::GetDescriptor();
    std::u16string remoteDescripter = data.ReadInterfaceToken();
    if (myDescripter != remoteDescripter) {
        ZLOGE("end##descriptor checked fail,myDescripter = %{public}s,remoteDescripter = %{public}s.",
            Str16ToStr8(myDescripter).c_str(), Str16ToStr8(remoteDescripter).c_str());
        return E_IPC;
    }
    if (static_cast<uint32_t>(UtdServiceInterfaceCode::CODE_HEAD) > code ||
        code >= static_cast<uint32_t>(UtdServiceInterfaceCode::CODE_BUTT)) {
        return E_IPC;
    }
    return (this->*HANDLERS[code])(data, reply);
}

int32_t UtdServiceStub::OnRegServiceNotifier(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> iUtdNotifier;
    if (!ITypesUtil::Unmarshal(data, iUtdNotifier)) {
        ZLOGE("Unmarshal failed!");
        return E_READ_PARCEL_ERROR;
    }
    if (iUtdNotifier == nullptr) {
        ZLOGE("iUtdNotifier is null!");
        return E_ERROR;
    }
    int32_t status = RegServiceNotifier(iUtdNotifier);
    if (!ITypesUtil::Marshal(reply, static_cast<int32_t>(E_OK), status)) {
        ZLOGE("Marshal failed:%{public}d", status);
        return E_WRITE_PARCEL_ERROR;
    }
    return E_OK;
}

int32_t UtdServiceStub::OnRegisterTypeDescriptors(MessageParcel &data, MessageParcel &reply)
{
    std::vector<TypeDescriptorCfg> descriptors;
    if (!ITypesUtil::Unmarshal(data, descriptors)) {
        ZLOGE("Unmarshal failed!");
        return E_READ_PARCEL_ERROR;
    }
    int32_t status = RegisterTypeDescriptors(descriptors);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal failed:%{public}d", status);
        return E_WRITE_PARCEL_ERROR;
    }
    return E_OK;
}

int32_t UtdServiceStub::OnUnregisterTypeDescriptors(MessageParcel &data, MessageParcel &reply)
{
    std::vector<std::string> typeIds;
    if (!ITypesUtil::Unmarshal(data, typeIds)) {
        ZLOGE("Unmarshal failed!");
        return E_READ_PARCEL_ERROR;
    }
    int32_t status = UnregisterTypeDescriptors(typeIds);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal failed:%{public}d", status);
        return E_WRITE_PARCEL_ERROR;
    }
    return E_OK;
}

} // namespace UDMF
} // namespace OHOS