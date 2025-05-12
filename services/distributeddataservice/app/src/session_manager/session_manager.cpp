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

#include "session_manager.h"

#define LOG_TAG "SessionManager"

#include <algorithm>

#include "auth_delegate.h"
#include "checker/checker_manager.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "user_delegate.h"
#include "utils/anonymous.h"
#include "utils/converter.h"
#include "types.h"
#include "device_manager_adapter.h"
namespace OHOS::DistributedData {
using namespace OHOS::DistributedKv;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Account = AccountDelegate;
SessionManager &SessionManager::GetInstance()
{
    static SessionManager instance;
    return instance;
}

Session SessionManager::GetSession(const SessionPoint &local, const std::string &targetDeviceId) const
{
    ZLOGD("begin. peer device:%{public}s", Anonymous::Change(targetDeviceId).c_str());
    Session session;
    session.appId = local.appId;
    session.storeId = local.storeId;
    session.sourceUserId = local.userId;
    session.sourceDeviceId = local.deviceId;
    session.targetDeviceId = targetDeviceId;
    session.accountId = local.accountId;
    auto users = UserDelegate::GetInstance().GetRemoteUserStatus(targetDeviceId);
    // system service
    if (local.userId == UserDelegate::SYSTEM_USER) {
        session.targetUserIds.push_back(UserDelegate::SYSTEM_USER);
    }
    
    AclParams aclParams;
    if (!GetSendAuthParams(local, targetDeviceId, aclParams)) {
        ZLOGE("get send auth params failed:%{public}s", Anonymous::Change(targetDeviceId).c_str());
        return session;
    }

    std::vector<uint32_t> targetUsers {};
    for (const auto &user : users) {
        aclParams.accCallee.userId = user.id;
        auto [isPermitted, isSameAccount] = AuthDelegate::GetInstance()->CheckAccess(local.userId, user.id,
            targetDeviceId, aclParams);
        ZLOGD("targetDeviceId:%{public}s, user.id:%{public}d, isPermitted:%{public}d, isSameAccount: %{public}d",
            Anonymous::Change(targetDeviceId).c_str(), user.id, isPermitted, isSameAccount);
        if (isPermitted) {
            auto it = std::find(session.targetUserIds.begin(), session.targetUserIds.end(), user.id);
            if (it == session.targetUserIds.end() && isSameAccount) {
                session.targetUserIds.push_back(user.id);
            }
            if (it == session.targetUserIds.end() && !isSameAccount) {
                targetUsers.push_back(user.id);
            }
        }
    }
    session.targetUserIds.insert(session.targetUserIds.end(), targetUsers.begin(), targetUsers.end());
    ZLOGD("access to peer users:%{public}s", DistributedData::Serializable::Marshall(session.targetUserIds).c_str());
    return session;
}

bool SessionManager::GetSendAuthParams(const SessionPoint &local, const std::string &targetDeviceId,
    AclParams &aclParams) const
{
    std::vector<StoreMetaData> metaData;
    if (!MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ local.deviceId }), metaData)) {
        ZLOGE("load meta failed, deviceId:%{public}s, user:%{public}d", Anonymous::Change(local.deviceId).c_str(),
            local.userId);
        return false;
    }
    for (const auto &storeMeta : metaData) {
        if (storeMeta.appId == local.appId && storeMeta.storeId == local.storeId) {
            aclParams.accCaller.bundleName = storeMeta.bundleName;
            aclParams.accCaller.accountId = local.accountId;
            aclParams.accCaller.userId = local.userId;
            aclParams.accCaller.networkId = DmAdapter::GetInstance().ToNetworkID(local.deviceId);

            aclParams.accCallee.networkId = DmAdapter::GetInstance().ToNetworkID(targetDeviceId);
            aclParams.authType = storeMeta.authType;
            return true;
        }
    }
    ZLOGE("get params failed,appId:%{public}s,localDevId:%{public}s,tarDevid:%{public}s,user:%{public}d,",
        local.appId.c_str(), Anonymous::Change(local.deviceId).c_str(),
        Anonymous::Change(targetDeviceId).c_str(), local.userId);
    return false;
}

bool SessionManager::GetRecvAuthParams(const SessionPoint &local, const SessionPoint &peer, bool accountFlag,
    AclParams &aclParams) const
{
    std::vector<StoreMetaData> metaData;
    if (!MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ peer.deviceId }), metaData)) {
        ZLOGE("load meta failed, deviceId:%{public}s, user:%{public}d", Anonymous::Change(peer.deviceId).c_str(),
            peer.userId);
        return false;
    }
    for (const auto &storeMeta : metaData) {
        if (storeMeta.appId == local.appId) {
            auto accountId = AccountDelegate::GetInstance()->GetCurrentAccountId();
            aclParams.accCaller.bundleName = storeMeta.bundleName;
            aclParams.accCaller.accountId = accountId;
            aclParams.accCaller.userId = local.userId;
            aclParams.accCaller.networkId = DmAdapter::GetInstance().ToNetworkID(local.deviceId);

            aclParams.accCallee.accountId = accountFlag ? peer.accountId : accountId;
            aclParams.accCallee.userId = peer.userId;
            aclParams.accCallee.networkId = DmAdapter::GetInstance().ToNetworkID(peer.deviceId);
            aclParams.authType = storeMeta.authType;
            return true;
        }
    }

    ZLOGE("get params failed,appId:%{public}s,tarDevid:%{public}s,user:%{public}d,peer:%{public}d",
        local.appId.c_str(), Anonymous::Change(peer.deviceId).c_str(), local.userId, peer.userId);
    return false;
}

bool SessionManager::CheckSession(const SessionPoint &local, const SessionPoint &peer, bool accountFlag) const
{
    AclParams aclParams;
    if (!GetRecvAuthParams(local, peer, accountFlag, aclParams)) {
        ZLOGE("get recv auth params failed:%{public}s", Anonymous::Change(peer.deviceId).c_str());
        return false;
    }
    auto [isPermitted, isSameAccount] = AuthDelegate::GetInstance()->CheckAccess(local.userId,
        peer.userId, peer.deviceId, aclParams);
    ZLOGD("peer.deviceId:%{public}s, peer.userId:%{public}d, isPermitted:%{public}d, isSameAccount: %{public}d",
        Anonymous::Change(peer.deviceId).c_str(), peer.userId, isPermitted, isSameAccount);
    if (isPermitted && local.userId != UserDelegate::SYSTEM_USER) {
        isPermitted = Account::GetInstance()->IsUserForeground(local.userId);
    }
    return isPermitted;
}

bool Session::Marshal(json &node) const
{
    bool ret = true;
    ret = SetValue(node[GET_NAME(sourceDeviceId)], sourceDeviceId) && ret;
    ret = SetValue(node[GET_NAME(targetDeviceId)], targetDeviceId) && ret;
    ret = SetValue(node[GET_NAME(sourceUserId)], sourceUserId) && ret;
    ret = SetValue(node[GET_NAME(targetUserIds)], targetUserIds) && ret;
    ret = SetValue(node[GET_NAME(appId)], appId) && ret;
    ret = SetValue(node[GET_NAME(storeId)], storeId) && ret;
    ret = SetValue(node[GET_NAME(accountId)], accountId) && ret;
    return ret;
}

bool Session::Unmarshal(const json &node)
{
    bool ret = true;
    ret = GetValue(node, GET_NAME(sourceDeviceId), sourceDeviceId) && ret;
    ret = GetValue(node, GET_NAME(targetDeviceId), targetDeviceId) && ret;
    ret = GetValue(node, GET_NAME(sourceUserId), sourceUserId) && ret;
    ret = GetValue(node, GET_NAME(targetUserIds), targetUserIds) && ret;
    ret = GetValue(node, GET_NAME(appId), appId) && ret;
    ret = GetValue(node, GET_NAME(storeId), storeId) && ret;
    ret = GetValue(node, GET_NAME(accountId), accountId) && ret;
    return ret;
}
} // namespace OHOS::DistributedData
