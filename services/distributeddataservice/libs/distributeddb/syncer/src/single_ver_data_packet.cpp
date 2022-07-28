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

#include "single_ver_data_packet.h"
#include "icommunicator.h"
#include "single_ver_kvdb_sync_interface.h"
#include "query_sync_object.h"
#include "generic_single_ver_kv_entry.h"
#include "sync_types.h"
#include "version.h"
#include "parcel.h"


namespace DistributedDB {
DataRequestPacket::~DataRequestPacket()
{
    for (auto &entry : data_) {
        delete entry;
        entry = nullptr;
    }
}

void DataRequestPacket::SetData(std::vector<SendDataItem> &data)
{
    data_ = std::move(data);
}

const std::vector<SendDataItem> &DataRequestPacket::GetData() const
{
    return data_;
}

void DataRequestPacket::SetCompressData(std::vector<uint8_t> &compressData)
{
    compressData_ = std::move(compressData);
}

const std::vector<uint8_t> &DataRequestPacket::GetCompressData() const
{
    return compressData_;
}

void DataRequestPacket::SetEndWaterMark(WaterMark waterMark)
{
    endWaterMark_ = waterMark;
}

WaterMark DataRequestPacket::GetEndWaterMark() const
{
    return endWaterMark_;
}

void DataRequestPacket::SetLocalWaterMark(WaterMark waterMark)
{
    localWaterMark_ = waterMark;
}

WaterMark DataRequestPacket::GetLocalWaterMark() const
{
    return localWaterMark_;
}

void DataRequestPacket::SetPeerWaterMark(WaterMark waterMark)
{
    peerWaterMark_ = waterMark;
}

WaterMark DataRequestPacket::GetPeerWaterMark() const
{
    return peerWaterMark_;
}

void DataRequestPacket::SetSendCode(int32_t errCode)
{
    sendCode_ = errCode;
}

int32_t DataRequestPacket::GetSendCode() const
{
    return sendCode_;
}

void DataRequestPacket::SetMode(int32_t mode)
{
    mode_ = mode;
}

int32_t DataRequestPacket::GetMode() const
{
    return mode_;
}

void DataRequestPacket::SetSessionId(uint32_t sessionId)
{
    sessionId_ = sessionId;
}

uint32_t DataRequestPacket::GetSessionId() const
{
    return sessionId_;
}

void DataRequestPacket::SetVersion(uint32_t version)
{
    version_ = version;
}

uint32_t DataRequestPacket::GetVersion() const
{
    return version_;
}

void DataRequestPacket::SetReserved(std::vector<uint64_t> &reserved)
{
    reserved_ = std::move(reserved);
}

void DataRequestPacket::SetReserved(std::vector<uint64_t> &&reserved)
{
    reserved_ = reserved;
}

std::vector<uint64_t> DataRequestPacket::GetReserved() const
{
    return reserved_;
}

uint64_t DataRequestPacket::GetPacketId() const
{
    uint64_t packetId = 0;
    std::vector<uint64_t> DataRequestReserve = GetReserved();
    if (DataRequestReserve.size() > REQUEST_PACKET_RESERVED_INDEX_PACKETID) {
        return DataRequestReserve[REQUEST_PACKET_RESERVED_INDEX_PACKETID];
    } else {
        return packetId;
    }
}

uint32_t DataRequestPacket::CalculateLen(uint32_t messageId) const
{
    uint64_t totalLen = GenericSingleVerKvEntry::CalculateLens(
        IsCompressData() ? std::vector<SendDataItem> {} : data_, version_); // for data
    totalLen += Parcel::GetUInt64Len(); // endWaterMark
    totalLen += Parcel::GetUInt64Len(); // localWaterMark
    totalLen += Parcel::GetUInt64Len(); // peerWaterMark
    totalLen += Parcel::GetIntLen(); // sendCode
    totalLen += Parcel::GetIntLen(); // mode
    totalLen += Parcel::GetUInt32Len(); // sessionId
    totalLen += Parcel::GetUInt32Len(); // version
    totalLen += Parcel::GetVectorLen<uint64_t>(reserved_); // reserved

    if (version_ > SOFTWARE_VERSION_RELEASE_2_0) {
        totalLen += Parcel::GetUInt32Len(); // flag bit0 used for isLastSequence
    }
    totalLen = Parcel::GetEightByteAlign(totalLen); // 8-byte align
    if (totalLen > INT32_MAX) {
        return 0;
    }
    if (messageId == QUERY_SYNC_MESSAGE) {
        // deleted watermark
        totalLen += Parcel::GetUInt64Len();
        // query id
        totalLen += Parcel::GetStringLen(queryId_);
        // add for queryObject
        totalLen += query_.CalculateParcelLen(SOFTWARE_VERSION_CURRENT);
    }
    if (IsCompressData()) {
        totalLen += GenericSingleVerKvEntry::CalculateCompressedLens(compressData_); // add forcompressData_
    }
    totalLen += Parcel::GetUInt32Len(); // extraCondition size
    for (const auto &entry : extraConditions_) {
        totalLen += Parcel::GetStringLen(entry.first);
        totalLen += Parcel::GetStringLen(entry.second);
    }
    totalLen = Parcel::GetEightByteAlign(totalLen); // 8-byte align
    if (totalLen > INT32_MAX) {
        return 0;
    }
    return totalLen;
}

void DataRequestPacket::SetFlag(uint32_t flag)
{
    flag_ = flag;
}

uint32_t DataRequestPacket::GetFlag() const
{
    return flag_;
}

bool DataRequestPacket::IsLastSequence() const
{
    return ((flag_ & IS_LAST_SEQUENCE) == IS_LAST_SEQUENCE);
}

void DataRequestPacket::SetLastSequence()
{
    flag_ = flag_ | IS_LAST_SEQUENCE;
}

bool DataRequestPacket::IsNeedUpdateWaterMark() const
{
    return !((flag_ & IS_UPDATE_WATER) == IS_UPDATE_WATER);
}

void DataRequestPacket::SetUpdateWaterMark()
{
    flag_ = flag_ | IS_UPDATE_WATER;
}

void DataRequestPacket::SetCompressDataMark()
{
    flag_ = flag_ | IS_COMPRESS_DATA;
}

bool DataRequestPacket::IsCompressData() const
{
    return ((flag_ & IS_COMPRESS_DATA) == IS_COMPRESS_DATA);
}

void DataRequestPacket::SetCompressAlgo(CompressAlgorithm algo)
{
    algo_ = algo;
}

CompressAlgorithm DataRequestPacket::GetCompressAlgo() const
{
    return algo_;
}

void DataRequestPacket::SetBasicInfo(int sendCode, uint32_t version, int32_t mode)
{
    SetSendCode(sendCode);
    SetVersion(version);
    SetMode(mode);
}

void DataRequestPacket::SetWaterMark(WaterMark localMark, WaterMark peerMark, WaterMark deletedWatermark)
{
    localWaterMark_ = localMark;
    peerWaterMark_ = peerMark;
    deletedWatermark_ = deletedWatermark;
}

void DataRequestPacket::SetQuery(const QuerySyncObject &query)
{
    query_ = query;
}

QuerySyncObject DataRequestPacket::GetQuery() const
{
    return query_;
}

void DataRequestPacket::SetQueryId(const std::string &queryId)
{
    queryId_ = queryId;
}

std::string DataRequestPacket::GetQueryId() const
{
    return queryId_;
}

void DataRequestPacket::SetDeletedWaterMark(WaterMark watermark)
{
    deletedWatermark_ = watermark;
}

WaterMark DataRequestPacket::GetDeletedWaterMark() const
{
    return deletedWatermark_;
}

void DataRequestPacket::SetExtraConditions(const std::map<std::string, std::string> &extraConditions)
{
    extraConditions_ = extraConditions;
}
std::map<std::string, std::string> DataRequestPacket::GetExtraConditions() const
{
    return extraConditions_;
}

void DataAckPacket::SetData(uint64_t data)
{
    data_ = data;
}

uint64_t DataAckPacket::GetData() const
{
    return data_;
}

void DataAckPacket::SetRecvCode(int32_t errorCode)
{
    recvCode_ = errorCode;
}

int32_t DataAckPacket::GetRecvCode() const
{
    return recvCode_;
}

void DataAckPacket::SetVersion(uint32_t version)
{
    version_ = version;
}

uint32_t DataAckPacket::GetVersion() const
{
    return version_;
}

void DataAckPacket::SetReserved(std::vector<uint64_t> &reserved)
{
    reserved_ = std::move(reserved);
}

std::vector<uint64_t> DataAckPacket::GetReserved() const
{
    return reserved_;
}

uint64_t DataAckPacket::GetPacketId() const
{
    uint64_t packetId = 0;
    std::vector<uint64_t> DataAckReserve = GetReserved();
    if (DataAckReserve.size() > ACK_PACKET_RESERVED_INDEX_PACKETID) {
        packetId = DataAckReserve[ACK_PACKET_RESERVED_INDEX_PACKETID];
    }
    // while remote db is close and open again, it may not carry packetId
    // so the second index is deletewatermark if it is the query Sync, should drop the deletewatermark here
    if (packetId > MAX_PACKETID) {
        return 0;
    }
    return packetId;
}

bool DataAckPacket::IsPacketIdValid(uint64_t packetId)
{
    return (packetId > 0);
}

uint32_t DataAckPacket::CalculateLen() const
{
    uint64_t len = Parcel::GetUInt64Len(); // ackWaterMark
    len += Parcel::GetIntLen(); // recvCode
    len += Parcel::GetUInt32Len(); // version
    len += Parcel::GetVectorLen<uint64_t>(reserved_); // reserved

    len = Parcel::GetEightByteAlign(len);
    if (len > INT32_MAX) {
        return 0;
    }
    return len;
}

void ControlRequestPacket::SetPacketHead(int sendCode, uint32_t version, int32_t controlCmd, uint32_t flag)
{
    sendCode_ = sendCode;
    version_ = version;
    controlCmdType_ = static_cast<uint32_t>(controlCmd);
    flag_ = flag;
}

int32_t ControlRequestPacket::GetSendCode() const
{
    return sendCode_;
}

uint32_t ControlRequestPacket::GetVersion() const
{
    return version_;
}

uint32_t ControlRequestPacket::GetcontrolCmdType() const
{
    return controlCmdType_;
}

uint32_t ControlRequestPacket::GetFlag() const
{
    return flag_;
}

void ControlRequestPacket::SetQuery(const QuerySyncObject &query)
{
    (void)query;
}

uint32_t ControlRequestPacket::CalculateLen() const
{
    uint64_t len = Parcel::GetUInt32Len(); // version_
    len += Parcel::GetIntLen(); // sendCode_
    len += Parcel::GetUInt32Len(); // controlCmdType_
    len += Parcel::GetUInt32Len(); // flag

    len = Parcel::GetEightByteAlign(len);
    if (len > INT32_MAX) {
        return 0;
    }
    return len;
}

void SubscribeRequest::SetQuery(const QuerySyncObject &query)
{
    query_ = query;
}

QuerySyncObject SubscribeRequest::GetQuery() const
{
    return query_;
}

uint32_t SubscribeRequest::CalculateLen() const
{
    uint64_t totalLen = ControlRequestPacket::CalculateLen();
    if (totalLen == 0) {
        LOGE("[SubscribeRequest] cal packet len failed");
        return 0;
    }
    // add for queryObject
    totalLen += query_.CalculateParcelLen(SOFTWARE_VERSION_CURRENT);
    if (totalLen > INT32_MAX) {
        return 0;
    }
    return totalLen;
}

bool SubscribeRequest::IsAutoSubscribe() const
{
    return ((GetFlag() & IS_AUTO_SUBSCRIBE) == IS_AUTO_SUBSCRIBE);
}

void ControlAckPacket::SetPacketHead(int recvCode, uint32_t version, int32_t controlCmd, uint32_t flag)
{
    recvCode_ = recvCode;
    version_ = version;
    controlCmdType_ = static_cast<uint32_t>(controlCmd);
    flag_ = flag;
}

int32_t ControlAckPacket::GetRecvCode() const
{
    return recvCode_;
}

uint32_t ControlAckPacket::GetVersion() const
{
    return version_;
}

uint32_t ControlAckPacket::GetcontrolCmdType() const
{
    return controlCmdType_;
}

uint32_t ControlAckPacket::GetFlag() const
{
    return flag_;
}

uint32_t ControlAckPacket::CalculateLen() const
{
    uint64_t len = Parcel::GetUInt32Len(); // version_
    len += Parcel::GetIntLen(); // recvCode_
    len += Parcel::GetUInt32Len(); // controlCmdType_
    len += Parcel::GetUInt32Len(); // flag
    len = Parcel::GetEightByteAlign(len);
    if (len > INT32_MAX) {
        return 0;
    }
    return len;
}
} // namespace DistributedDB
