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

#ifndef DATASHARESERVICE_BUNDLEMGR_PROXY_H
#define DATASHARESERVICE_BUNDLEMGR_PROXY_H

#include <memory>
#include <string>

#include "bundle_info.h"
#include "bundlemgr/bundle_mgr_proxy.h"
#include "concurrent_map.h"
namespace OHOS::DataShare {
class BundleMgrProxy final : public std::enable_shared_from_this<BundleMgrProxy> {
public:
    ~BundleMgrProxy();
    static std::shared_ptr<BundleMgrProxy> GetInstance();
    bool GetBundleInfoFromBMS(const std::string &bundleName, int32_t userId, AppExecFwk::BundleInfo &bundleInfo);
    void Delete(const std::string &bundleName, int32_t userId);

private:
    BundleMgrProxy() = default;
    class ServiceDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit ServiceDeathRecipient(std::weak_ptr<BundleMgrProxy> owner) : owner_(owner) {}
        void OnRemoteDied(const wptr<IRemoteObject> &object) override
        {
            auto owner = owner_.lock();
            if (owner != nullptr) {
                owner->OnProxyDied();
            }
        }

    private:
        std::weak_ptr<BundleMgrProxy> owner_;
    };
    sptr<AppExecFwk::BundleMgrProxy> GetBundleMgrProxy();
    void OnProxyDied();
    std::mutex mutex_;
    sptr<IRemoteObject> proxy_;
    sptr<BundleMgrProxy::ServiceDeathRecipient> deathRecipient_;
    ConcurrentMap<std::string, AppExecFwk::BundleInfo> bundleCache_;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_BUNDLEMGR_PROXY_H
