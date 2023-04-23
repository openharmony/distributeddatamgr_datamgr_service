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
#include "kv_store_executor.h"
#include "sqlite3.h"
#include "json_common.h"

namespace DocumentDB {
class SqliteStoreExecutor : public KvStoreExecutor {
public:
    static int CreateDatabase(const std::string &path, const DBConfig &config, sqlite3 *&db);

    SqliteStoreExecutor(sqlite3 *handle);
    ~SqliteStoreExecutor() override;

    int GetDBConfig(std::string &config);
    int SetDBConfig(const std::string &config);

    int PutData(const std::string &collName, const Key &key, const Value &value) override;
    int GetData(const std::string &collName, const Key &key, Value &value) const override;
    int GetFilededData(const std::string &collName, const JsonObject &filterObj, std::vector<std::pair<std::string, std::string>> &values) const override;
    int DelData(const std::string &collName, const Key &key) override;

    int CreateCollection(const std::string &name, bool ignoreExists) override;
    int DropCollection(const std::string &name, bool ignoreNonExists) override;
    bool IsCollectionExists(const std::string &name, int &errCode) override;

    int GetCollectionOption(const std::string &name, std::string &option) override;
    int SetCollectionOption(const std::string &name, const std::string &option) override;
    int CleanCollectionOption(const std::string &name) override;

private:
    sqlite3 *dbHandle_ = nullptr;
};
} // DocumentDB
#endif // SQLITE_STORE_EXECUTOR_IMPL_H