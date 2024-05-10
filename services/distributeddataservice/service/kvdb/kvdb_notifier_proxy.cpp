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

#define LOG_TAG "KVDBNotifierProxy"

#include "kvdb_notifier_proxy.h"
#include <cinttypes>
#include "distributeddata_kvdb_ipc_interface_code.h"
#include "itypes_util.h"
#include "log_print.h"
#include "message_parcel.h"
#include "message_option.h"
#include "utils/anonymous.h"

namespace OHOS {
namespace DistributedKv {
using namespace OHOS::DistributedData;
#define IPC_SEND(code, reply, ...)                                              \
    ({                                                                          \
        int32_t __status = SUCCESS;                                             \
        do {                                                                    \
            MessageParcel request;                                              \
            if (!request.WriteInterfaceToken(GetDescriptor())) {                \
                __status = IPC_PARCEL_ERROR;                                    \
                break;                                                          \
            }                                                                   \
            if (!ITypesUtil::Marshal(request, ##__VA_ARGS__)) {                 \
                __status = IPC_PARCEL_ERROR;                                    \
                break;                                                          \
            }                                                                   \
            MessageOption option;                                               \
            auto result = remote_->SendRequest((code), request, reply, option); \
            if (result != 0) {                                                  \
                __status = IPC_ERROR;                                           \
                break;                                                          \
            }                                                                   \
        } while (0);                                                            \
        __status;                                                               \
    })

KVDBNotifierProxy::KVDBNotifierProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IKVDBNotifier>(impl)
{
    remote_ = Remote();
}

void KVDBNotifierProxy::SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(static_cast<uint32_t>(
        KVDBNotifierCode::TRANS_SYNC_COMPLETED), reply, results, sequenceId);
    if (status != SUCCESS) {
        ZLOGE("status:%{public}d, results size:%{public}zu, sequenceId:%{public}" PRIu64,
            status, results.size(), sequenceId);
    }
}

void KVDBNotifierProxy::SyncCompleted(uint64_t seqNum, ProgressDetail &&detail)
{
    MessageParcel reply;
    int32_t status =
        IPC_SEND(static_cast<uint32_t>(KVDBNotifierCode::TRANS_CLOUD_SYNC_COMPLETED), reply, seqNum, detail);
    if (status != SUCCESS) {
        ZLOGE("status:%{public}d, sequenceId:%{public}" PRIu64, status, seqNum);
    }
}

void KVDBNotifierProxy::OnRemoteChange(const std::map<std::string, bool> &mask)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(static_cast<uint32_t>(
        KVDBNotifierCode::TRANS_ON_REMOTE_CHANGED), reply, mask);
    if (status != SUCCESS) {
        ZLOGE("status:%{public}d, mask:%{public}zu", status, mask.size());
    }
}

void KVDBNotifierProxy::OnSwitchChange(const SwitchNotification &notification)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(static_cast<uint32_t>(
        KVDBNotifierCode::TRANS_ON_SWITCH_CHANGED), reply, notification);
    if (status != SUCCESS) {
        ZLOGE("status:%{public}d, device:%{public}s",
            status, Anonymous::Change(notification.deviceId).c_str());
    }
}
} // namespace DistributedKv
} // namespace OHOS