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

#define LOG_TAG "DataMiningEtlPxy"
#include "ipc/etl_service_proxy.h"

#include "log_print.h"

namespace OHOS::DataMining {
namespace {
constexpr int32_t E_OK = 0;
constexpr int32_t E_ERROR = -1;
}

EtlServiceProxy::EtlServiceProxy(const sptr<IRemoteObject> &remote) : IRemoteProxy<IEtlService>(remote)
{
}

int32_t EtlServiceProxy::RegisterPlugin(const std::string &pluginContent)
{
    return SendStringRequest(IEtlService::REGISTER_PLUGIN, pluginContent);
}

int32_t EtlServiceProxy::RegisterPipeline(const std::string &pipelineContent)
{
    return SendStringRequest(IEtlService::REGISTER_PIPELINE, pipelineContent);
}

int32_t EtlServiceProxy::Dispatch(const std::string &dispatchContent)
{
    return SendStringRequest(IEtlService::DISPATCH, dispatchContent);
}

int32_t EtlServiceProxy::SendStringRequest(uint32_t code, const std::string &content)
{
    auto remote = Remote();
    if (remote == nullptr) {
        return E_ERROR;
    }

    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    if (!data.WriteInterfaceToken(GetDescriptor()) || !data.WriteString(content)) {
        return E_ERROR;
    }

    int32_t status = remote->SendRequest(code, data, reply, option);
    if (status != E_OK) {
        ZLOGE("send request failed, code:%{public}u, status:%{public}d", code, status);
        return status;
    }
    return reply.ReadInt32();
}
} // namespace OHOS::DataMining
