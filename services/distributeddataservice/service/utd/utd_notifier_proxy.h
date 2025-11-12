/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef UTD_NOTIFIER_PROXY_H
#define UTD_NOTIFIER_PROXY_H

#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "utd_notifier.h"

namespace OHOS {
namespace UDMF {
class UtdNotifierProxy : public IRemoteProxy<IUtdNotifier> {
public:
    explicit UtdNotifierProxy(const sptr<IRemoteObject> &impl);
    ~UtdNotifierProxy() = default;
    int32_t RegisterDynamicUtd(const std::vector<TypeDescriptorCfg> &descriptors,
        const std::string &bundleName) override;
    int32_t UnregisterDynamicUtd(const std::vector<std::string> &typeIds, const std::string &bundleName) override;
    int32_t DynamicUtdChange() override;

private:
    static inline BrokerDelegator<UtdNotifierProxy> delegator_;
    sptr<IRemoteObject> remote_;
};
} // namespace UDMF
} // namespace OHOS
#endif // UTD_NOTIFIER_PROXY_H