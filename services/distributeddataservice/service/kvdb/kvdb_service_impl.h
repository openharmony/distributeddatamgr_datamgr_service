/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_SERVICE_IMPL_H
#define OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_SERVICE_IMPL_H
#include <set>
#include <vector>

#include "concurrent_map.h"
#include "device_matrix.h"
#include "kv_store_delegate_manager.h"
#include "kv_store_nb_delegate.h"
#include "kvdb_notifier_proxy.h"
#include "kvdb_service_stub.h"
#include "kvdb_watcher.h"
#include "kvstore_sync_manager.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "metadata/strategy_meta_data.h"
#include "store/auto_cache.h"
#include "store/general_value.h"
#include "utils/ref_count.h"
namespace OHOS::DistributedKv {
class API_EXPORT KVDBServiceImpl final : public KVDBServiceStub {
public:
    using DBLaunchParam = DistributedDB::AutoLaunchParam;
    using Handler = std::function<void(int, std::map<std::string, std::vector<std::string>> &)>;
    using RefCount = DistributedData::RefCount;
    API_EXPORT KVDBServiceImpl();
    virtual ~KVDBServiceImpl();
    Status GetStoreIds(const AppId &appId, std::vector<StoreId> &storeIds) override;
    Status BeforeCreate(const AppId &appId, const StoreId &storeId, const Options &options) override;
    Status AfterCreate(const AppId &appId, const StoreId &storeId, const Options &options,
        const std::vector<uint8_t> &password) override;
    Status Delete(const AppId &appId, const StoreId &storeId) override;
    Status Close(const AppId &appId, const StoreId &storeId) override;
    Status CloudSync(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo) override;
    Status Sync(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo) override;
    Status SyncExt(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo) override;
    Status RegServiceNotifier(const AppId &appId, sptr<IKVDBNotifier> notifier) override;
    Status UnregServiceNotifier(const AppId &appId) override;
    Status SetSyncParam(const AppId &appId, const StoreId &storeId, const KvSyncParam &syncParam) override;
    Status GetSyncParam(const AppId &appId, const StoreId &storeId, KvSyncParam &syncParam) override;
    Status EnableCapability(const AppId &appId, const StoreId &storeId) override;
    Status DisableCapability(const AppId &appId, const StoreId &storeId) override;
    Status SetCapability(const AppId &appId, const StoreId &storeId, const std::vector<std::string> &local,
        const std::vector<std::string> &remote) override;
    Status AddSubscribeInfo(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo) override;
    Status RmvSubscribeInfo(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo) override;
    Status Subscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer) override;
    Status Unsubscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer) override;
    Status GetBackupPassword(const AppId &appId, const StoreId &storeId, std::vector<uint8_t> &password) override;
    Status NotifyDataChange(const AppId &appId, const StoreId &storeId) override;
    Status PutSwitch(const AppId &appId, const SwitchData &data) override;
    Status GetSwitch(const AppId &appId, const std::string &networkId, SwitchData &data) override;
    Status SubscribeSwitchData(const AppId &appId) override;
    Status UnsubscribeSwitchData(const AppId &appId) override;
    int32_t OnBind(const BindInfo &bindInfo) override;
    int32_t OnInitialize() override;
    int32_t OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &appId) override;
    int32_t ResolveAutoLaunch(const std::string &identifier, DBLaunchParam &param) override;
    int32_t OnUserChange(uint32_t code, const std::string &user, const std::string &account) override;
    int32_t Online(const std::string &device) override;
    int32_t OnReady(const std::string &device) override;
    int32_t OnSessionReady(const std::string &device) override;

private:
    using StoreMetaData = OHOS::DistributedData::StoreMetaData;
    using StrategyMeta = OHOS::DistributedData::StrategyMeta;
    using StoreMetaDataLocal = OHOS::DistributedData::StoreMetaDataLocal;
    using ChangeType = OHOS::DistributedData::DeviceMatrix::ChangeType;
    using DBStore = DistributedDB::KvStoreNbDelegate;
    using DBManager = DistributedDB::KvStoreDelegateManager;
    using SyncEnd = KvStoreSyncManager::SyncEnd;
    using DBResult = std::map<std::string, DistributedDB::DBStatus>;
    using DBStatus = DistributedDB::DBStatus;
    using DBMode = DistributedDB::SyncMode;
    using Action = OHOS::DistributedData::MetaDataManager::Action;
    enum SyncAction {
        ACTION_SYNC,
        ACTION_SUBSCRIBE,
        ACTION_UNSUBSCRIBE,
    };
    struct SyncAgent {
        pid_t pid_ = 0;
        int32_t switchesObserverCount_ = 0;
        bool changed_ = false;
        AppId appId_;
        int32_t count_ = 0;
        sptr<IKVDBNotifier> notifier_;
        std::map<std::string, uint32_t> delayTimes_;
        std::shared_ptr<KVDBWatcher> watcher_;
        std::set<sptr<KvStoreObserverProxy>> observers_;
        void ReInit(pid_t pid, const AppId &appId);
        void SetObserver(sptr<KvStoreObserverProxy> observer);
        void SetWatcher(std::shared_ptr<KVDBWatcher> watcher);
        void ClearObservers();
    };
    class Factory {
    public:
        Factory();
        ~Factory();
    private:
        std::shared_ptr<KVDBServiceImpl> product_;
    };

    void Init();
    void AddOptions(const Options &options, StoreMetaData &metaData);
    StoreMetaData GetStoreMetaData(const AppId &appId, const StoreId &storeId);
    StrategyMeta GetStrategyMeta(const AppId &appId, const StoreId &storeId);
    int32_t GetInstIndex(uint32_t tokenId, const AppId &appId);
    Status DoCloudSync(const StoreMetaData &meta, const SyncInfo &syncInfo);
    Status DoSync(const StoreMetaData &meta, const SyncInfo &info, const SyncEnd &complete, int32_t type);
    Status DoSyncInOrder(const StoreMetaData &meta, const SyncInfo &info, const SyncEnd &complete, int32_t type);
    Status DoSyncBegin(const std::vector<std::string> &devices, const StoreMetaData &meta,
        const SyncInfo &info, const SyncEnd &complete, int32_t type);
    Status DoComplete(const StoreMetaData &meta, const SyncInfo &info, RefCount refCount, const DBResult &dbResult);
    uint32_t GetSyncDelayTime(uint32_t delay, const StoreId &storeId);
    Status ConvertDbStatus(DBStatus status) const;
    DBMode ConvertDBMode(SyncMode syncMode) const;
    ChangeType ConvertType(SyncMode syncMode) const;
    SwitchState ConvertAction(Action action) const;
    SyncMode GetSyncMode(bool local, bool remote) const;
    std::vector<std::string> ConvertDevices(const std::vector<std::string> &deviceIds) const;
    DistributedData::GeneralStore::SyncMode ConvertGeneralSyncMode(SyncMode syncMode, SyncAction syncAction) const;
    DBResult HandleGenBriefDetails(const DistributedData::GenDetails &details);
    ProgressDetail HandleGenDetails(const DistributedData::GenDetails &details);
    void OnAsyncComplete(uint32_t tokenId, uint64_t seqNum, ProgressDetail &&detail);
    DistributedData::AutoCache::Watchers GetWatchers(uint32_t tokenId, const std::string &storeId);
    using SyncResult = std::pair<std::vector<std::string>, std::map<std::string, DBStatus>>;
    SyncResult ProcessResult(const std::map<std::string, int32_t> &results);
    void SaveLocalMetaData(const Options &options, const StoreMetaData &metaData);
    void RegisterKvServiceInfo();
    void RegisterHandler();
    void RegisterMatrixChange();
    void DumpKvServiceInfo(int fd, std::map<std::string, std::vector<std::string>> &params);
    void TryToSync(const StoreMetaData &metaData, bool force = false);
    void SyncOnSessionReady(const std::string &device);
    void OldOnlineSync(const StoreMetaData &data, const std::string &deviceId, RefCount refCount, uint16_t mask);
    bool IsRemoteChange(const StoreMetaData &metaData, const std::string &device);
    static Factory factory_;
    ConcurrentMap<uint32_t, SyncAgent> syncAgents_;
    std::shared_ptr<ExecutorPool> executors_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_SERVICE_IMPL_H
