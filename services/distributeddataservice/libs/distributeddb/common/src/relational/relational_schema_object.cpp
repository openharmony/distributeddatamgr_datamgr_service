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
        attrStr += "[\"";
        for (auto itField = (*itUniqueDefine).begin(); itField != (*itUniqueDefine).end(); ++itField) {
            attrStr += *itField;
            if (itField != (*itUniqueDefine).end() - 1) {
                attrStr += "\",\"";
            }
        }
        attrStr += "\"]";
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

const std::string &TableInfo::GetDevId() const
{
    return devId_;
}

void TableInfo::SetDevId(const std::string &devId)
{
    devId_ = devId;
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

namespace {
    std::string VectorJoin(const CompositeFields &fields, char con)
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
    for (const auto &it : fields_) {
        jsonField.stringValue = it.second.ToAttributeString();
        tableJson.InsertField(FieldPath { "DEFINE", it.first }, FieldType::LEAF_FIELD_STRING, jsonField);
    }
    for (const auto &it : uniqueDefines_) {
        jsonField.stringValue = VectorJoin(it, ',');
    }
    return tableJson;
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
    AddUniqueDefineString(attrStr);
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

uint32_t RelationalSyncOpinion::CalculateParcelLen(uint32_t softWareVersion) const
{
    uint64_t len = Parcel::GetStringLen(MAGIC);
    len += Parcel::GetUInt32Len();
    len += Parcel::GetUInt32Len();
    len = Parcel::GetEightByteAlign(len);
    for (const auto &it : opinions_) {
        len += Parcel::GetStringLen(it.first);
        len += Parcel::GetUInt32Len();
        len += Parcel::GetUInt32Len();
        len = Parcel::GetEightByteAlign(len);
    }
    if (len > UINT32_MAX) {
        return 0;
    }
    return static_cast<uint32_t>(len);
}

int RelationalSyncOpinion::SerializeData(Parcel &parcel, uint32_t softWareVersion) const
{
    (void)parcel.WriteString(MAGIC);
    (void)parcel.WriteUInt32(SYNC_OPINION_VERSION);
    (void)parcel.WriteUInt32(static_cast<uint32_t>(opinions_.size()));
    (void)parcel.EightByteAlign();
    for (const auto &it : opinions_) {
        (void)parcel.WriteString(it.first);
        (void)parcel.WriteUInt32(it.second.permitSync);
        (void)parcel.WriteUInt32(it.second.requirePeerConvert);
        (void)parcel.EightByteAlign();
    }
    return parcel.IsError() ? -E_INVALID_ARGS : E_OK;
}

int RelationalSyncOpinion::DeserializeData(Parcel &parcel, RelationalSyncOpinion &opinion)
{
    if (!parcel.IsContinueRead()) {
        return E_OK;
    }
    std::string magicStr;
    (void)parcel.ReadString(magicStr);
    if (magicStr != MAGIC) {
        LOGE("Deserialize sync opinion failed while read MAGIC string [%s]", magicStr.c_str());
        return -E_INVALID_ARGS;
    }
    uint32_t version;
    (void)parcel.ReadUInt32(version);
    if (version != SYNC_OPINION_VERSION) {
        LOGE("Not support sync opinion version: %u", version);
        return -E_NOT_SUPPORT;
    }
    uint32_t opinionSize;
    (void)parcel.ReadUInt32(opinionSize);
    (void)parcel.EightByteAlign();
    for (uint32_t i = 0; i < opinionSize; i++) {
        std::string tableName;
        SyncOpinion tableOpinion;
        (void)parcel.ReadString(tableName);
        uint32_t permitSync;
        (void)parcel.ReadUInt32(permitSync);
        tableOpinion.permitSync = static_cast<bool>(permitSync);
        uint32_t requirePeerConvert;
        (void)parcel.ReadUInt32(requirePeerConvert);
        tableOpinion.requirePeerConvert = static_cast<bool>(requirePeerConvert);
        (void)parcel.EightByteAlign();
        opinion.AddSyncOpinion(tableName, tableOpinion);
    }
    return parcel.IsError() ? -E_INVALID_ARGS : E_OK;
}

SyncOpinion RelationalSyncOpinion::GetTableOpinion(const std::string &tableName) const
{
    auto it = opinions_.find(tableName);
    if (it == opinions_.end()) {
        return {};
    }
    return it->second;
}

const std::map<std::string, SyncOpinion> &RelationalSyncOpinion::GetOpinions() const
{
    return opinions_;
}

void RelationalSyncOpinion::AddSyncOpinion(const std::string &tableName, const SyncOpinion &opinion)
{
    opinions_[tableName] = opinion;
}

SyncStrategy RelationalSyncStrategy::GetTableStrategy(const std::string &tableName) const
{
    auto it = strategies_.find(tableName);
    if (it == strategies_.end()) {
        return {};
    }
    return it->second;
}

void RelationalSyncStrategy::AddSyncStrategy(const std::string &tableName, const SyncStrategy &strategy)
{
    strategies_[tableName] = strategy;
}

const std::map<std::string, SyncStrategy> &RelationalSyncStrategy::GetStrategies() const
{
    return strategies_;
}

RelationalSyncOpinion RelationalSchemaObject::MakeLocalSyncOpinion(const RelationalSchemaObject &localSchema,
    const std::string &remoteSchema, uint8_t remoteSchemaType)
{
    SchemaType localType = localSchema.GetSchemaType();
    SchemaType remoteType = ReadSchemaType(remoteSchemaType);

    if (remoteType == SchemaType::UNRECOGNIZED) {
        LOGW("[RelationalSchema][opinion] Remote schema type %d is unrecognized.", remoteSchemaType);
        return {};
    }

    if (remoteType != SchemaType::RELATIVE) {
        LOGW("[RelationalSchema][opinion] Not support sync with schema type: local-type=[%s] remote-type=[%s]",
            SchemaUtils::SchemaTypeString(localType).c_str(), SchemaUtils::SchemaTypeString(remoteType).c_str());
        return {};
    }

    if (!localSchema.IsSchemaValid()) {
        LOGW("[RelationalSchema][opinion] Local schema is not valid");
        return {};
    }

    RelationalSchemaObject remoteSchemaObj;
    int errCode = remoteSchemaObj.ParseFromSchemaString(remoteSchema);
    if (errCode != E_OK) {
        LOGW("[RelationalSchema][opinion] Parse remote schema failed %d, remote schema type %s", errCode,
            SchemaUtils::SchemaTypeString(remoteType).c_str());
        return {};
    }

    RelationalSyncOpinion opinion;
    for (const auto &it : localSchema.GetTables()) {
        if (remoteSchemaObj.GetTable(it.first).GetTableName() != it.first) {
            LOGW("[RelationalSchema][opinion] Table was missing in remote schema");
            continue;
        }
        // remote table is compatible(equal or upgrade) based on local table, permit sync and don't need check
        errCode = it.second.CompareWithTable(remoteSchemaObj.GetTable(it.first));
        if (errCode != -E_RELATIONAL_TABLE_INCOMPATIBLE) {
            opinion.AddSyncOpinion(it.first, {true, false, false});
            continue;
        }
        // local table is compatible upgrade based on remote table, permit sync and need check
        errCode = remoteSchemaObj.GetTable(it.first).CompareWithTable(it.second);
        if (errCode != -E_RELATIONAL_TABLE_INCOMPATIBLE) {
            opinion.AddSyncOpinion(it.first, {true, false, true});
            continue;
        }
        // local table is incompatible with remote table mutually, don't permit sync and need check
        LOGW("[RelationalSchema][opinion] Local table is incompatible with remote table mutually.");
        opinion.AddSyncOpinion(it.first, {false, true, true});
    }

    return opinion;
}

RelationalSyncStrategy RelationalSchemaObject::ConcludeSyncStrategy(const RelationalSyncOpinion &localOpinion,
    const RelationalSyncOpinion &remoteOpinion)
{
    RelationalSyncStrategy syncStrategy;
    for (const auto &itLocal : localOpinion.GetOpinions()) {
        if (remoteOpinion.GetOpinions().find(itLocal.first) == remoteOpinion.GetOpinions().end()) {
            LOGW("[RelationalSchema][Strategy] Table opinion is not found from remote.");
            continue;
        }

        SyncStrategy strategy;
        SyncOpinion localTableOpinion = itLocal.second;
        SyncOpinion remoteTableOpinion = remoteOpinion.GetTableOpinion(itLocal.first);

        strategy.permitSync = (localTableOpinion.permitSync || remoteTableOpinion.permitSync);

        bool convertConflict = (localTableOpinion.requirePeerConvert && remoteTableOpinion.requirePeerConvert);
        if (convertConflict) {
            // Should not both need peer convert
            strategy.permitSync = false;
        }
        // No peer convert required means local side has convert capability, convert locally takes precedence
        strategy.convertOnSend = (!localTableOpinion.requirePeerConvert);
        strategy.convertOnReceive = remoteTableOpinion.requirePeerConvert;

        strategy.checkOnReceive = localTableOpinion.checkOnReceive;
        LOGI("[RelationalSchema][Strategy] PermitSync=%d, SendConvert=%d, ReceiveConvert=%d, ReceiveCheck=%d.",
            strategy.permitSync, strategy.convertOnSend, strategy.convertOnReceive, strategy.checkOnReceive);
        syncStrategy.AddSyncStrategy(itLocal.first, strategy);
    }

    return syncStrategy;
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

    if (inSchemaString.size() > SchemaConstant::SCHEMA_STRING_SIZE_LIMIT) {
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
    int errCode = ParseCheckSchemaVersionMode(inJsonObject);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = ParseCheckSchemaType(inJsonObject);
    if (errCode != E_OK) {
        return errCode;
    }
    return ParseCheckSchemaTableDefine(inJsonObject);
}

int RelationalSchemaObject::ParseCheckSchemaVersionMode(const JsonObject &inJsonObject)
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
    errCode = ParseCheckTableUnique(inJsonObject, resultTable);
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

int RelationalSchemaObject::ParseCheckTableUnique(const JsonObject &inJsonObject, TableInfo &resultTable)
{
    if (!inJsonObject.IsFieldPathExist(FieldPath {"UNIQUE"})) { // UNIQUE is not necessary
        return E_OK;
    }
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