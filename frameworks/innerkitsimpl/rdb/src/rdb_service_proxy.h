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

#ifndef DISTRIBUTED_RDB_SERVICE_PROXY_H
#define DISTRIBUTED_RDB_SERVICE_PROXY_H

#include <atomic>
#include <list>
#include <iremote_proxy.h>
#include "irdb_service.h"
#include "rdb_notifier.h"
#include "concurrent_map.h"

namespace OHOS::DistributedRdb {
class RdbServiceProxy : public IRemoteProxy<IRdbService> {
public:
    using ObserverMapValue = std::pair<std::list<RdbStoreObserver*>, RdbSyncerParam>;
    using ObserverMap = ConcurrentMap<std::string, ObserverMapValue>;

    explicit RdbServiceProxy(const sptr<IRemoteObject>& object);

    std::string ObtainDistributedTableName(const std::string& device, const std::string& table) override;

    std::vector<std::string> GetConnectDevices() override;

    int32_t InitNotifier(const RdbSyncerParam& param);
    int32_t InitNotifier(const RdbSyncerParam& param, const sptr<IRemoteObject> notifier) override;
    
    int32_t SetDistributedTables(const RdbSyncerParam& param, const std::vector<std::string>& tables) override;
    
    int32_t Sync(const RdbSyncerParam& param, const SyncOption& option,
                 const RdbPredicates& predicates, const SyncCallback& callback) override;

    int32_t Subscribe(const RdbSyncerParam& param, const SubscribeOption& option,
                      RdbStoreObserver *observer) override;

    int32_t UnSubscribe(const RdbSyncerParam& param, const SubscribeOption& option,
                        RdbStoreObserver *observer) override;

    ObserverMap ExportObservers();

    void ImportObservers(ObserverMap& observers);

protected:
    int32_t DoSync(const RdbSyncerParam& param, const SyncOption& option,
                   const RdbPredicates& predicates, SyncResult& result) override;

    int32_t DoAsync(const RdbSyncerParam& param, uint32_t seqNum, const SyncOption& option,
                    const RdbPredicates& predicates) override;

    int32_t DoSubscribe(const RdbSyncerParam& param) override;

    int32_t DoUnSubscribe(const RdbSyncerParam& param) override;

private:
    uint32_t GetSeqNum();
    
    int32_t DoSync(const RdbSyncerParam& param, const SyncOption& option,
                   const RdbPredicates& predicates, const SyncCallback& callback);

    int32_t DoAsync(const RdbSyncerParam& param, const SyncOption& option,
                    const RdbPredicates& predicates, const SyncCallback& callback);

    void OnSyncComplete(uint32_t seqNum, const SyncResult& result);
    
    void OnDataChange(const std::string& storeName, const std::vector<std::string>& devices);
    
    std::atomic<uint32_t> seqNum_ {};

    ConcurrentMap<uint32_t, SyncCallback> syncCallbacks_;
    ObserverMap observers_;
    sptr<RdbNotifierStub> notifier_;

    static inline BrokerDelegator<RdbServiceProxy> delegator_;
};
}
#endif
