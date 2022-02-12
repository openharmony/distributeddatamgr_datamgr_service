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
#include "itypes_util.h"

namespace OHOS::DistributedRdb {
int32_t RdbServiceStub::OnRemoteObtainDistributedTableName(MessageParcel &data, MessageParcel &reply)
{
    std::string device;
    if (!data.ReadString(device)) {
        ZLOGE("read device failed");
        reply.WriteString("");
        return RDB_OK;
    }

    std::string table;
    if (!data.ReadString(table)) {
        ZLOGE("read table failed");
        reply.WriteString("");
        return RDB_OK;
    }

    reply.WriteString(ObtainDistributedTableName(device, table));
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteGetConnectDevices(MessageParcel& data, MessageParcel& reply)
{
    std::vector<std::string> devices = GetConnectDevices();
    if (!reply.WriteStringVector(devices)) {
        ZLOGE("write devices failed");
    }

    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteInitNotifier(MessageParcel &data, MessageParcel &reply)
{
    int32_t error = RDB_ERROR;
    RdbSyncerParam param;
    if (!DistributedKv::ITypesUtil::UnMarshalling(data, param)) {
        ZLOGE("read param failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    auto notifier = data.ReadRemoteObject();
    if (notifier == nullptr) {
        ZLOGE("read object failed");
        reply.WriteInt32(error);
        return RDB_OK;
    }
    if (InitNotifier(param, notifier) != RDB_OK) {
        ZLOGE("init notifier failed");
        reply.WriteInt32(error);
        return RDB_OK;
    }
    ZLOGI("success");
    reply.WriteInt32(RDB_OK);
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteSetDistributedTables(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    if (!DistributedKv::ITypesUtil::UnMarshalling(data, param)) {
        ZLOGE("read param failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    std::vector<std::string> tables;
    data.ReadStringVector(&tables);
    reply.WriteInt32(SetDistributedTables(param, tables));
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteDoSync(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    if (!DistributedKv::ITypesUtil::UnMarshalling(data, param)) {
        ZLOGE("read param failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    SyncOption option {};
    if (!DistributedKv::ITypesUtil::UnMarshalling(data, option)) {
        ZLOGE("read option failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    RdbPredicates predicates;
    if (!DistributedKv::ITypesUtil::UnMarshalling(data, predicates)) {
        ZLOGE("read predicates failed");
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
    if (!DistributedKv::ITypesUtil::UnMarshalling(data, param)) {
        ZLOGE("read param failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    uint32_t seqNum;
    if (!data.ReadUint32(seqNum)) {
        ZLOGI("read seq num failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    SyncOption option {};
    if (!DistributedKv::ITypesUtil::UnMarshalling(data, option)) {
        ZLOGE("read option failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    RdbPredicates predicates;
    if (!DistributedKv::ITypesUtil::UnMarshalling(data, predicates)) {
        ZLOGE("read predicates failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    
    reply.WriteInt32(DoAsync(param, seqNum, option, predicates));
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteDoSubscribe(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    if (!DistributedKv::ITypesUtil::UnMarshalling(data, param)) {
        ZLOGE("read param failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    reply.WriteInt32(DoSubscribe(param));
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteDoUnSubscribe(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    if (!DistributedKv::ITypesUtil::UnMarshalling(data, param)) {
        ZLOGE("read param failed");
        reply.WriteInt32(RDB_ERROR);
        return RDB_OK;
    }
    reply.WriteInt32(DoUnSubscribe(param));
    return RDB_OK;
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
        return RDB_ERROR;
    }
    if (code >= 0 && code < RDB_SERVICE_CMD_MAX) {
        return (this->*HANDLERS[code])(data, reply);
    }
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}
}