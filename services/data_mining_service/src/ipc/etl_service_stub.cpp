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

#define LOG_TAG "DataMiningEtlStub"
#include "ipc/etl_service_stub.h"

#include "ipc_types.h"
#include "log_print.h"

namespace OHOS::DataMining {
namespace {
constexpr int32_t E_ERROR = IPC_STUB_UNKNOW_TRANS_ERR;
}

int32_t EtlServiceStub::OnRemoteRequest(
    uint32_t code, OHOS::MessageParcel &data, OHOS::MessageParcel &reply, OHOS::MessageOption &option)
{
    (void)option;
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        return E_ERROR;
    }

    std::string content = data.ReadString();

    int32_t status = E_ERROR;
    switch (code) {
        case IEtlService::REGISTER_PLUGIN:
            status = RegisterPlugin(content);
            break;
        case IEtlService::REGISTER_PIPELINE:
            status = RegisterPipeline(content);
            break;
        case IEtlService::DISPATCH:
            status = Dispatch(content);
            break;
        default:
            ZLOGE("unknown request code:%{public}u", code);
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    if (!reply.WriteInt32(status)) {
        return E_ERROR;
    }
    return status;
}
} // namespace OHOS::DataMining
