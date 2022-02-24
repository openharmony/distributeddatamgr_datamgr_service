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
#include "single_ver_data_message_schedule.h"

#include "db_common.h"
#include "log_print.h"
#include "version.h"
#include "single_ver_data_sync.h"

namespace DistributedDB {
SingleVerDataMessageSchedule::~SingleVerDataMessageSchedule()
{
    LOGD("~SingleVerDataMessageSchedule");
    ClearMsg();
}

void SingleVerDataMessageSchedule::Initialize(const std::string &label, const std::string &deviceId)
{
    label_ = label;
    deviceId_ = deviceId;
}

void SingleVerDataMessageSchedule::PutMsg(Message *inMsg)
{
    if (inMsg == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(queueLock_);
    msgQueue_.push(inMsg);
    isNeedReload_ = true;
}

bool SingleVerDataMessageSchedule::IsNeedReloadQueue()
{
    std::lock_guard<std::mutex> lock(queueLock_);
    return isNeedReload_;
}

Message *SingleVerDataMessageSchedule::MoveNextMsg(SingleVerSyncTaskContext *context, bool &isNeedHandle,
    bool &isNeedContinue)
{
    uint32_t remoteVersion = context->GetRemoteSoftwareVersion();
    if (remoteVersion < SOFTWARE_VERSION_RELEASE_3_0) {
        // just get last msg when remote version is < 103 or >=103 but just open db now
        return GetLastMsgFromQueue();
    }
    {
        std::lock_guard<std::mutex> lock(workingLock_);
        if (isWorking_) {
            isNeedContinue = false;
            return nullptr;
        }
        isWorking_ = true;
    }
    ResetTimer(context);
    UpdateMsgMap();
    Message *msg = GetMsgFromMap(isNeedHandle);
    isNeedContinue = true;
    if (msg == nullptr) {
        std::lock_guard<std::mutex> lock(workingLock_);
        isWorking_ = false;
        return nullptr;
    }
    return msg;
}

void SingleVerDataMessageSchedule::ScheduleInfoHandle(bool isNeedHandleStatus, bool isNeedClearMap,
    const Message *inMsg)
{
    if (isNeedHandleStatus) {
        const DataRequestPacket *packet = inMsg->GetObject<DataRequestPacket>();
        if (packet == nullptr) {
            LOGE("[DataMsgSchedule] packet is nullptr");
            return;
        }
        uint64_t curPacketId = packet->GetPacketId();
        {
            std::lock_guard<std::mutex> lock(lock_);
            finishedPacketId_ = curPacketId;
            if (isNeedClearMap) {
                ClearMsgMapWithNoLock();
                expectedSequenceId_ = 1;
            } else {
                LOGI("[DataMsgSchedule] DealMsg seqId=%u finishedPacketId=%llu ok,label=%s,dev=%s", expectedSequenceId_,
                    finishedPacketId_, label_.c_str(), STR_MASK(deviceId_));
                expectedSequenceId_++;
            }
        }
    }
    std::lock_guard<std::mutex> lock(workingLock_);
    isWorking_ = false;
}

void SingleVerDataMessageSchedule::ClearMsg()
{
    StopTimer();
    ClearMsgQueue();
    ClearMsgMap();
}

void SingleVerDataMessageSchedule::UpdateMsgMap()
{
    std::queue<Message *> msgTmpQueue;
    {
        std::lock_guard<std::mutex> lock(queueLock_);
        while (!msgQueue_.empty()) {
            msgTmpQueue.push(msgQueue_.front());
            msgQueue_.pop();
        }
        isNeedReload_ = false;
    }
    UpdateMsgMapInner(msgTmpQueue);
}

void SingleVerDataMessageSchedule::UpdateMsgMapInner(std::queue<Message *> &msgTmpQueue)
{
    // update msg map
    std::lock_guard<std::mutex> lock(lock_);
    while (!msgTmpQueue.empty()) {
        Message *msg = msgTmpQueue.front();
        msgTmpQueue.pop();
        if (msg == nullptr) {
            continue;
        }
        const DataRequestPacket *packet = msg->GetObject<DataRequestPacket>();
        if (packet == nullptr) {
            delete msg;
            continue;
        }
        uint32_t sessionId = msg->GetSessionId();
        uint32_t sequenceId = msg->GetSequenceId();
        uint64_t packetId = packet->GetPacketId();
        if (prevSessionId_ != 0 && sessionId == prevSessionId_) {
            LOGD("[DataMsgSchedule] recv prev sessionId msg,drop msg,label=%s,dev=%s", label_.c_str(),
                STR_MASK(deviceId_));
            delete msg;
            continue;
        }
        if (sessionId != currentSessionId_) {
            // make sure all msg sessionId is same in msgMap
            ClearMsgMapWithNoLock();
            prevSessionId_ = currentSessionId_;
            currentSessionId_ = sessionId;
            finishedPacketId_ = 0;
            expectedSequenceId_ = 1;
        }
        if (messageMap_.count(sequenceId) > 0) {
            const auto *cachePacket = messageMap_[sequenceId]->GetObject<DataRequestPacket>();
            if (cachePacket != nullptr) {
                LOGD("[DataMsgSchedule] msg packetId=%llu,cachePacketId=%llu,label=%s,dev=%s", packetId,
                    cachePacket->GetPacketId(), label_.c_str(), STR_MASK(deviceId_));
                if (packetId != 0 && packetId < cachePacket->GetPacketId()) {
                    delete msg;
                    continue;
                }
            }
            delete messageMap_[sequenceId];
            messageMap_[sequenceId] = nullptr;
        }
        messageMap_[sequenceId] = msg;
        LOGD("[DataMsgSchedule] put into msgMap seqId=%llu,packetId=%llu,label=%s,dev=%s", sequenceId, packetId,
            label_.c_str(), STR_MASK(deviceId_));
    }
}

Message *SingleVerDataMessageSchedule::GetMsgFromMap(bool &isNeedHandle)
{
    isNeedHandle = true;
    std::lock_guard<std::mutex> lock(lock_);
    while (!messageMap_.empty()) {
        auto iter = messageMap_.begin();
        Message *msg = iter->second;
        messageMap_.erase(iter);
        const DataRequestPacket *packet = msg->GetObject<DataRequestPacket>();
        if (packet == nullptr) {
            LOGE("[DataMsgSchedule] expected error");
            delete msg;
            continue;
        }
        uint32_t sequenceId = msg->GetSequenceId();
        uint64_t packetId = packet->GetPacketId();
        if (sequenceId < expectedSequenceId_) {
            uint64_t revisePacketId = finishedPacketId_ - (expectedSequenceId_ - 1 - sequenceId);
            LOGI("[DataMsgSchedule] drop msg because seqId less than exSeqId");
            if (packetId < revisePacketId) {
                delete msg;
                continue;
            }
            // means already handle the msg, and just send E_OK ack in dataSync
            isNeedHandle = false;
            return msg;
        }
        if (sequenceId == expectedSequenceId_) {
            if (packetId < finishedPacketId_) {
                LOGI("[DataMsgSchedule] drop msg because packetId less than finishedPacketId");
                delete msg;
                continue;
            }
            // if packetId == finishedPacketId_ need handle
            // it will happened while watermark/need_abilitySync when last ack is missing
            return msg;
        }
        // sequenceId > expectedSequenceId_, not need handle, put into map again
        messageMap_[sequenceId] = msg;
        return nullptr;
    }
    return nullptr;
}

Message *SingleVerDataMessageSchedule::GetLastMsgFromQueue()
{
    std::lock_guard<std::mutex> lock(queueLock_);
    isNeedReload_ = false;
    while (!msgQueue_.empty()) {
        Message *msg = msgQueue_.front();
        msgQueue_.pop();
        if (msgQueue_.empty()) { // means last msg
            return msg;
        }
        delete msg;
    }
    return nullptr;
}

void SingleVerDataMessageSchedule::ClearMsgMap()
{
    std::lock_guard<std::mutex> lock(lock_);
    ClearMsgMapWithNoLock();
}

void SingleVerDataMessageSchedule::ClearMsgMapWithNoLock()
{
    LOGD("[DataMsgSchedule] begin to ClearMsgMapWithNoLock");
    for (auto &iter : messageMap_) {
        delete iter.second;
        iter.second = nullptr;
    }
    messageMap_.clear();
}

void SingleVerDataMessageSchedule::ClearMsgQueue()
{
    std::lock_guard<std::mutex> lock(queueLock_);
    while (!msgQueue_.empty()) {
        Message *msg = msgQueue_.front();
        msgQueue_.pop();
        delete msg;
    }
}

void SingleVerDataMessageSchedule::StartTimer(SingleVerSyncTaskContext *context)
{
    std::lock_guard<std::mutex> lock(lock_);
    TimerId timerId = 0;
    RefObject::IncObjRef(context);
    TimerAction timeOutCallback = std::bind(&SingleVerDataMessageSchedule::TimeOut, this, std::placeholders::_1);
    int errCode = RuntimeContext::GetInstance()->SetTimer(IDLE_TIME_OUT, timeOutCallback,
        [context]() {
            int errCode = RuntimeContext::GetInstance()->ScheduleTask([context]() {
                RefObject::DecObjRef(context);
            });
            if (errCode != E_OK) {
                LOGE("[DataMsgSchedule] timer finalizer ScheduleTask,errCode=%d", errCode);
            }
        }, timerId);
    if (errCode != E_OK) {
        RefObject::DecObjRef(context);
        LOGE("[DataMsgSchedule] timer ScheduleTask, errCode=%d", errCode);
        return;
    }
    timerId_ = timerId;
    LOGD("[DataMsgSchedule] StartTimer,TimerId=%llu", timerId_);
}

void SingleVerDataMessageSchedule::StopTimer()
{
    TimerId timerId;
    {
        std::lock_guard<std::mutex> lock(lock_);
        LOGD("[DataMsgSchedule] StopTimer,remove TimerId=%llu", timerId_);
        if (timerId_ == 0) {
            return;
        }
        timerId = timerId_;
        timerId_ = 0;
    }
    RuntimeContext::GetInstance()->RemoveTimer(timerId);
}

void SingleVerDataMessageSchedule::ResetTimer(SingleVerSyncTaskContext *context)
{
    StopTimer();
    StartTimer(context);
}

int SingleVerDataMessageSchedule::TimeOut(TimerId timerId)
{
    if (IsNeedReloadQueue()) {
        LOGI("[DataMsgSchedule] new msg exists, no need to timeout handle");
        return E_OK;
    }
    {
        std::lock_guard<std::mutex> lock(workingLock_);
        if (isWorking_) {
            LOGI("[DataMsgSchedule] other thread is handle msg, no need to timeout handle");
            return E_OK;
        }
    }
    {
        std::lock_guard<std::mutex> lock(lock_);
        LOGI("[DataMsgSchedule] timeout handling, stop timerId_[%llu]", timerId, timerId_);
        if (timerId == timerId_) {
            ClearMsgMapWithNoLock();
            timerId_ = 0;
        }
    }
    RuntimeContext::GetInstance()->RemoveTimer(timerId);
    return E_OK;
}
}