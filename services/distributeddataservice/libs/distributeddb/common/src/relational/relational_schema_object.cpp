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

#include "json_object.h"
#include "schema_constant.h"
#include "schema_utils.h"

namespace DistributedDB {
const std::string &FieldInfo::GetFieldName() const
{
    return fieldName_;
}

void FieldInfo::SetFieldName(const std::string &fileName)
{
    fieldName_ = fileName;
}

const std::string &FieldInfo::GetDataType() const
{
    return dataType_;
}

static StorageType AffinityType(const std::string &dataType)
{
    return StorageType::STORAGE_TYPE_NULL;
}

void FieldInfo::SetDataType(const std::string &dataType)
{
    dataType_ = dataType;
    transform(dataType_.begin(), dataType_.end(), dataType_.begin(), ::tolower);
    storageType_ = AffinityType(dataType_);
}

bool FieldInfo::IsNotNull() const
{
    return isNotNull_;
}

void FieldInfo::SetNotNull(bool isNotNull)
{
    isNotNull_ = isNotNull;
}

bool FieldInfo::HasDefaultValue() const
{
    return hasDefaultValue_;
}

const std::string &FieldInfo::GetDefaultValue() const
{
    return defaultValue_;
}

void FieldInfo::SetDefaultValue(const std::string &value)
{
    hasDefaultValue_ = true;
    defaultValue_ = value;
}

// convert to StorageType according "Determination Of Column Affinity"
StorageType FieldInfo::GetStorageType() const
{
    return storageType_;
}

void FieldInfo::SetStorageType(StorageType storageType)
{
    storageType_ = storageType;
}

int FieldInfo::GetColumnId() const
{
    return cid_;
}

void FieldInfo::SetColumnId(int cid)
{
    cid_ = cid;
}

std::string FieldInfo::ToAttributeString() const
{
    std::string attrStr = "\"" + fieldName_ + "\": {";
    attrStr += "\"COLUMN_ID\":" + std::to_string(cid_) + ",";
    attrStr += "\"TYPE\":\"" + dataType_ + "\",";
    attrStr += "\"NOT_NULL\":" + std::string(isNotNull_ ? "true" : "false");
    if (hasDefaultValue_) {
        attrStr += ",";
        attrStr += "\"DEFAULT\":\"" + defaultValue_ + "\"";
    }
    attrStr += "}";
    return attrStr;
}

int FieldInfo::CompareWithField(const FieldInfo &inField) const
{
    if (fieldName_ != inField.GetFieldName() || dataType_ != inField.GetDataType() ||
        isNotNull_ != inField.IsNotNull()) {
        return false;
    }
    if (hasDefaultValue_ && inField.HasDefaultValue()) {
        return defaultValue_ == inField.GetDefaultValue();
    }
    return hasDefaultValue_ == inField.HasDefaultValue();
}

const std::string &TableInfo::GetTableName() const
{
    return tableName_;
}

void TableInfo::SetTableName(const std::string &tableName)
{
    tableName_ = tableName;
}

void TableInfo::SetAutoIncrement(bool autoInc)
{
    autoInc_ = autoInc;
}

bool TableInfo::GetAutoIncrement() const
{
    return autoInc_;
}

const std::string &TableInfo::GetCreateTableSql() const
{
    return sql_;
}

void TableInfo::SetCreateTableSql(std::string sql)
{
    sql_ = sql;
    for (auto &c : sql) {
        c = static_cast<char>(std::toupper(c));
    }
    if (sql.find("AUTOINCREMENT") != std::string::npos) {
        autoInc_ = true;
    }
}

const std::map<std::string, FieldInfo> &TableInfo::GetFields() const
{
    return fields_;
}

const std::vector<FieldInfo> &TableInfo::GetFieldInfos() const
{
    if (!fieldInfos_.empty() && fieldInfos_.size() == fields_.size()) {
        return fieldInfos_;
    }
    fieldInfos_.resize(fields_.size());
    if (fieldInfos_.size() != fields_.size()) {
        LOGE("GetField error, alloc memory failed.");
        return fieldInfos_;
    }
    for (const auto &entry : fields_) {
        if (static_cast<size_t>(entry.second.GetColumnId()) >= fieldInfos_.size()) {
            LOGE("Cid is over field size.");
            fieldInfos_.clear();
            return fieldInfos_;
        }
        fieldInfos_.at(entry.second.GetColumnId()) = entry.second;
    }
    return fieldInfos_;
}

std::string TableInfo::GetFieldName(uint32_t cid) const
{
    if (cid >= fields_.size() || GetFieldInfos().empty()) {
        return {};
    }
    return GetFieldInfos().at(cid).GetFieldName();
}

bool TableInfo::IsValid() const
{
    return !tableName_.empty();
}

void TableInfo::AddField(const FieldInfo &field)
{
    fields_[field.GetFieldName()] = field;
}

const std::map<std::string, CompositeFields> &TableInfo::GetIndexDefine() const
{
    return indexDefines_;
}

void TableInfo::AddIndexDefine(const std::string &indexName, const CompositeFields &indexDefine)
{
    indexDefines_[indexName] = indexDefine;
}

const FieldName &TableInfo::GetPrimaryKey() const
{
    return primaryKey_;
}

void TableInfo::SetPrimaryKey(const FieldName &fieldName)
{
    primaryKey_ = fieldName;
}

void TableInfo::AddFieldDefineString(std::string &attrStr) const
{
    if (fields_.empty()) {
        return;
    }
    attrStr += R"("DEFINE": {)";
    for (auto itField = fields_.begin(); itField != fields_.end(); ++itField) {
        attrStr += itField->second.ToAttributeString();
        if (itField != std::prev(fields_.end(), 1)) {
            attrStr += ",";
        }
    }
    attrStr += "},";
}

void TableInfo::AddIndexDefineString(std::string &attrStr) const
{
    if (indexDefines_.empty()) {
        return;
    }
    attrStr += R"(,"INDEX": {)";
    for (auto itIndexDefine = indexDefines_.begin(); itIndexDefine != indexDefines_.end(); ++itIndexDefine) {
        attrStr += "\"" + (*itIndexDefine).first + "\": [\"";
        for (auto itField = itIndexDefine->second.begin(); itField != itIndexDefine->second.end(); ++itField) {
            attrStr += *itField;
            if (itField != itIndexDefine->second.end() - 1) {
                attrStr += "\",\"";
            }
        }
        attrStr += "\"]";
        if (itIndexDefine != std::prev(indexDefines_.end(), 1)) {
            attrStr += ",";
        }
    }
    attrStr += "}";
}

int TableInfo::CompareWithTable(const TableInfo &inTableInfo) const
{
    if (tableName_ != inTableInfo.GetTableName()) {
        LOGW("[Relational][Compare] Table name is not same");
        return -E_RELATIONAL_TABLE_INCOMPATIBLE;
    }

    if (primaryKey_ != inTableInfo.GetPrimaryKey()) {
        LOGW("[Relational][Compare] Table primary key is not same");
        return -E_RELATIONAL_TABLE_INCOMPATIBLE;
    }

    int fieldCompareResult = CompareWithTableFields(inTableInfo.GetFields());
    if (fieldCompareResult == -E_RELATIONAL_TABLE_INCOMPATIBLE) {
        LOGW("[Relational][Compare] Compare table fields with in table, %d", fieldCompareResult);
        return -E_RELATIONAL_TABLE_INCOMPATIBLE;
    }

    int indexCompareResult = CompareWithTableIndex(inTableInfo.GetIndexDefine());
    return (fieldCompareResult == -E_RELATIONAL_TABLE_EQUAL) ? indexCompareResult : fieldCompareResult;
}

int TableInfo::CompareWithTableFields(const std::map<std::string, FieldInfo> &inTableFields) const
{
    auto itLocal = fields_.begin();
    auto itInTable = inTableFields.begin();
    int errCode = -E_RELATIONAL_TABLE_EQUAL;
    while (itLocal != fields_.end() && itInTable != inTableFields.end()) {
        if (itLocal->first == itInTable->first) { // Same field
            if (!itLocal->second.CompareWithField(itInTable->second)) { // Compare field
                LOGW("[Relational][Compare] Table field is incompatible"); // not compatible
                return -E_RELATIONAL_TABLE_INCOMPATIBLE;
            }
            itLocal++; // Compare next field
        } else { // Assume local table fields is a subset of in table
            if (itInTable->second.IsNotNull() && !itInTable->second.HasDefaultValue()) { // Upgrade field not compatible
                LOGW("[Relational][Compare] Table upgrade field should allowed to be empty or have default value.");
                return -E_RELATIONAL_TABLE_INCOMPATIBLE;
            }
            errCode = -E_RELATIONAL_TABLE_COMPATIBLE_UPGRADE;
        }
        itInTable++; // Next in table field
    }

    if (itLocal != fields_.end()) {
        LOGW("[Relational][Compare] Table field is missing");
        return -E_RELATIONAL_TABLE_INCOMPATIBLE;
    }

    if (itInTable == inTableFields.end()) {
        return errCode;
    }

    while (itInTable != inTableFields.end()) {
        if (itInTable->second.IsNotNull() && !itInTable->second.HasDefaultValue()) {
            LOGW("[Relational][Compare] Table upgrade field should allowed to be empty or have default value.");
            return -E_RELATIONAL_TABLE_INCOMPATIBLE;
        }
        itInTable++;
    }
    return -E_RELATIONAL_TABLE_COMPATIBLE_UPGRADE;
}

int TableInfo::CompareWithTableIndex(const std::map<std::string, CompositeFields> &inTableIndex) const
{
    // Index comparison results do not affect synchronization decisions
    auto itLocal = indexDefines_.begin();
    auto itInTable = inTableIndex.begin();
    while (itLocal != indexDefines_.end() && itInTable != inTableIndex.end()) {
        if (itLocal->first != itInTable->first || itLocal->second != itInTable->second) {
            return -E_RELATIONAL_TABLE_COMPATIBLE;
        }
        itLocal++;
        itInTable++;
    }
    return (itLocal == indexDefines_.end() && itInTable == inTableIndex.end()) ? -E_RELATIONAL_TABLE_EQUAL :
        -E_RELATIONAL_TABLE_COMPATIBLE;
}

std::string TableInfo::ToTableInfoString() const
{
    std::string attrStr;
    attrStr += "{";
    attrStr += R"("NAME": ")" + tableName_ + "\",";
    AddFieldDefineString(attrStr);
    attrStr += R"("AUTOINCREMENT": )";
    if (autoInc_) {
        attrStr += "true,";
    } else {
        attrStr += "false,";
    }
    if (!primaryKey_.empty()) {
        attrStr += R"("PRIMARY_KEY": ")" + primaryKey_ + "\"";
    }
    AddIndexDefineString(attrStr);
    attrStr += "}";
    return attrStr;
}

std::map<FieldPath, SchemaAttribute> TableInfo::GetSchemaDefine() const
{
    std::map<FieldPath, SchemaAttribute> schemaDefine;
    for (const auto &[fieldName, fieldInfo] : GetFields()) {
        FieldValue defaultValue;
        defaultValue.stringValue = fieldInfo.GetDefaultValue();
        schemaDefine[std::vector { fieldName }] = SchemaAttribute {
            .type = FieldType::LEAF_FIELD_NULL,     // For relational schema, the json field type is unimportant.
            .isIndexable = true,                    // For relational schema, all field is indexable.
            .hasNotNullConstraint = fieldInfo.IsNotNull(),
            .hasDefaultValue = fieldInfo.HasDefaultValue(),
            .defaultValue = defaultValue,
            .customFieldType = {}
        };
    }
    return schemaDefine;
}

namespace {
    const std::string MAGIC = "relational_opinion";
}

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
    schemaString_ += R"("SCHEMA_VERSION":"2.0",)";
    schemaString_ += R"("SCHEMA_TYPE":"RELATIVE",)";
    schemaString_ += R"("TABLES":[)";
    for (auto it = tables_.begin(); it != tables_.end(); it++) {
        if (it != tables_.begin()) {
            schemaString_ += ",";
        }
        schemaString_ += it->second.ToTableInfoString();
    }
    schemaString_ += R"(])";
    schemaString_ += "}";
}

void RelationalSchemaObject::AddRelationalTable(const TableInfo &tb)
{
    tables_[tb.GetTableName()] = tb;
    isValid_ = true;
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
        LOGE("[RelationalSchema][Parse] Expect %s fieldType %d but: %d.", fieldName.c_str(), expectType, fieldType);
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
    return ParseCheckSchemaTableDefine(inJsonObject);
}

int RelationalSchemaObject::ParseCheckSchemaVersion(const JsonObject &inJsonObject)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, SchemaConstant::KEYWORD_SCHEMA_VERSION,
        FieldType::LEAF_FIELD_STRING, true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }

    if (SchemaUtils::Strip(fieldValue.stringValue) != SchemaConstant::SCHEMA_SUPPORT_VERSION_V2) {
        LOGE("[RelationalSchema][Parse] Unexpected SCHEMA_VERSION=%s.", fieldValue.stringValue.c_str());
        return -E_SCHEMA_PARSE_FAIL;
    }
    schemaVersion_ = SchemaConstant::SCHEMA_SUPPORT_VERSION_V2;
    return E_OK;
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
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, "PRIMARY_KEY", FieldType::LEAF_FIELD_STRING, false, fieldValue);
    if (errCode == E_OK) {
        resultTable.SetPrimaryKey(fieldValue.stringValue);
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
}
#endif