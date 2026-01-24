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

#include <map>

#include "serializable/serializable.h"
#include "visibility.h"

namespace OHOS::DistributedData {
class API_EXPORT CloudDbSyncConfig final : public Serializable {
public:
    struct TableSyncConfig final : public Serializable {
        std::string tableName = "";
        bool cloudSyncEnabled = true;

        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
    };

    struct DbSyncConfig final : public Serializable {
        std::string dbName = "";
        bool cloudSyncEnabled = true;
        std::vector<TableSyncConfig> tableConfigs;

        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
    };

    std::string bundleName = "";
    std::vector<DbSyncConfig> dbConfigs;

    API_LOCAL bool Marshal(json &node) const override;
    API_LOCAL bool Unmarshal(const json &node) override;

    std::string GetKey(int32_t userId, const std::string &bundleName);

private:
    static constexpr const char *KEY_PREFIX = "CLOUD_DB_SYNC_CONFIG";
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_DB_SYNC_CONFIG_H