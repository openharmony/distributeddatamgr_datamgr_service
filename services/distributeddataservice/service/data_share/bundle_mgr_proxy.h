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

#ifndef DATASHARESERVICE_BUNDLEMGR_PROXY_H
#define DATASHARESERVICE_BUNDLEMGR_PROXY_H

#include <string>

#include "bundlemgr/bundle_mgr_proxy.h"
namespace OHOS::DataShare {
class BundleMgrProxy {
public:
    bool GetBundleInfoFromBMS(const std::string &bundleName, uint32_t tokenId, AppExecFwk::BundleInfo &bundleInfo);

private:
    class ServiceDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit ServiceDeathRecipient(BundleMgrProxy* owner) : owner_(owner) {}
        void OnRemoteDied(const wptr<IRemoteObject> &object) override
        {
            if (owner_ != nullptr) {
                owner_->OnProxyDied();
            }
        }
    private:
        BundleMgrProxy* owner_;
    };
    sptr<AppExecFwk::BundleMgrProxy> GetBundleMgrProxy();
    void OnProxyDied();
    std::mutex mutex_;
    sptr<AppExecFwk::BundleMgrProxy> proxy_;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_BUNDLEMGR_PROXY_H
