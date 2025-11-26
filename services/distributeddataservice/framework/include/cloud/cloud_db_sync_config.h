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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_DB_SYNC_CONFIG_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_DB_SYNC_CONFIG_H

#include "serializable/serializable.h"
#include "visibility.h"

namespace OHOS::DistributedData {
class API_EXPORT CloudDbSyncConfig final : public Serializable {
public:
    struct API_EXPORT TableSyncConfig final : public Serializable {
        std::string tableName = "";
        bool cloudSyncEnabled = false;

        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
        bool operator==(const TableSyncConfig &config) const;
        bool operator!=(const TableSyncConfig &config) const;
    };

    struct API_EXPORT DbSyncConfig final : public Serializable {
        std::string dbName = "";
        bool cloudSyncEnabled = false;
        std::vector<TableSyncConfig> tableConfigs;

        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
        bool operator==(const DbSyncConfig &config) const;
        bool operator!=(const DbSyncConfig &config) const;
    };

    struct API_EXPORT AppSyncConfig final : public Serializable {
        std::string bundleName = "";
        std::vector<DbSyncConfig> dbConfigs;

        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
        bool operator==(const AppSyncConfig &config) const;
        bool operator!=(const AppSyncConfig &config) const;
    };

    std::map<std::string, AppSyncConfig> appConfigs;

    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    bool operator==(const CloudDbSyncConfig &config) const;
    bool operator!=(const CloudDbSyncConfig &config) const;

    static std::string GetKey(int32_t userId);

private:
    static constexpr const char *KEY_PREFIX = "CLOUD_DB_SYNC_CONFIG";
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_DB_SYNC_CONFIG_H