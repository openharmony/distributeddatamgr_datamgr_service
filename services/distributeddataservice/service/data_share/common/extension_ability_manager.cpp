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
#define LOG_TAG "ExtensionAbilityManager"

#include "extension_ability_manager.h"

#include "extension_mgr_proxy.h"
#include "log_print.h"
#include "datashare_errno.h"
#include "uri_utils.h"

namespace OHOS::DataShare {
ExtensionAbilityManager &ExtensionAbilityManager::GetInstance()
{
    static ExtensionAbilityManager instance;
    return instance;
}

void ExtensionAbilityManager::SetExecutorPool(std::shared_ptr<ExecutorPool> executor)
{
    executor_ = executor;
}

int32_t ExtensionAbilityManager::ConnectExtension(const std::string &uri, const std::string &bundleName,
    const sptr<IRemoteObject> &callback)
{
    auto absent = connectCallbackCache_.ComputeIfAbsent(bundleName, [callback](const std::string &) {
        return std::move(callback);
    });
    if (!absent) {
        ZLOGE("Extension is running bundleName:%{public}s, uri:%{public}s",
            bundleName.c_str(), URIUtils::Anonymous(uri).c_str());
        return E_ERROR;
    }
    ErrCode ret = ExtensionMgrProxy::GetInstance()->Connect(uri, callback, nullptr);
    if (ret != E_OK) {
        ZLOGE("Connect ability failed, ret:%{public}d, uri:%{public}s, bundleName:%{public}s",
            ret, URIUtils::Anonymous(uri).c_str(), bundleName.c_str());
        return E_ERROR;
    }
    return E_OK;
}

void ExtensionAbilityManager::DelayDisconnect(const std::string &bundleName)
{
    executor_->Schedule(std::chrono::seconds(WAIT_DISCONNECT_TIME), [bundleName, this]() {
        ZLOGI("Delay disconnect %{public}s", bundleName.c_str());
        connectCallbackCache_.ComputeIfPresent(bundleName, [bundleName](const std::string &,
            sptr<IRemoteObject> &disconnect) {
            if (disconnect == nullptr) {
                ZLOGI("Delay disconnect nullptr %{public}s", bundleName.c_str());
                return false;
            }
            auto ret = ExtensionMgrProxy::GetInstance()->DisConnect(disconnect);
            if (ret != E_OK) {
                ZLOGE("Delay disConnect failed bundleName: %{public}s, ret: %{public}d", bundleName.c_str(), ret);
            }
            return false;
        });
    });
}
} // namespace OHOS::DataShare

