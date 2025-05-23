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

#ifndef UDMF_NOTIFIER_PROXY_H
#define UDMF_NOTIFIER_PROXY_H

#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iudmf_notifier.h"

namespace OHOS {
namespace UDMF {
class UdmfNotifierProxy : public IRemoteProxy<IUdmfNotifier> {
public:
    explicit UdmfNotifierProxy(const sptr<IRemoteObject> &impl);
    ~UdmfNotifierProxy() = default;
    void HandleDelayObserver(const std::string &key, const DataLoadInfo &dataLoadInfo) override;

private:
    static inline BrokerDelegator<UdmfNotifierProxy> delegator_;
    sptr<IRemoteObject> remote_;
};

class DelayDataCallbackProxy : public IRemoteProxy<IDelayDataCallback> {
public:
    explicit DelayDataCallbackProxy(const sptr<IRemoteObject> &impl);
    ~DelayDataCallbackProxy() = default;
    void DelayDataCallback(const std::string &key, const UnifiedData &data) override;

private:
    static inline BrokerDelegator<DelayDataCallbackProxy> delegator_;
    sptr<IRemoteObject> remote_;
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_NOTIFIER_PROXY_H