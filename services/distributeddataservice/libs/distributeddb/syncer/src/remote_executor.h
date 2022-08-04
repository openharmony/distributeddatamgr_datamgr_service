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

#ifndef REMOTE_EXECUTOR_H
#define REMOTE_EXECUTOR_H

#include <deque>
#include <queue>

#include "db_types.h"
#include "distributeddb/result_set.h"
#include "icommunicator.h"
#include "isync_interface.h"
#include "message.h"
#include "relational_result_set_impl.h"
#include "remote_executor_packet.h"
#include "runtime_context.h"

namespace DistributedDB {
class RemoteExecutor : public RefObject {
public:
    enum class Status {
        WAITING = 0,
        WORKING
    };

    using OnFinished = std::function<void(int32_t, std::shared_ptr<ResultSet>)>;

    struct Task {
        Status status = Status::WAITING;
        uint32_t taskId = 0u;
        uint64_t timeout = 0u;
        uint32_t targetCount = 0;
        uint32_t currentCount = 0;
        uint64_t connectionId = 0u;
        std::string target;
        RemoteCondition condition;
        OnFinished onFinished = nullptr;
        std::shared_ptr<RelationalResultSetImpl> result;
    };

    RemoteExecutor();

    ~RemoteExecutor() = default;

    int Initialize(ISyncInterface *syncInterface, ICommunicator *communicator);

    int RemoteQuery(const std::string device, const RemoteCondition &condition,
        uint64_t timeout, uint64_t connectionId, std::shared_ptr<ResultSet> &result);

    // receive request and ack, and process in another thread
    int ReceiveMessage(const std::string &targetDev, Message *inMsg);

    void NotifyDeviceOffline(const std::string &device);

    void NotifyUserChange();

    void Close();

    void NotifyConnectionClosed(uint64_t connectionId);

private:

    void ReceiveMessageInner(const std::string &targetDev, Message *inMsg);

    int ReceiveRemoteExecutorRequest(const std::string &targetDev, Message *inMsg);

    int ReceiveRemoteExecutorAck(const std::string &targetDev, Message *inMsg);

    void ParseOneRequestMessage(const std::string &device, Message *inMsg);

    int CheckPermissions(const std::string &device);

    int SendRemoteExecutorData(const std::string &device, const Message *inMsg);

    bool CheckParamValid(const std::string &device, uint64_t timeout) const;

    bool CheckTaskExeStatus(const std::string &device);

    uint32_t GenerateSessionId();
    uint32_t GenerateTaskId();

    int RemoteQueryInner(const Task &task);
    void TryExecuteTaskInLock(const std::string &device);
    void DoRollBack(uint32_t sessionId);

    int RequestStart(uint32_t sessionId);

    void ResponseFailed(int errCode, uint32_t sessionId, uint32_t sequenceId, const std::string &device);
    int ResponseData(RelationalRowDataSet &&dataSet, uint32_t sessionId, uint32_t sequenceId, bool isLast,
        const std::string &device);
    int ResponseStart(RemoteExecutorAckPacket *packet, uint32_t sessionId, uint32_t sequenceId,
        const std::string &device);

    void StartTimer(uint64_t timeout, uint32_t sessionId);
    void RemoveTimer(uint32_t sessionId);
    int TimeoutCallBack(TimerId timerId);
    void DoTimeout(TimerId timerId);

    void DoSendFailed(uint32_t sessionId, int errCode);
    void DoFinished(uint32_t sessionId, int errCode);

    int ClearTaskInfo(uint32_t sessionId, Task &task);
    void ClearInnerSource();

    int FillRequestPacket(RemoteExecutorRequestPacket *packet, uint32_t sessionId, std::string &target);

    bool IsPackgetValid(uint32_t sessionId);
    void ReceiveDataWithValidSession(const std::string &targetDev, uint32_t sessionId, uint32_t sequenceId,
        const RemoteExecutorAckPacket *packget);

    void RemoveTaskByDevice(const std::string &device, std::vector<uint32_t> &removeList);
    void RemoveAllTask(int errCode);
    void RemoveTaskByConnection(uint64_t connectionId, std::vector<uint32_t> &removeList);

    ICommunicator *GetAndIncCommunicator() const;
    ISyncInterface *GetAndIncSyncInterface() const;

    std::mutex taskLock_;
    std::map<std::string, std::deque<uint32_t>> searchTaskQueue_; // key is device, value is sessionId queue
    std::map<std::string, std::set<uint32_t>> deviceWorkingSet_; // key is device, value is sessionId set
    std::map<uint32_t, Task> taskMap_; // key is sessionId

    std::mutex timeoutLock_;
    std::map<TimerId, uint32_t> timeoutMap_; // use to abort task when timeout
    std::map<uint32_t, TimerId> taskFinishMap_; // use to wake up timer when task finished

    std::mutex msgQueueLock_;
    std::queue<std::pair<std::string, Message *>> searchMessageQueue_;
    std::atomic<uint32_t> workingThreadsCount_;

    mutable std::mutex innerSourceLock_;
    ISyncInterface *syncInterface_;
    ICommunicator *communicator_;

    uint32_t lastSessionId_;
    uint32_t lastTaskId_;
    std::atomic<bool> closed_;

    std::condition_variable clearCV_;  // msgQueueLock_
};
}
#endif