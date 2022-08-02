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

#ifndef COLLABORATION_LOG_TABLE_MANAGER_H
#define COLLABORATION_LOG_TABLE_MANAGER_H
#include "sqlite_log_table_manager.h"

namespace DistributedDB {
class CollaborationLogTableManager : public SqliteLogTableManager {
public:
    CollaborationLogTableManager() = default;
    ~CollaborationLogTableManager() override = default;

    std::string CalcPrimaryKeyHash(const std::string &references, const TableInfo &table,
        const std::string &identity) override;

private:
    bool IsCollaborationWithoutKey(const TableInfo &table);

    std::string GetPrimaryKeySql(const TableInfo &table) override;
    std::string GetIndexSql(const TableInfo &table) override;

    std::string GetInsertTrigger(const TableInfo &table, const std::string &identity) override;
    std::string GetUpdateTrigger(const TableInfo &table, const std::string &identity) override;
    std::string GetDeleteTrigger(const TableInfo &table, const std::string &identity) override;
};
}
#endif // COLLABORATION_LOG_TABLE_MANAGER_H