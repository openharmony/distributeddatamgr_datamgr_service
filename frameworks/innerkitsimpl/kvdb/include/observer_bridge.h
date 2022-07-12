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
#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_OBSERVER_BRIDGE_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_OBSERVER_BRIDGE_H
#include "convertor.h"
#include "kv_store_nb_delegate.h"
#include "kv_store_observer.h"
#include "kvstore_observer.h"
#include "kvstore_observer_client.h"
namespace OHOS::DistributedKv {
class IKvStoreObserver;
class ObserverBridge : public DistributedDB::KvStoreObserver {
public:
    using Observer = DistributedKv::KvStoreObserver;
    using DBEntry = DistributedDB::Entry;
    using DBKey = DistributedDB::Key;
    using DBChangedData = DistributedDB::KvStoreChangedData;

    ObserverBridge(AppId appId, StoreId storeId, std::shared_ptr<Observer> observer, const Convertor &cvt);
    ~ObserverBridge();
    Status RegisterRemoteObserver();
    Status UnregisterRemoteObserver();
    void OnChange(const DBChangedData &data) override;

private:
    class ObserverClient : public KvStoreObserverClient {
    public:
        ObserverClient(std::shared_ptr<Observer> observer, const Convertor &cvt);
        void OnChange(const ChangeNotification &data) override;

    private:
        const Convertor &convert_;
    };

    template<class T>
    static std::vector<Entry> ConvertDB(const T &dbEntries, std::string &deviceId, const Convertor &convert);
    AppId appId_;
    StoreId storeId_;
    std::shared_ptr<DistributedKv::KvStoreObserver> observer_;
    sptr<IKvStoreObserver> remote_;
    const Convertor &convert_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_OBSERVER_BRIDGE_H
