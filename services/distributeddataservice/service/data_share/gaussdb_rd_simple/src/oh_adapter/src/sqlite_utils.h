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

#ifndef SQLITE_UTILS_H
#define SQLITE_UTILS_H

#include <functional>
#include <string>
#include <vector>

#include "sqlite3sym.h"

namespace DocumentDB {
enum class TransactType
{
    DEFERRED,
    IMMEDIATE,
};

class SQLiteUtils {
public:
    static int CreateDataBase(const std::string &path, int flag, sqlite3 *&db);

    static int GetStatement(sqlite3 *db, const std::string &sql, sqlite3_stmt *&statement);
    static int StepWithRetry(sqlite3_stmt *statement);
    static int ResetStatement(sqlite3_stmt *&statement, bool finalize);

    static int BindBlobToStatement(sqlite3_stmt *statement, int index, const std::vector<uint8_t> &value);
    static int GetColumnBlobValue(sqlite3_stmt *statement, int index, std::vector<uint8_t> &value);

    static int BindTextToStatement(sqlite3_stmt *statement, int index, const std::string &value);
    static int GetColumnTextValue(sqlite3_stmt *statement, int index, std::string &value);

    static int BeginTransaction(sqlite3 *db, TransactType type = TransactType::DEFERRED);
    static int CommitTransaction(sqlite3 *db);
    static int RollbackTransaction(sqlite3 *db);

    static int ExecSql(sqlite3 *db, const std::string &sql);
    static int ExecSql(sqlite3 *db, const std::string &sql, const std::function<int(sqlite3_stmt *)> &bindCallback,
        const std::function<int(sqlite3_stmt *)> &resultCallback);

private:
    static void SqliteLogCallback(void *data, int err, const char *msg);
};
} // namespace DocumentDB
#endif // SQLITE_UTILS_H