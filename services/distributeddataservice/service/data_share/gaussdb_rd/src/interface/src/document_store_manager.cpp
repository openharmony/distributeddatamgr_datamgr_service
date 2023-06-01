/*
* Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "document_store_manager.h"

#include "db_config.h"
#include "doc_errno.h"
#include "grd_base/grd_type_export.h"
#include "kv_store_manager.h"
#include "log_print.h"
#include "os_api.h"

namespace DocumentDB {
namespace {
bool CheckDBOpenFlag(unsigned int flag)
{
    unsigned int mask = ~(GRD_DB_OPEN_CREATE | GRD_DB_OPEN_CHECK_FOR_ABNORMAL | GRD_DB_OPEN_CHECK);
    unsigned int invalidOpt = (GRD_DB_OPEN_CHECK_FOR_ABNORMAL | GRD_DB_OPEN_CHECK);
    return ((flag & mask) == 0x00) && ((flag & invalidOpt) != invalidOpt);
}

bool CheckDBCloseFlag(unsigned int flag)
{
    return (flag == GRD_DB_CLOSE) || (flag == GRD_DB_CLOSE_IGNORE_ERROR);
}

bool CheckDBCreate(uint32_t flags, const std::string &path)
{
    if ((flags & GRD_DB_OPEN_CREATE) == 0 && !OSAPI::CheckPathExistence(path)) {
        return false;
    }
    return true;
}
} // namespace

std::mutex DocumentStoreManager::openCloseMutex_;
std::map<std::string, int> DocumentStoreManager::dbConnCount_;

int DocumentStoreManager::GetDocumentStore(const std::string &path, const std::string &config, uint32_t flags,
    DocumentStore *&store)
{
    std::string canonicalPath;
    std::string dbName;
    int errCode = CheckDBPath(path, canonicalPath, dbName);
    if (errCode != E_OK) {
        GLOGE("Check document db file path failed.");
        return errCode;
    }

    DBConfig dbConfig = DBConfig::ReadConfig(config, errCode);
    if (errCode != E_OK) {
        GLOGE("Read db config str failed. %d", errCode);
        return errCode;
    }

    if (!CheckDBOpenFlag(flags)) {
        GLOGE("Check document db open flags failed.");
        return -E_INVALID_ARGS;
    }
    if (!CheckDBCreate(flags, path)) {
        GLOGE("Open db failed, file no exists.");
        return -E_INVALID_ARGS;
    }

    std::lock_guard<std::mutex> lock(openCloseMutex_);

    std::string dbRealPath = canonicalPath + "/" + dbName;
    auto it = dbConnCount_.find(dbRealPath);
    bool isFirstOpen = (it == dbConnCount_.end() || it->second == 0);

    KvStoreExecutor *executor = nullptr;
    errCode = KvStoreManager::GetKvStore(dbRealPath, dbConfig, isFirstOpen, executor);
    if (errCode != E_OK) {
        GLOGE("Open document store failed. %d", errCode);
        return errCode;
    }

    store = new (std::nothrow) DocumentStore(executor);
    if (store == nullptr) {
        delete executor;
        GLOGE("Memory allocation failed!");
        return -E_FAILED_MEMORY_ALLOCATE;
    }

    store->OnClose([dbRealPath]() {
        dbConnCount_[dbRealPath]--;
    });

    if (isFirstOpen) {
        dbConnCount_[dbRealPath] = 1;
    } else {
        dbConnCount_[dbRealPath]++;
    }
    return errCode;
}

int DocumentStoreManager::CloseDocumentStore(DocumentStore *store, uint32_t flags)
{
    if (!CheckDBCloseFlag(flags)) {
        GLOGE("Check document db close flags failed.");
        return -E_INVALID_ARGS;
    }

    std::lock_guard<std::mutex> lock(openCloseMutex_);
    int errCode = store->Close(flags);
    if (errCode != E_OK) {
        GLOGE("Close document store failed. %d", errCode);
        return errCode;
    }

    delete store;
    return E_OK;
}

int DocumentStoreManager::CheckDBPath(const std::string &path, std::string &canonicalPath, std::string &dbName)
{
    if (path.empty()) {
        GLOGE("Invalid path empty");
        return -E_INVALID_ARGS;
    }

    if (path.back() == '/') {
        GLOGE("Invalid path end with slash");
        return -E_FAILED_FILE_OPERATION;
    }

    std::string dirPath;
    OSAPI::SplitFilePath(path, dirPath, dbName);

    int errCode = OSAPI::GetRealPath(dirPath, canonicalPath);
    if (errCode != E_OK) {
        GLOGE("Get real path failed. %d", errCode);
        return -E_FILE_OPERATION;
    }

    if (!OSAPI::CheckPermission(canonicalPath)) {
        GLOGE("Check path permission failed.");
        return -E_FILE_OPERATION;
    }

    return E_OK;
}
} // namespace DocumentDB