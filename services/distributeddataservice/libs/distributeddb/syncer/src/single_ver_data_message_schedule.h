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

#ifndef SINGLE_VER_DATA_MESSAGE_SCHEDULE_H
#define SINGLE_VER_DATA_MESSAGE_SCHEDULE_H
#include <memory>
#include <map>
#include <mutex>
#include <queue>

#include "message.h"
#include "runtime_context.h"
#include "single_ver_sync_task_context.h"

namespace DistributedDB {
class SingleVerDataMessageSchedule {
public:
    SingleVerDataMessageSchedule() = default;
    ~SingleVerDataMessageSchedule();
    void Initialize(const std::string &label, const std::string &deviceId);
    void PutMsg(Message *inMsg);
    bool IsNeedReloadQueue();
    Message *MoveNextMsg(SingleVerSyncTaskContext *context, bool &isNeedHandle, bool &isNeedContinue);
    void ScheduleInfoHandle(bool isNeedHandleStatus, bool isNeedClearMap, const Message *inMsg);
    void ClearMsg();
private:
    void UpdateMsgMap();
    void UpdateMsgMapInner(std::queue<Message *> &msgTmpQueue);
    Message *UpdateMsgMapIfNeed(Message *msg);
    Message *GetMsgFromMap(bool &isNeedHandle);
    Message *GetLastMsgFromQueue();
    void ClearMsgMap();
    void ClearMsgMapWithNoLock();
    void ClearMsgQueue();
    void StartTimer(SingleVerSyncTaskContext *context);
    void StopTimer();
    void ResetTimer(SingleVerSyncTaskContext *context);
    // when timeout queue size is 0 because thread can move queue msg to map if isNeedReload which is
    // activated when queue has new msg is true
    // so only need clear map msg
    int TimeOut(TimerId timerId);

    static constexpr int IDLE_TIME_OUT = 5 * 60 * 1000; // 5min
    std::mutex queueLock_;
    std::queue<Message *> msgQueue_;
    bool isNeedReload_ = false;
    // only one thread is deal msg
    std::mutex workingLock_;
    bool isWorking_ = false;
    // first:sequenceId second:Message*, deal msg from low sequenceId to high sequenceId
    std::mutex lock_;
    std::map<uint32_t, Message *> messageMap_;
    uint32_t prevSessionId_ = 0; // drop the msg if msg sessionId is prev sessionId.
    uint32_t currentSessionId_ = 0;
    uint64_t finishedPacketId_ = 0; // next msg packetId should larger than it
    uint32_t expectedSequenceId_ = 0; // handle next msg which sequenceId is equal to it
    TimerId timerId_ = 0;

    std::string label_;
    std::string deviceId_;
};
}
#endif // SINGLE_VER_DATA_MESSAGE_SCHEDULE_H