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

#ifndef DISTRIBUTEDDATASERVICE_RDB_SERVICE_H
#define DISTRIBUTEDDATASERVICE_RDB_SERVICE_H

#include <map>
#include <mutex>
#include <string>
#include <variant>

#include "cloud/cloud_event.h"
#include "commonevent/data_change_event.h"
#include "commonevent/set_searchable_event.h"
#include "concurrent_map.h"
#include "crypto/crypto_manager.h"
#include "feature/static_acts.h"
#include "rdb_flow_control_manager.h"
#include "lru_bucket.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "process_communicator_impl.h"
#include "rdb_notifier_proxy.h"
#include "rdb_query.h"
#include "rdb_service_stub.h"
#include "rdb_watcher.h"
#include "snapshot/bind_event.h"
#include "store/auto_cache.h"
#include "store/general_store.h"
#include "store/general_value.h"
#include "store_observer.h"
#include "visibility.h"

namespace OHOS::DistributedRdb {
using namespace OHOS::AppDistributedKv;
class RdbServiceImpl : public RdbServiceStub {
public:
    using StoreMetaData = OHOS::DistributedData::StoreMetaData;
    using SecretKeyMetaData = DistributedData::SecretKeyMetaData;
    using DetailAsync = DistributedData::GeneralStore::DetailAsync;
    using Database = DistributedData::Database;
    using Handler = std::function<void(int, std::map<std::string, std::vector<std::string>> &)>;
    using StoreInfo = DistributedData::StoreInfo;
    RdbServiceImpl();
    virtual ~RdbServiceImpl();

    /* IPC interface */
    std::string ObtainDistributedTableName(const RdbSyncerParam &param, const std::string &device,
        const std::string &table) override;

    int32_t InitNotifier(const RdbSyncerParam &param, sptr<IRemoteObject> notifier) override;

    int32_t SetDistributedTables(const RdbSyncerParam &param, const std::vector<std::string> &tables,
        const std::vector<Reference> &references, bool isRebuild, int32_t type = DISTRIBUTED_DEVICE) override;

    std::pair<int32_t, std::shared_ptr<ResultSet>> RemoteQuery(const RdbSyncerParam& param, const std::string& device,
        const std::string& sql, const std::vector<std::string>& selectionArgs) override;

    int32_t Sync(const RdbSyncerParam &param, const Option &option, const PredicatesMemo &predicates,
        const AsyncDetail &async) override;

    int32_t Subscribe(const RdbSyncerParam &param,
                      const SubscribeOption &option,
                      std::shared_ptr<RdbStoreObserver> observer) override;

    int32_t UnSubscribe(const RdbSyncerParam &param, const SubscribeOption &option,
        std::shared_ptr<RdbStoreObserver>observer) override;

    int32_t RegisterAutoSyncCallback(const RdbSyncerParam& param,
        std::shared_ptr<DetailProgressObserver> observer) override;

    int32_t UnregisterAutoSyncCallback(const RdbSyncerParam& param,
        std::shared_ptr<DetailProgressObserver> observer) override;

    int32_t ResolveAutoLaunch(const std::string &identifier, DistributedDB::AutoLaunchParam &param) override;

    int32_t OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &bundleName) override;

    int32_t OnFeatureExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &bundleName) override;

    int32_t Delete(const RdbSyncerParam &param) override;

    std::pair<int32_t, std::shared_ptr<ResultSet>> QuerySharingResource(const RdbSyncerParam& param,
        const PredicatesMemo& predicates, const std::vector<std::string>& columns) override;

    int32_t OnBind(const BindInfo &bindInfo) override;

    int32_t OnReady(const std::string &device) override;

    int32_t OnInitialize() override;

    int32_t NotifyDataChange(const RdbSyncerParam &param, const RdbChangedData &rdbChangedData,
        const RdbNotifyConfig &rdbNotifyConfig) override;
    int32_t SetSearchable(const RdbSyncerParam& param, bool isSearchable) override;
    int32_t Disable(const RdbSyncerParam& param) override;
    int32_t Enable(const RdbSyncerParam& param) override;

    int32_t BeforeOpen(RdbSyncerParam &param) override;

    std::pair<int32_t, bool> IsSupportSilent(const RdbSyncerParam &param) override;

    int32_t AfterOpen(const RdbSyncerParam &param) override;

    int32_t ReportStatistic(const RdbSyncerParam &param, const RdbStatEvent &statEvent) override;

    int32_t GetPassword(const RdbSyncerParam &param, std::vector<std::vector<uint8_t>> &password) override;

    std::pair<int32_t, uint32_t> LockCloudContainer(const RdbSyncerParam &param) override;

    int32_t UnlockCloudContainer(const RdbSyncerParam &param) override;

    int32_t GetDebugInfo(const RdbSyncerParam &param, std::map<std::string, RdbDebugInfo> &debugInfo) override;

    int32_t GetDfxInfo(const RdbSyncerParam &param, DistributedRdb::RdbDfxInfo &dfxInfo) override;

    int32_t VerifyPromiseInfo(const RdbSyncerParam &param) override;

    int32_t StopCloudSync(const RdbSyncerParam &param) override;

private:
    using Watchers = DistributedData::AutoCache::Watchers;
    using StaticActs = DistributedData::StaticActs;
    using DBStatus = DistributedDB::DBStatus;
    using SyncResult = std::pair<std::vector<std::string>, std::map<std::string, DBStatus>>;
    using AutoCache = DistributedData::AutoCache;
    using CryptoManager = DistributedData::CryptoManager;
    using FlowControlManager = OHOS::DistributedData::FlowControlManager;
    struct SyncAgent {
        SyncAgent() = default;
        explicit SyncAgent(const std::string &bundleName);
        int32_t count_ = 0;
        std::map<std::string, int> callBackStores_;
        std::string bundleName_;
        sptr<RdbNotifierProxy> notifier_ = nullptr;
        std::shared_ptr<RdbWatcher> watcher_ = nullptr;
        void SetNotifier(sptr<RdbNotifierProxy> notifier);
        void SetWatcher(std::shared_ptr<RdbWatcher> watcher);
    };
    using SyncAgents = std::map<int32_t, SyncAgent>;

    struct GlobalEvent {
        void AddEvent(const std::string& path, const DistributedData::DataChangeEvent::EventInfo& eventInfo);
        std::optional<DistributedData::DataChangeEvent::EventInfo> StealEvent(const std::string& path);
    private:
        std::mutex mutex;
        std::map<std::string, DistributedData::DataChangeEvent::EventInfo> events_;
    };

    class RdbStatic : public StaticActs {
    public:
        ~RdbStatic() override {};
        int32_t OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index,
            int32_t tokenId = -1) override;
        int32_t OnAppUpdate(const std::string &bundleName, int32_t user, int32_t index) override;
        int32_t OnClearAppStorage(const std::string &bundleName, int32_t user, int32_t index, int32_t tokenId) override;
    private:
        static constexpr int32_t INVALID_TOKENID = 0;
        int32_t CloseStore(const std::string &bundleName, int32_t user, int32_t index,
            int32_t tokenId = INVALID_TOKENID) const;
    };

    class Factory {
    public:
        Factory();
        ~Factory();
    private:
        std::shared_ptr<RdbServiceImpl> product_;
        std::shared_ptr<RdbStatic> staticActs_;
    };

    static constexpr uint32_t WAIT_TIME = 30 * 1000;
    static constexpr uint32_t SHARE_WAIT_TIME = 60; // seconds
    static constexpr uint32_t SAVE_CHANNEL_INTERVAL = 5; // minutes
    static constexpr uint32_t SYNC_DURATION = 60 * 1000; // 1min
    static constexpr uint32_t SYNC_APP_LIMIT_TIMES = 5;
    static constexpr uint32_t SYNC_GLOBAL_LIMIT_TIMES = 20;

    void RegisterRdbServiceInfo();

    void RegisterHandler();

    void RegisterEvent();

    void DumpRdbServiceInfo(int fd, std::map<std::string, std::vector<std::string>> &params);

    void DoCloudSync(const StoreMetaData &metaData, const Option &option, const PredicatesMemo &predicates,
        const AsyncDetail &async);

    void DoCompensateSync(const DistributedData::BindEvent& event);
    std::function<int()> GetSyncTask(const StoreMetaData &metaData, const RdbService::Option &option,
        const PredicatesMemo &predicates, const AsyncDetail &async);
    int DoSync(const StoreMetaData &meta, const RdbService::Option &option, const PredicatesMemo &predicates,
        const AsyncDetail &async);
    bool IsSyncLimitApp(const StoreMetaData &meta);

    int DoAutoSync(const std::vector<std::string> &devices, const StoreMetaData &metaData,
        const std::vector<std::string> &tables);

    bool IsSupportAutoSyncDeviceType(const std::string &localDeviceId, const std::string &remoteDeviceId);
    
    std::vector<std::string> GetReuseDevice(const std::vector<std::string> &devices, const StoreMetaData &metaData);

    void OnCollaborationChange(const StoreMetaData &metaData, const RdbChangedData &changedData);

    void OnSearchableChange(const StoreMetaData &metaData, const RdbNotifyConfig &config,
        const RdbChangedData &changedData);
    int32_t SetDeviceDistributedTables(int32_t tableType, StoreMetaData &metaData,
        std::shared_ptr<DistributedData::GeneralStore> store);
    void SetCloudDistributedTables(const RdbSyncerParam &param, StoreMetaData &metaData);

    Watchers GetWatchers(uint32_t tokenId, const std::string &storeName);

    DetailAsync GetCallbacks(uint32_t tokenId, const std::string &storeName);

    std::shared_ptr<DistributedData::GeneralStore> GetStore(const StoreMetaData &storeMetaData);

    void OnAsyncComplete(uint32_t tokenId, pid_t pid, uint32_t seqNum, Details &&result);

    int32_t Upgrade(const StoreMetaData &metaData, const StoreMetaData &old);

    void GetCloudSchema(const StoreMetaData &metaData);

    void PostHeartbeatTask(int32_t pid, uint32_t delay, StoreInfo &storeInfo,
        DistributedData::DataChangeEvent::EventInfo &eventInfo);

    void RemoveHeartbeatTask(int32_t pid, const std::string &path);

    bool TryUpdateDeviceId(const StoreMetaData &oldMeta, StoreMetaData &meta);

    void SaveLaunchInfo(StoreMetaData &meta);

    void SaveAutoSyncInfo(const StoreMetaData &meta, const std::vector<std::string> &devices);

    void DoChannelsMemento(bool immediately = false);

    bool IsSpecialChannel(const std::string &device);

    static bool IsValidAccess(const std::string& bundleName, const std::string& storeName);

    static bool IsValidPath(const std::string& param);

    static bool IsValidCustomDir(const std::string &customDir, int32_t upLimit);

    static bool IsValidParam(const RdbSyncerParam &param);

    static StoreMetaData GetStoreMetaData(const RdbSyncerParam &param);

    static std::pair<bool, StoreMetaData> LoadStoreMetaData(const RdbSyncerParam &param);

    static void SaveSyncMeta(const StoreMetaData &meta);

    static std::pair<bool, StoreMetaData> LoadSyncMeta(const Database &database);

    static std::pair<int32_t, std::shared_ptr<DistributedData::Cursor>> AllocResource(
        StoreInfo& storeInfo, std::shared_ptr<RdbQuery> rdbQuery);

    static Details HandleGenDetails(const DistributedData::GenDetails &details);

    static std::string TransferStringToHex(const std::string& origStr);

    static std::string RemoveSuffix(const std::string& name);

    static std::pair<int32_t, int32_t> GetInstIndexAndUser(uint32_t tokenId, const std::string &bundleName);

    static std::string GetSubUser(int32_t subUser);

    static void SetReturnParam(const StoreMetaData &metadata, RdbSyncerParam &param);

    static bool IsNeedMetaSync(const StoreMetaData &meta, const std::vector<std::string> &uuids);

    static SyncResult ProcessResult(const std::map<std::string, int32_t> &results);

    static StoreInfo GetStoreInfoEx(const StoreMetaData &metaData);

    static int32_t SaveDebugInfo(const StoreMetaData &metaData, const RdbSyncerParam &param);

    static int32_t SaveDfxInfo(const StoreMetaData &metaData, const RdbSyncerParam &param);

    static int32_t SavePromiseInfo(const StoreMetaData &metaData, const RdbSyncerParam &param);

    static bool SaveAppIDMeta(const StoreMetaData &meta, const StoreMetaData &old);

    static int32_t PostSearchEvent(int32_t evtId, const StoreMetaData &param,
        DistributedData::SetSearchableEvent::EventInfo &eventInfo);

    static bool IsCollaboration(const StoreMetaData &metaData);

    std::vector<uint8_t> LoadSecretKey(const StoreMetaData &metaData, CryptoManager::SecretKeyType secretKeyType);

    void SaveSecretKeyMeta(const StoreMetaData &metaData, const std::vector<uint8_t> &password);

    static Factory factory_;
    ConcurrentMap<uint32_t, SyncAgents> syncAgents_;
    static std::shared_ptr<RdbFlowControlManager> rdbFlowControlManager_;
    std::shared_ptr<ExecutorPool> executors_;
    std::shared_ptr<GlobalEvent> eventContainer_;
    ConcurrentMap<int32_t, std::map<std::string, ExecutorPool::TaskId>> heartbeatTaskIds_;

    LRUBucket<std::string, std::monostate> specialChannels_ { 10 };
    ExecutorPool::TaskId saveChannelsTask_ = ExecutorPool::INVALID_TASK_ID;
};
} // namespace OHOS::DistributedRdb
#endif