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

#ifndef RUNTIME_CONFIG_H
#define RUNTIME_CONFIG_H

#include <memory>
#include <mutex>

#include "iprocess_communicator.h"
#include "iprocess_system_api_adapter.h"
#include "types.h"
namespace DistributedDB {
class RuntimeConfig final {
public:
    DB_API RuntimeConfig() = default;
    DB_API ~RuntimeConfig() = default;

    DB_API static DBStatus SetProcessLabel(const std::string &appId, const std::string &userId);

    DB_API static DBStatus SetProcessCommunicator(const std::shared_ptr<IProcessCommunicator> &inCommunicator);

    DB_API static DBStatus SetPermissionCheckCallback(const PermissionCheckCallbackV2 &callback);

    DB_API static DBStatus SetProcessSystemAPIAdapter(const std::shared_ptr<IProcessSystemApiAdapter> &adapter);

private:
    static std::mutex communicatorMutex_;
    static std::shared_ptr<IProcessCommunicator> processCommunicator_;
};
} // namespace DistributedDB

#endif // RUNTIME_CONFIG_H