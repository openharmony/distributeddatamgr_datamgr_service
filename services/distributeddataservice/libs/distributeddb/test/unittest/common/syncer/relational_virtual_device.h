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
#ifndef RELATIONAL_VIRTUAL_DEVICE_H
#define RELATIONAL_VIRTUAL_DEVICE_H
#ifdef RELATIONAL_STORE

#include "data_transformer.h"
#include "generic_virtual_device.h"
#include "relational_schema_object.h"
#include "virtual_relational_ver_sync_db_interface.h"

namespace DistributedDB {
class RelationalVirtualDevice final : public GenericVirtualDevice {
public:
    explicit RelationalVirtualDevice(const std::string &deviceId);
    ~RelationalVirtualDevice() override;

    int PutData(const std::string &tableName, const std::vector<VirtualRowData> &dataList);
    int GetAllSyncData(const std::string &tableName, std::vector<VirtualRowData> &data);
    int GetSyncData(const std::string &tableName, const std::string &hashKey, VirtualRowData &data);
    void SetLocalFieldInfo(const std::vector<FieldInfo> &localFieldInfo);
    void SetTableInfo(const TableInfo &tableInfo);
    int Sync(SyncMode mode, bool wait) override;
    void EraseSyncData(const std::string &tableName);
};
}
#endif
#endif // RELATIONAL_VIRTUAL_DEVICE_H
