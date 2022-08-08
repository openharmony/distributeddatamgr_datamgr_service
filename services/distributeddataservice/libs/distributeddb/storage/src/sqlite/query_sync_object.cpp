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

#include "query_sync_object.h"

#include "db_errno.h"
#include "log_print.h"
#include "db_common.h"
#include "version.h"

namespace DistributedDB {
namespace {
const std::string MAGIC = "remote query";

int SerializeDataObjNode(Parcel &parcel, const QueryObjNode &objNode)
{
    if (objNode.operFlag == QueryObjType::OPER_ILLEGAL) {
        return -E_INVALID_QUERY_FORMAT;
    }
    (void)parcel.WriteUInt32(static_cast<uint32_t>(objNode.operFlag));
    parcel.EightByteAlign();
    (void)parcel.WriteString(objNode.fieldName);
    (void)parcel.WriteInt(static_cast<int32_t>(objNode.type));
    (void)parcel.WriteUInt32(objNode.fieldValue.size());

    for (const FieldValue &value : objNode.fieldValue) {
        (void)parcel.WriteString(value.stringValue);

        // string may not closely arranged continuously
        // longValue is maximum length in union
        (void)parcel.WriteInt64(value.longValue);
    }
    if (parcel.IsError()) {
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

int DeSerializeDataObjNode(Parcel &parcel, QueryObjNode &objNode)
{
    uint32_t readOperFlag = 0;
    (void)parcel.ReadUInt32(readOperFlag);
    objNode.operFlag = static_cast<QueryObjType>(readOperFlag);
    parcel.EightByteAlign();

    (void)parcel.ReadString(objNode.fieldName);

    int readInt = -1;
    (void)parcel.ReadInt(readInt);
    objNode.type = static_cast<QueryValueType>(readInt);

    uint32_t valueSize = 0;
    (void)parcel.ReadUInt32(valueSize);
    if (parcel.IsError()) {
        return -E_INVALID_ARGS;
    }

    for (size_t i = 0; i < valueSize; i++) {
        FieldValue value;
        (void)parcel.ReadString(value.stringValue);

        (void)parcel.ReadInt64(value.longValue);
        if (parcel.IsError()) {
            return -E_INVALID_ARGS;
        }
        objNode.fieldValue.push_back(value);
    }
    return E_OK;
}
}

QuerySyncObject::QuerySyncObject()
{}

QuerySyncObject::QuerySyncObject(const std::list<QueryObjNode> &queryObjNodes, const std::vector<uint8_t> &prefixKey,
    const std::set<Key> &keys)
    : QueryObject(queryObjNodes, prefixKey, keys)
{}

QuerySyncObject::QuerySyncObject(const Query &query)
    : QueryObject(query)
{}

QuerySyncObject::~QuerySyncObject()
{}

uint32_t QuerySyncObject::GetVersion() const
{
    uint32_t version = QUERY_SYNC_OBJECT_VERSION_0;
    if (isTableNameSpecified_ || !keys_.empty()) {
        version = QUERY_SYNC_OBJECT_VERSION_1;
    }
    return version;
}

int QuerySyncObject::GetObjContext(ObjContext &objContext) const
{
    if (!isValid_) {
        return -E_INVALID_QUERY_FORMAT;
    }
    objContext.version = GetVersion();
    objContext.prefixKey.assign(prefixKey_.begin(), prefixKey_.end());
    objContext.suggestIndex = suggestIndex_;
    objContext.queryObjNodes = queryObjNodes_;
    return E_OK;
}

std::string QuerySyncObject::GetIdentify() const
{
    if (!isValid_) {
        return std::string();
    }
    // suggestionIndex is local attribute, do not need to be propagated to remote
    uint64_t len = Parcel::GetVectorCharLen(prefixKey_);
    for (const QueryObjNode &node : queryObjNodes_) {
        if (node.operFlag == QueryObjType::LIMIT || node.operFlag == QueryObjType::ORDERBY ||
            node.operFlag == QueryObjType::SUGGEST_INDEX) {
            continue;
        }
        // operFlag and valueType is int
        len += Parcel::GetUInt32Len() + Parcel::GetIntLen() + Parcel::GetStringLen(node.fieldName);
        for (const FieldValue &value : node.fieldValue) {
            len += Parcel::GetStringLen(value.stringValue) + Parcel::GetInt64Len();
        }
    }

    // QUERY_SYNC_OBJECT_VERSION_1 added.
    len += isTableNameSpecified_ ? Parcel::GetStringLen(tableName_) : 0;
    for (const auto &key : keys_) {
        len += Parcel::GetVectorCharLen(key);
    }  // QUERY_SYNC_OBJECT_VERSION_1 end.

    std::vector<uint8_t> buff(len, 0); // It will affect the hash result, the default value cannot be modified
    Parcel parcel(buff.data(), len);

    // The order needs to be consistent, otherwise it will affect the hash result
    (void)parcel.WriteVectorChar(prefixKey_);
    for (const QueryObjNode &node : queryObjNodes_) {
        if (node.operFlag == QueryObjType::LIMIT || node.operFlag == QueryObjType::ORDERBY ||
            node.operFlag == QueryObjType::SUGGEST_INDEX) {
            continue;
        }
        (void)parcel.WriteUInt32(static_cast<uint32_t>(node.operFlag));
        (void)parcel.WriteInt(static_cast<int32_t>(node.type));
        (void)parcel.WriteString(node.fieldName);
        for (const FieldValue &value : node.fieldValue) {
            (void)parcel.WriteInt64(value.longValue);
            (void)parcel.WriteString(value.stringValue);
        }
    }

    // QUERY_SYNC_OBJECT_VERSION_1 added.
    if (isTableNameSpecified_) {
        (void)parcel.WriteString(tableName_);
    }
    for (const auto &key : keys_) {
        (void)parcel.WriteVectorChar(key);
    }  // QUERY_SYNC_OBJECT_VERSION_1 end.

    std::vector<uint8_t> hashBuff;
    if (parcel.IsError() || DBCommon::CalcValueHash(buff, hashBuff) != E_OK) {
        return std::string();
    }

    return DBCommon::VectorToHexString(hashBuff);
}

uint32_t QuerySyncObject::CalculateParcelLen(uint32_t softWareVersion) const
{
    if (softWareVersion == SOFTWARE_VERSION_CURRENT) {
        return CalculateLen();
    }
    LOGE("current not support!");
    return 0;
}

int QuerySyncObject::SerializeData(Parcel &parcel, uint32_t softWareVersion)
{
    ObjContext context;
    int errCode = GetObjContext(context);
    if (errCode != E_OK) {
        return errCode;
    }
    (void)parcel.WriteString(MAGIC);
    (void)parcel.WriteUInt32(context.version);
    (void)parcel.WriteVectorChar(context.prefixKey);
    (void)parcel.WriteString(context.suggestIndex);
    (void)parcel.WriteUInt32(context.queryObjNodes.size());

    parcel.EightByteAlign();

    for (const QueryObjNode &node : context.queryObjNodes) {
        errCode = SerializeDataObjNode(parcel, node);
        if (errCode != E_OK) {
            return errCode;
        }
    }

    // QUERY_SYNC_OBJECT_VERSION_1 added.
    if (context.version >= QUERY_SYNC_OBJECT_VERSION_1) {
        (void)parcel.WriteUInt32(static_cast<uint32_t>(isTableNameSpecified_));
        if (isTableNameSpecified_) {
            (void)parcel.WriteString(tableName_);
        }
        (void)parcel.WriteUInt32(keys_.size());
        for (const auto &key : keys_) {
            (void)parcel.WriteVectorChar(key);
        }
    }  // QUERY_SYNC_OBJECT_VERSION_1 end.

    if (parcel.IsError()) { // parcel almost success
        return -E_INVALID_ARGS;
    }
    parcel.EightByteAlign();
    return E_OK;
}

namespace {
int DeSerializeVersion1Data(uint32_t version, Parcel &parcel, std::string &tableName, std::set<Key> &keys)
{
    if (version >= QUERY_SYNC_OBJECT_VERSION_1) {
        uint32_t isTblNameExist = 0;
        (void)parcel.ReadUInt32(isTblNameExist);
        if (isTblNameExist) {
            (void)parcel.ReadString(tableName);
        }
        uint32_t keysSize = 0;
        (void)parcel.ReadUInt32(keysSize);
        if (keysSize > DBConstant::MAX_INKEYS_SIZE) {
            return -E_PARSE_FAIL;
        }
        for (uint32_t i = 0; i < keysSize; ++i) {
            Key key;
            (void)parcel.ReadVector(key);
            keys.emplace(key);
        }
    }
    return E_OK;
}
}

int QuerySyncObject::DeSerializeData(Parcel &parcel, QuerySyncObject &queryObj)
{
    std::string magic;
    (void)parcel.ReadString(magic);
    if (magic != MAGIC) {
        return -E_INVALID_ARGS;
    }

    ObjContext context;
    (void)parcel.ReadUInt32(context.version);
    if (context.version > QUERY_SYNC_OBJECT_VERSION_CURRENT) {
        LOGE("Parcel version and deserialize version not matched! ver=%u", context.version);
        return -E_VERSION_NOT_SUPPORT;
    }

    (void)parcel.ReadVectorChar(context.prefixKey);
    (void)parcel.ReadString(context.suggestIndex);

    uint32_t nodesSize = 0;
    (void)parcel.ReadUInt32(nodesSize);
    if (parcel.IsError()) { // almost success
        return -E_INVALID_ARGS;
    }
    parcel.EightByteAlign();
    for (size_t i = 0; i < nodesSize; i++) {
        QueryObjNode node;
        int errCode = DeSerializeDataObjNode(parcel, node);
        if (errCode != E_OK) {
            return errCode;
        }
        context.queryObjNodes.emplace_back(node);
    }

    // QUERY_SYNC_OBJECT_VERSION_1 added.
    std::string tableName;
    std::set<Key> keys;
    int errCode = DeSerializeVersion1Data(context.version, parcel, tableName, keys);
    if (errCode != E_OK) {
        return errCode;
    }  // QUERY_SYNC_OBJECT_VERSION_1 end.

    if (parcel.IsError()) { // almost success
        return -E_INVALID_ARGS;
    }
    queryObj = QuerySyncObject(context.queryObjNodes, context.prefixKey, keys);
    if (!tableName.empty()) {
        queryObj.SetTableName(tableName);
    }
    return E_OK;
}

uint32_t QuerySyncObject::CalculateLen() const
{
    uint64_t len = Parcel::GetStringLen(MAGIC);
    len += Parcel::GetUInt32Len(); // version
    len += Parcel::GetVectorCharLen(prefixKey_);
    len += Parcel::GetStringLen(suggestIndex_);
    len += Parcel::GetUInt32Len(); // nodes size
    len = Parcel::GetEightByteAlign(len);
    for (const QueryObjNode &node : queryObjNodes_) {
        if (node.operFlag == QueryObjType::OPER_ILLEGAL) {
            LOGE("contain illegal operator for query sync!");
            return 0;
        }
        // operflag, fieldName, query value type, value size, union max size, string value
        len += Parcel::GetUInt32Len();
        len = Parcel::GetEightByteAlign(len);
        len += Parcel::GetStringLen(node.fieldName) +
            Parcel::GetIntLen() + Parcel::GetUInt32Len();
        for (size_t i = 0; i < node.fieldValue.size(); i++) {
            len += Parcel::GetInt64Len() + Parcel::GetStringLen(node.fieldValue[i].stringValue);
        }
    }

    // QUERY_SYNC_OBJECT_VERSION_1 added.
    len += Parcel::GetUInt32Len(); // whether the table name exists.
    if (isTableNameSpecified_) {
        len += Parcel::GetStringLen(tableName_);
    }
    len += Parcel::GetUInt32Len(); // size of keys_
    for (const auto &key : keys_) {
        len += Parcel::GetVectorCharLen(key);
    }  // QUERY_SYNC_OBJECT_VERSION_1 end.

    len = Parcel::GetEightByteAlign(len);
    if (len > INT32_MAX) {
        return 0;
    }
    return static_cast<uint32_t>(len);
}

std::string QuerySyncObject::GetRelationTableName() const
{
    if (!isTableNameSpecified_) {
        return {};
    }
    return tableName_;
}
}