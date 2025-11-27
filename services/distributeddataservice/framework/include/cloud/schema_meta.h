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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SCHEMA_META_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SCHEMA_META_H
#include "serializable/serializable.h"
namespace OHOS::DistributedData {
struct API_EXPORT Field final : public Serializable {
    std::string colName;
    std::string alias;
    int32_t type = 0;
    bool primary = false;
    bool nullable = true;
    bool dupCheckCol = false;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    bool operator==(const Field &field) const;
};

struct API_EXPORT Table final : public Serializable {
    std::string name;
    std::string sharedTableName;
    std::string alias;
    std::vector<Field> fields;
    std::vector<std::string> deviceSyncFields = {};
    std::vector<std::string> cloudSyncFields = {};
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    bool operator==(const Table &table) const;
};

struct API_EXPORT Database final : public Serializable {
    std::string name = "";
    std::string alias;
    std::vector<Table> tables;
    uint32_t autoSyncType = 0;
    std::string user = "";
    std::string deviceId = "";
    uint32_t version = 0;
    std::string bundleName = "";
    API_EXPORT std::string GetKey() const;
    API_EXPORT static std::string GetKey(const std::initializer_list<std::string> &fields);
    API_EXPORT static std::string GetPrefix(const std::initializer_list<std::string> &fields);
    std::vector<std::string> GetTableNames() const;
    std::vector<std::string> GetSyncTables() const;
    std::vector<std::string> GetCloudTables() const;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    bool operator==(const Database &database) const;
};

class API_EXPORT SchemaMeta final : public Serializable {
public:
    using Database = Database;
    using Table = Table;
    using Field = Field;
    static constexpr const char *DELETE_FIELD = "#_deleted";
    static constexpr const char *GID_FIELD = "#_gid";
    static constexpr const char *CREATE_FIELD = "#_createTime";
    static constexpr const char *MODIFY_FIELD = "#_modifyTime";
    static constexpr const char *CURSOR_FIELD = "#_cursor";
    static constexpr const char *ERROR_FIELD = "#_error";
    static constexpr const char *VERSION_FIELD = "#_version";
    static constexpr const char *REFERENCE_FIELD = "#_reference";
    static constexpr const char *CLOUD_OWNER = "cloud_owner";
    static constexpr const char *CLOUD_PRIVILEGE = "cloud_privilege";
    static constexpr const char *SHARING_RESOURCE = "#_sharing_resource";
    static constexpr const char *HASH_KEY = "#_hash_key";

    static constexpr uint32_t CURRENT_VERSION = 0x10001;
    static constexpr uint32_t CLEAN_WATER_VERSION = 0x10001;
    static inline uint32_t GetLowVersion(uint32_t metaVersion = CURRENT_VERSION)
    {
        return metaVersion & 0xFFFF;
    }

    static inline uint32_t GetHighVersion(uint32_t metaVersion = CURRENT_VERSION)
    {
        return metaVersion & ~0xFFFF;
    }
    uint32_t metaVersion = CURRENT_VERSION;
    int32_t version = 0;
    std::string bundleName;
    std::vector<Database> databases;
    bool e2eeEnable = false;

    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    bool IsValid() const;
    Database GetDataBase(const std::string &storeId);
    std::vector<std::string> GetStores();
    bool operator==(const SchemaMeta &meta) const;
    bool operator!=(const SchemaMeta &meta) const;
};

// Table mode of device data sync time
enum AutoSyncType {
    IS_NOT_AUTO_SYNC = 0,
    SYNC_ON_CHANGE = 1, // datachange sync
    SYNC_ON_READY, // onready sync
    SYNC_ON_CHANGE_READY //datachange and onready sync
};

} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SCHEMA_META_H