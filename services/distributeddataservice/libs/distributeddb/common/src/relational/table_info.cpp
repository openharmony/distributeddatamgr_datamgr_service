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
#include "table_info.h"

#include <algorithm>

#include "db_common.h"
#include "db_errno.h"
#include "log_print.h"

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

namespace {
inline uint32_t AffinityParrernHex(const std::string &ss)
{
    uint32_t res = 0;
    for (const auto &c : ss) {
        res = (res << 8) + c; // 8: shift length
    }
    return res;
}

static uint32_t affinityTable[] = {
    AffinityParrernHex("char"), AffinityParrernHex("clob"), AffinityParrernHex("text"),
    AffinityParrernHex("blob"), AffinityParrernHex("real"), AffinityParrernHex("floa"),
    AffinityParrernHex("doub"), AffinityParrernHex("int")
};

enum AffinityPattern : uint32_t {
    AFFINITY_CHAR,
    AFFINITY_CLOB,
    AFFINITY_TEXT,
    AFFINITY_BLOB,
    AFFINITY_REAL,
    AFFINITY_FLOA,
    AFFINITY_DOUB,
    AFFINITY_INT,
};
}

static StorageType AffinityType(const std::string &dataType)
{
    StorageType type = StorageType::STORAGE_TYPE_NULL;
    uint32_t hex = 0;
    for (uint32_t i = 0; i < dataType.length(); i++) {
        hex = (hex << 8) + (std::tolower(dataType[i])); // 8: shift length
        if (hex == affinityTable[AFFINITY_CHAR]) {
            type = StorageType::STORAGE_TYPE_TEXT;
        } else if (hex == affinityTable[AFFINITY_CLOB]) {
            type = StorageType::STORAGE_TYPE_TEXT;
        } else if (hex == affinityTable[AFFINITY_TEXT]) {
            type = StorageType::STORAGE_TYPE_TEXT;
        } else if (hex == affinityTable[AFFINITY_BLOB] && (type == StorageType::STORAGE_TYPE_NULL ||
            type == StorageType::STORAGE_TYPE_REAL)) {
            type = StorageType::STORAGE_TYPE_BLOB;
        } else if (hex == affinityTable[AFFINITY_REAL] && type == StorageType::STORAGE_TYPE_NULL) {
            type = StorageType::STORAGE_TYPE_REAL;
        } else if (hex == affinityTable[AFFINITY_FLOA] && type == StorageType::STORAGE_TYPE_NULL) {
            type = StorageType::STORAGE_TYPE_REAL;
        } else if (hex == affinityTable[AFFINITY_DOUB] && type == StorageType::STORAGE_TYPE_NULL) {
            type = StorageType::STORAGE_TYPE_REAL;
        } else if ((hex & 0x00ffffff) == affinityTable[AFFINITY_INT]) { // 0x00ffffff: mask for 3 byte
            type = StorageType::STORAGE_TYPE_INTEGER;
        }
    }
    return type;
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

int FieldInfo::CompareWithField(const FieldInfo &inField, bool isLite) const
{
    if (fieldName_ != inField.GetFieldName() || isNotNull_ != inField.IsNotNull()) {
        return false;
    }
    if (isLite) {
        if (storageType_ != inField.GetStorageType()) {
            return false;
        }
    } else {
        if (dataType_ != inField.GetDataType()) {
            return false;
        }
    }
    if (hasDefaultValue_ && inField.HasDefaultValue()) {
        // lite schema only uses NULL as default value
        return (isLite && defaultValue_ == "NULL") || defaultValue_ == inField.GetDefaultValue();
    }
    return hasDefaultValue_ == inField.HasDefaultValue();
}

bool FieldInfo::Empty() const
{
    return fieldName_.empty();
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

void TableInfo::SetCreateTableSql(const std::string &sql)
{
    sql_ = sql;
    for (auto &c : sql_) {
        c = static_cast<char>(std::toupper(c));
    }
    if (DBCommon::HasConstraint(DBCommon::TrimSpace(sql_), "AUTOINCREMENT", " ", " ,")) {
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

const std::vector<CompositeFields> &TableInfo::GetUniqueDefine() const
{
    return uniqueDefines_;
}

void TableInfo::AddIndexDefine(const std::string &indexName, const CompositeFields &indexDefine)
{
    indexDefines_[indexName] = indexDefine;
}

void TableInfo::SetUniqueDefine(const std::vector<CompositeFields> &uniqueDefine)
{
    uniqueDefines_ = uniqueDefine;
    std::sort(uniqueDefines_.begin(), uniqueDefines_.end());
}

const std::map<int, FieldName> &TableInfo::GetPrimaryKey() const
{
    return primaryKey_;
}

CompositeFields TableInfo::GetIdentifyKey() const
{
    if (primaryKey_.size() == 1 && primaryKey_.at(0) == "rowid") {
        if (!uniqueDefines_.empty()) {
            return uniqueDefines_.at(0);
        }
    }
    CompositeFields key;
    for (const auto &it : primaryKey_) {
        key.emplace_back(it.second);
    }
    return key;
}

void TableInfo::SetPrimaryKey(const std::map<int, FieldName> &key)
{
    primaryKey_ = key;
}

void TableInfo::SetPrimaryKey(const FieldName &fieldName, int keyIndex)
{
    if (keyIndex <= 0) {
        LOGW("Set primary key index %d less than or equal to 0", keyIndex);
        return;
    }

    primaryKey_[keyIndex - 1] = fieldName;
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

void TableInfo::AddUniqueDefineString(std::string &attrStr) const
{
    if (uniqueDefines_.empty()) {
        return;
    }

    attrStr += R"("UNIQUE":[)";
    for (const auto &unique : uniqueDefines_) {
        attrStr += "[";
        for (const auto &it : unique) {
            attrStr += "\"" + it + "\",";
        }
        attrStr.pop_back();
        attrStr += "],";
    }
    attrStr.pop_back();
    attrStr += "],";
}

int TableInfo::CompareWithTable(const TableInfo &inTableInfo, const std::string &schemaVersion) const
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

    if (schemaVersion == SchemaConstant::SCHEMA_SUPPORT_VERSION_V2_1) {
        int uniqueCompareResult = CompareWithTableUnique(inTableInfo.GetUniqueDefine());
        if (uniqueCompareResult == -E_RELATIONAL_TABLE_INCOMPATIBLE) {
            LOGW("[Relational][Compare] Compare table unique with in table, %d", fieldCompareResult);
            return -E_RELATIONAL_TABLE_INCOMPATIBLE;
        }

        if (autoInc_ != inTableInfo.GetAutoIncrement()) {
            LOGW("[Relational][Compare] Compare table auto increment with in table");
            return -E_RELATIONAL_TABLE_INCOMPATIBLE;
        }
    }

    int indexCompareResult = CompareWithTableIndex(inTableInfo.GetIndexDefine());
    return (fieldCompareResult == -E_RELATIONAL_TABLE_EQUAL) ? indexCompareResult : fieldCompareResult;
}

int TableInfo::CompareWithTableFields(const std::map<std::string, FieldInfo> &inTableFields, bool isLite) const
{
    auto itLocal = fields_.begin();
    auto itInTable = inTableFields.begin();
    int errCode = -E_RELATIONAL_TABLE_EQUAL;
    while (itLocal != fields_.end() && itInTable != inTableFields.end()) {
        LOGD("--> field name %s", itLocal->first.c_str());
        if (itLocal->first == itInTable->first) { // Same field
            if (!itLocal->second.CompareWithField(itInTable->second, isLite)) { // Compare field
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

int TableInfo::CompareWithTableUnique(const std::vector<CompositeFields> &inTableUnique) const
{
    if (uniqueDefines_.size() != inTableUnique.size()) {
        return -E_RELATIONAL_TABLE_INCOMPATIBLE;
    }

    auto itLocal = uniqueDefines_.begin();
    auto itInTable = inTableUnique.begin();
    while (itLocal != uniqueDefines_.end()) {
        if (*itLocal != *itInTable) {
            return -E_RELATIONAL_TABLE_INCOMPATIBLE;
        }
        itLocal++;
        itInTable++;
    }
    return -E_RELATIONAL_TABLE_EQUAL;
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
void Difference(const std::map<std::string, FieldInfo> &first, const std::map<std::string, FieldInfo> &second,
    std::map<std::string, FieldInfo> &orphanFst, std::map<std::string, FieldInfo> &orphanSnd,
    std::map<std::string, FieldInfo> &bothAppear)
{
    auto itFirst = first.begin();
    auto itSecond = second.begin();
    while (itFirst != first.end()) {
        if (itSecond == second.end()) {
            break;
        }
        if (itFirst->first == itSecond->first) {
            bothAppear.insert(*itFirst);
            itFirst++;
            itSecond++;
            continue;
        } else if (itFirst->first < itSecond->first) {
            orphanFst.insert(*itFirst++);
        } else {
            orphanSnd.insert(*itSecond++);
        }
    }

    while (itFirst != first.end()) {
        orphanFst.insert(*itFirst++);
    }

    while (itSecond != second.end()) {
        orphanSnd.insert(*itSecond++);
    }
}
}

int TableInfo::CompareWithLiteTableFields(const std::map<std::string, FieldInfo> &liteTableFields) const
{
    std::map<std::string, FieldInfo> orphanLocal;
    std::map<std::string, FieldInfo> orphanLite;
    std::map<std::string, FieldInfo> bothAppear;
    Difference(fields_, liteTableFields, orphanLocal, orphanLite, bothAppear);

    if (!orphanLocal.empty() && !orphanLite.empty()) {
        LOGE("[Relational][Compare] Only one side should have upgrade fields");
        return -E_RELATIONAL_TABLE_INCOMPATIBLE;
    }

    for (const auto &it : bothAppear) {
        if (!it.second.CompareWithField(liteTableFields.at(it.first), true)) {
            LOGE("[Relational][Compare] field is incompatible");
            return -E_RELATIONAL_TABLE_INCOMPATIBLE;
        }
    }

    for (const auto &it : orphanLocal) {
        if (it.second.IsNotNull() && !it.second.HasDefaultValue()) {
            LOGE("[Relational][Compare] field is upgrade incompatible");
            return -E_RELATIONAL_TABLE_INCOMPATIBLE;
        }
    }

    for (const auto &it : orphanLite) {
        if (it.second.IsNotNull() && !it.second.HasDefaultValue()) {
            LOGE("[Relational][Compare] field is upgrade incompatible");
            return -E_RELATIONAL_TABLE_INCOMPATIBLE;
        }
    }

    return E_OK;
}

int TableInfo::CompareWithLiteSchemaTable(const TableInfo &liteTableInfo) const
{
    LOGE("--> check table primary key, %d %d %d", primaryKey_.empty(), liteTableInfo.GetPrimaryKey().empty(), autoInc_);

    if (!liteTableInfo.GetPrimaryKey().empty() && (primaryKey_.at(0) != "rowid") &&
        liteTableInfo.GetPrimaryKey() != primaryKey_) {
        LOGE("[Relational][Compare] Table primary key is not same");
        return -E_RELATIONAL_TABLE_INCOMPATIBLE;
    }
    if (!liteTableInfo.GetPrimaryKey().empty() && (primaryKey_.at(0) == "rowid")) {
        LOGE("[Relational][Compare] Table primary key is not same");
        return -E_RELATIONAL_TABLE_INCOMPATIBLE;
    }
    if ((liteTableInfo.GetPrimaryKey().empty() && (primaryKey_.at(0) != "rowid") && !autoInc_)) {
        LOGE("[Relational][Compare] Table primary key is not same");
        return -E_RELATIONAL_TABLE_INCOMPATIBLE;
    }

    return CompareWithLiteTableFields(liteTableInfo.GetFields());
}

std::string TableInfo::ToTableInfoString(const std::string &schemaVersion) const
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
    if (primaryKey_.size() == 1 && schemaVersion == SchemaConstant::SCHEMA_SUPPORT_VERSION_V2) {
        attrStr += R"("PRIMARY_KEY": ")" + primaryKey_.at(0) + "\"";
    } else {
        if (!primaryKey_.empty()) {
            std::string primaryKey;
            for (const auto &item : primaryKey_) {
                primaryKey += "\"" + item.second + "\",";
            }
            primaryKey.pop_back(); // remove the last comma
            attrStr += R"("PRIMARY_KEY": [)" + primaryKey + "]";
        }
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

void TableInfo::SetTableId(int id)
{
    id_ = id;
}

int TableInfo::GetTableId() const
{
    return id_;
}

bool TableInfo::Empty() const
{
    return tableName_.empty() || fields_.empty();
}
} // namespace DistributeDB