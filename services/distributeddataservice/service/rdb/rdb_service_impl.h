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
#include "concurrent_map.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "rdb_notifier_proxy.h"
#include "rdb_syncer.h"
#include "rdb_watcher.h"
#include "store/auto_cache.h"
#include "store_observer.h"
#include "visibility.h"
namespace OHOS::DistributedRdb {
class API_EXPORT RdbServiceImpl : public RdbServiceStub {
public:
    using StoreMetaData = OHOS::DistributedData::StoreMetaData;
    using SecretKeyMetaData = DistributedData::SecretKeyMetaData;
    RdbServiceImpl();

    void OnClientDied(pid_t pid);

    /* IPC interface */
    std::string ObtainDistributedTableName(const std::string& device, const std::string& table) override;

    int32_t InitNotifier(const RdbSyncerParam &param, sptr<IRemoteObject> notifier) override;

    int32_t SetDistributedTables(const RdbSyncerParam &param, const std::vector<std::string> &tables,
        int32_t type = DISTRIBUTED_DEVICE) override;

    int32_t RemoteQuery(const RdbSyncerParam& param, const std::string& device, const std::string& sql,
                        const std::vector<std::string>& selectionArgs, sptr<IRemoteObject>& resultSet) override;

    int32_t Sync(const RdbSyncerParam &param, const Option &option, const PredicatesMemo &predicates,
        const AsyncDetail &async) override;

    int32_t Subscribe(const RdbSyncerParam &param, const SubscribeOption &option, RdbStoreObserver *observer) override;

    int32_t UnSubscribe(const RdbSyncerParam &param, const SubscribeOption &option,
        RdbStoreObserver *observer) override;

    void OnDataChange(pid_t pid, uint32_t tokenId, const DistributedDB::StoreChangedData& data);

    int32_t ResolveAutoLaunch(const std::string &identifier, DistributedDB::AutoLaunchParam &param) override;

    int32_t OnInitialize() override;

    int32_t OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &bundleName) override;

    int32_t GetSchema(const RdbSyncerParam &param) override;

    int32_t OnBind(const BindInfo &bindInfo) override;

private:
    using Watchers = DistributedData::AutoCache::Watchers;
    struct SyncAgent {
        pid_t pid_ = 0;
        int32_t count_ = 0;
        std::string bundleName_;
        sptr<RdbNotifierProxy> notifier_ = nullptr;
        std::shared_ptr<RdbWatcher> watcher_ = nullptr;
        void ReInit(pid_t pid, const std::string &bundleName);
        void SetNotifier(sptr<RdbNotifierProxy> notifier);
        void SetWatcher(std::shared_ptr<RdbWatcher> watcher);
    };

    class Factory {
    public:
        Factory();
        ~Factory();
    private:
        std::shared_ptr<RdbServiceImpl> product_;
    };
    using StoreSyncersType = std::map<std::string, std::shared_ptr<RdbSyncer>>;

    static constexpr int32_t MAX_SYNCER_NUM = 50;
    static constexpr int32_t MAX_SYNCER_PER_PROCESS = 10;
    static constexpr int32_t SYNCER_TIMEOUT = 60 * 1000; // ms

    std::pair<int32_t, Details> DoSync(const RdbSyncerParam &param, const Option &option, const PredicatesMemo &pred);

    int32_t DoAsync(const RdbSyncerParam &param, const Option &option, const PredicatesMemo &pred);

    Watchers GetWatchers(uint32_t tokenId, const std::string &storeName);

    std::string GenIdentifier(const RdbSyncerParam& param);

    bool CheckAccess(const std::string& bundleName, const std::string& storeName);

    void SyncerTimeout(std::shared_ptr<RdbSyncer> syncer);

    std::shared_ptr<RdbSyncer> GetRdbSyncer(const RdbSyncerParam& param);

    void OnAsyncComplete(uint32_t tokenId, uint32_t seqNum, Details&& result);

    int32_t CreateMetaData(const RdbSyncerParam &param, StoreMetaData &old);

    StoreMetaData GetStoreMetaData(const RdbSyncerParam &param);

    int32_t SetSecretKey(const RdbSyncerParam &param, const StoreMetaData &meta);

    int32_t Upgrade(const RdbSyncerParam &param, const StoreMetaData &old);

    static std::string TransferStringToHex(const std::string& origStr);

    static Factory factory_;
    int32_t syncerNum_ {};
    ConcurrentMap<pid_t, StoreSyncersType> syncers_;
    ConcurrentMap<std::string, std::pair<pid_t, uint32_t>> identifiers_;
    ConcurrentMap<uint32_t, SyncAgent> syncAgents_;
    RdbStoreObserverImpl autoLaunchObserver_;
    std::shared_ptr<ExecutorPool> executors_;
};
} // namespace OHOS::DistributedRdb
#endif
