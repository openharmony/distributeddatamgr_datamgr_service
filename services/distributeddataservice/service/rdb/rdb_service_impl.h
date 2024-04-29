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

#include "rdb_service_stub.h"

#include <map>
#include <mutex>
#include <string>
#include "cloud/cloud_event.h"
#include "concurrent_map.h"
#include "feature/static_acts.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "rdb_notifier_proxy.h"
#include "rdb_watcher.h"
#include "rdb_query.h"
#include "store/auto_cache.h"
#include "store/general_store.h"
#include "store_observer.h"
#include "visibility.h"
#include "store/general_value.h"
#include "snapshot/bind_event.h"

namespace OHOS::DistributedRdb {
class API_EXPORT RdbServiceImpl : public RdbServiceStub {
public:
    using StoreMetaData = OHOS::DistributedData::StoreMetaData;
    using SecretKeyMetaData = DistributedData::SecretKeyMetaData;
    using DetailAsync = DistributedData::GeneralStore::DetailAsync;
    using Handler = std::function<void(int, std::map<std::string, std::vector<std::string>> &)>;
    using StoreInfo = DistributedData::StoreInfo;
    RdbServiceImpl();
    virtual ~RdbServiceImpl();

    /* IPC interface */
    std::string ObtainDistributedTableName(const std::string& device, const std::string& table) override;

    int32_t InitNotifier(const RdbSyncerParam &param, sptr<IRemoteObject> notifier) override;

    int32_t SetDistributedTables(const RdbSyncerParam &param, const std::vector<std::string> &tables,
        const std::vector<Reference> &references, int32_t type = DISTRIBUTED_DEVICE) override;

    std::pair<int32_t, std::shared_ptr<ResultSet>> RemoteQuery(const RdbSyncerParam& param, const std::string& device,
        const std::string& sql, const std::vector<std::string>& selectionArgs) override;

    int32_t Sync(const RdbSyncerParam &param, const Option &option, const PredicatesMemo &predicates,
        const AsyncDetail &async) override;

    int32_t Subscribe(const RdbSyncerParam &param, const SubscribeOption &option, RdbStoreObserver *observer) override;

    int32_t UnSubscribe(const RdbSyncerParam &param, const SubscribeOption &option,
        RdbStoreObserver *observer) override;

    int32_t RegisterAutoSyncCallback(const RdbSyncerParam& param,
        std::shared_ptr<DetailProgressObserver> observer) override;

    int32_t UnregisterAutoSyncCallback(const RdbSyncerParam& param,
        std::shared_ptr<DetailProgressObserver> observer) override;

    int32_t ResolveAutoLaunch(const std::string &identifier, DistributedDB::AutoLaunchParam &param) override;

    int32_t OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &bundleName) override;

    int32_t Delete(const RdbSyncerParam &param) override;

    std::pair<int32_t, std::shared_ptr<ResultSet>> QuerySharingResource(const RdbSyncerParam& param,
        const PredicatesMemo& predicates, const std::vector<std::string>& columns) override;

    int32_t OnBind(const BindInfo &bindInfo) override;

    int32_t OnInitialize() override;

    int32_t NotifyDataChange(const RdbSyncerParam &param, const RdbChangedData &rdbChangedData) override;
    int32_t Disable(const RdbSyncerParam& param) override;
    int32_t Enable(const RdbSyncerParam& param) override;

    int32_t BeforeOpen(RdbSyncerParam &param) override;

    int32_t AfterOpen(const RdbSyncerParam &param) override;

private:
    using Watchers = DistributedData::AutoCache::Watchers;
    using StaticActs = DistributedData::StaticActs;
    struct SyncAgent {
        pid_t pid_ = 0;
        int32_t count_ = 0;
        std::map<std::string, int> callBackStores_;
        std::string bundleName_;
        sptr<RdbNotifierProxy> notifier_ = nullptr;
        std::shared_ptr<RdbWatcher> watcher_ = nullptr;
        void ReInit(pid_t pid, const std::string &bundleName);
        void SetNotifier(sptr<RdbNotifierProxy> notifier);
        void SetWatcher(std::shared_ptr<RdbWatcher> watcher);
    };

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

    void DumpRdbServiceInfo(int fd, std::map<std::string, std::vector<std::string>> &params);

    void DoCloudSync(const RdbSyncerParam &param, const Option &option, const PredicatesMemo &predicates,
        const AsyncDetail &async);

    void DoCompensateSync(const DistributedData::BindEvent& event);

    int DoSync(const RdbSyncerParam &param, const Option &option, const PredicatesMemo &predicates,
        const AsyncDetail &async);

    Watchers GetWatchers(uint32_t tokenId, const std::string &storeName);

    DetailAsync GetCallbacks(uint32_t tokenId, const std::string &storeName);

    bool CheckAccess(const std::string& bundleName, const std::string& storeName);

    std::shared_ptr<DistributedData::GeneralStore> GetStore(const RdbSyncerParam& param);

    void OnAsyncComplete(uint32_t tokenId, uint32_t seqNum, Details&& result);

    StoreMetaData GetStoreMetaData(const RdbSyncerParam &param);

    int32_t SetSecretKey(const RdbSyncerParam &param, const StoreMetaData &meta);

    int32_t Upgrade(const RdbSyncerParam &param, const StoreMetaData &old);

    std::pair<int32_t, std::shared_ptr<DistributedData::Cursor>> AllocResource(
        StoreInfo& storeInfo, std::shared_ptr<RdbQuery> rdbQuery);

    static Details HandleGenDetails(const DistributedData::GenDetails &details);

    static std::string TransferStringToHex(const std::string& origStr);

    static std::string RemoveSuffix(const std::string& name);

    static std::pair<int32_t, int32_t> GetInstIndexAndUser(uint32_t tokenId, const std::string &bundleName);

    static bool GetPassword(const StoreMetaData &metaData, DistributedDB::CipherPassword &password);

    void GetCloudSchema(const RdbSyncerParam &param);

    void SetReturnParam(StoreMetaData &metadata, RdbSyncerParam &param);

    static Factory factory_;
    ConcurrentMap<uint32_t, SyncAgent> syncAgents_;
    std::shared_ptr<ExecutorPool> executors_;
};
} // namespace OHOS::DistributedRdb
#endif
