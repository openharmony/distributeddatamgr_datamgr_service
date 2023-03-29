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
#include "log_print.h"
#include "sqlite_store_executor_impl.h"
#include "sqlite_utils.h"

namespace DocumentDB {
constexpr const char *APP_ID = "APP_ID";
constexpr const char *USER_ID = "USER_ID";
constexpr const char *STORE_ID = "STORE_ID";

int KvStoreManager::GetKvStore(const std::string &path, const DBConfig &config, KvStoreExecutor *&executor)
{
    if (executor != nullptr) {
        return -E_INVALID_ARGS;
    }

    sqlite3 *db = nullptr;
    int errCode = SqliteStoreExecutor::CreateDatabase(path, config, db);
    if (errCode != E_OK) {
        GLOGE("Get kv store failed. %d", errCode);
        return errCode;
    }

    auto *sqliteExecutor = new (std::nothrow) SqliteStoreExecutor(db);
    if (sqliteExecutor == nullptr) {
        return -E_OUT_OF_MEMORY;
    }

    std::string oriConfigStr;
    errCode = sqliteExecutor->GetDBConfig(oriConfigStr);
    GLOGD("----> Get original db config: [%s] %d", oriConfigStr.c_str(), errCode);
    if (errCode == -E_NOT_FOUND) {
        errCode = sqliteExecutor->SetDBConfig(config.ToString());
    } else if (errCode != E_OK) {
        goto END;
    } else {
        DBConfig oriDbConfig = DBConfig::ReadConfig(oriConfigStr, errCode);
        if (errCode != E_OK) {
            GLOGE("Read db config changed. %d", errCode);
            goto END;
        }
        if (config != oriDbConfig) {
            errCode = -E_INVALID_CONFIG_VALUE;
            GLOGE("Get kv store failed, db config changed. %d", errCode);
            goto END;
        }
    }

    executor = sqliteExecutor;
    return E_OK;

END:
    delete sqliteExecutor;
    sqliteExecutor = nullptr;
    return errCode;
}
}