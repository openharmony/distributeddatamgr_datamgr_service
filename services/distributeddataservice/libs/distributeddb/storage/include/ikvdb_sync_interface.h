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

#ifndef I_KVDB_SYNC_INTERFACE_H
#define I_KVDB_SYNC_INTERFACE_H

#include <string>

#include "db_types.h"
#include "kvdb_properties.h"
#include "sync_generic_interface.h"

namespace DistributedDB {
class IKvDBSyncInterface : public SyncGenericInterface {
public:

    // Constructor/Destructor.
    IKvDBSyncInterface() = default;
    ~IKvDBSyncInterface() override = default;
};
} // namespace DistributedDB

#endif // I_KVDB_SYNC_INTERFACE_H
