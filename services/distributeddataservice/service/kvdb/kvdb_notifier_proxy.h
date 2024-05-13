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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_NOTIFIER_PROXY_H
#define OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_NOTIFIER_PROXY_H

#include "ikvdb_notifier.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace DistributedKv {
class API_EXPORT KVDBNotifierProxy : public IRemoteProxy<IKVDBNotifier> {
public:
    explicit KVDBNotifierProxy(const sptr<IRemoteObject> &impl);
    ~KVDBNotifierProxy() = default;
    void SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId) override;
    void SyncCompleted(uint64_t seqNum, ProgressDetail &&detail) override;
    void OnRemoteChange(const std::map<std::string, bool> &mask) override;
    void OnSwitchChange(const SwitchNotification &notification) override;

private:
    static inline BrokerDelegator<KVDBNotifierProxy> delegator_;
    sptr<IRemoteObject> remote_;
};
} // namespace DistributedKv
} // namespace OHOS

#endif // OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_NOTIFIER_PROXY_H