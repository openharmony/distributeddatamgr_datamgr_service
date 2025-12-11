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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_SYNC_CONFIG_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_SYNC_CONFIG_H

#include <map>
#include <mutex>

#include "cloud/cloud_db_sync_config.h"
#include "cloud_types.h"
namespace OHOS::CloudData {
class SyncConfig {
public:
    using CloudDbSyncConfig = OHOS::DistributedData::CloudDbSyncConfig;
    using TableSyncConfig = CloudDbSyncConfig::TableSyncConfig;

    static bool UpdateConfig(int32_t userId, const std::string &bundleName,
        const std::map<std::string, DBSwitchInfo> &dbInfo);
    static bool IsDbEnable(int32_t userId, const std::string &bundleName, const std::string &dbName);
    static bool FilterCloudEnabledTables(int32_t userId, const std::string &bundleName, const std::string &dbName,
        std::vector<std::string> &tables);

private:
    static bool UpdateTableConfigs(std::vector<TableSyncConfig> &tableConfigs,
        const std::map<std::string, bool> &tableInfo);
    static bool UpdateSingleDbConfig(CloudDbSyncConfig &config, const std::string &dbName,
        const DBSwitchInfo &dbSwitch);
    static std::mutex metaMutex_;
};
} // namespace OHOS::CloudData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_SYNC_CONFIG_H