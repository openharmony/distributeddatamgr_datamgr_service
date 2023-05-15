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

#include "doc_errno.h"
#include "document_store_manager.h"
#include "grd_base/grd_error.h"
#include "grd_type_inner.h"
#include "log_print.h"

using namespace DocumentDB;

GRD_API int32_t GRD_DBOpen(const char *dbPath, const char *configStr, uint32_t flags, GRD_DB **db)
{
    if (db == nullptr) {
        return GRD_INVALID_ARGS;
    }
    std::string path = (dbPath == nullptr ? "" : dbPath);
    std::string config = (configStr == nullptr ? "" : configStr);
    DocumentStore *store = nullptr;
    int ret = DocumentStoreManager::GetDocumentStore(path, config, flags, store);
    if (ret != E_OK || store == nullptr) {
        return TransferDocErr(ret);
    }

    *db = new (std::nothrow) GRD_DB();
    if (*db == nullptr) {
        (void)DocumentStoreManager::CloseDocumentStore(store, GRD_DB_CLOSE_IGNORE_ERROR);
        store = nullptr;
        return GRD_FAILED_MEMORY_ALLOCATE;
    }

    (*db)->store_ = store;
    return TransferDocErr(ret);
}

GRD_API int32_t GRD_DBClose(GRD_DB *db, uint32_t flags)
{
    if (db == nullptr || db->store_ == nullptr) {
        return GRD_INVALID_ARGS;
    }

    int ret = DocumentStoreManager::CloseDocumentStore(db->store_, flags);
    if (ret != E_OK) {
        return TransferDocErr(ret);
    }

    db->store_ = nullptr;
    delete db;
    return GRD_OK;
}

GRD_API int32_t GRD_Flush(GRD_DB *db, uint32_t flags)
{
    if (db == nullptr || db->store_ == nullptr) {
        return GRD_INVALID_ARGS;
    }
    if (flags != GRD_DB_FLUSH_ASYNC && flags != GRD_DB_FLUSH_SYNC) {
        return GRD_INVALID_ARGS;
    }
    return GRD_OK;
}