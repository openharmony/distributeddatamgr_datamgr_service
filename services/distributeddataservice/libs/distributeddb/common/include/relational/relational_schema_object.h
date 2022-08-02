/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef RELATIONAL_SCHEMA_OBJECT_H
#define RELATIONAL_SCHEMA_OBJECT_H
#ifdef RELATIONAL_STORE
#include <map>
#include "data_value.h"
#include "json_object.h"
#include "parcel.h"
#include "ischema.h"
#include "schema_constant.h"
#include "table_info.h"

namespace DistributedDB {
class RelationalSchemaObject : public ISchema {
public:
    RelationalSchemaObject() = default;
    ~RelationalSchemaObject() override = default;

    bool IsSchemaValid() const override;

    SchemaType GetSchemaType() const override;

    std::string ToSchemaString() const override;

    // Should be called on an invalid SchemaObject, create new SchemaObject if need to reparse
    int ParseFromSchemaString(const std::string &inSchemaString) override;

    void AddRelationalTable(const TableInfo& tb);

    void RemoveRelationalTable(const std::string &tableName);

    const std::map<std::string, TableInfo> &GetTables() const;

    std::vector<std::string> GetTableNames() const;

    TableInfo GetTable(const std::string& tableName) const;

    std::string GetSchemaVersion() const;

    DistributedTableMode GetTableMode() const;
    void SetTableMode(DistributedTableMode mode);

private:
    int CompareAgainstSchemaObject(const std::string &inSchemaString, std::map<std::string, int> &cmpRst) const;

    int CompareAgainstSchemaObject(const RelationalSchemaObject &inSchemaObject,
        std::map<std::string, int> &cmpRst) const;

    int ParseRelationalSchema(const JsonObject &inJsonObject);
    int ParseCheckSchemaType(const JsonObject &inJsonObject);
    int ParseCheckTableMode(const JsonObject &inJsonObject);
    int ParseCheckSchemaVersion(const JsonObject &inJsonObject);
    int ParseCheckSchemaTableDefine(const JsonObject &inJsonObject);
    int ParseCheckTableInfo(const JsonObject &inJsonObject);
    int ParseCheckTableName(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckTableDefine(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckTableFieldInfo(const JsonObject &inJsonObject, const FieldPath &path, FieldInfo &table);
    int ParseCheckTableAutoInc(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckTableIndex(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckTableUnique(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckTablePrimaryKey(const JsonObject &inJsonObject, TableInfo &resultTable);

    void GenerateSchemaString();

    bool isValid_ = false; // set to true after parse success from string or add at least one relational table
    SchemaType schemaType_ = SchemaType::RELATIVE; // Default RELATIVE
    std::string schemaString_; // The minified and valid schemaString
    std::string schemaVersion_ = SchemaConstant::SCHEMA_SUPPORT_VERSION_V2; // Default version 2.0
    std::map<std::string, TableInfo> tables_;

    DistributedTableMode tableMode_ = DistributedTableMode::SPLIT_BY_DEVICE;
};
} // namespace DistributedDB
#endif // RELATIONAL_STORE
#endif // RELATIONAL_SCHEMA_OBJECT_H