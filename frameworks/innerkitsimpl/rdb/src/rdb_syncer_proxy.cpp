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

#include "rdb_syncer_proxy.h"

#define LOG_TAG "RdbSyncerProxy"
#include "log_print.h"

namespace OHOS::DistributedRdb {
RdbSyncerProxy::RdbSyncerProxy(const sptr<IRemoteObject> &object)
    : IRemoteProxy<IRdbSyncer>(object)
{
}

int RdbSyncerProxy::SetDistributedTables(const std::vector<std::string> &tables)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(IRdbSyncer::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return -1;
    }
    if (!data.WriteStringVector(tables)) {
        ZLOGE("write tables failed");
        return -1;
    }
    
    MessageParcel reply;
    MessageOption option;
    if (Remote()->SendRequest(RDB_SYNCER_CMD_SET_DIST_TABLES, data, reply, option) != 0) {
        ZLOGE("send request failed");
        return -1;
    }
    
    int32_t res = -1;
    return reply.ReadInt32(res) ? res : -1;
}
}

