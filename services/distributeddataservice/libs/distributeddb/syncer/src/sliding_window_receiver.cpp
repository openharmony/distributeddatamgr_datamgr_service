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

#include "sliding_window_receiver.h"
#include "sync_task_context.h"
#include "db_common.h"

namespace DistributedDB {
SlidingWindowReceiver::~SlidingWindowReceiver()
{
    Clear();
    dataSync_ = nullptr;
    context_ = nullptr;
}

void SlidingWindowReceiver::Clear()
{
    StopTimer();
    std::lock_guard<std::mutex> lock(lock_);
    ClearMap();
    isWaterMarkErrHappened_ = false;
}

int SlidingWindowReceiver::Initialize(SingleVerSyncTaskContext *context,
    std::shared_ptr<SingleVerDataSync> &dataSync)
{
    if (context == nullptr || dataSync == nullptr) {
        LOGE("[slwr] Initialize invalid args");
        return -E_INVALID_ARGS;
    }
    context_ = context;
    dataSync_ = dataSync;
    return E_OK;
}

int SlidingWindowReceiver::Receive(Message *inMsg)
{
    LOGD("[slwr] receive msg");
    if (inMsg == nullptr) {
        return -E_INVALID_ARGS;
    }
    int errCode = PutMsg(inMsg);
    if (errCode == E_OK) {
        StopTimer();
        StartTimer();
        DealMsg();
        return -E_NOT_NEED_DELETE_MSG;
    }
    return errCode;
}

int SlidingWindowReceiver::PutMsg(Message *inMsg)
{
    const DataRequestPacket *packet = inMsg->GetObject<DataRequestPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    uint32_t sessionId = inMsg->GetSessionId();
    uint32_t sequenceId = inMsg->GetSequenceId();
    bool isLastSequence = packet->IsLastSequence();
    std::unique_lock<std::mutex> lock(lock_);
    if (workingId_ != 0 && sessionId_ != 0) {
        LOGI("[PutMsg] task is running, wait for workdingId=%u end,seId=%u", workingId_, sequenceId);
        workingTaskcv_.wait(lock, [this] { return workingId_ == 0; });
    }
    if (sessionId_ != sessionId) {
        ResetInfo();
        sessionId_ = sessionId;
        messageMap_[sequenceId] = inMsg;
        SetEndField(isLastSequence, sequenceId);
        return E_OK;
    }
    // maybe remote has not receive ack, we resend ack.
    if (sequenceId <= hasFinishedMaxId_) {
        LOGI("[slwr] seId=%u,FinishedMId_=%u,label=%s", sequenceId, hasFinishedMaxId_, dataSync_->GetLabel().c_str());
        lock.unlock();
        dataSync_->SendDataAck(context_, inMsg, E_OK, 0);
        return -E_SLIDING_WINDOW_RECEIVER_INVALID_MSG;
    }
    int errCode = ErrHandle(sequenceId, packet->GetPacketId());
    if (errCode != E_OK) {
        return errCode;
    }
    if (messageMap_.count(sequenceId) > 0) {
        LOGI("[slwr] PutMsg sequenceId already in map label=%s,deviceId=%s",
            dataSync_->GetLabel().c_str(), STR_MASK(context_->GetDeviceId()));
        // it cannot be null here
        const auto *cachePacket = messageMap_[sequenceId]->GetObject<DataRequestPacket>();
        if (cachePacket->GetPacketId() > packet->GetPacketId()) {
            LOGI("[slwr] PutMsg receive old packet %u, new packet is %u",
                 packet->GetPacketId(), cachePacket->GetPacketId());
            return -E_SLIDING_WINDOW_RECEIVER_INVALID_MSG;
        }
        delete messageMap_[sequenceId];
        messageMap_[sequenceId] = nullptr;
    }
    messageMap_[sequenceId] = inMsg;
    SetEndField(isLastSequence, sequenceId);
    return E_OK;
}

void SlidingWindowReceiver::DealMsg()
{
    while (true) {
        Message *msg = nullptr;
        uint64_t curPacketId = 0;
        {
            std::lock_guard<std::mutex> lock(lock_);
            if (workingId_ != 0 || messageMap_.count(hasFinishedMaxId_ + 1) == 0) {
                LOGI("[slwr] DealMsg do nothing workingId_=%u,hasFinishedMaxId_=%u,label=%s,deviceId=%s",
                    workingId_, hasFinishedMaxId_, dataSync_->GetLabel().c_str(), STR_MASK(context_->GetDeviceId()));
                return;
            }
            uint32_t curWorkingId = hasFinishedMaxId_ + 1;
            msg = messageMap_[curWorkingId];
            messageMap_.erase(curWorkingId);
            const auto *packet = msg->GetObject<DataRequestPacket>();
            curPacketId = packet->GetPacketId();
            if (curPacketId != 0 && curPacketId < hasFinishedPacketId_) {
                delete msg;
                msg = nullptr;
                LOGI("[slwr] DealMsg ignore msg, packetId:%u,label=%s,deviceId=%s",
                    curPacketId, dataSync_->GetLabel().c_str(), STR_MASK(context_->GetDeviceId()));
                continue;
            }
            workingId_ = curWorkingId;
            LOGI("[slwr] DealMsg workingId_=%u,label=%s,deviceId=%s", workingId_,
                dataSync_->GetLabel().c_str(), STR_MASK(context_->GetDeviceId()));
        }
        int errCode = context_->HandleDataRequestRecv(msg);
        delete msg;
        msg = nullptr;
        {
            std::lock_guard<std::mutex> lock(lock_);
            workingId_ = 0;
            bool isWaterMarkErr = context_->IsReceiveWaterMarkErr();
            if (isWaterMarkErr || errCode == -E_NEED_ABILITY_SYNC) {
                ClearMap();
                hasFinishedMaxId_ = 0;
                endId_ = 0;
                isWaterMarkErrHappened_ = true;
            } else {
                hasFinishedMaxId_++;
                hasFinishedPacketId_ = curPacketId;
                LOGI("[slwr] DealMsg ok hasFinishedMaxId_=%u,label=%s,deviceId=%s", hasFinishedMaxId_,
                    dataSync_->GetLabel().c_str(), STR_MASK(context_->GetDeviceId()));
            }
            context_->SetReceiveWaterMarkErr(false);
            workingTaskcv_.notify_all();
        }
    }
}

int SlidingWindowReceiver::TimeOut(TimerId timerId)
{
    {
        std::lock_guard<std::mutex> lock(lock_);
        LOGI("[slwr] TimeOut,timerId[%llu], timerId_[%llu]", timerId, timerId_);
        if (timerId == timerId_) {
            ClearMap();
        }
        timerId_ = 0;
    }
    RuntimeContext::GetInstance()->RemoveTimer(timerId);
    return E_OK;
}

void SlidingWindowReceiver::StartTimer()
{
    std::lock_guard<std::mutex> lock(lock_);
    TimerId timerId = 0;
    RefObject::IncObjRef(context_);
    TimerAction timeOutCallback = std::bind(&SlidingWindowReceiver::TimeOut, this, std::placeholders::_1);
    int errCode = RuntimeContext::GetInstance()->SetTimer(IDLE_TIME_OUT, timeOutCallback,
        [this]() {
            int ret = RuntimeContext::GetInstance()->ScheduleTask([this]() {
                RefObject::DecObjRef(context_);
            });
            if (ret != E_OK) {
                LOGE("[slwr] timer finalizer ScheduleTask, errCode=%d", ret);
            }
        }, timerId);
    if (errCode != E_OK) {
        RefObject::DecObjRef(context_);
        LOGE("[slwr] timer ScheduleTask, errCode=%d", errCode);
        return;
    }
    StopTimer(timerId_);
    timerId_ = timerId;
    LOGD("[slwr] StartTimer timerId[%llu] success", timerId_);
}

void SlidingWindowReceiver::StopTimer()
{
    TimerId timerId;
    {
        std::lock_guard<std::mutex> lock(lock_);
        timerId = timerId_;
        if (timerId_ == 0) {
            return;
        }
        timerId_ = 0;
    }
    StopTimer(timerId);
}

void SlidingWindowReceiver::StopTimer(TimerId timerId)
{
    if (timerId == 0) return;
    LOGD("[slwr] StopTimer,remove Timer id[%llu]", timerId);
    RuntimeContext::GetInstance()->RemoveTimer(timerId);
}

void SlidingWindowReceiver::ClearMap()
{
    LOGD("[slwr] ClearMap");
    for (auto &iter : messageMap_) {
        delete iter.second;
        iter.second = nullptr;
    }
    messageMap_.clear();
}

void SlidingWindowReceiver::ResetInfo()
{
    ClearMap();
    hasFinishedMaxId_ = 0;
    hasFinishedPacketId_ = 0;
    workingId_ = 0;
    endId_ = 0;
    isWaterMarkErrHappened_ = false;
}

int SlidingWindowReceiver::ErrHandle(uint32_t sequenceId, uint64_t packetId)
{
    if (sequenceId == workingId_ || (endId_ != 0 && sequenceId > endId_)
        || (packetId > 0 && packetId < hasFinishedPacketId_)) {
        LOGI("[slwr] PutMsg sequenceId:%u, endId_:%u, workingId_:%u, packetId:%u!",
            sequenceId, endId_, workingId_, packetId);
        return -E_SLIDING_WINDOW_RECEIVER_INVALID_MSG;
    }
    // if waterMark err when DealMsg(), the waterMark of msg in messageMap_ is also err, so we clear messageMap_
    // but before the sender receive the waterMark err ack, the receiver may receive waterMark err msg, so before
    // receiver receive the sequenceId 1 msg, it will drop msg.
    // note, there is a low probability risk, if sender receive the waterMark err ack, it correct the warterMark then
    // resend data, if the msg of bigger sequenceId arrive earlier than sequenceId 1, it will be drop, the sender
    // will sync timeout.
    if (isWaterMarkErrHappened_) {
        if (sequenceId == 1) {
            isWaterMarkErrHappened_ = false;
        } else {
            LOGI("[slwr] PutMsg With waterMark Error sequenceId:%u, endId_:%u, workingId_:%u, packetId:%u!",
                sequenceId, endId_, workingId_, packetId);
            return -E_SLIDING_WINDOW_RECEIVER_INVALID_MSG; // drop invalid packet
        }
    }
    return E_OK;
}

void SlidingWindowReceiver::SetEndField(bool isLastSequence, uint32_t sequenceId)
{
    if (isLastSequence) {
        endId_ = sequenceId;
    }
}
}  // namespace DistributedDB