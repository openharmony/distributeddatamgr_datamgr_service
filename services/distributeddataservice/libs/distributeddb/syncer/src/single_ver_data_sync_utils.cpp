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
#include "single_ver_data_sync_utils.h"

#include <mutex>
#include "db_common.h"
#include "version.h"
#include "log_print.h"
#include "message.h"
namespace DistributedDB {
bool SingleVerDataSyncUtils::QuerySyncCheck(const SingleVerSyncTaskContext *context)
{
    if (!context->IsQuerySync()) {
        return true;
    }
    uint32_t version = std::min(context->GetRemoteSoftwareVersion(), SOFTWARE_VERSION_CURRENT);
    // for 101 version, no need to do abilitySync, just send request to remote
    if (version <= SOFTWARE_VERSION_RELEASE_1_0) {
        return true;
    }
    if (version < SOFTWARE_VERSION_RELEASE_4_0) {
        LOGE("[SingleVerDataSync] not support query sync when remote ver lower than 104");
        return false;
    }
    if (version < SOFTWARE_VERSION_RELEASE_5_0 && !(context->GetQuery().IsQueryOnlyByKey())) {
        LOGE("[SingleVerDataSync] remote version only support prefix key");
        return false;
    }
    if (context->GetQuery().HasInKeys() &&
        context->IsNotSupportAbility(SyncConfig::INKEYS_QUERY)) {
        return false;
    }
    return true;
}

int SingleVerDataSyncUtils::AckMsgErrnoCheck(const SingleVerSyncTaskContext *context, const Message *message)
{
    if (context == nullptr || message == nullptr) {
        return -E_INVALID_ARGS;
    }
    if (message->IsFeedbackError()) {
        LOGE("[DataSync][AckMsgErrnoCheck] message errNo=%d", message->GetErrorNo());
        return -static_cast<int>(message->GetErrorNo());
    }
    return E_OK;
}

int SingleVerDataSyncUtils::RequestQueryCheck(const DataRequestPacket *packet, SyncGenericInterface *storage)
{
    if (storage == nullptr || packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    if (SyncOperation::GetSyncType(packet->GetMode()) != SyncType::QUERY_SYNC_TYPE) {
        return E_OK;
    }
    QuerySyncObject syncQuery = packet->GetQuery();
    int errCode = storage->CheckAndInitQueryCondition(syncQuery);
    if (errCode != E_OK) {
        LOGE("[SingleVerDataSync] check sync query failed,errCode=%d", errCode);
        return errCode;
    }
    return E_OK;
}

bool SingleVerDataSyncUtils::IsPermitLocalDeviceRecvData(const std::string &deviceId,
    const SecurityOption &remoteSecOption)
{
    return RuntimeContext::GetInstance()->CheckDeviceSecurityAbility(deviceId, remoteSecOption);
}

bool SingleVerDataSyncUtils::IsPermitRemoteDeviceRecvData(const std::string &deviceId,
    const SecurityOption &remoteSecOption, SyncGenericInterface *storage)
{
    if (storage == nullptr) {
        return -E_INVALID_ARGS;
    }
    SecurityOption localSecOption;
    if (remoteSecOption.securityLabel == NOT_SURPPORT_SEC_CLASSIFICATION) {
        return true;
    }
    int errCode = storage->GetSecurityOption(localSecOption);
    if (errCode == -E_NOT_SUPPORT) {
        return true;
    }
    return RuntimeContext::GetInstance()->CheckDeviceSecurityAbility(deviceId, localSecOption);
}

void SingleVerDataSyncUtils::TransDbDataItemToSendDataItem(const std::string &localHashName,
    std::vector<SendDataItem> &outData)
{
    for (size_t i = 0; i < outData.size(); i++) {
        if (outData[i] == nullptr) {
            continue;
        }
        outData[i]->SetOrigDevice(outData[i]->GetOrigDevice().empty() ? localHashName : outData[i]->GetOrigDevice());
        if (i == 0 || i == (outData.size() - 1)) {
            LOGD("[DataSync][TransToSendItem] printData packet=%zu,timestamp=%" PRIu64 ",flag=%" PRIu64, i,
                outData[i]->GetTimestamp(), outData[i]->GetFlag());
        }
    }
}

std::string SingleVerDataSyncUtils::TransferForeignOrigDevName(const std::string &deviceName,
    const std::string &localHashName)
{
    if (localHashName == deviceName) {
        return "";
    }
    return deviceName;
}

void SingleVerDataSyncUtils::TransSendDataItemToLocal(const SingleVerSyncTaskContext *context,
    const std::string &localHashName, const std::vector<SendDataItem> &data)
{
    TimeOffset offset = context->GetTimeOffset();
    Timestamp currentLocalTime = context->GetCurrentLocalTime();
    for (auto &item : data) {
        if (item == nullptr) {
            continue;
        }
        item->SetOrigDevice(TransferForeignOrigDevName(item->GetOrigDevice(), localHashName));
        Timestamp tempTimestamp = item->GetTimestamp();
        Timestamp tempWriteTimestamp = item->GetWriteTimestamp();
        item->SetTimestamp(tempTimestamp - static_cast<Timestamp>(offset));
        if (tempWriteTimestamp != 0) {
            item->SetWriteTimestamp(tempWriteTimestamp - static_cast<Timestamp>(offset));
        }

        if (item->GetTimestamp() > currentLocalTime) {
            item->SetTimestamp(currentLocalTime);
        }
        if (item->GetWriteTimestamp() > currentLocalTime) {
            item->SetWriteTimestamp(currentLocalTime);
        }
    }
}

void SingleVerDataSyncUtils::TranslateErrCodeIfNeed(int mode, uint32_t version, int &errCode)
{
    // once get data occur E_EKEYREVOKED error, should also send request to remote dev to pull data.
    if (SyncOperation::TransferSyncMode(mode) == SyncModeType::PUSH_AND_PULL &&
        version > SOFTWARE_VERSION_RELEASE_2_0 && errCode == -E_EKEYREVOKED) {
        errCode = E_OK;
    }
}

int SingleVerDataSyncUtils::RunPermissionCheck(SingleVerSyncTaskContext *context, const SyncGenericInterface* storage,
    const std::string &label, int mode)
{
    std::string appId = storage->GetDbProperties().GetStringProp(KvDBProperties::APP_ID, "");
    std::string userId = storage->GetDbProperties().GetStringProp(KvDBProperties::USER_ID, "");
    std::string storeId = storage->GetDbProperties().GetStringProp(KvDBProperties::STORE_ID, "");
    uint8_t flag;
    switch (mode) {
        case SyncModeType::PUSH:
            flag = CHECK_FLAG_RECEIVE;
            break;
        case SyncModeType::PULL:
            flag = CHECK_FLAG_SEND;
            break;
        case SyncModeType::PUSH_AND_PULL:
            flag = CHECK_FLAG_SEND | CHECK_FLAG_RECEIVE;
            break;
        default:
            flag = CHECK_FLAG_RECEIVE;
            break;
    }
    int errCode = RuntimeContext::GetInstance()->RunPermissionCheck(userId, appId, storeId, context->GetDeviceId(),
        flag);
    if (errCode != E_OK) {
        LOGE("[DataSync][RunPermissionCheck] check failed flag=%" PRIu8 ",Label=%s,dev=%s", flag, label.c_str(),
            STR_MASK(context->GetDeviceId()));
    }
    return errCode;
}

bool SingleVerDataSyncUtils::CheckPermitReceiveData(const SingleVerSyncTaskContext *context,
    const ICommunicator *communicator)
{
    SecurityOption remoteSecOption = context->GetRemoteSeccurityOption();
    std::string localDeviceId;
    if (communicator == nullptr || remoteSecOption.securityLabel == NOT_SURPPORT_SEC_CLASSIFICATION) {
        return true;
    }
    communicator->GetLocalIdentity(localDeviceId);
    bool isPermitSync = SingleVerDataSyncUtils::IsPermitLocalDeviceRecvData(localDeviceId, remoteSecOption);
    if (isPermitSync) {
        return isPermitSync;
    }
    LOGE("[DataSync][PermitReceiveData] check failed: permitReceive=%d, localDev=%s, seclabel=%d, secflag=%d",
        isPermitSync, STR_MASK(localDeviceId), remoteSecOption.securityLabel, remoteSecOption.securityFlag);
    return isPermitSync;
}

void SingleVerDataSyncUtils::SetPacketId(DataRequestPacket *packet, SingleVerSyncTaskContext *context, uint32_t version)
{
    if (version > SOFTWARE_VERSION_RELEASE_2_0) {
        context->IncPacketId(); // begin from 1
        std::vector<uint64_t> reserved {context->GetPacketId()};
        packet->SetReserved(reserved);
    }
}

int SingleVerDataSyncUtils::GetMessageId(SyncType syncType)
{
    if (syncType == SyncType::QUERY_SYNC_TYPE) {
        return QUERY_SYNC_MESSAGE;
    }
    return DATA_SYNC_MESSAGE;
}

void SingleVerDataSyncUtils::PushAndPullKeyRevokHandle(SingleVerSyncTaskContext *context)
{
    // for push_and_pull mode it may be EKEYREVOKED error before receive watermarkexception
    // should clear errCode and restart pushpull request.
    int mode = SyncOperation::TransferSyncMode(context->GetMode());
    if (context->GetRemoteSoftwareVersion() > SOFTWARE_VERSION_RELEASE_2_0 && mode == SyncModeType::PUSH_AND_PULL &&
        context->GetTaskErrCode() == -E_EKEYREVOKED) {
        context->SetTaskErrCode(E_OK);
    }
}

int SingleVerDataSyncUtils::GetReSendMode(int mode, uint32_t sequenceId, SyncType syncType)
{
    int curMode = SyncOperation::TransferSyncMode(mode);
    if (curMode == SyncModeType::PUSH || curMode == SyncModeType::PULL) {
        return mode;
    }
    if (curMode == SyncModeType::RESPONSE_PULL) {
        return (syncType == SyncType::QUERY_SYNC_TYPE) ? SyncModeType::QUERY_PUSH : SyncModeType::PUSH;
    }
    // set push_and_pull mode when resend first sequenceId to inform remote to run RESPONSE_PULL task
    // for sequenceId which is larger than first, only need to send data, means to set push or query_push mode
    if (sequenceId == 1) {
        return (syncType == SyncType::QUERY_SYNC_TYPE) ? SyncModeType::QUERY_PUSH_PULL : SyncModeType::PUSH_AND_PULL;
    }
    return (syncType == SyncType::QUERY_SYNC_TYPE) ? SyncModeType::QUERY_PUSH : SyncModeType::PUSH;
}

void SingleVerDataSyncUtils::FillControlRequestPacket(ControlRequestPacket *packet, SingleVerSyncTaskContext *context)
{
    uint32_t version = std::min(context->GetRemoteSoftwareVersion(), SOFTWARE_VERSION_CURRENT);
    uint32_t flag = 0;
    if (context->GetMode() == SyncModeType::SUBSCRIBE_QUERY && context->IsAutoSubscribe()) {
        flag = flag | SubscribeRequest::IS_AUTO_SUBSCRIBE;
    }
    packet->SetPacketHead(E_OK, version, GetControlCmdType(context->GetMode()), flag);
    packet->SetQuery(context->GetQuery());
}

ControlCmdType SingleVerDataSyncUtils::GetControlCmdType(int mode)
{
    if (mode == SyncModeType::SUBSCRIBE_QUERY) {
        return ControlCmdType::SUBSCRIBE_QUERY_CMD;
    } else if  (mode == SyncModeType::UNSUBSCRIBE_QUERY) {
        return ControlCmdType::UNSUBSCRIBE_QUERY_CMD;
    }
    return ControlCmdType::INVALID_CONTROL_CMD;
}

int SingleVerDataSyncUtils::GetModeByControlCmdType(ControlCmdType controlCmd)
{
    if (controlCmd == ControlCmdType::SUBSCRIBE_QUERY_CMD) {
        return SyncModeType::SUBSCRIBE_QUERY;
    } else if  (controlCmd == ControlCmdType::UNSUBSCRIBE_QUERY_CMD) {
        return SyncModeType::UNSUBSCRIBE_QUERY;
    }
    return SyncModeType::INVALID_MODE;
}

bool SingleVerDataSyncUtils::IsNeedTriggerQueryAutoSync(Message *inMsg, QuerySyncObject &query)
{
    if (inMsg == nullptr) {
        return false;
    }
    if (inMsg->GetMessageId() != CONTROL_SYNC_MESSAGE) {
        return false;
    }
    const ControlRequestPacket *packet = inMsg->GetObject<ControlRequestPacket>();
    if (packet == nullptr) {
        return false;
    }
    uint32_t controlCmdType = packet->GetcontrolCmdType();
    if (controlCmdType == ControlCmdType::SUBSCRIBE_QUERY_CMD && inMsg->GetMessageType() == TYPE_REQUEST) {
        const SubscribeRequest *subPacket = inMsg->GetObject<SubscribeRequest>();
        if (packet == nullptr) {
            return false;
        }
        query = subPacket->GetQuery();
        LOGI("[SingleVerDataSync] receive sub scribe query cmd,begin to trigger query auto sync");
        return true;
    }
    return false;
}

void SingleVerDataSyncUtils::ControlAckErrorHandle(const SingleVerSyncTaskContext *context,
    const std::shared_ptr<SubscribeManager> &subManager)
{
    if (context->GetMode() == SyncModeType::SUBSCRIBE_QUERY) {
        // reserve before need clear
        subManager->DeleteLocalSubscribeQuery(context->GetDeviceId(), context->GetQuery());
    }
}

void SingleVerDataSyncUtils::SetMessageHeadInfo(Message &message, uint16_t inMsgType, const std::string &inTarget,
    uint32_t inSequenceId, uint32_t inSessionId)
{
    message.SetMessageType(inMsgType);
    message.SetTarget(inTarget);
    message.SetSequenceId(inSequenceId);
    message.SetSessionId(inSessionId);
}

bool SingleVerDataSyncUtils::IsGetDataSuccessfully(int errCode)
{
    return (errCode == E_OK || errCode == -E_UNFINISHED);
}

Timestamp SingleVerDataSyncUtils::GetMaxSendDataTime(const std::vector<SendDataItem> &inData)
{
    Timestamp stamp = 0;
    for (size_t i = 0; i < inData.size(); i++) {
        if (inData[i] == nullptr) {
            continue;
        }
        Timestamp tempStamp = inData[i]->GetTimestamp();
        if (stamp < tempStamp) {
            stamp = tempStamp;
        }
    }
    return stamp;
}

SyncTimeRange SingleVerDataSyncUtils::GetFullSyncDataTimeRange(const std::vector<SendDataItem> &inData,
    WaterMark localMark, UpdateWaterMark &isUpdate)
{
    Timestamp maxTimestamp = localMark;
    Timestamp minTimestamp = localMark;
    for (size_t i = 0; i < inData.size(); i++) {
        if (inData[i] == nullptr) {
            continue;
        }
        Timestamp tempStamp = inData[i]->GetTimestamp();
        if (maxTimestamp < tempStamp) {
            maxTimestamp = tempStamp;
        }
        if (minTimestamp > tempStamp) {
            minTimestamp = tempStamp;
        }
        isUpdate.normalUpdateMark = true;
    }
    return {minTimestamp, 0, maxTimestamp, 0};
}

SyncTimeRange SingleVerDataSyncUtils::GetQuerySyncDataTimeRange(const std::vector<SendDataItem> &inData,
    WaterMark localMark, WaterMark deleteLocalMark, UpdateWaterMark &isUpdate)
{
    SyncTimeRange dataTimeRange = {localMark, deleteLocalMark, localMark, deleteLocalMark};
    for (size_t i = 0; i < inData.size(); i++) {
        if (inData[i] == nullptr) {
            continue;
        }
        Timestamp tempStamp = inData[i]->GetTimestamp();
        if ((inData[i]->GetFlag() & DataItem::DELETE_FLAG) == 0) { // query data
            if (dataTimeRange.endTime < tempStamp) {
                dataTimeRange.endTime = tempStamp;
            }
            if (dataTimeRange.beginTime > tempStamp) {
                dataTimeRange.beginTime = tempStamp;
            }
            isUpdate.normalUpdateMark = true;
        }
        if ((inData[i]->GetFlag() & DataItem::DELETE_FLAG) != 0) { // delete data
            if (dataTimeRange.deleteEndTime < tempStamp) {
                dataTimeRange.deleteEndTime = tempStamp;
            }
            if (dataTimeRange.deleteBeginTime > tempStamp) {
                dataTimeRange.deleteBeginTime = tempStamp;
            }
            isUpdate.deleteUpdateMark = true;
        }
    }
    return dataTimeRange;
}

SyncTimeRange SingleVerDataSyncUtils::ReviseLocalMark(SyncType syncType, SyncTimeRange &dataTimeRange,
    UpdateWaterMark updateMark)
{
    SyncTimeRange tmpDataTime = dataTimeRange;
    if (updateMark.deleteUpdateMark && syncType == SyncType::QUERY_SYNC_TYPE) {
        tmpDataTime.deleteEndTime += 1;
    }
    if (updateMark.normalUpdateMark) {
        tmpDataTime.endTime += 1;
    }
    return tmpDataTime;
}

SyncTimeRange SingleVerDataSyncUtils::GetRecvDataTimeRange(SyncType syncType,
    const std::vector<SendDataItem> &data, UpdateWaterMark &isUpdate)
{
    if (syncType != SyncType::QUERY_SYNC_TYPE) {
        return SingleVerDataSyncUtils::GetFullSyncDataTimeRange(data, 0, isUpdate);
    }
    return SingleVerDataSyncUtils::GetQuerySyncDataTimeRange(data, 0, 0, isUpdate);
}

SyncTimeRange SingleVerDataSyncUtils::GetSyncDataTimeRange(SyncType syncType, WaterMark localMark, WaterMark deleteMark,
    const std::vector<SendDataItem> &inData, UpdateWaterMark &isUpdate)
{
    if (syncType != SyncType::QUERY_SYNC_TYPE) {
        return SingleVerDataSyncUtils::GetFullSyncDataTimeRange(inData, localMark, isUpdate);
    }
    return SingleVerDataSyncUtils::GetQuerySyncDataTimeRange(inData, localMark, deleteMark, isUpdate);
}
}