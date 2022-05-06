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

#include "user_change_monitor.h"

#include "db_errno.h"
#include "log_print.h"

namespace DistributedDB {
UserChangeMonitor::UserChangeMonitor()
    : userNotifier_(nullptr),
      isStarted_(false)
{
}

UserChangeMonitor::~UserChangeMonitor()
{
    Stop();
}

int UserChangeMonitor::Start()
{
    if (isStarted_) {
        return E_OK;
    }

    int errCode = PrepareNotifierChain();
    if (errCode != E_OK) {
        return errCode;
    }
    isStarted_ = true;
    return E_OK;
}

void UserChangeMonitor::Stop()
{
    if (!isStarted_) {
        return;
    }
    if (userNotifier_ != nullptr) {
        userNotifier_->UnRegisterEventType(USER_ACTIVE_EVENT);
        userNotifier_->UnRegisterEventType(USER_NON_ACTIVE_EVENT);
        userNotifier_->UnRegisterEventType(USER_ACTIVE_TO_NON_ACTIVE_EVENT);
        RefObject::KillAndDecObjRef(userNotifier_);
        userNotifier_ = nullptr;
    }
    isStarted_ = false;
}

NotificationChain::Listener *UserChangeMonitor::RegisterUserChangedListerner(const UserChangedAction &action,
    EventType event, int &errCode)
{
    std::shared_lock<std::shared_mutex> lockGuard(userChangeMonitorLock_);
    if (action == nullptr) {
        errCode = -E_INVALID_ARGS;
        return nullptr;
    }
    if (userNotifier_ == nullptr) {
        errCode = -E_NOT_INIT;
        return nullptr;
    }
    LOGI("[UserChangeMonitor] RegisterUserChangedListerner event=%d", event);
    return userNotifier_->RegisterListener(event, action, nullptr, errCode);
}

int UserChangeMonitor::PrepareNotifierChain()
{
    int errCode = E_OK;
    std::unique_lock<std::shared_mutex> lockGuard(userChangeMonitorLock_);
    if (userNotifier_ != nullptr) {
        return E_OK;
    }
    userNotifier_ = new (std::nothrow) NotificationChain();
    if (userNotifier_ == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    errCode = userNotifier_->RegisterEventType(USER_ACTIVE_EVENT);
    if (errCode != E_OK) {
        goto ERROR_HANDLE;
    }
    errCode = userNotifier_->RegisterEventType(USER_NON_ACTIVE_EVENT);
    if (errCode != E_OK) {
        userNotifier_->UnRegisterEventType(USER_ACTIVE_EVENT);
        goto ERROR_HANDLE;
    }
    errCode = userNotifier_->RegisterEventType(USER_ACTIVE_TO_NON_ACTIVE_EVENT);
    if (errCode != E_OK) {
        userNotifier_->UnRegisterEventType(USER_ACTIVE_EVENT);
        userNotifier_->UnRegisterEventType(USER_NON_ACTIVE_EVENT);
        goto ERROR_HANDLE;
    }
    return errCode;
ERROR_HANDLE:
    RefObject::KillAndDecObjRef(userNotifier_);
    userNotifier_ = nullptr;
    return errCode;
}

void UserChangeMonitor::NotifyUserChanged() const
{
    std::shared_lock<std::shared_mutex> lockGuard(userChangeMonitorLock_);
    if (userNotifier_ == nullptr) {
        LOGD("NotifyUNotifyUserChangedserChange fail, userChangedNotifier is null.");
        return;
    }
    LOGI("[UserChangeMonitor] begin to notify event");
    userNotifier_->NotifyEvent(USER_ACTIVE_EVENT, nullptr);
    userNotifier_->NotifyEvent(USER_NON_ACTIVE_EVENT, nullptr);
    userNotifier_->NotifyEvent(USER_ACTIVE_TO_NON_ACTIVE_EVENT, nullptr);
}
} // namespace DistributedDB