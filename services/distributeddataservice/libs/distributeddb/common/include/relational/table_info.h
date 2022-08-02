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
#ifndef TABLE_INFO_H
#define TABLE_INFO_H

#include <map>
#include <string>
#include <vector>

#include "db_types.h"
#include "schema_constant.h"
#include "data_value.h"
#include "ischema.h"

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

    int CompareWithField(const FieldInfo &inField, bool isLite = false) const;

    bool Empty() const;
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
    const std::map<std::string, CompositeFields> &GetIndexDefine() const;
    const std::map<int, FieldName> &GetPrimaryKey() const;
    const std::vector<CompositeFields> &GetUniqueDefine() const;

    void SetTableName(const std::string &tableName);
    void SetAutoIncrement(bool autoInc);
    void SetCreateTableSql(const std::string &sql); // set 'autoInc_' flag when set sql
    void AddField(const FieldInfo &field);
    void AddIndexDefine(const std::string &indexName, const CompositeFields &indexDefine);
    void SetPrimaryKey(const std::map<int, FieldName> &key);
    void SetPrimaryKey(const FieldName &fieldName, int keyIndex);
    std::string ToTableInfoString(const std::string &schemaVersion) const;

    void SetUniqueDefine(const std::vector<CompositeFields> &uniqueDefine);

    int CompareWithTable(const TableInfo &inTableInfo,
        const std::string &schemaVersion = SchemaConstant::SCHEMA_SUPPORT_VERSION_V2) const;
    int CompareWithLiteSchemaTable(const TableInfo &liteTableInfo) const;

    std::map<FieldPath, SchemaAttribute> GetSchemaDefine() const;
    std::string GetFieldName(uint32_t cid) const;  // cid begin with 0
    const std::vector<FieldInfo> &GetFieldInfos() const;  // Sort by cid
    bool IsValid() const;

    // return a key to Identify a row data,
    // If there is a primary key, return the primary key，
    // If there is no primary key, return the first unique key,
    // If there is no unique key, return the rowid.
    CompositeFields GetIdentifyKey() const;

    void SetTableId(int id);
    int GetTableId() const;

    bool Empty() const;

private:
    void AddFieldDefineString(std::string &attrStr) const;
    void AddIndexDefineString(std::string &attrStr) const;
    void AddUniqueDefineString(std::string &attrStr) const;

    int CompareWithTableFields(const std::map<std::string, FieldInfo> &inTableFields, bool isLite = false) const;
    int CompareWithTableIndex(const std::map<std::string, CompositeFields> &inTableIndex) const;
    int CompareWithTableUnique(const std::vector<CompositeFields> &inTableUnique) const;

    int CompareWithLiteTableFields(const std::map<std::string, FieldInfo> &liteTableFields) const;

    std::string tableName_;
    bool autoInc_ = false; // only 'INTEGER PRIMARY KEY' could be defined as 'AUTOINCREMENT'
    std::string sql_;
    std::map<std::string, FieldInfo> fields_;
    std::map<int, FieldName> primaryKey_;
    std::map<std::string, CompositeFields> indexDefines_;
    mutable std::vector<FieldInfo> fieldInfos_;

    std::vector<CompositeFields> uniqueDefines_;
    int id_ = -1;
};
} // namespace DistributedDB
#endif // TABLE_INFO_H