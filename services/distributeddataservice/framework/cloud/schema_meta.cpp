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

#include "cloud/schema_meta.h"
namespace OHOS::DistributedData {
static constexpr const char *KEY_PREFIX = "DistributedSchema";
static constexpr const char *KEY_SEPARATOR = "###";
bool SchemaMeta::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(metaVersion)], metaVersion);
    SetValue(node[GET_NAME(version)], version);
    SetValue(node[GET_NAME(bundleName)], bundleName);
    SetValue(node[GET_NAME(databases)], databases);
    return true;
}

bool SchemaMeta::Unmarshal(const Serializable::json &node)
{
    if (!GetValue(node, GET_NAME(metaVersion), metaVersion)) {
        metaVersion = 0;
    }
    GetValue(node, GET_NAME(version), version);
    GetValue(node, GET_NAME(bundleName), bundleName);
    GetValue(node, GET_NAME(databases), databases);
    GetValue(node, GET_NAME(dbSchema), databases);
    return true;
}

std::vector<std::string> Database::GetTableNames() const
{
    std::vector<std::string> tableNames;
    tableNames.reserve(tables.size());
    for (auto &table : tables) {
        tableNames.push_back(table.name);
        if (!table.sharedTableName.empty()) {
            tableNames.push_back(table.sharedTableName);
        }
    }
    return tableNames;
}

std::string Database::GetKey() const
{
    return GetKey({user, "default", bundleName, name});
}

std::string Database::GetKey(const std::initializer_list<std::string> &fields)
{
    std::string prefix = KEY_PREFIX;
    for (const auto &field : fields) {
        prefix.append(KEY_SEPARATOR).append(field);
    }
    return prefix;
}

std::string Database::GetPrefix(const std::initializer_list<std::string> &fields)
{
    return GetKey(fields).append(KEY_SEPARATOR);
}

bool Database::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(name)], name);
    SetValue(node[GET_NAME(alias)], alias);
    SetValue(node[GET_NAME(tables)], tables);
    SetValue(node[GET_NAME(version)], version);
    SetValue(node[GET_NAME(bundleName)], bundleName);
    SetValue(node[GET_NAME(user)], user);
    SetValue(node[GET_NAME(deviceId)], deviceId);
    SetValue(node[GET_NAME(autoSyncType)], autoSyncType);
    return true;
}

bool Database::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(name), name);
    GetValue(node, GET_NAME(alias), alias);
    GetValue(node, GET_NAME(tables), tables);
    GetValue(node, GET_NAME(dbName), name);
    GetValue(node, GET_NAME(autoSyncType), autoSyncType);
    GetValue(node, GET_NAME(user), user);
    GetValue(node, GET_NAME(deviceId), deviceId);
    GetValue(node, GET_NAME(version), version);
    GetValue(node, GET_NAME(bundleName), bundleName);
    return true;
}

bool Table::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(name)], name);
    SetValue(node[GET_NAME(sharedTableName)], sharedTableName);
    SetValue(node[GET_NAME(alias)], alias);
    SetValue(node[GET_NAME(fields)], fields);
    SetValue(node[GET_NAME(nearbyFields)], nearbyFields);
    return true;
}

bool Table::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(name), name);
    GetValue(node, GET_NAME(sharedTableName), sharedTableName);
    GetValue(node, GET_NAME(alias), alias);
    GetValue(node, GET_NAME(fields), fields);
    GetValue(node, GET_NAME(tableName), name);
    GetValue(node, GET_NAME(nearbyFields), nearbyFields);
    return true;
}

bool Field::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(colName)], colName);
    SetValue(node[GET_NAME(alias)], alias);
    SetValue(node[GET_NAME(type)], type);
    SetValue(node[GET_NAME(primary)], primary);
    SetValue(node[GET_NAME(nullable)], nullable);
    return true;
}

bool Field::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(colName), colName);
    GetValue(node, GET_NAME(alias), alias);
    GetValue(node, GET_NAME(type), type);
    GetValue(node, GET_NAME(primary), primary);
    GetValue(node, GET_NAME(nullable), nullable);
    GetValue(node, GET_NAME(columnName), colName);
    GetValue(node, GET_NAME(notNull), nullable);
    return true;
}

Database SchemaMeta::GetDataBase(const std::string &storeId)
{
    for (const auto &database : databases) {
        if (database.name == storeId) {
            return database;
        }
    }
    return {};
}

bool SchemaMeta::IsValid() const
{
    return !bundleName.empty() && !databases.empty();
}
} // namespace OHOS::DistributedData