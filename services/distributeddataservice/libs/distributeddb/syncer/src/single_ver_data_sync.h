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

#ifndef SINGLE_VER_DATA_SYNC_NEW_H
#define SINGLE_VER_DATA_SYNC_NEW_H

#include "icommunicator.h"
#include "isync_interface.h"
#include "meta_data.h"
#include "parcel.h"
#include "single_ver_data_message_schedule.h"
#include "single_ver_data_packet.h"
#include "single_ver_kvdb_sync_interface.h"
#include "single_ver_sync_task_context.h"
#include "sync_generic_interface.h"
#include "sync_types.h"
#include "version.h"

namespace DistributedDB {
using SendDataItem = SingleVerKvEntry *;
struct ReSendInfo {
    TimeStamp start = 0;
    TimeStamp end = 0;
    TimeStamp deleteBeginTime = 0;
    TimeStamp deleteEndTime = 0;
    // packetId is used for matched ackpacket packetId which saved in ackPacket.reserve
    // if equaled, means need to handle the ack, or drop. it is always increased
    uint64_t packetId = 0;
};

struct DataSyncReSendInfo {
    uint32_t sessionId = 0;
    uint32_t sequenceId = 0;
    TimeStamp start = 0; // means normal or sync data localwatermark
    TimeStamp end = 0;
    TimeStamp deleteDataStart = 0; // means delete data localwatermark
    TimeStamp deleteDataEnd = 0;
    uint64_t packetId = 0;
};

struct SyncEntry {
    std::vector<SendDataItem> entries;
    std::vector<uint8_t> compressedEntries;
};

class SingleVerDataSync {
public:
    SingleVerDataSync();
    virtual ~SingleVerDataSync();

    DISABLE_COPY_ASSIGN_MOVE(SingleVerDataSync);

    int Initialize(ISyncInterface *inStorage, ICommunicator *inCommunicateHandle,
        std::shared_ptr<Metadata> &inMetadata, const std::string &deviceId);

    int SyncStart(int mode, SingleVerSyncTaskContext *context);

    int TryContinueSync(SingleVerSyncTaskContext *context, const Message *message);

    void ClearSyncStatus();

    int PushStart(SingleVerSyncTaskContext *context);

    int PushPullStart(SingleVerSyncTaskContext *context);

    int PullRequestStart(SingleVerSyncTaskContext *context);

    int PullResponseStart(SingleVerSyncTaskContext *context);

    int DataRequestRecv(SingleVerSyncTaskContext *context, const Message *message, WaterMark &pullEndWatermark);

    bool AckPacketIdCheck(const Message *message);

    int AckRecv(SingleVerSyncTaskContext *context, const Message *message);

    void SendSaveDataNotifyPacket(SingleVerSyncTaskContext *context, uint32_t pktVersion, uint32_t sessionId,
        uint32_t sequenceId, uint32_t inMsgId);

    virtual int SendDataAck(SingleVerSyncTaskContext *context, const Message *message, int32_t recvCode,
        WaterMark maxSendDataTime);

    int CheckPermitSendData(int inMode, SingleVerSyncTaskContext *context);

    std::string GetLabel() const;

    std::string GetDeviceId() const;

    bool WaterMarkErrHandle(SyncType syncType, SingleVerSyncTaskContext *context, const Message *message);

    int ControlCmdStart(SingleVerSyncTaskContext *context);

    int ControlCmdRequestRecv(SingleVerSyncTaskContext *context, const Message *message);

    int ControlCmdAckRecv(SingleVerSyncTaskContext *context, const Message *message);

    void PutDataMsg(Message *message);

    Message *MoveNextDataMsg(SingleVerSyncTaskContext *context, bool &isNeedHandle, bool &isNeedContinue);

    bool IsNeedReloadQueue();

    void SendFinishedDataAck(SingleVerSyncTaskContext *context, const Message *message);

    void ScheduleInfoHandle(bool isNeedHandleStatus, bool isNeedClearMap, const Message *message);

    void ClearDataMsg();

protected:
    static const int SEND_FINISHED = 0xff;
    static const int LOCAL_WATER_MARK_NOT_INIT = 0xaa;
    static const int PEER_WATER_MARK_NOT_INIT = 0x55;
    static const int WATER_MARK_INVALID = 0xbb;
    static const int MTU_SIZE = 28311552; // 27MB

    void ResetSyncStatus(int inMode, SingleVerSyncTaskContext *context);

    int InnerSyncStart(SingleVerSyncTaskContext *context);

    void InnerClearSyncStatus();

    int ReSendData(SingleVerSyncTaskContext *context);

    int32_t ReSend(SingleVerSyncTaskContext *context, DataSyncReSendInfo reSendInfo);

    void SetSessionEndTimeStamp(TimeStamp end);

    TimeStamp GetSessionEndTimeStamp() const;

    void FillDataRequestPacket(DataRequestPacket *packet, SingleVerSyncTaskContext *context,
        SyncEntry &syncData, int sendCode, int mode);

    int RequestStart(SingleVerSyncTaskContext *context, int mode);

    SyncTimeRange GetSyncDataTimeRange(SyncType syncType, SingleVerSyncTaskContext *context,
        const std::vector<SendDataItem> &inData, UpdateWaterMark &isUpdate);

    int GetData(SingleVerSyncTaskContext *context, std::vector<SendDataItem> &outData, size_t packetSize);

    int GetDataWithPerformanceRecord(SingleVerSyncTaskContext *context, SyncEntry &syncOutData);

    int Send(SingleVerSyncTaskContext *context, const Message *message, const CommErrHandler &handler,
        uint32_t packetLen);

    int GetUnsyncData(SingleVerSyncTaskContext *context, std::vector<SendDataItem> &outData, size_t packetSize);

    int GetNextUnsyncData(SingleVerSyncTaskContext *context, std::vector<SendDataItem> &outData, size_t packetSize);

    int SaveData(const SingleVerSyncTaskContext *context, const std::vector<SendDataItem> &inData, SyncType curType,
        const QuerySyncObject &query);

    int SaveLocalWaterMark(SyncType syncType, const SingleVerSyncTaskContext *context,
        SyncTimeRange dataTimeRange, bool isCheckBeforUpdate = false) const;

    void GetLocalWaterMark(SyncType syncType, const std::string &queryIdentify, const SingleVerSyncTaskContext *context,
        WaterMark &watermark) const;

    void GetPeerWaterMark(SyncType syncType, const std::string &queryIdentify, const DeviceID &deviceId,
        WaterMark &watermark) const;

    void GetPeerDeleteSyncWaterMark(const DeviceID &deviceId, WaterMark &waterMark);

    void GetLocalDeleteSyncWaterMark(const SingleVerSyncTaskContext *context, WaterMark &waterMark) const;

    int RemoveDeviceDataHandle(SingleVerSyncTaskContext *context, const Message *message, WaterMark maxSendDataTime);

    int DealRemoveDeviceDataByAck(SingleVerSyncTaskContext *context, WaterMark ackWaterMark,
        const std::vector<uint64_t> &reserved);

    int SendDataPacket(SyncType syncType, const DataRequestPacket *packet, SingleVerSyncTaskContext *context);

    void UpdateQueryPeerWaterMark(SyncType syncType, const std::string &queryId, SyncTimeRange &dataTime,
        const SingleVerSyncTaskContext *context, UpdateWaterMark isUpdateWaterMark);

    void UpdatePeerWaterMark(SyncType syncType, const std::string &queryId, const SingleVerSyncTaskContext *context,
        WaterMark peerWatermark, WaterMark peerDeletedWatermark);

    std::string GetLocalDeviceName();

    int DoAbilitySyncIfNeed(SingleVerSyncTaskContext *context, const Message *message, bool isControlMsg = false);

    int DataRequestRecvPre(SingleVerSyncTaskContext *context, const Message *message);

    void GetPullEndWatermark(const SingleVerSyncTaskContext *context, const DataRequestPacket *packet,
        WaterMark &pullEndWatermark) const;

    int DealWaterMarkException(SingleVerSyncTaskContext *context, WaterMark ackWaterMark,
        const std::vector<uint64_t> &reserved);

    int RunPermissionCheck(SingleVerSyncTaskContext *context, const Message *message,
        const DataRequestPacket *packet);

    void SendResetWatchDogPacket(SingleVerSyncTaskContext *context, uint32_t packetLen);

    int SendReSendPacket(const DataRequestPacket *packet, SingleVerSyncTaskContext *context,
        uint32_t sessionId, uint32_t sequenceId);

    int SendPullResponseDataPkt(int ackCode, SyncEntry &syncOutData, SingleVerSyncTaskContext *context);

    int CheckSchemaStrategy(SingleVerSyncTaskContext *context, const Message *message);

    void RemotePushFinished(int sendCode, int inMode, uint32_t msgSessionId, uint32_t contextSessionId);

    void SetAckPacket(DataAckPacket &ackPacket, SingleVerSyncTaskContext *context, const DataRequestPacket *packet,
        int32_t recvCode, WaterMark maxSendDataTime);

    int GetReSendData(SyncEntry &syncData, SingleVerSyncTaskContext *context,
        DataSyncReSendInfo reSendInfo);

    virtual int RemoveDeviceDataIfNeed(SingleVerSyncTaskContext *context);

    virtual void UpdateSendInfo(SyncTimeRange dataTimeRange, SingleVerSyncTaskContext *context);

    void FillRequestReSendPacket(const SingleVerSyncTaskContext *context, DataRequestPacket *packet,
        DataSyncReSendInfo reSendInfo, SyncEntry &syncData, int sendCode);

    void UpdateMtuSize();

    DataSizeSpecInfo GetDataSizeSpecInfo(size_t packetSize);

    int InterceptData(SyncEntry &syncEntry);

    int ControlCmdStartCheck(SingleVerSyncTaskContext *context);

    int SendControlPacket(const ControlRequestPacket *packet, SingleVerSyncTaskContext *context);

    int ControlCmdRequestRecvPre(SingleVerSyncTaskContext *context, const Message *message);
    int SubscribeRequestRecvPre(SingleVerSyncTaskContext *context, const SubscribeRequest *packet,
        const Message *message);
    int SubscribeRequestRecv(SingleVerSyncTaskContext *context, const Message *message);
    int UnsubscribeRequestRecv(SingleVerSyncTaskContext *context, const Message *message);
    int SendControlAck(SingleVerSyncTaskContext *context, const Message *message, int32_t recvCode,
        uint32_t controlCmdType, const CommErrHandler &handler = nullptr);

    void SetSendConfig(const std::string &dstTarget, bool nonBlock, uint32_t timeout, SendConfig &sendConf);

    uint32_t mtuSize_;
    SyncGenericInterface* storage_;
    ICommunicator* communicateHandle_;
    std::shared_ptr<Metadata> metadata_;
    std::string label_;
    std::string deviceId_;

    SingleVerDataMessageSchedule msgSchedule_;

    static const int HIGH_VERSION_WINDOW_SIZE = 3;
    static const int LOW_VERSION_WINDOW_SIZE = 1;
    // below param is about sliding sync info, is different from every sync task
    std::mutex lock_;
    int mode_ = 0; // sync mode, may diff from context mode if trigger pull_response while push finish
    uint32_t sessionId_ = 0;
    // sequenceId as key
    std::map<uint32_t, ReSendInfo> reSendMap_;
    // remaining sending window
    int32_t windowSize_ = 0;
    // max sequenceId has been sent
    uint32_t maxSequenceIdHasSent_ = 0;
    bool isAllDataHasSent_ = false;
    // in a sync session, the last data timeStamp
    TimeStamp sessionEndTimeStamp_ = 0;
};
}  // namespace DistributedDB

#endif // SINGLE_VER_DATA_SYNC_NEW_H
