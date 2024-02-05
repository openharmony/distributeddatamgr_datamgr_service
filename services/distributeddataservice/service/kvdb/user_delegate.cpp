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

#define LOG_TAG "UserDelegate"
#include "user_delegate.h"
#include <chrono>
#include <cinttypes>
#include <thread>
#include "communicator/device_manager_adapter.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "utils/anonymous.h"

namespace OHOS::DistributedData {
using namespace OHOS::DistributedKv;
using namespace std::chrono;
std::string GetLocalDeviceId()
{
    return DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
}

std::vector<UserStatus> UserDelegate::GetLocalUserStatus()
{
    ZLOGI("begin");
    auto deviceId = GetLocalDeviceId();
    if (deviceId.empty()) {
        ZLOGE("failed to get local device id");
        return {};
    }
    return GetUsers(deviceId);
}

std::set<std::string> UserDelegate::GetLocalUsers()
{
    auto deviceId = GetLocalDeviceId();
    if (deviceId.empty()) {
        ZLOGE("failed to get local device id");
        return {};
    }
    std::set<std::string> users;
    deviceUser_.Compute(deviceId, [&users](const auto &key, auto &value) {
        if (value.empty()) {
            UserMetaData userMetaData;
            MetaDataManager::GetInstance().LoadMeta(UserMetaRow::GetKeyFor(key), userMetaData);
            for (const auto &user : userMetaData.users) {
                value[user.id] = user.isActive;
            }
        }
        for (const auto [user, active] : value) {
            users.emplace(std::to_string(user));
        }
        return !value.empty();
    });
    return users;
}

std::vector<DistributedData::UserStatus> UserDelegate::GetRemoteUserStatus(const std::string &deviceId)
{
    if (deviceId.empty()) {
        ZLOGE("error input device id");
        return {};
    }
    return GetUsers(deviceId);
}

std::vector<UserStatus> UserDelegate::GetUsers(const std::string &deviceId)
{
    std::vector<UserStatus> userStatus;
    deviceUser_.Compute(deviceId, [&userStatus](const auto &key, auto &users) {
        if (users.empty()) {
            UserMetaData userMetaData;
            MetaDataManager::GetInstance().LoadMeta(UserMetaRow::GetKeyFor(key), userMetaData);
            for (const auto &user : userMetaData.users) {
                users[user.id] = user.isActive;
            }
        }
        for (const auto [key, value] : users) {
            userStatus.emplace_back(key, value);
        }
        return !users.empty();
    });
    auto time =
        static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    ZLOGI("device:%{public}s, users:%{public}s times %{public}" PRIu64 ".", Anonymous::Change(deviceId).c_str(),
        Serializable::Marshall(userStatus).c_str(), time);
    return userStatus;
}

void UserDelegate::DeleteUsers(const std::string &deviceId)
{
    deviceUser_.Erase(deviceId);
}

void UserDelegate::UpdateUsers(const std::string &deviceId, const std::vector<UserStatus> &userStatus)
{
    ZLOGI("begin, device:%{public}s, users:%{public}zu", Anonymous::Change(deviceId).c_str(), userStatus.size());
    deviceUser_.Compute(deviceId, [&userStatus](const auto &key, std::map<int, bool> &users) {
        users = {};
        for (const auto &user : userStatus) {
            users[user.id] = user.isActive;
        }
        ZLOGI("end, device:%{public}s, users:%{public}zu", Anonymous::Change(key).c_str(), users.size());
        return true;
    });
}

bool UserDelegate::InitLocalUserMeta()
{
    std::vector<int> users;
    auto ret = AccountDelegate::GetInstance()->QueryUsers(users);
    if (!ret || users.empty()) {
        ZLOGE("failed to query os accounts, ret:%{public}d", ret);
        return false;
    }
    std::vector<UserStatus> userStatus = { { 0, true } };
    for (const auto &user : users) {
        userStatus.emplace_back(user, true);
    }
    UserMetaData userMetaData;
    userMetaData.deviceId = GetLocalDeviceId();
    UpdateUsers(userMetaData.deviceId, userStatus);
    deviceUser_.ComputeIfPresent(userMetaData.deviceId, [&userMetaData](const auto &, std::map<int, bool> &users) {
        for (const auto &[key, value] : users) {
            userMetaData.users.emplace_back(key, value);
        }
        return true;
    });
    ZLOGI("put user meta data save meta data");
    return MetaDataManager::GetInstance().SaveMeta(UserMetaRow::GetKeyFor(userMetaData.deviceId), userMetaData);
}

UserDelegate &UserDelegate::GetInstance()
{
    static UserDelegate instance;
    return instance;
}

void UserDelegate::Init(const std::shared_ptr<ExecutorPool>& executors)
{
    auto ret = AccountDelegate::GetInstance()->Subscribe(std::make_shared<LocalUserObserver>(*this));
    MetaDataManager::GetInstance().Subscribe(
        UserMetaRow::KEY_PREFIX, [this](const std::string &key, const std::string &value, int32_t flag) -> auto {
            UserMetaData metaData;
            UserMetaData::Unmarshall(value, metaData);
            ZLOGD("flag:%{public}d, value:%{public}s", flag, Anonymous::Change(metaData.deviceId).c_str());
            if (metaData.deviceId == GetLocalDeviceId()) {
                ZLOGD("ignore local device user meta change");
                return false;
            }
            if (flag == MetaDataManager::INSERT || flag == MetaDataManager::UPDATE) {
                UpdateUsers(metaData.deviceId, metaData.users);
            } else if (flag == MetaDataManager::DELETE) {
                DeleteUsers(metaData.deviceId);
            } else {
                ZLOGD("ignored operation");
            }
            return true;
    });
    if (!executors_) {
        executors_ = executors;
    }
    executors_->Execute(GeTask());
    ZLOGD("subscribe os account ret:%{public}d", ret);
}

ExecutorPool::Task UserDelegate::GeTask()
{
    return [this] {
        auto ret = InitLocalUserMeta();
        if (ret) {
            return;
        }
        executors_->Schedule(std::chrono::milliseconds(RETRY_INTERVAL), GeTask());
    };
}

bool UserDelegate::NotifyUserEvent(const UserDelegate::UserEvent &userEvent)
{
    // update all local user status
    (void) userEvent;
    return InitLocalUserMeta();
}

UserDelegate::LocalUserObserver::LocalUserObserver(UserDelegate &userDelegate) : userDelegate_(userDelegate)
{
}

void UserDelegate::LocalUserObserver::OnAccountChanged(const DistributedKv::AccountEventInfo &eventInfo)
{
    ZLOGI("event info:%{public}s, %{public}d", eventInfo.userId.c_str(), eventInfo.status);
    userDelegate_.NotifyUserEvent({}); // just notify
}

std::string UserDelegate::LocalUserObserver::Name()
{
    return "user_delegate";
}
} // namespace OHOS::DistributedData
