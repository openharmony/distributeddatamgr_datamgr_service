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
#include "document_store_manager.h"
#include "doc_errno.h"
#include "grd_base/grd_type_export.h"
#include "kv_store_manager.h"
#include "log_print.h"
#include "os_api.h"

namespace DocumentDB {
int DocumentStoreManager::GetDocumentStore(const std::string &path, const std::string &config, DocumentStore *&store)
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

    KvStoreExecutor *executor = nullptr;
    errCode = KvStoreManager::GetKvStore(canonicalPath + "/" + dbName, dbConfig, executor);
    if (errCode != E_OK) {
        GLOGE("Open document store failed. %d", errCode);
        return errCode;
    }

    store = new (std::nothrow) DocumentStore(executor);
    if (store == nullptr) {
        return -E_OUT_OF_MEMORY;
    }

    return errCode;
}

int DocumentStoreManager::CloseDocumentStore(DocumentStore *store, CloseType type)
{
    if (type == CloseType::NORMAL) {
        // TODO: check result set
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

    GLOGD("----> path: %s, dirPath: %s, dbName: %s, canonicalPath: %s", path.c_str(), dirPath.c_str(), dbName.c_str(),
        canonicalPath.c_str());
    return true;
}

bool DocumentStoreManager::CheckDBConfig(const std::string &config, int &errCode)
{
    return true;
}
} // DocumentDB