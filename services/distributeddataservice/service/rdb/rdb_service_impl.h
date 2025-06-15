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

#include "cloud/cloud_event.h"
#include "commonevent/data_change_event.h"
#include "commonevent/set_searchable_event.h"
#include "concurrent_map.h"
#include "feature/static_acts.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
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
#include "process_communicator_impl.h"

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

    int32_t AfterOpen(const RdbSyncerParam &param) override;

    int32_t ReportStatistic(const RdbSyncerParam &param, const RdbStatEvent &statEvent) override;

    int32_t GetPassword(const RdbSyncerParam &param, std::vector<std::vector<uint8_t>> &password) override;

    std::pair<int32_t, uint32_t> LockCloudContainer(const RdbSyncerParam &param) override;

    int32_t UnlockCloudContainer(const RdbSyncerParam &param) override;

    int32_t GetDebugInfo(const RdbSyncerParam &param, std::map<std::string, RdbDebugInfo> &debugInfo) override;

    int32_t GetDfxInfo(const RdbSyncerParam &param, DistributedRdb::RdbDfxInfo &dfxInfo) override;

    int32_t VerifyPromiseInfo(const RdbSyncerParam &param) override;

private:
    using Watchers = DistributedData::AutoCache::Watchers;
    using StaticActs = DistributedData::StaticActs;
    using DBStatus = DistributedDB::DBStatus;
    using SyncResult = std::pair<std::vector<std::string>, std::map<std::string, DBStatus>>;
    using AutoCache = DistributedData::AutoCache;
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

    class RdbStatic : public StaticActs {
    public:
        ~RdbStatic() override {};
        int32_t OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index) override;
        int32_t OnAppUpdate(const std::string &bundleName, int32_t user, int32_t index) override;
        int32_t OnClearAppStorage(const std::string &bundleName, int32_t user, int32_t index, int32_t tokenId) override;
    private:
        static constexpr inline int32_t INVALID_TOKENID = 0;
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

    static constexpr inline uint32_t WAIT_TIME = 30 * 1000;
    static constexpr inline uint32_t SHARE_WAIT_TIME = 60; // seconds

    void RegisterRdbServiceInfo();

    void RegisterHandler();

    void RegisterEvent();

    void DumpRdbServiceInfo(int fd, std::map<std::string, std::vector<std::string>> &params);

    void DoCloudSync(const RdbSyncerParam &param, const Option &option, const PredicatesMemo &predicates,
        const AsyncDetail &async);

    void DoCompensateSync(const DistributedData::BindEvent& event);

    int DoSync(const RdbSyncerParam &param, const Option &option, const PredicatesMemo &predicates,
        const AsyncDetail &async);

    int DoAutoSync(
        const std::vector<std::string> &devices, const Database &dataBase, std::vector<std::string> tableNames);

    std::vector<std::string> GetReuseDevice(const std::vector<std::string> &devices);
    int DoOnlineSync(const std::vector<std::string> &devices, const Database &dataBase);

    int DoDataChangeSync(const StoreInfo &storeInfo, const RdbChangedData &rdbChangedData);

    Watchers GetWatchers(uint32_t tokenId, const std::string &storeName);

    DetailAsync GetCallbacks(uint32_t tokenId, const std::string &storeName);

    std::shared_ptr<DistributedData::GeneralStore> GetStore(const RdbSyncerParam& param);

    std::shared_ptr<DistributedData::GeneralStore> GetStore(const StoreMetaData &storeMetaData);

    void OnAsyncComplete(uint32_t tokenId, pid_t pid, uint32_t seqNum, Details &&result);

    int32_t Upgrade(const RdbSyncerParam &param, const StoreMetaData &old);

    void GetSchema(const RdbSyncerParam &param);

    bool IsPostImmediately(const int32_t callingPid, const RdbNotifyConfig &rdbNotifyConfig, StoreInfo &storeInfo,
        DistributedData::DataChangeEvent::EventInfo &eventInfo, const std::string &storeName);

    bool TryUpdateDeviceId(const RdbSyncerParam &param, const StoreMetaData &oldMeta, StoreMetaData &meta);

    void SaveLaunchInfo(StoreMetaData &meta);

    static bool CheckAccess(const std::string& bundleName, const std::string& storeName);

    static bool CheckInvalidPath(const std::string& param);

    static bool CheckCustomDir(const std::string &customDir, int32_t upLimit);

    static bool CheckParam(const RdbSyncerParam &param);

    static StoreMetaData GetStoreMetaData(const RdbSyncerParam &param);

    static std::string GetPath(const RdbSyncerParam &param);

    static StoreMetaData GetStoreMetaData(const Database &dataBase);

    static int32_t SaveSecretKey(const RdbSyncerParam &param, const StoreMetaData &meta);

    static std::pair<int32_t, std::shared_ptr<DistributedData::Cursor>> AllocResource(
        StoreInfo& storeInfo, std::shared_ptr<RdbQuery> rdbQuery);

    static Details HandleGenDetails(const DistributedData::GenDetails &details);

    static std::string TransferStringToHex(const std::string& origStr);

    static std::string RemoveSuffix(const std::string& name);

    static std::pair<int32_t, int32_t> GetInstIndexAndUser(uint32_t tokenId, const std::string &bundleName);

    static std::string GetSubUser(const int32_t subUser);

    static bool GetDBPassword(const StoreMetaData &metaData, DistributedDB::CipherPassword &password);
    
    static bool SaveAppIDMeta(const StoreMetaData &meta, const StoreMetaData &old);

    static void SetReturnParam(const StoreMetaData &metadata, RdbSyncerParam &param);

    static bool IsNeedMetaSync(const StoreMetaData &meta, const std::vector<std::string> &uuids);

    static SyncResult ProcessResult(const std::map<std::string, int32_t> &results);

    static StoreInfo GetStoreInfo(const RdbSyncerParam &param);

    static int32_t SaveDebugInfo(const StoreMetaData &metaData, const RdbSyncerParam &param);

    static int32_t SaveDfxInfo(const StoreMetaData &metaData, const RdbSyncerParam &param);

    static int32_t SavePromiseInfo(const StoreMetaData &metaData, const RdbSyncerParam &param);

    static int32_t PostSearchEvent(int32_t evtId, const RdbSyncerParam& param,
        DistributedData::SetSearchableEvent::EventInfo &eventInfo);

    static void UpdateMeta(const StoreMetaData &meta, const StoreMetaData &localMeta, AutoCache::Store store);

    static bool UpgradeCloneSecretKey(const StoreMetaData &meta);

    static Factory factory_;
    ConcurrentMap<uint32_t, SyncAgents> syncAgents_;
    std::shared_ptr<ExecutorPool> executors_;
    ConcurrentMap<int32_t, std::map<std::string, ExecutorPool::TaskId>> heartbeatTaskIds_;
};
} // namespace OHOS::DistributedRdb
#endif