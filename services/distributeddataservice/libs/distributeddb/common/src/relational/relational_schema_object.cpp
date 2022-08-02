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
#ifdef RELATIONAL_STORE
#include "relational_schema_object.h"

#include <algorithm>

#include "db_common.h"
#include "json_object.h"
#include "schema_constant.h"
#include "schema_utils.h"

namespace DistributedDB {
bool RelationalSchemaObject::IsSchemaValid() const
{
    return isValid_;
}

SchemaType RelationalSchemaObject::GetSchemaType() const
{
    return schemaType_;
}

std::string RelationalSchemaObject::ToSchemaString() const
{
    return schemaString_;
}

int RelationalSchemaObject::ParseFromSchemaString(const std::string &inSchemaString)
{
    if (isValid_) {
        return -E_NOT_PERMIT;
    }

    if (inSchemaString.empty() || inSchemaString.size() > SchemaConstant::SCHEMA_STRING_SIZE_LIMIT) {
        LOGE("[RelationalSchema][Parse] SchemaSize=%zu is invalid.", inSchemaString.size());
        return -E_INVALID_ARGS;
    }
    JsonObject schemaObj;
    int errCode = schemaObj.Parse(inSchemaString);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Schema json string parse failed: %d.", errCode);
        return errCode;
    }

    errCode = ParseRelationalSchema(schemaObj);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Parse to relational schema failed: %d.", errCode);
        return errCode;
    }

    schemaType_ = SchemaType::RELATIVE;
    schemaString_ = schemaObj.ToString();
    isValid_ = true;
    return E_OK;
}

void RelationalSchemaObject::GenerateSchemaString()
{
    schemaString_ = {};
    schemaString_ += "{";
    schemaString_ += R"("SCHEMA_VERSION":")" + schemaVersion_ + R"(",)";
    schemaString_ += R"("SCHEMA_TYPE":"RELATIVE",)";
    if (schemaVersion_ == SchemaConstant::SCHEMA_SUPPORT_VERSION_V2_1) {
        std::string modeString = tableMode_ == DistributedTableMode::COLLABORATION ?
            SchemaConstant::KEYWORD_TABLE_COLLABORATION : SchemaConstant::KEYWORD_TABLE_SPLIT_DEVICE;
        schemaString_ += R"("TABLE_MODE":")" + modeString + R"(",)";
    }
    schemaString_ += R"("TABLES":[)";
    for (auto it = tables_.begin(); it != tables_.end(); it++) {
        if (it != tables_.begin()) {
            schemaString_ += ",";
        }
        schemaString_ += it->second.ToTableInfoString(schemaVersion_);
    }
    schemaString_ += R"(])";
    schemaString_ += "}";
}

void RelationalSchemaObject::AddRelationalTable(const TableInfo &tb)
{
    tables_[tb.GetTableName()] = tb;
    isValid_ = true;
    if (tb.GetPrimaryKey().size() > 1) { // Table with composite primary keys
        // Composite primary keys are supported since version 2.1
        schemaVersion_ = SchemaConstant::SCHEMA_CURRENT_VERSION;
    }
    GenerateSchemaString();
}

void RelationalSchemaObject::RemoveRelationalTable(const std::string &tableName)
{
    tables_.erase(tableName);
    GenerateSchemaString();
}

const std::map<std::string, TableInfo> &RelationalSchemaObject::GetTables() const
{
    return tables_;
}

std::vector<std::string> RelationalSchemaObject::GetTableNames() const
{
    std::vector<std::string> tableNames;
    for (const auto &it : tables_) {
        tableNames.emplace_back(it.first);
    }
    return tableNames;
}

TableInfo RelationalSchemaObject::GetTable(const std::string &tableName) const
{
    auto it = tables_.find(tableName);
    if (it != tables_.end()) {
        return it->second;
    }
    return {};
}

std::string RelationalSchemaObject::GetSchemaVersion() const
{
    return schemaVersion_;
}

DistributedTableMode RelationalSchemaObject::GetTableMode() const
{
    return tableMode_;
}

void RelationalSchemaObject::SetTableMode(DistributedTableMode mode)
{
    tableMode_ = mode;
    if (tableMode_ == DistributedTableMode::COLLABORATION) {
        schemaVersion_ = SchemaConstant::SCHEMA_CURRENT_VERSION;
    }
    GenerateSchemaString();
}

int RelationalSchemaObject::CompareAgainstSchemaObject(const std::string &inSchemaString,
    std::map<std::string, int> &cmpRst) const
{
    return E_OK;
}

int RelationalSchemaObject::CompareAgainstSchemaObject(const RelationalSchemaObject &inSchemaObject,
    std::map<std::string, int> &cmpRst) const
{
    return E_OK;
}

namespace {
int GetMemberFromJsonObject(const JsonObject &inJsonObject, const std::string &fieldName, FieldType expectType,
    bool isNecessary, FieldValue &fieldValue)
{
    if (!inJsonObject.IsFieldPathExist(FieldPath {fieldName})) {
        if (isNecessary) {
            LOGE("[RelationalSchema][Parse] Get schema %s not exist. isNecessary: %d", fieldName.c_str(), isNecessary);
            return -E_SCHEMA_PARSE_FAIL;
        }
        return -E_NOT_FOUND;
    }

    FieldType fieldType;
    int errCode = inJsonObject.GetFieldTypeByFieldPath(FieldPath {fieldName}, fieldType);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema %s fieldType failed: %d.", fieldName.c_str(), errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }

    if (fieldType != expectType) {
        LOGE("[RelationalSchema][Parse] Expect %s fieldType %d but: %d.", fieldName.c_str(),
            static_cast<int>(expectType), static_cast<int>(fieldType));
        return -E_SCHEMA_PARSE_FAIL;
    }

    errCode = inJsonObject.GetFieldValueByFieldPath(FieldPath {fieldName}, fieldValue);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema %s value failed: %d.", fieldName.c_str(), errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    return E_OK;
}
}

int RelationalSchemaObject::ParseRelationalSchema(const JsonObject &inJsonObject)
{
    int errCode = ParseCheckSchemaVersion(inJsonObject);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckSchemaType(inJsonObject);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckTableMode(inJsonObject);
    if (errCode != E_OK) {
        return errCode;
    }
    return ParseCheckSchemaTableDefine(inJsonObject);
}

namespace {
inline bool IsSchemaVersionValid(const std::string &version)
{
    std::string stripedVersion = SchemaUtils::Strip(version);
    return stripedVersion == SchemaConstant::SCHEMA_SUPPORT_VERSION_V2 ||
        stripedVersion == SchemaConstant::SCHEMA_SUPPORT_VERSION_V2_1;
}
}

int RelationalSchemaObject::ParseCheckSchemaVersion(const JsonObject &inJsonObject)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, SchemaConstant::KEYWORD_SCHEMA_VERSION,
        FieldType::LEAF_FIELD_STRING, true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }

    if (IsSchemaVersionValid(fieldValue.stringValue)) {
        schemaVersion_ = fieldValue.stringValue;
        return E_OK;
    }

    LOGE("[RelationalSchema][Parse] Unexpected SCHEMA_VERSION=%s.", fieldValue.stringValue.c_str());
    return -E_SCHEMA_PARSE_FAIL;
}

int RelationalSchemaObject::ParseCheckSchemaType(const JsonObject &inJsonObject)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, SchemaConstant::KEYWORD_SCHEMA_TYPE,
        FieldType::LEAF_FIELD_STRING, true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }

    if (SchemaUtils::Strip(fieldValue.stringValue) != SchemaConstant::KEYWORD_TYPE_RELATIVE) {
        LOGE("[RelationalSchema][Parse] Unexpected SCHEMA_TYPE=%s.", fieldValue.stringValue.c_str());
        return -E_SCHEMA_PARSE_FAIL;
    }
    schemaType_ = SchemaType::RELATIVE;
    return E_OK;
}

namespace {
inline bool IsTableModeValid(const std::string &mode)
{
    std::string stripedMode = SchemaUtils::Strip(mode);
    return stripedMode == SchemaConstant::KEYWORD_TABLE_SPLIT_DEVICE ||
        stripedMode == SchemaConstant::KEYWORD_TABLE_COLLABORATION;
}
}

int RelationalSchemaObject::ParseCheckTableMode(const JsonObject &inJsonObject)
{
    if (schemaVersion_ == SchemaConstant::SCHEMA_SUPPORT_VERSION_V2) {
        return E_OK; // version 2 has no table mode, no parsing required
    }

    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, SchemaConstant::KEYWORD_TABLE_MODE,
        FieldType::LEAF_FIELD_STRING, true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }

    if (!IsTableModeValid(fieldValue.stringValue)) {
        LOGE("[RelationalSchema][Parse] Unexpected TABLE_MODE=%s.", fieldValue.stringValue.c_str());
        return -E_SCHEMA_PARSE_FAIL;
    }

    tableMode_ = SchemaUtils::Strip(fieldValue.stringValue) == SchemaConstant::KEYWORD_TABLE_SPLIT_DEVICE ?
        DistributedDB::SPLIT_BY_DEVICE : DistributedTableMode::COLLABORATION;
    return E_OK;
}

int RelationalSchemaObject::ParseCheckSchemaTableDefine(const JsonObject &inJsonObject)
{
    FieldType fieldType;
    int errCode = inJsonObject.GetFieldTypeByFieldPath(FieldPath {SchemaConstant::KEYWORD_SCHEMA_TABLE}, fieldType);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema TABLES fieldType failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    if (FieldType::LEAF_FIELD_ARRAY != fieldType) {
        LOGE("[RelationalSchema][Parse] Expect TABLES fieldType ARRAY but %s.",
            SchemaUtils::FieldTypeString(fieldType).c_str());
        return -E_SCHEMA_PARSE_FAIL;
    }
    std::vector<JsonObject> tables;
    errCode = inJsonObject.GetObjectArrayByFieldPath(FieldPath{SchemaConstant::KEYWORD_SCHEMA_TABLE}, tables);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema TABLES value failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    for (const JsonObject &table : tables) {
        errCode = ParseCheckTableInfo(table);
        if (errCode != E_OK) {
            LOGE("[RelationalSchema][Parse] Parse schema TABLES failed: %d.", errCode);
            return errCode;
        }
    }
    return E_OK;
}

int RelationalSchemaObject::ParseCheckTableInfo(const JsonObject &inJsonObject)
{
    TableInfo resultTable;
    int errCode = ParseCheckTableName(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckTableDefine(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckTableAutoInc(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckTablePrimaryKey(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckTableIndex(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckTableUnique(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }
    tables_[resultTable.GetTableName()] = resultTable;
    return E_OK;
}

int RelationalSchemaObject::ParseCheckTableName(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, "NAME", FieldType::LEAF_FIELD_STRING,
        true, fieldValue);
    if (errCode == E_OK) {
        resultTable.SetTableName(fieldValue.stringValue);
    }
    return errCode;
}

int RelationalSchemaObject::ParseCheckTableDefine(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    std::map<FieldPath, FieldType> tableFields;
    int errCode = inJsonObject.GetSubFieldPathAndType(FieldPath {"DEFINE"}, tableFields);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema TABLES DEFINE failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }

    for (const auto &field : tableFields) {
        if (field.second != FieldType::INTERNAL_FIELD_OBJECT) {
            LOGE("[RelationalSchema][Parse] Expect schema TABLES DEFINE fieldType INTERNAL OBJECT but : %s.",
                SchemaUtils::FieldTypeString(field.second).c_str());
            return -E_SCHEMA_PARSE_FAIL;
        }

        JsonObject fieldObj;
        errCode = inJsonObject.GetObjectByFieldPath(field.first, fieldObj);
        if (errCode != E_OK) {
            LOGE("[RelationalSchema][Parse] Get table field object failed. %d", errCode);
            return errCode;
        }

        FieldInfo fieldInfo;
        fieldInfo.SetFieldName(field.first[1]); // 1 : table name element in path
        errCode = ParseCheckTableFieldInfo(fieldObj, field.first, fieldInfo);
        if (errCode != E_OK) {
            LOGE("[RelationalSchema][Parse] Parse table field info failed. %d", errCode);
            return -E_SCHEMA_PARSE_FAIL;
        }
        resultTable.AddField(fieldInfo);
    }
    return E_OK;
}

int RelationalSchemaObject::ParseCheckTableFieldInfo(const JsonObject &inJsonObject, const FieldPath &path,
    FieldInfo &field)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, "COLUMN_ID", FieldType::LEAF_FIELD_INTEGER, true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }
    field.SetColumnId(fieldValue.integerValue);

    errCode = GetMemberFromJsonObject(inJsonObject, "TYPE", FieldType::LEAF_FIELD_STRING, true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }
    field.SetDataType(fieldValue.stringValue);

    errCode = GetMemberFromJsonObject(inJsonObject, "NOT_NULL", FieldType::LEAF_FIELD_BOOL, true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }
    field.SetNotNull(fieldValue.boolValue);

    errCode = GetMemberFromJsonObject(inJsonObject, "DEFAULT", FieldType::LEAF_FIELD_STRING, false, fieldValue);
    if (errCode == E_OK) {
        field.SetDefaultValue(fieldValue.stringValue);
    } else if (errCode != -E_NOT_FOUND) {
        return errCode;
    }

    return E_OK;
}

int RelationalSchemaObject::ParseCheckTableAutoInc(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, "AUTOINCREMENT", FieldType::LEAF_FIELD_BOOL, false, fieldValue);
    if (errCode == E_OK) {
        resultTable.SetAutoIncrement(fieldValue.boolValue);
    } else if (errCode != -E_NOT_FOUND) {
        return errCode;
    }
    return E_OK;
}

int RelationalSchemaObject::ParseCheckTablePrimaryKey(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    if (!inJsonObject.IsFieldPathExist(FieldPath {"PRIMARY_KEY"})) {
        return E_OK;
    }

    FieldType type;
    int errCode = inJsonObject.GetFieldTypeByFieldPath(FieldPath {"PRIMARY_KEY"}, type);
    if (errCode != E_OK) {
        return errCode;
    }

    if (type == FieldType::LEAF_FIELD_STRING) { // Compatible with schema 2.0
        FieldValue fieldValue;
        errCode = GetMemberFromJsonObject(inJsonObject, "PRIMARY_KEY", FieldType::LEAF_FIELD_STRING, false, fieldValue);
        if (errCode == E_OK) {
            resultTable.SetPrimaryKey(fieldValue.stringValue, 1);
        }
    } else if (type == FieldType::LEAF_FIELD_ARRAY) {
        CompositeFields multiPrimaryKey;
        errCode = inJsonObject.GetStringArrayByFieldPath(FieldPath {"PRIMARY_KEY"}, multiPrimaryKey);
        if (errCode == E_OK) {
            int index = 1; // primary key index
            for (const auto &item : multiPrimaryKey) {
                resultTable.SetPrimaryKey(item, index++);
            }
        }
    } else {
        errCode = -E_SCHEMA_PARSE_FAIL;
    }
    return errCode;
}

int RelationalSchemaObject::ParseCheckTableIndex(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    if (!inJsonObject.IsFieldPathExist(FieldPath {"INDEX"})) { // INDEX is not necessary
        return E_OK;
    }
    std::map<FieldPath, FieldType> tableFields;
    int errCode = inJsonObject.GetSubFieldPathAndType(FieldPath {"INDEX"}, tableFields);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema TABLES INDEX failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }

    for (const auto &field : tableFields) {
        if (field.second != FieldType::LEAF_FIELD_ARRAY) {
            LOGE("[RelationalSchema][Parse] Expect schema TABLES INDEX fieldType ARRAY but : %s.",
                SchemaUtils::FieldTypeString(field.second).c_str());
            return -E_SCHEMA_PARSE_FAIL;
        }
        CompositeFields indexDefine;
        errCode = inJsonObject.GetStringArrayByFieldPath(field.first, indexDefine);
        if (errCode != E_OK) {
            LOGE("[RelationalSchema][Parse] Get schema TABLES INDEX field value failed: %d.", errCode);
            return -E_SCHEMA_PARSE_FAIL;
        }
        resultTable.AddIndexDefine(field.first[1], indexDefine); // 1 : second element in path
    }
    return E_OK;
}

int RelationalSchemaObject::ParseCheckTableUnique(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    if (!inJsonObject.IsFieldPathExist(FieldPath {"UNIQUE"})) { // UNIQUE is not necessary
        return E_OK;
    }

    std::vector<CompositeFields> uniques;
    int errCode = inJsonObject.GetArrayContentOfStringOrStringArray(FieldPath {"UNIQUE"}, uniques);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get schema TABLES UNIQUE failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    resultTable.SetUniqueDefine(uniques);
    return E_OK;
}
}
#endif