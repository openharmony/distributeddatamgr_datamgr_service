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
#include "types.h"
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

const static std::string TAG = "DistributedRdbTestTools";

const std::string NORMAL_PATH = "/data/test/";
const std::string NON_EXISTENT_PATH = "/data/test/nonExistent_rdb/";
const std::string UNREADABLE_PATH = "/data/test/unreadable_rdb/";
const std::string UNWRITABLE_PATH = "/data/test/unwritable_rdb/";

const std::string NULL_STOREID = "";
const std::string ILLEGAL_STOREID = "rdb_$%#@~%";
const std::string MODE_STOREID = "rdb_mode";
const std::string FULL_STOREID = "rdb_full";
const std::string SUPER_STOREID = "rdb_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" +
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

const std::string NON_EXISTENT_TABLE = "non_table";
const std::string KEYWORD_START_TABLE = "naturalbase_rdb_a";

const std::string NORMAL_TABLE = "NORMAL_RDB";
const std::string CONSTRAINT_TABLE = "CONSTRAINT_RDB";

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

class DistributedRdbTestTools final {
public:
    DistributedRdbTestTools() {}
    ~DistributedRdbTestTools() {}
    // Relational Database OpenStore And CreateDistributeTable
    static DistributedDB::DBStatus GetOpenStoreStatus(const RelatetionalStoreManager *&manager, RelatetionalStoreDelegate *&delegate,
        const RdbParameters &param);
    static DistributedDB::DBStatus GetCreateDistributedTableStatus(const RelatetionalStoreDelegate *&delegate, 
        const std::string &tableName);
    static bool CloseStore(const DistributedDB::RelatetionalStoreDelegate *&delegate);
private: 
}
#endif // DISTRIBUTED_RDB_MODULE_TEST_TOOLS_H