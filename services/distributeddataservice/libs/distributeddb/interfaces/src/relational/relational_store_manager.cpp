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
#include "relational_store_manager.h"

#include <thread>

#include "relational_store_instance.h"
#include "db_common.h"
#include "param_check_utils.h"
#include "log_print.h"
#include "db_errno.h"
#include "kv_store_errno.h"
#include "relational_store_delegate_impl.h"
#include "platform_specific.h"

namespace DistributedDB {
RelationalStoreManager::RelationalStoreManager(const std::string &appId, const std::string &userId)
    : appId_(appId),
      userId_(userId)
{}

static void InitStoreProp(const RelationalStoreDelegate::Option &option, const std::string &storePath,
    const std::string &appId, const std::string &userId, RelationalDBProperties &properties)
{
    properties.SetBoolProp(RelationalDBProperties::CREATE_IF_NECESSARY, option.createIfNecessary);
    properties.SetStringProp(RelationalDBProperties::DATA_DIR, storePath);
    properties.SetStringProp(RelationalDBProperties::APP_ID, appId);
    properties.SetStringProp(RelationalDBProperties::USER_ID, userId);
    properties.SetStringProp(RelationalDBProperties::STORE_ID, storePath); // same as dir
    std::string identifier = userId + "-" + appId + "-" + storePath;
    std::string hashIdentifier = DBCommon::TransferHashString(identifier);
    properties.SetStringProp(RelationalDBProperties::IDENTIFIER_DATA, hashIdentifier);
}

static const int GET_CONNECT_RETRY = 3;
static const int RETRY_GET_CONN_INTER = 30;

static RelationalStoreConnection *GetOneConnectionWithRetry(const RelationalDBProperties &properties, int &errCode)
{
    for (int i = 0; i < GET_CONNECT_RETRY; i++) {
        auto conn = RelationalStoreInstance::GetDatabaseConnection(properties, errCode);
        if (conn != nullptr) {
            return conn;
        }
        if (errCode == -E_STALE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_GET_CONN_INTER));
        } else {
            return nullptr;
        }
    }
    return nullptr;
}

void RelationalStoreManager::OpenStore(const std::string &path, const RelationalStoreDelegate::Option &option,
    const std::function<void(DBStatus, RelationalStoreDelegate *)> &callback)
{
    if (!callback) {
        LOGE("[KvStoreMgr] Invalid callback for kv store!");
        return;
    }

    if (!ParamCheckUtils::CheckStoreParameter("Relational_default_id", appId_, userId_) || path.empty()) {
        callback(INVALID_ARGS, nullptr);
        return;
    }

    RelationalDBProperties properties;
    InitStoreProp(option, path, appId_, userId_, properties);

    int errCode = E_OK;
    auto *conn = GetOneConnectionWithRetry(properties, errCode);
    DBStatus status = TransferDBErrno(errCode);
    if (conn == nullptr) {
        callback(status, nullptr);
        return;
    }

    auto store = new (std::nothrow) RelationalStoreDelegateImpl(conn, path);
    if (store == nullptr) {
        conn->Close();
        callback(DB_ERROR, nullptr);
        return;
    }

    (void)conn->TriggerAutoSync();
    callback(OK, store);
}

DBStatus RelationalStoreManager::CloseStore(RelationalStoreDelegate *store)
{
    if (store == nullptr) {
        return INVALID_ARGS;
    }

    auto storeImpl = static_cast<RelationalStoreDelegateImpl *>(store);
    DBStatus status = storeImpl->Close();
    if (status == BUSY) {
        LOGD("NbDelegateImpl is busy now.");
        return BUSY;
    }
    storeImpl->SetReleaseFlag(true);
    delete store;
    store = nullptr;
    return OK;
}

static int RemoveFile(const std::string &fileName)
{
    if (!OS::CheckPathExistence(fileName)) {
        return E_OK;
    }

    if (OS::RemoveFile(fileName.c_str()) != E_OK) {
        LOGE("Remove file failed:%d", errno);
        return -E_REMOVE_FILE;
    }
    return E_OK;
}

static int RemoveDB(const std::string &path)
{
    if (RemoveFile(path) != E_OK) {
        LOGE("Remove the db file failed:%d", errno);
        return -E_REMOVE_FILE;
    }

    std::string dbFile = path + "-wal";
    if (RemoveFile(dbFile) != E_OK) {
        LOGE("Remove the wal file failed:%d", errno);
        return -E_REMOVE_FILE;
    }

    dbFile = path + "-shm";
    if (RemoveFile(dbFile) != E_OK) {
        LOGE("Remove the shm file failed:%d", errno);
        return -E_REMOVE_FILE;
    }
    return E_OK;
}

DBStatus RelationalStoreManager::DeleteStore(const std::string &path)
{
    if (path.empty()) {
        LOGE("Invalid store info for deleting");
        return INVALID_ARGS;
    }

    std::string identifier = userId_ + "-" + appId_ + "-" + path;
    std::string hashIdentifier = DBCommon::TransferHashString(identifier);

    auto *manager = RelationalStoreInstance::GetInstance();
    int errCode  = manager->CheckDatabaseFileStatus(hashIdentifier);
    if (errCode != E_OK) {
        LOGE("The store is busy!");
        return BUSY;
    }

    // RemoveDB
    errCode = RemoveDB(path);
    if (errCode == E_OK) {
        LOGI("Database deleted successfully!");
        return OK;
    }
    LOGE("Delete the kv store error:%d", errCode);
    return TransferDBErrno(errCode);
}
} // namespace DistributedDB
#endif