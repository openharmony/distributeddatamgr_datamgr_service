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

#include "single_ver_sync_state_machine.h"

#include <cmath>
#include <climits>
#include <algorithm>

#include "db_errno.h"
#include "log_print.h"
#include "sync_operation.h"
#include "message_transform.h"
#include "sync_types.h"
#include "db_common.h"
#include "runtime_context.h"
#include "performance_analysis.h"
#include "single_ver_sync_target.h"
#include "single_ver_data_sync.h"
#include "single_ver_data_sync_utils.h"

namespace DistributedDB {
using Event = SingleVerSyncStateMachine::Event;
using State = SingleVerSyncStateMachine::State;
namespace {
    // used for state switch table
    const int CURRENT_STATE_INDEX = 0;
    const int EVENT_INDEX = 1;
    const int OUTPUT_STATE_INDEX = 2;

    // drop v1 and v2 table by one optimize, dataSend mode in all version go with slide window mode.
    // State switch table v3, has three columns, CurrentState, Event, and OutSate
    const std::vector<std::vector<uint8_t>> STATE_SWITCH_TABLE_V3 = {
        {State::IDLE, Event::START_SYNC_EVENT, State::TIME_SYNC},

        // In TIME_SYNC state
        {State::TIME_SYNC, Event::TIME_SYNC_FINISHED_EVENT, State::ABILITY_SYNC},
        {State::TIME_SYNC, Event::TIME_OUT_EVENT, State::SYNC_TIME_OUT},
        {State::TIME_SYNC, Event::INNER_ERR_EVENT, State::INNER_ERR},

        // In ABILITY_SYNC state, compare version num and schema
        {State::ABILITY_SYNC, Event::VERSION_NOT_SUPPOR_EVENT, State::INNER_ERR},
        {State::ABILITY_SYNC, Event::ABILITY_SYNC_FINISHED_EVENT, State::START_INITIACTIVE_DATA_SYNC},
        {State::ABILITY_SYNC, Event::TIME_OUT_EVENT, State::SYNC_TIME_OUT},
        {State::ABILITY_SYNC, Event::INNER_ERR_EVENT, State::INNER_ERR},
        {State::ABILITY_SYNC, Event::CONTROL_CMD_EVENT, State::SYNC_CONTROL_CMD},

        // In START_INITIACTIVE_DATA_SYNC state, send a sync request, and send first packt of data sync
        {State::START_INITIACTIVE_DATA_SYNC, Event::NEED_ABILITY_SYNC_EVENT, State::ABILITY_SYNC},
        {State::START_INITIACTIVE_DATA_SYNC, Event::TIME_OUT_EVENT, State::SYNC_TIME_OUT},
        {State::START_INITIACTIVE_DATA_SYNC, Event::INNER_ERR_EVENT, State::INNER_ERR},
        {State::START_INITIACTIVE_DATA_SYNC, Event::SEND_FINISHED_EVENT, State::START_PASSIVE_DATA_SYNC},
        {State::START_INITIACTIVE_DATA_SYNC, Event::RE_SEND_DATA_EVENT, State::START_INITIACTIVE_DATA_SYNC},

        // In START_PASSIVE_DATA_SYNC state, do response pull request, and send first packt of data sync
        {State::START_PASSIVE_DATA_SYNC, Event::SEND_FINISHED_EVENT, State::START_PASSIVE_DATA_SYNC},
        {State::START_PASSIVE_DATA_SYNC, Event::RESPONSE_TASK_FINISHED_EVENT, State::WAIT_FOR_RECEIVE_DATA_FINISH},
        {State::START_PASSIVE_DATA_SYNC, Event::TIME_OUT_EVENT, State::SYNC_TIME_OUT},
        {State::START_PASSIVE_DATA_SYNC, Event::INNER_ERR_EVENT, State::INNER_ERR},
        {State::START_PASSIVE_DATA_SYNC, Event::NEED_ABILITY_SYNC_EVENT, State::ABILITY_SYNC},
        {State::START_PASSIVE_DATA_SYNC, Event::RE_SEND_DATA_EVENT, State::START_PASSIVE_DATA_SYNC},

        // In WAIT_FOR_RECEIVE_DATA_FINISH,
        {State::WAIT_FOR_RECEIVE_DATA_FINISH, Event::RECV_FINISHED_EVENT, State::SYNC_TASK_FINISHED},
        {State::WAIT_FOR_RECEIVE_DATA_FINISH, Event::START_PULL_RESPONSE_EVENT, State::START_PASSIVE_DATA_SYNC},
        {State::WAIT_FOR_RECEIVE_DATA_FINISH, Event::TIME_OUT_EVENT, State::SYNC_TIME_OUT},
        {State::WAIT_FOR_RECEIVE_DATA_FINISH, Event::INNER_ERR_EVENT, State::INNER_ERR},
        {State::WAIT_FOR_RECEIVE_DATA_FINISH, Event::NEED_ABILITY_SYNC_EVENT, State::ABILITY_SYNC},

        {State::SYNC_CONTROL_CMD, Event::SEND_FINISHED_EVENT, State::SYNC_TASK_FINISHED},
        {State::SYNC_CONTROL_CMD, Event::TIME_OUT_EVENT, State::SYNC_TIME_OUT},
        {State::SYNC_CONTROL_CMD, Event::INNER_ERR_EVENT, State::INNER_ERR},
        {State::SYNC_CONTROL_CMD, Event::NEED_ABILITY_SYNC_EVENT, State::ABILITY_SYNC},

        // In SYNC_TASK_FINISHED,
        {State::SYNC_TASK_FINISHED, Event::ALL_TASK_FINISHED_EVENT, State::IDLE},
        {State::SYNC_TASK_FINISHED, Event::START_SYNC_EVENT, State::TIME_SYNC},

        // SYNC_TIME_OUT and INNE_ERR state, just do some exception resolve
        {State::SYNC_TIME_OUT, Event::ANY_EVENT, State::SYNC_TASK_FINISHED},
        {State::INNER_ERR, Event::ANY_EVENT, State::SYNC_TASK_FINISHED},
    };
}

std::mutex SingleVerSyncStateMachine::stateSwitchTableLock_;
std::vector<StateSwitchTable> SingleVerSyncStateMachine::stateSwitchTables_;
bool SingleVerSyncStateMachine::isStateSwitchTableInited_ = false;

SingleVerSyncStateMachine::SingleVerSyncStateMachine()
    : context_(nullptr),
      syncInterface_(nullptr),
      timeSync_(nullptr),
      abilitySync_(nullptr),
      dataSync_(nullptr),
      currentRemoteVersionId_(0)
{
}

SingleVerSyncStateMachine::~SingleVerSyncStateMachine()
{
    LOGD("~SingleVerSyncStateMachine");
    Clear();
}

int SingleVerSyncStateMachine::Initialize(ISyncTaskContext *context, ISyncInterface *syncInterface,
    std::shared_ptr<Metadata> &metaData, ICommunicator *communicator)
{
    if ((context == nullptr) || (syncInterface == nullptr) || (metaData == nullptr) || (communicator == nullptr)) {
        return -E_INVALID_ARGS;
    }

    int errCode = SyncStateMachine::Initialize(context, syncInterface, metaData, communicator);
    if (errCode != E_OK) {
        return errCode;
    }

    timeSync_ = std::make_unique<TimeSync>();
    dataSync_ = std::make_shared<SingleVerDataSync>();
    abilitySync_ = std::make_unique<AbilitySync>();

    errCode = timeSync_->Initialize(communicator, metaData, syncInterface, context->GetDeviceId());
    if (errCode != E_OK) {
        goto ERROR_OUT;
    }
    errCode = dataSync_->Initialize(syncInterface, communicator, metaData, context->GetDeviceId());
    if (errCode != E_OK) {
        goto ERROR_OUT;
    }
    errCode = abilitySync_->Initialize(communicator, syncInterface, metaData, context->GetDeviceId());
    if (errCode != E_OK) {
        goto ERROR_OUT;
    }

    currentState_ = IDLE;
    context_ = static_cast<SingleVerSyncTaskContext *>(context);
    syncInterface_ = static_cast<SingleVerKvDBSyncInterface *>(syncInterface);

    InitStateSwitchTables();
    InitStateMapping();
    return E_OK;

ERROR_OUT:
    Clear();
    return errCode;
}

void SingleVerSyncStateMachine::SyncStep()
{
    RefObject::IncObjRef(context_);
    RefObject::IncObjRef(communicator_);
    int errCode = RuntimeContext::GetInstance()->ScheduleTask(
        std::bind(&SingleVerSyncStateMachine::SyncStepInnerLocked, this));
    if (errCode != E_OK) {
        LOGE("[StateMachine][SyncStep] Schedule SyncStep failed");
        RefObject::DecObjRef(communicator_);
        RefObject::DecObjRef(context_);
    }
}

int SingleVerSyncStateMachine::ReceiveMessageCallback(Message *inMsg)
{
    int errCode = MessageCallbackPre(inMsg);
    if (errCode != E_OK) {
        return errCode;
    }
    switch (inMsg->GetMessageId()) {
        case TIME_SYNC_MESSAGE:
            errCode = TimeMarkSyncRecv(inMsg);
            break;
        case ABILITY_SYNC_MESSAGE:
            errCode = AbilitySyncRecv(inMsg);
            break;
        case DATA_SYNC_MESSAGE:
        case QUERY_SYNC_MESSAGE:
            errCode = DataPktRecv(inMsg);
            break;
        case CONTROL_SYNC_MESSAGE:
            errCode = ControlPktRecv(inMsg);
            break;
        default:
            errCode = -E_NOT_SUPPORT;
    }
    return errCode;
}

void SingleVerSyncStateMachine::SyncStepInnerLocked()
{
    if (context_->IncUsedCount() != E_OK) {
        goto SYNC_STEP_OUT;
    }
    {
        std::lock_guard<std::mutex> lock(stateMachineLock_);
        SyncStepInner();
    }
    context_->SafeExit();

SYNC_STEP_OUT:
    RefObject::DecObjRef(communicator_);
    RefObject::DecObjRef(context_);
}

void SingleVerSyncStateMachine::SyncStepInner()
{
    Event event = INNER_ERR_EVENT;
    do {
        auto iter = stateMapping_.find(currentState_);
        if (iter != stateMapping_.end()) {
            event = static_cast<Event>(iter->second());
        } else {
            LOGE("[StateMachine][SyncStepInner] can not find state=%d,label=%s,dev=%s", currentState_,
                dataSync_->GetLabel().c_str(), STR_MASK(context_->GetDeviceId()));
            break;
        }
    } while (event != Event::WAIT_ACK_EVENT && SwitchMachineState(event) == E_OK && currentState_ != IDLE);
}

void SingleVerSyncStateMachine::SetCurStateErrStatus()
{
    currentState_ = State::INNER_ERR;
}

int SingleVerSyncStateMachine::StartSyncInner()
{
    PerformanceAnalysis *performance = PerformanceAnalysis::GetInstance();
    if (performance != nullptr) {
        performance->StepTimeRecordStart(PT_TEST_RECORDS::RECORD_MACHINE_START_TO_PUSH_SEND);
    }
    int errCode = PrepareNextSyncTask();
    if (errCode == E_OK) {
        SwitchStateAndStep(Event::START_SYNC_EVENT);
    }
    return errCode;
}

void SingleVerSyncStateMachine::AbortInner()
{
    LOGE("[StateMachine][AbortInner] error occurred,abort,label=%s,dev=%s", dataSync_->GetLabel().c_str(),
        STR_MASK(context_->GetDeviceId()));
    if (context_->IsKilled()) {
        dataSync_->ClearDataMsg();
    }
    dataSync_->ClearSyncStatus();
    ContinueToken token;
    context_->GetContinueToken(token);
    if (token != nullptr) {
        syncInterface_->ReleaseContinueToken(token);
    }
    context_->SetContinueToken(nullptr);
    context_->Clear();
}

const std::vector<StateSwitchTable> &SingleVerSyncStateMachine::GetStateSwitchTables() const
{
    return stateSwitchTables_;
}

int SingleVerSyncStateMachine::PrepareNextSyncTask()
{
    int errCode = StartWatchDog();
    if (errCode != E_OK) {
        LOGE("[StateMachine][PrepareNextSyncTask] WatchDog start failed,err=%d", errCode);
        return errCode;
    }

    if (currentState_ != State::IDLE && currentState_ != State::SYNC_TASK_FINISHED) {
        LOGW("[StateMachine][PrepareNextSyncTask] PreSync may get an err, state=%d,dev=%s",
            currentState_, STR_MASK(context_->GetDeviceId()));
        currentState_ = State::IDLE;
    }
    return E_OK;
}

void SingleVerSyncStateMachine::SendSaveDataNotifyPacket(uint32_t sessionId, uint32_t sequenceId, uint32_t inMsgId)
{
    dataSync_->SendSaveDataNotifyPacket(context_,
        std::min(context_->GetRemoteSoftwareVersion(), SOFTWARE_VERSION_CURRENT), sessionId, sequenceId, inMsgId);
}

void SingleVerSyncStateMachine::CommErrAbort()
{
    std::lock_guard<std::mutex> lock(stateMachineLock_);
    if (SwitchMachineState(Event::INNER_ERR_EVENT) == E_OK) {
        SyncStep();
    }
}

void SingleVerSyncStateMachine::InitStateSwitchTables()
{
    if (isStateSwitchTableInited_) {
        return;
    }

    std::lock_guard<std::mutex> lock(stateSwitchTableLock_);
    if (isStateSwitchTableInited_) {
        return;
    }

    InitStateSwitchTable(SINGLE_VER_SYNC_PROCTOL_V3, STATE_SWITCH_TABLE_V3);
    std::sort(stateSwitchTables_.begin(), stateSwitchTables_.end(),
        [](const auto &tableA, const auto &tableB) {
            return tableA.version > tableB.version;
        }); // descending
    isStateSwitchTableInited_ = true;
}

void SingleVerSyncStateMachine::InitStateSwitchTable(uint32_t version,
    const std::vector<std::vector<uint8_t>> &switchTable)
{
    StateSwitchTable table;
    table.version = version;
    for (const auto &stateSwitch : switchTable) {
        if (stateSwitch.size() <= OUTPUT_STATE_INDEX) {
            LOGE("[StateMachine][InitSwitchTable] stateSwitch size err,size=%llu", stateSwitch.size());
            return;
        }
        if (table.switchTable.count(stateSwitch[CURRENT_STATE_INDEX]) == 0) {
            EventToState eventToState; // new EventToState
            eventToState[stateSwitch[EVENT_INDEX]] = stateSwitch[OUTPUT_STATE_INDEX];
            table.switchTable[stateSwitch[CURRENT_STATE_INDEX]] = eventToState;
        } else { // key stateSwitch[CURRENT_STATE_INDEX] already has EventToState
            EventToState &eventToState = table.switchTable[stateSwitch[CURRENT_STATE_INDEX]];
            eventToState[stateSwitch[EVENT_INDEX]] = stateSwitch[OUTPUT_STATE_INDEX];
        }
    }
    stateSwitchTables_.push_back(table);
}

void SingleVerSyncStateMachine::InitStateMapping()
{
    stateMapping_[TIME_SYNC] = std::bind(&SingleVerSyncStateMachine::DoTimeSync, this);
    stateMapping_[ABILITY_SYNC] = std::bind(&SingleVerSyncStateMachine::DoAbilitySync, this);
    stateMapping_[WAIT_FOR_RECEIVE_DATA_FINISH] = std::bind(&SingleVerSyncStateMachine::DoWaitForDataRecv, this);
    stateMapping_[SYNC_TASK_FINISHED] = std::bind(&SingleVerSyncStateMachine::DoSyncTaskFinished, this);
    stateMapping_[SYNC_TIME_OUT] = std::bind(&SingleVerSyncStateMachine::DoTimeout, this);
    stateMapping_[INNER_ERR] = std::bind(&SingleVerSyncStateMachine::DoInnerErr, this);
    stateMapping_[START_INITIACTIVE_DATA_SYNC] =
        std::bind(&SingleVerSyncStateMachine::DoInitiactiveDataSyncWithSlidingWindow, this);
    stateMapping_[START_PASSIVE_DATA_SYNC] =
        std::bind(&SingleVerSyncStateMachine::DoPassiveDataSyncWithSlidingWindow, this);
    stateMapping_[SYNC_CONTROL_CMD] = std::bind(&SingleVerSyncStateMachine::DoInitiactiveControlSync, this);
}

Event SingleVerSyncStateMachine::DoInitiactiveDataSyncWithSlidingWindow()
{
    LOGD("[StateMachine][activeDataSync] mode=%d,label=%s,dev=%s", context_->GetMode(),
        dataSync_->GetLabel().c_str(), STR_MASK(context_->GetDeviceId()));
    int errCode = E_OK;
    switch (context_->GetMode()) {
        case SyncModeType::PUSH:
        case SyncModeType::QUERY_PUSH:
            context_->SetOperationStatus(SyncOperation::OP_RECV_FINISHED);
            errCode = dataSync_->SyncStart(context_->GetMode(), context_);
            break;
        case SyncModeType::PULL:
        case SyncModeType::QUERY_PULL:
            context_->SetOperationStatus(SyncOperation::OP_SEND_FINISHED);
            errCode = dataSync_->SyncStart(context_->GetMode(), context_);
            break;
        case SyncModeType::PUSH_AND_PULL:
        case SyncModeType::QUERY_PUSH_PULL:
            errCode = dataSync_->SyncStart(context_->GetMode(), context_);
            break;
        case SyncModeType::RESPONSE_PULL:
            errCode = dataSync_->SyncStart(context_->GetMode(), context_);
            break;
        default:
            errCode = -E_NOT_SUPPORT;
            break;
    }
    if (errCode == E_OK) {
        return Event::WAIT_ACK_EVENT;
    }
    // once E_EKEYREVOKED error occurred, PUSH_AND_PULL mode should wait for ack to pull remote data.
    if (SyncOperation::TransferSyncMode(context_->GetMode()) == SyncModeType::PUSH_AND_PULL &&
        errCode == -E_EKEYREVOKED) {
        return Event::WAIT_ACK_EVENT;
    }

    // when response task step dataSync again while request task is running,  ignore the errCode
    bool ignoreInnerErr = (context_->GetResponseSessionId() != 0 && context_->GetRequestSessionId() != 0);
    Event event = TransformErrCodeToEvent(errCode);
    return (ignoreInnerErr && event == INNER_ERR_EVENT) ? SEND_FINISHED_EVENT : event;
}

Event SingleVerSyncStateMachine::DoPassiveDataSyncWithSlidingWindow()
{
    {
        RefObject::AutoLock lock(context_);
        if (context_->GetRspTargetQueueSize() != 0) {
            PreStartPullResponse();
        } else {
            return RESPONSE_TASK_FINISHED_EVENT;
        }
    }
    int errCode = dataSync_->SyncStart(SyncModeType::RESPONSE_PULL, context_);
    if (errCode != E_OK) {
        LOGW("[SingleVerSyncStateMachine][DoPassiveDataSyncWithSlidingWindow] response pull send failed[%d]", errCode);
        return RESPONSE_TASK_FINISHED_EVENT;
    }
    return Event::WAIT_ACK_EVENT;
}

Event SingleVerSyncStateMachine::DoInitiactiveControlSync()
{
    LOGD("[StateMachine][activeControlSync] mode=%d,label=%s,dev=%s", context_->GetMode(),
        dataSync_->GetLabel().c_str(), STR_MASK(context_->GetDeviceId()));
    context_->SetOperationStatus(SyncOperation::OP_RECV_FINISHED);
    int errCode = dataSync_->ControlCmdStart(context_);
    if (errCode == E_OK) {
        return Event::WAIT_ACK_EVENT;
    }
    context_->SetTaskErrCode(errCode);
    return TransformErrCodeToEvent(errCode);
}

int SingleVerSyncStateMachine::HandleControlAckRecv(const Message *inMsg)
{
    std::lock_guard<std::mutex> lock(stateMachineLock_);
    if (IsNeedResetWatchdog(inMsg)) {
        (void)ResetWatchDog();
    }
    int errCode = dataSync_->ControlCmdAckRecv(context_, inMsg);
    ControlAckRecvErrCodeHandle(errCode);
    SwitchStateAndStep(TransformErrCodeToEvent(errCode));
    return E_OK;
}

Event SingleVerSyncStateMachine::DoWaitForDataRecv() const
{
    if (context_->GetRspTargetQueueSize() != 0) {
        return START_PULL_RESPONSE_EVENT;
    }
    if (context_->GetOperationStatus() == SyncOperation::OP_FINISHED_ALL) {
        return RECV_FINISHED_EVENT;
    }
    if (SyncOperation::TransferSyncMode(context_->GetMode()) == SyncModeType::PUSH_AND_PULL &&
        context_->GetOperationStatus() == SyncOperation::OP_EKEYREVOKED_FAILURE &&
        context_->GetRemoteSoftwareVersion() > SOFTWARE_VERSION_RELEASE_2_0) {
        return RECV_FINISHED_EVENT;
    }
    return Event::WAIT_ACK_EVENT;
}

Event SingleVerSyncStateMachine::DoTimeSync()
{
    if (timeSync_->IsNeedSync()) {
        CommErrHandler handler = nullptr;
        // Auto sync need do retry don't use errHandler to return.
        if (!context_->IsAutoSync()) {
            handler = std::bind(&SyncTaskContext::CommErrHandlerFunc, std::placeholders::_1,
                context_, context_->GetRequestSessionId());
        }
        int errCode = timeSync_->SyncStart(handler, context_->GetRequestSessionId());
        if (errCode == E_OK) {
            return Event::WAIT_ACK_EVENT;
        }
        context_->SetTaskErrCode(errCode);
        return TransformErrCodeToEvent(errCode);
    }

    return Event::TIME_SYNC_FINISHED_EVENT;
}

Event SingleVerSyncStateMachine::DoAbilitySync()
{
    uint16_t remoteCommunicatorVersion = 0;
    int errCode = communicator_->GetRemoteCommunicatorVersion(context_->GetDeviceId(), remoteCommunicatorVersion);
    if (errCode != E_OK) {
        LOGE("[StateMachine][DoAbilitySync] Get RemoteCommunicatorVersion errCode=%d", errCode);
        return Event::INNER_ERR_EVENT;
    }
    // Fistr version, not support AbilitySync
    if (remoteCommunicatorVersion == 0) {
        context_->SetRemoteSoftwareVersion(SOFTWARE_VERSION_EARLIEST);
        return GetEventAfterTimeSync(context_->GetMode());
    }
    if (context_->GetIsNeedResetAbilitySync()) {
        abilitySync_->SetAbilitySyncFinishedStatus(false);
        context_->SetIsNeedResetAbilitySync(false);
    }
    if (abilitySync_->GetAbilitySyncFinishedStatus()) {
        return GetEventAfterTimeSync(context_->GetMode());
    }

    CommErrHandler handler = std::bind(&SyncTaskContext::CommErrHandlerFunc, std::placeholders::_1,
        context_, context_->GetRequestSessionId());
    LOGI("[StateMachine][AbilitySync] start abilitySync,label=%s,dev=%s", dataSync_->GetLabel().c_str(),
        STR_MASK(context_->GetDeviceId()));
    errCode = abilitySync_->SyncStart(context_->GetRequestSessionId(), context_->GetSequenceId(),
        remoteCommunicatorVersion, handler);
    if (errCode != E_OK) {
        LOGE("[StateMachine][DoAbilitySync] ability sync start failed,errCode=%d", errCode);
        return TransformErrCodeToEvent(errCode);
    }
    return Event::WAIT_ACK_EVENT;
}

Event SingleVerSyncStateMachine::GetEventAfterTimeSync(int mode) const
{
    if (mode == SyncModeType::SUBSCRIBE_QUERY || mode == SyncModeType::UNSUBSCRIBE_QUERY) {
        return Event::CONTROL_CMD_EVENT;
    }
    return Event::ABILITY_SYNC_FINISHED_EVENT;
}

Event SingleVerSyncStateMachine::DoSyncTaskFinished()
{
    StopWatchDog();
    dataSync_->ClearSyncStatus();
    RefObject::AutoLock lock(syncContext_);
    int errCode = ExecNextTask();
    if (errCode == E_OK) {
        return Event::START_SYNC_EVENT;
    }
    return TransformErrCodeToEvent(errCode);
}

Event SingleVerSyncStateMachine::DoTimeout()
{
    RefObject::AutoLock lock(context_);
    if (context_->GetMode() == SyncModeType::SUBSCRIBE_QUERY) {
        std::shared_ptr<SubscribeManager> subManager = context_->GetSubscribeManager();
        if (subManager != nullptr) {
            subManager->DeleteLocalSubscribeQuery(context_->GetDeviceId(), context_->GetQuery());
        }
    }
    context_->Abort(SyncOperation::OP_TIMEOUT);
    context_->Clear();
    AbortInner();
    return Event::ANY_EVENT;
}

Event SingleVerSyncStateMachine::DoInnerErr()
{
    RefObject::AutoLock lock(context_);
    if (!context_->IsCommNormal()) {
        if (context_->GetMode() == SyncModeType::SUBSCRIBE_QUERY) {
            std::shared_ptr<SubscribeManager> subManager = context_->GetSubscribeManager();
            if (subManager != nullptr) {
                subManager->DeleteLocalSubscribeQuery(context_->GetDeviceId(), context_->GetQuery());
            }
        }
        context_->Abort(SyncOperation::OP_COMM_ABNORMAL);
    } else {
        int status = GetSyncOperationStatus(context_->GetTaskErrCode());
        context_->Abort(status);
    }
    context_->Clear();
    AbortInner();
    return Event::ANY_EVENT;
}

int SingleVerSyncStateMachine::AbilitySyncRecv(const Message *inMsg)
{
    if (inMsg->GetMessageType() == TYPE_REQUEST) {
        return abilitySync_->RequestRecv(inMsg, context_);
    }

    if (inMsg->GetMessageType() == TYPE_RESPONSE && AbilityMsgSessionIdCheck(inMsg)) {
        int errCode = abilitySync_->AckRecv(inMsg, context_);
        std::lock_guard<std::mutex> lock(stateMachineLock_);
        (void)ResetWatchDog();
        if (errCode != E_OK) {
            LOGE("[StateMachine][AbilitySyncRecv] handle ackRecv failed,errCode=%d", errCode);
            SwitchStateAndStep(TransformErrCodeToEvent(errCode));
        } else if (context_->GetRemoteSoftwareVersion() <= SOFTWARE_VERSION_RELEASE_2_0) {
            abilitySync_->SetAbilitySyncFinishedStatus(true);
            LOGI("[StateMachine][AbilitySyncRecv] ability Sync Finished,label=%s,dev=%s",
                dataSync_->GetLabel().c_str(), STR_MASK(context_->GetDeviceId()));
            currentRemoteVersionId_ = context_->GetRemoteSoftwareVersionId();
            JumpStatusAfterAbilitySync(context_->GetMode());
        }
        return E_OK;
    }
    if (inMsg->GetMessageType() == TYPE_NOTIFY) {
            const AbilitySyncAckPacket *packet = inMsg->GetObject<AbilitySyncAckPacket>();
            if (packet == nullptr) {
                return -E_INVALID_ARGS;
            }
            int ackCode = packet->GetAckCode();
            if (ackCode != AbilitySync::CHECK_SUCCESS && ackCode != AbilitySync::LAST_NOTIFY) {
                LOGE("[StateMachine][AbilitySyncRecv] ackCode check failed,ackCode=%d", ackCode);
                std::lock_guard<std::mutex> lock(stateMachineLock_);
                SwitchStateAndStep(Event::INNER_ERR_EVENT);
                return E_OK;
            }
            if (ackCode == AbilitySync::LAST_NOTIFY && AbilityMsgSessionIdCheck(inMsg)) {
                abilitySync_->SetAbilitySyncFinishedStatus(true);
                // while recv last notify means ability sync finished,it is better to reset watchDog to avoid timeout.
                LOGI("[StateMachine][AbilitySyncRecv] ability sync finished,label=%s,dev=%s",
                    dataSync_->GetLabel().c_str(), STR_MASK(context_->GetDeviceId()));
                currentRemoteVersionId_ = context_->GetRemoteSoftwareVersionId();
                (static_cast<SingleVerSyncTaskContext *>(context_))->SetIsSchemaSync(true);
                std::lock_guard<std::mutex> lock(stateMachineLock_);
                (void)ResetWatchDog();
                JumpStatusAfterAbilitySync(context_->GetMode());
            } else if (ackCode != AbilitySync::LAST_NOTIFY) {
                abilitySync_->AckNotifyRecv(inMsg, context_);
            }
        return E_OK;
    }

    LOGE("[StateMachine][AbilitySyncRecv] msg type invalid");
    return -E_NOT_SUPPORT;
}

int SingleVerSyncStateMachine::HandleDataRequestRecv(const Message *inMsg)
{
    TimeOffset offset = 0;
    uint32_t timeout = communicator_->GetTimeout(context_->GetDeviceId());
    // If message is data sync request, we should check timeoffset.
    int errCode = timeSync_->GetTimeOffset(offset, timeout);
    if (errCode != E_OK) {
        LOGE("[StateMachine][HandleDataRequestRecv] GetTimeOffset err! errCode=%d", errCode);
        return errCode;
    }
    context_->SetTimeOffset(offset);
    PerformanceAnalysis *performance = PerformanceAnalysis::GetInstance();
    if (performance != nullptr) {
        performance->StepTimeRecordStart(PT_TEST_RECORDS::RECORD_DATA_REQUEST_RECV_TO_SEND_ACK);
    }
    DecRefCountOfFeedDogTimer(SyncDirectionFlag::RECEIVE);
    {
        std::lock_guard<std::mutex> lockWatchDog(stateMachineLock_);
        if (IsNeedResetWatchdog(inMsg)) {
            (void)ResetWatchDog();
        }
    }

    // RequestRecv will save data, it may cost a long time.
    // So we need to send save data notify to keep remote alive.
    bool isNeedStop = StartSaveDataNotify(inMsg->GetSessionId(), inMsg->GetSequenceId(), inMsg->GetMessageId());
    WaterMark pullEndWaterkark = 0;
    errCode = dataSync_->DataRequestRecv(context_, inMsg, pullEndWaterkark);
    if (performance != nullptr) {
        performance->StepTimeRecordEnd(PT_TEST_RECORDS::RECORD_DATA_REQUEST_RECV_TO_SEND_ACK);
    }
    if (isNeedStop) {
        StopSaveDataNotify();
    }
    // only higher than 102 version receive this errCode here.
    // while both RequestSessionId is not equal,but get this errCode;slwr would seem to handle first secquencid.
    // so while receive the same secquencid after abiitysync it wouldn't handle.
    if (errCode == -E_NEED_ABILITY_SYNC) {
        return errCode;
    }
    std::lock_guard<std::mutex> lock(stateMachineLock_);
    DataRecvErrCodeHandle(inMsg->GetSessionId(), errCode);
    if (pullEndWaterkark > 0) {
        AddPullResponseTarget(inMsg, pullEndWaterkark);
    }
    return E_OK;
}

void SingleVerSyncStateMachine::HandleDataAckRecvWithSlidingWindow(int errCode, const Message *inMsg,
    bool ignoreInnerErr)
{
    if (errCode == -E_RE_SEND_DATA) { // LOCAL_WATER_MARK_NOT_INIT
        dataSync_->ClearSyncStatus();
    }
    if (errCode == -E_NO_DATA_SEND || errCode == -E_SEND_DATA) {
        int ret = dataSync_->TryContinueSync(context_, inMsg);
        if (ret == -E_FINISHED) {
            SwitchStateAndStep(Event::SEND_FINISHED_EVENT);
            return;
        } else if (ret == E_OK) { // do nothing and waiting for all ack receive
            return;
        }
        errCode = ret;
    }
    ResponsePullError(errCode, ignoreInnerErr);
}

void SingleVerSyncStateMachine::NeedAbilitySyncHandle()
{
    // if the remote device version num is overdue,
    // mean the version num has been reset when syncing data,
    // there should not clear the new version cache again.
    if (currentRemoteVersionId_ == context_->GetRemoteSoftwareVersionId()) {
        LOGI("[StateMachine] set remote version 0, currentRemoteVersionId_ = %llu", currentRemoteVersionId_);
        context_->SetRemoteSoftwareVersion(0);
    } else {
        currentRemoteVersionId_ = context_->GetRemoteSoftwareVersionId();
    }
    abilitySync_->SetAbilitySyncFinishedStatus(false);
    dataSync_->ClearSyncStatus();
}

int SingleVerSyncStateMachine::HandleDataAckRecv(const Message *inMsg)
{
    if (inMsg->GetMessageType() == TYPE_RESPONSE) {
        DecRefCountOfFeedDogTimer(SyncDirectionFlag::SEND);
    }
    std::lock_guard<std::mutex> lock(stateMachineLock_);
    // Unfortunately we use stateMachineLock_ in many sync process
    // So a bad ack will check before the lock and wait
    // And then another process is running, it will get the lock.After this process, the ack became invalid.
    // If we don't check ack again, it will be delivered to dataSyncer.
    if (!IsPacketValid(inMsg)) {
        return -E_INVALID_ARGS;
    }
    if (IsNeedResetWatchdog(inMsg)) {
        (void)ResetWatchDog();
    }
    if (context_->GetRemoteSoftwareVersion() > SOFTWARE_VERSION_RELEASE_2_0 && !dataSync_->AckPacketIdCheck(inMsg)) {
        // packetId not match but sequence id matched scene, means resend map has be rebuilt
        // this is old ack, shoulb be dropped and wait for the same packetId sequence.
        return E_OK;
    }
    // AckRecv will save meta data, it may cost a long time. if another thread is saving data
    // So we need to send save data notify to keep remote alive.
    // eg. remote do pull sync
    bool isNeedStop = StartSaveDataNotify(inMsg->GetSessionId(), inMsg->GetSequenceId(), inMsg->GetMessageId());
    int errCode = dataSync_->AckRecv(context_, inMsg);
    if (isNeedStop) {
        StopSaveDataNotify();
    }
    if (errCode == -E_NEED_ABILITY_SYNC || errCode == -E_RE_SEND_DATA) {
        StopFeedDogForSync(SyncDirectionFlag::SEND);
    } else if (errCode == -E_SAVE_DATA_NOTIFY) {
        return errCode;
    }
    // when this msg is from response task while request task is running,  ignore the errCode
    bool ignoreInnerErr = inMsg->GetSessionId() == context_->GetResponseSessionId() &&
        context_->GetRequestSessionId() != 0;
    DataAckRecvErrCodeHandle(errCode, !ignoreInnerErr);
    HandleDataAckRecvWithSlidingWindow(errCode, inMsg, ignoreInnerErr);
    return errCode;
}

int SingleVerSyncStateMachine::DataPktRecv(Message *inMsg)
{
    PerformanceAnalysis *performance = PerformanceAnalysis::GetInstance();
    int errCode = E_OK;
    switch (inMsg->GetMessageType()) {
        case TYPE_REQUEST:
            ScheduleMsgAndHandle(inMsg);
            errCode = -E_NOT_NEED_DELETE_MSG;
            break;
        case TYPE_RESPONSE:
        case TYPE_NOTIFY:
            if (performance != nullptr) {
                performance->StepTimeRecordEnd(PT_TEST_RECORDS::RECORD_DATA_SEND_REQUEST_TO_ACK_RECV);
                performance->StepTimeRecordStart(PT_TEST_RECORDS::RECORD_ACK_RECV_TO_USER_CALL_BACK);
            }
            errCode = HandleDataAckRecv(inMsg);
            break;
        default:
            errCode = -E_INVALID_ARGS;
            break;
    }
    return errCode;
}

void SingleVerSyncStateMachine::ScheduleMsgAndHandle(Message *inMsg)
{
    dataSync_->PutDataMsg(inMsg);
    while (true) {
        bool isNeedHandle = true;
        bool isNeedContinue = true;
        Message *msg = dataSync_->MoveNextDataMsg(context_, isNeedHandle, isNeedContinue);
        if (!isNeedContinue) {
            break;
        }
        if (msg == nullptr) {
            if (dataSync_->IsNeedReloadQueue()) {
                continue;
            }
            break;
        }
        bool isNeedClearMap = false;
        if (isNeedHandle) {
            int errCode = HandleDataRequestRecv(msg);
            if (context_->IsReceiveWaterMarkErr() || errCode == -E_NEED_ABILITY_SYNC) {
                isNeedClearMap = true;
            }
            if (errCode == -E_TIMEOUT) {
                isNeedHandle = false;
            }
        } else {
            dataSync_->SendFinishedDataAck(context_, msg);
        }
        if (context_->GetRemoteSoftwareVersion() < SOFTWARE_VERSION_RELEASE_3_0) {
            // for lower version, no need to handle map schedule info, just reset schedule working status
            isNeedHandle = false;
        }
        dataSync_->ScheduleInfoHandle(isNeedHandle, isNeedClearMap, msg);
        delete msg;
    }
}

int SingleVerSyncStateMachine::ControlPktRecv(Message *inMsg)
{
    int errCode = E_OK;
    switch (inMsg->GetMessageType()) {
        case TYPE_REQUEST:
            errCode = dataSync_->ControlCmdRequestRecv(context_, inMsg);
            break;
        case TYPE_RESPONSE:
            errCode = HandleControlAckRecv(inMsg);
            break;
        default:
            errCode = -E_INVALID_ARGS;
            break;
    }
    return errCode;
}

void SingleVerSyncStateMachine::StepToTimeout(TimerId timerId)
{
    std::lock_guard<std::mutex> lock(stateMachineLock_);
    TimerId timer = syncContext_->GetTimerId();
    if (timer != timerId) {
        return;
    }
    SwitchStateAndStep(Event::TIME_OUT_EVENT);
}

int SingleVerSyncStateMachine::GetSyncOperationStatus(int errCode) const
{
    static const std::map<int, int> statusMap = {
        { -E_SCHEMA_MISMATCH, SyncOperation::OP_SCHEMA_INCOMPATIBLE },
        { -E_EKEYREVOKED, SyncOperation::OP_EKEYREVOKED_FAILURE },
        { -E_SECURITY_OPTION_CHECK_ERROR, SyncOperation::OP_SECURITY_OPTION_CHECK_FAILURE },
        { -E_BUSY, SyncOperation::OP_BUSY_FAILURE },
        { -E_NOT_PERMIT, SyncOperation::OP_PERMISSION_CHECK_FAILED },
        { -E_TIMEOUT, SyncOperation::OP_TIMEOUT },
        { -E_INVALID_QUERY_FORMAT, SyncOperation::OP_QUERY_FORMAT_FAILURE },
        { -E_INVALID_QUERY_FIELD, SyncOperation::OP_QUERY_FIELD_FAILURE },
        { -E_FEEDBACK_UNKNOWN_MESSAGE, SyncOperation::OP_NOT_SUPPORT },
        { -E_FEEDBACK_COMMUNICATOR_NOT_FOUND, SyncOperation::OP_COMM_ABNORMAL },
        { -E_NOT_SUPPORT, SyncOperation::OP_NOT_SUPPORT },
        { -E_INTERCEPT_DATA_FAIL, SyncOperation::OP_INTERCEPT_DATA_FAIL },
        { -E_MAX_LIMITS, SyncOperation::OP_MAX_LIMITS },
        { -E_DISTRIBUTED_SCHEMA_CHANGED, SyncOperation::OP_SCHEMA_CHANGED },
        { -E_NOT_REGISTER, SyncOperation::OP_NOT_SUPPORT },
    };
    auto iter = statusMap.find(errCode);
    if (iter != statusMap.end()) {
        return iter->second;
    }
    return SyncOperation::OP_FAILED;
}

int SingleVerSyncStateMachine::TimeMarkSyncRecv(const Message *inMsg)
{
    LOGD("[StateMachine][TimeMarkSyncRecv] type=%d,label=%s,dev=%s", inMsg->GetMessageType(),
        dataSync_->GetLabel().c_str(), STR_MASK(context_->GetDeviceId()));
    {
        std::lock_guard<std::mutex> lock(stateMachineLock_);
        (void)ResetWatchDog();
    }
    if (inMsg->GetMessageType() == TYPE_REQUEST) {
        return timeSync_->RequestRecv(inMsg);
    } else if (inMsg->GetMessageType() == TYPE_RESPONSE) {
        int errCode = timeSync_->AckRecv(inMsg, context_->GetRequestSessionId());
        if (errCode != E_OK) {
            LOGE("[StateMachine][TimeMarkSyncRecv] AckRecv failed errCode=%d", errCode);
            if (inMsg->GetSessionId() != 0 && inMsg->GetSessionId() == context_->GetRequestSessionId()) {
                context_->SetTaskErrCode(errCode);
                CommErrAbort();
            }
            return errCode;
        }
        std::lock_guard<std::mutex> lock(stateMachineLock_);
        SwitchStateAndStep(TIME_SYNC_FINISHED_EVENT);
        return E_OK;
    } else {
        return -E_INVALID_ARGS;
    }
}

void SingleVerSyncStateMachine::Clear()
{
    dataSync_ = nullptr;
    timeSync_ = nullptr;
    abilitySync_ = nullptr;
    context_ = nullptr;
    syncInterface_ = nullptr;
}

bool SingleVerSyncStateMachine::IsPacketValid(const Message *inMsg) const
{
    if (inMsg == nullptr) {
        return false;
    }

    if ((inMsg->GetMessageId() < TIME_SYNC_MESSAGE) || (inMsg->GetMessageId() >= UNKNOW_MESSAGE)) {
        LOGE("[StateMachine][IsPacketValid] Message is invalid, id=%d", inMsg->GetMessageId());
        return false;
    }
    // filter invalid ack at first
    bool isResponseType = (inMsg->GetMessageType() == TYPE_RESPONSE);
    if (isResponseType && (inMsg->GetMessageId() == CONTROL_SYNC_MESSAGE) &&
        (inMsg->GetSessionId() != context_->GetRequestSessionId())) {
        LOGE("[StateMachine][IsPacketValid] Control Message is invalid, label=%s, dev=%s",
            dataSync_->GetLabel().c_str(), STR_MASK(context_->GetDeviceId()));
        return false;
    }
    if (isResponseType && (inMsg->GetMessageId() != TIME_SYNC_MESSAGE) &&
        (inMsg->GetSessionId() != context_->GetRequestSessionId()) &&
        (inMsg->GetSessionId() != context_->GetResponseSessionId())) {
        LOGE("[StateMachine][IsPacketValid] Data Message is invalid, label=%s, dev=%s",
            dataSync_->GetLabel().c_str(), STR_MASK(context_->GetDeviceId()));
        return false;
    }

    // timeSync and abilitySync don't need to check sequenceId
    if (inMsg->GetMessageId() == TIME_SYNC_MESSAGE || inMsg->GetMessageId() == ABILITY_SYNC_MESSAGE ||
        inMsg->GetMessageId() == CONTROL_SYNC_MESSAGE) {
        return true;
    }
    // sequenceId will be checked in dataSync
    return true;
}

void SingleVerSyncStateMachine::PreStartPullResponse()
{
    SingleVerSyncTarget target;
    context_->PopResponseTarget(target);
    context_->SetEndMark(target.GetEndWaterMark());
    context_->SetResponseSessionId(target.GetResponseSessionId());
    context_->SetMode(SyncModeType::RESPONSE_PULL);
    context_->ReSetSequenceId();
    context_->SetQuerySync(target.IsQuerySync());
    context_->SetQuery(target.GetQuery());
}

bool SingleVerSyncStateMachine::CheckIsStartPullResponse() const
{
    // Other state will step to do pull response, only this statem we need to step the statemachine
    if (currentState_ == WAIT_FOR_RECEIVE_DATA_FINISH) {
        return true;
    }
    return false;
}

int SingleVerSyncStateMachine::MessageCallbackPre(const Message *inMsg)
{
    RefObject::AutoLock lock(context_);
    if (context_->IsKilled()) {
        return -E_OBJ_IS_KILLED;
    }

    if (!IsPacketValid(inMsg)) {
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

void SingleVerSyncStateMachine::AddPullResponseTarget(const Message *inMsg, WaterMark pullEndWatermark)
{
    int messageType = static_cast<int>(inMsg->GetMessageId());
    uint32_t sessionId = inMsg->GetSessionId();
    if (pullEndWatermark == 0) {
        LOGE("[StateMachine][AddPullResponseTarget] pullEndWatermark is 0!");
        return;
    }
    if (context_->GetResponseSessionId() == sessionId || context_->FindResponseSyncTarget(sessionId)) {
        LOGI("[StateMachine][AddPullResponseTarget] task is already running");
        return;
    }
    const DataRequestPacket *packet = inMsg->GetObject<DataRequestPacket>();
    if (packet == nullptr) {
        LOGE("[AddPullResponseTarget] get packet object failed");
        return;
    }
    SingleVerSyncTarget *targetTmp = new (std::nothrow) SingleVerSyncTarget;
    if (targetTmp == nullptr) {
        LOGE("[StateMachine][AddPullResponseTarget] add failed, may oom");
        return;
    }
    targetTmp->SetTaskType(ISyncTarget::RESPONSE);
    if (messageType == QUERY_SYNC_MESSAGE) {
        targetTmp->SetQuery(packet->GetQuery());
        targetTmp->SetQuerySync(true);
    }
    targetTmp->SetMode(SyncModeType::RESPONSE_PULL);
    targetTmp->SetEndWaterMark(pullEndWatermark);
    targetTmp->SetResponseSessionId(sessionId);
    if (context_->AddSyncTarget(targetTmp) != E_OK) {
        delete targetTmp;
        return;
    }
    if (CheckIsStartPullResponse()) {
        SwitchStateAndStep(TransformErrCodeToEvent(-E_NEED_PULL_REPONSE));
    }
}

Event SingleVerSyncStateMachine::TransformErrCodeToEvent(int errCode)
{
    switch (errCode) {
        case -E_TIMEOUT:
            return TransforTimeOutErrCodeToEvent();
        case -VERSION_NOT_SUPPOR_EVENT:
            return Event::VERSION_NOT_SUPPOR_EVENT;
        case -E_SEND_DATA:
            return Event::SEND_DATA_EVENT;
        case -E_NO_DATA_SEND:
            return Event::SEND_FINISHED_EVENT;
        case -E_RECV_FINISHED:
            return Event::RECV_FINISHED_EVENT;
        case -E_NEED_ABILITY_SYNC:
            return Event::NEED_ABILITY_SYNC_EVENT;
        case -E_NO_SYNC_TASK:
            return Event::ALL_TASK_FINISHED_EVENT;
        case -E_NEED_PULL_REPONSE:
            return Event::START_PULL_RESPONSE_EVENT;
        case -E_RE_SEND_DATA:
            return Event::RE_SEND_DATA_EVENT;
        default:
            return Event::INNER_ERR_EVENT;
    }
}

bool SingleVerSyncStateMachine::IsNeedResetWatchdog(const Message *inMsg) const
{
    if (inMsg == nullptr) {
        return false;
    }

    if (IsNeedErrCodeHandle(inMsg->GetSessionId())) {
        return true;
    }

    int msgType = inMsg->GetMessageType();
    if (msgType == TYPE_RESPONSE || msgType == TYPE_NOTIFY) {
        if (inMsg->GetSessionId() == context_->GetResponseSessionId()) {
            // Pull response ack also should reset watchdog
            return true;
        }
    }

    return false;
}

Event SingleVerSyncStateMachine::TransforTimeOutErrCodeToEvent()
{
    if (syncContext_->IsSyncTaskNeedRetry() && (syncContext_->GetRetryTime() < syncContext_->GetSyncRetryTimes())) {
        return Event::WAIT_TIME_OUT_EVENT;
    } else {
        return Event::TIME_OUT_EVENT;
    }
}

bool SingleVerSyncStateMachine::IsNeedErrCodeHandle(uint32_t sessionId) const
{
    // omit to set sessionId so version_3 should skip to compare sessionid.
    if (sessionId == context_->GetRequestSessionId() ||
        context_->GetRemoteSoftwareVersion() == SOFTWARE_VERSION_RELEASE_2_0) {
        return true;
    }
    return false;
}

void SingleVerSyncStateMachine::PushPullDataRequestEvokeErrHandle()
{
    // the pushpull sync task should wait for send finished after remote dev get data occur E_EKEYREVOKED error.
    if (context_->GetRemoteSoftwareVersion() > SOFTWARE_VERSION_RELEASE_2_0 &&
        SyncOperation::TransferSyncMode(context_->GetMode()) == SyncModeType::PUSH_AND_PULL) {
        LOGI("data request errCode = %d, wait for send finished", -E_EKEYREVOKED);
        context_->SetTaskErrCode(-E_EKEYREVOKED);
        context_->SetOperationStatus(SyncOperation::OP_RECV_FINISHED);
        SwitchStateAndStep(Event::RECV_FINISHED_EVENT);
    } else {
        context_->SetTaskErrCode(-E_EKEYREVOKED);
        SwitchStateAndStep(Event::INNER_ERR_EVENT);
    }
}

void SingleVerSyncStateMachine::DataRecvErrCodeHandle(uint32_t sessionId, int errCode)
{
    if (IsNeedErrCodeHandle(sessionId)) {
        switch (errCode) {
            case E_OK:
                break;
            case -E_RECV_FINISHED:
                context_->SetOperationStatus(SyncOperation::OP_RECV_FINISHED);
                SwitchStateAndStep(Event::RECV_FINISHED_EVENT);
                break;
            case -E_EKEYREVOKED:
                PushPullDataRequestEvokeErrHandle();
                break;
            case -E_BUSY:
            case -E_DISTRIBUTED_SCHEMA_CHANGED:
            case -E_DISTRIBUTED_SCHEMA_NOT_FOUND:
            case -E_FEEDBACK_COMMUNICATOR_NOT_FOUND:
            case -E_FEEDBACK_UNKNOWN_MESSAGE:
            case -E_INTERCEPT_DATA_FAIL:
            case -E_INVALID_QUERY_FIELD:
            case -E_INVALID_QUERY_FORMAT:
            case -E_MAX_LIMITS:
            case -E_NOT_REGISTER:
            case -E_NOT_SUPPORT:
            case -E_SECURITY_OPTION_CHECK_ERROR:
                context_->SetTaskErrCode(errCode);
                SwitchStateAndStep(Event::INNER_ERR_EVENT);
                break;
            default:
                SwitchStateAndStep(Event::INNER_ERR_EVENT);
                break;
        }
    }
}

bool SingleVerSyncStateMachine::AbilityMsgSessionIdCheck(const Message *inMsg)
{
    if (inMsg != nullptr && inMsg->GetSessionId() == context_->GetRequestSessionId()) {
        return true;
    }
    LOGE("[AbilitySync] session check failed,dev=%s", STR_MASK(context_->GetDeviceId()));
    return false;
}

SyncType SingleVerSyncStateMachine::GetSyncType(uint32_t messageId) const
{
    if (messageId == QUERY_SYNC_MESSAGE) {
        return SyncType::QUERY_SYNC_TYPE;
    }
    return SyncType::MANUAL_FULL_SYNC_TYPE;
}

void SingleVerSyncStateMachine::DataAckRecvErrCodeHandle(int errCode, bool handleError)
{
    switch (errCode) {
        case -E_NEED_ABILITY_SYNC:
            NeedAbilitySyncHandle();
            break;
        case -E_NOT_PERMIT:
            if (handleError) {
                context_->SetOperationStatus(SyncOperation::OP_PERMISSION_CHECK_FAILED);
            }
            break;
        case -E_BUSY:
        case -E_DISTRIBUTED_SCHEMA_CHANGED:
        case -E_DISTRIBUTED_SCHEMA_NOT_FOUND:
        case -E_EKEYREVOKED:
        case -E_FEEDBACK_COMMUNICATOR_NOT_FOUND:
        case -E_FEEDBACK_UNKNOWN_MESSAGE:
        case -E_INTERCEPT_DATA_FAIL:
        case -E_INVALID_QUERY_FIELD:
        case -E_INVALID_QUERY_FORMAT:
        case -E_MAX_LIMITS:
        case -E_NOT_REGISTER:
        case -E_NOT_SUPPORT:
        case -E_SECURITY_OPTION_CHECK_ERROR:
            if (handleError) {
                context_->SetTaskErrCode(errCode);
            }
            break;
        default:
            break;
    }
}

bool SingleVerSyncStateMachine::IsNeedTriggerQueryAutoSync(Message *inMsg, QuerySyncObject &query)
{
    return SingleVerDataSyncUtils::IsNeedTriggerQueryAutoSync(inMsg, query);
}

void SingleVerSyncStateMachine::JumpStatusAfterAbilitySync(int mode)
{
    if ((mode == SyncModeType::SUBSCRIBE_QUERY) || (mode == SyncModeType::UNSUBSCRIBE_QUERY)) {
        SwitchStateAndStep(CONTROL_CMD_EVENT);
    } else {
        SwitchStateAndStep(ABILITY_SYNC_FINISHED_EVENT);
    }
}

void SingleVerSyncStateMachine::ControlAckRecvErrCodeHandle(int errCode)
{
    switch (errCode) {
        case -E_NEED_ABILITY_SYNC:
            NeedAbilitySyncHandle();
            break;
        case -E_NO_DATA_SEND:
            context_->SetOperationStatus(SyncOperation::OP_SEND_FINISHED);
            break;
        case -E_NOT_PERMIT:
            context_->SetOperationStatus(SyncOperation::OP_PERMISSION_CHECK_FAILED);
            break;
        // other errCode use default
        default:
            context_->SetTaskErrCode(errCode);
            break;
    }
}

void SingleVerSyncStateMachine::GetLocalWaterMark(const DeviceID &deviceId, uint64_t &outValue)
{
    metadata_->GetLocalWaterMark(deviceId, outValue);
}

int SingleVerSyncStateMachine::GetSendQueryWaterMark(const std::string &queryId,  const DeviceID &deviceId,
    bool isAutoLift, uint64_t &outValue)
{
    return metadata_->GetSendQueryWaterMark(queryId, deviceId, outValue, isAutoLift);
}

void SingleVerSyncStateMachine::ResponsePullError(int errCode, bool ignoreInnerErr)
{
    Event event = TransformErrCodeToEvent(errCode);
    SwitchStateAndStep((ignoreInnerErr && event == INNER_ERR_EVENT) ? RESPONSE_TASK_FINISHED_EVENT : event);
}
} // namespace DistributedDB
