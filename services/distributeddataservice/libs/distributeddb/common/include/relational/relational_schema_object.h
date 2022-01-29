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
#include "schema.h"
#include "schema_constant.h"

namespace DistributedDB {
using CompositeFields = std::vector<FieldName>;
class FieldInfo {
public:
    const std::string &GetFieldName() const;
    void SetFieldName(const std::string &fileName);
    const std::string &GetDataType() const;
    void SetDataType(const std::string &dataType);
    bool IsNotNull() const;
    void SetNotNull(bool isNotNull);
    // Use string type to save the default value define in the create table sql.
    // No need to use the real value because sqlite will complete them.
    bool HasDefaultValue() const;
    const std::string &GetDefaultValue() const;
    void SetDefaultValue(const std::string &value);
    // convert to StorageType according "Determination Of Column Affinity"
    StorageType GetStorageType() const;
    void SetStorageType(StorageType storageType);

    int GetColumnId() const;
    void SetColumnId(int cid);

    // return field define string like ("fieldName": "MY INT(21), NOT NULL, DEFAULT 123")
    std::string ToAttributeString() const;

    int CompareWithField(const FieldInfo &inField) const;
private:
    std::string fieldName_;
    std::string dataType_; // Type may be null
    StorageType storageType_ = StorageType::STORAGE_TYPE_NONE;
    bool isNotNull_ = false;
    bool hasDefaultValue_ = false;
    std::string defaultValue_;
    int64_t cid_ = -1;
};

class TableInfo {
public:
    const std::string &GetTableName() const;
    bool GetAutoIncrement() const;
    const std::string &GetCreateTableSql() const;
    const std::map<FieldName, FieldInfo> &GetFields() const; // <colName, colAttr>
    const std::vector<CompositeFields> &GetUniqueDefine() const;
    const std::map<std::string, CompositeFields> &GetIndexDefine() const;
    const FieldName &GetPrimaryKey() const;
    const std::vector<std::string> &GetTriggers() const;
    const std::string &GetDevId() const;

    void SetTableName(const std::string &tableName);
    void SetAutoIncrement(bool autoInc);
    void SetCreateTableSql(std::string sql); // set 'autoInc_' flag when set sql
    void AddField(const FieldInfo &field);
    void AddUniqueDefine(const CompositeFields &uniqueDefine);
    void SetUniqueDefines(const std::vector<CompositeFields> &uniqueDefines);
    void AddIndexDefine(const std::string &indexName, const CompositeFields &indexDefine);
    void SetPrimaryKey(const FieldName &fieldName); // not support composite index now
    void AddTrigger(const std::string &triggerName);
    std::string ToTableInfoString() const;
    void SetDevId(const std::string &devId);

    int CompareWithTable(const TableInfo &inTableInfo) const;
    std::map<FieldPath, SchemaAttribute> GetSchemaDefine() const;
    std::string GetFieldName(uint32_t cid) const;  // cid begin with 0
    bool IsValid() const;

private:
    void AddFieldDefineString(std::string &attrStr) const;
    void AddUniqueDefineString(std::string &attrStr) const;
    void AddIndexDefineString(std::string &attrStr) const;

    int CompareWithTableFields(const std::map<std::string, FieldInfo> &inTableFields) const;
    int CompareWithTableIndex(const std::map<std::string, CompositeFields> &inTableIndex) const;

    std::string tableName_;
    std::string devId_;
    bool autoInc_ = false; // only 'INTEGER PRIMARY KEY' could be defined as 'AUTOINCREMENT'
    std::string sql_;
    std::map<std::string, FieldInfo> fields_;
    FieldName primaryKey_;
    std::vector<CompositeFields> uniqueDefines_;
    std::map<std::string, CompositeFields> indexDefines_;
    std::vector<std::string> triggers_;
    JsonObject ToJsonObject() const;
    mutable std::vector<std::string> fieldNames_;
};

class RelationalSyncOpinion {
public:
    uint32_t CalculateParcelLen(uint32_t softWareVersion) const;
    int SerializeData(Parcel &parcel, uint32_t softWareVersion) const;
    static int DeserializeData(Parcel &parcel, RelationalSyncOpinion &opinion);
    SyncOpinion GetTableOpinion(const std::string& tableName) const;
    const std::map<std::string, SyncOpinion> &GetOpinions() const;
    void AddSyncOpinion(const std::string &tableName, const SyncOpinion &opinion);
private:
    std::map<std::string, SyncOpinion> opinions_;
};

class RelationalSyncStrategy {
public:
    SyncStrategy GetTableStrategy(const std::string &tableName) const;
    void AddSyncStrategy(const std::string &tableName, const SyncStrategy &strategy);
    const std::map<std::string, SyncStrategy> &GetStrategies() const;
private:
    std::map<std::string, SyncStrategy> strategies_;
};

class RelationalSchemaObject : public ISchema {
public:
    RelationalSchemaObject() = default;
    ~RelationalSchemaObject() override = default;

    static RelationalSyncOpinion MakeLocalSyncOpinion(const RelationalSchemaObject &localSchema,
        const std::string &remoteSchemaStr, uint8_t remoteSchemaType);

    // The remoteOpinion.checkOnReceive is ignored
    static RelationalSyncStrategy ConcludeSyncStrategy(const RelationalSyncOpinion &localOpinion,
        const RelationalSyncOpinion &remoteOpinion);

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

private:
    int CompareAgainstSchemaObject(const std::string &inSchemaString, std::map<std::string, int> &cmpRst) const;

    int CompareAgainstSchemaObject(const RelationalSchemaObject &inSchemaObject,
        std::map<std::string, int> &cmpRst) const;

    int ParseRelationalSchema(const JsonObject &inJsonObject);
    int ParseCheckSchemaType(const JsonObject &inJsonObject);
    int ParseCheckSchemaVersionMode(const JsonObject &inJsonObject);
    int ParseCheckSchemaTableDefine(const JsonObject &inJsonObject);
    int ParseCheckTableInfo(const JsonObject &inJsonObject);
    int ParseCheckTableName(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckTableDefine(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckTableFieldInfo(const JsonObject &inJsonObject, const FieldPath &path, FieldInfo &table);
    int ParseCheckTableAutoInc(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckTableUnique(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckTableIndex(const JsonObject &inJsonObject, TableInfo &resultTable);
    int ParseCheckTablePrimaryKey(const JsonObject &inJsonObject, TableInfo &resultTable);

    void GenerateSchemaString();

    bool isValid_ = false; // set to true after parse success from string or add at least one relational table
    SchemaType schemaType_ = SchemaType::RELATIVE; // Default RELATIVE
    std::string schemaString_; // The minified and valid schemaString
    std::string schemaVersion_ = SCHEMA_SUPPORT_VERSION_V2;
    std::map<std::string, TableInfo> tables_;
};
} // namespace DistributedDB
#endif // RELATIONAL_STORE
#endif // RELATIONAL_SCHEMA_OBJECT_H