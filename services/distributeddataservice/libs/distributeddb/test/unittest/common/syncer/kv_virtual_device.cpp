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
#include "kv_virtual_device.h"

#include "log_print.h"
#include "virtual_multi_ver_sync_db_interface.h"

namespace DistributedDB {
KvVirtualDevice::KvVirtualDevice(const std::string &deviceId) : GenericVirtualDevice(deviceId)
{
}

KvVirtualDevice::~KvVirtualDevice()
{
}

int KvVirtualDevice::GetData(const Key &key, VirtualDataItem &item)
{
    VirtualSingleVerSyncDBInterface *syncAble = static_cast<VirtualSingleVerSyncDBInterface *>(storage_);
    return syncAble->GetSyncData(key, item);
}

int KvVirtualDevice::GetData(const Key &key, Value &value)
{
    VirtualMultiVerSyncDBInterface *syncInterface = static_cast<VirtualMultiVerSyncDBInterface *>(storage_);
    return syncInterface->GetData(key, value);
}

int KvVirtualDevice::PutData(const Key &key, const Value &value, const TimeStamp &time, int flag)
{
    VirtualSingleVerSyncDBInterface *syncAble = static_cast<VirtualSingleVerSyncDBInterface *>(storage_);
    LOGI("dev %s put data time %llu", deviceId_.c_str(), time);
    return syncAble->PutData(key, value, time, flag);
}

int KvVirtualDevice::PutData(const Key &key, const Value &value)
{
    VirtualMultiVerSyncDBInterface *syncInterface = static_cast<VirtualMultiVerSyncDBInterface *>(storage_);
    return syncInterface->PutData(key, value);
}

int KvVirtualDevice::DeleteData(const Key &key)
{
    VirtualMultiVerSyncDBInterface *syncInterface = static_cast<VirtualMultiVerSyncDBInterface *>(storage_);
    return syncInterface->DeleteData(key);
}

int KvVirtualDevice::StartTransaction()
{
    VirtualMultiVerSyncDBInterface *syncInterface = static_cast<VirtualMultiVerSyncDBInterface *>(storage_);
    return syncInterface->StartTransaction();
}

int KvVirtualDevice::Commit()
{
    VirtualMultiVerSyncDBInterface *syncInterface = static_cast<VirtualMultiVerSyncDBInterface *>(storage_);
    return syncInterface->Commit();
}


void KvVirtualDevice::SetSaveDataDelayTime(uint64_t milliDelayTime)
{
    VirtualSingleVerSyncDBInterface *syncInterface = static_cast<VirtualSingleVerSyncDBInterface *>(storage_);
    syncInterface->SetSaveDataDelayTime(milliDelayTime);
}

int KvVirtualDevice::Subscribe(QuerySyncObject query, bool wait, int id)
{
    auto operation = new (std::nothrow) SyncOperation(id, {remoteDeviceId_}, SUBSCRIBE_QUERY, nullptr, wait);
    if (operation == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    operation->Initialize();
    operation->SetOnSyncFinished([operation](int id) {
        operation->NotifyIfNeed();
    });
    operation->SetQuery(query);
    context_->AddSyncOperation(operation);
    operation->WaitIfNeed();
    RefObject::KillAndDecObjRef(operation);
    return E_OK;
}

int KvVirtualDevice::UnSubscribe(QuerySyncObject query, bool wait, int id)
{
    auto operation = new (std::nothrow) SyncOperation(id, {remoteDeviceId_}, UNSUBSCRIBE_QUERY, nullptr, wait);
    if (operation == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    operation->Initialize();
    operation->SetOnSyncFinished([operation](int id) {
        operation->NotifyIfNeed();
    });
    operation->SetQuery(query);
    context_->AddSyncOperation(operation);
    operation->WaitIfNeed();
    RefObject::KillAndDecObjRef(operation);
    return E_OK;
}
} // namespace DistributedDB