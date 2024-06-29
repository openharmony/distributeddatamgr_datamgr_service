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
#include "device_auth.h"
#include "device_auth_defines.h"
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
    bool CheckAccess(
        const SessionPoint &from, int peerUserId, const std::string &peerDeviceId, bool isSend = true) override;
private:
    bool IsUserActive(const std::vector<UserStatus> &users, int32_t userId);
    bool GetParams(const SessionPoint &from, const std::string &peerDeviceId, std::string &bundleName, int32_t &auth);
    bool CheckUsers(int localUserId, int peerUserId, const std::string &peerDeviceId);
    static constexpr pid_t UID_CAPACITY = 10000;
    static constexpr int SYSTEM_USER = 0;
    static constexpr int32_t DEFAULT = 0;
    static constexpr int32_t IDENTICAL_ACCOUNT = 1
};

int32_t AuthHandler::GetGroupType(
    int localUserId, int peerUserId, const std::string &peerDeviceId, const std::string &appId)
{
    auto group = GetGroupInfo(localUserId, appId, peerDeviceId);
    if (group.groupType < GroupType::ALL_GROUP) {
        ZLOGE("failed to parse group json(%{public}d)", group.groupType);
    }
    return group.groupType;
}

AuthHandler::RelatedGroup AuthHandler::GetGroupInfo(
    int32_t localUserId, const std::string &appId, const std::string &peerDeviceId)
{
    auto groupManager = GetGmInstance();
    if (groupManager == nullptr || groupManager->getRelatedGroups == nullptr || groupManager->destroyInfo == nullptr) {
        ZLOGE("failed to get group manager");
        return {};
    }
    char *groupInfo = nullptr;
    uint32_t groupNum = 0;
    ZLOGI("get related groups, user:%{public}d, app:%{public}s", localUserId, appId.c_str());
    auto ret = groupManager->getRelatedGroups(localUserId, appId.c_str(), peerDeviceId.c_str(), &groupInfo, &groupNum);
    if (groupInfo == nullptr) {
        ZLOGE("failed to get related groups, ret:%{public}d", ret);
        return {};
    }
    ZLOGI("get related group json :%{public}s", groupInfo);
    std::vector<RelatedGroup> groups;
    RelatedGroup::Unmarshall(groupInfo, groups);
    groupManager->destroyInfo(&groupInfo);

    // same account has priority
    std::sort(groups.begin(), groups.end(),
        [](const RelatedGroup &group1, const RelatedGroup &group2) { return group1.groupType < group2.groupType; });
    if (!groups.empty()) {
        ZLOGI("get group type:%{public}d", groups.front().groupType);
        return groups.front();
    }
    ZLOGD("there is no group to access to peer device:%{public}s", Anonymous::Change(peerDeviceId).c_str());
    return {};
}

std::vector<std::string> AuthHandler::GetTrustedDevicesByType(
    AUTH_GROUP_TYPE type, int32_t localUserId, const std::string &appId)
{
    auto groupManager = GetGmInstance();
    if (groupManager == nullptr || groupManager->getRelatedGroups == nullptr
        || groupManager->getTrustedDevices == nullptr || groupManager->destroyInfo == nullptr) {
        ZLOGE("failed to get group manager");
        return {};
    }

    char *groupsJson = nullptr;
    uint32_t groupNum = 0;
    ZLOGI("get joined groups, user:%{public}d, app:%{public}s, type:%{public}d", localUserId, appId.c_str(), type);
    auto ret = groupManager->getJoinedGroups(localUserId, appId.c_str(), type, &groupsJson, &groupNum);
    if (groupsJson == nullptr) {
        ZLOGE("failed to get joined groups, ret:%{public}d", ret);
        return {};
    }
    ZLOGI("get joined group json :%{public}s", groupsJson);
    std::vector<RelatedGroup> groups;
    RelatedGroup::Unmarshall(groupsJson, groups);
    groupManager->destroyInfo(&groupsJson);

    std::vector<std::string> trustedDevices;
    for (const auto &group : groups) {
        if (group.groupType != type) {
            continue;
        }
        char *devicesJson = nullptr;
        uint32_t devNum = 0;
        ret = groupManager->getTrustedDevices(localUserId, appId.c_str(), group.groupId.c_str(), &devicesJson, &devNum);
        if (devicesJson == nullptr) {
            ZLOGE("failed to get trusted devicesJson, ret:%{public}d", ret);
            return {};
        }
        ZLOGI("get trusted device json:%{public}s", devicesJson);
        std::vector<TrustDevice> devices;
        TrustDevice::Unmarshall(devicesJson, devices);
        groupManager->destroyInfo(&devicesJson);
        for (const auto &item : devices) {
            auto &dmAdapter = DeviceManagerAdapter::GetInstance();
            auto networkId = dmAdapter.ToNetworkID(item.authId);
            auto uuid = dmAdapter.GetUuidByNetworkId(networkId);
            trustedDevices.push_back(uuid);
        }
    }

    return trustedDevices;
}

bool AuthHandlerStub::GetParams(const SessionPoint &from, const std::string &peerDeviceId, std::string &bundleName, int32_t &auth)
{
    std::vector<StoreMetaData> metaData;
    if (!MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ from.deviceId }), metaData)) {
        ZLOGW("load meta failed, deviceId:%{public}s", Anonymous::Change(from.deviceId).c_str());
        return false;
    }
    for (const auto &storeMeta : metaDate) {
        if (storeMeta.appId == from.appId) {
            bundleName = storeMeta.bundleName;
            auth = storeMeta.authTypes;
            break;
        }
    }
    if (bundleName.empty()) {
        ZLOGE("not find bundleName");
        return false;
    }
    return true;
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

bool AuthHandlerStub::CheckAccess(
        const SessionPoint &from, int peerUserId, const std::string &peerDeviceId, bool isSend)
{
    std::string bundleName = "";
    int32_t authType = DEFAULT;
    if (!GetParams(from, peerDeviceId, bundleName, authType)) {
        ZLOGE("getParams failed");
        return false;
    }
    if (authType == DEFAULT) {
        return CheckUsers(from.userId, peerUserId, peerDeviceId);
    }
    if (authType == IDENTICAL_ACCOUNT && DmAdapter::GetInstance().IsSameAccount(peerDeviceId)) {
        return CheckUsers(rom.userId, peerUserId, peerDeviceId);
    }
    ZLOGE("CheckAccess failed. authType:%{public}d, bundleName:%{public}s", authType, bundleName.c_str());
    return false; 
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