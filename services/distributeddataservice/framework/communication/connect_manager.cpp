/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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

#define LOG_TAG "ConnectManager"
#include "communication/connect_manager.h"

#include "log_print.h"

namespace OHOS::AppDistributedKv {
std::mutex ConnectManager::mtx_;
std::shared_ptr<ConnectManager> ConnectManager::instance_ = nullptr;
ConnectManager::CloseSessionTask ConnectManager::closeSessionTask_ = nullptr;
ConcurrentMap<std::string, ConnectManager::SessionCloseListener> ConnectManager::sessionCloseListener_;
ConcurrentMap<std::string, ConnectManager::SessionOpenListener> ConnectManager::sessionOpenListener_;

std::shared_ptr<ConnectManager> ConnectManager::GetInstance()
{
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&]() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (instance_ == nullptr) {
            instance_ = std::make_shared<ConnectManager>();
        }
    });
    return instance_;
}

bool ConnectManager::RegisterInstance(std::shared_ptr<ConnectManager> instance)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (instance_ != nullptr) {
        ZLOGW("ConnectManager instance has been replaced!");
    }
    instance_ = instance;
    return true;
}

bool ConnectManager::CloseSession(const std::string &networkId)
{
    if (closeSessionTask_ != nullptr) {
        return closeSessionTask_(networkId);
    }
    return false;
}

bool ConnectManager::RegisterCloseSessionTask(CloseSessionTask task)
{
    if (closeSessionTask_ != nullptr) {
        ZLOGE("Register close session task error, task already exists.");
        return false;
    }
    closeSessionTask_ = std::move(task);
    return true;
}

bool ConnectManager::RegisterSessionCloseListener(const std::string &name, SessionCloseListener listener)
{
    bool success = false;
    sessionCloseListener_.Compute(name, [&success, &listener](const auto &key, auto &value) {
        if (value != nullptr) {
            ZLOGE("Register session close listener error, type:%{public}s already exists.", key.c_str());
            return true;
        }
        value = std::move(listener);
        success = true;
        return true;
    });
    return success;
}

void ConnectManager::UnRegisterSessionCloseListener(const std::string &name)
{
    sessionCloseListener_.Erase(name);
}

void ConnectManager::OnSessionClose(const std::string &networkId)
{
    sessionCloseListener_.ForEach([&networkId](const auto &key, auto &listener) {
        listener(networkId);
        return false;
    });
}

bool ConnectManager::RegisterSessionOpenListener(const std::string &name, SessionOpenListener listener)
{
    bool success = false;
    sessionOpenListener_.Compute(name, [&success, &listener](const auto &key, auto &value) {
        if (value != nullptr) {
            ZLOGE("Register session open listener error, type:%{public}s already exists.", key.c_str());
            return true;
        }
        value = std::move(listener);
        success = true;
        return true;
    });
    return success;
}

void ConnectManager::UnRegisterSessionOpenListener(const std::string &name)
{
    sessionOpenListener_.Erase(name);
}

void ConnectManager::OnSessionOpen(const std::string &networkId)
{
    sessionOpenListener_.ForEach([&networkId](const auto &key, auto &listener) {
        listener(networkId);
        return false;
    });
}

void ConnectManager::OnStart()
{
}

void ConnectManager::OnDestory()
{
}

int32_t ConnectManager::ApplyConnect(__attribute__((unused)) const std::string &networkId, ConnectTask task)
{
    task();
    return 0;
}
} // OHOS::AppDistributedKv