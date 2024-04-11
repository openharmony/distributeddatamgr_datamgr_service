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
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
};

struct API_EXPORT Table final : public Serializable {
    std::string name;
    std::string sharedTableName;
    std::string alias;
    std::vector<Field> fields;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
};

struct API_EXPORT Database final : public Serializable {
    std::string name = "";
    std::string alias;
    std::vector<Table> tables;
    std::vector<std::string> GetTableNames() const;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
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
    int32_t version = 0;
    std::string bundleName;
    std::vector<Database> databases;

    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    bool IsValid() const;
    Database GetDataBase(const std::string &storeId);
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SCHEMA_META_H
