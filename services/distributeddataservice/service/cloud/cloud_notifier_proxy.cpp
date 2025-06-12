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
#define LOG_TAG "CloudNotifierProxy"

#include "cloud_notifier_proxy.h"

#include "cloud_service.h"
#include "itypes_util.h"
#include "log_print.h"
#include "utils/anonymous.h"

namespace OHOS::CloudData {
using namespace DistributedRdb;
using NotifierCode = RelationalStore::ICloudNotifierInterfaceCode;

CloudNotifierProxy::CloudNotifierProxy(const sptr<IRemoteObject> &object)
    : IRemoteProxy<CloudNotifierProxyBroker>(object)
{}

CloudNotifierProxy::~CloudNotifierProxy() noexcept {}

int32_t CloudNotifierProxy::OnComplete(uint32_t seqNum, DistributedRdb::Details &&result)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return CloudService::IPC_PARCEL_ERROR;
    }
    if (!ITypesUtil::Marshal(data, seqNum, result)) {
        return CloudService::IPC_PARCEL_ERROR;
    }

    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    auto remote = Remote();
    if (remote == nullptr) {
        ZLOGE("get remote failed, seqNum:%{public}u", seqNum);
        return CloudService::IPC_ERROR;
    }
    auto status = remote->SendRequest(static_cast<uint32_t>(NotifierCode::CLOUD_NOTIFIER_CMD_SYNC_COMPLETE),
        data, reply, option);
    if (status != CloudService::SUCCESS) {
        ZLOGE("seqNum:%{public}u, send request failed, status:%{public}d", seqNum, status);
    }
    return status;
}
} // namespace OHOS::CloudData
