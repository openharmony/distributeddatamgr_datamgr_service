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

#ifndef SINGLE_VER_KVDB_SYNC_INTERFACE_H
#define SINGLE_VER_KVDB_SYNC_INTERFACE_H

#include "ikvdb_sync_interface.h"
#include "single_ver_kv_entry.h"
#include "iprocess_system_api_adapter.h"
#include "query_object.h"
#include "intercepted_data.h"

namespace DistributedDB {
using MulDevTimeRanges = std::map<DeviceID, std::pair<Timestamp, Timestamp>>;
using MulDevSinVerKvEntry = std::map<DeviceID, std::vector<SingleVerKvEntry *>>;
using MulDevDataItems = std::map<DeviceID, std::vector<DataItem>>;

class SingleVerKvDBSyncInterface : public IKvDBSyncInterface {
public:
    SingleVerKvDBSyncInterface() = default;
    ~SingleVerKvDBSyncInterface() override = default;

    virtual SchemaObject GetSchemaInfo() const = 0;
};
}

#endif // SINGLE_VER_KVDB_SYNC_INTERFACE_H
