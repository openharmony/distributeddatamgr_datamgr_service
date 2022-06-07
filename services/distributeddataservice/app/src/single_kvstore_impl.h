/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef SINGLE_KVSTORE_IMPL_H
#define SINGLE_KVSTORE_IMPL_H

#include <mutex>
#include <set>
#include <memory>
#include <shared_mutex>
#include "flowctrl_manager/kvstore_flowctrl_manager.h"
#include "ikvstore_observer.h"
#include "ikvstore_single.h"
#include "ikvstore_sync_callback.h"
#include "kv_store_delegate_manager.h"
#include "kv_store_nb_delegate.h"
#include "kvstore_observer_impl.h"
#include "kvstore_resultset_impl.h"
#include "kvstore_sync_manager.h"
#include "inner_types.h"

namespace OHOS::DistributedKv {
class SingleKvStoreImpl : public SingleKvStoreStub {
public:
    SingleKvStoreImpl(const Options &options, const std::string &userId, const std::string &bundleName,
        const std::string &storeId, const std::string &appId, const std::string &directory,
        DistributedDB::KvStoreNbDelegate *delegate);
    ~SingleKvStoreImpl();
    std::string GetStoreId();
    Status Put(const Key &key, const Value &value) override;
    Status Delete(const Key &key) override;
    Status Get(const Key &key, Value &value) override;
    Status SubscribeKvStore(const SubscribeType subscribeType, sptr<IKvStoreObserver> observer) override;
    Status UnSubscribeKvStore(const SubscribeType subscribeType, sptr<IKvStoreObserver> observer) override;
    Status GetEntries(const Key &prefixKey, std::vector<Entry> &entries) override;
    Status GetEntriesWithQuery(const std::string &query, std::vector<Entry> &entries) override;
    void GetResultSet(const Key &prefixKey, std::function<void(Status, sptr<IKvStoreResultSet>)> callback) override;
    void GetResultSetWithQuery(const std::string &query,
                               std::function<void(Status, sptr<IKvStoreResultSet>)> callback) override;
    Status GetCountWithQuery(const std::string &query, int &result) override;
    Status CloseResultSet(sptr<IKvStoreResultSet> resultSet) override;
    Status Sync(const std::vector<std::string> &deviceIds, SyncMode mode, uint32_t allowedDelayMs,
                uint64_t sequenceId) override;
    Status Sync(const std::vector<std::string> &deviceIds, SyncMode mode,
                const std::string &query, uint64_t sequenceId) override;
    Status RemoveDeviceData(const std::string &device) override;
    Status RegisterSyncCallback(sptr<IKvStoreSyncCallback> callback) override;
    Status UnRegisterSyncCallback() override;
    Status ReKey(const std::vector<uint8_t> &key);
    InnerStatus Close(DistributedDB::KvStoreDelegateManager *kvStoreDelegateManager);
    Status ForceClose(DistributedDB::KvStoreDelegateManager *kvStoreDelegateManager);
    void IncreaseOpenCount();
    Status PutBatch(const std::vector<Entry> &entries) override;
    Status DeleteBatch(const std::vector<Key> &keys) override;
    Status StartTransaction() override;
    Status Commit() override;
    Status Rollback() override;
    Status Control(KvControlCmd cmd, const KvParam &inputParam, sptr<KvParam> &output) override;
    Status SetCapabilityEnabled(bool enabled) override;
    Status SetCapabilityRange(const std::vector<std::string> &localLabels,
                              const std::vector<std::string> &remoteSupportLabels) override;
    Status GetSecurityLevel(SecurityLevel &securityLevel) override;
    bool Import(const std::string &bundleName) const;
    void OnDump(int fd) const;
    void DumpStoreName(int fd) const;
    void SetCompatibleIdentify(const std::string &changedDevice);
    void SetCompatibleIdentify();
    std::string GetStorePath() const;

protected:
    virtual KvStoreObserverImpl *CreateObserver(const SubscribeType subscribeType, sptr<IKvStoreObserver> observer);
    virtual KvStoreResultSetImpl *CreateResultSet(
        DistributedDB::KvStoreResultSet *resultSet, const DistributedDB::Key &prix);

private:
    Status CheckDbIsCorrupted(DistributedDB::DBStatus dbStatus, const char* funName);
    Status ConvertDbStatus(DistributedDB::DBStatus dbStatus);
    uint32_t GetSyncDelayTime(uint32_t allowedDelayMs) const;
    Status AddSync(const std::vector<std::string> &deviceIds, SyncMode mode, uint32_t delayMs,
                   uint64_t sequenceId);
    Status AddSync(const std::vector<std::string> &deviceIds, SyncMode mode,
                   const std::string &query, uint32_t delayMs, uint64_t sequenceId);
    Status RemoveAllSyncOperation();
    void DoSyncComplete(const std::map<std::string, DistributedDB::DBStatus> &devicesSyncResult,
                        const std::string &query, uint64_t sequenceId);
    Status DoSync(const std::vector<std::string> &deviceIds, SyncMode mode, const KvStoreSyncManager::SyncEnd &syncEnd,
                  uint64_t sequenceId);
    Status DoQuerySync(const std::vector<std::string> &deviceIds, SyncMode mode, const std::string &query,
                       const KvStoreSyncManager::SyncEnd &syncEnd, uint64_t sequenceId);
    int ConvertToDbObserverMode(SubscribeType subscribeType) const;
    DistributedDB::SyncMode ConvertToDbSyncMode(SyncMode syncMode) const;
    Status DoSubscribe(const std::vector<std::string> &deviceIds,
                       const std::string &query, const KvStoreSyncManager::SyncEnd &syncEnd);
    Status AddSubscribe(const std::vector<std::string> &deviceIds,
                        const std::string &query, uint32_t delayMs, uint64_t sequenceId);
    Status Subscribe(const std::vector<std::string> &deviceIds,
                     const std::string &query, uint64_t sequenceId) override;
    Status DoUnSubscribe(const std::vector<std::string> &deviceIds,
                         const std::string &query, const KvStoreSyncManager::SyncEnd &syncEnd);
    Status AddUnSubscribe(const std::vector<std::string> &deviceIds,
                          const std::string &query, uint32_t delayMs, uint64_t sequenceId);
    Status UnSubscribe(const std::vector<std::string> &deviceIds,
                       const std::string &query, uint64_t sequenceId) override;
    std::vector<std::string> MapNodeIdToUuids(const std::vector<std::string> &deviceIds);

    // kvstore options.
    const Options options_;
    // deviceAccount id get from service
    std::string deviceAccountId_;
    // appId get from PMS.
    const std::string bundleName_;
    // kvstore name.
    const std::string storeId_;

    const std::string appId_;

    // kvstore absolute path in distributeddatamgr.
    const std::string storePath_;
    // for top-app, 0 means synchronization immediately. for others, 0 means 1000ms.
    uint32_t defaultSyncDelayMs_{ 0 };
    std::atomic_uint32_t waitingSyncCount_{ 0 };
    std::atomic_uint32_t syncRetries_{ 0 };
    std::vector<std::string> lastSyncDeviceIds_{ };
    SyncMode lastSyncMode_{ SyncMode::PULL };
    uint32_t lastSyncDelayMs_{ 0 };

    // distributeddb is responsible for free kvStoreNbDelegate_,
    // (destruct will be done while calling CloseKvStore in KvStoreDelegateManager)
    // so DO NOT free it in SingleKvStoreImpl's destructor.
    mutable std::shared_mutex storeNbDelegateMutex_{};
    DistributedDB::KvStoreNbDelegate *kvStoreNbDelegate_;
    std::mutex observerMapMutex_;
    std::map<IRemoteObject *, KvStoreObserverImpl *> observerMap_;
    std::mutex storeResultSetMutex_;
    std::map<IRemoteObject *, sptr<KvStoreResultSetImpl>> storeResultSetMap_;
    sptr<IKvStoreSyncCallback> syncCallback_;
    int openCount_;

    // flowControl
    KvStoreFlowCtrlManager flowCtrl_;
    static constexpr int BURST_CAPACITY = 1000;
    static constexpr int SUSTAINED_CAPACITY = 10000;
};
}  // namespace OHOS::DistributedKv
#endif  // SINGLE_KVSTORE_IMPL_H
