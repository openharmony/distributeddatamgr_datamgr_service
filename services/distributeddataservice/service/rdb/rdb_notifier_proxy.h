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

#ifndef OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_INNERKITSIMPL_RDB_RDB_NOTIFIER_PROXY_H
#define OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_INNERKITSIMPL_RDB_RDB_NOTIFIER_PROXY_H
#include <iremote_broker.h>
#include <iremote_proxy.h>
#include "rdb_notifier.h"
namespace OHOS::DistributedRdb {
class RdbNotifierProxyBroker : public IRdbNotifier, public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedRdb.IRdbNotifier");
};

class RdbNotifierProxy : public IRemoteProxy<RdbNotifierProxyBroker> {
public:
    explicit RdbNotifierProxy(const sptr<IRemoteObject>& object);
    virtual ~RdbNotifierProxy() noexcept;

    int32_t OnComplete(uint32_t seqNum, Details &&result) override;

    int32_t OnComplete(const std::string& storeName, Details &&result) override;

    int32_t OnChange(const Origin &origin, const PrimaryFields &primaries, ChangeInfo &&changeInfo) override;

private:
    static inline BrokerDelegator<RdbNotifierProxy> delegator_;
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_INNERKITSIMPL_RDB_RDB_NOTIFIER_PROXY_H
