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

#ifndef MOCK_SINGLE_VER_STATE_MACHINE_H
#define MOCK_SINGLE_VER_STATE_MACHINE_H

#include <gmock/gmock.h>
#include "single_ver_sync_state_machine.h"

namespace DistributedDB {
class MockSingleVerStateMachine : public SingleVerSyncStateMachine {
public:
    void CallStepToTimeout(TimerId timerId)
    {
        SingleVerSyncStateMachine::StepToTimeout(timerId);
    }

    int CallExecNextTask()
    {
        return SyncStateMachine::ExecNextTask();
    }

    int CallTimeMarkSyncRecv(const Message *inMsg)
    {
        return SingleVerSyncStateMachine::TimeMarkSyncRecv(inMsg);
    }

    void CallDataAckRecvErrCodeHandle(int errCode, bool handleError)
    {
        SingleVerSyncStateMachine::DataAckRecvErrCodeHandle(errCode, handleError);
    }

    bool CallStartSaveDataNotify(uint32_t sessionId, uint32_t sequenceId, uint32_t inMsgId)
    {
        return SingleVerSyncStateMachine::StartSaveDataNotify(sessionId, sequenceId, inMsgId);
    }

    void CallStopSaveDataNotify()
    {
        SingleVerSyncStateMachine::StopSaveDataNotify();
    }

    void CallSwitchMachineState(uint8_t event)
    {
        SyncStateMachine::SwitchMachineState(event);
    }

    uint8_t GetCurrentState()
    {
        return currentState_;
    }

    void CallResponsePullError(int errCode, bool ignoreInnerErr)
    {
        SingleVerSyncStateMachine::ResponsePullError(errCode, ignoreInnerErr);
    }

    MOCK_METHOD1(SwitchStateAndStep, void(uint8_t));

    MOCK_METHOD0(PrepareNextSyncTask, int(void));

    MOCK_METHOD3(DoSaveDataNotify, void(uint32_t, uint32_t, uint32_t));

    MOCK_METHOD0(DoTimeSync, Event());
};
} // namespace DistributedDB
#endif  // #define MOCK_SINGLE_VER_STATE_MACHINE_H