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
#ifndef SQLITE_SINGLE_VER_RELATIONAL_CONTINUE_TOKEN_H
#define SQLITE_SINGLE_VER_RELATIONAL_CONTINUE_TOKEN_H
#ifdef RELATIONAL_STORE
#include <map>
#include <string>

#include "db_types.h"
#include "query_object.h"

namespace DistributedDB {
class SQLiteSingleVerRelationalContinueToken {
public:
    SQLiteSingleVerRelationalContinueToken(const SyncTimeRange &timeRange, const QueryObject &queryObject);
    ~SQLiteSingleVerRelationalContinueToken() = default;

    // Check the magic number at the beginning and end of the SQLiteSingleVerRelationalContinueToken.
    bool CheckValid() const;
    // The statement should be release by the caller.
    int GetStatement(sqlite3 *db, sqlite3_stmt *&queryStmt, sqlite3_stmt *&fullStmt, bool &isGettingDeletedData);
    void SetNextBeginTime(const DataItem &theLastItem);
    void FinishGetData();
    bool IsGetAllDataFinished() const;
    const QueryObject &GetQuery() const;

private:
    std::string GetDeletedDataSQL() const;
    int GetQuerySyncStatement(sqlite3 *db, sqlite3_stmt *&stmt);
    int GetDeletedDataStmt(sqlite3 *db, sqlite3_stmt *&stmt) const;
    int GetFullStatement(sqlite3 *db, sqlite3_stmt *&stmt);

    static const unsigned int MAGIC_BEGIN = 0x600D0AC7;  // for token guard
    static const unsigned int MAGIC_END = 0x0AC7600D;    // for token guard
    unsigned int magicBegin_ = MAGIC_BEGIN;
    int isGettingDeletedData_ = false;
    QueryObject queryObj_;
    const std::string &tableName_;
    SyncTimeRange timeRange_;
    unsigned int magicEnd_ = MAGIC_END;
};
}  // namespace DistributedDB
#endif  // RELATIONAL_STORE
#endif  // SQLITE_SINGLE_VER_RELATIONAL_CONTINUE_TOKEN_H