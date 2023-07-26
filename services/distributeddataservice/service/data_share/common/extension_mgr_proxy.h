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

#ifndef DATASHARESERVICE_EXTENSION_PROXY_H
#define DATASHARESERVICE_EXTENSION_PROXY_H

#include <memory>
#include <string>

#include "extension_manager_proxy.h"
namespace OHOS::DataShare {
class ExtensionMgrProxy final : public std::enable_shared_from_this<ExtensionMgrProxy> {
public:
    // do not use
    ExtensionMgrProxy() = default;
    ~ExtensionMgrProxy();
    static std::shared_ptr<ExtensionMgrProxy> GetInstance();
    int Connect(const std::string &uri, const sptr<IRemoteObject> &connect, const sptr<IRemoteObject> &callerToken);
    int DisConnect(sptr<IRemoteObject> connect);
private:
    using Proxy = AAFwk::ExtensionManagerProxy;
    class ServiceDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit ServiceDeathRecipient(std::weak_ptr<ExtensionMgrProxy> owner) : owner_(owner)
        {
        }
        void OnRemoteDied(const wptr<IRemoteObject> &object) override
        {
            auto owner = owner_.lock();
            if (owner != nullptr) {
                owner->OnProxyDied();
            }
        }

    private:
        std::weak_ptr<ExtensionMgrProxy> owner_;
    };
    void OnProxyDied();
    bool ConnectSA();
    std::mutex mutex_;
    sptr<IRemoteObject> sa_;
    sptr<Proxy> proxy_;
    sptr<ExtensionMgrProxy::ServiceDeathRecipient> deathRecipient_;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_BUNDLEMGR_PROXY_H
