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
#include "log_print.h"
#include "sqlite_utils.h"
#include "sqlite_store_executor_impl.h"

namespace DocumentDB {
SqliteStoreExecutor::SqliteStoreExecutor(sqlite3 *handle) : dbHandle_(handle)
{
}

SqliteStoreExecutor::~SqliteStoreExecutor()
{
    sqlite3_close_v2(dbHandle_);
    dbHandle_ = nullptr;
}

int SqliteStoreExecutor::PutData(const std::string &collName, const Key &key, const Value &value)
{
    if (dbHandle_ == nullptr) {
        return -E_ERROR;
    }

    std::string sql = "INSERT OR REPLACE INTO '" + collName + "' VALUES (?,?);";
    int errCode = SQLiteUtils::ExecSql(dbHandle_, sql, [key, value](sqlite3_stmt *stmt) {
        SQLiteUtils::BindBlobToStatement(stmt, 1, key);
        SQLiteUtils::BindBlobToStatement(stmt, 2, value);
        return E_OK;
    }, nullptr);
    if (errCode != SQLITE_OK) {
        GLOGE("[sqlite executor] create collectoin failed. err=%d", errCode);
        return errCode;
    }
    return E_OK;
}

int SqliteStoreExecutor::GetData(const std::string &collName, const Key &key, Value &value) const
{
    if (dbHandle_ == nullptr) {
        return -E_ERROR;
    }

    std::string sql = "SELECT value FROM '" + collName + "' WHERE key=?;";
    int errCode = SQLiteUtils::ExecSql(dbHandle_, sql, [key](sqlite3_stmt *stmt) {
        SQLiteUtils::BindBlobToStatement(stmt, 1, key);
        return E_OK;
    }, [&value](sqlite3_stmt *stmt) {
        SQLiteUtils::GetColumnBlobValue(stmt, 0, value);
        return E_OK;
    });
    if (errCode != SQLITE_OK) {
        GLOGE("[sqlite executor] create collectoin failed. err=%d", errCode);
        return errCode;
    }
    return E_OK;
}

int SqliteStoreExecutor::CreateCollection(const std::string &name, int flag)
{
    if (dbHandle_ == nullptr) {
        return -E_ERROR;
    }

    std::string sql = "CREATE TABLE IF NOT EXISTS '" + name + "' (key BLOB PRIMARY KEY, value BLOB);";
    int errCode = SQLiteUtils::ExecSql(dbHandle_, sql);
    if (errCode != SQLITE_OK) {
        GLOGE("[sqlite executor] create collectoin failed. err=%d", errCode);
        return errCode;
    }
    return E_OK;
}

int SqliteStoreExecutor::DropCollection(const std::string &name, int flag)
{
    if (dbHandle_ == nullptr) {
        return -E_ERROR;
    }

    std::string sql = "DROP TABLE IF EXISTS '" + name + "';";
    int errCode = SQLiteUtils::ExecSql(dbHandle_, sql);
    if (errCode != SQLITE_OK) {
        GLOGE("[sqlite executor] drop collectoin failed. err=%d", errCode);
        return errCode;
    }
    return E_OK;
}
} // DocumentDB