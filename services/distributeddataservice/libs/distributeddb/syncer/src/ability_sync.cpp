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

#include "ability_sync.h"

#include "message_transform.h"
#include "version.h"
#include "db_errno.h"
#include "log_print.h"
#include "sync_types.h"
#include "db_common.h"
#include "single_ver_kvdb_sync_interface.h"
#include "single_ver_sync_task_context.h"
#include "single_ver_kv_sync_task_context.h"
#ifdef RELATIONAL_STORE
#include "relational_db_sync_interface.h"
#include "single_ver_relational_sync_task_context.h"
#endif

namespace DistributedDB {
AbilitySyncRequestPacket::AbilitySyncRequestPacket()
    : protocolVersion_(ABILITY_SYNC_VERSION_V1),
      sendCode_(E_OK),
      softwareVersion_(SOFTWARE_VERSION_CURRENT),
      secLabel_(0),
      secFlag_(0),
      schemaType_(0),
      dbCreateTime_(0)
{
}

AbilitySyncRequestPacket::~AbilitySyncRequestPacket()
{
}

void AbilitySyncRequestPacket::SetProtocolVersion(uint32_t protocolVersion)
{
    protocolVersion_ = protocolVersion;
}

uint32_t AbilitySyncRequestPacket::GetProtocolVersion() const
{
    return protocolVersion_;
}

void AbilitySyncRequestPacket::SetSendCode(int32_t sendCode)
{
    sendCode_ = sendCode;
}

int32_t AbilitySyncRequestPacket::GetSendCode() const
{
    return sendCode_;
}

void AbilitySyncRequestPacket::SetSoftwareVersion(uint32_t swVersion)
{
    softwareVersion_ = swVersion;
}

uint32_t AbilitySyncRequestPacket::GetSoftwareVersion() const
{
    return softwareVersion_;
}

void AbilitySyncRequestPacket::SetSchema(const std::string &schema)
{
    schema_ = schema;
}

std::string AbilitySyncRequestPacket::GetSchema() const
{
    return schema_;
}

void AbilitySyncRequestPacket::SetSchemaType(uint32_t schemaType)
{
    schemaType_ = schemaType;
}

uint32_t AbilitySyncRequestPacket::GetSchemaType() const
{
    return schemaType_;
}

void AbilitySyncRequestPacket::SetSecLabel(int32_t secLabel)
{
    secLabel_ = secLabel;
}

int32_t AbilitySyncRequestPacket::GetSecLabel() const
{
    return secLabel_;
}

void AbilitySyncRequestPacket::SetSecFlag(int32_t secFlag)
{
    secFlag_ = secFlag;
}

int32_t AbilitySyncRequestPacket::GetSecFlag() const
{
    return secFlag_;
}

void AbilitySyncRequestPacket::SetDbCreateTime(uint64_t dbCreateTime)
{
    dbCreateTime_ = dbCreateTime;
}

uint64_t AbilitySyncRequestPacket::GetDbCreateTime() const
{
    return dbCreateTime_;
}

uint32_t AbilitySyncRequestPacket::CalculateLen() const
{
    uint64_t len = 0;
    len += Parcel::GetUInt32Len(); // protocolVersion_
    len += Parcel::GetIntLen(); // sendCode_
    len += Parcel::GetUInt32Len(); // softwareVersion_
    uint32_t schemLen = Parcel::GetStringLen(schema_);
    if (schemLen == 0) {
        LOGE("[AbilitySyncRequestPacket][CalculateLen] schemLen err!");
        return 0;
    }
    len += schemLen;
    len += Parcel::GetIntLen(); // secLabel_
    len += Parcel::GetIntLen(); // secFlag_
    len += Parcel::GetUInt32Len(); // schemaType_
    len += Parcel::GetUInt64Len(); // dbCreateTime_
    len += DbAbility::CalculateLen(dbAbility_); // dbAbility_
    // the reason why not 8-byte align is that old version is not 8-byte align
    // so it is not possible to set 8-byte align for high version.
    if (len > INT32_MAX) {
        LOGE("[AbilitySyncRequestPacket][CalculateLen]  err len:%llu", len);
        return 0;
    }
    return len;
}

DbAbility AbilitySyncRequestPacket::GetDbAbility() const
{
    return dbAbility_;
}

void AbilitySyncRequestPacket::SetDbAbility(const DbAbility &dbAbility)
{
    dbAbility_ = dbAbility;
}

AbilitySyncAckPacket::AbilitySyncAckPacket()
    : protocolVersion_(ABILITY_SYNC_VERSION_V1),
      softwareVersion_(SOFTWARE_VERSION_CURRENT),
      ackCode_(E_OK),
      secLabel_(0),
      secFlag_(0),
      schemaType_(0),
      permitSync_(0),
      requirePeerConvert_(0),
      dbCreateTime_(0)
{
}

AbilitySyncAckPacket::~AbilitySyncAckPacket()
{
}

void AbilitySyncAckPacket::SetProtocolVersion(uint32_t protocolVersion)
{
    protocolVersion_ = protocolVersion;
}

void AbilitySyncAckPacket::SetSoftwareVersion(uint32_t swVersion)
{
    softwareVersion_ = swVersion;
}

uint32_t AbilitySyncAckPacket::GetSoftwareVersion() const
{
    return softwareVersion_;
}

uint32_t AbilitySyncAckPacket::GetProtocolVersion() const
{
    return protocolVersion_;
}

void AbilitySyncAckPacket::SetAckCode(int32_t ackCode)
{
    ackCode_ = ackCode;
}

int32_t AbilitySyncAckPacket::GetAckCode() const
{
    return ackCode_;
}

void AbilitySyncAckPacket::SetSchema(const std::string &schema)
{
    schema_ = schema;
}

std::string AbilitySyncAckPacket::GetSchema() const
{
    return schema_;
}

void AbilitySyncAckPacket::SetSchemaType(uint32_t schemaType)
{
    schemaType_ = schemaType;
}

uint32_t AbilitySyncAckPacket::GetSchemaType() const
{
    return schemaType_;
}

void AbilitySyncAckPacket::SetSecLabel(int32_t secLabel)
{
    secLabel_ = secLabel;
}

int32_t AbilitySyncAckPacket::GetSecLabel() const
{
    return secLabel_;
}

void AbilitySyncAckPacket::SetSecFlag(int32_t secFlag)
{
    secFlag_ = secFlag;
}

int32_t AbilitySyncAckPacket::GetSecFlag() const
{
    return secFlag_;
}

void AbilitySyncAckPacket::SetPermitSync(uint32_t permitSync)
{
    permitSync_ = permitSync;
}

uint32_t AbilitySyncAckPacket::GetPermitSync() const
{
    return permitSync_;
}

void AbilitySyncAckPacket::SetRequirePeerConvert(uint32_t requirePeerConvert)
{
    requirePeerConvert_ = requirePeerConvert;
}

uint32_t AbilitySyncAckPacket::GetRequirePeerConvert() const
{
    return requirePeerConvert_;
}

void AbilitySyncAckPacket::SetDbCreateTime(uint64_t dbCreateTime)
{
    dbCreateTime_ = dbCreateTime;
}

uint64_t AbilitySyncAckPacket::GetDbCreateTime() const
{
    return dbCreateTime_;
}

uint32_t AbilitySyncAckPacket::CalculateLen() const
{
    uint64_t len = 0;
    len += Parcel::GetUInt32Len();
    len += Parcel::GetUInt32Len();
    len += Parcel::GetIntLen();
    uint32_t schemLen = Parcel::GetStringLen(schema_);
    if (schemLen == 0) {
        LOGE("[AbilitySyncAckPacket][CalculateLen] schemLen err!");
        return 0;
    }
    len += schemLen;
    len += Parcel::GetIntLen(); // secLabel_
    len += Parcel::GetIntLen(); // secFlag_
    len += Parcel::GetUInt32Len(); // schemaType_
    len += Parcel::GetUInt32Len(); // permitSync_
    len += Parcel::GetUInt32Len(); // requirePeerConvert_
    len += Parcel::GetUInt64Len(); // dbCreateTime_
    len += DbAbility::CalculateLen(dbAbility_); // dbAbility_
    len += relationalSyncOpinion_.CalculateParcelLen(softwareVersion_);
    if (len > INT32_MAX) {
        LOGE("[AbilitySyncAckPacket][CalculateLen]  err len:%llu", len);
        return 0;
    }
    return len;
}

DbAbility AbilitySyncAckPacket::GetDbAbility() const
{
    return dbAbility_;
}

void AbilitySyncAckPacket::SetDbAbility(const DbAbility &dbAbility)
{
    dbAbility_ = dbAbility;
}

void AbilitySyncAckPacket::SetRelationalSyncOpinion(const RelationalSyncOpinion &relationalSyncOpinion)
{
    relationalSyncOpinion_ = relationalSyncOpinion;
}

RelationalSyncOpinion AbilitySyncAckPacket::GetRelationalSyncOpinion() const
{
    return relationalSyncOpinion_;
}

AbilitySync::AbilitySync()
    : communicator_(nullptr),
      storageInterface_(nullptr),
      metadata_(nullptr),
      syncFinished_(false)
{
}

AbilitySync::~AbilitySync()
{
    communicator_ = nullptr;
    storageInterface_ = nullptr;
    metadata_ = nullptr;
}

int AbilitySync::Initialize(ICommunicator *inCommunicator, ISyncInterface *inStorage,
    std::shared_ptr<Metadata> &inMetadata, const std::string &deviceId)
{
    if (inCommunicator == nullptr || inStorage == nullptr || deviceId.empty() || inMetadata == nullptr) {
        return -E_INVALID_ARGS;
    }
    communicator_ = inCommunicator;
    storageInterface_ = inStorage;
    metadata_ = inMetadata;
    deviceId_ = deviceId;
    return E_OK;
}

int AbilitySync::SyncStart(uint32_t sessionId, uint32_t sequenceId, uint16_t remoteCommunicatorVersion,
    const CommErrHandler &handler)
{
    AbilitySyncRequestPacket packet;
    int errCode = SetAbilityRequestBodyInfo(packet, remoteCommunicatorVersion);
    if (errCode != E_OK) {
        return errCode;
    }
    Message *message = new (std::nothrow) Message(ABILITY_SYNC_MESSAGE);
    if (message == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    message->SetMessageType(TYPE_REQUEST);
    errCode = message->SetCopiedObject<>(packet);
    if (errCode != E_OK) {
        LOGE("[AbilitySync][SyncStart] SetCopiedObject failed, err %d", errCode);
        delete message;
        message = nullptr;
        return errCode;
    }
    message->SetVersion(MSG_VERSION_EXT);
    message->SetSessionId(sessionId);
    message->SetSequenceId(sequenceId);
    errCode = communicator_->SendMessage(deviceId_, message, false, SEND_TIME_OUT, handler);
    if (errCode != E_OK) {
        LOGE("[AbilitySync][SyncStart] SendPacket failed, err %d", errCode);
        delete message;
        message = nullptr;
    }
    return errCode;
}

int AbilitySync::AckRecv(const Message *message, ISyncTaskContext *context)
{
    int errCode = AckMsgCheck(message, context);
    if (errCode != E_OK) {
        return errCode;
    }
    const AbilitySyncAckPacket *packet = message->GetObject<AbilitySyncAckPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    uint32_t remoteSoftwareVersion = packet->GetSoftwareVersion();
    context->SetRemoteSoftwareVersion(remoteSoftwareVersion);
    std::string schema = packet->GetSchema();
    if (remoteSoftwareVersion > SOFTWARE_VERSION_RELEASE_2_0) {
        HandleVersionV3AckSecOptionParam(packet, context);
        AbilitySyncAckPacket ackPacket;
        errCode = HandleVersionV3AckSchemaParam(packet, ackPacket, context, true);
        if (errCode != E_OK) {
            return errCode;
        }
        auto singleVerContext = static_cast<SingleVerSyncTaskContext *>(context);
        auto query = singleVerContext->GetQuery();
        bool permitSync = (singleVerContext->GetSyncStrategy(query)).permitSync;
        if (!permitSync) {
            (static_cast<SingleVerSyncTaskContext *>(context))->SetTaskErrCode(-E_SCHEMA_MISMATCH);
            LOGE("[AbilitySync][AckRecv] scheme check failed");
            return -E_SCHEMA_MISMATCH;
        }
        if (remoteSoftwareVersion > SOFTWARE_VERSION_RELEASE_3_0) {
            errCode = metadata_->SetDbCreateTime(deviceId_, packet->GetDbCreateTime(), true);
            if (errCode != E_OK) {
                LOGE("[AbilitySync][AckRecv] set db create time failed,errCode=%d", errCode);
                context->SetTaskErrCode(errCode);
                return errCode;
            }
        }
        DbAbility remoteDbAbility = packet->GetDbAbility();
        (static_cast<SingleVerSyncTaskContext *>(context))->SetDbAbility(remoteDbAbility);
        (void)SendAck(message, AbilitySync::CHECK_SUCCESS, true, ackPacket);
        (static_cast<SingleVerSyncTaskContext *>(context))->SetIsSchemaSync(true);
    } else {
        bool isCompatible = static_cast<SyncGenericInterface *>(storageInterface_)->CheckCompatible(schema);
        if (!isCompatible) {
            (static_cast<SingleVerSyncTaskContext *>(context))->SetTaskErrCode(-E_SCHEMA_MISMATCH);
            LOGE("[AbilitySync][AckRecv] scheme check failed");
            return -E_SCHEMA_MISMATCH;
        }
        LOGI("[AbilitySync][AckRecv]remoteSoftwareVersion = %u, isCompatible = %d,", remoteSoftwareVersion,
            isCompatible);
    }
    return E_OK;
}

int AbilitySync::RequestRecv(const Message *message, ISyncTaskContext *context)
{
    if (message == nullptr || context == nullptr) {
        return -E_INVALID_ARGS;
    }
    const AbilitySyncRequestPacket *packet = message->GetObject<AbilitySyncRequestPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    if (packet->GetSendCode() == -E_VERSION_NOT_SUPPORT) {
        AbilitySyncAckPacket ackPacket;
        (void)SendAck(message, -E_VERSION_NOT_SUPPORT, false, ackPacket);
        LOGI("[AbilitySync][RequestRecv] version can not support, remote version is %u", packet->GetProtocolVersion());
        return -E_VERSION_NOT_SUPPORT;
    }

    std::string schema = packet->GetSchema();
    bool isCompatible = static_cast<SyncGenericInterface *>(storageInterface_)->CheckCompatible(schema);
    if (!isCompatible) {
        (static_cast<SingleVerSyncTaskContext *>(context))->SetTaskErrCode(-E_SCHEMA_MISMATCH);
    }
    uint32_t remoteSoftwareVersion = packet->GetSoftwareVersion();
    context->SetRemoteSoftwareVersion(remoteSoftwareVersion);
    return HandleRequestRecv(message, context, isCompatible);
}

int AbilitySync::AckNotifyRecv(const Message *message, ISyncTaskContext *context)
{
    if (message == nullptr || context == nullptr) {
        return -E_INVALID_ARGS;
    }
    if (message->GetErrorNo() == E_FEEDBACK_UNKNOWN_MESSAGE) {
        LOGE("[AbilitySync][AckNotifyRecv] Remote device dose not support this message id");
        context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_EARLIEST);
        return -E_FEEDBACK_UNKNOWN_MESSAGE;
    }
    const AbilitySyncAckPacket *packet = message->GetObject<AbilitySyncAckPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    int errCode = packet->GetAckCode();
    if (errCode != E_OK) {
        LOGE("[AbilitySync][AckNotifyRecv] received a errCode %d", errCode);
        return errCode;
    }
    std::string schema = packet->GetSchema();
    uint32_t remoteSoftwareVersion = packet->GetSoftwareVersion();
    context->SetRemoteSoftwareVersion(remoteSoftwareVersion);
    AbilitySyncAckPacket sendPacket;
    errCode = HandleVersionV3AckSchemaParam(packet, sendPacket, context, false);
    int ackCode = errCode;
    LOGI("[AckNotifyRecv] receive dev = %s ack notify, remoteSoftwareVersion = %u, ackCode = %d",
        STR_MASK(deviceId_), remoteSoftwareVersion, errCode);
    if (errCode == E_OK) {
        (static_cast<SingleVerSyncTaskContext *>(context))->SetIsSchemaSync(true);
        ackCode = AbilitySync::LAST_NOTIFY;
    }
    (void)SendAckWithEmptySchema(message, ackCode, true);
    return errCode;
}

bool AbilitySync::GetAbilitySyncFinishedStatus() const
{
    return syncFinished_;
}

void AbilitySync::SetAbilitySyncFinishedStatus(bool syncFinished)
{
    syncFinished_ = syncFinished;
}

bool AbilitySync::SecLabelCheck(const AbilitySyncRequestPacket *packet) const
{
    int32_t remoteSecLabel = packet->GetSecLabel();
    int32_t remoteSecFlag = packet->GetSecFlag();
    if (remoteSecLabel == NOT_SURPPORT_SEC_CLASSIFICATION || remoteSecLabel == SecurityLabel::NOT_SET) {
        return true;
    }
    SecurityOption option;
    int errCode = (static_cast<SyncGenericInterface *>(storageInterface_))->GetSecurityOption(option);
    LOGI("[AbilitySync][RequestRecv] local l:%d, f:%d, errCode:%d", option.securityLabel, option.securityFlag, errCode);
    if (errCode == -E_NOT_SUPPORT || option.securityLabel == SecurityLabel::NOT_SET) {
        return true;
    }
    if (remoteSecLabel == FAILED_GET_SEC_CLASSIFICATION || errCode != E_OK) {
        LOGE("[AbilitySync][RequestRecv] check error remoteL:%d, errCode:%d", remoteSecLabel, errCode);
        return false;
    }
    if (remoteSecLabel == option.securityLabel) {
        return true;
    } else {
        LOGE("[AbilitySync][RequestRecv] check error remote:%d , %d local:%d , %d",
            remoteSecLabel, remoteSecFlag, option.securityLabel, option.securityFlag);
        return false;
    }
}

void AbilitySync::HandleVersionV3RequestParam(const AbilitySyncRequestPacket *packet, ISyncTaskContext *context,
    const std::string &remoteSchema) const
{
    int32_t remoteSecLabel = packet->GetSecLabel();
    int32_t remoteSecFlag = packet->GetSecFlag();
    DbAbility remoteDbAbility = packet->GetDbAbility();
    (static_cast<SingleVerSyncTaskContext *>(context))->SetDbAbility(remoteDbAbility);
    (static_cast<SingleVerSyncTaskContext *>(context))->SetRemoteSeccurityOption({remoteSecLabel, remoteSecFlag});
    (static_cast<SingleVerSyncTaskContext *>(context))->SetReceivcPermitCheck(false);
    LOGI("[AbilitySync][HandleVersionV3RequestParam] remoteSecLabel = %d, remoteSecFlag = %d, remoteSchemaType = %u",
        remoteSecLabel, remoteSecFlag, packet->GetSchemaType());
}

void AbilitySync::HandleVersionV3AckSecOptionParam(const AbilitySyncAckPacket *packet,
    ISyncTaskContext *context) const
{
    int32_t remoteSecLabel = packet->GetSecLabel();
    int32_t remoteSecFlag = packet->GetSecFlag();
    SecurityOption secOption = {remoteSecLabel, remoteSecFlag};
    (static_cast<SingleVerSyncTaskContext *>(context))->SetRemoteSeccurityOption(secOption);
    (static_cast<SingleVerSyncTaskContext *>(context))->SetSendPermitCheck(false);
    LOGI("[AbilitySync][AckRecv] remoteSecLabel = %d, remoteSecFlag = %d", remoteSecLabel, remoteSecFlag);
}

int AbilitySync::HandleVersionV3AckSchemaParam(const AbilitySyncAckPacket *recvPacket,
    AbilitySyncAckPacket &sendPacket,  ISyncTaskContext *context, bool sendOpinion) const
{
    if (IsSingleRelationalVer()) {
        return HandleRelationAckSchemaParam(recvPacket, sendPacket, context, sendOpinion);
    }
    HandleKvAckSchemaParam(recvPacket, context, sendPacket);
    return E_OK;
}

void AbilitySync::GetPacketSecOption(SecurityOption &option) const
{
    int errCode = -E_NOT_SUPPORT;
    if (IsSingleKvVer()) {
        errCode =
            (static_cast<SingleVerKvDBSyncInterface *>(storageInterface_))->GetSecurityOption(option);
    }
    if (errCode == -E_NOT_SUPPORT) {
        LOGE("[AbilitySync][SyncStart] GetSecOpt not surpport sec classification");
        option.securityLabel = NOT_SURPPORT_SEC_CLASSIFICATION;
    } else if (errCode != E_OK) {
        LOGE("[AbilitySync][SyncStart] GetSecOpt errCode:%d", errCode);
        option.securityLabel = FAILED_GET_SEC_CLASSIFICATION;
    }
}

int AbilitySync::RegisterTransformFunc()
{
    TransformFunc func;
    func.computeFunc = std::bind(&AbilitySync::CalculateLen, std::placeholders::_1);
    func.serializeFunc = std::bind(&AbilitySync::Serialization, std::placeholders::_1,
                                   std::placeholders::_2, std::placeholders::_3);
    func.deserializeFunc = std::bind(&AbilitySync::DeSerialization, std::placeholders::_1,
                                     std::placeholders::_2, std::placeholders::_3);
    return MessageTransform::RegTransformFunction(ABILITY_SYNC_MESSAGE, func);
}

uint32_t AbilitySync::CalculateLen(const Message *inMsg)
{
    if ((inMsg == nullptr) || (inMsg->GetMessageId() != ABILITY_SYNC_MESSAGE)) {
        return 0;
    }
    int errCode;
    uint32_t len = 0;
    switch (inMsg->GetMessageType()) {
        case TYPE_REQUEST:
            errCode = RequestPacketCalculateLen(inMsg, len);
            if (errCode != E_OK) {
                LOGE("[AbilitySync][CalculateLen] request packet calc length err %d", errCode);
            }
            break;
        case TYPE_RESPONSE:
            errCode = AckPacketCalculateLen(inMsg, len);
            if (errCode != E_OK) {
                LOGE("[AbilitySync][CalculateLen] ack packet calc length err %d", errCode);
            }
            break;
        case TYPE_NOTIFY:
            errCode = AckPacketCalculateLen(inMsg, len);
            if (errCode != E_OK) {
                LOGE("[AbilitySync][CalculateLen] ack packet calc length err %d", errCode);
            }
            break;
        default:
            LOGE("[AbilitySync][CalculateLen] message type not support, type %d", inMsg->GetMessageType());
            break;
    }
    return len;
}

int AbilitySync::Serialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
{
    if ((buffer == nullptr) || (inMsg == nullptr)) {
        return -E_INVALID_ARGS;
    }

    switch (inMsg->GetMessageType()) {
        case TYPE_REQUEST:
            return RequestPacketSerialization(buffer, length, inMsg);
        case TYPE_RESPONSE:
            return AckPacketSerialization(buffer, length, inMsg);
        case TYPE_NOTIFY:
            return AckPacketSerialization(buffer, length, inMsg);
        default:
            return -E_MESSAGE_TYPE_ERROR;
    }
}

int AbilitySync::DeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg)
{
    if ((buffer == nullptr) || (inMsg == nullptr)) {
        return -E_INVALID_ARGS;
    }

    switch (inMsg->GetMessageType()) {
        case TYPE_REQUEST:
            return RequestPacketDeSerialization(buffer, length, inMsg);
        case TYPE_RESPONSE:
            return AckPacketDeSerialization(buffer, length, inMsg);
        case TYPE_NOTIFY:
            return AckPacketDeSerialization(buffer, length, inMsg);
        default:
            return -E_MESSAGE_TYPE_ERROR;
    }
}

int AbilitySync::RequestPacketCalculateLen(const Message *inMsg, uint32_t &len)
{
    const AbilitySyncRequestPacket *packet = inMsg->GetObject<AbilitySyncRequestPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }

    len = packet->CalculateLen();
    return E_OK;
}

int AbilitySync::AckPacketCalculateLen(const Message *inMsg, uint32_t &len)
{
    const AbilitySyncAckPacket *packet = inMsg->GetObject<AbilitySyncAckPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }

    len = packet->CalculateLen();
    return E_OK;
}

int AbilitySync::RequestPacketSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
{
    const AbilitySyncRequestPacket *packet = inMsg->GetObject<AbilitySyncRequestPacket>();
    if ((packet == nullptr) || (length != packet->CalculateLen())) {
        return -E_INVALID_ARGS;
    }

    Parcel parcel(buffer, length);
    parcel.WriteUInt32(packet->GetProtocolVersion());
    parcel.WriteInt(packet->GetSendCode());
    parcel.WriteUInt32(packet->GetSoftwareVersion());
    parcel.WriteString(packet->GetSchema());
    parcel.WriteInt(packet->GetSecLabel());
    parcel.WriteInt(packet->GetSecFlag());
    parcel.WriteUInt32(packet->GetSchemaType());
    parcel.WriteUInt64(packet->GetDbCreateTime());
    int errCode = DbAbility::Serialize(parcel, packet->GetDbAbility());
    if (parcel.IsError() || errCode != E_OK) {
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

int AbilitySync::AckPacketSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
{
    const AbilitySyncAckPacket *packet = inMsg->GetObject<AbilitySyncAckPacket>();
    if ((packet == nullptr) || (length != packet->CalculateLen())) {
        return -E_INVALID_ARGS;
    }

    Parcel parcel(buffer, length);
    parcel.WriteUInt32(ABILITY_SYNC_VERSION_V1);
    parcel.WriteUInt32(SOFTWARE_VERSION_CURRENT);
    parcel.WriteInt(packet->GetAckCode());
    parcel.WriteString(packet->GetSchema());
    parcel.WriteInt(packet->GetSecLabel());
    parcel.WriteInt(packet->GetSecFlag());
    parcel.WriteUInt32(packet->GetSchemaType());
    parcel.WriteUInt32(packet->GetPermitSync());
    parcel.WriteUInt32(packet->GetRequirePeerConvert());
    parcel.WriteUInt64(packet->GetDbCreateTime());
    int errCode = DbAbility::Serialize(parcel, packet->GetDbAbility());
    if (parcel.IsError() || errCode != E_OK) {
        return -E_PARSE_FAIL;
    }
    errCode = packet->GetRelationalSyncOpinion().SerializeData(parcel, SOFTWARE_VERSION_CURRENT);
    if (parcel.IsError() || errCode != E_OK) {
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

int AbilitySync::RequestPacketDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg)
{
    auto *packet = new (std::nothrow) AbilitySyncRequestPacket();
    if (packet == nullptr) {
        return -E_OUT_OF_MEMORY;
    }

    Parcel parcel(const_cast<uint8_t *>(buffer), length);
    uint32_t version = 0;
    uint32_t softwareVersion = 0;
    std::string schema;
    int32_t sendCode = 0;
    int errCode = -E_PARSE_FAIL;

    parcel.ReadUInt32(version);
    if (parcel.IsError()) {
        goto ERROR_OUT;
    }
    packet->SetProtocolVersion(version);
    if (version > ABILITY_SYNC_VERSION_V1) {
        packet->SetSendCode(-E_VERSION_NOT_SUPPORT);
        errCode = inMsg->SetExternalObject<>(packet);
        if (errCode != E_OK) {
            goto ERROR_OUT;
        }
        return errCode;
    }
    parcel.ReadInt(sendCode);
    parcel.ReadUInt32(softwareVersion);
    parcel.ReadString(schema);
    errCode = RequestPacketDeSerializationTailPart(parcel, packet, softwareVersion);
    if (parcel.IsError() || errCode != E_OK) {
        goto ERROR_OUT;
    }
    packet->SetSendCode(sendCode);
    packet->SetSoftwareVersion(softwareVersion);
    packet->SetSchema(schema);

    errCode = inMsg->SetExternalObject<>(packet);
    if (errCode == E_OK) {
        return E_OK;
    }

ERROR_OUT:
    delete packet;
    packet = nullptr;
    return errCode;
}

int AbilitySync::RequestPacketDeSerializationTailPart(Parcel &parcel, AbilitySyncRequestPacket *packet,
    uint32_t version)
{
    if (!parcel.IsError() && version > SOFTWARE_VERSION_RELEASE_2_0) {
        int32_t secLabel = 0;
        int32_t secFlag = 0;
        uint32_t schemaType = 0;
        parcel.ReadInt(secLabel);
        parcel.ReadInt(secFlag);
        parcel.ReadUInt32(schemaType);
        packet->SetSecLabel(secLabel);
        packet->SetSecFlag(secFlag);
        packet->SetSchemaType(schemaType);
    }
    if (!parcel.IsError() && version > SOFTWARE_VERSION_RELEASE_3_0) {
        uint64_t dbCreateTime = 0;
        parcel.ReadUInt64(dbCreateTime);
        packet->SetDbCreateTime(dbCreateTime);
    }
    DbAbility remoteDbAbility;
    int errCode = DbAbility::DeSerialize(parcel, remoteDbAbility);
    if (errCode != E_OK) {
        LOGE("[AbilitySync] request packet DeSerializ failed.");
        return errCode;
    }
    packet->SetDbAbility(remoteDbAbility);
    return E_OK;
}

int AbilitySync::AckPacketDeSerializationTailPart(Parcel &parcel, AbilitySyncAckPacket *packet, uint32_t version)
{
    if (!parcel.IsError() && version > SOFTWARE_VERSION_RELEASE_2_0) {
        int32_t secLabel = 0;
        int32_t secFlag = 0;
        uint32_t schemaType = 0;
        uint32_t permitSync = 0;
        uint32_t requirePeerConvert = 0;
        parcel.ReadInt(secLabel);
        parcel.ReadInt(secFlag);
        parcel.ReadUInt32(schemaType);
        parcel.ReadUInt32(permitSync);
        parcel.ReadUInt32(requirePeerConvert);
        packet->SetSecLabel(secLabel);
        packet->SetSecFlag(secFlag);
        packet->SetSchemaType(schemaType);
        packet->SetPermitSync(permitSync);
        packet->SetRequirePeerConvert(requirePeerConvert);
    }
    if (!parcel.IsError() && version > SOFTWARE_VERSION_RELEASE_3_0) {
        uint64_t dbCreateTime = 0;
        parcel.ReadUInt64(dbCreateTime);
        packet->SetDbCreateTime(dbCreateTime);
    }
    DbAbility remoteDbAbility;
    int errCode = DbAbility::DeSerialize(parcel, remoteDbAbility);
    if (errCode != E_OK) {
        LOGE("[AbilitySync] ack packet DeSerializ failed.");
        return errCode;
    }
    packet->SetDbAbility(remoteDbAbility);
    RelationalSyncOpinion relationalSyncOpinion;
    errCode = RelationalSyncOpinion::DeserializeData(parcel, relationalSyncOpinion);
    if (errCode != E_OK) {
        LOGE("[AbilitySync] ack packet DeSerializ RelationalSyncOpinion failed.");
        return errCode;
    }
    packet->SetRelationalSyncOpinion(relationalSyncOpinion);
    return E_OK;
}

int AbilitySync::AckPacketDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg)
{
    auto *packet = new (std::nothrow) AbilitySyncAckPacket();
    if (packet == nullptr) {
        return -E_OUT_OF_MEMORY;
    }

    Parcel parcel(const_cast<uint8_t *>(buffer), length);
    uint32_t version = 0;
    uint32_t softwareVersion = 0;
    int32_t ackCode = E_OK;
    std::string schema;
    int errCode;
    parcel.ReadUInt32(version);
    if (parcel.IsError()) {
        LOGE("[AbilitySync][RequestDeSerialization] read version failed!");
        errCode = -E_PARSE_FAIL;
        goto ERROR_OUT;
    }
    packet->SetProtocolVersion(version);
    if (version > ABILITY_SYNC_VERSION_V1) {
        packet->SetAckCode(-E_VERSION_NOT_SUPPORT);
        errCode = inMsg->SetExternalObject<>(packet);
        if (errCode != E_OK) {
            goto ERROR_OUT;
        }
        return errCode;
    }
    parcel.ReadUInt32(softwareVersion);
    parcel.ReadInt(ackCode);
    parcel.ReadString(schema);
    errCode = AckPacketDeSerializationTailPart(parcel, packet, softwareVersion);
    if (parcel.IsError() || errCode != E_OK) {
        LOGE("[AbilitySync][RequestDeSerialization] DeSerialization failed!");
        errCode = -E_PARSE_FAIL;
        goto ERROR_OUT;
    }
    packet->SetSoftwareVersion(softwareVersion);
    packet->SetAckCode(ackCode);
    packet->SetSchema(schema);
    errCode = inMsg->SetExternalObject<>(packet);
    if (errCode == E_OK) {
        return E_OK;
    }

ERROR_OUT:
    delete packet;
    packet = nullptr;
    return errCode;
}

int AbilitySync::SetAbilityRequestBodyInfo(AbilitySyncRequestPacket &packet, uint16_t remoteCommunicatorVersion) const
{
    uint64_t dbCreateTime;
    int errCode =
        (static_cast<SyncGenericInterface *>(storageInterface_))->GetDatabaseCreateTimeStamp(dbCreateTime);
    if (errCode != E_OK) {
        LOGE("[AbilitySync][FillAbilityRequest] GetDatabaseCreateTimeStamp failed, err %d", errCode);
        return errCode;
    }
    SecurityOption option;
    GetPacketSecOption(option);
    std::string schemaStr;
    uint32_t schemaType = 0;
    if (IsSingleKvVer()) {
        SchemaObject schemaObj = (static_cast<SingleVerKvDBSyncInterface *>(storageInterface_))->GetSchemaInfo();
        schemaStr = schemaObj.ToSchemaString();
        schemaType = static_cast<uint32_t>(schemaObj.GetSchemaType());
    } else if (IsSingleRelationalVer()) {
        auto schemaObj = (static_cast<RelationalDBSyncInterface *>(storageInterface_))->GetSchemaInfo();
        schemaStr = schemaObj.ToSchemaString();
        schemaType = static_cast<uint32_t>(schemaObj.GetSchemaType());
    }
    DbAbility dbAbility;
    errCode = GetDbAbilityInfo(dbAbility);
    if (errCode != E_OK) {
        LOGE("[AbilitySync][FillAbilityRequest] GetDbAbility failed, err %d", errCode);
        return errCode;
    }
    // 102 version is forbidden to sync with 103 json-schema or flatbuffer-schema
    // so schema should put null string while remote is 102 version to avoid this bug.
    if (remoteCommunicatorVersion == 1) {
        packet.SetSchema("");
        packet.SetSchemaType(0);
    } else {
        packet.SetSchema(schemaStr);
        packet.SetSchemaType(schemaType);
    }
    packet.SetProtocolVersion(ABILITY_SYNC_VERSION_V1);
    packet.SetSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    packet.SetSecLabel(option.securityLabel);
    packet.SetSecFlag(option.securityFlag);
    packet.SetDbCreateTime(dbCreateTime);
    packet.SetDbAbility(dbAbility);
    LOGI("[AbilitySync][FillRequest] ver=%u,Lab=%d,Flag=%d,dbCreateTime=%llu", SOFTWARE_VERSION_CURRENT,
        option.securityLabel, option.securityFlag, dbCreateTime);
    return E_OK;
}

int AbilitySync::SetAbilityAckBodyInfo(AbilitySyncAckPacket &ackPacket, int ackCode, bool isAckNotify) const
{
    int errCode = E_OK;
    ackPacket.SetProtocolVersion(ABILITY_SYNC_VERSION_V1);
    ackPacket.SetSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    if (!isAckNotify) {
        SecurityOption option;
        GetPacketSecOption(option);
        ackPacket.SetSecLabel(option.securityLabel);
        ackPacket.SetSecFlag(option.securityFlag);
        uint64_t dbCreateTime = 0;
        errCode =
            (static_cast<SyncGenericInterface *>(storageInterface_))->GetDatabaseCreateTimeStamp(dbCreateTime);
        if (errCode != E_OK) {
            LOGE("[AbilitySync][SyncStart] GetDatabaseCreateTimeStamp failed, err %d", errCode);
            ackCode = errCode;
        }
        DbAbility dbAbility;
        errCode = GetDbAbilityInfo(dbAbility);
        if (errCode != E_OK) {
            LOGE("[AbilitySync][FillAbilityRequest] GetDbAbility failed, err %d", errCode);
            return errCode;
        }
        ackPacket.SetDbCreateTime(dbCreateTime);
        ackPacket.SetDbAbility(dbAbility);
    }
    ackPacket.SetAckCode(ackCode);
    return E_OK;
}

void AbilitySync::SetAbilityAckSchemaInfo(AbilitySyncAckPacket &ackPacket, const ISchema &schemaObj) const
{
    ackPacket.SetSchema(schemaObj.ToSchemaString());
    ackPacket.SetSchemaType(static_cast<uint32_t>(schemaObj.GetSchemaType()));
}

void AbilitySync::SetAbilityAckSyncOpinionInfo(AbilitySyncAckPacket &ackPacket, SyncOpinion localOpinion) const
{
    ackPacket.SetPermitSync(localOpinion.permitSync);
    ackPacket.SetRequirePeerConvert(localOpinion.requirePeerConvert);
}

int AbilitySync::GetDbAbilityInfo(DbAbility &dbAbility) const
{
    int errCode = E_OK;
    for (const auto &item : ABILITYBITS) {
        errCode = dbAbility.SetAbilityItem(item, SUPPORT_MARK);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return errCode;
}

int AbilitySync::AckMsgCheck(const Message *message, ISyncTaskContext *context) const
{
    if (message == nullptr || context == nullptr) {
        return -E_INVALID_ARGS;
    }
    if (message->GetErrorNo() == E_FEEDBACK_UNKNOWN_MESSAGE) {
        LOGE("[AbilitySync][AckMsgCheck] Remote device dose not support this message id");
        context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_EARLIEST);
        context->SetTaskErrCode(-E_FEEDBACK_UNKNOWN_MESSAGE);
        return -E_FEEDBACK_UNKNOWN_MESSAGE;
    }
    if (message->GetErrorNo() == E_FEEDBACK_COMMUNICATOR_NOT_FOUND) {
        LOGE("[AbilitySync][AckMsgCheck] Remote db is closed");
        context->SetTaskErrCode(-E_FEEDBACK_COMMUNICATOR_NOT_FOUND);
        return -E_FEEDBACK_COMMUNICATOR_NOT_FOUND;
    }
    const AbilitySyncAckPacket *packet = message->GetObject<AbilitySyncAckPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    int ackCode = packet->GetAckCode();
    if (ackCode != E_OK) {
        LOGE("[AbilitySync][AckMsgCheck] received a errCode %d", ackCode);
        context->SetTaskErrCode(ackCode);
        return ackCode;
    }
    return E_OK;
}

bool AbilitySync::IsSingleKvVer() const
{
    return storageInterface_->GetInterfaceType() == ISyncInterface::SYNC_SVD;
}
bool AbilitySync::IsSingleRelationalVer() const
{
#ifdef RELATIONAL_STORE
    return storageInterface_->GetInterfaceType() == ISyncInterface::SYNC_RELATION;
#elif
    return false;
#endif
}

int AbilitySync::HandleRequestRecv(const Message *message, ISyncTaskContext *context, bool isCompatible)
{
    const AbilitySyncRequestPacket *packet = message->GetObject<AbilitySyncRequestPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    uint32_t remoteSoftwareVersion = packet->GetSoftwareVersion();
    int ackCode;
    std::string schema = packet->GetSchema();
    if (remoteSoftwareVersion <= SOFTWARE_VERSION_RELEASE_2_0) {
        LOGI("[AbilitySync][RequestRecv] remote version = %u, CheckSchemaCompatible = %d",
            remoteSoftwareVersion, isCompatible);
        return SendAckWithEmptySchema(message, E_OK, false);
    }
    HandleVersionV3RequestParam(packet, context, schema);
    if (SecLabelCheck(packet)) {
        ackCode = E_OK;
    } else {
        ackCode = -E_SECURITY_OPTION_CHECK_ERROR;
    }
    if (ackCode == E_OK && remoteSoftwareVersion > SOFTWARE_VERSION_RELEASE_3_0) {
        ackCode = metadata_->SetDbCreateTime(deviceId_, packet->GetDbCreateTime(), true);
    }
    AbilitySyncAckPacket ackPacket;
    if (IsSingleRelationalVer()) {
        ackPacket.SetRelationalSyncOpinion(MakeRelationSyncOpnion(packet, schema));
    } else {
        SetAbilityAckSyncOpinionInfo(ackPacket, MakeKvSyncOpnion(packet, schema));
    }
    LOGI("[AbilitySync][RequestRecv] remote dev=%s,ver=%u,schemaCompatible=%d", STR_MASK(deviceId_),
        remoteSoftwareVersion, isCompatible);
    return SendAck(message, ackCode, false, ackPacket);
}

int AbilitySync::SendAck(const Message *message, int ackCode, bool isAckNotify, AbilitySyncAckPacket &ackPacket)
{
    int errCode = SetAbilityAckBodyInfo(ackPacket, ackCode, isAckNotify);
    if (errCode != E_OK) {
        return errCode;
    }
    if (IsSingleRelationalVer()) {
        auto schemaObj = (static_cast<RelationalDBSyncInterface *>(storageInterface_))->GetSchemaInfo();
        SetAbilityAckSchemaInfo(ackPacket, schemaObj);
    } else if(IsSingleKvVer()) {
        SchemaObject schemaObject = static_cast<SingleVerKvDBSyncInterface *>(storageInterface_)->GetSchemaInfo();
        SetAbilityAckSchemaInfo(ackPacket, schemaObject);
    }
    return SendAck(message, ackPacket, isAckNotify);
}

int AbilitySync::SendAckWithEmptySchema(const Message *message, int ackCode,
    bool isAckNotify)
{
    AbilitySyncAckPacket ackPacket;
    int errCode = SetAbilityAckBodyInfo(ackPacket, ackCode, isAckNotify);
    if (errCode != E_OK) {
        return errCode;
    }
    SetAbilityAckSchemaInfo(ackPacket, SchemaObject());
    return SendAck(message, ackPacket, isAckNotify);
}

int AbilitySync::SendAck(const Message *inMsg, const AbilitySyncAckPacket &ackPacket, bool isAckNotify)
{
    Message *ackMessage = new (std::nothrow) Message(ABILITY_SYNC_MESSAGE);
    if (ackMessage == nullptr) {
        LOGE("[AbilitySync][SendAck] message create failed, may be memleak!");
        return -E_OUT_OF_MEMORY;
    }
    int errCode = ackMessage->SetCopiedObject<>(ackPacket);
    if (errCode != E_OK) {
        LOGE("[AbilitySync][SendAck] SetCopiedObject failed, err %d", errCode);
        delete ackMessage;
        ackMessage = nullptr;
        return errCode;
    }
    (!isAckNotify) ? ackMessage->SetMessageType(TYPE_RESPONSE) : ackMessage->SetMessageType(TYPE_NOTIFY);
    ackMessage->SetTarget(deviceId_);
    ackMessage->SetSessionId(inMsg->GetSessionId());
    ackMessage->SetSequenceId(inMsg->GetSequenceId());
    errCode = communicator_->SendMessage(deviceId_, ackMessage, false, SEND_TIME_OUT);
    if (errCode != E_OK) {
        LOGE("[AbilitySync][SendAck] SendPacket failed, err %d", errCode);
        delete ackMessage;
        ackMessage = nullptr;
    }
    return errCode;
}

SyncOpinion AbilitySync::MakeKvSyncOpnion(const AbilitySyncRequestPacket *packet, const std::string &remoteSchema) const
{
    uint8_t remoteSchemaType = packet->GetSchemaType();
    SchemaObject localSchema = (static_cast<SingleVerKvDBSyncInterface *>(storageInterface_))->GetSchemaInfo();
    SyncOpinion localSyncOpinion = SchemaObject::MakeLocalSyncOpinion(localSchema, remoteSchema, remoteSchemaType);
    return localSyncOpinion;
}

RelationalSyncOpinion AbilitySync::MakeRelationSyncOpnion(const AbilitySyncRequestPacket *packet,
    const std::string &remoteSchema) const
{
    uint8_t remoteSchemaType = packet->GetSchemaType();
    RelationalSchemaObject localSchema = (static_cast<RelationalDBSyncInterface *>(storageInterface_))->GetSchemaInfo();
    return RelationalSchemaObject::MakeLocalSyncOpinion(localSchema, remoteSchema, remoteSchemaType);
}

void AbilitySync::HandleKvAckSchemaParam(const AbilitySyncAckPacket *recvPacket,
    ISyncTaskContext *context, AbilitySyncAckPacket &sendPacket) const
{
    std::string remoteSchema = recvPacket->GetSchema();
    uint8_t remoteSchemaType = recvPacket->GetSchemaType();
    bool permitSync = static_cast<bool>(recvPacket->GetPermitSync());
    bool requirePeerConvert = static_cast<bool>(recvPacket->GetRequirePeerConvert());
    SyncOpinion remoteOpinion = {permitSync, requirePeerConvert, true};
    SchemaObject localSchema = (static_cast<SingleVerKvDBSyncInterface *>(storageInterface_))->GetSchemaInfo();
    SyncOpinion syncOpinion = SchemaObject::MakeLocalSyncOpinion(localSchema, remoteSchema, remoteSchemaType);
    SyncStrategy localStrategy = SchemaObject::ConcludeSyncStrategy(syncOpinion, remoteOpinion);
    SetAbilityAckSyncOpinionInfo(sendPacket, syncOpinion);
    (static_cast<SingleVerKvSyncTaskContext *>(context))->SetSyncStrategy(localStrategy);
}

int AbilitySync::HandleRelationAckSchemaParam(const AbilitySyncAckPacket *recvPacket, AbilitySyncAckPacket &sendPacket,
    ISyncTaskContext *context, bool sendOpinion) const
{
    std::string remoteSchema = recvPacket->GetSchema();
    uint8_t remoteSchemaType = recvPacket->GetSchemaType();
    auto localSchema = (static_cast<RelationalDBSyncInterface *>(storageInterface_))->GetSchemaInfo();
    auto localOpinion = RelationalSchemaObject::MakeLocalSyncOpinion(localSchema, remoteSchema, remoteSchemaType);
    auto localStrategy = RelationalSchemaObject::ConcludeSyncStrategy(localOpinion,
        recvPacket->GetRelationalSyncOpinion());
    (static_cast<SingleVerRelationalSyncTaskContext *>(context))->SetRelationalSyncStrategy(localStrategy);
    int errCode = (static_cast<RelationalDBSyncInterface *>(storageInterface_))->
        CreateDistributedDeviceTable(context->GetDeviceId(), localStrategy);
    if (errCode != E_OK) {
        LOGE("[AbilitySync][AckRecv] create distributed device table failed,errCode=%d", errCode);
    }
    if (sendOpinion) {
        sendPacket.SetRelationalSyncOpinion(localOpinion);
    }
    return errCode;
}
} // namespace DistributedDB