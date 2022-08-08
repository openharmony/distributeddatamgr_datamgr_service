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

#ifndef SINGLE_VER_SERIALIZE_MANAGER_NEW_H
#define SINGLE_VER_SERIALIZE_MANAGER_NEW_H

#include "icommunicator.h"
#include "isync_packet.h"
#include "message_transform.h"
#include "parcel.h"
#include "single_ver_data_packet.h"

namespace DistributedDB {
class SingleVerSerializeManager {
public:
    SingleVerSerializeManager();
    virtual ~SingleVerSerializeManager();

    DISABLE_COPY_ASSIGN_MOVE(SingleVerSerializeManager);

    static int Serialization(uint8_t *buffer, uint32_t length, const Message *inMsg);

    static int DeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg);

    static uint32_t CalculateLen(const Message *inMsg);

    static int RegisterTransformFunc();
private:
    static bool IsPacketValid(const Message *inMsg);

    static int DataSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg);
    static int ControlSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg);

    static int DataDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg);
    static int ControlDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg);

    static uint32_t CalculateDataLen(const Message *inMsg);
    static uint32_t CalculateControlLen(const Message *inMsg);

    static int DataPacketSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg);
    static int DataPacketSyncerPartSerialization(Parcel &parcel, const DataRequestPacket *packet);
    static int DataPacketQuerySyncSerialization(Parcel &parcel, const DataRequestPacket *packet);
    static int DataPacketExtraConditionsSerialization(Parcel &parcel, const DataRequestPacket *packet);
    static int DataPacketCalculateLen(const Message *inMsg, uint32_t &len);

    static int DataPacketQuerySyncDeSerialization(Parcel &parcel, DataRequestPacket *packet);
    static int DataPacketCompressDataDeSerialization(Parcel &parcel, DataRequestPacket *packet);
    static int DataPacketDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg);
    static int DataPacketSyncerPartDeSerialization(Parcel &parcel, DataRequestPacket *packet, uint32_t packLen,
        uint32_t length, uint32_t version);
    static int DataPacketExtraConditionsDeserialization(Parcel &parcel, DataRequestPacket *packet);

    static int AckPacketCalculateLen(const Message *inMsg, uint32_t &len);
    static int AckPacketSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg);
    static int AckPacketSyncerPartSerializationV1(Parcel &parcel, const DataAckPacket *packet);

    static int AckPacketSyncerPartDeSerializationV1(Parcel &parcel, DataAckPacket &packet);
    static int AckPacketDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg);

    static int ControlPacketCalculateLen(const Message *inMsg, uint32_t &len);
    static int ControlPacketSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg);
    static int ControlPacketDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg);

    static int AckControlPacketCalculateLen(const Message *inMsg, uint32_t &len);
    static int AckControlPacketSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg);
    static int AckControlPacketDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg);

    static int ControlRequestSerialization(Parcel &parcel, const Message *inMsg);
    static int ControlRequestDeSerialization(Parcel &parcel, ControlRequestPacket &packet);
    static int SubscribeCalculateLen(const Message *inMsg, uint32_t &len);
    static int SubscribeSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg);
    static int SubscribeDeSerialization(Parcel &parcel, Message *inMsg, ControlRequestPacket &controlPacket);

    static int RegisterCommunicatorTransformFunc();
    static void RegisterInnerTransformFunc();

    static uint32_t ISyncPacketCalculateLen(const Message *inMsg);
    static int ISyncPacketSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg);
    static int ISyncPacketDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg);
    static int BuildISyncPacket(Message *inMsg, ISyncPacket *&packet);

    static std::mutex handlesLock_;
    static std::map<uint32_t, TransformFunc> messageHandles_;
};
}  // namespace DistributedDB

#endif // SINGLE_VER_SERIALIZE_MANAGER_NEW_H