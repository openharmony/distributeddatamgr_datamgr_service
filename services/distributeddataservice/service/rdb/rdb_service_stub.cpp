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
#include "utils/anonymous.h"
#include "rdb_result_set_stub.h"

namespace OHOS::DistributedRdb {
using Anonymous = DistributedData::Anonymous;
int32_t RdbServiceStub::OnRemoteObtainDistributedTableName(MessageParcel &data, MessageParcel &reply)
{
    std::string device;
    std::string table;
    if (!ITypesUtil::Unmarshal(data, device, table)) {
        ZLOGE("Unmarshal device:%{public}s table:%{public}s", Anonymous::Change(device).c_str(), table.c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }

    std::string distributedTableName = ObtainDistributedTableName(device, table);
    if (!ITypesUtil::Marshal(reply, distributedTableName)) {
        ZLOGE("Marshal distributedTableName:%{public}s", distributedTableName.c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return RDB_OK;
}

int32_t RdbServiceStub::OnBeforeOpen(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    if (!ITypesUtil::Unmarshal(data, param)) {
        ZLOGE("Unmarshal bundleName_:%{public}s storeName_:%{public}s", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto status = BeforeOpen(param);
    if (!ITypesUtil::Marshal(reply, status, param)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return RDB_OK;
}

int32_t RdbServiceStub::OnAfterOpen(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    if (!ITypesUtil::Unmarshal(data, param)) {
        ZLOGE("Unmarshal bundleName_:%{public}s storeName_:%{public}s", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto status = AfterOpen(param);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return RDB_OK;
}

int32_t RdbServiceStub::OnDelete(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    if (!ITypesUtil::Unmarshal(data, param)) {
        ZLOGE("Unmarshal storeName_:%{public}s", Anonymous::Change(param.storeName_).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto status = Delete(param);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteInitNotifier(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    sptr<IRemoteObject> notifier;
    if (!ITypesUtil::Unmarshal(data, param, notifier) || notifier == nullptr) {
        ZLOGE("Unmarshal bundleName:%{public}s storeName_:%{public}s notifier is nullptr:%{public}d",
            param.bundleName_.c_str(), Anonymous::Change(param.storeName_).c_str(), notifier == nullptr);
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto status = InitNotifier(param, notifier);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteSetDistributedTables(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    std::vector<std::string> tables;
    std::vector<Reference> references;
    int32_t type;
    if (!ITypesUtil::Unmarshal(data, param, tables, references, type)) {
        ZLOGE("Unmarshal bundleName_:%{public}s storeName_:%{public}s tables size:%{public}zu type:%{public}d",
            param.bundleName_.c_str(), Anonymous::Change(param.storeName_).c_str(), tables.size(), type);
        return IPC_STUB_INVALID_DATA_ERR;
    }

    auto status = SetDistributedTables(param, tables, references, type);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteDoSync(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    Option option {};
    PredicatesMemo predicates;
    if (!ITypesUtil::Unmarshal(data, param, option, predicates)) {
        ZLOGE("Unmarshal bundleName_:%{public}s storeName_:%{public}s tables:%{public}zu", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str(), predicates.tables_.size());
        return IPC_STUB_INVALID_DATA_ERR;
    }

    Details result = {};
    auto status = Sync(param, option, predicates, [&result](Details &&details) { result = std::move(details); });
    if (!ITypesUtil::Marshal(reply, status, result)) {
        ZLOGE("Marshal status:0x%{public}x result size:%{public}zu", status, result.size());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteDoAsync(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    Option option {};
    PredicatesMemo predicates;
    if (!ITypesUtil::Unmarshal(data, param, option, predicates)) {
        ZLOGE("Unmarshal bundleName_:%{public}s storeName_:%{public}s seqNum:%{public}u table:%{public}s",
            param.bundleName_.c_str(), Anonymous::Change(param.storeName_).c_str(), option.seqNum,
            predicates.tables_.empty() ? "null" : predicates.tables_.begin()->c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto status = Sync(param, option, predicates, nullptr);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteDoSubscribe(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    SubscribeOption option;
    if (!ITypesUtil::Unmarshal(data, param, option)) {
        ZLOGE("Unmarshal bundleName_:%{public}s storeName_:%{public}s", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }

    auto status = Subscribe(param, option, nullptr);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteDoUnSubscribe(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    SubscribeOption option;
    if (!ITypesUtil::Unmarshal(data, param)) {
        ZLOGE("Unmarshal bundleName_:%{public}s storeName_:%{public}s", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }

    auto status = UnSubscribe(param, option, nullptr);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteDoRemoteQuery(MessageParcel& data, MessageParcel& reply)
{
    RdbSyncerParam param;
    std::string device;
    std::string sql;
    std::vector<std::string> selectionArgs;
    if (!ITypesUtil::Unmarshal(data, param, device, sql, selectionArgs)) {
        ZLOGE("Unmarshal bundleName_:%{public}s storeName_:%{public}s device:%{public}s sql:%{public}s "
            "selectionArgs size:%{public}zu", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str(), Anonymous::Change(device).c_str(),
            Anonymous::Change(sql).c_str(), selectionArgs.size());
        return IPC_STUB_INVALID_DATA_ERR;
    }

    auto [status, resultSet] = RemoteQuery(param, device, sql, selectionArgs);
    sptr<RdbResultSetStub> object = new RdbResultSetStub(resultSet);
    if (!ITypesUtil::Marshal(reply, status, object->AsObject())) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
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
    ZLOGI("code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    if (!CheckInterfaceToken(data)) {
        return RDB_ERROR;
    }
    if (code >= 0 && code < static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_MAX)) {
        return (this->*HANDLERS[code])(data, reply);
    }
    return RDB_ERROR;
}

int32_t RdbServiceStub::OnRemoteRegisterDetailProgressObserver(MessageParcel& data, MessageParcel& reply)
{
    RdbSyncerParam param;
    if (!ITypesUtil::Unmarshal(data, param)) {
        ZLOGE("Unmarshal bundleName_:%{public}s storeName_:%{public}s", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }

    auto status = RegisterAutoSyncCallback(param, nullptr);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteUnregisterDetailProgressObserver(MessageParcel& data, MessageParcel& reply)
{
    RdbSyncerParam param;
    if (!ITypesUtil::Unmarshal(data, param)) {
        ZLOGE("Unmarshal bundleName_:%{public}s storeName_:%{public}s", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }

    auto status = UnregisterAutoSyncCallback(param, nullptr);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteNotifyDataChange(MessageParcel &data, MessageParcel &reply)
{
    RdbSyncerParam param;
    RdbChangedData rdbChangedData;
    if (!ITypesUtil::Unmarshal(data, param, rdbChangedData)) {
        ZLOGE("Unmarshal bundleName_:%{public}s storeName_:%{public}s ", param.bundleName_.c_str(),
              Anonymous::Change(param.storeName_).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }

    auto status = NotifyDataChange(param, rdbChangedData);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return RDB_OK;
}

int32_t RdbServiceStub::OnRemoteQuerySharingResource(MessageParcel& data, MessageParcel& reply)
{
    RdbSyncerParam param;
    PredicatesMemo predicates;
    std::vector<std::string> columns;
    if (!ITypesUtil::Unmarshal(data, param, predicates, columns)) {
        ZLOGE("Unmarshal bundleName_:%{public}s storeName_:%{public}s", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }

    auto [status, resultSet] = QuerySharingResource(param, predicates, columns);
    sptr<RdbResultSetStub> object = new RdbResultSetStub(resultSet);
    if (!ITypesUtil::Marshal(reply, status, object->AsObject())) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return RDB_OK;
}

int32_t RdbServiceStub::OnDisable(MessageParcel& data, MessageParcel& reply)
{
    RdbSyncerParam param;
    if (!ITypesUtil::Unmarshal(data, param)) {
        ZLOGE("Unmarshal bundleName_:%{public}s storeName_:%{public}s", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }

    auto status = Disable(param);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return RDB_OK;
}

int32_t RdbServiceStub::OnEnable(MessageParcel& data, MessageParcel& reply)
{
    RdbSyncerParam param;
    if (!ITypesUtil::Unmarshal(data, param)) {
        ZLOGE("Unmarshal bundleName_:%{public}s storeName_:%{public}s", param.bundleName_.c_str(),
            Anonymous::Change(param.storeName_).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }

    auto status = Enable(param);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return RDB_OK;
}
} // namespace OHOS::DistributedRdb
