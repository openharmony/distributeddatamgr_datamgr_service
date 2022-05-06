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
#include "log/log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "user_delegate.h"
#include "utils/anonymous.h"

namespace OHOS::DistributedData {
using namespace OHOS::DistributedKv;
SessionManager &SessionManager::GetInstance()
{
    static SessionManager instance;
    return instance;
}

Session SessionManager::GetSession(const SessionPoint &from, const std::string &targetDeviceId) const
{
    ZLOGD("begin. peer device:%{public}s", Anonymous::Change(targetDeviceId).c_str());
    Session session;
    session.appId = from.appId;
    session.sourceUserId = from.userId;
    session.sourceDeviceId = from.deviceId;
    session.targetDeviceId = targetDeviceId;
    auto users = UserDelegate::GetInstance().GetRemoteUserStatus(targetDeviceId);
    StoreMetaData metaData;
    auto key = StoreMetaData::GetKey({ from.deviceId, std::to_string(from.userId),
                                       "default", from.appId, from.storeId });
    if (!MetaDataManager::GetInstance().LoadMeta(key, metaData)) {
        return session;
    }
    // system service
    if (from.userId == UserDelegate::SYSTEM_USER) {
        if (CheckerManager::GetInstance().GetAppId({ metaData.uid, metaData.tokenId, metaData.bundleName })
            == metaData.bundleName) {
            session.targetUserIds.push_back(UserDelegate::SYSTEM_USER);
        }
    }

    for (const auto &user : users) {
        bool isPermitted = AuthDelegate::GetInstance()->CheckAccess(from.userId, user.id, targetDeviceId, from.appId);
        ZLOGD("access to peer user %{public}d is %{public}d", user.id, isPermitted);
        if (isPermitted) {
            auto it = std::find(session.targetUserIds.begin(), session.targetUserIds.end(), user.id);
            if (it == session.targetUserIds.end()) {
                session.targetUserIds.push_back(user.id);
            }
        }
    }
    ZLOGD("end");
    return session;
}

bool SessionManager::CheckSession(const SessionPoint &from, const SessionPoint &to) const
{
    return AuthDelegate::GetInstance()->CheckAccess(from.userId, to.userId, to.deviceId, from.appId);
}

bool Session::Marshal(json &node) const
{
    bool ret = true;
    ret = SetValue(node[GET_NAME(sourceDeviceId)], sourceDeviceId) && ret;
    ret = SetValue(node[GET_NAME(targetDeviceId)], targetDeviceId) && ret;
    ret = SetValue(node[GET_NAME(sourceUserId)], sourceUserId) && ret;
    ret = SetValue(node[GET_NAME(targetUserIds)], targetUserIds) && ret;
    ret = SetValue(node[GET_NAME(appId)], appId) && ret;
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
    return ret;
}
} // namespace OHOS::DistributedData
