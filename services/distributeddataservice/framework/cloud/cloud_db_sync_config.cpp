/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "cloud/cloud_db_sync_config.h"

#include "utils/constant.h"

namespace OHOS::DistributedData {
bool CloudDbSyncConfig::TableSyncConfig::Marshal(json &node) const
{
    SetValue(node[GET_NAME(tableName)], tableName);
    SetValue(node[GET_NAME(cloudSyncEnabled)], cloudSyncEnabled);
    return true;
}

bool CloudDbSyncConfig::TableSyncConfig::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(tableName), tableName);
    GetValue(node, GET_NAME(cloudSyncEnabled), cloudSyncEnabled);
    return true;
}

bool CloudDbSyncConfig::TableSyncConfig::operator==(const TableSyncConfig &config) const
{
    return std::tie(tableName, cloudSyncEnabled) == std::tie(config.tableName, config.cloudSyncEnabled);
}

bool CloudDbSyncConfig::TableSyncConfig::operator!=(const TableSyncConfig &config) const
{
    return !(*this == config);
}

bool CloudDbSyncConfig::DbSyncConfig::Marshal(json &node) const
{
    SetValue(node[GET_NAME(dbName)], dbName);
    SetValue(node[GET_NAME(cloudSyncEnabled)], cloudSyncEnabled);
    SetValue(node[GET_NAME(tableConfigs)], tableConfigs);
    return true;
}

bool CloudDbSyncConfig::DbSyncConfig::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(dbName), dbName);
    GetValue(node, GET_NAME(cloudSyncEnabled), cloudSyncEnabled);
    GetValue(node, GET_NAME(tableConfigs), tableConfigs);
    return true;
}

bool CloudDbSyncConfig::DbSyncConfig::operator==(const DbSyncConfig &config) const
{
    return std::tie(dbName, cloudSyncEnabled, tableConfigs) ==
           std::tie(config.dbName, config.cloudSyncEnabled, config.tableConfigs);
}

bool CloudDbSyncConfig::DbSyncConfig::operator!=(const DbSyncConfig &config) const
{
    return !(*this == config);
}

bool CloudDbSyncConfig::Marshal(json &node) const
{
    SetValue(node[GET_NAME(bundleName)], bundleName);
    SetValue(node[GET_NAME(dbConfigs)], dbConfigs);
    return true;
}

bool CloudDbSyncConfig::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(bundleName), bundleName);
    GetValue(node, GET_NAME(dbConfigs), dbConfigs);
    return true;
}

bool CloudDbSyncConfig::operator==(const CloudDbSyncConfig &config) const
{
    return std::tie(bundleName, dbConfigs) == std::tie(config.bundleName, config.dbConfigs);
}

bool CloudDbSyncConfig::operator!=(const CloudDbSyncConfig &config) const
{
    return !(*this == config);
}

std::string CloudDbSyncConfig::GetKey(int32_t userId, const std::string &bundleName)
{
    return Constant::Join(KEY_PREFIX, Constant::KEY_SEPARATOR, { std::to_string(userId), bundleName });
}
} // namespace OHOS::DistributedData
