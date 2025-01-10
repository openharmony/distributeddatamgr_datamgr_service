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

#ifndef KVDB_OBSERVER_PROXY_H
#define KVDB_OBSERVER_PROXY_H

#include "change_notification.h"
#include "iremote_broker.h"
#include "ikvstore_observer.h"
#include "iremote_proxy.h"
#include "types.h"

namespace OHOS {
namespace DistributedKv {
class API_EXPORT KVDBObserverProxy : public IRemoteProxy<IKvStoreObserver> {
public:
    explicit KVDBObserverProxy(const sptr<IRemoteObject> &impl);
    ~KVDBObserverProxy() = default;
    void OnChange(const ChangeNotification &changeNotification) override;
    void OnChange(const DataOrigin &origin, Keys &&keys) override;
private:
    static inline BrokerDelegator<KVDBObserverProxy> delegator_;
};
}  // namespace DistributedKv
}  // namespace OHOS

#endif  // KVDB_OBSERVER_PROXY_H