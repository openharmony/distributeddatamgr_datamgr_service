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
#include <ipc_skeleton.h>
#include "log_print.h"
#include "itypes_util.h"

namespace OHOS::DistributedRdb {
int32_t RdbServiceStub::OnRemoteObtainDistributedTableName(MessageParcel &data, MessageParcel &reply)
{
    std::string device;
    std::string table;
    if (!DistributedKv::ITypesUtil::Unmarshal(data, device, table)) {
        ZLOGE("read from message parcel failed");
        reply.WriteString("");
        return RDB_OK;
    }

    reply.WriteString(ObtainDistributedTableName(device, table));
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteInitNotifier(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    sptr<IRemoteObject> notifier;
    if (!DistributedKv::ITypesUtil::Unmarshal(data, param, notifier)) {
        ZLOGE("read from message parcel failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    if (notifier == nullptr) {
        ZLOGE("notifier is null");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    if (InitNotifier(param, notifier) != RDB_OK) {
        ZLOGE("init notifier failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    ZLOGI("success");
    reply.WriteInt32(RDB_OK);
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteSetDistributedTables(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    std::vector<std::string> tables;
    if (!DistributedKv::ITypesUtil::Unmarshal(data, param, tables)) {
        ZLOGE("read from message parcel failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }

    reply.WriteInt32(SetDistributedTables(param, tables));
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteDoSync(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    SyncOption option {};
    RdbPredicates predicates;
    if (!DistributedKv::ITypesUtil::Unmarshal(data, param, option, predicates)) {
        ZLOGE("read from message parcel failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }

    SyncResult result;
    if (DoSync(param, option, predicates, result) != RDB_OK) {
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    if (!DistributedKv::ITypesUtil::Marshalling(result, reply)) {
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteDoAsync(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    uint32_t seqNum;
    SyncOption option {};
    RdbPredicates predicates;
    if (!DistributedKv::ITypesUtil::Unmarshal(data, param, seqNum, option, predicates)) {
        ZLOGE("read from message parcel failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }

    reply.WriteInt32(DoAsync(param, seqNum, option, predicates));
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteDoSubscribe(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    if (!DistributedKv::ITypesUtil::Unmarshal(data, param)) {
        ZLOGE("read from message parcel failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    reply.WriteInt32(DoSubscribe(param));
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteDoUnSubscribe(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    if (!DistributedKv::ITypesUtil::Unmarshal(data, param)) {
        ZLOGE("read from message parcel failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    reply.WriteInt32(DoUnSubscribe(param));
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteDoRemoteQuery(MessageParcel& data, MessageParcel& reply)
{
    RdbSyncerParam param;
    std::string device;
    std::string sql;
    std::vector<std::string> selectionArgs;
    if (!DistributedKv::ITypesUtil::Unmarshal(data, param, device, sql, selectionArgs)) {
        ZLOGE("read from message parcel failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }

    sptr<IRemoteObject> resultSet;
    int32_t status = RemoteQuery(param, device, sql, selectionArgs, resultSet);
    if (status != RDB_OK) {
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    reply.WriteInt32(RDB_OK);
    reply.WriteRemoteObject(resultSet);
    return RDB_OK;
}

bool RdbServiceStub::CheckInterfaceToken(MessageParcel& data)
{
    auto localDescriptor = GetDescriptor();
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (remoteDescriptor != localDescriptor) {
        ZLOGE("interface token is not equal");
        return false;
    }
    return true;
}

int RdbServiceStub::OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply)
{
    ZLOGD("code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    if (!CheckInterfaceToken(data)) {
        return RDB_ERROR;
    }
    if (code >= 0 && code < RDB_SERVICE_CMD_MAX) {
        return (this->*HANDLERS[code])(data, reply);
    }
    return RDB_ERROR;
}

int32_t RdbServiceStub::OnRemoteDoCreateTable(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    std::string writePermission;
    std::string readPermission;
    if (!DistributedKv::ITypesUtil::Unmarshal(data, param, writePermission, readPermission)) {
        ZLOGE("read from message parcel failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }

    int32_t status = CreateRDBTable(param, writePermission, readPermission);
    if (status != RDB_OK) {
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    reply.WriteInt32(RDB_OK);
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteDoDestroyTable(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    if (!DistributedKv::ITypesUtil::Unmarshal(data, param)) {
        ZLOGE("read from message parcel failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }

    int32_t status = DestroyRDBTable(param);
    if (status != RDB_OK) {
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    reply.WriteInt32(RDB_OK);
    return RDB_OK;
}
} // namespace OHOS::DistributedRdb
