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
#define LOG_TAG "CloudDbSyncConfig"

#include "cloud/cloud_db_sync_config.h"

#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "utils/constant.h"

namespace OHOS::DistributedData {
std::shared_mutex CloudDbSyncConfig::metaMutex_;
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

bool CloudDbSyncConfig::UpdateConfig(int32_t userId, const std::string &bundleName,
    const std::map<std::string, DbInfo> &dbInfo)
{
    if (dbInfo.empty()) {
        ZLOGW("dbInfo is empty for bundleName:%{public}s", bundleName.c_str());
        return false;
    }
    CloudDbSyncConfig config;
    {
        std::shared_lock<decltype(metaMutex_)> lock(metaMutex_);
        if (!MetaDataManager::GetInstance().LoadMeta(config.GetKey(userId, bundleName), config, true)) {
            config.bundleName = bundleName;
        }
    }
    bool hasChanges = false;
    for (const auto &[dbName, dbSwitch] : dbInfo) {
        hasChanges |= UpdateSingleDbConfig(config, dbName, dbSwitch);
    }
    if (hasChanges) {
        std::unique_lock<decltype(metaMutex_)> lock(metaMutex_);
        bool result = MetaDataManager::GetInstance().SaveMeta(config.GetKey(userId, bundleName), config, true);
        ZLOGI("Save config for %{public}s, result:%{public}d", bundleName.c_str(), result);
        return result;
    }
    ZLOGI("Config for %{public}s unchanged, skip saving", bundleName.c_str());
    return true;
}

bool CloudDbSyncConfig::IsDbEnable(int32_t userId, const std::string &bundleName, const std::string &dbName)
{
    std::shared_lock<decltype(metaMutex_)> lock(metaMutex_);
    CloudDbSyncConfig config;
    if (!MetaDataManager::GetInstance().LoadMeta(config.GetKey(userId, bundleName), config, true)) {
        return true;
    }
    auto dbIter = std::find_if(config.dbConfigs.begin(), config.dbConfigs.end(),
        [&dbName](const DbSyncConfig &dbConfig) { return dbConfig.dbName == dbName; });
    if (dbIter != config.dbConfigs.end()) {
        return dbIter->cloudSyncEnabled;
    }
    return true;
}

bool CloudDbSyncConfig::FilterCloudEnabledTables(int32_t userId, const std::string &bundleName,
    const std::string &dbName, std::vector<std::string> &tables)
{
    std::shared_lock<decltype(metaMutex_)> lock(metaMutex_);
    CloudDbSyncConfig config;
    if (!MetaDataManager::GetInstance().LoadMeta(config.GetKey(userId, bundleName), config, true)) {
        return true;
    }
    auto dbIter = std::find_if(config.dbConfigs.begin(), config.dbConfigs.end(),
        [&dbName](const DbSyncConfig &dbConfig) { return dbConfig.dbName == dbName; });
    if (dbIter == config.dbConfigs.end()) {
        return true;
    }
    auto &tableConfigs = dbIter->tableConfigs;
    auto isTableDisabled = [&tableConfigs](const std::string &table) {
        auto tableIter = std::find_if(tableConfigs.begin(), tableConfigs.end(),
            [&table](const TableSyncConfig &config) { return config.tableName == table && !config.cloudSyncEnabled; });
        return tableIter != tableConfigs.end();
    };
    tables.erase(std::remove_if(tables.begin(), tables.end(), isTableDisabled), tables.end());
    return !tables.empty();
}

bool CloudDbSyncConfig::UpdateTableConfigs(std::vector<TableSyncConfig> &tableConfigs,
    const std::map<std::string, bool> &tableInfo)
{
    bool hasChanges = false;
    for (const auto &pair : tableInfo) {
        const std::string &tableName = pair.first;
        bool tableEnable = pair.second;
        auto tableIter = std::find_if(tableConfigs.begin(), tableConfigs.end(),
            [&tableName](const TableSyncConfig &config) { return config.tableName == tableName; });
        if (tableIter == tableConfigs.end()) {
            TableSyncConfig tableConfig;
            tableConfig.tableName = tableName;
            tableConfig.cloudSyncEnabled = tableEnable;
            tableConfigs.push_back(std::move(tableConfig));
            hasChanges = true;
        } else if (tableIter->cloudSyncEnabled != tableEnable) {
            tableIter->cloudSyncEnabled = tableEnable;
            hasChanges = true;
        }
    }
    return hasChanges;
}

bool CloudDbSyncConfig::UpdateSingleDbConfig(CloudDbSyncConfig &config, const std::string &dbName,
    const DbInfo &dbSwitch)
{
    auto dbIter = std::find_if(config.dbConfigs.begin(), config.dbConfigs.end(),
        [&dbName](const CloudDbSyncConfig::DbSyncConfig &dbConfig) { return dbConfig.dbName == dbName; });
    if (dbIter == config.dbConfigs.end()) {
        CloudDbSyncConfig::DbSyncConfig newDbConfig;
        newDbConfig.dbName = dbName;
        newDbConfig.cloudSyncEnabled = dbSwitch.enable;
        for (const auto &[tableName, tableEnable] : dbSwitch.tableInfo) {
            TableSyncConfig tableConfig;
            tableConfig.tableName = tableName;
            tableConfig.cloudSyncEnabled = tableEnable;
            newDbConfig.tableConfigs.push_back(std::move(tableConfig));
        }
        config.dbConfigs.push_back(std::move(newDbConfig));
        return true;
    }
    bool hasChanges = false;
    if (dbIter->cloudSyncEnabled != dbSwitch.enable) {
        dbIter->cloudSyncEnabled = dbSwitch.enable;
        hasChanges = true;
    }
    if (!dbSwitch.tableInfo.empty()) {
        hasChanges |= UpdateTableConfigs(dbIter->tableConfigs, dbSwitch.tableInfo);
    }
    return hasChanges;
}
} // namespace OHOS::DistributedData
