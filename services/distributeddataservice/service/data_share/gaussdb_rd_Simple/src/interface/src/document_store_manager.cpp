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
#include "doc_errno.h"
#include "grd_base/grd_type_export.h"
#include "kv_store_manager.h"
#include "log_print.h"
#include "os_api.h"

namespace DocumentDB {
int DocumentStoreManager::GetDocumentStore(const std::string &path, DocumentStore *&store)
{
    KvStoreExecutor *executor = nullptr;
    KvStoreManager::GetKvStore(path, executor);
    store = new (std::nothrow) DocumentStore(executor);
    return E_OK;
}

int DocumentStoreManager::CloseDocumentStore(DocumentStore *store, CloseType type)
{
    if (type == CloseType::NORMAL) {
        // TODO: check result set
    }

    delete store;
    return E_OK;
}

bool DocumentStoreManager::CheckDBPath(const std::string &path, std::string &canonicalPath)
{
    if (path.empty()) {
        GLOGE("invalid path empty");
        return -E_INVALID_ARGS;
    }

    if (path.back() == '/') {
        GLOGE("invalid path end with slash");
        return -E_INVALID_ARGS;
    }

    std::string canonicalDir;
    int errCode = OSAPI::GetRealPath(path, canonicalDir);
    if (errCode == E_OK) {
        GLOGE("Get real path failed. %d", errCode);
    }
    return errCode;
}
} // DocumentDB