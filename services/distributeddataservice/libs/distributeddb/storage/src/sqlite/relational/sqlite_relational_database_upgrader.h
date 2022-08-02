/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef SQLITE_RELATIONAL_DATABASE_UPGRADER_H
#define SQLITE_RELATIONAL_DATABASE_UPGRADER_H

#include "database_upgrader.h"
#include "sqlite_utils.h"
#include "types_export.h"

namespace DistributedDB {
class SqliteRelationalDatabaseUpgrader : public DatabaseUpgrader {
public:
    explicit SqliteRelationalDatabaseUpgrader(sqlite3 *db);
    ~SqliteRelationalDatabaseUpgrader() override;
    int Upgrade() override;

private:
    int BeginUpgrade();
    int ExecuteUpgrade();
    int EndUpgrade(bool isSuccess);

    int UpgradeTrigger(const std::string &logTableVersion);
    sqlite3 *db_;
};
} // namespace DistributedDB
#endif // SQLITE_RELATIONAL_DATABASE_UPGRADER_H