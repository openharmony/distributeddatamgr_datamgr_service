/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifdef RELATIONAL_STORE
#include "relational_store_delegate_impl.h"

#include "db_errno.h"
#include "kv_store_errno.h"
#include "log_print.h"
#include "param_check_utils.h"
#include "sync_operation.h"

namespace DistributedDB {
RelationalStoreDelegateImpl::RelationalStoreDelegateImpl(RelationalStoreConnection *conn, const std::string &path)
    : conn_(conn),
      storePath_(path)
{}

RelationalStoreDelegateImpl::~RelationalStoreDelegateImpl()
{
    if (!releaseFlag_) {
        LOGF("[KvStoreNbDelegate] Can't release directly");
        return;
    }

    conn_ = nullptr;
};

DBStatus RelationalStoreDelegateImpl::RemoveDeviceData(const std::string &device)
{
    return RemoveDeviceData(device, {});
}

DBStatus RelationalStoreDelegateImpl::CreateDistributedTable(const std::string &tableName)
{
    if (!ParamCheckUtils::CheckRelationalTableName(tableName)) {
        LOGE("invalid table name.");
        return INVALID_ARGS;
    }

    if (conn_ == nullptr) {
        LOGE("[RelationalStore Delegate] Invalid connection for operation!");
        return DB_ERROR;
    }

    int errCode = conn_->CreateDistributedTable(tableName);
    if (errCode != E_OK) {
        LOGW("[RelationalStore Delegate] Create Distributed table failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::Sync(const std::vector<std::string> &devices, SyncMode mode,
    const Query &query, const SyncStatusCallback &onComplete, bool wait)
{
    if (conn_ == nullptr) {
        LOGE("Invalid connection for operation!");
        return DB_ERROR;
    }

    RelationalStoreConnection::SyncInfo syncInfo{devices, mode,
        std::bind(&RelationalStoreDelegateImpl::OnSyncComplete, std::placeholders::_1, onComplete), query, wait};
    int errCode = conn_->SyncToDevice(syncInfo);
    if (errCode != E_OK) {
        LOGW("[RelationalStore Delegate] sync data to device failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::RemoveDeviceData(const std::string &device, const std::string &tableName)
{
    if (conn_ == nullptr) {
        LOGE("Invalid connection for operation!");
        return DB_ERROR;
    }

    if (device.empty()) {
        LOGE("[RelationalStore Delegate] Remove device data with unspecified device name.");
        return INVALID_ARGS;
    }

    int errCode = conn_->RemoveDeviceData(device, tableName);
    if (errCode != E_OK) {
        LOGW("[RelationalStore Delegate] remove device data failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus RelationalStoreDelegateImpl::Close()
{
    if (conn_ == nullptr) {
        return OK;
    }

    int errCode = conn_->Close();
    if (errCode == -E_BUSY) {
        LOGW("[KvStoreDelegate] busy for close");
        return BUSY;
    }
    if (errCode != E_OK) {
        LOGE("Release db connection error:%d", errCode);
        return TransferDBErrno(errCode);
    }

    LOGI("[KvStoreDelegate] Close");
    conn_ = nullptr;
    return OK;
}

void RelationalStoreDelegateImpl::SetReleaseFlag(bool flag)
{
    releaseFlag_ = flag;
}

void RelationalStoreDelegateImpl::OnSyncComplete(const std::map<std::string, std::vector<TableStatus>> &devicesStatus,
    SyncStatusCallback &onComplete)
{
    const auto &statusMap = SyncOperation::DBStatusTransMap();
    std::map<std::string, std::vector<TableStatus>> res;
    for (const auto &[device, tablesStatus] : devicesStatus) {
        for (const auto &tableStatus : tablesStatus) {
            TableStatus table;
            table.tableName = tableStatus.tableName;
            DBStatus status = DB_ERROR;
            auto iterator = statusMap.find(tableStatus.status);
            if (iterator != statusMap.end()) {
                status = iterator->second;
            }
            table.status = status;
            res[device].push_back(table);
        }
    }
    if (onComplete) {
        onComplete(res);
    }
}
} // namespace DistributedDB
#endif