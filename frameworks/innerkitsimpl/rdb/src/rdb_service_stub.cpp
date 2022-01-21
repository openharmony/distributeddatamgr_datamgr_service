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

#define LOG_TAG "RdbServiceStub"

#include "rdb_service_stub.h"
#include "log_print.h"
#include "irdb_syncer.h"
#include "itypes_util.h"

namespace OHOS::DistributedRdb {
int RdbServiceStub::OnRemoteGetRdbSyncer(MessageParcel& data, MessageParcel& reply)
{
    RdbSyncerParam param;
    sptr<IRdbSyncer> store;
    if (DistributedKv::ITypesUtil::UnMarshalling(data, param)) {
        store = GetRdbSyncerInner(param);
    }
    reply.WriteRemoteObject(store->AsObject().GetRefPtr());
    return 0;
}

int RdbServiceStub::OnRemoteRegisterClientDeathRecipient(MessageParcel &data, MessageParcel &reply)
{
    std::string bundleName = data.ReadString();
    auto remoteObject = data.ReadRemoteObject();
    reply.WriteInt32(RegisterClientDeathRecipient(bundleName, remoteObject));
    return 0;
}

bool RdbServiceStub::CheckInterfaceToken(MessageParcel& data)
{
    auto localDescriptor = IRdbService::GetDescriptor();
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (remoteDescriptor != localDescriptor) {
        ZLOGE("interface token is not equal");
        return false;
    }
    return true;
}
int RdbServiceStub::OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option)
{
    ZLOGI("code=%{public}d", code);
    if (!CheckInterfaceToken(data)) {
        return -1;
    }
    if (code >= 0 && code < RDB_SERVICE_CMD_MAX) {
        return (this->*HANDLES[code])(data, reply);
    }
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}
}