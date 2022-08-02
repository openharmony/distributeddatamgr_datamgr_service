/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "schema_negotiate.h"

#include "log_print.h"
#include "schema_utils.h"

namespace DistributedDB {
// Some principle in current version describe below. (Relative-type will be introduced in future but not involved now)
// 1.   PermitSync: Be false may because schemaType-unrecognized, schemaType-different, schema-unparsable,
//      schemaVersion-unrecognized, schema-incompatible, and so on.
// 2.   RequirePeerConvert: Be true normally when permitSync false, for future possible sync and convert(by remote).
// 3.   checkOnReceive: Be false when local is KV-DB, or when local is not KV-DB only if schema type equal as well as
//      define equal or remote is the upgradation of local.
SyncOpinion SchemaNegotiate::MakeLocalSyncOpinion(const SchemaObject &localSchema, const std::string &remoteSchema,
    uint8_t remoteSchemaType)
{
    SchemaType localType = localSchema.GetSchemaType(); // An invalid schemaObject will return SchemaType::NONE
    SchemaType remoteType = ReadSchemaType(remoteSchemaType);
    // Logic below only be correct in current version, should be redesigned if new type added in the future
    // 1. If remote-type unrecognized(Include Relative-type), Do not permit sync.
    if (remoteType == SchemaType::UNRECOGNIZED) {
        LOGE("[Schema][Opinion] Remote-type=%" PRIu8 " unrecognized.", remoteSchemaType);
        return SyncOpinion{false, true, true};
    }
    // 2. If local-type is KV(Here remote-type is within recognized), Always permit sync.
    if (localType == SchemaType::NONE) {
        LOGI("[Schema][Opinion] Local-type KV.");
        return SyncOpinion{true, false, false};
    }
    // 3. If remote-type is KV(Here local-type can only be JSON or FLATBUFFER), Always permit sync but need check.
    if (remoteType == SchemaType::NONE) {
        LOGI("[Schema][Opinion] Remote-type KV.");
        return SyncOpinion{true, false, true};
    }
    // 4. If local-type differ with remote-type(Here both type can only be JSON or FLATBUFFER), Do not permit sync.
    if (localType != remoteType) {
        LOGE("[Schema][Opinion] Local-type=%s differ remote-type=%s.", SchemaUtils::SchemaTypeString(localType).c_str(),
            SchemaUtils::SchemaTypeString(remoteType).c_str());
        return SyncOpinion{false, true, true};
    }
    // 5. If schema parse fail, Do not permit sync.
    SchemaObject remoteSchemaObj;
    int errCode = remoteSchemaObj.ParseFromSchemaString(remoteSchema);
    if (errCode != E_OK) {
        LOGE("[Schema][Opinion] Parse remote-schema fail, errCode=%d, remote-type=%s.", errCode,
            SchemaUtils::SchemaTypeString(remoteType).c_str());
        return SyncOpinion{false, true, true};
    }
    // 6. If remote-schema is not incompatible based on local-schema(SchemaDefine Equal), Permit sync and don't check.
    errCode = localSchema.CompareAgainstSchemaObject(remoteSchemaObj);
    if (errCode != -E_SCHEMA_UNEQUAL_INCOMPATIBLE) {
        return SyncOpinion{true, false, false};
    }
    // 7. If local-schema is not incompatible based on remote-schema(Can only be COMPATIBLE_UPGRADE), Sync and check.
    errCode = remoteSchemaObj.CompareAgainstSchemaObject(localSchema);
    if (errCode != -E_SCHEMA_UNEQUAL_INCOMPATIBLE) {
        return SyncOpinion{true, false, true};
    }
    // 8. Local-schema incompatible with remote-schema mutually.
    LOGE("[Schema][Opinion] Local-schema incompatible with remote-schema mutually.");
    return SyncOpinion{false, true, true};
}

SyncStrategy SchemaNegotiate::ConcludeSyncStrategy(const SyncOpinion &localOpinion, const SyncOpinion &remoteOpinion)
{
    SyncStrategy outStrategy;
    // Any side permit sync, the final conclusion is permit sync.
    outStrategy.permitSync = (localOpinion.permitSync || remoteOpinion.permitSync);
    bool convertConflict = (localOpinion.requirePeerConvert && remoteOpinion.requirePeerConvert);
    if (convertConflict) {
        outStrategy.permitSync = false;
    }
    // Responsible for conversion on send now that local do not require remote to do conversion
    outStrategy.convertOnSend = (!localOpinion.requirePeerConvert);
    // Responsible for conversion on receive since remote will not do conversion on send and require local to convert
    outStrategy.convertOnReceive = remoteOpinion.requirePeerConvert;
    // Only depend on local opinion
    outStrategy.checkOnReceive = localOpinion.checkOnReceive;
    LOGI("[Schema][Strategy] PermitSync=%d, SendConvert=%d, ReceiveConvert=%d, ReceiveCheck=%d.",
        outStrategy.permitSync, outStrategy.convertOnSend, outStrategy.convertOnReceive, outStrategy.checkOnReceive);
    return outStrategy;
}

RelationalSyncOpinion SchemaNegotiate::MakeOpinionEachTable(const RelationalSchemaObject &localSchema,
    const RelationalSchemaObject &remoteSchema)
{
    RelationalSyncOpinion opinion;
    for (const auto &it : localSchema.GetTables()) {
        if (remoteSchema.GetTable(it.first).GetTableName() != it.first) {
            LOGW("[RelationalSchema][opinion] Table was missing in remote schema");
            continue;
        }
        // remote table is compatible(equal or upgrade) based on local table, permit sync and don't need check
        int errCode = it.second.CompareWithTable(remoteSchema.GetTable(it.first), localSchema.GetSchemaVersion());
        if (errCode != -E_RELATIONAL_TABLE_INCOMPATIBLE) {
            opinion[it.first] = {true, false, false};
            continue;
        }
        // local table is compatible upgrade based on remote table, permit sync and need check
        errCode = remoteSchema.GetTable(it.first).CompareWithTable(it.second, remoteSchema.GetSchemaVersion());
        if (errCode != -E_RELATIONAL_TABLE_INCOMPATIBLE) {
            opinion[it.first] = {true, false, true};
            continue;
        }
        // local table is incompatible with remote table mutually, don't permit sync and need check
        LOGW("[RelationalSchema][opinion] Local table is incompatible with remote table mutually.");
        opinion[it.first] = {false, true, true};
    }
    return opinion;
}

RelationalSyncOpinion SchemaNegotiate::MakeLocalSyncOpinion(const RelationalSchemaObject &localSchema,
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

    if (localSchema.GetSchemaVersion() != remoteSchemaObj.GetSchemaVersion()) {
        LOGW("[RelationalSchema][opinion] Schema version mismatch, local %s, remote %s",
            localSchema.GetSchemaVersion().c_str(), remoteSchemaObj.GetSchemaVersion().c_str());
        return {};
    }

    if (localSchema.GetSchemaVersion() == SchemaConstant::SCHEMA_SUPPORT_VERSION_V2_1 &&
        localSchema.GetTableMode() != remoteSchemaObj.GetTableMode()) {
        LOGW("[RelationalSchema][opinion] Schema table mode mismatch, local %d, remote %d",
            localSchema.GetTableMode(), remoteSchemaObj.GetTableMode());
        return {};
    }

    return MakeOpinionEachTable(localSchema, remoteSchemaObj);
}

RelationalSyncStrategy SchemaNegotiate::ConcludeSyncStrategy(const RelationalSyncOpinion &localOpinion,
    const RelationalSyncOpinion &remoteOpinion)
{
    RelationalSyncStrategy syncStrategy;
    for (const auto &itLocal : localOpinion) {
        if (remoteOpinion.find(itLocal.first) == remoteOpinion.end()) {
            LOGW("[RelationalSchema][Strategy] Table opinion is not found from remote.");
            continue;
        }
        SyncOpinion localTableOpinion = itLocal.second;
        SyncOpinion remoteTableOpinion = remoteOpinion.at(itLocal.first);
        syncStrategy[itLocal.first] = ConcludeSyncStrategy(localTableOpinion, remoteTableOpinion);
    }

    return syncStrategy;
}

namespace {
    const std::string MAGIC = "relational_opinion";
    const uint32_t SYNC_OPINION_VERSION = 1;
} // namespace


uint32_t SchemaNegotiate::CalculateParcelLen(const RelationalSyncOpinion &opinions)
{
    uint64_t len = Parcel::GetStringLen(MAGIC);
    len += Parcel::GetUInt32Len();
    len += Parcel::GetUInt32Len();
    len = Parcel::GetEightByteAlign(len);
    for (const auto &it : opinions) {
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

int SchemaNegotiate::SerializeData(const RelationalSyncOpinion &opinions, Parcel &parcel)
{
    (void)parcel.WriteString(MAGIC);
    (void)parcel.WriteUInt32(SYNC_OPINION_VERSION);
    (void)parcel.WriteUInt32(static_cast<uint32_t>(opinions.size()));
    (void)parcel.EightByteAlign();
    for (const auto &it : opinions) {
        (void)parcel.WriteString(it.first);
        (void)parcel.WriteUInt32(it.second.permitSync);
        (void)parcel.WriteUInt32(it.second.requirePeerConvert);
        (void)parcel.EightByteAlign();
    }
    return parcel.IsError() ? -E_INVALID_ARGS : E_OK;
}

int SchemaNegotiate::DeserializeData(Parcel &parcel, RelationalSyncOpinion &opinion)
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
    static const uint32_t MAX_OPINION_SIZE = 1024; // max 1024 opinions
    if (parcel.IsError() || opinionSize > MAX_OPINION_SIZE) {
        return -E_INVALID_ARGS;
    }
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
        opinion[tableName] =  tableOpinion;
    }
    return parcel.IsError() ? -E_INVALID_ARGS : E_OK;
}
}