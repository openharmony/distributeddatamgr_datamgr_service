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

#include "doc_errno.h"
#include "kv_store_manager.h"
#include "sqlite_store_executor_impl.h"

namespace DocumentDB {
constexpr const char *APP_ID = "APP_ID";
constexpr const char *USER_ID = "USER_ID";
constexpr const char *STORE_ID = "STORE_ID";


sqlite3 *CreateDataBase(const std::string &dbUri)
{
    sqlite3 *db = nullptr;
    if (int r = sqlite3_open_v2(dbUri.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        if (db != nullptr) {
            (void)sqlite3_close_v2(db);
            db = nullptr;
        }
    }
    return db;
}

int KvStoreManager::GetKvStore(const std::string &path, KvStoreExecutor *&executor)
{
    sqlite3 *db = CreateDataBase(path);
    if (db == nullptr) {
        return -E_ERROR;
    }

    executor = new (std::nothrow) SqliteStoreExecutor(db);
    return E_OK;
}
}