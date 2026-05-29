/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_ADAPTER_BMS_DELEGATE_IMPL_H
#define OHOS_DISTRIBUTED_DATA_ADAPTER_BMS_DELEGATE_IMPL_H

#include <cstdint>
#include <mutex>
#include <string>

#include "bms/bms_delegate.h"
#include "iremote_object.h"

namespace OHOS::AppExecFwk {
class IBundleMgr;
} // namespace OHOS::AppExecFwk

namespace OHOS::DistributedData {
class BmsDelegateImpl final : public BmsDelegate {
public:
    ~BmsDelegateImpl() override;
    std::string GetCallerAppIdentifier(const std::string &bundleName, int32_t userId) override;

private:
    class ServiceDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit ServiceDeathRecipient(BmsDelegateImpl *owner) : owner_(owner)
        {
        }

        void OnRemoteDied(const wptr<IRemoteObject> &object) override
        {
            if (owner_ != nullptr) {
                owner_->OnRemoteDied();
            }
        }

    private:
        BmsDelegateImpl *owner_;
    };

    void OnRemoteDied();
    sptr<AppExecFwk::IBundleMgr> GetBundleMgr();

    std::mutex mutex_;
    sptr<IRemoteObject> object_;
    sptr<BmsDelegateImpl::ServiceDeathRecipient> deathRecipient_;
};
} // namespace OHOS::DistributedData

#endif // OHOS_DISTRIBUTED_DATA_ADAPTER_BMS_DELEGATE_IMPL_H
