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

#ifndef KV_VIRTUAL_DEVICE_H
#define KV_VIRTUAL_DEVICE_H

#include "generic_virtual_device.h"

#include "virtual_single_ver_sync_db_Interface.h"
namespace DistributedDB {
class KvVirtualDevice final : public GenericVirtualDevice {
public:
    explicit KvVirtualDevice(const std::string &deviceId);
    ~KvVirtualDevice() override;

    int GetData(const Key &key, VirtualDataItem &item);
    int GetData(const Key &key, Value &value);
    int PutData(const Key &key, const Value &value, const Timestamp &time, int flag);
    int PutData(const Key &key, const Value &value);
    int DeleteData(const Key &key);
    int StartTransaction();
    int Commit();
    void SetSaveDataDelayTime(uint64_t milliDelayTime);

    int Subscribe(QuerySyncObject query, bool wait, int id);
    int UnSubscribe(QuerySyncObject query, bool wait, int id);
};
} // namespace DistributedDB

#endif  // KV_VIRTUAL_DEVICE_H
