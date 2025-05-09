/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <set>
#include <vector>
#include "user_delegate_mock.h"

namespace OHOS::DistributedData {
using namespace OHOS::DistributedKv;
using DistributedData::UserStatus;
std::string GetLocalDeviceId()
{
    return "";
}

UserDelegate &UserDelegate::GetInstance()
{
    static UserDelegate instance;
    return instance;
}

std::vector<UserStatus> UserDelegate::GetLocalUserStatus()
{
    if (BUserDelegate::userDelegate == nullptr) {
        return {};
    }
    return BUserDelegate::userDelegate->GetLocalUserStatus();
}

std::vector<DistributedData::UserStatus> UserDelegate::GetRemoteUserStatus(const std::string &deviceId)
{
    if (BUserDelegate::userDelegate == nullptr) {
        return {};
    }
    return BUserDelegate::userDelegate->GetRemoteUserStatus(deviceId);
}

std::set<std::string> UserDelegate::GetLocalUsers()
{
    return {};
}

std::vector<UserStatus> UserDelegate::GetUsers(const std::string &deviceId)
{
    return {};
}

void UserDelegate::DeleteUsers(const std::string &deviceId)
{
    return;
}

void UserDelegate::UpdateUsers(const std::string &deviceId, const std::vector<UserStatus> &userStatus)
{
    return;
}

bool UserDelegate::InitLocalUserMeta()
{
    return true;
}

void UserDelegate::Init(const std::shared_ptr<ExecutorPool> &executors)
{
    return;
}

ExecutorPool::Task UserDelegate::GeTask()
{
    return [this] {
        return;
    };
}

bool UserDelegate::NotifyUserEvent(const UserDelegate::UserEvent &userEvent)
{
    return true;
}

void UserDelegate::LocalUserObserver::OnAccountChanged(const AccountEventInfo &eventInfo, int32_t timeout)
{
    userDelegate_.NotifyUserEvent({});
}

UserDelegate::LocalUserObserver::LocalUserObserver(UserDelegate &userDelegate) : userDelegate_(userDelegate) {}

std::string UserDelegate::LocalUserObserver::Name()
{
    return "user_delegate";
}
}