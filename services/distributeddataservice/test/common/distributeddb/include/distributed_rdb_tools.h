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
#ifndef DISTRIBUTED_RDB_MODULE_TEST_TOOLS_H
#define DISTRIBUTED_RDB_MODULE_TEST_TOOLS_H
#include <condition_variable>
#include <thread>

#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "distributed_test_sysinfo.h"
#include "distributeddb_data_generator.h"
#include "log_print.h"
#ifdef TESTCASES_USING_GTEST
#define HWTEST_F(test_case_name, test_name, level) TEST_F(test_case_name, test_name)
#endif

struct RdbParameters {
    std::string path;
    std::string storeId;
    std::string appId;
    std::string userId;
    RdbParameters(str::string pathStr, std::string storeIdStr, std::string appIdStr, std::string userIdStr)
        : pathStr(pathStr), storeId(storeIdStr), appId(appIdStr), userId(userIdStr)
    {
    }
};

const static std::string TAG = "DistributedRdbTools";

const static std::string NORMAL_PATH = "/data/test/";
const static std::string NON_EXISTENT_PATH = "/data/test/nonExistent_rdb/";
const static std::string UNREADABLE_PATH = "/data/test/unreadable_rdb/";
const static std::string UNWRITABLE_PATH = "/data/test/unwritable_rdb/";

const static std::string NULL_STOREID = {};
const static std::string ILLEGAL_STOREID = "rdb_$%#@~%";
const static std::string MODE_STOREID = "rdb_mode";
const static std::string FULL_STOREID = "rdb_full";
const static std::string SUPER_STOREID = "rdb_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" +
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

const static std::string NON_EXISTENT_TABLE = "non_table";
const static std::string KEYWORD_START_TABLE = "naturalbase_rdb_a";

const static std::string NORMAL_TABLE = "NORMAL_RDB";
const static std::string CONSTRAINT_TABLE = "CONSTRAINT_RDB";

const static RdbParameters g_rdbParameter1(NORMAL_PATH, DistributedDBDataGenerator::STORE_ID_1,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_1);

const static RdbParameters g_rdbParameter2(NORMAL_PATH, DistributedDBDataGenerator::STORE_ID,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_1);

const static RdbParameters g_rdbParameter3(NORMAL_PATH, ILLEGAL_STOREID,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_1);

const static RdbParameters g_rdbParameter4(NORMAL_PATH, MODE_STOREID,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_1);

const static RdbParameters g_rdbParameter5(NORMAL_PATH, SUPER_STOREID,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_1);

const static RdbParameters g_rdbParameter6(NON_EXISTENT_PATH, DistributedDBDataGenerator::STORE_ID_1,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_1);

const static RdbParameters g_rdbParameter7(UNREADABLE_PATH, DistributedDBDataGenerator::STORE_ID_1,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_1);

const static RdbParameters g_rdbParameter8(UNWRITABLE_PATH, DistributedDBDataGenerator::STORE_ID_1,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_1);

const static RdbParameters g_rdbParameter9(NORMAL_PATH, FULL_STOREID,
    DistributedDBDataGenerator::APP_ID_1, DistributedDBDataGenerator::USER_ID_1);

class DistributedRdbTools final {
public:
    DistributedRdbTools() {}
    ~DistributedRdbTools() {}
    // Relational Database OpenStore And CreateDistributeTable
    static DistributedDB::DBStatus GetOpenStoreStatus(const RelatetionalStoreManager *&manager,
        RelatetionalStoreDelegate *&delegate, const RdbParameters &param);
    static DistributedDB::DBStatus GetCreateDistributedTableStatus(const RelatetionalStoreDelegate *&delegate,
        const std::string &tableName);
    static bool CloseStore(const DistributedDB::RelatetionalStoreDelegate *&delegate);

    static bool InitSqlite3Store(sqlite3 *&db, const RdbParameters &param);
    static bool InitTableDataAndTrigger(const sqlite3 *&db) ;
    static bool AlterTableAttributes(const sqlite3 *&db);
    static bool Sqlite3ExecOpration(const sqlite3 *&db, cont char *&sql_name);
    static void CloseSqlite3Store(sqlite3 *&db);
}
#endif // DISTRIBUTED_RDB_MODULE_TEST_TOOLS_H