/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#define LOG_TAG "AuthHandler"
#include "auth_delegate.h"

#include "checker/checker_manager.h"
#include "device_manager_adapter.h"
#include "log_print.h"
#include "user_delegate.h"
#include "utils/anonymous.h"
#include "metadata/store_meta_data.h"
#include "metadata/meta_data_manager.h"
namespace OHOS::DistributedData {
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
class AuthHandlerStub : public AuthHandler {
public:
    // override for mock auth in current version, need remove in the future
    std::pair<bool, bool> CheckAccess(int localUserId, int peerUserId, const std::string &peerDeviceId,
        const AclParams &aclParams) override;
private:
    bool IsUserActive(const std::vector<UserStatus> &users, int32_t userId);
    bool CheckUsers(int localUserId, int peerUserId, const std::string &peerDeviceId);
    bool IsSystemUser(int localUserId, int peerUserId);
    static constexpr pid_t UID_CAPACITY = 10000;
    static constexpr int SYSTEM_USER = 0;
};

bool AuthHandlerStub::IsSystemUser(int localUserId, int peerUserId)
{
    return localUserId == SYSTEM_USER && peerUserId == SYSTEM_USER;
}

bool AuthHandlerStub::CheckUsers(int localUserId, int peerUserId, const std::string &peerDeviceId)
{
    if (localUserId == SYSTEM_USER) {
        return peerUserId == SYSTEM_USER;
    }

    auto localUsers = UserDelegate::GetInstance().GetLocalUserStatus();
    auto peerUsers = UserDelegate::GetInstance().GetRemoteUserStatus(peerDeviceId);
    return peerUserId != SYSTEM_USER && IsUserActive(localUsers, localUserId) && IsUserActive(peerUsers, peerUserId);
}

std::pair<bool, bool> AuthHandlerStub::CheckAccess(int localUserId, int peerUserId, const std::string &peerDeviceId,
    const AclParams &aclParams, const std::string &appId)
{
    if (IsSystemUser(localUserId, peerUserId)) {
        return std::make_pair(true, false);
    }
    if (!CheckUsers(localUserId, peerUserId, peerDeviceId)) {
        return std::make_pair(false, false);
    }
    if (!DmAdapter::GetInstance().IsOHOSType(peerDeviceId)) {
        return std::make_pair(true, false);
    }
    if (aclParams.authType == static_cast<int32_t>(DistributedKv::AuthType::DEFAULT)) {
        if (DmAdapter::GetInstance().IsSameAccount(aclParams.accCaller, aclParams.accCallee)) {
            return std::make_pair(true, true);
        }
        if (DmAdapter::GetInstance().CheckAccessControl(aclParams.accCaller, aclParams.accCallee)) {
            return std::make_pair(true, false);
        }
        ZLOGE("CheckAccess failed. bundleName:%{public}s, localUser:%{public}d, peerUser:%{public}d",
            aclParams.accCaller.bundleName.c_str(), localUserId, peerUserId);
        return std::make_pair(false, false);
    }

    if (aclParams.authType == static_cast<int32_t>(DistributedKv::AuthType::IDENTICAL_ACCOUNT)) {
        auto isSameAccount = DmAdapter::GetInstance().IsSameAccount(aclParams.accCaller, aclParams.accCallee);
        return std::make_pair(isSameAccount, isSameAccount);
    }
    ZLOGE("CheckAccess failed.bundleName:%{public}s,peerDeviceId:%{public}s,authtype:%{public}d",
        aclParams.accCaller.bundleName.c_str(), Anonymous::Change(peerDeviceId).c_str(), aclParams.authType);
    return std::make_pair(false, false);
}

bool AuthHandlerStub::IsUserActive(const std::vector<UserStatus> &users, int32_t userId)
{
    for (const auto &user : users) {
        if (user.id == userId && user.isActive) {
            return true;
        }
    }
    return false;
}

AuthHandler *AuthDelegate::GetInstance()
{
    // change auth way in the future
    static AuthHandlerStub instance;
    return &instance;
}
} // namespace OHOS::DistributedData