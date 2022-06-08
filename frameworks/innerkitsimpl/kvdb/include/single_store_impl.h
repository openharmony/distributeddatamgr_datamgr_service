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
#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SINGLE_STORE_IMPL_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SINGLE_STORE_IMPL_H
#include <functional>
#include <shared_mutex>

#include "concurrent_map.h"
#include "kv_store_nb_delegate.h"
#include "kvdb_service.h"
#include "observer_bridge.h"
#include "single_kvstore.h"
#include "sync_observer.h"
namespace OHOS::DistributedKv {
class API_EXPORT SingleStoreImpl : public SingleKvStore {
public:
    using Observer = KvStoreObserver;
    using SyncCallback = KvStoreSyncCallback;
    using ResultSet = KvStoreResultSet;
    using DBStore = DistributedDB::KvStoreNbDelegate;
    using DBEntry = DistributedDB::Entry;
    using DBKey = DistributedDB::Key;
    using DBValue = DistributedDB::Value;
    using DBQuery = DistributedDB::Query;
    using SyncInfo = KVDBService::SyncInfo;
    using Convert = std::function<Key(const DBKey &key, std::string &deviceId)>;
    SingleStoreImpl(std::shared_ptr<DBStore> dbStore, const AppId &appId, const Options &options);
    ~SingleStoreImpl() = default;
    StoreId GetStoreId() const override;
    Status Put(const Key &key, const Value &value) override;
    Status PutBatch(const std::vector<Entry> &entries) override;
    Status Delete(const Key &key) override;
    Status DeleteBatch(const std::vector<Key> &keys) override;
    Status StartTransaction() override;
    Status Commit() override;
    Status Rollback() override;
    Status SubscribeKvStore(SubscribeType type, std::shared_ptr<Observer> observer) override;
    Status UnSubscribeKvStore(SubscribeType type, std::shared_ptr<Observer> observer) override;
    Status Get(const Key &key, Value &value) override;
    Status GetEntries(const Key &prefix, std::vector<Entry> &entries) const override;
    Status GetEntries(const DataQuery &query, std::vector<Entry> &entries) const override;
    Status GetResultSet(const Key &prefix, std::shared_ptr<ResultSet> &resultSet) const override;
    Status GetResultSet(const DataQuery &query, std::shared_ptr<ResultSet> &resultSet) const override;
    Status CloseResultSet(std::shared_ptr<ResultSet> &resultSet) override;
    Status GetCount(const DataQuery &query, int &count) const override;
    Status GetSecurityLevel(SecurityLevel &secLevel) const override;
    Status RemoveDeviceData(const std::string &device) override;
    Status Close();

    // IPC interface
    Status Sync(const std::vector<std::string> &devices, SyncMode mode, uint32_t delay) override;
    Status Sync(const std::vector<std::string> &devices, SyncMode mode, const DataQuery &query,
        std::shared_ptr<SyncCallback> syncCallback) override;
    Status RegisterSyncCallback(std::shared_ptr<SyncCallback> callback) override;
    Status UnRegisterSyncCallback() override;
    Status SetSyncParam(const KvSyncParam &syncParam) override;
    Status GetSyncParam(KvSyncParam &syncParam) override;
    Status SetCapabilityEnabled(bool enabled) const override;
    Status SetCapabilityRange(
        const std::vector<std::string> &localLabels, const std::vector<std::string> &remoteLabels) const override;
    Status SubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query) override;
    Status UnsubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query) override;

protected:
    static constexpr size_t MAX_KEY_LENGTH = 1024;
    virtual std::vector<uint8_t> ToLocalDBKey(const Key &key) const;
    virtual std::vector<uint8_t> ToWholeDBKey(const Key &key) const;
    virtual Key ToKey(DBKey &&key) const;
    virtual std::vector<uint8_t> GetPrefix(const Key &prefix) const;
    virtual std::vector<uint8_t> GetPrefix(const DataQuery &query) const;
    virtual Convert GetConvert() const;

private:
    Status GetResultSet(const DistributedDB::Query &query, std::shared_ptr<ResultSet> &resultSet) const;
    Status GetEntries(const DistributedDB::Query &query, std::vector<Entry> &entries) const;
    Status DoSync(const SyncInfo &syncInfo, std::shared_ptr<SyncCallback> observer);
    void DoAutoSync();
    std::function<void(ObserverBridge *)> BridgeReleaser();

    std::shared_ptr<DBStore> dbStore_ = nullptr;
    std::string appId_;
    std::string storeId_;
    bool autoSync_ = false;
    mutable std::shared_mutex rwMutex_;
    std::shared_ptr<SyncObserver> syncObserver_ = nullptr;
    ConcurrentMap<uintptr_t, std::pair<uint32_t, std::shared_ptr<ObserverBridge>>> observers_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SINGLE_STORE_IMPL_H
