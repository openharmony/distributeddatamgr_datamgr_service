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

#ifndef REMOTE_EXECUTOR_PACKET_H
#define REMOTE_EXECUTOR_PACKET_H

#include "isync_packet.h"
#include "message.h"
#include "prepared_stmt.h"
#include "relational_row_data_set.h"
#include "sync_types.h"
#include "relational_row_data_set.h"

namespace DistributedDB {
class RemoteExecutorRequestPacket : public ISyncPacket {
public:
    RemoteExecutorRequestPacket();
    ~RemoteExecutorRequestPacket() override;

    uint32_t GetVersion() const;

    void SetVersion(uint32_t version);

    uint32_t GetFlag() const;

    void SetFlag(uint32_t flag);

    PreparedStmt GetPreparedStmt() const;

    void SetPreparedStmt(const PreparedStmt &perparedStmt);

    bool IsNeedResponse() const;

    void SetNeedResponse();

    uint32_t CalculateLen() const override;

    int Serialization(Parcel &parcel) const override;

    int DeSerialization(Parcel &parcel) override;
private:
    uint32_t version_ = 0u;
    uint32_t flag_ = 0u; // 0x01 mean need reply ack
    PreparedStmt perparedStmt_;
};

class RemoteExecutorAckPacket : public ISyncPacket {
public:
    RemoteExecutorAckPacket();

    ~RemoteExecutorAckPacket() override;

    uint32_t GetVersion() const;

    void SetVersion(uint32_t version);

    uint32_t GetFlag() const;

    void SetFlag(uint32_t flag);

    int32_t GetAckCode() const;

    void SetAckCode(int32_t ackCode);

    void MoveInRowDataSet(RelationalRowDataSet &&rowDataSet);

    RelationalRowDataSet &&MoveOutRowDataSet() const;

    bool IsLastAck() const;

    void SetLastAck();

    uint32_t CalculateLen() const override;

    int Serialization(Parcel &parcel) const override;

    int DeSerialization(Parcel &parcel) override;
private:
    uint32_t version_ = 0u;
    int32_t ackCode_ = 0;
    uint32_t flag_ = 0u; // 0x01 mean last one
    mutable RelationalRowDataSet rowDataSet_;
};
}
#endif