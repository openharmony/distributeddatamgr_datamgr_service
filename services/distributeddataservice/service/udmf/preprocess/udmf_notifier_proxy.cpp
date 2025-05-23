/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#define LOG_TAG "UDMFNotifierProxy"

#include "udmf_notifier_proxy.h"
#include "distributeddata_udmf_ipc_interface_code.h"
#include "log_print.h"
#include "message_parcel.h"
#include "message_option.h"
#include "utils/anonymous.h"
#include "udmf_types_util.h"

namespace OHOS {
namespace UDMF {
#define IPC_SEND(code, reply, ...)                                                           \
    ({                                                                                       \
        int32_t ipcStatus = E_OK;                                                            \
        do {                                                                                 \
            MessageParcel request;                                                           \
            if (!request.WriteInterfaceToken(GetDescriptor())) {                             \
                ipcStatus = E_WRITE_PARCEL_ERROR;                                            \
                break;                                                                       \
            }                                                                                \
            if (!ITypesUtil::Marshal(request, ##__VA_ARGS__)) {                              \
                ipcStatus = E_WRITE_PARCEL_ERROR;                                            \
                break;                                                                       \
            }                                                                                \
            MessageOption option;                                                            \
            auto result = remote_->SendRequest((code), request, reply, option);              \
            if (result != 0) {                                                               \
                ZLOGE("SendRequest failed, result = %{public}d!", result);                   \
                ipcStatus = E_IPC;                                                           \
                break;                                                                       \
            }                                                                                \
                                                                                             \
            ITypesUtil::Unmarshal(reply, ipcStatus);                                         \
        } while (0);                                                                         \
        ipcStatus;                                                                           \
    })

UdmfNotifierProxy::UdmfNotifierProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IUdmfNotifier>(impl)
{
    remote_ = Remote();
}

void UdmfNotifierProxy::HandleDelayObserver(const std::string &key, const DataLoadInfo &dataLoadInfo)
{
    MessageParcel reply;
    int32_t status =
        IPC_SEND(static_cast<uint32_t>(UdmfServiceInterfaceCode::HANDLE_DELAY_OBSERVER), reply, key, dataLoadInfo);
    if (status != E_OK) {
        ZLOGE("status:%{public}d" PRIu64, status);
    }
}

DelayDataCallbackProxy::DelayDataCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IDelayDataCallback>(impl)
{
    remote_ = Remote();
}

void DelayDataCallbackProxy::DelayDataCallback(const std::string &key, const UnifiedData &data)
{
    MessageParcel reply;
    int32_t status =
        IPC_SEND(static_cast<uint32_t>(UdmfServiceInterfaceCode::HANDLE_DELAY_OBSERVER), reply, key, data);
    if (status != E_OK) {
        ZLOGE("status:%{public}d" PRIu64, status);
    }
}
} // namespace UDMF
} // namespace OHOS