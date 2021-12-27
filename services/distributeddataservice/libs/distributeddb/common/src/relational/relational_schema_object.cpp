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
#include <algorithm>
#include <schema_utils.h>

#include "json_object.h"
#include "relational_schema_object.h"

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

// return field define string like ("fieldName": "MY INT(21), NOT NULL, DEFAULT 123")
std::string FieldInfo::ToAttributeString() const
{
    std::string attrStr = "\"" + fieldName_ + "\": {";
    attrStr += "\"TYPE\":\"" + dataType_ + "\",";
    attrStr += "\"NOT_NULL\":" + std::string(isNotNull_ ? "true" : "false") + ",";
    if (hasDefaultValue_) {
        attrStr += "\"DEFAULT\":\"" + defaultValue_ + "\"";
    }
    attrStr += "}";
    return attrStr;
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

void TableInfo::AddField(const FieldInfo &field)
{
    fields_[field.GetFieldName()] = field;
}

const std::vector<CompositeFields> &TableInfo::GetUniqueDefine() const
{
    return uniqueDefines_;
}

void TableInfo::AddUniqueDefine(const CompositeFields &uniqueDefine)
{
    uniqueDefines_.push_back(uniqueDefine);
}

void TableInfo::SetUniqueDefines(const std::vector<CompositeFields> &uniqueDefines)
{
    uniqueDefines_ = uniqueDefines;
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

void TableInfo::AddTrigger(const std::string &triggerName)
{
    triggers_.push_back(triggerName);
}

const std::vector<std::string> &TableInfo::GetTriggers() const
{
    return triggers_;
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

void TableInfo::AddUniqueDefineString(std::string &attrStr) const
{
    if (uniqueDefines_.empty()) {
        return;
    }
    attrStr += R"("UNIQUE": [)";
    for (auto itUniqueDefine = uniqueDefines_.begin(); itUniqueDefine != uniqueDefines_.end(); ++itUniqueDefine) {
        attrStr += "\"";
        for (auto itField = (*itUniqueDefine).begin(); itField != (*itUniqueDefine).end(); ++itField) {
            attrStr += *itField;
            if (itField != (*itUniqueDefine).end() - 1) {
                attrStr += ", ";
            }
        }
        attrStr += "\"";
        if (itUniqueDefine != uniqueDefines_.end() - 1) {
            attrStr += ",";
        }
    }
    attrStr += "],";
}

void TableInfo::AddIndexDefineString(std::string &attrStr) const
{
    if (indexDefines_.empty()) {
        return;
    }
    attrStr += R"("INDEX": {)";
    for (auto itIndexDefine = indexDefines_.begin(); itIndexDefine != indexDefines_.end(); ++itIndexDefine) {
        attrStr += "\"" + (*itIndexDefine).first + "\": \"";
        for (auto itField = itIndexDefine->second.begin(); itField != itIndexDefine->second.end(); ++itField) {
            attrStr += *itField;
            if (itField != itIndexDefine->second.end() - 1) {
                attrStr += ",";
            }
        }
        attrStr += "\"";
        if (itIndexDefine != std::prev(indexDefines_.end(), 1)) {
            attrStr += ",";
        }
    }
    attrStr += "}";
}

const std::string &TableInfo::GetDevId() const
{
    return devId_;
}

void TableInfo::SetDevId(const std::string &devId)
{
    devId_ = devId;
}

namespace {
    std::string VectorJoin(const CompositeFields& fields, char con)
    {
        std::string res;
        auto it = fields.begin();
        res += *it++;
        for (; it != fields.end(); ++it) {
            res += con + *it;
        }
        return res;
    }
}

JsonObject TableInfo::ToJsonObject() const
{
    FieldValue jsonField;
    JsonObject tableJson;
    jsonField.stringValue = tableName_;
    tableJson.InsertField(FieldPath { "NAME" }, FieldType::LEAF_FIELD_STRING, jsonField);
    jsonField.boolValue = autoInc_;
    tableJson.InsertField(FieldPath { "AUTOINCREMENT" }, FieldType::LEAF_FIELD_BOOL, jsonField);
    jsonField.stringValue = primaryKey_;
    tableJson.InsertField(FieldPath { "PRIMARY_KEY" }, FieldType::LEAF_FIELD_STRING, jsonField);
    for (const auto& it : fields_) {
        jsonField.stringValue = it.second.ToAttributeString();
        tableJson.InsertField(FieldPath { "DEFINE", it.first }, FieldType::LEAF_FIELD_STRING, jsonField);
    }
    for (const auto& it : uniqueDefines_) {
        jsonField.stringValue = VectorJoin(it, ',');
    }
    return tableJson;
}

std::string TableInfo::ToTableInfoString() const
{
    std::string attrStr;
    attrStr += R"("NAME": ")" + tableName_ + "\",";
    AddFieldDefineString(attrStr);
    attrStr += R"("AUTOINCREMENT": )";
    if (autoInc_) {
        attrStr += "true,";
    } else {
        attrStr += "false,";
    }
    AddUniqueDefineString(attrStr);
    if (!primaryKey_.empty()) {
        attrStr += R"("PRIMARY_KEY": ")" + primaryKey_ + "\",";
    }
    AddIndexDefineString(attrStr);
    return attrStr;
}

int RelationalSyncOpinion::CalculateParcelLen(uint32_t softWareVersion) const
{
    return E_OK;
}

int RelationalSyncOpinion::Serialize(Parcel &parcel, uint32_t softWareVersion) const
{
    return E_OK;
}

int RelationalSyncOpinion::Deserialization(const Parcel &parcel)
{
    return E_OK;
}

const SyncOpinion &RelationalSyncOpinion::GetTableOpinion(const std::string& tableName) const
{
    return opinions_.at(tableName);
}

void RelationalSyncOpinion::AddSyncOpinion(const std::string &tableName, const SyncOpinion &opinion)
{
    opinions_[tableName] = opinion;
}

const SyncStrategy &RelationalSyncStrategy::GetTableStrategy(const std::string &tableName) const
{
    return strategies_.at(tableName);
}

void RelationalSyncStrategy::AddSyncStrategy(const std::string &tableName, const SyncStrategy &strategy)
{
    strategies_[tableName] = strategy;
}

RelationalSyncOpinion RelationalSchemaObject::MakeLocalSyncOpinion(
    const RelationalSchemaObject &localSchema, const std::string &remoteSchema, uint8_t remoteSchemaType)
{
    return RelationalSyncOpinion();
}

RelationalSyncStrategy RelationalSchemaObject::ConcludeSyncStrategy(
    const RelationalSyncOpinion &localOpinion, const RelationalSyncOpinion &remoteOpinion)
{
    return RelationalSyncStrategy();
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

    if (inSchemaString.size() > SCHEMA_STRING_SIZE_LIMIT) {
        LOGE("[RelationalSchema][Parse] SchemaSize=%zu Too Large.", inSchemaString.size());
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

void RelationalSchemaObject::AddRelationalTable(const TableInfo& tb)
{
    tables_[tb.GetTableName()] = tb;
}

const std::map<std::string, TableInfo> &RelationalSchemaObject::GetTables() const
{
    return tables_;
}

TableInfo RelationalSchemaObject::GetTable(const std::string& tableName) const
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
        LOGW("[RelationalSchema][Parse] Get schema %s not exist. isNecessary: %d", fieldName.c_str(), isNecessary);
        return isNecessary ? -E_SCHEMA_PARSE_FAIL : -E_NOT_FOUND;
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
    int errCode = ParseCheckSchemaVersionMode(inJsonObject);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckSchemaType(inJsonObject);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckSchemaTableDefine(inJsonObject);
    if (errCode != E_OK) {
        return errCode;
    }
    return E_OK;
}

int RelationalSchemaObject::ParseCheckSchemaVersionMode(const JsonObject &inJsonObject)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, KEYWORD_SCHEMA_VERSION, FieldType::LEAF_FIELD_STRING,
        true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }

    if (SchemaUtils::Strip(fieldValue.stringValue) != SCHEMA_SUPPORT_VERSION_V2) {
        LOGE("[RelationalSchema][Parse] Unexpected SCHEMA_VERSION=%s.", fieldValue.stringValue.c_str());
        return -E_SCHEMA_PARSE_FAIL;
    }
    schemaVersion_ = SCHEMA_SUPPORT_VERSION_V2;
    return E_OK;
}

int RelationalSchemaObject::ParseCheckSchemaType(const JsonObject &inJsonObject)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, KEYWORD_SCHEMA_TYPE, FieldType::LEAF_FIELD_STRING,
        true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }

    if (SchemaUtils::Strip(fieldValue.stringValue) != KEYWORD_TYPE_RELATIVE) {
        LOGE("[RelationalSchema][Parse] Unexpected SCHEMA_TYPE=%s.", fieldValue.stringValue.c_str());
        return -E_SCHEMA_PARSE_FAIL;
    }
    schemaType_ = SchemaType::RELATIVE;
    return E_OK;
}

int RelationalSchemaObject::ParseCheckSchemaTableDefine(const JsonObject &inJsonObject)
{
    FieldType fieldType;
    int errCode = inJsonObject.GetFieldTypeByFieldPath(FieldPath {KEYWORD_SCHEMA_TABLE}, fieldType);
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
    errCode = inJsonObject.GetObjectArrayByFieldPath(FieldPath{KEYWORD_SCHEMA_TABLE}, tables);
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
    errCode = ParseCheckTableUnique(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckTablePrimaryKey(inJsonObject, resultTable);
    if (errCode != E_OK) {
        return errCode;
    }
    return ParseCheckTableIndex(inJsonObject, resultTable);
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
            LOGE("[RelationalSchema][Parse] Get table field object failed. %s", errCode);
            return errCode;
        }

        FieldInfo fieldInfo;
        fieldInfo.SetFieldName(field.first[0]); // 0 : first element in path
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
    FieldInfo &table)
{
    FieldValue fieldValue;
    int errCode = GetMemberFromJsonObject(inJsonObject, "TYPE", FieldType::LEAF_FIELD_STRING, true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }
    table.SetDataType(fieldValue.stringValue);

    errCode = GetMemberFromJsonObject(inJsonObject, "NOT_NULL", FieldType::LEAF_FIELD_BOOL, true, fieldValue);
    if (errCode != E_OK) {
        return errCode;
    }
    table.SetNotNull(fieldValue.boolValue);

    errCode = GetMemberFromJsonObject(inJsonObject, "DEFAULT", FieldType::LEAF_FIELD_STRING, false, fieldValue);
    if (errCode == E_OK) {
        table.SetDefaultValue(fieldValue.stringValue);
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

int RelationalSchemaObject::ParseCheckTableUnique(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    std::vector<CompositeFields> uniqueArray;
    int errCode = inJsonObject.GetArrayContentOfStringOrStringArray(FieldPath {"UNIQUE"}, uniqueArray);
    if (errCode != E_OK) {
        LOGE("[RelationalSchema][Parse] Get unique array failed: %d.", errCode);
        return -E_SCHEMA_PARSE_FAIL;
    }
    resultTable.SetUniqueDefines(uniqueArray);
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