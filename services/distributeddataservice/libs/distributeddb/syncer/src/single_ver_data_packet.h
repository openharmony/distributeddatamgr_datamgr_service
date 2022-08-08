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

#ifndef SINGLE_VER_DATA_PACKET_NEW_H
#define SINGLE_VER_DATA_PACKET_NEW_H

#include "icommunicator.h"
#include "parcel.h"
#include "query_sync_object.h"
#include "single_ver_kvdb_sync_interface.h"
#include "sync_types.h"
#include "version.h"

namespace DistributedDB {
using SendDataItem = SingleVerKvEntry *;

class DataRequestPacket {
public:
    DataRequestPacket() {};
    virtual ~DataRequestPacket();

    void SetData(std::vector<SendDataItem> &data);

    const std::vector<SendDataItem> &GetData() const;
    const std::vector<uint8_t> &GetCompressedData() const;

    void SetCompressData(std::vector<uint8_t> &compressData);

    const std::vector<uint8_t> &GetCompressData() const;

    void SetEndWaterMark(WaterMark waterMark);

    WaterMark GetEndWaterMark() const;

    void SetLocalWaterMark(WaterMark waterMark);

    WaterMark GetLocalWaterMark() const;

    void SetPeerWaterMark(WaterMark waterMark);

    WaterMark GetPeerWaterMark() const;

    void SetSendCode(int32_t errCode);

    int32_t GetSendCode() const;

    void SetMode(int32_t mode);

    int32_t GetMode() const;

    void SetSessionId(uint32_t sessionId);

    uint32_t GetSessionId() const;

    void SetVersion(uint32_t version);

    uint32_t GetVersion() const;

    uint32_t CalculateLen(uint32_t messageId) const;

    void SetReserved(std::vector<uint64_t> &reserved);
    void SetReserved(std::vector<uint64_t> &&reserved);

    std::vector<uint64_t> GetReserved() const;

    uint64_t GetPacketId() const;

    void SetFlag(uint32_t flag);

    uint32_t GetFlag() const;

    bool IsLastSequence() const;

    void SetLastSequence();

    bool IsNeedUpdateWaterMark() const;

    void SetUpdateWaterMark();

    void SetBasicInfo(int sendCode, uint32_t version, int32_t mode);

    void SetWaterMark(WaterMark localMark, WaterMark peerMark, WaterMark deletedWatermark);

    void SetQuery(const QuerySyncObject &query);
    QuerySyncObject GetQuery() const;

    void SetQueryId(const std::string &queryId);
    std::string GetQueryId() const;

    void SetDeletedWaterMark(WaterMark watermark);
    WaterMark GetDeletedWaterMark() const;

    void SetCompressDataMark();
    bool IsCompressData() const;

    void SetCompressAlgo(CompressAlgorithm algo);
    CompressAlgorithm GetCompressAlgo() const;

    void SetExtraConditions(const std::map<std::string, std::string> &extraConditions);
    std::map<std::string, std::string> GetExtraConditions() const;
    bool IsExtraConditionData() const;

protected:
    std::vector<SendDataItem> data_;
    WaterMark endWaterMark_ = 0;
    WaterMark localWaterMark_ = 0;
    WaterMark peerWaterMark_ = 0;
    int32_t sendCode_ = 0;
    int32_t mode_ = SyncModeType::INVALID_MODE;
    uint32_t sessionId_ = 0;
    uint32_t version_ = SOFTWARE_VERSION_CURRENT;
    std::vector<uint64_t> reserved_;
    uint32_t flag_ = 0; // bit 0 used for isLastSequence
    // add for query sync mode
    QuerySyncObject query_;
    std::string queryId_;
    WaterMark deletedWatermark_ = 0;
    std::vector<uint8_t> compressData_; // if compressData size is above 0, means use compressData and ignore data_
    CompressAlgorithm algo_ = CompressAlgorithm::NONE; // used for param while serialize compress data
    std::map<std::string, std::string> extraConditions_; // use for checkpermission in annother device
    static const uint32_t IS_LAST_SEQUENCE = 0x1; // bit 0 used for isLastSequence, 1: is last, 0: not last
    static const uint32_t IS_UPDATE_WATER = 0x2; // bit 1 used for update watermark, 0: update, 1: not update
    static const uint32_t IS_COMPRESS_DATA = 0x4; // bit 3 used for compress data, 0: raw data, 1: compress data
    static const uint32_t IS_CONDITION_DATA = 0x8; // bit 4 used for extra condition data, 0: raw data
};

class DataAckPacket {
public:
    DataAckPacket() {};
    virtual ~DataAckPacket() {};

    void SetData(uint64_t data);

    uint64_t GetData() const;

    void SetRecvCode(int32_t errorCode);

    int32_t GetRecvCode() const;

    void SetVersion(uint32_t version);

    uint32_t GetVersion() const;

    void SetReserved(std::vector<uint64_t> &reserved);

    std::vector<uint64_t> GetReserved() const;

    uint64_t GetPacketId() const;

    static bool IsPacketIdValid(uint64_t packetId);

    uint32_t CalculateLen() const;

private:
    /*
     * data_ is waterMark when revCode_ == LOCAL_WATER_MARK_NOT_INIT || revCode_ == E_OK;
     * data_ is timer in milliSeconds when revCode_ == -E_SAVE_DATA_NOTIFY && data_ != 0.
     */
    uint64_t data_ = 0;
    int32_t recvCode_ = 0;
    uint32_t version_ = SOFTWARE_VERSION_CURRENT;
    std::vector<uint64_t> reserved_;
};

class ControlRequestPacket {
public:
    ControlRequestPacket() {};
    virtual ~ControlRequestPacket() {};
    void SetPacketHead(int sendCode, uint32_t version, int32_t controlCmd, uint32_t flag);

    int32_t GetSendCode() const;
    uint32_t GetVersion() const;
    uint32_t GetcontrolCmdType() const;
    uint32_t GetFlag() const;
    virtual void SetQuery(const QuerySyncObject &query);
    virtual uint32_t CalculateLen() const;
private:
    uint32_t version_ = SOFTWARE_VERSION_CURRENT;
    int32_t sendCode_ = 0;
    uint32_t controlCmdType_ = 0;
    uint32_t flag_ = 0;
};

class SubscribeRequest : public ControlRequestPacket {
public:
    SubscribeRequest() {};
    ~SubscribeRequest() override {};
    QuerySyncObject GetQuery() const;
    bool IsAutoSubscribe() const;
    void SetQuery(const QuerySyncObject &query) override;
    uint32_t CalculateLen() const override;
    static const uint32_t IS_AUTO_SUBSCRIBE = 0x1;
private:
    QuerySyncObject query_;
};

class ControlAckPacket {
public:
    ControlAckPacket() {};
    virtual ~ControlAckPacket() {};
    void SetPacketHead(int recvCode, uint32_t version, int32_t controlCmd, uint32_t flag);
    int32_t GetRecvCode() const;
    uint32_t GetVersion() const;
    uint32_t GetcontrolCmdType() const;
    uint32_t GetFlag() const;
    uint32_t CalculateLen() const;

private:
    uint32_t version_ = SOFTWARE_VERSION_CURRENT;
    int32_t recvCode_ = 0;
    uint32_t controlCmdType_ = 0;
    uint32_t flag_ = 0;
};
}  // namespace DistributedDB

#endif // SINGLE_VER_DATA_SYNC_NEW_H