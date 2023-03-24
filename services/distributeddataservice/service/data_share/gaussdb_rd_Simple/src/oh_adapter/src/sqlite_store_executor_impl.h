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

#include "kv_store_executor.h"
#include "sqlite3.h"

namespace DocumentDB {
class SqliteStoreExecutor : public KvStoreExecutor {
public:
    SqliteStoreExecutor(sqlite3 *handle);
    ~SqliteStoreExecutor() override;

    int PutData(const std::string &collName, const Key &key, const Value &value) override;
    int GetData(const std::string &collName, const Key &key, Value &value) const override;

    int CreateCollection(const std::string &name, int flag) override;
    int DropCollection(const std::string &name, int flag) override;

private:
    sqlite3 *dbHandle_ = nullptr;
};
} // DocumentDB
#endif // SQLITE_STORE_EXECUTOR_IMPL_H