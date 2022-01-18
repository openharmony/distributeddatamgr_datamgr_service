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

#define LOG_TAG "IRdbStoreStub"

#include "rdb_store_stub.h"
#include "log_print.h"

namespace OHOS::DistributedKv {
int RdbStoreStub::OnRemoteSetDistributedTables(MessageParcel &data, MessageParcel &reply)
{
    std::vector<std::string> tables;
    data.ReadStringVector(&tables);
    reply.WriteInt32(SetDistributedTables(tables));
    return 0;
}

bool RdbStoreStub::CheckInterfaceToken(MessageParcel& data)
{
    auto localDescriptor = IRdbStore::GetDescriptor();
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (remoteDescriptor != localDescriptor) {
        ZLOGE("interface token is not equal");
        return false;
    }
    return true;
}

int RdbStoreStub::OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option)
{
    ZLOGI("%{public}d", code);
    if (!CheckInterfaceToken(data))  {
        return -1;
    }
    if (code >= 0 && code < RDB_STORE_CMD_MAX) {
        return (this->*HANDLERS[code])(data, reply);
    }
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}
}

