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

#ifndef CLIENT_PERMISSION_VALIDATOR_H
#define CLIENT_PERMISSION_VALIDATOR_H

#include <cstdint>
#include <map>
#include <mutex>
#include "bundlemgr/on_permission_changed_callback_host.h"
#include "permission_validator.h"

namespace OHOS {
namespace DistributedKv {

const std::string DISTRIBUTED_DATASYNC = "ohos.permission.DISTRIBUTED_DATASYNC";

// Callback registered with BMS to listen App permission changes.
class ClientPermissionChangedCallback : public AppExecFwk::OnPermissionChangedCallbackHost {
public:
    ClientPermissionChangedCallback(std::int32_t pid, std::int32_t uid);

    ~ClientPermissionChangedCallback() override = default;

    std::int32_t GetPid();

    void OnChanged(const int32_t uid) override;
private:
    std::int32_t pid_;
    std::int32_t uid_;
};

struct AppPermissionInfo : AppThreadInfo {
    sptr<ClientPermissionChangedCallback> callback;
};

class ClientPermissionValidator {
public:
    static ClientPermissionValidator &GetInstance()
    {
        static ClientPermissionValidator clientPermissionValidator;
        return clientPermissionValidator;
    }

    bool RegisterPermissionChanged(const KvStoreTuple &kvStoreTuple, const AppThreadInfo &appThreadInfo);

    void UnregisterPermissionChanged(const KvStoreTuple &kvStoreTuple);

    void UpdateKvStoreTupleMap(const KvStoreTuple &srcKvStoreTuple, const KvStoreTuple &dstKvStoreTuple);

    void UpdatePermissionStatus(int32_t uid, const std::string &permissionType, bool permissionStatus);

    bool CheckClientSyncPermission(const KvStoreTuple &kvStoreTuple, std::uint32_t tokenId, std::int32_t curUid);

private:
    ClientPermissionValidator() = default;

    ~ClientPermissionValidator() = default;

    ClientPermissionValidator(const ClientPermissionValidator &clientPermissionValidator);

    const ClientPermissionValidator &operator=(const ClientPermissionValidator &clientPermissionValidator);

    void RebuildBundleManager();

    std::mutex tupleMutex_;
    std::map<KvStoreTuple, AppPermissionInfo, KvStoreTupleCmp> kvStoreTupleMap_;
    std::mutex permissionMutex_;
    std::map<std::int32_t, bool> dataSyncPermissionMap_;
};
} // namespace DistributedKv
} // namespace OHOS

#endif // CLIENT_PERMISSION_VALIDATOR_H
