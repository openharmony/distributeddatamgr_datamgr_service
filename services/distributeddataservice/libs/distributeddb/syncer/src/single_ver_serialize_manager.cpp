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

#include "single_ver_serialize_manager.h"

#include "db_common.h"
#include "generic_single_ver_kv_entry.h"
#include "icommunicator.h"
#include "log_print.h"
#include "message_transform.h"
#include "parcel.h"
#include "remote_executor_packet.h"
#include "sync_types.h"
#include "version.h"

namespace DistributedDB {
std::mutex SingleVerSerializeManager::handlesLock_;
std::map<uint32_t, TransformFunc> SingleVerSerializeManager::messageHandles_;
int SingleVerSerializeManager::Serialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
{
    if ((buffer == nullptr) || !(IsPacketValid(inMsg))) {
        return -E_MESSAGE_ID_ERROR;
    }
    SerializeFunc serializeFunc = nullptr;
    {
        std::lock_guard<std::mutex> autoLock(handlesLock_);
        if (messageHandles_.find(inMsg->GetMessageId()) != messageHandles_.end()) {
            serializeFunc = messageHandles_.at(inMsg->GetMessageId()).serializeFunc;
        }
    }
    if (serializeFunc != nullptr) {
        return serializeFunc(buffer, length, inMsg);
    }

    if (inMsg->GetMessageId() == CONTROL_SYNC_MESSAGE) {
        return ControlSerialization(buffer, length, inMsg);
    }
    return DataSerialization(buffer, length, inMsg);
}

int SingleVerSerializeManager::DataSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
{
    switch (inMsg->GetMessageType()) {
        case TYPE_REQUEST:
            return DataPacketSerialization(buffer, length, inMsg);
        case TYPE_RESPONSE:
        case TYPE_NOTIFY:
            return AckPacketSerialization(buffer, length, inMsg);
        default:
            return -E_MESSAGE_TYPE_ERROR;
    }
}

int SingleVerSerializeManager::ControlSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
{
    switch (inMsg->GetMessageType()) {
        case TYPE_REQUEST:
            return ControlPacketSerialization(buffer, length, inMsg);
        case TYPE_RESPONSE:
            return AckControlPacketSerialization(buffer, length, inMsg);
        default:
            return -E_MESSAGE_TYPE_ERROR;
    }
}

int SingleVerSerializeManager::DeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg)
{
    if ((buffer == nullptr) || !(IsPacketValid(inMsg))) {
        return -E_MESSAGE_ID_ERROR;
    }
    DeserializeFunc deserializeFunc = nullptr;
    {
        std::lock_guard<std::mutex> autoLock(handlesLock_);
        if (messageHandles_.find(inMsg->GetMessageId()) != messageHandles_.end()) {
            deserializeFunc = messageHandles_.at(inMsg->GetMessageId()).deserializeFunc;
        }
    }
    if (deserializeFunc != nullptr) {
        return deserializeFunc(buffer, length, inMsg);
    }
    if (inMsg->GetMessageId() == CONTROL_SYNC_MESSAGE) {
        return ControlDeSerialization(buffer, length, inMsg);
    }
    return DataDeSerialization(buffer, length, inMsg);
}

int SingleVerSerializeManager::DataDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg)
{
    switch (inMsg->GetMessageType()) {
        case TYPE_REQUEST:
            return DataPacketDeSerialization(buffer, length, inMsg);
        case TYPE_RESPONSE:
        case TYPE_NOTIFY:
            return AckPacketDeSerialization(buffer, length, inMsg);
        default:
            return -E_MESSAGE_TYPE_ERROR;
    }
}

int SingleVerSerializeManager::ControlDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg)
{
    switch (inMsg->GetMessageType()) {
        case TYPE_REQUEST:
            return ControlPacketDeSerialization(buffer, length, inMsg);
        case TYPE_RESPONSE:
            return AckControlPacketDeSerialization(buffer, length, inMsg);
        default:
            return -E_MESSAGE_TYPE_ERROR;
    }
}

uint32_t SingleVerSerializeManager::CalculateLen(const Message *inMsg)
{
    if (!(IsPacketValid(inMsg))) {
        return 0;
    }
    ComputeLengthFunc computeFunc = nullptr;
    {
        std::lock_guard<std::mutex> autoLock(handlesLock_);
        if (messageHandles_.find(inMsg->GetMessageId()) != messageHandles_.end()) {
            computeFunc = messageHandles_.at(inMsg->GetMessageId()).computeFunc;
        }
    }
    if (computeFunc != nullptr) {
        return computeFunc(inMsg);
    }
    if (inMsg->GetMessageId() == CONTROL_SYNC_MESSAGE) {
        return CalculateControlLen(inMsg);
    }
    return CalculateDataLen(inMsg);
}

uint32_t SingleVerSerializeManager::CalculateDataLen(const Message *inMsg)
{
    uint32_t len = 0;
    int errCode;
    switch (inMsg->GetMessageType()) {
        case TYPE_REQUEST:
            errCode = DataPacketCalculateLen(inMsg, len);
            if (errCode != E_OK) {
                LOGE("[CalculateDataLen] calculate data request packet len failed, errCode=%d", errCode);
                return 0;
            }
            return len;
        case TYPE_RESPONSE:
        case TYPE_NOTIFY:
            errCode = AckPacketCalculateLen(inMsg, len);
            if (errCode != E_OK) {
                LOGE("[CalculateDataLen] calculate data notify packet len failed errCode=%d", errCode);
                return 0;
            }
            return len;
        default:
            return 0;
    }
}

uint32_t SingleVerSerializeManager::CalculateControlLen(const Message *inMsg)
{
    uint32_t len = 0;
    int errCode;
    switch (inMsg->GetMessageType()) {
        case TYPE_REQUEST:
            errCode = ControlPacketCalculateLen(inMsg, len);
            if (errCode != E_OK) {
                LOGE("[CalculateControlLen] calculate control request packet len failed, errCode=%d", errCode);
                return 0;
            }
            return len;
        case TYPE_RESPONSE:
        case TYPE_NOTIFY:
            errCode = AckControlPacketCalculateLen(inMsg, len);
            if (errCode != E_OK) {
                LOGE("[CalculateControlLen] calculate control request packet len failed, errCode=%d", errCode);
                return 0;
            }
            return len;
        default:
            return 0;
    }
}

int SingleVerSerializeManager::RegisterTransformFunc()
{
    int errCode = RegisterCommunicatorTransformFunc();
    RegisterInnerTransformFunc();
    return errCode;
}

int SingleVerSerializeManager::DataPacketSyncerPartSerialization(Parcel &parcel, const DataRequestPacket *packet)
{
    parcel.WriteUInt64(packet->GetEndWaterMark());
    parcel.WriteUInt64(packet->GetLocalWaterMark());
    parcel.WriteUInt64(packet->GetPeerWaterMark());
    parcel.WriteInt(packet->GetSendCode());
    parcel.WriteInt(packet->GetMode());
    parcel.WriteUInt32(packet->GetSessionId());
    parcel.WriteVector<uint64_t>(packet->GetReserved());
    if (parcel.IsError()) {
        return -E_PARSE_FAIL;
    }
    if (packet->GetVersion() > SOFTWARE_VERSION_RELEASE_2_0) {
        parcel.WriteUInt32(packet->GetFlag());
        if (parcel.IsError()) {
            return -E_PARSE_FAIL;
        }
    }
    parcel.EightByteAlign();
    return E_OK;
}

int SingleVerSerializeManager::DataPacketSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
{
    auto packet = inMsg->GetObject<DataRequestPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    Parcel parcel(buffer, length);

    // version
    int errCode = parcel.WriteUInt32(packet->GetVersion());
    if (errCode != E_OK) {
        LOGE("[DataPacketSerialization] Serialize version failed");
        return errCode;
    }
    // sendDataItems
    errCode = GenericSingleVerKvEntry::SerializeDatas(
        (packet->IsCompressData() ? std::vector<SendDataItem> {} : packet->GetData()), parcel, packet->GetVersion());
    if (errCode != E_OK) {
        LOGE("[DataPacketSerialization] Serialize Data failed");
        return errCode;
    }

    // data sync
    errCode = DataPacketSyncerPartSerialization(parcel, packet);
    if (errCode != E_OK) {
        LOGE("[DataPacketSerialization] Serialize Data failed");
        return errCode;
    }
    if (inMsg->GetMessageId() == QUERY_SYNC_MESSAGE) {
        errCode = DataPacketQuerySyncSerialization(parcel, packet); // for query sync
        if (errCode != E_OK) {
            return errCode;
        }
    }
    if (packet->IsCompressData()) {
        // serialize compress data
        errCode = GenericSingleVerKvEntry::SerializeCompressedDatas(packet->GetData(), packet->GetCompressData(),
            parcel, packet->GetVersion(), packet->GetCompressAlgo());
        if (errCode != E_OK) {
            LOGE("[DataPacketSerialization] Serialize compress Data failed");
            return errCode;
        }
    }
    // flag mask add in 103
    if (packet->GetVersion() < SOFTWARE_VERSION_RELEASE_3_0 || !packet->IsExtraConditionData()) {
        return E_OK;
    }
    return DataPacketExtraConditionsSerialization(parcel, packet);
}

int SingleVerSerializeManager::DataPacketQuerySyncSerialization(Parcel &parcel, const DataRequestPacket *packet)
{
    // deleted record send watermark
    int errCode = parcel.WriteUInt64(packet->GetDeletedWaterMark());
    if (errCode != E_OK) {
        LOGE("[QuerySerialization] Serialize deleted record send watermark failed!");
        return errCode;
    }

    // query identify
    QuerySyncObject queryObj = packet->GetQuery();
    errCode = parcel.WriteString(packet->GetQueryId());
    if (errCode != E_OK) {
        LOGE("[QuerySerialization] Serialize query id failed!");
        return errCode;
    }
    if ((packet->GetVersion() > SOFTWARE_VERSION_RELEASE_4_0) || packet->GetMode() != QUERY_PUSH) {
        // need to check.
        errCode = queryObj.SerializeData(parcel, SOFTWARE_VERSION_CURRENT);
    }
    return errCode;
}

int SingleVerSerializeManager::DataPacketCalculateLen(const Message *inMsg, uint32_t &len)
{
    const DataRequestPacket *packet = inMsg->GetObject<DataRequestPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }

    len = packet->CalculateLen(inMsg->GetMessageId());
    return E_OK;
}

int SingleVerSerializeManager::AckPacketCalculateLen(const Message *inMsg, uint32_t &len)
{
    const DataAckPacket *packet = inMsg->GetObject<DataAckPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }

    len = packet->CalculateLen();
    return E_OK;
}

bool SingleVerSerializeManager::IsPacketValid(const Message *inMsg)
{
    if (inMsg == nullptr) {
        return false;
    }

    int msgType = inMsg->GetMessageType();
    if (msgType != TYPE_REQUEST && msgType != TYPE_RESPONSE && msgType != TYPE_NOTIFY) {
        LOGE("[DataSync][IsPacketValid] Message type ERROR! message type=%d", msgType);
        return false;
    }
    return true;
}

int SingleVerSerializeManager::AckPacketSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
{
    const DataAckPacket *packet = inMsg->GetObject<DataAckPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }

    Parcel parcel(buffer, length);
    parcel.WriteUInt32(packet->GetVersion());
    if (parcel.IsError()) {
        return -E_PARSE_FAIL;
    }
    // now V1 compatible for softWareVersion :{101, 102}
    return AckPacketSyncerPartSerializationV1(parcel, packet);
}

int SingleVerSerializeManager::AckPacketSyncerPartSerializationV1(Parcel &parcel, const DataAckPacket *packet)
{
    parcel.WriteUInt64(packet->GetData());
    parcel.WriteInt(packet->GetRecvCode());
    parcel.WriteVector<uint64_t>(packet->GetReserved());
    if (parcel.IsError()) {
        return -E_PARSE_FAIL;
    }
    parcel.EightByteAlign();
    return E_OK;
}

int SingleVerSerializeManager::DataPacketDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg)
{
    std::vector<SendDataItem> dataItems;
    uint32_t version;
    Parcel parcel(const_cast<uint8_t *>(buffer), length);
    uint32_t packLen = parcel.ReadUInt32(version);
    if (parcel.IsError()) {
        return -E_PARSE_FAIL;
    }

    if (version > SOFTWARE_VERSION_CURRENT) {
        return -E_VERSION_NOT_SUPPORT;
    }

    packLen += static_cast<uint32_t>(GenericSingleVerKvEntry::DeSerializeDatas(dataItems, parcel));
    if (parcel.IsError()) {
        return -E_PARSE_FAIL;
    }

    auto packet = new (std::nothrow) DataRequestPacket();
    if (packet == nullptr) {
        return -E_OUT_OF_MEMORY;
    }

    packet->SetVersion(version);
    packet->SetData(dataItems);
    int errCode = DataPacketSyncerPartDeSerialization(parcel, packet, packLen, length, version);
    if (errCode != E_OK) {
        goto ERROR;
    }
    if (inMsg->GetMessageId() == QUERY_SYNC_MESSAGE) {
        errCode = DataPacketQuerySyncDeSerialization(parcel, packet);
        if (errCode != E_OK) {
            goto ERROR;
        }
    }
    if (packet->IsCompressData()) {
        errCode = DataPacketCompressDataDeSerialization(parcel, packet);
        if (errCode != E_OK) {
            goto ERROR;
        }
    }
    errCode = DataPacketExtraConditionsDeserialization(parcel, packet);
    if (errCode != E_OK) {
        goto ERROR;
    }
    errCode = inMsg->SetExternalObject<>(packet);
    if (errCode != E_OK) {
        goto ERROR;
    }
    return errCode;

ERROR:
    delete packet;
    packet = nullptr;
    return errCode;
}

int SingleVerSerializeManager::DataPacketQuerySyncDeSerialization(Parcel &parcel, DataRequestPacket *packet)
{
    WaterMark deletedWatermark = 0;
    parcel.ReadUInt64(deletedWatermark);
    std::string queryId;
    parcel.ReadString(queryId);
    if (parcel.IsError()) {
        return -E_PARSE_FAIL;
    }
    // query identify
    QuerySyncObject querySyncObj;
    int errCode = E_OK;
    // for version 105, query is always sent.
    if ((packet->GetVersion() > SOFTWARE_VERSION_RELEASE_4_0) || packet->GetMode() != QUERY_PUSH) {
        // need to check.
        errCode = QuerySyncObject::DeSerializeData(parcel, querySyncObj);
    }
    if (errCode != E_OK) {
        LOGI("[SingleVerSerializeManager] DeSerializeData object failed.");
        return errCode;
    }
    packet->SetDeletedWaterMark(deletedWatermark);
    packet->SetQueryId(queryId);
    if ((packet->GetVersion() > SOFTWARE_VERSION_RELEASE_4_0) || packet->GetMode() != QUERY_PUSH) {
        packet->SetQuery(querySyncObj);
    }
    return E_OK;
}

int SingleVerSerializeManager::DataPacketCompressDataDeSerialization(Parcel &parcel, DataRequestPacket *packet)
{
    std::vector<SendDataItem> originalData;
    int errCode = GenericSingleVerKvEntry::DeSerializeCompressedDatas(originalData, parcel);
    if (errCode != E_OK) {
        LOGE("[SingleVerSerializeManager] DeSerializeComptressData failed, errCode=%d", errCode);
        return errCode;
    }
    packet->SetData(originalData);
    return E_OK;
}

int SingleVerSerializeManager::DataPacketSyncerPartDeSerialization(Parcel &parcel, DataRequestPacket *packet,
    uint32_t packLen, uint32_t length, uint32_t version)
{
    WaterMark waterMark;
    WaterMark localWaterMark;
    WaterMark peerWaterMark;
    int32_t sendCode;
    int32_t mode;
    uint32_t sessionId;
    uint32_t flag = 0;
    std::vector<uint64_t> reserved;

    packLen += parcel.ReadUInt64(waterMark);
    packLen += parcel.ReadUInt64(localWaterMark);
    packLen += parcel.ReadUInt64(peerWaterMark);
    packLen += parcel.ReadInt(sendCode);
    packLen += parcel.ReadInt(mode);
    packLen += parcel.ReadUInt32(sessionId);
    packLen += parcel.ReadVector<uint64_t>(reserved);
    if (version > SOFTWARE_VERSION_RELEASE_2_0) {
        packLen += parcel.ReadUInt32(flag);
        packet->SetFlag(flag);
    }
    packLen = Parcel::GetEightByteAlign(packLen);
    if (parcel.IsError()) {
        LOGE("[DataSync][DataPacketDeSerialization] deserialize failed! input len=%" PRIu32 ",packLen=%" PRIu32,
            length, packLen);
        return -E_LENGTH_ERROR;
    }
    parcel.EightByteAlign();
    packet->SetEndWaterMark(waterMark);
    packet->SetLocalWaterMark(localWaterMark);
    packet->SetPeerWaterMark(peerWaterMark);
    packet->SetSendCode(sendCode);
    packet->SetMode(mode);
    packet->SetSessionId(sessionId);
    packet->SetReserved(reserved);
    return E_OK;
}

int SingleVerSerializeManager::AckPacketDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg)
{
    DataAckPacket packet;
    Parcel parcel(const_cast<uint8_t *>(buffer), length);
    uint32_t version;

    parcel.ReadUInt32(version);
    if (parcel.IsError()) {
        return -E_INVALID_ARGS;
    }
    if (version > SOFTWARE_VERSION_CURRENT) {
        packet.SetVersion(version);
        packet.SetRecvCode(-E_VERSION_NOT_SUPPORT);
        return inMsg->SetCopiedObject<>(packet);
    }
    packet.SetVersion(version);
    // now V1 compatible for softWareVersion :{101, 102}
    int errCode = AckPacketSyncerPartDeSerializationV1(parcel, packet);
    if (errCode != E_OK) {
        return errCode;
    }

    return inMsg->SetCopiedObject<>(packet);
}

int SingleVerSerializeManager::AckPacketSyncerPartDeSerializationV1(Parcel &parcel, DataAckPacket &packet)
{
    WaterMark mark;
    int32_t errCode;
    std::vector<uint64_t> reserved;

    parcel.ReadUInt64(mark);
    parcel.ReadInt(errCode);
    parcel.ReadVector<uint64_t>(reserved);
    if (parcel.IsError()) {
        LOGE("[AckPacketSyncerPartDeSerializationV1] DeSerialization failed");
        return -E_INVALID_ARGS;
    }
    packet.SetData(mark);
    packet.SetRecvCode(errCode);
    packet.SetReserved(reserved);
    return E_OK;
}

int SingleVerSerializeManager::ControlPacketCalculateLen(const Message *inMsg, uint32_t &len)
{
    auto packet = inMsg->GetObject<ControlRequestPacket>();
    if (packet == nullptr || packet->GetcontrolCmdType() >= INVALID_CONTROL_CMD) {
        LOGE("[ControlPacketSerialization] invalid control cmd");
        return -E_INVALID_ARGS;
    }
    if (packet->GetcontrolCmdType() == SUBSCRIBE_QUERY_CMD || packet->GetcontrolCmdType() == UNSUBSCRIBE_QUERY_CMD) {
        return SingleVerSerializeManager::SubscribeCalculateLen(inMsg, len);
    }
    return E_OK;
}

int SingleVerSerializeManager::ControlPacketSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
{
    auto packet = inMsg->GetObject<ControlRequestPacket>();
    if (packet == nullptr || packet->GetcontrolCmdType() >= INVALID_CONTROL_CMD) {
        LOGE("[ControlPacketSerialization] invalid control cmd");
        return -E_INVALID_ARGS;
    }
    if (packet->GetcontrolCmdType() == SUBSCRIBE_QUERY_CMD || packet->GetcontrolCmdType() == UNSUBSCRIBE_QUERY_CMD) {
        return SingleVerSerializeManager::SubscribeSerialization(buffer, length, inMsg);
    }
    return E_OK;
}

int SingleVerSerializeManager::ControlPacketDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg)
{
    Parcel parcel(const_cast<uint8_t *>(buffer), length);
    ControlRequestPacket packet;
    int errCode = ControlRequestDeSerialization(parcel, packet);
    if (errCode != E_OK) {
        return errCode;
    }
    if (packet.GetcontrolCmdType() == SUBSCRIBE_QUERY_CMD || packet.GetcontrolCmdType() == UNSUBSCRIBE_QUERY_CMD) {
        errCode = SubscribeDeSerialization(parcel, inMsg, packet);
    }
    return errCode;
}

int SingleVerSerializeManager::AckControlPacketCalculateLen(const Message *inMsg, uint32_t &len)
{
    auto packet = inMsg->GetObject<ControlAckPacket>();
    if (packet == nullptr) {
        LOGE("[AckControlPacketCalculateLen] invalid control cmd");
        return -E_INVALID_ARGS;
    }
    len = packet->CalculateLen();
    return E_OK;
}

int SingleVerSerializeManager::AckControlPacketSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
{
    auto packet = inMsg->GetObject<ControlAckPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    Parcel parcel(buffer, length);
    parcel.WriteUInt32(packet->GetVersion());
    parcel.WriteInt(packet->GetRecvCode());
    parcel.WriteUInt32(packet->GetcontrolCmdType());
    parcel.WriteUInt32(packet->GetFlag());
    if (parcel.IsError()) {
        LOGE("[AckControlPacketSerialization] Serialization failed");
        return -E_INVALID_ARGS;
    }
    parcel.EightByteAlign();
    return E_OK;
}

int SingleVerSerializeManager::AckControlPacketDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg)
{
    auto packet = new (std::nothrow) ControlAckPacket();
    if (packet == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    Parcel parcel(const_cast<uint8_t *>(buffer), length);
    int32_t recvCode = 0;
    uint32_t version = 0;
    uint32_t controlCmdType = 0;
    uint32_t flag = 0;
    parcel.ReadUInt32(version);
    parcel.ReadInt(recvCode);
    parcel.ReadUInt32(controlCmdType);
    parcel.ReadUInt32(flag);
    int errCode;
    if (parcel.IsError()) {
        LOGE("[AckControlPacketDeSerialization] DeSerialization failed");
        errCode = -E_INVALID_ARGS;
        goto ERROR;
    }
    packet->SetPacketHead(recvCode, version, static_cast<int32_t>(controlCmdType), flag);
    errCode = inMsg->SetExternalObject<>(packet);
    if (errCode != E_OK) {
        goto ERROR;
    }
    return errCode;
ERROR:
    delete packet;
    packet = nullptr;
    return errCode;
}

int SingleVerSerializeManager::ControlRequestSerialization(Parcel &parcel, const Message *inMsg)
{
    auto packet = inMsg->GetObject<ControlRequestPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    parcel.WriteUInt32(packet->GetVersion());
    parcel.WriteInt(packet->GetSendCode());
    parcel.WriteUInt32(packet->GetcontrolCmdType());
    parcel.WriteUInt32(packet->GetFlag());
    if (parcel.IsError()) {
        LOGE("[ControlRequestSerialization] Serialization failed");
        return -E_INVALID_ARGS;
    }
    parcel.EightByteAlign();
    return E_OK;
}

int SingleVerSerializeManager::ControlRequestDeSerialization(Parcel &parcel, ControlRequestPacket &packet)
{
    uint32_t version = 0;
    int32_t sendCode = 0;
    uint32_t controlCmdType = 0;
    uint32_t flag = 0;
    parcel.ReadUInt32(version);
    if (version > SOFTWARE_VERSION_CURRENT) {
        return -E_VERSION_NOT_SUPPORT;
    }
    parcel.ReadInt(sendCode);
    parcel.ReadUInt32(controlCmdType);
    parcel.ReadUInt32(flag);
    if (parcel.IsError()) {
        LOGE("[ControlRequestDeSerialization] deserialize failed!");
        return -E_LENGTH_ERROR;
    }
    packet.SetPacketHead(sendCode, version, static_cast<int32_t>(controlCmdType), flag);
    return E_OK;
}

int SingleVerSerializeManager::SubscribeCalculateLen(const Message *inMsg, uint32_t &len)
{
    auto packet = inMsg->GetObject<SubscribeRequest>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    len = packet->CalculateLen();
    return E_OK;
}

int SingleVerSerializeManager::SubscribeSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
{
    auto packet = inMsg->GetObject<SubscribeRequest>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    Parcel parcel(buffer, length);
    int errCode = ControlRequestSerialization(parcel, inMsg);
    if (errCode != E_OK) {
        LOGE("[SubscribeSerialization] ControlRequestPacket Serialization failed, errCode=%d", errCode);
        return errCode;
    }
    QuerySyncObject queryObj = packet->GetQuery();
    errCode = queryObj.SerializeData(parcel, SOFTWARE_VERSION_CURRENT);
    if (errCode != E_OK) {
        LOGE("[SubscribeSerialization] query object Serialization failed, errCode=%d", errCode);
        return errCode;
    }
    return E_OK;
}

int SingleVerSerializeManager::SubscribeDeSerialization(Parcel &parcel, Message *inMsg,
    ControlRequestPacket &controlPacket)
{
    auto packet = new (std::nothrow) SubscribeRequest();
    if (packet == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    QuerySyncObject querySyncObj;
    int errCode = QuerySyncObject::DeSerializeData(parcel, querySyncObj);
    if (errCode != E_OK) {
        goto ERROR;
    }
    packet->SetPacketHead(controlPacket.GetSendCode(), controlPacket.GetVersion(),
        static_cast<int32_t>(controlPacket.GetcontrolCmdType()), controlPacket.GetFlag());
    packet->SetQuery(querySyncObj);
    errCode = inMsg->SetExternalObject<>(packet);
    if (errCode != E_OK) {
        goto ERROR;
    }
    return errCode;
ERROR:
    delete packet;
    packet = nullptr;
    return errCode;
}

int SingleVerSerializeManager::RegisterCommunicatorTransformFunc()
{
    TransformFunc func;
    func.computeFunc = std::bind(&SingleVerSerializeManager::CalculateLen, std::placeholders::_1);
    func.serializeFunc = std::bind(&SingleVerSerializeManager::Serialization, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3);
    func.deserializeFunc = std::bind(&SingleVerSerializeManager::DeSerialization, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3);

    static std::vector<MessageId> messageIds = {
        QUERY_SYNC_MESSAGE, DATA_SYNC_MESSAGE, CONTROL_SYNC_MESSAGE, REMOTE_EXECUTE_MESSAGE
    };
    int errCode = E_OK;
    for (auto &id : messageIds) {
        int retCode = MessageTransform::RegTransformFunction(static_cast<uint32_t>(id), func);
        if (retCode != E_OK) {
            LOGE("[SingleVerSerializeManager][RegisterTransformFunc] regist messageId %u failed %d",
                static_cast<uint32_t>(id), retCode);
            errCode = retCode;
        }
    }
    return errCode;
}

void SingleVerSerializeManager::RegisterInnerTransformFunc()
{
    TransformFunc func;
    func.computeFunc = std::bind(&SingleVerSerializeManager::ISyncPacketCalculateLen, std::placeholders::_1);
    func.serializeFunc = std::bind(&SingleVerSerializeManager::ISyncPacketSerialization,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    func.deserializeFunc = std::bind(&SingleVerSerializeManager::ISyncPacketDeSerialization,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    std::lock_guard<std::mutex> autoLock(handlesLock_);
    messageHandles_.emplace(static_cast<uint32_t>(REMOTE_EXECUTE_MESSAGE), func);
}

uint32_t SingleVerSerializeManager::ISyncPacketCalculateLen(const Message *inMsg)
{
    if (inMsg == nullptr) {
        return 0u;
    }
    uint32_t len = 0u;
    const auto packet = inMsg->GetObject<ISyncPacket>();
    if (packet != nullptr) {
        len = packet->CalculateLen();
    }
    return len;
}

int SingleVerSerializeManager::ISyncPacketSerialization(uint8_t *buffer, uint32_t length,
    const Message *inMsg)
{
    if (inMsg == nullptr) {
        return -E_INVALID_ARGS;
    }
    int errCode = E_OK;
    Parcel parcel(buffer, length);
    auto packet = inMsg->GetObject<ISyncPacket>();
    if (packet != nullptr) {
        errCode = packet->Serialization(parcel);
    }
    return errCode;
}

int SingleVerSerializeManager::ISyncPacketDeSerialization(const uint8_t *buffer, uint32_t length,
    Message *inMsg)
{
    if (inMsg == nullptr) {
        return -E_INVALID_ARGS;
    }
    ISyncPacket *packet = nullptr;
    int errCode = BuildISyncPacket(inMsg, packet);
    if (errCode != E_OK) {
        return errCode;
    }
    Parcel parcel(const_cast<uint8_t *>(buffer), length);
    do {
        errCode = packet->DeSerialization(parcel);
        if (errCode != E_OK) {
            break;
        }
        errCode = inMsg->SetExternalObject(packet);
    } while (false);
    if (errCode != E_OK) {
        delete packet;
        packet = nullptr;
    }
    return E_OK;
}

int SingleVerSerializeManager::BuildISyncPacket(Message *inMsg, ISyncPacket *&packet)
{
    uint32_t messageId = inMsg->GetMessageId();
    if (messageId != static_cast<uint32_t>(REMOTE_EXECUTE_MESSAGE)) {
        return -E_INVALID_ARGS;
    }
    switch (inMsg->GetMessageType()) {
        case TYPE_REQUEST:
            packet = new(std::nothrow) RemoteExecutorRequestPacket();
            break;
        case TYPE_RESPONSE:
            packet = new(std::nothrow) RemoteExecutorAckPacket();
            break;
        default:
            packet = nullptr;
            break;
    }
    if (packet == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    return E_OK;
}

int SingleVerSerializeManager::DataPacketExtraConditionsSerialization(Parcel &parcel, const DataRequestPacket *packet)
{
    std::map<std::string, std::string> extraConditions = packet->GetExtraConditions();
    if (extraConditions.size() > DBConstant::MAX_CONDITION_COUNT) {
        return -E_INVALID_ARGS;
    }
    parcel.WriteUInt32(static_cast<uint32_t>(extraConditions.size()));
    for (const auto &entry : extraConditions) {
        if (entry.first.length() > DBConstant::MAX_CONDITION_KEY_LEN ||
            entry.second.length() > DBConstant::MAX_CONDITION_VALUE_LEN) {
            return -E_INVALID_ARGS;
        }
        parcel.WriteString(entry.first);
        parcel.WriteString(entry.second);
    }
    parcel.EightByteAlign();
    if (parcel.IsError()) {
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

int SingleVerSerializeManager::DataPacketExtraConditionsDeserialization(Parcel &parcel, DataRequestPacket *packet)
{
    if (!packet->IsExtraConditionData()) {
        return E_OK;
    }
    uint32_t conditionSize = 0u;
    (void) parcel.ReadUInt32(conditionSize);
    if (conditionSize > DBConstant::MAX_CONDITION_COUNT) {
        return -E_INVALID_ARGS;
    }
    std::map<std::string, std::string> extraConditions;
    for (uint32_t i = 0; i < conditionSize; i++) {
        std::string conditionKey;
        std::string conditionVal;
        (void) parcel.ReadString(conditionKey);
        (void) parcel.ReadString(conditionVal);
        if (conditionKey.length() > DBConstant::MAX_CONDITION_KEY_LEN ||
            conditionVal.length() > DBConstant::MAX_CONDITION_VALUE_LEN) {
            return -E_INVALID_ARGS;
        }
        extraConditions[conditionKey] = conditionVal;
    }
    parcel.EightByteAlign();
    if (parcel.IsError()) {
        return -E_PARSE_FAIL;
    }
    packet->SetExtraConditions(extraConditions);
    return E_OK;
}
}  // namespace DistributedDB