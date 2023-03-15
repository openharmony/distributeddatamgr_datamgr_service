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

#include "grd_base/grd_db_api.h"
#include "grd_base/grd_error.h"
#include "doc_errno.h"
#include "document_store_manager.h"
#include "document_store.h"

using namespace DocumentDB;

typedef struct GRD_DB {
    DocumentStore *store_ = nullptr;
} GRD_DB;

int GRD_DBOpen(const char *dbPath, const char *configStr, unsigned int flags, GRD_DB **db)
{
    std::string path = dbPath;
    DocumentStore *store = nullptr;
    DocumentStoreManager::GetDocumentStore(path, store);
    *db = new (std::nothrow) GRD_DB();
    (*db)->store_ = store;
    return GRD_OK;
}

int GRD_DBClose(GRD_DB *db, unsigned int flags)
{
    if (db == nullptr) {
        return GRD_INVALID_ARGS;
    }

    DocumentStoreManager::CloseType closeType = (flags == GRD_DB_CLOSE) ? DocumentStoreManager::CloseType::NORMAL :
        DocumentStoreManager::CloseType::IGNORE_ERROR;
    int status = DocumentStoreManager::CloseDocumentStore(db->store_, closeType);
    if (status != E_OK) {
        return GRD_RESOURCE_BUSY;
    }

    delete db;
    return GRD_OK;
}
