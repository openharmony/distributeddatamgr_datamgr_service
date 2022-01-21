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
#include "sqlite_relational_store_connection.h"
#include "db_errno.h"
#include "log_print.h"

namespace DistributedDB {
SQLiteRelationalStoreConnection::SQLiteRelationalStoreConnection(SQLiteRelationalStore *store)
    : RelationalStoreConnection(store) {}
// Close and release the connection.
int SQLiteRelationalStoreConnection::Close()
{
    if (store_ == nullptr) {
        return -E_INVALID_CONNECTION;
    }

    if (isExclusive_.load()) {
        return -E_BUSY;
    }

    // check if transaction closed
    {
        std::lock_guard<std::mutex> transactionLock(transactionMutex_);
        if (writeHandle_ != nullptr) {
            LOGW("Transaction started, need to rollback before close.");
            int errCode = RollBack();
            if (errCode != E_OK) {
                LOGE("Rollback transaction failed, %d.", errCode);
            }
            ReleaseExecutor(writeHandle_);
        }
    }

    static_cast<SQLiteRelationalStore *>(store_)->ReleaseDBConnection(this);
    return E_OK;
}

std::string SQLiteRelationalStoreConnection::GetIdentifier()
{
    return std::string();
}

SQLiteSingleVerRelationalStorageExecutor *SQLiteRelationalStoreConnection::GetExecutor(bool isWrite, int &errCode) const
{
    auto *store = GetDB<SQLiteRelationalStore>();
    if (store == nullptr) {
        errCode = -E_NOT_INIT;
        LOGE("[RelationalConnection] store is null, get executor failed! errCode = [%d]", errCode);
        return nullptr;
    }

    return store->GetHandle(isWrite, errCode);
}

void SQLiteRelationalStoreConnection::ReleaseExecutor(SQLiteSingleVerRelationalStorageExecutor *&executor) const
{
    auto *store = GetDB<SQLiteRelationalStore>();
    if (store != nullptr) {
        store->ReleaseHandle(executor);
    }
}

int SQLiteRelationalStoreConnection::StartTransaction()
{
    std::lock_guard<std::mutex> lock(transactionMutex_);
    if (writeHandle_ != nullptr) {
        LOGD("Transaction started already.");
        return -E_TRANSACT_STATE;
    }

    int errCode = E_OK;
    auto *handle = GetExecutor(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->StartTransaction(TransactType::DEFERRED);
    if (errCode != E_OK) {
        ReleaseExecutor(handle);
        return errCode;
    }

    LOGD("[RelationalConnection] Start transaction finish.");
    writeHandle_ = handle;
    transactingFlag_.store(true);
    return E_OK;
}

// Commit the transaction
int SQLiteRelationalStoreConnection::Commit()
{
    std::lock_guard<std::mutex> lock(transactionMutex_);
    if (writeHandle_ == nullptr) {
        LOGE("single version database is null or the transaction has not been started");
        return -E_INVALID_DB;
    }

    int errCode = writeHandle_->Commit();
    ReleaseExecutor(writeHandle_);
    if (errCode == E_OK) {
        transactingFlag_.store(false);
    }
    LOGD("connection commit transaction!");
    return errCode;
}

// Roll back the transaction
int SQLiteRelationalStoreConnection::RollBack()
{
    std::lock_guard<std::mutex> lock(transactionMutex_);
    if (writeHandle_ == nullptr) {
        LOGE("Invalid handle for rollback or the transaction has not been started.");
        return -E_INVALID_DB;
    }

    int errCode = writeHandle_->Rollback();
    ReleaseExecutor(writeHandle_);
    if (errCode == E_OK) {
        transactingFlag_.store(false);
    }
    LOGI("connection rollback transaction!");
    return errCode;
}

int SQLiteRelationalStoreConnection::CreateDistributedTable(const std::string &tableName)
{
    auto *store = GetDB<SQLiteRelationalStore>();
    if (store == nullptr) {
        LOGE("[RelationalConnection] store is null, get DB failed!");
        return -E_INVALID_CONNECTION;
    }

    int errCode = store->CreateDistributedTable(tableName);
    if (errCode != E_OK) {
        LOGE("[RelationalConnection] crete distributed table failed. %d", errCode);
    }
    return errCode;
}

int SQLiteRelationalStoreConnection::RemoveDeviceData(const std::string &device)
{
    return RemoveDeviceData(device, {});
}

int SQLiteRelationalStoreConnection::RemoveDeviceData(const std::string &device, const std::string &tableName)
{
    auto *store = GetDB<SQLiteRelationalStore>();
    if (store == nullptr) {
        LOGE("[RelationalConnection] store is null, get DB failed!");
        return -E_INVALID_CONNECTION;
    }

    int errCode = store->RemoveDeviceData(device, tableName);
    if (errCode != E_OK) {
        LOGE("[RelationalConnection] remove device data failed. %d", errCode);
    }
    return errCode;
}

int SQLiteRelationalStoreConnection::Pragma(int cmd, void *parameter) // reserve for interface function fix
{
    return E_OK;
}

int SQLiteRelationalStoreConnection::TriggerAutoSync()
{
    return E_OK;
}

int SQLiteRelationalStoreConnection::SyncToDevice(SyncInfo &info)
{
    auto *store = GetDB<SQLiteRelationalStore>();
    if (store == nullptr) {
        LOGE("[RelationalConnection] store is null, get executor failed!");
        return -E_INVALID_CONNECTION;
    }

    {
        AutoLock lockGuard(this);
        IncObjRef(this);
        ISyncer::SyncParma syncParam;
        syncParam.devices = info.devices;
        syncParam.mode = info.mode;
        syncParam.wait = info.wait;
        syncParam.isQuerySync = true;
        syncParam.relationOnComplete = info.onComplete;
        syncParam.syncQuery = QuerySyncObject(info.query);
        syncParam.onFinalize =  [this]() { DecObjRef(this); };

        int errCode = store->Sync(syncParam);
        if (errCode != E_OK) {
            DecObjRef(this);
            return errCode;
        }
    }
    return E_OK;
}

int SQLiteRelationalStoreConnection::RegisterLifeCycleCallback(const DatabaseLifeCycleNotifier &notifier)
{
    auto *store = GetDB<SQLiteRelationalStore>();
    if (store == nullptr) {
        LOGE("[RelationalConnection] store is null, get executor failed!");
        return -E_INVALID_CONNECTION;
    }

    return store->RegisterLifeCycleCallback(notifier);
}
}
#endif