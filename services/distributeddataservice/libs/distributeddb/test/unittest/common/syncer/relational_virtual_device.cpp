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
#ifdef RELATIONAL_STORE
#include "relational_virtual_device.h"
#include "virtual_relational_ver_sync_db_interface.h"
namespace DistributedDB {
RelationalVirtualDevice::RelationalVirtualDevice(const std::string &deviceId) : GenericVirtualDevice(deviceId)
{
}

RelationalVirtualDevice::~RelationalVirtualDevice()
{
}

int RelationalVirtualDevice::PutData(const std::string &tableName, const std::vector<VirtualRowData> &dataList)
{
    return static_cast<VirtualRelationalVerSyncDBInterface *>(storage_)->PutLocalData(dataList, tableName);
}

int RelationalVirtualDevice::GetAllSyncData(const std::string &tableName, std::vector<VirtualRowData> &data)
{
    return static_cast<VirtualRelationalVerSyncDBInterface *>(storage_)->GetAllSyncData(tableName, data);
}

int RelationalVirtualDevice::GetSyncData(const std::string &tableName,
    const std::string &hashKey, VirtualRowData &data)
{
    return static_cast<VirtualRelationalVerSyncDBInterface *>(storage_)->GetVirtualSyncData(tableName, hashKey, data);
}

void RelationalVirtualDevice::SetLocalFieldInfo(const std::vector<FieldInfo> &localFieldInfo)
{
    static_cast<VirtualRelationalVerSyncDBInterface *>(storage_)->SetLocalFieldInfo(localFieldInfo);
}

int RelationalVirtualDevice::Sync(SyncMode mode, bool wait)
{
    return -E_NOT_SUPPORT;
}

void RelationalVirtualDevice::EraseSyncData(const std::string &tableName)
{
    static_cast<VirtualRelationalVerSyncDBInterface *>(storage_)->EraseSyncData(tableName);
}

void RelationalVirtualDevice::SetTableInfo(const TableInfo &tableInfo)
{
    static_cast<VirtualRelationalVerSyncDBInterface *>(storage_)->SetTableInfo(tableInfo);
}
} // DistributedDB
#endif