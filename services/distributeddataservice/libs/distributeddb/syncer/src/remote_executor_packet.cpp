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

#include "remote_executor_packet.h"

namespace DistributedDB {
namespace {
    constexpr uint8_t REQUEST_FLAG_RESPONSE_ACK = 1u;
    constexpr uint8_t ACK_FLAG_LAST_ACK = 1u;
}
RemoteExecutorRequestPacket::RemoteExecutorRequestPacket()
{
}

RemoteExecutorRequestPacket::~RemoteExecutorRequestPacket()
{
}

uint32_t RemoteExecutorRequestPacket::GetVersion() const
{
    return version_;
}

void RemoteExecutorRequestPacket::SetVersion(uint32_t version)
{
    version_ = version;
}

uint32_t RemoteExecutorRequestPacket::GetFlag() const
{
    return flag_;
}

void RemoteExecutorRequestPacket::SetFlag(uint32_t flag)
{
    flag_ = flag;
}

PreparedStmt RemoteExecutorRequestPacket::GetPreparedStmt() const
{
    return perparedStmt_;
}

void RemoteExecutorRequestPacket::SetPreparedStmt(const PreparedStmt &perparedStmt)
{
    perparedStmt_ = perparedStmt;
}

bool RemoteExecutorRequestPacket::IsNeedResponse() const
{
    return (flag_ & REQUEST_FLAG_RESPONSE_ACK) != 0;
}

void RemoteExecutorRequestPacket::SetNeedResponse()
{
    flag_ |= REQUEST_FLAG_RESPONSE_ACK;
}

uint32_t RemoteExecutorRequestPacket::CalculateLen() const
{
    uint32_t len = Parcel::GetUInt32Len(); // version
    len += Parcel::GetUInt32Len();  // flag
    len += perparedStmt_.CalcLength();
    return len;
}

int RemoteExecutorRequestPacket::Serialization(Parcel &parcel) const
{
    (void) parcel.WriteUInt32(version_);
    (void) parcel.WriteUInt32(flag_);
    (void) perparedStmt_.Serialize(parcel);
    if (parcel.IsError()) {
        LOGE("[RemoteExecutorRequestPacket] Serialization failed");
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

int RemoteExecutorRequestPacket::DeSerialization(Parcel &parcel)
{
    (void) parcel.ReadUInt32(version_);
    (void) parcel.ReadUInt32(flag_);
    (void) perparedStmt_.DeSerialize(parcel);
    if (parcel.IsError()) {
        LOGE("[RemoteExecutorRequestPacket] DeSerialization failed");
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

RemoteExecutorAckPacket::RemoteExecutorAckPacket()
{
}

RemoteExecutorAckPacket::~RemoteExecutorAckPacket()
{
}

uint32_t RemoteExecutorAckPacket::GetVersion() const
{
    return version_;
}

void RemoteExecutorAckPacket::SetVersion(uint32_t version)
{
    version_ = version;
}

uint32_t RemoteExecutorAckPacket::GetFlag() const
{
    return flag_;
}

void RemoteExecutorAckPacket::SetFlag(uint32_t flag)
{
    flag_ = flag;
}

int32_t RemoteExecutorAckPacket::GetAckCode() const
{
    return ackCode_;
}

void RemoteExecutorAckPacket::SetAckCode(int32_t ackCode)
{
    ackCode_ = ackCode;
}

void RemoteExecutorAckPacket::MoveInRowDataSet(RelationalRowDataSet &&rowDataSet)
{
    rowDataSet_ = std::move(rowDataSet);
}

RelationalRowDataSet &&RemoteExecutorAckPacket::MoveOutRowDataSet() const
{
    return std::move(rowDataSet_);
}

bool RemoteExecutorAckPacket::IsLastAck() const
{
    return (flag_ & ACK_FLAG_LAST_ACK) != 0;
}

void RemoteExecutorAckPacket::SetLastAck()
{
    flag_ |= ACK_FLAG_LAST_ACK;
}

uint32_t RemoteExecutorAckPacket::CalculateLen() const
{
    uint32_t len = Parcel::GetUInt32Len(); // version
    len += Parcel::GetIntLen();    // ackCode
    len += Parcel::GetUInt32Len();  // flag
    len = Parcel::GetEightByteAlign(len);
    len += rowDataSet_.CalcLength();
    return len;
}

int RemoteExecutorAckPacket::Serialization(Parcel &parcel) const
{
    (void) parcel.WriteUInt32(version_);
    (void) parcel.WriteInt(ackCode_);
    (void) parcel.WriteUInt32(flag_);
    parcel.EightByteAlign();
    (void) rowDataSet_.Serialize(parcel);
    if (parcel.IsError()) {
        LOGE("[RemoteExecutorAckPacket] Serialization failed");
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

int RemoteExecutorAckPacket::DeSerialization(Parcel &parcel)
{
    (void) parcel.ReadUInt32(version_);
    (void) parcel.ReadInt(ackCode_);
    (void) parcel.ReadUInt32(flag_);
    parcel.EightByteAlign();
    (void) rowDataSet_.DeSerialize(parcel);
    if (parcel.IsError()) {
        LOGE("[RemoteExecutorAckPacket] DeSerialization failed");
        return -E_INVALID_ARGS;
    }
    return E_OK;
}
}