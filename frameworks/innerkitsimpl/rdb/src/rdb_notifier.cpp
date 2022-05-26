/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#define LOG_TAG "RdbNotifier"

#include "rdb_notifier.h"
#include <ipc_skeleton.h>
#include "itypes_util.h"
#include "log_print.h"


namespace OHOS::DistributedRdb {
RdbNotifierProxy::RdbNotifierProxy(const sptr<IRemoteObject> &object)
    : IRemoteProxy<IRdbNotifier>(object)
{
    ZLOGI("construct");
}

RdbNotifierProxy::~RdbNotifierProxy() noexcept
{
    ZLOGI("destroy");
}

int32_t RdbNotifierProxy::OnComplete(uint32_t seqNum, const SyncResult &result)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(IRdbNotifier::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return RDB_ERROR;
    }
    if (!data.WriteUint32(seqNum)) {
        ZLOGE("write seq num failed");
        return RDB_ERROR;
    }
    if (!DistributedKv::ITypesUtil::Marshalling(result, data)) {
        return RDB_ERROR;
    }

    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    if (Remote()->SendRequest(RDB_NOTIFIER_CMD_SYNC_COMPLETE, data, reply, option) != 0) {
        ZLOGE("send request failed");
        return RDB_ERROR;
    }
    return RDB_OK;
}

int RdbNotifierProxy::OnChange(const std::string& storeName, const std::vector<std::string> &devices)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(IRdbNotifier::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return RDB_ERROR;
    }
    if (!data.WriteString(storeName)) {
        ZLOGE("write store name failed");
        return RDB_ERROR;
    }
    if (!data.WriteStringVector(devices)) {
        ZLOGE("write devices failed");
        return RDB_ERROR;
    }

    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    if (Remote()->SendRequest(RDB_NOTIFIER_CMD_DATA_CHANGE, data, reply, option) != 0) {
        ZLOGE("send request failed");
        return RDB_ERROR;
    }
    return RDB_OK;
}

RdbNotifierStub::RdbNotifierStub(const RdbSyncCompleteNotifier& completeNotifier,
                                 const RdbDataChangeNotifier& changeNotifier)
    : IRemoteStub<IRdbNotifier>(), completeNotifier_(completeNotifier), changeNotifier_(changeNotifier)
{
    ZLOGI("construct");
}

RdbNotifierStub::~RdbNotifierStub() noexcept
{
    ZLOGI("destroy");
}

bool RdbNotifierStub::CheckInterfaceToken(MessageParcel& data)
{
    auto localDescriptor = IRdbNotifier::GetDescriptor();
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (remoteDescriptor != localDescriptor) {
        ZLOGE("interface token is not equal");
        return false;
    }
    return true;
}

int RdbNotifierStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
                                     MessageOption &option)
{
    ZLOGD("code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    if (!CheckInterfaceToken(data)) {
        return RDB_ERROR;
    }

    if (code >= 0 && code < RDB_NOTIFIER_CMD_MAX) {
        return (this->*HANDLES[code])(data, reply);
    }

    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t RdbNotifierStub::OnCompleteInner(MessageParcel &data, MessageParcel &reply)
{
    uint32_t seqNum;
    if (!data.ReadUint32(seqNum)) {
        ZLOGI("read seq num failed");
        return RDB_ERROR;
    }
    SyncResult result;
    if (!DistributedKv::ITypesUtil::Unmarshal(data, result)) {
        ZLOGE("read sync result failed");
        return RDB_ERROR;
    }
    return OnComplete(seqNum, result);
}

int32_t RdbNotifierStub::OnComplete(uint32_t seqNum, const SyncResult &result)
{
    if (completeNotifier_) {
        completeNotifier_(seqNum, result);
    }
    return RDB_OK;
}

int32_t RdbNotifierStub::OnChangeInner(MessageParcel &data, MessageParcel &reply)
{
    std::string storeName;
    if (!data.ReadString(storeName)) {
        ZLOGE("read store name failed");
        return RDB_ERROR;
    }
    std::vector<std::string> devices;
    if (!data.ReadStringVector(&devices)) {
        ZLOGE("read devices failed");
        return RDB_ERROR;
    }
    return OnChange(storeName, devices);
}

int32_t RdbNotifierStub::OnChange(const std::string& storeName, const std::vector<std::string> &devices)
{
    if (changeNotifier_) {
        changeNotifier_(storeName, devices);
    }
    return RDB_OK;
}
} // namespace OHOS::DistributedRdb
