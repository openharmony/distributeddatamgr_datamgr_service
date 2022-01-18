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

#define LOG_TAG "RdbServiceProxy"

#include "rdb_service_proxy.h"
#include "irdb_store.h"
#include "log_print.h"

namespace OHOS::DistributedKv {
RdbServiceProxy::RdbServiceProxy(const sptr<IRemoteObject> &object)
    : IRemoteProxy<IRdbService>(object)
{
}

sptr<IRdbStore> RdbServiceProxy::GetRdbStore(const RdbStoreParam& param)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(IRdbService::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return nullptr;
    }
    if (!param.Marshalling(data)) {
        return nullptr;
    }
    
    MessageParcel reply;
    MessageOption option;
    if (Remote()->SendRequest(RDB_SERVICE_CMD_GET_STORE, data, reply, option) != 0) {
        ZLOGE("send request failed");
        return nullptr;
    }
    
    auto remoteObject = reply.ReadRemoteObject();
    return iface_cast<IRdbStore>(remoteObject);
}

int RdbServiceProxy::RegisterClientDeathRecipient(const std::string& bundleName, sptr<IRemoteObject> object)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(IRdbService::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return -1;
    }
    if (!data.WriteString(bundleName)) {
        ZLOGE("write bundle name failed");
        return -1;
    }
    if (!data.WriteRemoteObject(object)) {
        ZLOGE("write remote object failed");
        return -1;
    }
    MessageParcel reply;
    MessageOption option;
    if (Remote()->SendRequest(RDB_SERVICE_CMD_REGISTER_CLIENT_DEATH, data, reply, option) != 0) {
        ZLOGE("send request failed");
        return -1;
    }
    int32_t res = -1;
    return reply.ReadInt32(res) ? res : -1;
}
}