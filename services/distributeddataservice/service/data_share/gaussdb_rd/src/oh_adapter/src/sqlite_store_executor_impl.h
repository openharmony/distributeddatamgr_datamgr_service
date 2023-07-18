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

#ifndef SQLITE_STORE_EXECUTOR_IMPL_H
#define SQLITE_STORE_EXECUTOR_IMPL_H

#include "db_config.h"
#include "json_common.h"
#include "kv_store_executor.h"
#include "sqlite3.h"

namespace DocumentDB {
class SqliteStoreExecutorImpl : public KvStoreExecutor {
public:
    static int CreateDatabase(const std::string &path, const DBConfig &config, sqlite3 *&db);

    SqliteStoreExecutorImpl() = default;
    explicit SqliteStoreExecutorImpl(sqlite3 *handle);
    ~SqliteStoreExecutorImpl() override;

    int GetDBConfig(std::string &config);
    int SetDBConfig(const std::string &config);

    int StartTransaction() override;
    int Commit() override;
    int Rollback() override;

    int PutData(const std::string &collName, Key &key, const Value &value, bool isNeedAddKeyType = true) override;
    int InsertData(const std::string &collName, Key &key, const Value &value, bool isNeedAddKeyType = true) override;
    int GetDataByKey(const std::string &collName, Key &key, Value &value) const override;
    int GetDataById(const std::string &collName, Key &key, Value &value) const override;
    int GetDataByFilter(const std::string &collName, Key &key, const JsonObject &filterObj,
        std::pair<std::string, std::string> &values, int isIdExist) const override;
    int DelData(const std::string &collName, Key &key) override;

    int CreateCollection(const std::string &name, const std::string &option, bool ignoreExists) override;
    int DropCollection(const std::string &name, bool ignoreNonExists) override;
    bool IsCollectionExists(const std::string &name, int &errCode) override;

    int SetCollectionOption(const std::string &name, const std::string &option) override;
    int CleanCollectionOption(const std::string &name) override;

private:
    sqlite3 *dbHandle_ = nullptr;
};
} // namespace DocumentDB
#endif // SQLITE_STORE_EXECUTOR_IMPL_H