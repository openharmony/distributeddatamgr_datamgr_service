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

#include "generic_virtual_device.h"
#include "relational_schema_object.h"
#include "data_transformer.h"

namespace DistributedDB {
class RelationalVirtualDevice final : public GenericVirtualDevice {
public:
    explicit RelationalVirtualDevice(const std::string &deviceId);
    ~RelationalVirtualDevice() override;

    int PutData(const std::string &tableName, const std::vector<RowDataWithLog> &dataList);
    int GetAllSyncData(const std::string &tableName, std::vector<RowDataWithLog> &data);
    void SetLocalFieldInfo(const std::vector<FieldInfo> &localFieldInfo);
    int Sync(SyncMode mode, bool wait) override;
};
}
#endif
#endif // RELATIONAL_VIRTUAL_DEVICE_H
