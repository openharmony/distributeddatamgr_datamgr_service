/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#define LOG_TAG "AppConnectManager"

#include "app_connect_manager.h"

#include <chrono>

#include "datashare_errno.h"
#include "extension_mgr_proxy.h"
#include "log_print.h"

namespace OHOS::DataShare {
ConcurrentMap<std::string, BlockData<bool> *> AppConnectManager::blockCache_;
ConcurrentMap<std::string, sptr<AAFwk::IAbilityConnection>> AppConnectManager::callbackCache_;
std::shared_ptr<ExecutorPool> AppConnectManager::executor_ = std::make_shared<ExecutorPool>(AppConnectManager::MAX_THREADS,
    AppConnectManager::MIN_THREADS);
bool AppConnectManager::Wait(const std::string &bundleName,
    int maxWaitTime, std::function<bool()> connect, std::function<void()> disconnect)
{
    BlockData<bool> block(maxWaitTime, false);
    auto result = blockCache_.ComputeIfAbsent(bundleName, [&block](const std::string &key) {
        return &block;
    });
    if (!result) {
        ZLOGE("is running %{public}s", bundleName.c_str());
        return false;
    }
    ZLOGI("start connect %{public}s", bundleName.c_str());
    result = connect();
    if (!result) {
        ZLOGE("connect failed %{public}s", bundleName.c_str());
        blockCache_.Erase(bundleName);
        return false;
    }
    result = block.GetValue();
    ZLOGI("end wait %{public}s", bundleName.c_str());
    blockCache_.Erase(bundleName);
    disconnect();
    return result;
}

void AppConnectManager::Notify(const std::string &bundleName)
{
    ZLOGI("notify %{public}s", bundleName.c_str());
    blockCache_.ComputeIfPresent(bundleName, [&bundleName](const std::string &key, BlockData<bool> *value) {
        if (value == nullptr) {
            ZLOGI("nullptr %{public}s", bundleName.c_str());
            return false;
        }
        value->SetValue(true);
        return true;
    });
    DelayDisconnect(bundleName);
}

void AppConnectManager::SetCallback(const std::string &bundleName, sptr<AAFwk::IAbilityConnection> &callback)
{
    callbackCache_.ComputeIfAbsent(bundleName, [callback](const std::string &) {
        return std::move(callback);
    });
    return ;
}

void AppConnectManager::DelayDisconnect(const std::string &bundleName)
{
    executor_->Schedule(std::chrono::seconds(WAIT_DISCONNECT_TIME), [bundleName]() {
        ZLOGI("delay disconnect %{public}s", bundleName.c_str());
        callbackCache_.ComputeIfPresent(bundleName, [bundleName](const std::string &,
            sptr<AAFwk::IAbilityConnection> &disconnect) {
            if (disconnect == nullptr) {
                ZLOGI("Disconnect nullptr %{public}s", bundleName.c_str());
                return false;
            }
            auto ret = ExtensionMgrProxy::GetInstance()->DisConnect(disconnect->AsObject());
            if (ret != E_OK) {
                ZLOGE("DisConnect failed bundleName: %{public}s, ret: %{public}d", bundleName.c_str(), ret);
            }
            return false;
        });
    });
}
} // namespace OHOS::DataShare