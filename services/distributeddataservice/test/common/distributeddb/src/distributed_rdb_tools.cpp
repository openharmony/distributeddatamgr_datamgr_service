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
#include "distributed_rdb_tools.h"
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <random>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#ifndef USE_SQLITE_SYMBOLS
#include "sqlite3.h"
#else
#include "sqlite3sym.h"
#endif
#include "distributeddb_data_generator.h"
#include "distributed_test_sysinfo.h"
#include "platform_specific.h"
#include "securec.h"

using namespace std;
using namespace DistributedDB;
using namespace DistributedDBDataGenerator;

namespace {
    const int MAX_DISTRIBUTED_TABLE_COUNT = 32;
    std::string  gtableNameList[MAX_DISTRIBUTED_TABLE_COUNT + 1];
    const char *SQL_CREATE_NORMAL_TABLE = "CREATE TABLE IF NOT EXISTS NORMAL_RDB("  \
        "id    INT    NOT NULL    PRIMARY KEY  AUTO_INCREMENT,"  \
        "name  VARCHAR(100)   NOT NULL   DEFAULT \"rdb\");";

    const char *SQL_CREATE_CONSTRAINT_TABLE = "CREATE TABLE IF NOT EXISTS CONSTRAINT_RDB("  \
        "id    INT    NOT NULL    PRIMARY KEY  AUTO_INCREMENT,"  \
        "f_id  INT," \
        "age   INT   NOT NULL   UNIQUE," \
        "name  VARCHAR(100)   NOT NULL   DEFAULT \"rdb\"," \
        "address  CHAR(50) COLLATE NOCASE," \
        "identity  INT   NOT NULL, CHECK(identity > 10000) PRIMARY KEY(id, identity)," \
        "FOREGIN key(f_id) references NORMAL_RDB(id));";

    const char *SQL_CREATE_TRIGGER = "CREATE TRIGGER IF NOT EXISTS insertTrigger " \
        "AFTER INSERT ON CONSTRAINT_RDB " \
        "FOR EACH ROW " \
        "BEGIN " \
        "update NORMAL_RDB set name = \"name_001\" " \
        "END";

    const char *SQL_INSERT_NORMAL_TABLE = "INSERT INTO NORMAL_RDB (id,name)" \
        "VALUES (1, \'rdb_001\'), (2, \'rdb_002\'), (3, \'rdb_003\'), (4, \'rdb_004\'), (5, \'rdb_005\');";


    const char *SQL_ADD_FIELD_TABLE1 = "ALTER TABLE RDB_1 ADD COLUMN add_id INI";

    const char *SQL_ADD_INDEX_TABLE2 = "CREATE INDEX name_index RDB_2 (name)";

    const char *SQL_DROP_TABLE3 = "DROP TABLE RDB_3";

    const char *SQL_DROP_CREATE_TABLE3 = "CREATE TABLE IF NOT EXISTS RDB_3(" \
        "id    INT    NOT NULL   PRIMARY KEY  AUTO_INCREMENT);";

    const char *SQL_DROP_TABLE4 = "DROP TABLE RDB_4";

    const char *SQL_DROP_CREATE_TABLE4  = "CREATE TABLE IF NOT EXISTS RDB_4(" \
        "id    INT    NOT NULL   PRIMARY KEY  AUTO_INCREMENT, name  CHAR(100));";

    const char *SQL_JOURNAL_MODE =  "PRAGMA journal_mode = DELETE;";

    const char *SQL_SYNCHRONOUS_MODE =  "PRAGMA synchronous = FULL;";

    const char *SQL_INSERT_RDB5_TABLE = "INSERT INTO RDB_5 (id,name,age)" \
        "VALUES (1, \'rdb_005\', \'name_rdb5\');";
}

DBStatus DistributedRdbTools::GetOpenStoreStatus(const RelatetionalStoreManager *&manager,
    RelatetionalStoreDelegate *&delegate, const RdbParameters &param)
{
    if (manager == nullptr) {
        MST_LOG("%s GetRdbStore failed! manager nullptr.", TAG.c_str());
        return DBStatus::DB_ERROR;
    }
    DBStatus status = manager->OpenStore(param.path, param.storeId, delegate);
    if (delegate == nullptr) {
        MST_LOG("%s GetRdbStore failed! delegate nullptr.", TAG.c_str());
    }
    return status;
}

DBStatus DistributedRdbTools::GetCreateDistributedTableStatus(const RelatetionalStoreDelegate *&delegate,
    const std::string &tableName)
{
    if (delegate == nullptr) {
        MST_LOG("%s CreateDistributedTable failed! delegate nullptr.", TAG.c_str());
        return DBStatus::DB_ERROR;
    }
    DBStatus status = delegate->CreateDistributedTable(tableName);
    return status;
}

DBStatus DistributedRdbTools::CloseStore(const RelatetionalStoreDelegate *&delegate)
{
    if (delegate == nullptr) {
        MST_LOG("%s CloseStore failed! delegate nullptr.", TAG.c_str());
        return DBStatus::DB_ERROR;
    }
    DBStatus status = delegate->CloseStore();
    return status;
}

bool DistributedRdbTools::InitSqlite3Store(sqlite3 *&db, const RdbParameters &param)
{
    const std::string dbName = param.path + param.storeId + ".db";
    DBStributedDB::OS::RemoveFile(dbName);
    int errCode = sqlite3_open(dbName, &db);
    if (errCode != SQLITE_OK) {
        MST_LOG("sqlite3_open Failed!");
        return false;
    }
    int errCode1 = sqlite3_exec(db, "PRAGMA journal_mode = WAL;", 0, 0, 0);
    if (errCode1 != SQLITE_OK) {
        MST_LOG("PRAGMA journal_mode = WAL Failed!");
        return false;
    }
    return true;
}

namespace {
void SqliteExecSql(sqlite3 *db, const char *sql)
{
    char *errMsg = nullptr;
    int errCode = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (errCode != SQLITE_OK && errMsg != nullptr) {
        MST_LOG("sqlite3_exec sql Failed(%s)", errMsg.c_str());
        return false;
    }
    sqlite3_free(errMsg);
    errMsg = nullptr;
}
}

bool DistributedRdbTools::InitTableDataAndTrigger(const sqlite3 *&db)
{
    if (db == nullptr) {
        MST_LOG("openStore Failed");
        return false;
    }
    SqliteExecSql(db, SQL_CREATE_NORMAL_TABLE);
    SqliteExecSql(db, SQL_CREATE_CONSTRAINT_TABLE);
    SqliteExecSql(db, SQL_CREATE_TRIGGER);
    SqliteExecSql(db, SQL_INSERT_NORMAL_TABLE);

    for (int i = 1; i <= MAX_DISTRIBUTED_TABLE_COUNT + 1; i++) {
        std::string str_0 = "RDB_" + std::to_string(i);
        std::string str_1 = "CREATE TABLE IF NOT EXISTS "
        std::string str_2 = "( id    INT    NOT NULL   PRIMARY KEY  AUTO_INCREMENT,"  \
            "name  VARCHAR(100)   NOT NULL, age VARCHAR(100)   NOT NULL);";
        std::string sql = str_1 + str_0 + str_2;

        SqliteExecSql(db, sql.c_str());
        gtableNameList[i-1] = str_0;
    }
    return true;
}

bool DistributedRdbTools::AlterTableAttributes(const sqlite3 *&db)
{
    if (db == nullptr) {
        MST_LOG("openStore Failed");
        return false;
    }
    char *errMsg = nullptr;
    int errCode = sqlite3_exec(db, SQL_ADD_FIELD_TABLE1, nullptr, nullptr, &errMsg);
    if (errCode != SQLITE_OK && errMsg != nullptr) {
        MST_LOG("sqlite3_exec SQL_ADD_FIELD_TABLE1 Failed(%s)", errMsg.c_str());
        return false;
    }

    int errCode1 = sqlite3_exec(db, SQL_ADD_INDEX_TABLE2, nullptr, nullptr, &errMsg);
    if (errCode1 != SQLITE_OK && errMsg != nullptr) {
        MST_LOG("sqlite3_exec SQL_ADD_INDEX_TABLE2 Failed(%s)", errMsg.c_str());
        return false;
    }

    int errCode2 = sqlite3_exec(db, SQL_DROP_TABLE3, nullptr, nullptr, &errMsg);
    if (errCode2 != SQLITE_OK && errMsg != nullptr) {
        MST_LOG("sqlite3_exec SQL_DROP_TABLE3 Failed(%s)", errMsg.c_str());
        return false;
    }

    int errCode3 = sqlite3_exec(db, SQL_DROP_CREATE_TABLE3, nullptr, nullptr, &errMsg);
    if (errCode3 != SQLITE_OK && errMsg != nullptr) {
        MST_LOG("sqlite3_exec SQL_ADD_INDEX_TABLE Failed(%s)", errMsg.c_str());
        return false;
    }

    int errCode4 = sqlite3_exec(db, SQL_DROP_TABLE4, nullptr, nullptr, &errMsg);
    if (errCode4 != SQLITE_OK && errMsg != nullptr) {
        MST_LOG("sqlite3_exec SQL_DROP_TABLE4 Failed(%s)", errMsg.c_str());
        return false;
    }

    int errCode5 = sqlite3_exec(db, SQL_DROP_CREATE_TABLE4, nullptr, nullptr, &errMsg);
    if (errCode5 != SQLITE_OK && errMsg != nullptr) {
        MST_LOG("sqlite3_exec SQL_DROP_CREATE_TABLE4 Failed(%s)", errMsg.c_str());
        return false;
    }
    return true;
}


bool DistributedRdbTools::Sqlite3ExecOpration(const sqlite3 *&db, cont char *&sql_name)
{
    if (db == nullptr) {
        MST_LOG("openStore Failed");
        return false;
    }
    int errCode = sqlite3_exec(db, sql_name, 0, 0, 0);
    if (errCode != SQLITE_OK) {
        MST_LOG("%s Failed!", sql_name);
        return false;
    }
    return true;
}

void DistributedRdbTools::CloseSqlite3Store(sqlite3 *&db)
{
    if (db != nullptr) {
        sqlite3_close(db);
        db = nullptr;
    }
}