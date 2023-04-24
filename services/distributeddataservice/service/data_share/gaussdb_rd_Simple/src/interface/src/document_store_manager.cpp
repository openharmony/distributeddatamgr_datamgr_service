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

#include "db_config.h"
#include "doc_errno.h"
#include "grd_base/grd_type_export.h"
#include "document_store_manager.h"
#include "log_print.h"
#include "os_api.h"
#include "kv_store_manager.h"

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

bool CheckDBCreate(unsigned int flags, const std::string &path)
{
    if ((flags & GRD_DB_OPEN_CREATE) == 0 && !OSAPI::CheckPathExistence(path)) {
        return false;
    }
    return true;
}
}

int DocumentStoreManager::GetDocumentStore(const std::string &path, const std::string &config, unsigned int flags,
    DocumentStore *&store)
{
    std::string canonicalPath;
    std::string dbName;
    int errCode = E_OK;
    if (!CheckDBPath(path, canonicalPath, dbName, errCode)) {
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

    KvStoreExecutor *executor = nullptr;
    errCode = KvStoreManager::GetKvStore(canonicalPath + "/" + dbName, dbConfig, executor);
    if (errCode != E_OK) {
        GLOGE("Open document store failed. %d", errCode);
        return errCode;
    }

    store = new (std::nothrow) DocumentStore(executor);
    if (store == nullptr) {
        GLOGE("Memory allocation failed!");
        return -E_FAILED_MEMORY_ALLOCATE;
    }
    if (store == nullptr) {
        return -E_OUT_OF_MEMORY;
    }

    return errCode;
}

int DocumentStoreManager::CloseDocumentStore(DocumentStore *store, unsigned int flags)
{
    if (!CheckDBCloseFlag(flags)) {
        GLOGE("Check document db close flags failed.");
        return -E_INVALID_ARGS;
    }

    delete store;
    return E_OK;
}

bool DocumentStoreManager::CheckDBPath(const std::string &path, std::string &canonicalPath, std::string &dbName,
    int &errCode)
{
    if (path.empty()) {
        GLOGE("Invalid path empty");
        errCode = -E_INVALID_ARGS;
        return false;
    }

    if (path.back() == '/') {
        GLOGE("Invalid path end with slash");
        errCode = -E_INVALID_ARGS;
        return false;
    }

    std::string dirPath;
    OSAPI::SplitFilePath(path, dirPath, dbName);

    int innerErrCode = OSAPI::GetRealPath(dirPath, canonicalPath);
    if (innerErrCode != E_OK) {
        GLOGE("Get real path failed. %d", errCode);
        errCode = -E_FILE_OPERATION;
        return false;
    }

    if (!OSAPI::CheckPermission(canonicalPath)) {
        GLOGE("Check path permission failed. %d", errCode);
        errCode = -E_FILE_OPERATION;
        return false;
    }

    return true;
}

bool DocumentStoreManager::CheckDBConfig(const std::string &config, int &errCode)
{
    return true;
}
} // DocumentDB