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

#include "generic_syncer.h"

#include "db_common.h"
#include "db_errno.h"
#include "log_print.h"
#include "ref_object.h"
#include "sqlite_single_ver_natural_store.h"
#include "time_sync.h"
#include "single_ver_data_sync.h"
#ifndef OMIT_MULTI_VER
#include "commit_history_sync.h"
#include "multi_ver_data_sync.h"
#include "value_slice_sync.h"
#endif
#include "device_manager.h"
#include "db_constant.h"
#include "ability_sync.h"
#include "single_ver_serialize_manager.h"

namespace DistributedDB {
const int GenericSyncer::MIN_VALID_SYNC_ID = 1;
std::mutex GenericSyncer::moduleInitLock_;
int GenericSyncer::currentSyncId_ = 0;
std::mutex GenericSyncer::syncIdLock_;
GenericSyncer::GenericSyncer()
    : syncEngine_(nullptr),
      syncInterface_(nullptr),
      timeHelper_(nullptr),
      metadata_(nullptr),
      initialized_(false),
      queuedManualSyncSize_(0),
      queuedManualSyncLimit_(DBConstant::QUEUED_SYNC_LIMIT_DEFAULT),
      manualSyncEnable_(true),
      closing_(false),
      engineFinalize_(false)
{
}

GenericSyncer::~GenericSyncer()
{
    LOGD("[GenericSyncer] ~GenericSyncer!");
    if (syncEngine_ != nullptr) {
        syncEngine_->OnKill([this]() { this->syncEngine_->Close(); });
        RefObject::KillAndDecObjRef(syncEngine_);
        // waiting all thread exist
        std::mutex engineMutex;
        std::unique_lock<std::mutex> cvLock(engineMutex);
        bool engineFinalize = engineFinalizeCv_.wait_for(cvLock, std::chrono::milliseconds(DBConstant::MIN_TIMEOUT),
            [this]() { return engineFinalize_; });
        if (!engineFinalize) {
            LOGW("syncer finalize before engine finalize!");
        }
        syncEngine_ = nullptr;
    }
    timeHelper_ = nullptr;
    metadata_ = nullptr;
    syncInterface_ = nullptr;
}

int GenericSyncer::Initialize(ISyncInterface *syncInterface, bool isNeedActive)
{
    if (syncInterface == nullptr) {
        LOGE("[Syncer] Init failed, the syncInterface is null!");
        return -E_INVALID_ARGS;
    }

    {
        std::lock_guard<std::mutex> lock(syncerLock_);
        if (initialized_) {
            return E_OK;
        }
        if (closing_) {
            LOGE("[Syncer] Syncer is closing, return!");
            return -E_BUSY;
        }
        std::vector<uint8_t> label = syncInterface->GetIdentifier();
        label.resize(3); // only show 3 Bytes enough
        label_ = DBCommon::VectorToHexString(label);

        // As metadata_ will be used in EraseDeviceWaterMark, it should not be clear even if engine init failed.
        // It will be clear in destructor.
        int errCodeMetadata = InitMetaData(syncInterface);

        // As timeHelper_ will be used in GetTimestamp, it should not be clear even if engine init failed.
        // It will be clear in destructor.
        int errCodeTimeHelper = InitTimeHelper(syncInterface);
        if (errCodeMetadata != E_OK || errCodeTimeHelper != E_OK) {
            return -E_INTERNAL_ERROR;
        }
        int errCode = CheckSyncActive(syncInterface, isNeedActive);
        if (errCode != E_OK) {
            return errCode;
        }

        if (!RuntimeContext::GetInstance()->IsCommunicatorAggregatorValid()) {
            LOGW("[Syncer] Communicator component not ready!");
            return -E_NOT_INIT;
        }

        errCode = SyncModuleInit();
        if (errCode != E_OK) {
            LOGE("[Syncer] Sync ModuleInit ERR!");
            return -E_INTERNAL_ERROR;
        }

        errCode = InitSyncEngine(syncInterface);
        if (errCode != E_OK) {
            return errCode;
        }
        syncEngine_->SetEqualIdentifier();
        initialized_ = true;
    }

    // RegConnectCallback may start an auto sync, this function can not in syncerLock_
    syncEngine_->RegConnectCallback();
    return E_OK;
}

int GenericSyncer::Close(bool isClosedOperation)
{
    {
        std::lock_guard<std::mutex> lock(syncerLock_);
        if (!initialized_) {
            LOGW("[Syncer] Syncer[%s] don't need to close, because it has no been init", label_.c_str());
            timeHelper_ = nullptr;
            metadata_ = nullptr;
            return -E_NOT_INIT;
        }
        initialized_ = false;
        if (closing_) {
            LOGE("[Syncer] Syncer is closing, return!");
            return -E_BUSY;
        }
        closing_ = true;
    }
    ClearSyncOperations(isClosedOperation);
    if (syncEngine_ != nullptr) {
        syncEngine_->Close();
        LOGD("[Syncer] Close SyncEngine!");
        std::lock_guard<std::mutex> lock(syncerLock_);
        closing_ = false;
    }
    timeHelper_ = nullptr;
    metadata_ = nullptr;
    return E_OK;
}

int GenericSyncer::Sync(const std::vector<std::string> &devices, int mode,
    const std::function<void(const std::map<std::string, int> &)> &onComplete,
    const std::function<void(void)> &onFinalize, bool wait = false)
{
    SyncParma param;
    param.devices = devices;
    param.mode = mode;
    param.onComplete = onComplete;
    param.onFinalize = onFinalize;
    param.wait = wait;
    return Sync(param);
}

int GenericSyncer::Sync(const InternalSyncParma &param)
{
    SyncParma syncParam;
    syncParam.devices = param.devices;
    syncParam.mode = param.mode;
    syncParam.isQuerySync = param.isQuerySync;
    syncParam.syncQuery = param.syncQuery;
    return Sync(syncParam);
}

int GenericSyncer::Sync(const SyncParma &param)
{
    return Sync(param, DBConstant::IGNORE_CONNECTION_ID);
}

int GenericSyncer::Sync(const SyncParma &param, uint64_t connectionId)
{
    int errCode = SyncParamCheck(param);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = AddQueuedManualSyncSize(param.mode, param.wait);
    if (errCode != E_OK) {
        return errCode;
    }

    uint32_t syncId = GenerateSyncId();
    errCode = PrepareSync(param, syncId, connectionId);
    if (errCode != E_OK) {
        LOGE("[Syncer] PrepareSync failed when sync called, err %d", errCode);
        return errCode;
    }
    PerformanceAnalysis::GetInstance()->StepTimeRecordEnd(PT_TEST_RECORDS::RECORD_SYNC_TOTAL);
    return E_OK;
}

int GenericSyncer::PrepareSync(const SyncParma &param, uint32_t syncId, uint64_t connectionId)
{
    auto *operation =
        new (std::nothrow) SyncOperation(syncId, param.devices, param.mode, param.onComplete, param.wait);
    if (operation == nullptr) {
        SubQueuedSyncSize();
        return -E_OUT_OF_MEMORY;
    }
    operation->SetIdentifier(syncInterface_->GetIdentifier());
    {
        std::lock_guard<std::mutex> autoLock(syncerLock_);
        PerformanceAnalysis::GetInstance()->StepTimeRecordStart(PT_TEST_RECORDS::RECORD_SYNC_TOTAL);
        InitSyncOperation(operation, param);
        LOGI("[Syncer] GenerateSyncId %" PRIu32 ", mode = %d, wait = %d, label = %s, devices = %s", syncId, param.mode,
            param.wait, label_.c_str(), GetSyncDevicesStr(param.devices).c_str());
        AddSyncOperation(operation);
        PerformanceAnalysis::GetInstance()->StepTimeRecordEnd(PT_TEST_RECORDS::RECORD_SYNC_TOTAL);
    }
    if (!param.wait && connectionId != DBConstant::IGNORE_CONNECTION_ID) {
        std::lock_guard<std::mutex> lockGuard(syncIdLock_);
        connectionIdMap_[connectionId].push_back(static_cast<int>(syncId));
        syncIdMap_[static_cast<int>(syncId)] = connectionId;
    }
    if (operation->CheckIsAllFinished()) {
        operation->Finished();
        RefObject::KillAndDecObjRef(operation);
    } else {
        operation->WaitIfNeed();
        RefObject::DecObjRef(operation);
    }
    return E_OK;
}

int GenericSyncer::RemoveSyncOperation(int syncId)
{
    SyncOperation *operation = nullptr;
    std::unique_lock<std::mutex> lock(operationMapLock_);
    auto iter = syncOperationMap_.find(syncId);
    if (iter != syncOperationMap_.end()) {
        LOGD("[Syncer] RemoveSyncOperation id:%d.", syncId);
        operation = iter->second;
        syncOperationMap_.erase(syncId);
        lock.unlock();
        if ((!operation->IsAutoSync()) && (!operation->IsBlockSync()) && (!operation->IsAutoControlCmd())) {
            SubQueuedSyncSize();
        }
        operation->NotifyIfNeed();
        RefObject::KillAndDecObjRef(operation);
        operation = nullptr;
        std::lock_guard<std::mutex> lockGuard(syncIdLock_);
        if (syncIdMap_.find(syncId) == syncIdMap_.end()) {
            return E_OK;
        }
        uint64_t connectionId = syncIdMap_[syncId];
        if (connectionIdMap_.find(connectionId) != connectionIdMap_.end()) {
            connectionIdMap_[connectionId].remove(syncId);
        }
        syncIdMap_.erase(syncId);
        return E_OK;
    }
    return -E_INVALID_ARGS;
}

int GenericSyncer::StopSync(uint64_t connectionId)
{
    std::list<int> syncIdList;
    {
        std::lock_guard<std::mutex> lockGuard(syncIdLock_);
        if (connectionIdMap_.find(connectionId) == connectionIdMap_.end()) {
            return E_OK;
        }
        syncIdList = connectionIdMap_[connectionId];
        connectionIdMap_.erase(connectionId);
    }
    for (auto syncId : syncIdList) {
        RemoveSyncOperation(syncId);
    }
    return E_OK;
}

uint64_t GenericSyncer::GetTimestamp()
{
    if (timeHelper_ == nullptr) {
        return TimeHelper::GetSysCurrentTime();
    }
    return timeHelper_->GetTime();
}

void GenericSyncer::QueryAutoSync(const InternalSyncParma &param)
{
    (void)param;
}

void GenericSyncer::AddSyncOperation(SyncOperation *operation)
{
    if (operation == nullptr) {
        return;
    }

    LOGD("[Syncer] AddSyncOperation.");
    syncEngine_->AddSyncOperation(operation);

    if (operation->CheckIsAllFinished()) {
        return;
    }

    std::lock_guard<std::mutex> lock(operationMapLock_);
    syncOperationMap_.insert(std::pair<int, SyncOperation *>(operation->GetSyncId(), operation));
    // To make sure operation alive before WaitIfNeed out
    RefObject::IncObjRef(operation);
}

void GenericSyncer::SyncOperationKillCallbackInner(int syncId)
{
    if (syncEngine_ != nullptr) {
        LOGI("[Syncer] Operation on kill id = %d", syncId);
        syncEngine_->RemoveSyncOperation(syncId);
    }
}

void GenericSyncer::SyncOperationKillCallback(int syncId)
{
    SyncOperationKillCallbackInner(syncId);
}

int GenericSyncer::InitMetaData(ISyncInterface *syncInterface)
{
    if (metadata_ != nullptr) {
        return E_OK;
    }

    metadata_ = std::make_shared<Metadata>();
    int errCode = metadata_->Initialize(syncInterface);
    if (errCode != E_OK) {
        LOGE("[Syncer] metadata Initializeate failed! err %d.", errCode);
        metadata_ = nullptr;
    }
    return errCode;
}

int GenericSyncer::InitTimeHelper(ISyncInterface *syncInterface)
{
    if (timeHelper_ != nullptr) {
        return E_OK;
    }

    timeHelper_ = std::make_shared<TimeHelper>();
    int errCode = timeHelper_->Initialize(syncInterface, metadata_);
    if (errCode != E_OK) {
        LOGE("[Syncer] TimeHelper init failed! err:%d.", errCode);
        timeHelper_ = nullptr;
    }
    return errCode;
}

int GenericSyncer::InitSyncEngine(ISyncInterface *syncInterface)
{
    if (syncEngine_ != nullptr && syncEngine_->IsEngineActive()) {
        LOGI("[Syncer] syncEngine is active");
        return E_OK;
    }
    int errCode = BuildSyncEngine();
    if (errCode != E_OK) {
        return errCode;
    }
    const std::function<void(std::string)> onlineFunc = std::bind(&GenericSyncer::RemoteDataChanged,
        this, std::placeholders::_1);
    const std::function<void(std::string)> offlineFunc = std::bind(&GenericSyncer::RemoteDeviceOffline,
        this, std::placeholders::_1);
    const std::function<void(const InternalSyncParma &param)> queryAutoSyncFunc =
        std::bind(&GenericSyncer::QueryAutoSync, this, std::placeholders::_1);
    errCode = syncEngine_->Initialize(syncInterface, metadata_, onlineFunc, offlineFunc, queryAutoSyncFunc);
    if (errCode == E_OK) {
        syncInterface_ = syncInterface;
        syncInterface->IncRefCount();
        label_ = syncEngine_->GetLabel();
        return E_OK;
    } else {
        LOGE("[Syncer] SyncEngine init failed! err:%d.", errCode);
        if (syncEngine_ != nullptr) {
            RefObject::KillAndDecObjRef(syncEngine_);
            syncEngine_ = nullptr;
        }
        return errCode;
    }
}

int GenericSyncer::CheckSyncActive(ISyncInterface *syncInterface, bool isNeedActive)
{
    bool isSyncDualTupleMode = syncInterface->GetDbProperties().GetBoolProp(KvDBProperties::SYNC_DUAL_TUPLE_MODE,
        false);
    if (!isSyncDualTupleMode || isNeedActive) {
        return E_OK;
    }
    LOGI("[Syncer] syncer no need to active");
    int errCode = BuildSyncEngine();
    if (errCode != E_OK) {
        return errCode;
    }
    return -E_NO_NEED_ACTIVE;
}

uint32_t GenericSyncer::GenerateSyncId()
{
    std::lock_guard<std::mutex> lock(syncIdLock_);
    currentSyncId_++;
    // if overflow, reset to 1
    if (currentSyncId_ <= 0) {
        currentSyncId_ = MIN_VALID_SYNC_ID;
    }
    return currentSyncId_;
}

bool GenericSyncer::IsValidMode(int mode) const
{
    if ((mode >= SyncModeType::INVALID_MODE) || (mode < SyncModeType::PUSH)) {
        LOGE("[Syncer] Sync mode is not valid!");
        return false;
    }
    return true;
}

int GenericSyncer::SyncConditionCheck(QuerySyncObject &query, int mode, bool isQuerySync,
    const std::vector<std::string> &devices) const
{
    (void)query;
    (void)mode;
    (void)isQuerySync;
    (void)(devices);
    return E_OK;
}

bool GenericSyncer::IsValidDevices(const std::vector<std::string> &devices) const
{
    if (devices.empty()) {
        LOGE("[Syncer] devices is empty!");
        return false;
    }
    return true;
}

void GenericSyncer::ClearSyncOperations(bool isClosedOperation)
{
    std::vector<SyncOperation *> syncOperation;
    {
        std::lock_guard<std::mutex> lock(operationMapLock_);
        for (auto &item : syncOperationMap_) {
            bool isBlockSync = item.second->IsBlockSync();
            if (isBlockSync || !isClosedOperation) {
                int status = (!isClosedOperation) ? SyncOperation::OP_USER_CHANGED : SyncOperation::OP_FAILED;
                item.second->SetUnfinishedDevStatus(status);
                RefObject::IncObjRef(item.second);
                syncOperation.push_back(item.second);
            }
        }
    }
    for (auto &operation : syncOperation) {
        // block sync operation or userChange will trigger remove sync operation
        // caller won't blocked for block sync
        // caller won't blocked for userChange operation no mater it is block or non-block sync
        TriggerSyncFinished(operation);
        RefObject::DecObjRef(operation);
    }
    {
        std::lock_guard<std::mutex> lock(operationMapLock_);
        for (auto &iter : syncOperationMap_) {
            RefObject::KillAndDecObjRef(iter.second);
            iter.second = nullptr;
        }
        syncOperationMap_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(syncIdLock_);
        connectionIdMap_.clear();
        syncIdMap_.clear();
    }
}

void GenericSyncer::TriggerSyncFinished(SyncOperation *operation)
{
    if (operation != nullptr && operation->CheckIsAllFinished()) {
        operation->Finished();
    }
}

void GenericSyncer::OnSyncFinished(int syncId)
{
    (void)(RemoveSyncOperation(syncId));
}

int GenericSyncer::SyncModuleInit()
{
    static bool isInit = false;
    std::lock_guard<std::mutex> lock(moduleInitLock_);
    if (!isInit) {
        int errCode = SyncResourceInit();
        if (errCode != E_OK) {
            return errCode;
        }
        isInit = true;
        return E_OK;
    }
    return E_OK;
}

int GenericSyncer::SyncResourceInit()
{
    int errCode = TimeSync::RegisterTransformFunc();
    if (errCode != E_OK) {
        LOGE("Register timesync message transform func ERR!");
        return errCode;
    }
    errCode = SingleVerSerializeManager::RegisterTransformFunc();
    if (errCode != E_OK) {
        LOGE("Register SingleVerDataSync message transform func ERR!");
        return errCode;
    }
#ifndef OMIT_MULTI_VER
    errCode = CommitHistorySync::RegisterTransformFunc();
    if (errCode != E_OK) {
        LOGE("Register CommitHistorySync message transform func ERR!");
        return errCode;
    }
    errCode = MultiVerDataSync::RegisterTransformFunc();
    if (errCode != E_OK) {
        LOGE("Register MultiVerDataSync message transform func ERR!");
        return errCode;
    }
    errCode = ValueSliceSync::RegisterTransformFunc();
    if (errCode != E_OK) {
        LOGE("Register ValueSliceSync message transform func ERR!");
        return errCode;
    }
#endif
    errCode = DeviceManager::RegisterTransformFunc();
    if (errCode != E_OK) {
        LOGE("Register DeviceManager message transform func ERR!");
        return errCode;
    }
    errCode = AbilitySync::RegisterTransformFunc();
    if (errCode != E_OK) {
        LOGE("Register AbilitySync message transform func ERR!");
        return errCode;
    }
    return E_OK;
}

int GenericSyncer::GetQueuedSyncSize(int *queuedSyncSize) const
{
    if (queuedSyncSize == nullptr) {
        return -E_INVALID_ARGS;
    }
    std::lock_guard<std::mutex> lock(queuedManualSyncLock_);
    *queuedSyncSize = queuedManualSyncSize_;
    LOGI("[GenericSyncer] GetQueuedSyncSize:%d", queuedManualSyncSize_);
    return E_OK;
}

int GenericSyncer::SetQueuedSyncLimit(const int *queuedSyncLimit)
{
    if (queuedSyncLimit == nullptr) {
        return -E_INVALID_ARGS;
    }
    std::lock_guard<std::mutex> lock(queuedManualSyncLock_);
    queuedManualSyncLimit_ = *queuedSyncLimit;
    LOGI("[GenericSyncer] SetQueuedSyncLimit:%d", queuedManualSyncLimit_);
    return E_OK;
}

int GenericSyncer::GetQueuedSyncLimit(int *queuedSyncLimit) const
{
    if (queuedSyncLimit == nullptr) {
        return -E_INVALID_ARGS;
    }
    std::lock_guard<std::mutex> lock(queuedManualSyncLock_);
    *queuedSyncLimit = queuedManualSyncLimit_;
    LOGI("[GenericSyncer] GetQueuedSyncLimit:%d", queuedManualSyncLimit_);
    return E_OK;
}

bool GenericSyncer::IsManualSync(int inMode) const
{
    int mode = SyncOperation::TransferSyncMode(inMode);
    if ((mode == SyncModeType::PULL) || (mode == SyncModeType::PUSH) || (mode == SyncModeType::PUSH_AND_PULL) ||
        (mode == SyncModeType::SUBSCRIBE_QUERY) || (mode == SyncModeType::UNSUBSCRIBE_QUERY)) {
        return true;
    }
    return false;
}

int GenericSyncer::AddQueuedManualSyncSize(int mode, bool wait)
{
    if (IsManualSync(mode) && (!wait)) {
        std::lock_guard<std::mutex> lock(queuedManualSyncLock_);
        if (!manualSyncEnable_) {
            LOGI("[GenericSyncer] manualSyncEnable is Disable");
            return -E_BUSY;
        }
        queuedManualSyncSize_++;
    }
    return E_OK;
}

bool GenericSyncer::IsQueuedManualSyncFull(int mode, bool wait) const
{
    std::lock_guard<std::mutex> lock(queuedManualSyncLock_);
    if (IsManualSync(mode) && (!manualSyncEnable_)) {
        LOGI("[GenericSyncer] manualSyncEnable_:false");
        return true;
    }
    if (IsManualSync(mode) && (!wait)) {
        if (queuedManualSyncSize_ < queuedManualSyncLimit_) {
            return false;
        } else {
            LOGD("[GenericSyncer] queuedManualSyncSize_:%d < queuedManualSyncLimit_:%d", queuedManualSyncSize_,
                queuedManualSyncLimit_);
            return true;
        }
    } else {
        return false;
    }
}

void GenericSyncer::SubQueuedSyncSize(void)
{
    std::lock_guard<std::mutex> lock(queuedManualSyncLock_);
    queuedManualSyncSize_--;
    if (queuedManualSyncSize_ < 0) {
        LOGE("[GenericSyncer] queuedManualSyncSize_ < 0!");
        queuedManualSyncSize_ = 0;
    }
}

int GenericSyncer::DisableManualSync(void)
{
    std::lock_guard<std::mutex> lock(queuedManualSyncLock_);
    if (queuedManualSyncSize_ > 0) {
        LOGD("[GenericSyncer] DisableManualSync fail, queuedManualSyncSize_:%d", queuedManualSyncSize_);
        return -E_BUSY;
    }
    manualSyncEnable_ = false;
    LOGD("[GenericSyncer] DisableManualSync ok");
    return E_OK;
}

int GenericSyncer::EnableManualSync(void)
{
    std::lock_guard<std::mutex> lock(queuedManualSyncLock_);
    manualSyncEnable_ = true;
    LOGD("[GenericSyncer] EnableManualSync ok");
    return E_OK;
}

int GenericSyncer::GetLocalIdentity(std::string &outTarget) const
{
    std::string deviceId;
    int errCode =  RuntimeContext::GetInstance()->GetLocalIdentity(deviceId);
    if (errCode != E_OK) {
        LOGE("[GenericSyncer] GetLocalIdentity fail errCode:%d", errCode);
        return errCode;
    }
    outTarget = DBCommon::TransferHashString(deviceId);
    return E_OK;
}

void GenericSyncer::GetOnlineDevices(std::vector<std::string> &devices) const
{
    // Get devices from AutoLaunch first.
    if (syncInterface_ == nullptr) {
        LOGI("[Syncer] GetOnlineDevices syncInterface_ is nullptr");
        return;
    }
    bool isSyncDualTupleMode = syncInterface_->GetDbProperties().GetBoolProp(KvDBProperties::SYNC_DUAL_TUPLE_MODE,
        false);
    std::string identifier;
    if (isSyncDualTupleMode) {
        identifier = syncInterface_->GetDbProperties().GetStringProp(KvDBProperties::DUAL_TUPLE_IDENTIFIER_DATA, "");
    } else {
        identifier = syncInterface_->GetDbProperties().GetStringProp(KvDBProperties::IDENTIFIER_DATA, "");
    }
    RuntimeContext::GetInstance()->GetAutoLaunchSyncDevices(identifier, devices);
    if (!devices.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(syncerLock_);
    if (closing_) {
        LOGE("[Syncer] Syncer is closing, return!");
        return;
    }
    if (syncEngine_ != nullptr) {
        syncEngine_->GetOnlineDevices(devices);
    }
}

int GenericSyncer::SetSyncRetry(bool isRetry)
{
    if (syncEngine_ == nullptr) {
        return -E_NOT_INIT;
    }
    syncEngine_->SetSyncRetry(isRetry);
    return E_OK;
}

int GenericSyncer::SetEqualIdentifier(const std::string &identifier, const std::vector<std::string> &targets)
{
    std::lock_guard<std::mutex> lock(syncerLock_);
    if (syncEngine_ == nullptr) {
        return -E_NOT_INIT;
    }
    int errCode = syncEngine_->SetEqualIdentifier(identifier, targets);
    if (errCode == E_OK) {
        syncEngine_->SetEqualIdentifierMap(identifier, targets);
    }
    return errCode;
}

std::string GenericSyncer::GetSyncDevicesStr(const std::vector<std::string> &devices) const
{
    std::string syncDevices;
    for (const auto &dev:devices) {
        syncDevices += STR_MASK(dev);
        syncDevices += ",";
    }
    return syncDevices.substr(0, syncDevices.size() - 1);
}

int GenericSyncer::StatusCheck() const
{
    if (!initialized_) {
        LOGE("[Syncer] Syncer is not initialized, return!");
        return -E_NOT_INIT;
    }
    if (closing_) {
        LOGE("[Syncer] Syncer is closing, return!");
        return -E_BUSY;
    }
    return E_OK;
}

int GenericSyncer::SyncParamCheck(const SyncParma &param) const
{
    std::lock_guard<std::mutex> lock(syncerLock_);
    int errCode = StatusCheck();
    if (errCode != E_OK) {
        return errCode;
    }
    if (!IsValidDevices(param.devices) || !IsValidMode(param.mode)) {
        return -E_INVALID_ARGS;
    }
    if (IsQueuedManualSyncFull(param.mode, param.wait)) {
        LOGE("[Syncer] -E_BUSY");
        return -E_BUSY;
    }
    QuerySyncObject syncQuery = param.syncQuery;
    return SyncConditionCheck(syncQuery, param.mode, param.isQuerySync, param.devices);
}

void GenericSyncer::InitSyncOperation(SyncOperation *operation, const SyncParma &param)
{
    operation->SetIdentifier(syncInterface_->GetIdentifier());
    operation->Initialize();
    operation->OnKill(std::bind(&GenericSyncer::SyncOperationKillCallback, this, operation->GetSyncId()));
    std::function<void(int)> onFinished = std::bind(&GenericSyncer::OnSyncFinished, this, std::placeholders::_1);
    operation->SetOnSyncFinished(onFinished);
    operation->SetOnSyncFinalize(param.onFinalize);
    if (param.isQuerySync) {
        operation->SetQuery(param.syncQuery);
    }
}

int GenericSyncer::BuildSyncEngine()
{
    if (syncEngine_ != nullptr) {
        return E_OK;
    }
    syncEngine_ = CreateSyncEngine();
    if (syncEngine_ == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    syncEngine_->OnLastRef([this]() {
        LOGD("[Syncer] SyncEngine finalized");
        engineFinalize_ = true;
        engineFinalizeCv_.notify_all();
    });
    return E_OK;
}
} // namespace DistributedDB
