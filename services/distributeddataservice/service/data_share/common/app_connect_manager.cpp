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

#include "extension_ability_manager.h"
#include "log_print.h"

namespace OHOS::DataShare {
ConcurrentMap<std::string, BlockData<bool> *> AppConnectManager::blockCache_;
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
    ExtensionAbilityManager::GetInstance().DelayDisconnect(bundleName);
}
} // namespace OHOS::DataShare