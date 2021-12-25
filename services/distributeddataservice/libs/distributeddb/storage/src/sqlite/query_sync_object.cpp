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

QuerySyncObject::QuerySyncObject(const std::list<QueryObjNode> &queryObjNodes, const std::vector<uint8_t> &prefixKey)
    : QueryObject(queryObjNodes, prefixKey)
{}

QuerySyncObject::QuerySyncObject(const Query &query)
    : QueryObject(query)
{}

QuerySyncObject::~QuerySyncObject()
{}

int QuerySyncObject::GetObjContext(ObjContext &objContext) const
{
    if (!isValid_) {
        return -E_INVALID_QUERY_FORMAT;
    }
    objContext.version = isTableNameSpecified_ ? QUERY_SYNC_OBJECT_VERSION_1 : QUERY_SYNC_OBJECT_VERSION_0;
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
    if (isTableNameSpecified_) {
        len += Parcel::GetStringLen(tableName_);
    }
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

    std::vector<uint8_t> buff(len, 0); // It will affect the hash result, the default value cannot be modified
    Parcel parcel(buff.data(), len);

    // The order needs to be consistent, otherwise it will affect the hash result
    (void)parcel.WriteVectorChar(prefixKey_);
    if (isTableNameSpecified_) {
        (void)parcel.WriteString(tableName_);
    }
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

    if (parcel.IsError()) {
        return std::string();
    }

    std::vector<uint8_t> hashBuff;
    int errCode = DBCommon::CalcValueHash(buff, hashBuff);
    if (errCode != E_OK) {
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
    if (isTableNameSpecified_) {
        (void)parcel.WriteString(tableName_);
    }
    (void)parcel.WriteUInt32(context.queryObjNodes.size());

    parcel.EightByteAlign();

    for (const QueryObjNode &node : context.queryObjNodes) {
        errCode = SerializeDataObjNode(parcel, node);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    if (parcel.IsError()) { // parcel almost success
        return -E_INVALID_ARGS;
    }
    parcel.EightByteAlign();
    return E_OK;
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
    if (context.version > QUERY_SYNC_OBJECT_VERSION_1) {
        LOGE("Parcel version and deserialize version not matched! ver=%u", context.version);
        return -E_VERSION_NOT_SUPPORT;
    }

    (void)parcel.ReadVectorChar(context.prefixKey);
    (void)parcel.ReadString(context.suggestIndex);
    std::string tableName;
    if (context.version == QUERY_SYNC_OBJECT_VERSION_1) {
        parcel.ReadString(tableName);
    }

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

    if (parcel.IsError()) { // almost success
        return -E_INVALID_ARGS;
    }
    queryObj = QuerySyncObject(context.queryObjNodes, context.prefixKey);
    if (context.version == QUERY_SYNC_OBJECT_VERSION_1) {
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
    if (isTableNameSpecified_) {
        len += Parcel::GetStringLen(tableName_);
    }
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