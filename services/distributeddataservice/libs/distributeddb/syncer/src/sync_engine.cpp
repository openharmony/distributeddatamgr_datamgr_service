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

#include "sync_engine.h"

#include <algorithm>
#include <deque>
#include <functional>

#include "ability_sync.h"
#include "db_common.h"
#include "db_errno.h"
#include "device_manager.h"
#include "hash.h"
#include "isync_state_machine.h"
#include "log_print.h"
#include "runtime_context.h"
#include "single_ver_serialize_manager.h"
#include "subscribe_manager.h"
#include "time_sync.h"

#ifndef OMIT_MULTI_VER
#include "commit_history_sync.h"
#include "multi_ver_data_sync.h"
#include "value_slice_sync.h"
#endif

namespace DistributedDB {
int SyncEngine::queueCacheSize_ = 0;
int SyncEngine::maxQueueCacheSize_ = DEFAULT_CACHE_SIZE;
unsigned int SyncEngine::discardMsgNum_ = 0;
std::mutex SyncEngine::queueLock_;

SyncEngine::SyncEngine()
    : syncInterface_(nullptr),
      communicator_(nullptr),
      deviceManager_(nullptr),
      metadata_(nullptr),
      timeChangedListener_(nullptr),
      execTaskCount_(0),
      isSyncRetry_(false),
      communicatorProxy_(nullptr),
      isActive_(false)
{
}

SyncEngine::~SyncEngine()
{
    LOGD("[SyncEngine] ~SyncEngine!");
    ClearInnerResource();
    equalIdentifierMap_.clear();
    subManager_ = nullptr;
    LOGD("[SyncEngine] ~SyncEngine ok!");
}

int SyncEngine::Initialize(ISyncInterface *syncInterface, std::shared_ptr<Metadata> &metadata,
    const std::function<void(std::string)> &onRemoteDataChanged, const std::function<void(std::string)> &offlineChanged,
    const std::function<void(const InternalSyncParma &param)> &queryAutoSyncCallback)
{
    if ((syncInterface == nullptr) || (metadata == nullptr)) {
        return -E_INVALID_ARGS;
    }
    int errCode = StartAutoSubscribeTimer();
    if (errCode != OK) {
        return errCode;
    }
    syncInterface_ = syncInterface;
    errCode = InitComunicator(syncInterface);
    if (errCode != E_OK) {
        LOGE("[SyncEngine] Init Communicator failed");
        // There need to set nullptr. other wise, syncInterface will be
        // DecRef in th destroy-method.
        StopAutoSubscribeTimer();
        syncInterface_ = nullptr;
        return errCode;
    }
    onRemoteDataChanged_ = onRemoteDataChanged;
    offlineChanged_ = offlineChanged;
    queryAutoSyncCallback_ = queryAutoSyncCallback;
    errCode = InitDeviceManager(onRemoteDataChanged, offlineChanged);
    if (errCode != E_OK) {
        // reset ptr if initialize device manager failed
        syncInterface_ = nullptr;
        StopAutoSubscribeTimer();
        return errCode;
    }
    if (subManager_ == nullptr) {
        subManager_ = std::make_shared<SubscribeManager>();
    }
    metadata_ = metadata;
    errCode = InitTimeChangedListener();
    if (errCode != E_OK) {
        syncInterface_ = nullptr;
        StopAutoSubscribeTimer();
        return errCode;
    }
    isActive_ = true;
    LOGI("[SyncEngine] Engine init ok");
    return E_OK;
}

int SyncEngine::Close()
{
    LOGI("[SyncEngine] SyncEngine[%s] close enter!", label_.c_str());
    isActive_ = false;
    UnRegCommunicatorsCallback();
    StopAutoSubscribeTimer();

    std::unique_lock<std::mutex> closeLock(execTaskCountLock_);
    execTaskCv_.wait(closeLock, [&]() {
        return execTaskCount_ == 0;
    });
    // Clear SyncContexts
    {
        std::unique_lock<std::mutex> lock(contextMapLock_);
        for (auto &iter : syncTaskContextMap_) {
            ISyncTaskContext *tempContext = iter.second;
            lock.unlock();
            RefObject::KillAndDecObjRef(tempContext);
            tempContext = nullptr;
            lock.lock();
            iter.second = nullptr;
        }
        syncTaskContextMap_.clear();
    }

    ReleaseCommunicators();
    std::lock_guard<std::mutex> msgLock(queueLock_);
    while (!msgQueue_.empty()) {
        Message *inMsg = msgQueue_.front();
        msgQueue_.pop_front();
        if (inMsg != nullptr) {
            queueCacheSize_ -= GetMsgSize(inMsg);
            delete inMsg;
            inMsg = nullptr;
        }
    }
    // close db, rekey or import scene, need clear all remote query info
    // local query info will destroy with syncEngine destruct
    if (subManager_ != nullptr) {
        subManager_->ClearAllRemoteQuery();
    }
    ClearInnerResource();
    LOGI("[SyncEngine] SyncEngine closed!");
    return E_OK;
}

int SyncEngine::AddSyncOperation(SyncOperation *operation)
{
    if (operation == nullptr) {
        LOGE("[SyncEngine] operation is nullptr");
        return -E_INVALID_ARGS;
    }

    std::vector<std::string> devices = operation->GetDevices();
    for (const auto &deviceId : devices) {
        if (deviceId.size() == 0) {
            operation->SetStatus(deviceId, SyncOperation::OP_INVALID_ARGS);
            continue;
        }
        operation->SetStatus(deviceId, SyncOperation::OP_WAITING);
        int errCode = AddSyncOperForContext(deviceId, operation);
        if (errCode != E_OK) {
            operation->SetStatus(deviceId, SyncOperation::OP_FAILED);
        }
    }
    return E_OK;
}

void SyncEngine::RemoveSyncOperation(int syncId)
{
    std::lock_guard<std::mutex> lock(contextMapLock_);
    for (auto &iter : syncTaskContextMap_) {
        ISyncTaskContext *context = iter.second;
        if (context != nullptr) {
            context->RemoveSyncOperation(syncId);
        }
    }
}

void SyncEngine::BroadCastDataChanged() const
{
    if (deviceManager_ != nullptr) {
        (void)deviceManager_->SendBroadCast(LOCAL_DATA_CHANGED);
    }
}

void SyncEngine::RegConnectCallback()
{
    if (communicator_ == nullptr) {
        LOGE("[SyncEngine][RegConnCB] communicator is not set!");
        return;
    }
    LOGD("[SyncEngine] RegOnConnectCallback");
    int errCode = communicator_->RegOnConnectCallback(
        std::bind(&DeviceManager::OnDeviceConnectCallback, deviceManager_,
            std::placeholders::_1, std::placeholders::_2), nullptr);
    if (errCode != E_OK) {
        LOGE("[SyncEngine][RegConnCB] register failed, auto sync can not use! err %d", errCode);
        return;
    }
    communicator_->Activate();
}

void SyncEngine::GetOnlineDevices(std::vector<std::string> &devices) const
{
    devices.clear();
    if (deviceManager_ != nullptr) {
        deviceManager_->GetOnlineDevices(devices);
    }
}

int SyncEngine::InitDeviceManager(const std::function<void(std::string)> &onRemoteDataChanged,
    const std::function<void(std::string)> &offlineChanged)
{
    deviceManager_ = new (std::nothrow) DeviceManager();
    if (deviceManager_ == nullptr) {
        LOGE("[SyncEngine] deviceManager alloc failed!");
        return -E_OUT_OF_MEMORY;
    }

    int errCode = deviceManager_->Initialize(communicatorProxy_, onRemoteDataChanged, offlineChanged);
    if (errCode != E_OK) {
        LOGE("[SyncEngine] deviceManager init failed! err %d", errCode);
        delete deviceManager_;
        deviceManager_ = nullptr;
        return errCode;
    }
    return E_OK;
}

int SyncEngine::InitComunicator(const ISyncInterface *syncInterface)
{
    ICommunicatorAggregator *communicatorAggregator = nullptr;
    int errCode = RuntimeContext::GetInstance()->GetCommunicatorAggregator(communicatorAggregator);
    if (communicatorAggregator == nullptr) {
        LOGE("[SyncEngine] Get ICommunicatorAggregator error when init the sync engine err = %d", errCode);
        return errCode;
    }
    std::vector<uint8_t> label = syncInterface->GetIdentifier();
    bool isSyncDualTupleMode = syncInterface->GetDbProperties().GetBoolProp(KvDBProperties::SYNC_DUAL_TUPLE_MODE,
        false);
    if (isSyncDualTupleMode) {
        std::vector<uint8_t> dualTuplelabel = syncInterface->GetDualTupleIdentifier();
        LOGI("[SyncEngine] dual tuple mode, original identifier=%.6s, target identifier=%.6s", VEC_TO_STR(label),
            VEC_TO_STR(dualTuplelabel));
        communicator_ = communicatorAggregator->AllocCommunicator(dualTuplelabel, errCode);
    } else {
        communicator_ = communicatorAggregator->AllocCommunicator(label, errCode);
    }
    if (communicator_ == nullptr) {
        LOGE("[SyncEngine] AllocCommunicator error when init the sync engine! err = %d", errCode);
        return errCode;
    }

    errCode = communicator_->RegOnMessageCallback(
        std::bind(&SyncEngine::MessageReciveCallback, this, std::placeholders::_1, std::placeholders::_2),
        []() {});
    if (errCode != E_OK) {
        LOGE("[SyncEngine] SyncRequestCallback register failed! err = %d", errCode);
        communicatorAggregator->ReleaseCommunicator(communicator_);
        communicator_ = nullptr;
        return errCode;
    }

    communicatorProxy_ = new (std::nothrow) CommunicatorProxy();
    if (communicatorProxy_ == nullptr) {
        communicatorAggregator->ReleaseCommunicator(communicator_);
        communicator_ = nullptr;
        return -E_OUT_OF_MEMORY;
    }

    communicatorProxy_->SetMainCommunicator(communicator_);
    label.resize(3); // only show 3 Bytes enough
    label_ = DBCommon::VectorToHexString(label);
    LOGD("[SyncEngine] RegOnConnectCallback");
    return errCode;
}

int SyncEngine::AddSyncOperForContext(const std::string &deviceId, SyncOperation *operation)
{
    int errCode = E_OK;
    ISyncTaskContext *context = nullptr;
    {
        std::lock_guard<std::mutex> lock(contextMapLock_);
        context = FindSyncTaskContext(deviceId);
        if (context == nullptr) {
            if (!IsKilled()) {
                context = GetSyncTaskContext(deviceId, errCode);
            }
            if (context == nullptr) {
                return errCode;
            }
        }
        if (context->IsKilled()) {
            return -E_OBJ_IS_KILLED;
        }
        // IncRef for SyncEngine to make sure context is valid, to avoid a big lock
        RefObject::IncObjRef(context);
    }

    errCode = context->AddSyncOperation(operation);
    RefObject::DecObjRef(context);
    return errCode;
}

void SyncEngine::MessageReciveCallbackTask(ISyncTaskContext *context, const ICommunicator *communicator,
    Message *inMsg)
{
    std::string deviceId = context->GetDeviceId();

    if (inMsg->GetMessageId() != LOCAL_DATA_CHANGED) {
        int errCode = context->ReceiveMessageCallback(inMsg);
        if (errCode == -E_NOT_NEED_DELETE_MSG) {
            goto MSG_CALLBACK_OUT_NOT_DEL;
        }
        // add auto sync here while recv subscribe request
        QuerySyncObject syncObject;
        if (errCode == E_OK && context->IsNeedTriggerQueryAutoSync(inMsg, syncObject)) {
            InternalSyncParma param;
            GetQueryAutoSyncParam(deviceId, syncObject, param);
            queryAutoSyncCallback_(param);
        }
    }

    delete inMsg;
    inMsg = nullptr;
MSG_CALLBACK_OUT_NOT_DEL:
    ScheduleTaskOut(context, communicator);
}

void SyncEngine::RemoteDataChangedTask(ISyncTaskContext *context, const ICommunicator *communicator, Message *inMsg)
{
    do {
        std::string deviceId = context->GetDeviceId();
        if (onRemoteDataChanged_ && deviceManager_->IsDeviceOnline(deviceId)) {
            onRemoteDataChanged_(deviceId);
        } else {
            LOGE("[SyncEngine] onRemoteDataChanged is null!");
        }
    } while (false);
    delete inMsg;
    inMsg = nullptr;
    ScheduleTaskOut(context, communicator);
}

void SyncEngine::ScheduleTaskOut(ISyncTaskContext *context, const ICommunicator *communicator)
{
    (void)DealMsgUtilQueueEmpty();
    DecExecTaskCount();
    RefObject::DecObjRef(communicator);
    RefObject::DecObjRef(context);
}

int SyncEngine::DealMsgUtilQueueEmpty()
{
    int errCode = E_OK;
    Message *inMsg = nullptr;
    {
        std::lock_guard<std::mutex> lock(queueLock_);
        if (msgQueue_.empty()) {
            return errCode;
        }
        inMsg = msgQueue_.front();
        msgQueue_.pop_front();
        queueCacheSize_ -= GetMsgSize(inMsg);
    }

    IncExecTaskCount();
    // it will deal with the first message in queue, we should increase object reference counts and sure that resources
    // could be prevented from destroying by other threads.
    do {
        ISyncTaskContext *nextContext = GetConextForMsg(inMsg->GetTarget(), errCode);
        if (errCode != E_OK) {
            break;
        }
        errCode = ScheduleDealMsg(nextContext, inMsg);
        if (errCode != E_OK) {
            RefObject::DecObjRef(nextContext);
        }
    } while (false);
    if (errCode != E_OK) {
        delete inMsg;
        inMsg = nullptr;
        DecExecTaskCount();
    }
    return errCode;
}

ISyncTaskContext *SyncEngine::GetConextForMsg(const std::string &targetDev, int &errCode)
{
    ISyncTaskContext *context = nullptr;
    {
        std::lock_guard<std::mutex> lock(contextMapLock_);
        context = FindSyncTaskContext(targetDev);
        if (context != nullptr) {
            if (context->IsKilled()) {
                errCode = -E_OBJ_IS_KILLED;
                return nullptr;
            }
        } else {
            if (IsKilled()) {
                errCode = -E_OBJ_IS_KILLED;
                return nullptr;
            }
            context = GetSyncTaskContext(targetDev, errCode);
            if (context == nullptr) {
                return nullptr;
            }
        }
        // IncRef for context to make sure context is valid, when task run another thread
        RefObject::IncObjRef(context);
    }
    return context;
}

int SyncEngine::ScheduleDealMsg(ISyncTaskContext *context, Message *inMsg)
{
    if (inMsg == nullptr) {
        LOGE("[SyncEngine] MessageReciveCallback inMsg is null!");
        DecExecTaskCount();
        return E_OK;
    }
    RefObject::IncObjRef(communicatorProxy_);
    int errCode = E_OK;
    // deal remote local data changed message
    if (inMsg->GetMessageId() == LOCAL_DATA_CHANGED) {
        RemoteDataChangedTask(context, communicatorProxy_, inMsg);
    } else {
        errCode = RuntimeContext::GetInstance()->ScheduleTask(std::bind(&SyncEngine::MessageReciveCallbackTask,
            this, context, communicatorProxy_, inMsg));
    }

    if (errCode != E_OK) {
        LOGE("[SyncEngine] MessageReciveCallbackTask Schedule failed err %d", errCode);
        RefObject::DecObjRef(communicatorProxy_);
    }
    return errCode;
}

void SyncEngine::MessageReciveCallback(const std::string &targetDev, Message *inMsg)
{
    IncExecTaskCount();
    int errCode = MessageReciveCallbackInner(targetDev, inMsg);
    if (errCode != E_OK) {
        delete inMsg;
        inMsg = nullptr;
        DecExecTaskCount();
        LOGE("[SyncEngine] MessageReciveCallback failed!");
    }
}

int SyncEngine::MessageReciveCallbackInner(const std::string &targetDev, Message *inMsg)
{
    if (targetDev.empty() || inMsg == nullptr) {
        LOGE("[SyncEngine][MessageReciveCallback] from a invalid device or inMsg is null ");
        return -E_INVALID_ARGS;
    }
    if (!isActive_) {
        LOGE("[SyncEngine] engine is closing, ignore msg");
        return -E_BUSY;
    }
    int msgSize = 0;
    if (!IsSkipCalculateLen(inMsg)) {
        msgSize = GetMsgSize(inMsg);
        if (msgSize <= 0) {
            LOGE("[SyncEngine] GetMsgSize makes a mistake");
            return -E_NOT_SUPPORT;
        }
    }

    {
        std::lock_guard<std::mutex> lock(queueLock_);
        if ((queueCacheSize_ + msgSize) > maxQueueCacheSize_) {
            LOGE("[SyncEngine] The size of message queue is beyond maximum");
            discardMsgNum_++;
            return -E_BUSY;
        }

        if (execTaskCount_ > MAX_EXEC_NUM) {
            PutMsgIntoQueue(targetDev, inMsg, msgSize);
            // task dont exec here
            DecExecTaskCount();
            return E_OK;
        }
    }

    int errCode = E_OK;
    ISyncTaskContext *nextContext = GetConextForMsg(targetDev, errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    LOGD("[SyncEngine] MessageReciveCallback MSG ID = %d", inMsg->GetMessageId());
    return ScheduleDealMsg(nextContext, inMsg);
}

void SyncEngine::PutMsgIntoQueue(const std::string &targetDev, Message *inMsg, int msgSize)
{
    if (inMsg->GetMessageId() == LOCAL_DATA_CHANGED) {
        auto iter = std::find_if(msgQueue_.begin(), msgQueue_.end(),
            [&targetDev](const Message *msg) {
                return targetDev == msg->GetTarget() && msg->GetMessageId() == LOCAL_DATA_CHANGED;
            });
        if (iter != msgQueue_.end()) {
            delete inMsg;
            inMsg = nullptr;
            return;
        }
    }
    inMsg->SetTarget(targetDev);
    msgQueue_.push_back(inMsg);
    queueCacheSize_ += msgSize;
    LOGE("[SyncEngine] The quantity of executing threads is beyond maximum. msgQueueSize = %zu", msgQueue_.size());
}

int SyncEngine::GetMsgSize(const Message *inMsg) const
{
    switch (inMsg->GetMessageId()) {
        case TIME_SYNC_MESSAGE:
            return TimeSync::CalculateLen(inMsg);
        case ABILITY_SYNC_MESSAGE:
            return AbilitySync::CalculateLen(inMsg);
        case DATA_SYNC_MESSAGE:
        case QUERY_SYNC_MESSAGE:
        case CONTROL_SYNC_MESSAGE:
            return SingleVerSerializeManager::CalculateLen(inMsg);
#ifndef OMIT_MULTI_VER
        case COMMIT_HISTORY_SYNC_MESSAGE:
            return CommitHistorySync::CalculateLen(inMsg);
        case MULTI_VER_DATA_SYNC_MESSAGE:
            return MultiVerDataSync::CalculateLen(inMsg);
        case VALUE_SLICE_SYNC_MESSAGE:
            return ValueSliceSync::CalculateLen(inMsg);
#endif
        case LOCAL_DATA_CHANGED:
            return DeviceManager::CalculateLen();
        default:
            LOGE("[SyncEngine] GetMsgSize not support msgId:%u", inMsg->GetMessageId());
            return -E_NOT_SUPPORT;
    }
}

ISyncTaskContext *SyncEngine::FindSyncTaskContext(const std::string &deviceId)
{
    auto iter = syncTaskContextMap_.find(deviceId);
    if (iter != syncTaskContextMap_.end()) {
        ISyncTaskContext *context = iter->second;
        return context;
    }
    return nullptr;
}

ISyncTaskContext *SyncEngine::GetSyncTaskContextAndInc(const std::string &deviceId)
{
    ISyncTaskContext *context = nullptr;
    std::lock_guard<std::mutex> lock(contextMapLock_);
    context = FindSyncTaskContext(deviceId);
    if (context == nullptr) {
        LOGI("[SyncEngine] dev=%s, context is null, no need to clear sync operation", STR_MASK(deviceId));
        return nullptr;
    }
    if (context->IsKilled()) {
        LOGI("[SyncEngine] context is killing");
        return nullptr;
    }
    RefObject::IncObjRef(context);
    return context;
}

ISyncTaskContext *SyncEngine::GetSyncTaskContext(const std::string &deviceId, int &errCode)
{
    ISyncTaskContext *context = CreateSyncTaskContext();
    if (context == nullptr) {
        errCode = -E_OUT_OF_MEMORY;
        LOGE("[SyncEngine] SyncTaskContext alloc failed, may be no memory available!");
        return nullptr;
    }
    errCode = context->Initialize(deviceId, syncInterface_, metadata_, communicatorProxy_);
    if (errCode != E_OK) {
        LOGE("[SyncEngine] context init failed err %d, dev %s", errCode, STR_MASK(deviceId));
        RefObject::DecObjRef(context);
        context = nullptr;
        return nullptr;
    }
    syncTaskContextMap_.insert(std::pair<std::string, ISyncTaskContext *>(deviceId, context));
    // IncRef for SyncEngine to make sure SyncEngine is valid when context access
    RefObject::IncObjRef(this);
    context->OnLastRef([this, deviceId]() {
        LOGD("[SyncEngine] SyncTaskContext for id %s finalized", STR_MASK(deviceId));
        RefObject::DecObjRef(this);
    });
    context->RegOnSyncTask(std::bind(&SyncEngine::ExecSyncTask, this, context));
    return context;
}

int SyncEngine::ExecSyncTask(ISyncTaskContext *context)
{
    if (IsKilled()) {
        return -E_OBJ_IS_KILLED;
    }

    AutoLock lockGuard(context);
    int status = context->GetTaskExecStatus();
    if ((status == SyncTaskContext::RUNNING) || context->IsKilled()) {
        return -E_NOT_SUPPORT;
    }
    context->SetTaskExecStatus(ISyncTaskContext::RUNNING);
    if (!context->IsTargetQueueEmpty()) {
        context->MoveToNextTarget();
        int checkErrCode = E_OK;
        // rdb don't support PermissionCheck
        if (syncInterface_->GetInterfaceType() != ISyncInterface::SYNC_RELATION) {
            checkErrCode = RunPermissionCheck(context->GetDeviceId(),
                GetPermissionCheckFlag(context->IsAutoSync(), context->GetMode()));
        }
        if (checkErrCode != E_OK) {
            context->SetOperationStatus(SyncOperation::OP_PERMISSION_CHECK_FAILED);
            context->SetTaskExecStatus(ISyncTaskContext::FINISHED);
            return checkErrCode;
        }
        context->UnlockObj();
        int errCode = context->StartStateMachine();
        context->LockObj();
        if (errCode != E_OK) {
            LOGE("[SyncEngine] machine StartSync failed");
            context->SetOperationStatus(SyncOperation::OP_FAILED);
            return errCode;
        }
    } else {
        LOGD("[SyncEngine] ExecSyncTask finished");
        context->SetTaskExecStatus(ISyncTaskContext::FINISHED);
    }
    return E_OK;
}

int SyncEngine::GetQueueCacheSize() const
{
    return queueCacheSize_;
}

unsigned int SyncEngine::GetDiscardMsgNum() const
{
    return discardMsgNum_;
}

unsigned int SyncEngine::GetMaxExecNum() const
{
    return MAX_EXEC_NUM;
}

void SyncEngine::SetMaxQueueCacheSize(int value)
{
    maxQueueCacheSize_ = value;
}

uint8_t SyncEngine::GetPermissionCheckFlag(bool isAutoSync, int syncMode)
{
    uint8_t flag = 0;
    int mode = SyncOperation::TransferSyncMode(syncMode);
    if (mode == SyncModeType::PUSH || mode == SyncModeType::RESPONSE_PULL) {
        flag = CHECK_FLAG_SEND;
    } else if (mode == SyncModeType::PULL) {
        flag = CHECK_FLAG_RECEIVE;
    } else if (mode == SyncModeType::PUSH_AND_PULL) {
        flag = CHECK_FLAG_SEND | CHECK_FLAG_RECEIVE;
    }
    if (isAutoSync) {
        flag = flag | CHECK_FLAG_AUTOSYNC;
    }
    if (mode != SyncModeType::RESPONSE_PULL) {
        // it means this sync is started by local
        flag = flag | CHECK_FLAG_SPONSOR;
    }
    return flag;
}

int SyncEngine::RunPermissionCheck(const std::string &deviceId, uint8_t flag) const
{
    std::string appId = syncInterface_->GetDbProperties().GetStringProp(KvDBProperties::APP_ID, "");
    std::string userId = syncInterface_->GetDbProperties().GetStringProp(KvDBProperties::USER_ID, "");
    std::string storeId = syncInterface_->GetDbProperties().GetStringProp(KvDBProperties::STORE_ID, "");
    int errCode = RuntimeContext::GetInstance()->RunPermissionCheck(userId, appId, storeId, deviceId, flag);
    if (errCode != E_OK) {
        LOGE("[SyncEngine] RunPermissionCheck not pass errCode:%d, flag:%d, %s Label=%s",
            errCode, flag, STR_MASK(deviceId), label_.c_str());
    }
    return errCode;
}

std::string SyncEngine::GetLabel() const
{
    return label_;
}

bool SyncEngine::GetSyncRetry() const
{
    return isSyncRetry_;
}

void SyncEngine::SetSyncRetry(bool isRetry)
{
    if (isSyncRetry_ == isRetry) {
        LOGI("sync retry is equal, syncTry=%d, no need to set.", isRetry);
        return;
    }
    isSyncRetry_ = isRetry;
    LOGI("[SyncEngine] SetSyncRetry:%d ok", isRetry);
    std::lock_guard<std::mutex> lock(contextMapLock_);
    for (auto &iter : syncTaskContextMap_) {
        ISyncTaskContext *context = iter.second;
        if (context != nullptr) {
            context->SetSyncRetry(isRetry);
        }
    }
}

int SyncEngine::SetEqualIdentifier(const std::string &identifier, const std::vector<std::string> &targets)
{
    if (!isActive_) {
        LOGI("[SyncEngine] engine is closed, just put into map");
        return E_OK;
    }
    ICommunicator *communicator = nullptr;
    {
        std::lock_guard<std::mutex> lock(equalCommunicatorsLock_);
        if (equalCommunicators_.count(identifier) != 0) {
            communicator = equalCommunicators_[identifier];
        } else {
            int errCode = E_OK;
            communicator = AllocCommunicator(identifier, errCode);
            if (communicator == nullptr) {
                return errCode;
            }
            equalCommunicators_[identifier] = communicator;
        }
    }
    std::string targetDevices;
    for (const auto &dev : targets) {
        targetDevices += DBCommon::StringMasking(dev) + ",";
    }
    LOGI("[SyncEngine] set equal identifier=%s, original=%s, targetDevices=%s",
        DBCommon::TransferStringToHex(identifier).c_str(), label_.c_str(),
        targetDevices.substr(0, targetDevices.size() - 1).c_str());
    communicatorProxy_->SetEqualCommunicator(communicator, identifier, targets);
    communicator->Activate();
    return E_OK;
}

void SyncEngine::SetEqualIdentifier()
{
    std::map<std::string, std::vector<std::string>> equalIdentifier; // key: equalIdentifier value: devices
    for (auto &item : equalIdentifierMap_) {
        if (equalIdentifier.find(item.second) == equalIdentifier.end()) {
            equalIdentifier[item.second] = {item.first};
        } else {
            equalIdentifier[item.second].push_back(item.first);
        }
    }
    for (auto &item : equalIdentifier) {
        SetEqualIdentifier(item.first, item.second);
    }
}

void SyncEngine::SetEqualIdentifierMap(const std::string &identifier, const std::vector<std::string> &targets)
{
    for (auto iter = equalIdentifierMap_.begin(); iter != equalIdentifierMap_.end();) {
        if (identifier == iter->second) {
            iter = equalIdentifierMap_.erase(iter);
            continue;
        }
        iter++;
    }
    for (auto &device : targets) {
        equalIdentifierMap_[device] = identifier;
    }
}

void SyncEngine::OfflineHandleByDevice(const std::string &deviceId)
{
    if (communicatorProxy_ == nullptr) {
        return;
    }
    // db closed or device is offline
    // clear remote subscribe and trigger
    std::vector<std::string> remoteQueryId;
    subManager_->GetRemoteSubscribeQueryIds(deviceId, remoteQueryId);
    subManager_->ClearRemoteSubscribeQuery(deviceId);
    static_cast<SingleVerKvDBSyncInterface *>(syncInterface_)->RemoveSubscribe(remoteQueryId);
    // get context and Inc context if context is not nullprt
    ISyncTaskContext *context = GetSyncTaskContextAndInc(deviceId);
    if (context != nullptr) {
        context->SetIsNeedResetAbilitySync(true);
    }
    if (communicatorProxy_->IsDeviceOnline(deviceId)) {
        LOGI("[SyncEngine] target dev=%s is online, no need to clear task.", STR_MASK(deviceId));
        RefObject::DecObjRef(context);
        return;
    }
    // means device is offline, clear local subscribe
    subManager_->ClearLocalSubscribeQuery(deviceId);
    // clear sync task
    if (context != nullptr) {
        context->ClearAllSyncTask();
        RefObject::DecObjRef(context);
    }
}

void SyncEngine::GetLocalSubscribeQueries(const std::string &device, std::vector<QuerySyncObject> &subscribeQueries)
{
    subManager_->GetLocalSubscribeQueries(device, subscribeQueries);
}

void SyncEngine::GetRemoteSubscribeQueryIds(const std::string &device, std::vector<std::string> &subscribeQueryIds)
{
    subManager_->GetRemoteSubscribeQueryIds(device, subscribeQueryIds);
}

void SyncEngine::GetRemoteSubscribeQueries(const std::string &device, std::vector<QuerySyncObject> &subscribeQueries)
{
    subManager_->GetRemoteSubscribeQueries(device, subscribeQueries);
}

void SyncEngine::PutUnfiniedSubQueries(const std::string &device, std::vector<QuerySyncObject> &subscribeQueries)
{
    subManager_->PutLocalUnFiniedSubQueries(device, subscribeQueries);
}

void SyncEngine::GetAllUnFinishSubQueries(std::map<std::string, std::vector<QuerySyncObject>> &allSyncQueries)
{
    subManager_->GetAllUnFinishSubQueries(allSyncQueries);
}

ICommunicator *SyncEngine::AllocCommunicator(const std::string &identifier, int &errCode)
{
    ICommunicatorAggregator *communicatorAggregator = nullptr;
    errCode = RuntimeContext::GetInstance()->GetCommunicatorAggregator(communicatorAggregator);
    if (communicatorAggregator == nullptr) {
        LOGE("[SyncEngine] Get ICommunicatorAggregator error when SetEqualIdentifier err = %d", errCode);
        return nullptr;
    }
    std::vector<uint8_t> identifierVect(identifier.begin(), identifier.end());
    auto communicator = communicatorAggregator->AllocCommunicator(identifierVect, errCode);
    if (communicator == nullptr) {
        LOGE("[SyncEngine] AllocCommunicator error when SetEqualIdentifier! err = %d", errCode);
        return communicator;
    }

    errCode = communicator->RegOnMessageCallback(
        std::bind(&SyncEngine::MessageReciveCallback, this, std::placeholders::_1, std::placeholders::_2),
        []() {});
    if (errCode != E_OK) {
        LOGE("[SyncEngine] SyncRequestCallback register failed in SetEqualIdentifier! err = %d", errCode);
        communicatorAggregator->ReleaseCommunicator(communicator);
        return nullptr;
    }

    errCode = communicator->RegOnConnectCallback(
        std::bind(&DeviceManager::OnDeviceConnectCallback, deviceManager_,
            std::placeholders::_1, std::placeholders::_2), nullptr);
    if (errCode != E_OK) {
        LOGE("[SyncEngine][RegConnCB] register failed in SetEqualIdentifier! err %d", errCode);
        communicator->RegOnMessageCallback(nullptr, nullptr);
        communicatorAggregator->ReleaseCommunicator(communicator);
        return nullptr;
    }

    return communicator;
}

void SyncEngine::UnRegCommunicatorsCallback()
{
    if (communicator_ != nullptr) {
        communicator_->RegOnMessageCallback(nullptr, nullptr);
        communicator_->RegOnConnectCallback(nullptr, nullptr);
        communicator_->RegOnSendableCallback(nullptr, nullptr);
    }
    std::lock_guard<std::mutex> lock(equalCommunicatorsLock_);
    for (const auto &iter : equalCommunicators_) {
        iter.second->RegOnMessageCallback(nullptr, nullptr);
        iter.second->RegOnConnectCallback(nullptr, nullptr);
        iter.second->RegOnSendableCallback(nullptr, nullptr);
    }
}

void SyncEngine::ReleaseCommunicators()
{
    RefObject::KillAndDecObjRef(communicatorProxy_);
    communicatorProxy_ = nullptr;
    ICommunicatorAggregator *communicatorAggregator = nullptr;
    int errCode = RuntimeContext::GetInstance()->GetCommunicatorAggregator(communicatorAggregator);
    if (communicatorAggregator == nullptr) {
        LOGF("[SyncEngine] ICommunicatorAggregator get failed when fialize SyncEngine err %d", errCode);
        return;
    }

    if (communicator_ != nullptr) {
        communicatorAggregator->ReleaseCommunicator(communicator_);
        communicator_ = nullptr;
    }

    std::lock_guard<std::mutex> lock(equalCommunicatorsLock_);
    for (auto &iter : equalCommunicators_) {
        communicatorAggregator->ReleaseCommunicator(iter.second);
    }
    equalCommunicators_.clear();
}

bool SyncEngine::IsSkipCalculateLen(const Message *inMsg)
{
    if (inMsg->IsFeedbackError()) {
        LOGE("[SyncEngine] Feedback Message with errorNo=%u.", inMsg->GetErrorNo());
        return true;
    }
    return false;
}

void SyncEngine::GetSubscribeSyncParam(const std::string &device, const QuerySyncObject &query,
    InternalSyncParma &outParam)
{
    outParam.devices = { device };
    outParam.mode = SyncModeType::AUTO_SUBSCRIBE_QUERY;
    outParam.isQuerySync = true;
    outParam.syncQuery = query;
}

void SyncEngine::GetQueryAutoSyncParam(const std::string &device, const QuerySyncObject &query,
    InternalSyncParma &outParam)
{
    outParam.devices = { device };
    outParam.mode = SyncModeType::AUTO_PUSH;
    outParam.isQuerySync = true;
    outParam.syncQuery = query;
}

int SyncEngine::StartAutoSubscribeTimer()
{
    return E_OK;
}

void SyncEngine::StopAutoSubscribeTimer()
{
}

int SyncEngine::InitTimeChangedListener()
{
    int errCode = E_OK;
    timeChangedListener_ = RuntimeContext::GetInstance()->RegisterTimeChangedLister(
        [this](void *changedOffset) {
            if (changedOffset == nullptr) {
                return;
            }
            TimeOffset changedTimeOffset = *(reinterpret_cast<TimeOffset *>(changedOffset)) *
                static_cast<TimeOffset>(TimeHelper::TO_100_NS);
            TimeOffset orgOffset = this->metadata_->GetLocalTimeOffset() - changedTimeOffset;
            Timestamp currentSysTime = TimeHelper::GetSysCurrentTime();
            Timestamp maxItemTime = 0;
            this->syncInterface_->GetMaxTimestamp(maxItemTime);
            if ((currentSysTime + static_cast<Timestamp>(orgOffset)) <= maxItemTime) {
                orgOffset = static_cast<TimeOffset>(maxItemTime - currentSysTime + TimeHelper::MS_TO_100_NS); // 1ms
            }
            this->metadata_->SaveLocalTimeOffset(orgOffset);
        }, errCode);
    if (timeChangedListener_ == nullptr) {
        LOGE("[SyncEngine] Init RegisterTimeChangedLister failed");
        return errCode;
    }
    return E_OK;
}

int SyncEngine::SubscribeLimitCheck(const std::vector<std::string> &devices, QuerySyncObject &query) const
{
    return subManager_->LocalSubscribeLimitCheck(devices, query);
}


void SyncEngine::ClearInnerResource()
{
    if (timeChangedListener_ != nullptr) {
        timeChangedListener_->Drop(true);
        timeChangedListener_ = nullptr;
    }
    if (syncInterface_ != nullptr) {
        syncInterface_->DecRefCount();
        syncInterface_ = nullptr;
    }
    if (deviceManager_ != nullptr) {
        delete deviceManager_;
        deviceManager_ = nullptr;
    }
    communicator_ = nullptr;
    metadata_ = nullptr;
    onRemoteDataChanged_ = nullptr;
    offlineChanged_ = nullptr;
    queryAutoSyncCallback_ = nullptr;
}

bool SyncEngine::IsEngineActive() const
{
    return isActive_;
}

void SyncEngine::SchemaChange()
{
    std::lock_guard<std::mutex> lock(contextMapLock_);
    for (auto &entry : syncTaskContextMap_) {
        auto context = entry.second;
        if (context->IsKilled()) {
            continue;
        }
        // IncRef for SyncEngine to make sure context is valid, to avoid a big lock
        context->SchemaChange();
    }
}

void SyncEngine::IncExecTaskCount()
{
    std::lock_guard<std::mutex> incLock(execTaskCountLock_);
    execTaskCount_++;
}

void SyncEngine::DecExecTaskCount()
{
    {
        std::lock_guard<std::mutex> decLock(execTaskCountLock_);
        execTaskCount_--;
    }
    execTaskCv_.notify_all();
}
} // namespace DistributedDB
