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
#include "generic_virtual_device.h"

#include "multi_ver_sync_task_context.h"
#include "single_ver_kv_sync_task_context.h"
#include "single_ver_relational_sync_task_context.h"

namespace DistributedDB {
GenericVirtualDevice::GenericVirtualDevice(std::string deviceId)
    : communicateHandle_(nullptr),
      communicatorAggregator_(nullptr),
      storage_(nullptr),
      metadata_(nullptr),
      deviceId_(std::move(deviceId)),
      remoteDeviceId_("real_device"),
      context_(nullptr),
      onRemoteDataChanged_(nullptr),
      subManager_(nullptr)
{
}

GenericVirtualDevice::~GenericVirtualDevice()
{
    std::mutex cvMutex;
    std::condition_variable cv;
    bool finished = false;
    Offline();

    if (communicateHandle_ != nullptr) {
        communicateHandle_->RegOnMessageCallback(nullptr, nullptr);
        communicatorAggregator_->ReleaseCommunicator(communicateHandle_);
        communicateHandle_ = nullptr;
    }
    communicatorAggregator_ = nullptr;

    if (context_ != nullptr) {
        ISyncInterface *storage = storage_;
        context_->OnLastRef([storage, &cv, &cvMutex, &finished]() {
            delete storage;
            {
                std::lock_guard<std::mutex> lock(cvMutex);
                finished = true;
            }
            cv.notify_one();
        });
        RefObject::KillAndDecObjRef(context_);
        std::unique_lock<std::mutex> lock(cvMutex);
        cv.wait(lock, [&finished] { return finished; });
    } else {
        delete storage_;
    }
    context_ = nullptr;
    metadata_ = nullptr;
    storage_ = nullptr;
}

int GenericVirtualDevice::Initialize(VirtualCommunicatorAggregator *comAggregator, ISyncInterface *syncInterface)
{
    if ((comAggregator == nullptr) || (syncInterface == nullptr)) {
        return -E_INVALID_ARGS;
    }

    communicatorAggregator_ = comAggregator;
    int errCode = E_OK;
    communicateHandle_ = communicatorAggregator_->AllocCommunicator(deviceId_, errCode);
    if (communicateHandle_ == nullptr) {
        return errCode;
    }

    storage_ = syncInterface;
    metadata_ = std::make_shared<Metadata>();
    if (metadata_->Initialize(storage_) != E_OK) {
        LOGE("metadata_ init failed");
        return -E_NOT_SUPPORT;
    }
    if (storage_->GetInterfaceType() == IKvDBSyncInterface::SYNC_SVD) {
        context_ = new (std::nothrow) SingleVerKvSyncTaskContext;
        subManager_ = std::make_shared<SubscribeManager>();
        static_cast<SingleVerSyncTaskContext *>(context_)->SetSubscribeManager(subManager_);
    } else if (storage_->GetInterfaceType() == IKvDBSyncInterface::SYNC_RELATION) {
        context_ = new (std::nothrow) SingleVerRelationalSyncTaskContext;
    } else {
        context_ = new (std::nothrow) MultiVerSyncTaskContext;
    }
    if (context_ == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    communicateHandle_->RegOnMessageCallback(
        std::bind(&GenericVirtualDevice::MessageCallback, this, std::placeholders::_1, std::placeholders::_2), []() {});
    context_->Initialize(remoteDeviceId_, storage_, metadata_, communicateHandle_);
    context_->SetRetryStatus(SyncTaskContext::NO_NEED_RETRY);
    context_->RegOnSyncTask(std::bind(&GenericVirtualDevice::StartResponseTask, this));
    return E_OK;
}

void GenericVirtualDevice::SetDeviceId(const std::string &deviceId)
{
    deviceId_ = deviceId;
}

std::string GenericVirtualDevice::GetDeviceId() const
{
    return deviceId_;
}

int GenericVirtualDevice::MessageCallback(const std::string &deviceId, Message *inMsg)
{
    if (inMsg->GetMessageId() == LOCAL_DATA_CHANGED) {
        if (onRemoteDataChanged_) {
            onRemoteDataChanged_(deviceId);
            delete inMsg;
            inMsg = nullptr;
            return E_OK;
        }
        delete inMsg;
        inMsg = nullptr;
        return -E_INVALID_ARGS;
    }

    LOGD("[GenericVirtualDevice] onMessage, src %s", deviceId.c_str());
    RefObject::IncObjRef(context_);
    RefObject::IncObjRef(communicateHandle_);
    SyncTaskContext *context = context_;
    ICommunicator *communicateHandle = communicateHandle_;
    std::thread thread([context, communicateHandle, inMsg]() {
        int errCode = context->ReceiveMessageCallback(inMsg);
        if (errCode != -E_NOT_NEED_DELETE_MSG) {
            delete inMsg;
        }
        RefObject::DecObjRef(context);
        RefObject::DecObjRef(communicateHandle);
    });
    thread.detach();
    return E_OK;
}

void GenericVirtualDevice::OnRemoteDataChanged(const std::function<void(const std::string &)> &callback)
{
    onRemoteDataChanged_ = callback;
}

void GenericVirtualDevice::Online()
{
    static_cast<VirtualCommunicator *>(communicateHandle_)->Enable();
    communicatorAggregator_->OnlineDevice(deviceId_);
}

void GenericVirtualDevice::Offline()
{
    static_cast<VirtualCommunicator *>(communicateHandle_)->Disable();
    communicatorAggregator_->OfflineDevice(deviceId_);
}

int GenericVirtualDevice::StartResponseTask()
{
    LOGD("[KvVirtualDevice] StartResponseTask");
    RefObject::AutoLock lockGuard(context_);
    int status = context_->GetTaskExecStatus();
    if ((status == SyncTaskContext::RUNNING) || context_->IsKilled()) {
        LOGD("[KvVirtualDevice] StartResponseTask status:%d", status);
        return -E_NOT_SUPPORT;
    }
    if (context_->IsTargetQueueEmpty()) {
        LOGD("[KvVirtualDevice] StartResponseTask IsTargetQueueEmpty is empty");
        return E_OK;
    }
    context_->SetTaskExecStatus(ISyncTaskContext::RUNNING);
    context_->MoveToNextTarget();
    LOGI("[KvVirtualDevice] machine StartSync");
    context_->UnlockObj();
    int errCode = context_->StartStateMachine();
    context_->LockObj();
    if (errCode != E_OK) {
        LOGE("[KvVirtualDevice] machine StartSync failed");
        context_->SetOperationStatus(SyncOperation::OP_FAILED);
    }
    return errCode;
}

TimeOffset GenericVirtualDevice::GetLocalTimeOffset() const
{
    return metadata_->GetLocalTimeOffset();
}

int GenericVirtualDevice::Sync(SyncMode mode, bool wait)
{
    auto operation = new (std::nothrow) SyncOperation(1, {remoteDeviceId_}, mode, nullptr, wait);
    if (operation == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    operation->Initialize();
    operation->SetOnSyncFinished([operation](int id) {
        operation->NotifyIfNeed();
    });
    context_->AddSyncOperation(operation);
    operation->WaitIfNeed();
    RefObject::KillAndDecObjRef(operation);
    return E_OK;
}

int GenericVirtualDevice::Sync(SyncMode mode, const Query &query, bool wait)
{
    return Sync(mode, query, nullptr, wait);
}

int GenericVirtualDevice::Sync(SyncMode mode, const Query &query,
    const SyncOperation::UserCallback &callBack, bool wait)
{
    auto operation = new (std::nothrow) SyncOperation(1, {remoteDeviceId_}, mode, callBack, wait);
    if (operation == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    operation->Initialize();
    operation->SetOnSyncFinished([operation](int id) {
        operation->NotifyIfNeed();
    });
    QuerySyncObject querySyncObject(query);
    int errCode = querySyncObject.Init();
    if (errCode != E_OK) {
        return errCode;
    }
    operation->SetQuery(querySyncObject);
    context_->AddSyncOperation(operation);
    operation->WaitIfNeed();
    RefObject::KillAndDecObjRef(operation);
    return errCode;
}
} // DistributedDB