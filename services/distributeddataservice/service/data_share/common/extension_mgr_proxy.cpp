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
#include "extension_mgr_proxy.h"

#include "app_connect_manager.h"
#include "extension_ability_info.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "system_ability_definition.h"
#include "want.h"
namespace OHOS::DataShare {
void ExtensionMgrProxy::OnProxyDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (sa_ != nullptr) {
        sa_->RemoveDeathRecipient(deathRecipient_);
    }
    deathRecipient_ = nullptr;
    proxy_ = nullptr;
}

ExtensionMgrProxy::~ExtensionMgrProxy()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (sa_ != nullptr) {
        sa_->RemoveDeathRecipient(deathRecipient_);
    }
}

std::shared_ptr<ExtensionMgrProxy> ExtensionMgrProxy::GetInstance()
{
    static std::shared_ptr<ExtensionMgrProxy> proxy = std::make_shared<ExtensionMgrProxy>();
    return proxy;
}

int ExtensionMgrProxy::Connect(
    const std::string &uri, const sptr<IRemoteObject> &connect, const sptr<IRemoteObject> &callerToken)
{
    AAFwk::Want want;
    want.SetUri(uri);
    std::lock_guard<std::mutex> lock(mutex_);
    if (ConnectSA()) {
        int ret = proxy_->ConnectAbilityCommon(want, connect, callerToken, AppExecFwk::ExtensionAbilityType::DATASHARE);
        if (ret != ERR_OK) {
            ZLOGE("ConnectAbilityCommon failed, %{public}d", ret);
        }
        return ret;
    }
    return -1;
}

bool ExtensionMgrProxy::ConnectSA()
{
    if (proxy_ != nullptr) {
        return true;
    }
    sptr<ISystemAbilityManager> systemAbilityManager =
        SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        ZLOGE("Failed to get system ability mgr.");
        return false;
    }

    sa_ = systemAbilityManager->GetSystemAbility(ABILITY_MGR_SERVICE_ID);
    if (sa_ == nullptr) {
        ZLOGE("Failed to GetSystemAbility.");
        return false;
    }
    deathRecipient_ = new (std::nothrow) ExtensionMgrProxy::ServiceDeathRecipient(weak_from_this());
    if (deathRecipient_ == nullptr) {
        ZLOGE("deathRecipient alloc failed.");
        return false;
    }
    sa_->AddDeathRecipient(deathRecipient_);
    proxy_ = new (std::nothrow)Proxy(sa_);
    if (proxy_ == nullptr) {
        ZLOGE("proxy_ null, new failed");
        return false;
    }
    return true;
}

int ExtensionMgrProxy::DisConnect(sptr<IRemoteObject> connect)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (ConnectSA()) {
        int ret = proxy_->DisconnectAbility(connect);
        if (ret != ERR_OK) {
            ZLOGE("DisconnectAbility failed, %{public}d", ret);
        }
        return ret;
    }
    return -1;
}
} // namespace OHOS::DataShare